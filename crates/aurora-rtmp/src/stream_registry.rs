//! Registry of the streams that are currently live.
//!
//! A publisher fills the entry for its stream key: the metadata it derived from the
//! sequence headers and the sequence headers themselves. Players subscribe to a key and
//! receive the metadata, the cached sequence headers and every frame published from then
//! on.

use std::collections::HashMap;
use std::time::Instant;

use parking_lot::RwLock;
use tokio::sync::mpsc;

use crate::rtmp_flv::StreamMetadata;

/// A media frame on its way to the players.
#[derive(Debug, Clone)]
pub struct MediaPacket {
    /// 8 for audio, 9 for video.
    pub msg_type_id: u8,
    pub timestamp: u32,
    pub payload: Vec<u8>,
}

/// What a subscribed player receives.
#[derive(Debug, Clone)]
pub enum StreamEvent {
    /// A media frame of the live stream.
    Packet(MediaPacket),
    /// The publisher went away, no more frames are coming.
    Ended,
}

/// The part of a live stream a player needs to start decoding.
#[derive(Debug, Clone)]
pub struct StreamSnapshot {
    pub key: String,
    pub metadata: StreamMetadata,
    /// The AVC/HEVC sequence header, the FLV video tag body.
    pub video_header: Option<Vec<u8>>,
    /// The AAC sequence header, the FLV audio tag body.
    pub audio_header: Option<Vec<u8>>,
    pub started_at: Instant,
}

struct LiveStream {
    snapshot: StreamSnapshot,
    subscribers: Vec<mpsc::UnboundedSender<StreamEvent>>,
}

pub struct StreamRegistry {
    streams: RwLock<HashMap<String, LiveStream>>,
}

impl std::fmt::Debug for StreamRegistry {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("StreamRegistry")
            .field("streams", &self.streams.read().keys().collect::<Vec<_>>())
            .finish()
    }
}

impl StreamRegistry {
    pub fn new() -> Self {
        Self {
            streams: RwLock::new(HashMap::new()),
        }
    }

    /// Creates the stream entry if it does not exist yet.
    pub fn ensure(&self, key: &str) {
        let mut streams = self.streams.write();
        streams.entry(key.to_string()).or_insert_with(|| LiveStream {
            snapshot: StreamSnapshot {
                key: key.to_string(),
                metadata: StreamMetadata::default(),
                video_header: None,
                audio_header: None,
                started_at: Instant::now(),
            },
            subscribers: Vec::new(),
        });
    }

    pub fn set_metadata(&self, key: &str, metadata: StreamMetadata) {
        let mut streams = self.streams.write();
        if let Some(stream) = streams.get_mut(key) {
            stream.snapshot.metadata = metadata;
        }
    }

    /// Stores the AVC/HEVC sequence header, the first thing a player needs.
    pub fn set_video_header(&self, key: &str, header: Vec<u8>) {
        let mut streams = self.streams.write();
        if let Some(stream) = streams.get_mut(key) {
            stream.snapshot.video_header = Some(header);
        }
    }

    /// Stores the AAC sequence header.
    pub fn set_audio_header(&self, key: &str, header: Vec<u8>) {
        let mut streams = self.streams.write();
        if let Some(stream) = streams.get_mut(key) {
            stream.snapshot.audio_header = Some(header);
        }
    }

    /// Hands a frame to every player of the stream.
    pub fn broadcast(&self, key: &str, packet: MediaPacket) {
        let mut streams = self.streams.write();
        let stream = match streams.get_mut(key) {
            Some(s) => s,
            None => return,
        };

        // Drop the players that went away.
        stream
            .subscribers
            .retain(|tx| tx.send(StreamEvent::Packet(packet.clone())).is_ok());
    }

    /// Subscribes a player to a live stream.
    pub fn subscribe(
        &self,
        key: &str,
    ) -> Option<(StreamSnapshot, mpsc::UnboundedReceiver<StreamEvent>)> {
        let (tx, rx) = mpsc::unbounded_channel();
        let mut streams = self.streams.write();
        let stream = streams.get_mut(key)?;
        stream.subscribers.push(tx);
        Some((stream.snapshot.clone(), rx))
    }

    pub fn snapshot(&self, key: &str) -> Option<StreamSnapshot> {
        self.streams.read().get(key).map(|s| s.snapshot.clone())
    }

    pub fn list(&self) -> Vec<StreamSnapshot> {
        self.streams.read().values().map(|s| s.snapshot.clone()).collect()
    }

    /// Removes a stream and tells its players that the stream ended.
    pub fn remove(&self, key: &str) {
        if let Some(stream) = self.streams.write().remove(key) {
            for subscriber in stream.subscribers {
                let _ = subscriber.send(StreamEvent::Ended);
            }
        }
    }

    pub fn contains(&self, key: &str) -> bool {
        self.streams.read().contains_key(key)
    }
}

impl Default for StreamRegistry {
    fn default() -> Self {
        Self::new()
    }
}
