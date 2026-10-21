use std::sync::Arc;

use bytes::{BufMut, Bytes, BytesMut};
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::tcp::{OwnedReadHalf, OwnedWriteHalf};
use tokio::sync::mpsc;

use aurora_core::error::Result;
use aurora_core::event_bus::{CodecInfo, Event, EventBus};
use aurora_core::plugin::PluginContext;
use parking_lot::RwLock;

use crate::rtmp_amf::{self, AmfValue};
use crate::rtmp_chunk::{
    encode_message, ChunkDecoder, ChunkError, ChunkHeader, ChunkMessage, CSID_AUDIO, CSID_COMMAND,
    CSID_DATA, CSID_PROTOCOL_CONTROL, CSID_VIDEO, DEFAULT_CHUNK_SIZE,
};
use crate::rtmp_flv::{self, StreamMetadata};
use crate::stream_registry::{MediaPacket, StreamEvent, StreamRegistry};

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
const MSG_AMF0_DATA: u8 = 18;

/// User control event: stream begin.
const USER_CONTROL_STREAM_BEGIN: u16 = 0;

/// The stream id the server hands out for the first `createStream`.
const STREAM_ID: u32 = 1;

/// Something the session wants to send to the peer.
#[derive(Debug)]
enum Outgoing {
    Message {
        msg_type_id: u8,
        msg_stream_id: u32,
        chunk_stream_id: u32,
        timestamp: u32,
        payload: Bytes,
    },
}

