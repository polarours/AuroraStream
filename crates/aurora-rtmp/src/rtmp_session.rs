use std::sync::Arc;

use bytes::{BufMut, Bytes, BytesMut};
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::tcp::{OwnedReadHalf, OwnedWriteHalf};

use aurora_core::error::Result;
use aurora_core::event_bus::{CodecInfo, Event, EventBus};
use aurora_core::plugin::PluginContext;
use parking_lot::RwLock;

use crate::rtmp_amf::{self, AmfValue};
use crate::rtmp_chunk::{
    encode_message, ChunkDecoder, ChunkError, ChunkHeader, ChunkMessage, CSID_COMMAND,
    CSID_PROTOCOL_CONTROL, DEFAULT_CHUNK_SIZE,
};

/// Chunk size the server asks the peer to use.
const SERVER_CHUNK_SIZE: u32 = 4096;
/// Protocol control message type ids.
const MSG_SET_CHUNK_SIZE: u8 = 1;
const MSG_ABORT: u8 = 2;
const MSG_ACKNOWLEDGEMENT: u8 = 3;
const MSG_USER_CONTROL: u8 = 4;
const MSG_WINDOW_ACK_SIZE: u8 = 5;
const MSG_SET_PEER_BANDWIDTH: u8 = 6;
const MSG_AUDIO: u8 = 8;
const MSG_VIDEO: u8 = 9;
const MSG_AMF0_COMMAND: u8 = 20;

/// User control event: stream begin.
const USER_CONTROL_STREAM_BEGIN: u16 = 0;

/// State of a single RTMP connection, exposed through the plugin HTTP route.
#[derive(Debug, Clone)]
pub struct SessionInfo {
    pub id: u32,
    pub peer: String,
    pub state: String,
    pub stream_key: Option<String>,
    pub bytes_received: std::sync::Arc<std::sync::atomic::AtomicU64>,
}

#[derive(Debug)]
pub struct RtmpSession {
    id: u32,
    state: SessionState,
    stream_key: Option<String>,
    app: Option<String>,
    transaction_id: f64,
    window_ack_size: u32,
    peer_bandwidth: u32,
    /// Chunk size we use when sending to the peer.
    write_chunk_size: u32,
    bytes_received: Arc<std::sync::atomic::AtomicU64>,
    video_codec: Option<String>,
    audio_codec: Option<String>,
    ctx: PluginContext,
    sessions: Arc<RwLock<Vec<SessionInfo>>>,
    reader: tokio::io::BufReader<OwnedReadHalf>,
    writer: OwnedWriteHalf,
    decoder: ChunkDecoder,
}

#[derive(Debug, Clone, PartialEq)]
pub enum SessionState {
    Init,
    Connecting,
    Connected,
    Publishing,
    Playing,
    Closed,
}

impl RtmpSession {
    pub fn new(
        id: u32,
        stream: tokio::net::TcpStream,
        ctx: PluginContext,
        sessions: Arc<RwLock<Vec<SessionInfo>>>,
        bytes_received: Arc<std::sync::atomic::AtomicU64>,
    ) -> Self {
        let (read_half, write_half) = stream.into_split();
        Self {
            id,
            state: SessionState::Init,
            stream_key: None,
            app: None,
            transaction_id: 0.0,
            window_ack_size: 2_500_000,
            peer_bandwidth: 2_500_000,
            write_chunk_size: DEFAULT_CHUNK_SIZE,
            bytes_received,
            video_codec: None,
            audio_codec: None,
            ctx,
            sessions,
            reader: tokio::io::BufReader::with_capacity(8192, read_half),
            writer: write_half,
            decoder: ChunkDecoder::new(DEFAULT_CHUNK_SIZE),
        }
    }

    pub fn stream_key(&self) -> Option<&str> {
        self.stream_key.as_deref()
    }

    pub fn state(&self) -> &SessionState {
        &self.state
    }

    pub fn bytes_received(&self) -> u64 {
        self.bytes_received.load(std::sync::atomic::Ordering::Relaxed)
    }

    pub async fn run(&mut self, event_bus: &EventBus) -> Result<()> {
        self.send_initial_messages().await?;

        let mut buf = vec![0u8; 16 * 1024];
        loop {
            let n = self.reader.read(&mut buf).await?;
            if n == 0 {
                break;
            }
            self.bytes_received
                .fetch_add(n as u64, std::sync::atomic::Ordering::Relaxed);

            let messages = match self.decoder.decode(&buf[..n]) {
                Ok(m) => m,
                Err(ChunkError::Malformed(msg)) => {
                    self.ctx.warn(format!("RTMP session {}: malformed chunk: {}", self.id, msg));
                    break;
                }
            };

            for message in messages {
                self.handle_message(&message, event_bus).await?;
            }
        }

        self.state = SessionState::Closed;
        Ok(())
    }

