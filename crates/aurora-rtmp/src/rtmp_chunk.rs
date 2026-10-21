use std::collections::HashMap;

use bytes::{BufMut, Bytes, BytesMut};

/// Chunk stream used for protocol control messages (set chunk size, window ack size,
/// set peer bandwidth). Fixed by the RTMP specification.
pub const CSID_PROTOCOL_CONTROL: u32 = 2;
/// Chunk stream used for user control messages and AMF commands.
pub const CSID_COMMAND: u32 = 3;

/// Largest chunk stream id that fits into the one byte basic header.
const MAX_SHORT_CSID: u32 = 63;
/// Largest chunk stream id that fits into the two byte basic header.
const MAX_MEDIUM_CSID: u32 = 319;

/// Default chunk size before the peer negotiates one.
pub const DEFAULT_CHUNK_SIZE: u32 = 128;

#[derive(Debug, Clone)]
pub struct ChunkHeader {
    /// 0 = full header, 1 = delta + length + type, 2 = delta only, 3 = continuation.
    pub fmt: u8,
    pub chunk_stream_id: u32,
    pub timestamp: u32,
    pub timestamp_delta: u32,
    pub msg_length: u32,
    pub msg_type_id: u8,
    pub msg_stream_id: u32,
}

impl Default for ChunkHeader {
    fn default() -> Self {
        Self {
            fmt: 0,
            chunk_stream_id: CSID_COMMAND,
            timestamp: 0,
            timestamp_delta: 0,
            msg_length: 0,
            msg_type_id: 0,
            msg_stream_id: 0,
        }
    }
}

#[derive(Debug, Clone)]
pub struct ChunkMessage {
    pub header: ChunkHeader,
    pub payload: Bytes,
}

impl ChunkMessage {
    pub fn new(msg_type_id: u8, msg_stream_id: u32, payload: Bytes) -> Self {
        Self::with_csid(msg_type_id, msg_stream_id, CSID_COMMAND, payload)
    }

    pub fn with_csid(
        msg_type_id: u8,
        msg_stream_id: u32,
        chunk_stream_id: u32,
        payload: Bytes,
    ) -> Self {
        Self {
            header: ChunkHeader {
                fmt: 0,
                chunk_stream_id,
                timestamp: 0,
                timestamp_delta: 0,
                msg_length: payload.len() as u32,
                msg_type_id,
                msg_stream_id,
            },
            payload,
        }
    }
}

#[derive(Debug)]
pub enum ChunkError {
    /// The peer sent a chunk we cannot make sense of; the connection is unusable.
    Malformed(String),
}

/// Per chunk stream reassembly state.
///
/// RTMP allows a message to be split over several chunks. The header of the first chunk
/// describes the message; continuation chunks (fmt 3) inherit it.
#[derive(Debug, Default)]
struct ChunkStream {
    timestamp: u32,
    timestamp_delta: u32,
    msg_length: u32,
    msg_type_id: u8,
    msg_stream_id: u32,
    payload: Vec<u8>,
}

/// Streaming RTMP chunk decoder.
///
/// Feed it whatever arrives on the socket; it returns the messages that became complete.
#[derive(Debug)]
pub struct ChunkDecoder {
    chunk_size: u32,
    pending: Vec<u8>,
    streams: HashMap<u32, ChunkStream>,
}

impl ChunkDecoder {
    pub fn new(chunk_size: u32) -> Self {
        Self {
            chunk_size,
            pending: Vec::new(),
            streams: HashMap::new(),
        }
    }

    pub fn set_chunk_size(&mut self, chunk_size: u32) {
        self.chunk_size = chunk_size.max(1);
    }

    /// Appends `data` to the internal buffer and returns every message that is now
    /// complete.
    pub fn decode(&mut self, data: &[u8]) -> Result<Vec<ChunkMessage>, ChunkError> {
        self.pending.extend_from_slice(data);

        let mut messages = Vec::new();
        loop {
            match self.decode_one(&mut messages)? {
                true => continue,
                false => break,
            }
        }
        Ok(messages)
    }

