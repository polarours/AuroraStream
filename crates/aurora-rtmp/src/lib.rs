use std::sync::Arc;

use tokio::net::TcpListener;

use aurora_core::error::{AuroraError, Result};
use aurora_core::event_bus::Event;
use aurora_core::plugin::{HttpRoute, Plugin, PluginContext};

use crate::flv_stream::FlvStream;

mod flv_muxer;
mod flv_stream;
mod rtmp_amf;
mod rtmp_chunk;
mod rtmp_flv;
mod rtmp_handshake;
mod rtmp_session;
mod stream_registry;

pub use rtmp_amf::AmfValue;
pub use rtmp_flv::StreamMetadata;
pub use stream_registry::{MediaPacket, StreamRegistry, StreamSnapshot};
pub use rtmp_chunk::{ChunkHeader, ChunkMessage};
pub use rtmp_handshake::handshake;
pub use rtmp_session::{RtmpSession, SessionInfo, SessionState};

pub struct RtmpPlugin {
    ctx: Option<PluginContext>,
    next_session_id: Arc<parking_lot::RwLock<u32>>,
    sessions: Arc<parking_lot::RwLock<Vec<SessionInfo>>>,
    registry: Arc<StreamRegistry>,
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
            registry: Arc::new(StreamRegistry::new()),
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
    registry: Arc<StreamRegistry>,
) {
    loop {
        match listener.accept().await {
            Ok((stream, peer)) => {
                ctx.info(format!("RTMP connection from {}", peer));

                let ctx = ctx.clone();
                let next_id = Arc::clone(&next_id);
                let sessions = Arc::clone(&sessions);
                let registry = Arc::clone(&registry);

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
                        Arc::clone(&registry),
                    );
                    let result = session.run(&ctx.event_bus).await;

                    sessions.write().retain(|s| s.id != session_id);

                    ctx.event_bus.publish_all(Event::ConnectionClosed {
                        peer: peer.to_string(),
                    });

                    // Only the publisher ends the stream. A player that disconnects
                    // must not take the live stream down with it.
                    if session.is_publisher() {
                        if let Some(stream_key) = session.stream_key() {
                            ctx.event_bus.publish_all(Event::StreamStop {
                                stream_key: stream_key.to_string(),
                            });
                        }
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
        let registry = Arc::clone(&self.registry);
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
                        _ = accept_loop(listener, ctx, next_id, sessions, registry) => {}
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
        let registry = Arc::clone(&self.registry);
        let registry_flv = Arc::clone(&self.registry);

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
        },
        HttpRoute {
            method: "GET".to_string(),
            path: "/flv/*key".to_string(),
            handler: Arc::new(move |req| {
                let key = match extract_stream_key(&req) {
                    Some(key) => key,
                    None => return not_found("no stream key given"),
                };

                let (snapshot, receiver) = match registry_flv.subscribe(&key) {
                    Some(subscription) => subscription,
                    None => return not_found(&format!("stream '{}' is not live", key)),
                };

                // The FLV header, the metadata and the sequence headers are sent once,
                // the live frames follow.
                let mut prefix = bytes::BytesMut::new();
                prefix.extend_from_slice(&flv_muxer::flv_header());
                prefix.extend_from_slice(&flv_muxer::flv_tag(
                    flv_muxer::TAG_SCRIPT,
                    0,
                    &rtmp_flv::build_flv_metadata(&snapshot.metadata),
                ));
                if let Some(video_header) = &snapshot.video_header {
                    prefix.extend_from_slice(&flv_muxer::flv_tag(
                        flv_muxer::TAG_VIDEO,
                        0,
                        video_header,
                    ));
                }
                if let Some(audio_header) = &snapshot.audio_header {
                    prefix.extend_from_slice(&flv_muxer::flv_tag(
                        flv_muxer::TAG_AUDIO,
                        0,
                        audio_header,
                    ));
                }

                let stream = FlvStream::new(receiver, prefix.freeze());
                http::Response::builder()
                    .status(200)
                    .header("content-type", "video/x-flv")
                    .header("cache-control", "no-cache")
                    .header("x-accel-buffering", "no")
                    .body(axum::body::Body::from_stream(stream))
                    .unwrap()
            }),
        },
        HttpRoute {
            method: "GET".to_string(),
            path: "/api/rtmp/streams".to_string(),
            handler: Arc::new(move |_req| {
                let streams = registry.list();
                let body = serde_json::json!({
                    "streams": streams
                        .iter()
                        .map(|s| serde_json::json!({
                            "key": s.key,
                            "video_codec": s.metadata.video_codec,
                            "audio_codec": s.metadata.audio_codec,
                            "width": s.metadata.width,
                            "height": s.metadata.height,
                            "sample_rate": s.metadata.sample_rate,
                            "channels": s.metadata.channels,
                            "uptime": s.started_at.elapsed().as_secs(),
                        }))
                        .collect::<Vec<_>>(),
                    "count": streams.len(),
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

/// Builds a 404 response.
fn not_found(message: &str) -> http::Response<axum::body::Body> {
    let body = serde_json::json!({ "error": message });
    http::Response::builder()
        .status(404)
        .header("content-type", "application/json")
        .body(axum::body::Body::from(
            serde_json::to_string(&body).unwrap(),
        ))
        .unwrap()
}

/// Reads the stream key out of a request.
///
/// The key is either the `stream` query parameter or the path behind `/flv/`, so both
/// `/flv/live/demo.flv` and `/flv?stream=live/demo` work.
fn extract_stream_key(req: &http::Request<axum::body::Body>) -> Option<String> {
    if let Some(query) = req.uri().query() {
        for pair in query.split('&') {
            if let Some((name, value)) = pair.split_once('=') {
                if name == "stream" {
                    let decoded = percent_decode(value);
                    if !decoded.is_empty() {
                        return Some(decoded);
                    }
                }
            }
        }
    }

    let path = req.uri().path();
    let rest = path.strip_prefix("/flv/")?;
    let rest = rest.strip_suffix(".flv").unwrap_or(rest);
    let decoded = percent_decode(rest);
    if decoded.is_empty() {
        None
    } else {
        Some(decoded)
    }
}

/// Decodes the percent escapes of a URL component.
fn percent_decode(value: &str) -> String {
    let bytes = value.as_bytes();
    let mut out = Vec::with_capacity(bytes.len());
    let mut i = 0;

    while i < bytes.len() {
        if bytes[i] == b'%' && i + 2 < bytes.len() {
            let hex = std::str::from_utf8(&bytes[i + 1..i + 3]).unwrap_or("");
            if let Ok(byte) = u8::from_str_radix(hex, 16) {
                out.push(byte);
                i += 3;
                continue;
            }
        }
        out.push(bytes[i]);
        i += 1;
    }

    String::from_utf8_lossy(&out).into_owned()
}

impl Clone for RtmpPlugin {
    fn clone(&self) -> Self {
        Self {
            ctx: self.ctx.clone(),
            next_session_id: Arc::clone(&self.next_session_id),
            sessions: Arc::clone(&self.sessions),
            registry: Arc::clone(&self.registry),
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