    async fn send_initial_messages(&mut self) -> Result<()> {
        // Window acknowledgement size.
        self.send_control(MSG_WINDOW_ACK_SIZE, self.window_ack_size.to_be_bytes().to_vec())
            .await?;
        // Set peer bandwidth: window size plus the limit type.
        let mut bandwidth = self.peer_bandwidth.to_be_bytes().to_vec();
        bandwidth.push(2);
        self.send_control(MSG_SET_PEER_BANDWIDTH, bandwidth).await?;
        // Ask the peer to use a larger chunk size.
        self.send_control(MSG_SET_CHUNK_SIZE, SERVER_CHUNK_SIZE.to_be_bytes().to_vec())
            .await?;
        self.write_chunk_size = SERVER_CHUNK_SIZE;

        self.ctx.debug(format!("RTMP session {}: {}", self.id, self.window_ack_size
        ));
        Ok(())
    }

    /// Sends a protocol control message on the control chunk stream.
    async fn send_control(&mut self, msg_type_id: u8, payload: Vec<u8>) -> Result<()> {
        self.send_message(msg_type_id, 0, CSID_PROTOCOL_CONTROL, payload.into())
            .await
    }

    /// Sends an AMF command on the command chunk stream.
    async fn send_command(&mut self, payload: BytesMut) -> Result<()> {
        self.send_message(MSG_AMF0_COMMAND, 0, CSID_COMMAND, payload.into())
            .await
    }

    async fn send_message(
        &mut self,
        msg_type_id: u8,
        msg_stream_id: u32,
        chunk_stream_id: u32,
        payload: Bytes,
    ) -> Result<()> {
        let header = ChunkHeader {
            fmt: 0,
            chunk_stream_id,
            timestamp: 0,
            timestamp_delta: 0,
            msg_length: payload.len() as u32,
            msg_type_id,
            msg_stream_id,
        };
        let data = encode_message(&header, &payload, self.write_chunk_size);
        self.writer.write_all(&data).await?;
        self.writer.flush().await?;
        Ok(())
    }

    async fn handle_message(
        &mut self,
        message: &ChunkMessage,
        event_bus: &EventBus,
    ) -> Result<()> {
        match message.header.msg_type_id {
            MSG_SET_CHUNK_SIZE => {
                if message.payload.len() >= 4 {
                    let size = u32::from_be_bytes([
                        message.payload[0],
                        message.payload[1],
                        message.payload[2],
                        message.payload[3],
                    ]);
                    self.decoder.set_chunk_size(size);
                    self.ctx.debug(format!("RTMP session {}: {}", self.id, size));
                }
            }
            MSG_ABORT => {}
            MSG_ACKNOWLEDGEMENT => {}
            MSG_USER_CONTROL => {
                self.handle_user_control(&message.payload);
            }
            MSG_WINDOW_ACK_SIZE => {
                if message.payload.len() >= 4 {
                    self.window_ack_size = u32::from_be_bytes([
                        message.payload[0],
                        message.payload[1],
                        message.payload[2],
                        message.payload[3],
                    ]);
                }
            }
            MSG_SET_PEER_BANDWIDTH => {
                if message.payload.len() >= 4 {
                    self.peer_bandwidth = u32::from_be_bytes([
                        message.payload[0],
                        message.payload[1],
                        message.payload[2],
                        message.payload[3],
                    ]);
                }
            }
            MSG_AUDIO => {
                self.handle_audio(&message.payload, event_bus);
            }
            MSG_VIDEO => {
                self.handle_video(&message.payload, event_bus);
            }
            MSG_AMF0_COMMAND => {
                self.handle_amf_message(&message.payload, event_bus).await?;
            }
            other => {
                self.ctx.debug(format!("RTMP session {}: {}", self.id, other
                ));
            }
        }
        Ok(())
    }

    fn handle_user_control(&mut self, payload: &[u8]) {
        if payload.len() < 6 {
            return;
        }
        let event = u16::from_be_bytes([payload[0], payload[1]]);
        if event == USER_CONTROL_STREAM_BEGIN {
            self.ctx.debug(format!("RTMP session {}: {}", self.id, u32::from_be_bytes([payload[2], payload[3], payload[4], payload[5]]))
            );
        }
    }

