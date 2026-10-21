use std::sync::Arc;

use parking_lot::RwLock;
use tokio::sync::mpsc;

use aurora_core::error::Result;
use aurora_core::event_bus::Event;
use aurora_core::plugin::{HttpRoute, Plugin, PluginContext};

pub struct RemoteDesktopPlugin {
    ctx: Option<PluginContext>,
    sessions: Arc<RwLock<Vec<DesktopSession>>>,
    stop_tx: Option<mpsc::Sender<()>>,
}

#[derive(Debug, Clone)]
struct DesktopSession {
    id: u32,
    width: u32,
    height: u32,
    client_info: String,
}

impl DesktopSession {
    fn to_json(&self) -> serde_json::Value {
        serde_json::json!({
            "id": self.id,
            "width": self.width,
            "height": self.height,
            "client_info": self.client_info,
        })
    }
}

impl std::fmt::Debug for RemoteDesktopPlugin {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("RemoteDesktopPlugin").finish()
    }
}

impl RemoteDesktopPlugin {
    pub fn new() -> Self {
        Self {
            ctx: None,
            sessions: Arc::new(RwLock::new(Vec::new())),
            stop_tx: None,
        }
    }
}

impl Plugin for RemoteDesktopPlugin {
    fn name(&self) -> &str {
        "remote_desktop"
    }

    fn version(&self) -> &str {
        "0.1.0"
    }

    fn on_load(&mut self, ctx: &PluginContext) -> Result<()> {
        self.ctx = Some(ctx.clone());
        ctx.info("Remote Desktop plugin loaded");
        Ok(())
    }

    fn on_start(&mut self) -> Result<()> {
        if let Some(ctx) = &self.ctx {
            ctx.info("Remote Desktop plugin started");
        }
        Ok(())
    }

    fn on_stop(&mut self) -> Result<()> {
        if let Some(tx) = self.stop_tx.take() {
            let _ = tx.blocking_send(());
        }
        self.sessions.write().clear();
        if let Some(ctx) = &self.ctx {
            ctx.info("Remote Desktop plugin stopped");
        }
        Ok(())
    }

    fn on_unload(&mut self) -> Result<()> {
        self.sessions.write().clear();
        if let Some(ctx) = &self.ctx {
            ctx.info("Remote Desktop plugin unloaded");
        }
        Ok(())
    }

    fn on_event(&mut self, _event: &Event) -> Result<()> {
        Ok(())
    }

    fn routes(&self) -> Vec<HttpRoute> {
        let sessions = Arc::clone(&self.sessions);

        vec![
            HttpRoute {
                method: "GET".to_string(),
                path: "/api/remote_desktop/sessions".to_string(),
                handler: Arc::new(move |_req| {
                    let sessions = sessions.read();
                    let body = serde_json::json!({
                        "sessions": sessions.iter().map(|s| s.to_json()).collect::<Vec<_>>(),
                        "count": sessions.len(),
                    });
                    http::Response::builder()
                        .status(200)
                        .header("content-type", "application/json")
                        .body(axum::body::Body::from(
                            serde_json::to_string(&body).unwrap(),
                        ))
                        .unwrap()
                }),
            },
            HttpRoute {
                method: "POST".to_string(),
                path: "/api/remote_desktop/start".to_string(),
                handler: Arc::new(|_req| {
                    let body = serde_json::json!({
                        "status": "started",
                        "session_id": 1,
                    });
                    http::Response::builder()
                        .status(200)
                        .header("content-type", "application/json")
                        .body(axum::body::Body::from(
                            serde_json::to_string(&body).unwrap(),
                        ))
                        .unwrap()
                }),
            },
        ]
    }
}

// See the note in crates/aurora-rtmp/src/lib.rs about the current plugin ABI.
#[allow(improper_ctypes_definitions)]
#[no_mangle]
pub extern "C" fn aurora_plugin_create() -> *mut dyn Plugin {
    Box::into_raw(Box::new(RemoteDesktopPlugin::new()))
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