/// State of a single RTMP connection, exposed through the plugin HTTP route.
#[derive(Debug, Clone)]
pub struct SessionInfo {
    pub id: u32,
    pub peer: String,
    pub state: String,
    pub stream_key: Option<String>,
    pub bytes_received: Arc<std::sync::atomic::AtomicU64>,
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
    registry: Arc<StreamRegistry>,
    /// Metadata derived from the sequence headers of a published stream.
    metadata: StreamMetadata,
    /// The publisher side of this session.
    publisher: bool,
    reader: tokio::io::BufReader<OwnedReadHalf>,
    /// The socket half the writer task owns.
    writer: Option<OwnedWriteHalf>,
    /// Outgoing queue, drained by the writer task.
    out_tx: mpsc::UnboundedSender<Outgoing>,
    out_rx: Option<mpsc::UnboundedReceiver<Outgoing>>,
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
        registry: Arc<StreamRegistry>,
    ) -> Self {
        let (read_half, write_half) = stream.into_split();
        let (out_tx, out_rx) = mpsc::unbounded_channel();

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
            registry,
            metadata: StreamMetadata::default(),
            publisher: false,
            reader: tokio::io::BufReader::with_capacity(8192, read_half),
            writer: Some(write_half),
            out_tx,
            out_rx: Some(out_rx),
            decoder: ChunkDecoder::new(DEFAULT_CHUNK_SIZE),
        }
    }

    pub fn stream_key(&self) -> Option<&str> {
        self.stream_key.as_deref()
    }

    pub fn state(&self) -> &SessionState {
        &self.state
    }

    /// Whether this session publishes the stream instead of playing it.
    pub fn is_publisher(&self) -> bool {
        self.publisher
    }

    pub fn bytes_received(&self) -> u64 {
        self.bytes_received.load(std::sync::atomic::Ordering::Relaxed)
    }

    pub async fn run(&mut self, event_bus: &EventBus) -> Result<()> {
        self.send_initial_messages().await?;

        // The writer runs in its own task so that the session can keep reading the
        // socket while media frames are queued for the peer.
        let chunk_size = self.write_chunk_size;
        let session_id = self.id;
        if let Some(mut out_rx) = self.out_rx.take() {
            let mut writer = self.writer.take();
            tokio::spawn(async move {
                while let Some(outgoing) = out_rx.recv().await {
                    let Some(writer) = writer.as_mut() else {
                        break;
                    };
                    if let Err(e) = write_outgoing(writer, &outgoing, chunk_size).await {
                        tracing::debug!("RTMP session {}: write failed: {}", session_id, e);
                        break;
                    }
                }
            });
        }

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

        // A publisher that goes away takes its stream with it.
        if self.publisher {
            if let Some(key) = &self.stream_key {
                self.registry.remove(key);
            }
        }

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
        self.send_message(msg_type_id, 0, CSID_PROTOCOL_CONTROL, 0, payload.into())
            .await
    }

    /// Sends an AMF command on the command chunk stream.
    async fn send_command(&mut self, payload: BytesMut) -> Result<()> {
        self.send_message(MSG_AMF0_COMMAND, 0, CSID_COMMAND, 0, payload.into())
            .await
    }

    /// Queues a message for the peer.
    async fn send_message(
        &mut self,
        msg_type_id: u8,
        msg_stream_id: u32,
        chunk_stream_id: u32,
        timestamp: u32,
        payload: Bytes,
    ) -> Result<()> {
        let _ = self.out_tx.send(Outgoing::Message {
            msg_type_id,
            msg_stream_id,
            chunk_stream_id,
            timestamp,
            payload,
        });
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
                self.handle_audio(&message.payload, message.header.timestamp, event_bus);
            }
            MSG_VIDEO => {
                self.handle_video(&message.payload, message.header.timestamp, event_bus);
            }
            MSG_AMF0_COMMAND => {
                self.handle_amf_message(&message.payload, event_bus).await?;
            }
            other => {
                self.ctx.debug(format!("RTMP session {}: unhandled message type {}", self.id, other
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
            self.ctx.debug(format!("RTMP session {}: peer began stream {}", self.id, u32::from_be_bytes([payload[2], payload[3], payload[4], payload[5]])
            ));
        }
    }

    fn handle_audio(&mut self, payload: &[u8], timestamp: u32, event_bus: &EventBus) {
        if payload.is_empty() {
            return;
        }

        let is_sequence_header = payload.len() > 1 && payload[0] >> 4 == 10 && payload[1] == 0;

        rtmp_flv::parse_audio_tag(payload, &mut self.metadata);
        if self.audio_codec.is_none() {
            self.audio_codec = self.metadata.audio_codec.clone();
            if self.audio_codec.is_some() {
                self.publish_codec_update(event_bus);
            }
        }

        if let Some(key) = self.stream_key.clone() {
            self.registry.ensure(&key);
            self.registry.set_metadata(&key, self.metadata.clone());
            if is_sequence_header {
                self.registry.set_audio_header(&key, payload.to_vec());
            }
            self.registry.broadcast(
                &key,
                MediaPacket {
                    msg_type_id: MSG_AUDIO,
                    timestamp,
                    payload: payload.to_vec(),
                },
            );
        }

        let stream_key = self.stream_key.clone().unwrap_or_default();
        event_bus.publish_all(Event::EncodedAudio {
            stream_key,
            data: payload.to_vec(),
            timestamp,
        });
    }

    fn handle_video(&mut self, payload: &[u8], timestamp: u32, event_bus: &EventBus) {
        if payload.is_empty() {
            return;
        }

        // FLV video tag: the low nibble of the first byte is the codec id.
        let frame_type = payload[0] >> 4;
        let codec_id = payload[0] & 0x0f;
        // AVC and HEVC carry an extra packet type byte, 0 marks the sequence header.
        let is_sequence_header = matches!(codec_id, 7 | 12) && payload.len() > 1 && payload[1] == 0;

        rtmp_flv::parse_video_tag(payload, &mut self.metadata);
        if self.video_codec.is_none() {
            self.video_codec = self.metadata.video_codec.clone();
            if self.video_codec.is_some() {
                self.publish_codec_update(event_bus);
            }
        }

        if let Some(key) = self.stream_key.clone() {
            self.registry.ensure(&key);
            self.registry.set_metadata(&key, self.metadata.clone());
            if is_sequence_header {
                self.registry.set_video_header(&key, payload.to_vec());
            }
            self.registry.broadcast(
                &key,
                MediaPacket {
                    msg_type_id: MSG_VIDEO,
                    timestamp,
                    payload: payload.to_vec(),
                },
            );
        }

        let stream_key = self.stream_key.clone().unwrap_or_default();
        event_bus.publish_all(Event::EncodedVideo {
            stream_key,
            data: payload.to_vec(),
            timestamp,
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
                self.ctx.debug(format!("RTMP session {}: unknown AMF command {}", self.id, other));
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
        let out =
            rtmp_amf::encode_command("_result", transaction_id, &[properties, information]);
        self.send_command(out).await?;

        self.state = SessionState::Connected;
        self.ctx.info(format!(
            "RTMP session {}: connect (app={})",
            self.id,
            self.app.as_deref().unwrap_or("live")
        ));
        Ok(())
    }

    async fn handle_create_stream(&mut self, transaction_id: f64) -> Result<()> {
        let out = rtmp_amf::encode_command("_result", transaction_id, &[
            AmfValue::Null,
            AmfValue::Number(STREAM_ID as f64),
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
        self.publisher = true;
        self.update_session("publishing", Some(stream_key.clone()));

        // Make the stream known so that players can subscribe to it.
        self.registry.ensure(&stream_key);

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
            codec_info: self.codec_info(),
        });

        self.ctx
            .info(format!("RTMP session {}: publishing {}", self.id, stream_key));
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

        // Subscribe to the live stream and hand the player the metadata and the cached
        // sequence headers before the live frames start.
        match self.registry.subscribe(&stream_key) {
            Some((snapshot, mut media_rx)) => {
                self.metadata = snapshot.metadata.clone();

                let meta_data = rtmp_flv::build_on_meta_data(&snapshot.metadata);
                self.send_message(
                    MSG_AMF0_DATA,
                    STREAM_ID,
                    CSID_DATA,
                    0,
                    meta_data.into(),
                )
                .await?;

                if let Some(video_header) = snapshot.video_header {
                    self.send_message(
                        MSG_VIDEO,
                        STREAM_ID,
                        CSID_VIDEO,
                        0,
                        video_header.into(),
                    )
                    .await?;
                }
                if let Some(audio_header) = snapshot.audio_header {
                    self.send_message(
                        MSG_AUDIO,
                        STREAM_ID,
                        CSID_AUDIO,
                        0,
                        audio_header.into(),
                    )
                    .await?;
                }

                // Forward the live frames to the player.
                let out_tx = self.out_tx.clone();
                let session_id = self.id;
                let ctx = self.ctx.clone();
                tokio::spawn(async move {
                    while let Some(event) = media_rx.recv().await {
                        match event {
                            StreamEvent::Packet(packet) => {
                                let (msg_type_id, chunk_stream_id) =
                                    if packet.msg_type_id == MSG_AUDIO {
                                        (MSG_AUDIO, CSID_AUDIO)
                                    } else {
                                        (MSG_VIDEO, CSID_VIDEO)
                                    };
                                let outgoing = Outgoing::Message {
                                    msg_type_id,
                                    msg_stream_id: STREAM_ID,
                                    chunk_stream_id,
                                    timestamp: packet.timestamp,
                                    payload: packet.payload.into(),
                                };
                                if out_tx.send(outgoing).is_err() {
                                    break;
                                }
                            }
                            StreamEvent::Ended => {
                                // The publisher stopped, tell the player.
                                let status = rtmp_amf::encode_status(
                                    0.0,
                                    "status",
                                    "NetStream.Play.UnpublishNotify",
                                    "The stream is now unpublished.",
                                );
                                let outgoing = Outgoing::Message {
                                    msg_type_id: MSG_AMF0_COMMAND,
                                    msg_stream_id: STREAM_ID,
                                    chunk_stream_id: CSID_COMMAND,
                                    timestamp: 0,
                                    payload: status.into(),
                                };
                                let _ = out_tx.send(outgoing);
                                break;
                            }
                        }
                    }
                    ctx.debug(format!("RTMP session {}: media source closed", session_id));
                });

                self.ctx
                    .info(format!("RTMP session {}: playing {}", self.id, stream_key));
            }
            None => {
                // The stream does not exist. Tell the peer that the play failed.
                let status = rtmp_amf::encode_status(
                    transaction_id,
                    "error",
                    "NetStream.Play.StreamNotFound",
                    "The stream does not exist.",
                );
                self.send_command(status).await?;
                self.ctx.warn(format!(
                    "RTMP session {}: play of unknown stream {}",
                    self.id, stream_key
                ));
                return Ok(());
            }
        }

        event_bus.publish_all(Event::StreamPlay {
            stream_key: stream_key.clone(),
        });

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
            codec_info: self.codec_info(),
        });
    }

    /// The codec information of the stream as far as it is known.
    fn codec_info(&self) -> CodecInfo {
        CodecInfo {
            video_codec: self.video_codec.clone(),
            audio_codec: self.audio_codec.clone(),
            width: if self.metadata.width > 0 {
                Some(self.metadata.width)
            } else {
                None
            },
            height: if self.metadata.height > 0 {
                Some(self.metadata.height)
            } else {
                None
            },
            sample_rate: if self.metadata.sample_rate > 0 {
                Some(self.metadata.sample_rate)
            } else {
                None
            },
            channels: if self.metadata.channels > 0 {
                Some(self.metadata.channels)
            } else {
                None
            },
        }
    }

    /// Sends the user control message "Stream Begin" for stream id 1.
    async fn send_stream_begin(&mut self) -> Result<()> {
        let mut payload = BytesMut::new();
        payload.put_u16(USER_CONTROL_STREAM_BEGIN);
        payload.put_u32(STREAM_ID);
        self.send_message(MSG_USER_CONTROL, 0, CSID_COMMAND, 0, payload.freeze())
            .await
    }
}

/// Writes one outgoing message to the socket.
async fn write_outgoing(
    writer: &mut OwnedWriteHalf,
    outgoing: &Outgoing,
    chunk_size: u32,
) -> std::io::Result<()> {
    let Outgoing::Message {
        msg_type_id,
        msg_stream_id,
        chunk_stream_id,
        timestamp,
        payload,
    } = outgoing;

    let header = ChunkHeader {
        fmt: 0,
        chunk_stream_id: *chunk_stream_id,
        timestamp: *timestamp,
        timestamp_delta: 0,
        msg_length: payload.len() as u32,
        msg_type_id: *msg_type_id,
        msg_stream_id: *msg_stream_id,
    };
    let data = encode_message(&header, payload, chunk_size);
    writer.write_all(&data).await?;
    writer.flush().await?;
    Ok(())
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