    fn handle_audio(&mut self, payload: &[u8], event_bus: &EventBus) {
        if payload.is_empty() {
            return;
        }

        // FLV audio tag: the high nibble of the first byte is the sound format.
        if self.audio_codec.is_none() {
            self.audio_codec = match payload[0] >> 4 {
                10 => Some("aac".to_string()),
                2 => Some("mp3".to_string()),
                7 => Some("g711a".to_string()),
                8 => Some("g711u".to_string()),
                _ => None,
            };
            if self.audio_codec.is_some() {
                self.publish_codec_update(event_bus);
            }
        }

        let stream_key = self.stream_key.clone().unwrap_or_default();
        event_bus.publish_all(Event::EncodedAudio {
            stream_key,
            data: payload.to_vec(),
            timestamp: 0,
        });
    }

    fn handle_video(&mut self, payload: &[u8], event_bus: &EventBus) {
        if payload.is_empty() {
            return;
        }

        // FLV video tag: the low nibble of the first byte is the codec id.
        let frame_type = payload[0] >> 4;
        let codec_id = payload[0] & 0x0f;
        if self.video_codec.is_none() {
            self.video_codec = match codec_id {
                7 => Some("h264".to_string()),
                12 => Some("h265".to_string()),
                4 => Some("vp6".to_string()),
                _ => None,
            };
            if self.video_codec.is_some() {
                self.publish_codec_update(event_bus);
            }
        }

        let stream_key = self.stream_key.clone().unwrap_or_default();
        event_bus.publish_all(Event::EncodedVideo {
            stream_key,
            data: payload.to_vec(),
            timestamp: 0,
            keyframe: frame_type == 1,
        });
    }

    pub async fn handle_amf_message(
        &mut self,
        payload: &[u8],
        event_bus: &EventBus,
    ) -> Result<()> {
        let (command, transaction_id, args) = match parse_command(payload) {
            Some(v) => v,
            None => {
                self.ctx.debug(format!("RTMP session {}: unreadable AMF command", self.id));
                return Ok(());
            }
        };
        self.transaction_id = transaction_id;

        match command.as_str() {
            "connect" => self.handle_connect(transaction_id, &args).await,
            "createStream" => self.handle_create_stream(transaction_id).await,
            "publish" => self.handle_publish(transaction_id, &args, event_bus).await,
            "play" => self.handle_play(transaction_id, &args, event_bus).await,
            "deleteStream" | "FCUnpublish" | "FCUnpublish2" | "releaseStream" | "closeStream" => {
                Ok(())
            }
            "_checkbw" => {
                let out =
                    rtmp_amf::encode_command("_result", transaction_id, &[AmfValue::Null]);
                self.send_command(out).await
            }
            other => {
                self.ctx.debug(format!("RTMP session {}: {}", self.id, other));
                Ok(())
            }
        }
    }

    async fn handle_connect(
        &mut self,
        transaction_id: f64,
        args: &[AmfValue],
    ) -> Result<()> {
        self.state = SessionState::Connecting;

        // The command object carries the application name and the tcUrl.
        if let Some(cmd) = args.first().and_then(|a| a.as_object()) {
            for (key, value) in cmd {
                if key == "app" {
                    if let Some(app) = value.as_str() {
                        self.app = Some(app.trim_end_matches('/').to_string());
                    }
                }
            }
        }

        let properties = AmfValue::Object(vec![
            (
                "fmsVer".to_string(),
                AmfValue::String("FMS/3,0,1,123".to_string()),
            ),
            ("capabilities".to_string(), AmfValue::Number(31.0)),
        ]);
        let information = AmfValue::Object(vec![
            ("level".to_string(), AmfValue::String("status".to_string())),
            (
                "code".to_string(),
                AmfValue::String("NetConnection.Connect.Success".to_string()),
            ),
            (
                "description".to_string(),
                AmfValue::String("Connection succeeded.".to_string()),
            ),
        ]);
        let out = rtmp_amf::encode_command(
            "_result",
            transaction_id,
            &[properties, information],
        );
        self.send_command(out).await?;

        self.state = SessionState::Connected;
        self.ctx.info(format!("RTMP session {}: connect (app={})", self.id, self.app.as_deref().unwrap_or("live")));
        Ok(())
    }

    async fn handle_create_stream(&mut self, transaction_id: f64) -> Result<()> {
        let out = rtmp_amf::encode_command("_result", transaction_id, &[
            AmfValue::Null,
            AmfValue::Number(1.0),
        ]);
        self.send_command(out).await?;
        self.ctx.debug(format!("RTMP session {}: createStream", self.id));
        Ok(())
    }

