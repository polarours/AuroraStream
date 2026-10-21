use std::collections::{HashMap, HashSet};
use std::net::SocketAddr;
use std::sync::Arc;

use axum::{
    extract::{Path, Request, State},
    http::StatusCode,
    response::IntoResponse,
    routing::get,
    Json, Router,
};
use parking_lot::RwLock;
use serde_json::{json, Value};

use crate::config::Config;
use crate::error::Result;
use crate::plugin::HttpHandler;
use crate::stats::ServerStats;

pub struct HttpGateway {
    config: Config,
    plugin_routes: Arc<RwLock<HashMap<String, Vec<String>>>>,
    plugin_handlers: Arc<RwLock<Vec<(String, HttpHandler)>>>,
    stats: Arc<ServerStats>,
}

impl Clone for HttpGateway {
    fn clone(&self) -> Self {
        Self {
            config: self.config.clone(),
            plugin_routes: Arc::clone(&self.plugin_routes),
            plugin_handlers: Arc::clone(&self.plugin_handlers),
            stats: Arc::clone(&self.stats),
        }
    }
}

#[derive(Clone)]
struct GatewayState {
    plugin_routes: Arc<RwLock<HashMap<String, Vec<String>>>>,
    stats: Arc<ServerStats>,
}

impl HttpGateway {
    pub fn new(config: Config, stats: Arc<ServerStats>) -> Self {
        Self {
            config,
            plugin_routes: Arc::new(RwLock::new(HashMap::new())),
            plugin_handlers: Arc::new(RwLock::new(Vec::new())),
            stats,
        }
    }

    /// Records the routes a plugin advertises, used by `/api/plugins`.
    pub fn register_plugin_routes(&self, plugin_name: &str, routes: Vec<String>) {
        let mut pr = self.plugin_routes.write();
        pr.insert(plugin_name.to_string(), routes);
    }

    /// Registers the actual handler of a plugin route so it can be mounted.
    pub fn register_plugin_handler(&self, path: &str, handler: HttpHandler) {
        self.plugin_handlers
            .write()
            .push((path.to_string(), handler));
    }

    /// Binds the HTTP port. Binding is separated from serving so that a port clash
    /// fails the engine startup instead of only being logged from a background task.
    /// Drops the handlers the plugins registered.
    ///
    /// Called once the plugins are stopped: the handler closures live in the plugin
    /// shared objects, and the host should not keep calling into a stopped plugin.
    pub fn clear_plugin_handlers(&self) {
        self.plugin_handlers.write().clear();
    }

    pub async fn bind(&self) -> Result<tokio::net::TcpListener> {
        let addr: SocketAddr = format!(
            "{}:{}",
            self.config.server.host, self.config.server.http_port
        )
        .parse()
        .map_err(|e| crate::error::AuroraError::Network(format!("invalid address: {}", e)))?;

        tracing::info!("HTTP gateway listening on {}", addr);

        let listener = tokio::net::TcpListener::bind(addr).await?;
        Ok(listener)
    }

    /// Serves the API on an already bound listener.
    pub async fn serve(&self, listener: tokio::net::TcpListener) -> Result<()> {
        let state = GatewayState {
            plugin_routes: Arc::clone(&self.plugin_routes),
            stats: Arc::clone(&self.stats),
        };

        let mut app = Router::new()
            .route("/", get(root_handler))
            .route("/api/health", get(health_handler))
            .route("/api/stats", get(stats_handler))
            .route("/api/streams", get(streams_handler))
            .route("/api/plugins", get(list_plugins_handler))
            .route("/api/plugins/:name", get(plugin_info_handler));

        // Mount the handlers the plugins registered. They receive the whole request, so
        // they can inspect the method and the body themselves.
        let mut mounted_paths: HashSet<String> = [
            "/",
            "/api/health",
            "/api/stats",
            "/api/streams",
            "/api/plugins",
        ]
        .iter()
        .map(|p| p.to_string())
        .collect();

        for (path, handler) in self.plugin_handlers.read().iter() {
            if !mounted_paths.insert(path.clone()) {
                tracing::warn!("plugin route {} is already mounted, skipping", path);
                continue;
            }
            let handler = handler.clone();
            app = app.route(
                path.as_str(),
                axum::routing::any(move |req: Request| {
                    let handler = handler.clone();
                    async move { handler(req) }
                }),
            );
        }

        let app = app.with_state(state);

        axum::serve(listener, app)
            .await
            .map_err(|e| crate::error::AuroraError::Network(format!("server error: {}", e)))?;

        Ok(())
    }
}

async fn root_handler() -> impl IntoResponse {
    Json(json!({
        "name": "AuroraStream",
        "version": "0.1.0",
        "status": "running"
    }))
}

async fn health_handler(State(state): State<GatewayState>) -> impl IntoResponse {
    Json(json!({
        "status": "healthy",
        "uptime": state.stats.uptime_secs(),
        "connections": state.stats.active_connections(),
        "streams": state.stats.active_streams(),
    }))
}

async fn stats_handler(State(state): State<GatewayState>) -> impl IntoResponse {
    let streams: Vec<Value> = state
        .stats
        .streams()
        .iter()
        .map(|s| {
            json!({
                "key": s.key,
                "video_codec": s.codec_info.video_codec,
                "audio_codec": s.codec_info.audio_codec,
                "uptime": s.started_at.elapsed().as_secs(),
            })
        })
        .collect();

    Json(json!({
        "uptime": state.stats.uptime_secs(),
        "connections": state.stats.active_connections(),
        "streams": state.stats.active_streams(),
        "stream_list": streams,
    }))
}

async fn streams_handler(State(state): State<GatewayState>) -> impl IntoResponse {
    let streams: Vec<Value> = state
        .stats
        .streams()
        .iter()
        .map(|s| {
            json!({
                "key": s.key,
                "video_codec": s.codec_info.video_codec,
                "audio_codec": s.codec_info.audio_codec,
            })
        })
        .collect();

    Json(json!({ "streams": streams, "count": streams.len() }))
}

async fn list_plugins_handler(State(state): State<GatewayState>) -> impl IntoResponse {
    let routes = state.plugin_routes.read();
    let plugins: Vec<Value> = routes
        .iter()
        .map(|(name, routes)| {
            json!({
                "name": name,
                "routes": routes,
            })
        })
        .collect();

    Json(json!({ "plugins": plugins }))
}

async fn plugin_info_handler(
    State(state): State<GatewayState>,
    Path(name): Path<String>,
) -> impl IntoResponse {
    let routes = state.plugin_routes.read();
    if let Some(plugin_routes) = routes.get(&name) {
        Json(json!({
            "name": name,
            "routes": plugin_routes,
        }))
        .into_response()
    } else {
        (
            StatusCode::NOT_FOUND,
            Json(json!({
                "error": format!("plugin '{}' not found", name)
            })),
        )
            .into_response()
    }
}
