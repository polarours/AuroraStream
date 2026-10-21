#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CodecId {
    H264,
    H265,
    Vp8,
    Vp9,
    Av1,
    Aac,
    Opus,
    Mp3,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MediaType {
    Video,
    Audio,
}

#[derive(Debug, Clone)]
pub struct MediaPacket {
    pub codec_id: CodecId,
    pub media_type: MediaType,
    pub data: Vec<u8>,
    pub pts: i64,
    pub dts: i64,
    pub keyframe: bool,
    pub stream_index: u32,
}

impl MediaPacket {
    pub fn new(
        codec_id: CodecId,
        media_type: MediaType,
        data: Vec<u8>,
        pts: i64,
        dts: i64,
        keyframe: bool,
    ) -> Self {
        Self {
            codec_id,
            media_type,
            data,
            pts,
            dts,
            keyframe,
            stream_index: 0,
        }
    }
}