    async fn handle_publish(
        &mut self,
        transaction_id: f64,
        args: &[AmfValue],
        event_bus: &EventBus,
    ) -> Result<()> {
        let stream_name = extract_stream_name(args);
        let app = self.app.clone().unwrap_or_else(|| "live".to_string());
        let stream_key = match &stream_name {
            Some(name) if !name.is_empty() => format!("{}/{}", app, name),
            _ => app.clone(),
        };

        self.stream_key = Some(stream_key.clone());
        self.state = SessionState::Publishing;
        self.update_session("publishing", Some(stream_key.clone()));

        // Tell the peer the stream is live, then announce it on the event bus.
        let status = rtmp_amf::encode_status(
            transaction_id,
            "status",
            "NetStream.Publish.Start",
            "Start publishing",
        );
        self.send_command(status).await?;
        self.send_stream_begin().await?;

        event_bus.publish_all(Event::StreamPublish {
            stream_key: stream_key.clone(),
            codec_info: CodecInfo {
                video_codec: self.video_codec.clone(),
                audio_codec: self.audio_codec.clone(),
                width: None,
                height: None,
                sample_rate: None,
                channels: None,
            },
        });

        self.ctx.info(format!("RTMP session {}: publishing {}", self.id, stream_key));
        Ok(())
    }

    async fn handle_play(
        &mut self,
        transaction_id: f64,
        args: &[AmfValue],
        event_bus: &EventBus,
    ) -> Result<()> {
        let stream_name = extract_stream_name(args);
        let app = self.app.clone().unwrap_or_else(|| "live".to_string());
        let stream_key = match &stream_name {
            Some(name) if !name.is_empty() => format!("{}/{}", app, name),
            _ => app.clone(),
        };

        self.stream_key = Some(stream_key.clone());
        self.state = SessionState::Playing;
        self.update_session("playing", Some(stream_key.clone()));

        let reset = rtmp_amf::encode_status(
            transaction_id,
            "status",
            "NetStream.Play.Reset",
            "Playing and resetting stream.",
        );
        self.send_command(reset).await?;
        let start = rtmp_amf::encode_status(
            transaction_id,
            "status",
            "NetStream.Play.Start",
            "Started playing stream.",
        );
        self.send_command(start).await?;
        self.send_stream_begin().await?;

        event_bus.publish_all(Event::StreamPlay {
            stream_key: stream_key.clone(),
        });

        self.ctx.info(format!("RTMP session {}: playing {}", self.id, stream_key));
        Ok(())
    }

    /// Updates the session entry the plugin exposes over HTTP.
    fn update_session(&self, state: &str, stream_key: Option<String>) {
        let mut sessions = self.sessions.write();
        if let Some(entry) = sessions.iter_mut().find(|s| s.id == self.id) {
            entry.state = state.to_string();
            if stream_key.is_some() {
                entry.stream_key = stream_key;
            }
        }
    }

    /// Republishes the stream event now that the codecs are known.
    fn publish_codec_update(&self, event_bus: &EventBus) {
        let stream_key = match &self.stream_key {
            Some(key) => key.clone(),
            None => return,
        };
        event_bus.publish_all(Event::StreamPublish {
            stream_key,
            codec_info: CodecInfo {
                video_codec: self.video_codec.clone(),
                audio_codec: self.audio_codec.clone(),
                width: None,
                height: None,
                sample_rate: None,
                channels: None,
            },
        });
    }

    /// Sends the user control message "Stream Begin" for stream id 1.
    async fn send_stream_begin(&mut self) -> Result<()> {
        let mut payload = BytesMut::new();
        payload.put_u16(USER_CONTROL_STREAM_BEGIN);
        payload.put_u32(1);
        self.send_message(MSG_USER_CONTROL, 0, CSID_COMMAND, payload.freeze())
            .await
    }
}

/// Parses an AMF0 command into its name, transaction id and arguments.
fn parse_command(payload: &[u8]) -> Option<(String, f64, Vec<AmfValue>)> {
    let mut offset = 0usize;

    let name = rtmp_amf::parse(payload, &mut offset)
        .ok()?
        .as_str()?
        .to_string();
    let transaction_id = rtmp_amf::parse(payload, &mut offset).ok()?.as_number().unwrap_or(0.0);

    let mut args = Vec::new();
    while offset < payload.len() {
        match rtmp_amf::parse(payload, &mut offset) {
            Ok(value) => args.push(value),
            Err(_) => break,
        }
    }

    Some((name, transaction_id, args))
}

/// Extracts the stream name from a `publish` or `play` command.
///
/// `publish` carries the publishing name as the argument after the command object,
/// `play` carries it inside the command object as `streamName`.
fn extract_stream_name(args: &[AmfValue]) -> Option<String> {
    if let Some(name) = args.get(1).and_then(|a| a.as_str()) {
        return Some(name.to_string());
    }
    if let Some(name) = args
        .first()
        .and_then(|a| a.get("streamName"))
        .and_then(|a| a.as_str())
    {
        return Some(name.to_string());
    }
    args.get(1)
        .and_then(|a| a.as_str())
        .map(|s| s.to_string())
}
