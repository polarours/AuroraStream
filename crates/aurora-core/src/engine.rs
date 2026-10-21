use std::sync::Arc;
use std::time::Duration;

use crate::config::Config;
use crate::error::Result;
use crate::event_bus::{Event, EventBus};
use crate::http_gateway::HttpGateway;
use crate::memory_pool::MemoryPool;
use crate::plugin::{LogLevel, Plugin, PluginContext};
use crate::plugin_loader::PluginLoader;
use crate::stats::ServerStats;

pub struct Engine {
    config: Config,
    event_bus: Arc<EventBus>,
    memory_pool: Arc<MemoryPool>,
    stats: Arc<ServerStats>,
    plugins: Vec<Box<dyn Plugin>>,
    plugin_loader: PluginLoader,
    gateway: HttpGateway,
}

impl Engine {
    pub fn new(config: Config) -> Self {
        let event_bus = Arc::new(EventBus::new());
        let memory_pool = Arc::new(MemoryPool::default());
        let stats = Arc::new(ServerStats::new());
        let gateway = HttpGateway::new(config.clone(), Arc::clone(&stats));

        Self {
            config,
            event_bus,
            memory_pool,
            stats,
            plugins: Vec::new(),
            plugin_loader: PluginLoader::new(),
            gateway,
        }
    }

    pub async fn run(&mut self) -> Result<()> {
        self.init_logging();
        self.print_banner();

        self.load_plugins()?;
        self.start_plugins()?;
        self.start_gateway().await?;
        self.run_event_loop().await?;
        self.stop_plugins().await;

        Ok(())
    }

    fn init_logging(&self) {
        let filter = tracing_subscriber::EnvFilter::try_from_default_env()
            .unwrap_or_else(|_| tracing_subscriber::EnvFilter::new(&self.config.logging.level));

        tracing_subscriber::fmt().with_env_filter(filter).init();
    }

    fn print_banner(&self) {
        tracing::info!("╔══════════════════════════════════════╗");
        tracing::info!("║      AuroraStream v0.1.0             ║");
        tracing::info!("║  Lightweight Plugin-based Streaming  ║");
        tracing::info!("╚══════════════════════════════════════╝");
        tracing::info!("HTTP port: {}", self.config.server.http_port);
        tracing::info!("RTMP port: {}", self.config.server.rtmp_port);
    }

    /// Builds the context handed to the plugins.
    ///
    /// The logging sink routes plugin messages into the host subscriber: a plugin is a
    /// separate shared object with its own copy of `tracing`, so its own subscriber would
    /// never be seen by the host.
    fn plugin_context(&self) -> PluginContext {
        PluginContext {
            event_bus: Arc::clone(&self.event_bus),
            memory_pool: Arc::clone(&self.memory_pool),
            config: self.config.clone(),
            log: Arc::new(|level, message| match level {
                LogLevel::Error => tracing::error!(target: "plugin", "{}", message),
                LogLevel::Warn => tracing::warn!(target: "plugin", "{}", message),
                LogLevel::Info => tracing::info!(target: "plugin", "{}", message),
                LogLevel::Debug => tracing::debug!(target: "plugin", "{}", message),
                LogLevel::Trace => tracing::trace!(target: "plugin", "{}", message),
            }),
        }
    }

    fn load_plugins(&mut self) -> Result<()> {
        if !self.config.plugins.auto_load {
            tracing::info!("auto-load disabled, skipping plugin loading");
            return Ok(());
        }

        let dir = &self.config.plugins.directory;
        tracing::info!("loading plugins from: {}", dir);

        let mut plugins = self.plugin_loader.load_from_dir(dir)?;
        tracing::info!("loaded {} plugin(s)", plugins.len());

        let ctx = self.plugin_context();

        for plugin in &mut plugins {
            plugin.on_load(&ctx)?;

            let routes = plugin.routes();
            self.gateway.register_plugin_routes(
                plugin.name(),
                routes
                    .iter()
                    .map(|r| format!("{} {}", r.method, r.path))
                    .collect(),
            );
            for route in routes {
                self.gateway
                    .register_plugin_handler(&route.path, route.handler);
            }
        }

        self.plugins = plugins;
        Ok(())
    }

    fn start_plugins(&mut self) -> Result<()> {
        for plugin in &mut self.plugins {
            plugin.on_start()?;
            tracing::info!("started plugin: {}", plugin.name());

            self.event_bus
                .publish_all(crate::event_bus::Event::PluginLoaded {
                    plugin_name: plugin.name().to_string(),
                });
        }
        Ok(())
    }

