use std::sync::Arc;

use tokio::net::TcpListener;

use aurora_core::error::{AuroraError, Result};
use aurora_core::event_bus::Event;
use aurora_core::plugin::{HttpRoute, Plugin, PluginContext};

mod rtmp_amf;
mod rtmp_chunk;
mod rtmp_handshake;
mod rtmp_session;

pub use rtmp_amf::AmfValue;
pub use rtmp_chunk::{ChunkHeader, ChunkMessage};
pub use rtmp_handshake::handshake;
pub use rtmp_session::{RtmpSession, SessionInfo, SessionState};

pub struct RtmpPlugin {
    ctx: Option<PluginContext>,
    next_session_id: Arc<parking_lot::RwLock<u32>>,
    sessions: Arc<parking_lot::RwLock<Vec<SessionInfo>>>,
    stop_tx: Option<tokio::sync::oneshot::Sender<()>>,
    server_thread: Option<std::thread::JoinHandle<()>>,
}

impl std::fmt::Debug for RtmpPlugin {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("RtmpPlugin").finish()
    }
}

impl RtmpPlugin {
    pub fn new() -> Self {
        Self {
            ctx: None,
            next_session_id: Arc::new(parking_lot::RwLock::new(1)),
            sessions: Arc::new(parking_lot::RwLock::new(Vec::new())),
            stop_tx: None,
            server_thread: None,
        }
    }
}

/// Accept loop for incoming RTMP connections.
///
/// Runs on the plugin's own Tokio runtime (see `on_start`), never on the host's.
async fn accept_loop(
    listener: TcpListener,
    ctx: PluginContext,
    next_id: Arc<parking_lot::RwLock<u32>>,
    sessions: Arc<parking_lot::RwLock<Vec<SessionInfo>>>,
) {
    loop {
        match listener.accept().await {
            Ok((stream, peer)) => {
                ctx.info(format!("RTMP connection from {}", peer));

                let ctx = ctx.clone();
                let next_id = Arc::clone(&next_id);
                let sessions = Arc::clone(&sessions);

                tokio::spawn(async move {
                    let session_id = {
                        let mut id = next_id.write();
                        let current = *id;
                        *id += 1;
                        current
                    };

                    sessions.write().push(SessionInfo {
                        id: session_id,
                        peer: peer.to_string(),
                        state: "handshaking".to_string(),
                        stream_key: None,
                        bytes_received: Arc::new(std::sync::atomic::AtomicU64::new(0)),
                    });
                    let bytes_received = sessions
                        .read()
                        .iter()
                        .find(|s| s.id == session_id)
                        .map(|s| Arc::clone(&s.bytes_received))
                        .unwrap_or_else(|| Arc::new(std::sync::atomic::AtomicU64::new(0)));

                    ctx.event_bus.publish_all(Event::ConnectionOpened {
                        peer: peer.to_string(),
                    });

                    // The RTMP handshake has to happen before the chunk stream starts,
                    // otherwise the handshake bytes are parsed as chunk data.
                    let mut stream = stream;
                    if let Err(e) = rtmp_handshake::handshake(&mut stream, &ctx).await {
                        ctx.error(format!(
                            "RTMP session {}: handshake failed: {}",
                            session_id, e
                        ));
                        ctx.event_bus.publish_all(Event::ConnectionClosed {
                            peer: peer.to_string(),
                        });
                        sessions.write().retain(|s| s.id != session_id);
                        return;
                    }

                    let mut session = RtmpSession::new(
                        session_id,
                        stream,
                        ctx.clone(),
                        Arc::clone(&sessions),
                        Arc::clone(&bytes_received),
                    );
                    let result = session.run(&ctx.event_bus).await;

                    sessions.write().retain(|s| s.id != session_id);

                    ctx.event_bus.publish_all(Event::ConnectionClosed {
                        peer: peer.to_string(),
                    });

                    if let Some(stream_key) = session.stream_key() {
                        ctx.event_bus.publish_all(Event::StreamStop {
                            stream_key: stream_key.to_string(),
                        });
                    }

                    if let Err(e) = result {
                        ctx.error(format!("RTMP session {} error: {}", session_id, e));
                    }
                });
            }
            Err(e) => {
                ctx.error(format!("RTMP accept error: {}", e));
                tokio::time::sleep(std::time::Duration::from_millis(100)).await;
            }
        }
    }
}

impl Plugin for RtmpPlugin {
    fn name(&self) -> &str {
        "rtmp"
    }

    fn version(&self) -> &str {
        "0.1.0"
    }

    fn on_load(&mut self, ctx: &PluginContext) -> Result<()> {
        self.ctx = Some(ctx.clone());
        ctx.info("RTMP plugin loaded");
        Ok(())
    }