    /// Decodes a single chunk. Returns `Ok(true)` when progress was made.
    fn decode_one(&mut self, out: &mut Vec<ChunkMessage>) -> Result<bool, ChunkError> {
        if self.pending.is_empty() {
            return Ok(false);
        }

        // ---- basic header ----
        let b0 = self.pending[0];
        let fmt = (b0 >> 6) & 0x03;
        let csid_raw = (b0 & 0x3f) as u32;

        let (chunk_stream_id, mut offset) = if csid_raw == 0 {
            if self.pending.len() < 2 {
                return Ok(false);
            }
            (64 + self.pending[1] as u32, 2)
        } else if csid_raw == 1 {
            if self.pending.len() < 3 {
                return Ok(false);
            }
            (
                64 + self.pending[1] as u32 + self.pending[2] as u32 * 256,
                3,
            )
        } else {
            (csid_raw, 1)
        };

        // ---- message header ----
        // fmt 1/2/3 inherit missing fields from the previous chunk on this stream.
        let prev = self.streams.get(&chunk_stream_id).map(|s| {
            (
                s.timestamp,
                s.timestamp_delta,
                s.msg_length,
                s.msg_type_id,
                s.msg_stream_id,
                s.payload.len(),
            )
        });

        let (mut timestamp, mut timestamp_delta, msg_length, msg_type_id, msg_stream_id) =
            match fmt {
                0 => {
                    if self.pending.len() < offset + 11 {
                        return Ok(false);
                    }
                    let ts = read_u24(&self.pending, offset);
                    let len = read_u24(&self.pending, offset + 3);
                    let ty = self.pending[offset + 6];
                    let sid = read_u32_le(&self.pending, offset + 7);
                    offset += 11;
                    (ts, 0u32, len, ty, sid)
                }
                1 => {
                    if self.pending.len() < offset + 7 {
                        return Ok(false);
                    }
                    let delta = read_u24(&self.pending, offset);
                    let len = read_u24(&self.pending, offset + 3);
                    let ty = self.pending[offset + 6];
                    offset += 7;
                    let base = prev.map(|p| p.0).unwrap_or(0);
                    let sid = prev.map(|p| p.4).unwrap_or(0);
                    (base.wrapping_add(delta), delta, len, ty, sid)
                }
                2 => {
                    if self.pending.len() < offset + 3 {
                        return Ok(false);
                    }
                    let delta = read_u24(&self.pending, offset);
                    offset += 3;
                    let p = prev.ok_or_else(|| {
                        ChunkError::Malformed(
                            "fmt=2 chunk without a preceding chunk header".to_string(),
                        )
                    })?;
                    (p.0.wrapping_add(delta), delta, p.2, p.3, p.4)
                }
                _ => {
                    let p = prev.ok_or_else(|| {
                        ChunkError::Malformed(
                            "fmt=3 continuation without a preceding chunk header".to_string(),
                        )
                    })?;
                    (p.0, p.1, p.2, p.3, p.4)
                }
            };

        // ---- extended timestamp ----
        let extended = (fmt == 0 && timestamp == 0x00FF_FFFF)
            || ((fmt == 1 || fmt == 2) && timestamp_delta == 0x00FF_FFFF);
        if extended {
            if self.pending.len() < offset + 4 {
                return Ok(false);
            }
            let ext = read_u32(&self.pending, offset);
            offset += 4;
            if fmt == 0 {
                timestamp = ext;
            } else {
                let base = if fmt == 1 {
                    prev.map(|p| p.0).unwrap_or(0)
                } else {
                    prev.map(|p| p.0).unwrap_or(0)
                };
                timestamp_delta = ext;
                timestamp = base.wrapping_add(ext);
            }
        }

        // ---- payload ----
        let state = self.streams.entry(chunk_stream_id).or_default();

        // A fmt 0/1 chunk starts a new message, everything else continues the current one.
        if fmt == 0 || fmt == 1 {
            state.payload.clear();
        } else if state.payload.len() as u32 >= state.msg_length {
            // A continuation arriving on a stream whose previous message already
            // completed starts a new message reusing the previous header.
            state.payload.clear();
        }

        state.timestamp = timestamp;
        state.timestamp_delta = timestamp_delta;
        state.msg_length = msg_length;
        state.msg_type_id = msg_type_id;
        state.msg_stream_id = msg_stream_id;

        if state.payload.len() as u32 > msg_length {
            return Err(ChunkError::Malformed(format!(
                "chunk stream {} exceeded the announced message length",
                chunk_stream_id
            )));
        }

        let remaining = msg_length as usize - state.payload.len();
        let take = remaining.min(self.chunk_size as usize);

        if self.pending.len() < offset + take {
            // Not enough payload bytes buffered yet.
            return Ok(false);
        }

        state
            .payload
            .extend_from_slice(&self.pending[offset..offset + take]);
        self.pending.drain(..offset + take);

        if state.payload.len() as u32 == msg_length {
            let payload = Bytes::from(std::mem::take(&mut state.payload));
            out.push(ChunkMessage {
                header: ChunkHeader {
                    fmt,
                    chunk_stream_id,
                    timestamp,
                    timestamp_delta,
                    msg_length,
                    msg_type_id,
                    msg_stream_id,
                },
                payload,
            });
        }

        Ok(true)
    }
}

