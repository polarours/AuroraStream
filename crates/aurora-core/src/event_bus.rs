use std::collections::{HashMap, VecDeque};
use std::sync::mpsc;

use parking_lot::RwLock;

use crate::media::{AudioFrame, VideoFrame};

#[derive(Debug, Clone)]
pub enum Event {
    StreamPublish {
        stream_key: String,
        codec_info: CodecInfo,
    },
    StreamPlay {
        stream_key: String,
    },
    StreamStop {
        stream_key: String,
    },
    VideoFrame {
        stream_key: String,
        frame: VideoFrame,
    },
    AudioFrame {
        stream_key: String,
        frame: AudioFrame,
    },
    /// Encoded (still compressed) video payload as it arrived on the wire.
    EncodedVideo {
        stream_key: String,
        data: Vec<u8>,
        timestamp: u32,
        keyframe: bool,
    },
    /// Encoded (still compressed) audio payload as it arrived on the wire.
    EncodedAudio {
        stream_key: String,
        data: Vec<u8>,
        timestamp: u32,
    },
    /// A new client connection was accepted.
    ConnectionOpened {
        peer: String,
    },
    /// A client connection was closed.
    ConnectionClosed {
        peer: String,
    },
    PluginLoaded {
        plugin_name: String,
    },
    PluginUnloaded {
        plugin_name: String,
    },
    Custom(String, Vec<u8>),
}

#[derive(Debug, Clone)]
pub struct CodecInfo {
    pub video_codec: Option<String>,
    pub audio_codec: Option<String>,
    pub width: Option<u32>,
    pub height: Option<u32>,
    pub sample_rate: Option<u32>,
    pub channels: Option<u32>,
}

struct Subscriber {
    // `std::sync::mpsc::Sender` is `Send` but not `Sync`, so it is wrapped in a mutex to
    // keep the whole bus `Sync`. `std::sync::Mutex` lives in the shared `std`, so the host
    // and a plugin `.so` operate on the very same implementation.
    sender: std::sync::Mutex<mpsc::Sender<Event>>,
}

pub struct EventBus {
    subscribers: RwLock<HashMap<String, Vec<Subscriber>>>,
    queue: RwLock<VecDeque<Event>>,
}

impl EventBus {
    pub fn new() -> Self {
        Self {
            subscribers: RwLock::new(HashMap::new()),
            queue: RwLock::new(VecDeque::new()),
        }
    }

    pub fn subscribe(&self, topic: &str) -> mpsc::Receiver<Event> {
        let (tx, rx) = mpsc::channel();
        let mut subs = self.subscribers.write();
        subs.entry(topic.to_string())
            .or_insert_with(Vec::new)
            .push(Subscriber {
                sender: std::sync::Mutex::new(tx),
            });
        rx
    }

    pub fn publish(&self, topic: &str, event: Event) {
        let subs = self.subscribers.read();
        if let Some(subscribers) = subs.get(topic) {
            for sub in subscribers {
                let _ = sub.sender.lock().unwrap().send(event.clone());
            }
        }
    }

    pub fn publish_all(&self, event: Event) {
        let topic = match &event {
            Event::StreamPublish { .. } => "stream.publish",
            Event::StreamPlay { .. } => "stream.play",
            Event::StreamStop { .. } => "stream.stop",
            Event::VideoFrame { .. } => "media.video",
            Event::AudioFrame { .. } => "media.audio",
            Event::EncodedVideo { .. } => "media.video",
            Event::EncodedAudio { .. } => "media.audio",
            Event::ConnectionOpened { .. } => "connection.opened",
            Event::ConnectionClosed { .. } => "connection.closed",
            Event::PluginLoaded { .. } => "plugin.loaded",
            Event::PluginUnloaded { .. } => "plugin.unloaded",
            Event::Custom(name, _) => name,
        };
        let topic = topic.to_string();

        // Every event is also queued so the engine can fan it out to the plugins that
        // implement `Plugin::on_event`.
        self.queue.write().push_back(event.clone());

        self.publish(&topic, event);
    }

    /// Removes and returns every event queued since the last drain.
    pub fn drain(&self) -> Vec<Event> {
        let mut queue = self.queue.write();
        queue.drain(..).collect()
    }

    pub fn topics(&self) -> Vec<String> {
        let subs = self.subscribers.read();
        subs.keys().cloned().collect()
    }
}

impl Default for EventBus {
    fn default() -> Self {
        Self::new()
    }
}