    fn on_start(&mut self) -> Result<()> {
        let ctx = self.ctx.clone().unwrap();
        let addr = format!("{}:{}", ctx.config.server.host, ctx.config.server.rtmp_port);

        // Bind with the standard library so a port clash is reported to the engine
        // right away; the socket is handed over to Tokio inside the plugin runtime.
        let std_listener = std::net::TcpListener::bind(&addr)?;
        std_listener.set_nonblocking(true)?;

        let next_id = Arc::clone(&self.next_session_id);
        let sessions = Arc::clone(&self.sessions);
        let (stop_tx, stop_rx) = tokio::sync::oneshot::channel::<()>();
        self.stop_tx = Some(stop_tx);

        // A plugin `.so` statically links its own copy of Tokio, so it cannot observe the
        // host's runtime context: calling `TcpListener::from_std` or `tokio::spawn` from
        // the host's runtime panics with "there is no reactor running". The plugin
        // therefore owns a runtime of its own, driven from a dedicated thread. Every
        // Tokio API used by this plugin is invoked from within that runtime.
        let handle = std::thread::Builder::new()
            .name("aurora-rtmp".to_string())
            .spawn(move || {
                let runtime = match tokio::runtime::Runtime::new() {
                    Ok(rt) => rt,
                    Err(e) => {
                        ctx.error(format!("RTMP plugin could not start its runtime: {}", e));
                        return;
                    }
                };

                runtime.block_on(async move {
                    let listener = match TcpListener::from_std(std_listener) {
                        Ok(l) => l,
                        Err(e) => {
                            ctx.error(format!("RTMP server could not listen on {}: {}", addr, e));
                            return;
                        }
                    };

                    ctx.info(format!("RTMP server listening on {}", addr));

                    tokio::select! {
                        _ = accept_loop(listener, ctx, next_id, sessions) => {}
                        _ = stop_rx => {}
                    }
                });
            })
            .map_err(|e| AuroraError::Plugin(format!("failed to spawn RTMP thread: {}", e)))?;

        self.server_thread = Some(handle);
        Ok(())
    }

    fn on_stop(&mut self) -> Result<()> {
        if let Some(tx) = self.stop_tx.take() {
            let _ = tx.send(());
        }
        if let Some(handle) = self.server_thread.take() {
            let _ = handle.join();
        }
        if let Some(ctx) = &self.ctx {
            ctx.info("RTMP plugin stopped");
        }
        Ok(())
    }

    fn on_unload(&mut self) -> Result<()> {
        self.sessions.write().clear();
        if let Some(ctx) = &self.ctx {
            ctx.info("RTMP plugin unloaded");
        }
        Ok(())
    }

    fn on_event(&mut self, event: &Event) -> Result<()> {
        match event {
            Event::StreamPublish { stream_key, .. } => {
                if let Some(ctx) = &self.ctx {
                    ctx.debug(format!("stream published: {}", stream_key));
                }
            }
            Event::StreamStop { stream_key } => {
                if let Some(ctx) = &self.ctx {
                    ctx.info(format!("stream stopped: {}", stream_key));
                }
            }
            _ => {}
        }
        Ok(())
    }

    fn routes(&self) -> Vec<HttpRoute> {
        let sessions = Arc::clone(&self.sessions);

        vec![HttpRoute {
            method: "GET".to_string(),
            path: "/api/rtmp/sessions".to_string(),
            handler: Arc::new(move |_req| {
                let list = sessions.read();
                let body = serde_json::json!({
                    "sessions": list
                        .iter()
                        .map(|s| serde_json::json!({
                            "id": s.id,
                            "peer": s.peer,
                            "state": s.state,
                            "stream_key": s.stream_key,
                            "bytes_received": s.bytes_received.load(std::sync::atomic::Ordering::Relaxed),
                        }))
                        .collect::<Vec<_>>(),
                    "count": list.len(),
                });
                http::Response::builder()
                    .status(200)
                    .header("content-type", "application/json")
                    .body(axum::body::Body::from(
                        serde_json::to_string(&body).unwrap(),
                    ))
                    .unwrap()
            }),
        }]
    }
}

impl Clone for RtmpPlugin {
    fn clone(&self) -> Self {
        Self {
            ctx: self.ctx.clone(),
            next_session_id: Arc::clone(&self.next_session_id),
            sessions: Arc::clone(&self.sessions),
            stop_tx: None,
            server_thread: None,
        }
    }
}

// The plugin ABI currently hands a Rust trait object across the shared object
// boundary. It works because both sides are built from the same sources, but it is
// not a stable ABI; a `#[repr(C)]` versioned interface is the planned replacement.
#[allow(improper_ctypes_definitions)]
#[no_mangle]
pub extern "C" fn aurora_plugin_create() -> *mut dyn Plugin {
    Box::into_raw(Box::new(RtmpPlugin::new()))
}

#[allow(improper_ctypes_definitions)]
#[no_mangle]
pub extern "C" fn aurora_plugin_destroy(plugin: *mut dyn Plugin) {
    unsafe {
        if !plugin.is_null() {
            drop(Box::from_raw(plugin));
        }
    }
}