/// Encodes a complete message, splitting the payload into `chunk_size` sized chunks.
///
/// The first chunk carries the full message header, the continuations use fmt 3.
pub fn encode_message(header: &ChunkHeader, payload: &[u8], chunk_size: u32) -> BytesMut {
    let chunk_size = chunk_size.max(1) as usize;
    let csid = header.chunk_stream_id;
    let mut out = BytesMut::with_capacity(payload.len() + 16);

    let mut first = true;
    let mut offset = 0usize;
    loop {
        let end = (offset + chunk_size).min(payload.len());
        let fmt = if first { 0 } else { 3 };

        write_basic_header(&mut out, fmt, csid);

        if first {
            write_u24(&mut out, header.timestamp);
            write_u24(&mut out, payload.len() as u32);
            out.put_u8(header.msg_type_id);
            out.put_u32_le(header.msg_stream_id);
            if header.timestamp >= 0x00FF_FFFF {
                out.put_u32(header.timestamp);
            }
        }

        out.put_slice(&payload[offset..end]);
        offset = end;
        first = false;

        if offset >= payload.len() {
            break;
        }
    }

    out
}

fn write_basic_header(out: &mut BytesMut, fmt: u8, csid: u32) {
    let fmt = fmt & 0x03;
    if csid >= 2 && csid <= MAX_SHORT_CSID {
        out.put_u8((fmt << 6) | csid as u8);
    } else if csid >= 64 && csid <= MAX_MEDIUM_CSID {
        out.put_u8(fmt << 6);
        out.put_u8((csid - 64) as u8);
    } else {
        out.put_u8((fmt << 6) | 0x01);
        let id = csid.saturating_sub(64);
        out.put_u8((id & 0xff) as u8);
        out.put_u8(((id >> 8) & 0xff) as u8);
    }
}

fn read_u24(data: &[u8], offset: usize) -> u32 {
    ((data[offset] as u32) << 16) | ((data[offset + 1] as u32) << 8) | (data[offset + 2] as u32)
}

fn read_u32(data: &[u8], offset: usize) -> u32 {
    ((data[offset] as u32) << 24)
        | ((data[offset + 1] as u32) << 16)
        | ((data[offset + 2] as u32) << 8)
        | (data[offset + 3] as u32)
}

fn read_u32_le(data: &[u8], offset: usize) -> u32 {
    ((data[offset + 3] as u32) << 24)
        | ((data[offset + 2] as u32) << 16)
        | ((data[offset + 1] as u32) << 8)
        | (data[offset] as u32)
}

fn write_u24(buf: &mut BytesMut, val: u32) {
    buf.put_u8((val >> 16) as u8);
    buf.put_u8((val >> 8) as u8);
    buf.put_u8(val as u8);
}
