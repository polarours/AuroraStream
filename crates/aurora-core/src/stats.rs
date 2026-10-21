use std::sync::atomic::{AtomicU32, Ordering};
use std::time::Instant;

use parking_lot::RwLock;

use crate::event_bus::CodecInfo;

/// Information about a stream that is currently live.
#[derive(Debug, Clone)]
pub struct StreamInfo {
    pub key: String,
    pub codec_info: CodecInfo,
    pub started_at: Instant,
}

/// Live counters of the running server.
///
/// The HTTP gateway and the TUI read from this, so it has to be cheap to update and
/// safe to share across threads.
#[derive(Debug)]
pub struct ServerStats {
    started_at: Instant,
    connections: AtomicU32,
    streams: RwLock<Vec<StreamInfo>>,
}

impl ServerStats {
    pub fn new() -> Self {
        Self {
            started_at: Instant::now(),
            connections: AtomicU32::new(0),
            streams: RwLock::new(Vec::new()),
        }
    }

    /// Seconds since the engine started.
    pub fn uptime_secs(&self) -> u64 {
        self.started_at.elapsed().as_secs()
    }

    pub fn connection_opened(&self) {
        self.connections.fetch_add(1, Ordering::Relaxed);
    }

    pub fn connection_closed(&self) {
        let _ = self
            .connections
            .try_update(Ordering::Relaxed, Ordering::Relaxed, |v| {
                Some(v.saturating_sub(1))
            });
    }

    pub fn active_connections(&self) -> u32 {
        self.connections.load(Ordering::Relaxed)
    }

    pub fn stream_started(&self, key: &str, codec_info: CodecInfo) {
        let mut streams = self.streams.write();
        if let Some(existing) = streams.iter_mut().find(|s| s.key == key) {
            existing.codec_info = codec_info;
            existing.started_at = Instant::now();
        } else {
            streams.push(StreamInfo {
                key: key.to_string(),
                codec_info,
                started_at: Instant::now(),
            });
        }
    }

    pub fn stream_stopped(&self, key: &str) {
        self.streams.write().retain(|s| s.key != key);
    }

    pub fn has_stream(&self, key: &str) -> bool {
        self.streams.read().iter().any(|s| s.key == key)
    }

    pub fn active_streams(&self) -> u32 {
        self.streams.read().len() as u32
    }

    pub fn streams(&self) -> Vec<StreamInfo> {
        self.streams.read().clone()
    }
}

impl Default for ServerStats {
    fn default() -> Self {
        Self::new()
    }
}