    async fn start_gateway(&self) -> Result<()> {
        // Bind first: a port clash has to abort the startup instead of leaving the
        // engine running without an API.
        let listener = self.gateway.bind().await?;
        let gateway = self.gateway.clone();
        tokio::spawn(async move {
            if let Err(e) = gateway.serve(listener).await {
                tracing::error!("HTTP gateway error: {}", e);
            }
        });
        Ok(())
    }

    async fn run_event_loop(&mut self) -> Result<()> {
        tracing::info!("engine running, press Ctrl+C to stop");

        let mut ticker = tokio::time::interval(Duration::from_millis(100));
        ticker.set_missed_tick_behavior(tokio::time::MissedTickBehavior::Delay);

        // Resolves when the process is asked to terminate, so `systemctl stop` and
        // `docker stop` shut the engine down cleanly instead of killing it.
        let mut sigterm = sigterm_future();

        loop {
            tokio::select! {
                result = tokio::signal::ctrl_c() => {
                    result?;
                    tracing::info!("shutdown signal received");
                    break;
                }
                result = &mut sigterm => {
                    result?;
                    tracing::info!("SIGTERM received");
                    break;
                }
                _ = ticker.tick() => {
                    self.dispatch_events();
                }
            }
        }

        Ok(())
    }

    /// Hands every queued event to the plugins and updates the server statistics.
    fn dispatch_events(&mut self) {
        for event in self.event_bus.drain() {
            match &event {
                Event::StreamPublish {
                    stream_key,
                    codec_info,
                } => {
                    let is_new = !self.stats.has_stream(stream_key);
                    self.stats
                        .stream_started(stream_key, codec_info.clone());
                    if is_new {
                        tracing::info!(
                            "stream published: {} (video={:?}, audio={:?})",
                            stream_key,
                            codec_info.video_codec,
                            codec_info.audio_codec
                        );
                    } else {
                        // The publisher reports the codecs again once the first media
                        // packet revealed them.
                        tracing::debug!(
                            "stream {} codecs: video={:?}, audio={:?}",
                            stream_key,
                            codec_info.video_codec,
                            codec_info.audio_codec
                        );
                    }
                }
                Event::ConnectionOpened { peer } => {
                    tracing::debug!("connection opened: {}", peer);
                    self.stats.connection_opened();
                }
                Event::ConnectionClosed { peer } => {
                    tracing::debug!("connection closed: {}", peer);
                    self.stats.connection_closed();
                }
                Event::StreamPlay { stream_key } => {
                    tracing::info!("stream play: {}", stream_key);
                }
                Event::StreamStop { stream_key } => {
                    tracing::info!("stream stopped: {}", stream_key);
                    self.stats.stream_stopped(stream_key);
                }
                Event::EncodedVideo {
                    stream_key,
                    data,
                    keyframe,
                    ..
                } => {
                    tracing::debug!(
                        "video frame on {}: {} bytes (keyframe={})",
                        stream_key,
                        data.len(),
                        keyframe
                    );
                }
                Event::EncodedAudio {
                    stream_key,
                    data,
                    ..
                } => {
                    tracing::debug!("audio frame on {}: {} bytes", stream_key, data.len());
                }
                _ => {}
            }

            for plugin in &mut self.plugins {
                if let Err(e) = plugin.on_event(&event) {
                    tracing::error!(
                        "plugin {} failed to handle event: {}",
                        plugin.name(),
                        e
                    );
                }
            }
        }
    }

    async fn stop_plugins(&mut self) {
        for plugin in &mut self.plugins {
            if let Err(e) = plugin.on_stop() {
                tracing::error!("error stopping plugin {}: {}", plugin.name(), e);
            }
            if let Err(e) = plugin.on_unload() {
                tracing::error!("error unloading plugin {}: {}", plugin.name(), e);
            }
            tracing::info!("stopped plugin: {}", plugin.name());
        }
        // The plugin handlers are backed by code inside the plugin shared objects.
        self.gateway.clear_plugin_handlers();
        tracing::info!("all plugins stopped");
    }
}

#[cfg(unix)]
async fn wait_for_sigterm() -> Result<()> {
    use tokio::signal::unix::{signal, SignalKind};

    let mut sigterm = signal(SignalKind::terminate())?;
    sigterm.recv().await;
    Ok(())
}

/// A future that resolves once the process is asked to terminate.
///
/// On platforms without SIGTERM it never resolves.
fn sigterm_future() -> std::pin::Pin<Box<dyn std::future::Future<Output = Result<()>> + Send>> {
    #[cfg(unix)]
    {
        Box::pin(wait_for_sigterm())
    }
    #[cfg(not(unix))]
    {
        Box::pin(async {
            std::future::pending::<()>().await;
            Ok(())
        })
    }
}
