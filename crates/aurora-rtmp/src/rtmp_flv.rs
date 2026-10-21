//! Helpers for the FLV payload format that RTMP carries.
//!
//! A publisher sends audio and video messages whose body is exactly an FLV tag body.
//! The server has to understand those bodies well enough to
//!
//! * detect the codec of a stream,
//! * extract the dimensions from the AVC sequence header,
//! * extract the sample rate and channel count from the AAC sequence header, and
//! * rebuild an `onMetaData` script message for the players.

use crate::rtmp_amf::{self, AmfValue};

/// FLV video codec ids.
pub const CODEC_ID_AVC: u8 = 7;
pub const CODEC_ID_HEVC: u8 = 12;

/// FLV audio sound format for AAC.
pub const SOUND_FORMAT_AAC: u8 = 10;

/// Sampling rates indexed by the `samplingFrequencyIndex` of an AudioSpecificConfig.
const SAMPLE_RATES: [u32; 16] = [
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0,
    0, 0,
];

/// Everything the server learned about a stream from its sequence headers.
#[derive(Debug, Clone, Default)]
pub struct StreamMetadata {
    pub video_codec: Option<String>,
    pub audio_codec: Option<String>,
    pub width: u32,
    pub height: u32,
    pub frame_rate: f64,
    pub sample_rate: u32,
    pub channels: u32,
}

/// Parses the body of an FLV video tag.
///
/// Returns the metadata that could be derived from it. For an AVC sequence header the
/// dimensions are read out of the SPS.
pub fn parse_video_tag(body: &[u8], meta: &mut StreamMetadata) {
    if body.len() < 1 {
        return;
    }

    let codec_id = body[0] & 0x0f;

    meta.video_codec = match codec_id {
        CODEC_ID_AVC => Some("h264".to_string()),
        CODEC_ID_HEVC => Some("h265".to_string()),
        _ => None,
    };

    // AVC and HEVC carry an extra packet type byte.
    if codec_id != CODEC_ID_AVC && codec_id != CODEC_ID_HEVC {
        return;
    }
    // The tag body is: frame type and codec id (1 byte), packet type (1 byte),
    // composition time (3 bytes) and then the payload.
    if body.len() < 5 {
        return;
    }

    let packet_type = body[1];
    if packet_type != 0 {
        // Not a sequence header, nothing to derive.
        return;
    }

    let record = &body[5..];
    if codec_id == CODEC_ID_AVC {
        if let Some((width, height)) = parse_avc_sequence_header(record) {
            meta.width = width;
            meta.height = height;
        }
    }
}

/// Parses the body of an FLV audio tag.
pub fn parse_audio_tag(body: &[u8], meta: &mut StreamMetadata) {
    if body.len() < 1 {
        return;
    }

    let sound_format = body[0] >> 4;
    meta.audio_codec = match sound_format {
        SOUND_FORMAT_AAC => Some("aac".to_string()),
        2 => Some("mp3".to_string()),
        7 => Some("g711a".to_string()),
        8 => Some("g711u".to_string()),
        _ => None,
    };

    if sound_format != SOUND_FORMAT_AAC || body.len() < 2 {
        return;
    }
    if body[1] != 0 {
        // Not an AAC sequence header.
        return;
    }

    let config = &body[2..];
    if let Some((sample_rate, channels)) = parse_audio_specific_config(config) {
        meta.sample_rate = sample_rate;
        meta.channels = channels;
    }
}

/// Reads the width and the height out of an AVCDecoderConfigurationRecord.
pub fn parse_avc_sequence_header(record: &[u8]) -> Option<(u32, u32)> {
    if record.len() < 8 {
        return None;
    }

    // configurationVersion(1) profile(1) compat(1) level(1) lengthSize(1) numSPS(1)
    let sps_count = record[5] & 0x1f;
    if sps_count == 0 {
        return None;
    }

    let sps_len = u16::from_be_bytes([record[6], record[7]]) as usize;
    let start: usize = 8;
    let end = start.checked_add(sps_len)?;
    if end > record.len() {
        return None;
    }

    // The parameter set is a NAL unit: one header byte followed by the RBSP.
    let nal = &record[start..end];
    if nal.is_empty() {
        return None;
    }
    let rbsp = remove_emulation_prevention(&nal[1..]);

    parse_sps_dimensions(&rbsp)
}

/// Removes the emulation prevention bytes of an RBSP.
///
/// A `00 00 03` sequence inside the payload is an escaped `00 00` and has to be
/// restored before the bit stream can be read.
fn remove_emulation_prevention(rbsp: &[u8]) -> Vec<u8> {
    let mut out = Vec::with_capacity(rbsp.len());
    let mut zeros = 0u32;

    for byte in rbsp {
        if zeros >= 2 && *byte == 0x03 {
            zeros = 0;
            continue;
        }
        zeros = if *byte == 0 { zeros + 1 } else { 0 };
        out.push(*byte);
    }

    out
}

/// Reads the sample rate and the channel count out of an AudioSpecificConfig.
pub fn parse_audio_specific_config(config: &[u8]) -> Option<(u32, u32)> {
    if config.len() < 2 {
        return None;
    }

    let mut reader = BitReader::new(config);
    let object_type = reader.read_bits(5)?;
    if object_type == 31 {
        // The real value continues in six more bits.
        reader.read_bits(6)?;
    }
    let frequency_index = reader.read_bits(4)?;
    let channels = reader.read_bits(4)?;

    let sample_rate = SAMPLE_RATES
        .get(frequency_index as usize)
        .copied()
        .unwrap_or(0);

    Some((sample_rate, channels))
}

/// Parses an H.264 SPS far enough to get the picture dimensions.
pub fn parse_sps_dimensions(sps: &[u8]) -> Option<(u32, u32)> {
    let mut reader = BitReader::new(sps);

    let profile_idc = reader.read_bits(8)?;
    reader.read_bits(8)?; // constraint flags and reserved
    let _level_idc = reader.read_bits(8)?;
    let _sps_id = reader.read_ue()?;

    let mut chroma_format_idc = 1u32;
    if matches!(
        profile_idc,
        100 | 110 | 122 | 244 | 44 | 83 | 86 | 118 | 128 | 138 | 139 | 134 | 135
    ) {
        chroma_format_idc = reader.read_ue()?;
        if chroma_format_idc == 3 {
            reader.read_bit()?;
        }
        reader.read_ue()?; // bit_depth_luma_minus8
        reader.read_ue()?; // bit_depth_chroma_minus8
        reader.read_bit()?; // qpprime_y_zero_transform_bypass_flag
        if reader.read_bit()? {
            // seq_scaling_matrix_present_flag
            let count = if chroma_format_idc != 3 { 8 } else { 12 };
            for i in 0..count {
                if reader.read_bit()? {
                    skip_scaling_list(&mut reader, if i < 6 { 16 } else { 64 })?;
                }
            }
        }
    }

    reader.read_ue()?; // log2_max_frame_num_minus4
    let pic_order_cnt_type = reader.read_ue()?;
    match pic_order_cnt_type {
        0 => {
            reader.read_ue()?; // log2_max_pic_order_cnt_lsb_minus4
        }
        1 => {
            reader.read_bit()?; // delta_pic_order_always_zero_flag
            reader.read_se()?; // offset_for_non_ref_pic
            reader.read_se()?; // offset_for_top_to_bottom_field
            let cycle = reader.read_ue()?;
            for _ in 0..cycle {
                reader.read_se()?;
            }
        }
        _ => {}
    }

    reader.read_ue()?; // max_num_ref_frames
    reader.read_bit()?; // gaps_in_frame_num_value_allowed_flag

    let width_in_mbs = reader.read_ue()? + 1;
    let height_in_map_units = reader.read_ue()? + 1;
    let frame_mbs_only = reader.read_bit()?;
    if !frame_mbs_only {
        reader.read_bit()?; // mb_adaptive_frame_field_flag
    }
    reader.read_bit()?; // direct_8x8_inference_flag

    let (mut crop_left, mut crop_right, mut crop_top, mut crop_bottom) = (0u32, 0u32, 0u32, 0u32);
    if reader.read_bit()? {
        crop_left = reader.read_ue()?;
        crop_right = reader.read_ue()?;
        crop_top = reader.read_ue()?;
        crop_bottom = reader.read_ue()?;
    }

    let sub_width = if chroma_format_idc == 3 { 1 } else { 2 };
    let sub_height = if chroma_format_idc == 1 { 2 } else { 1 };

    let crop_unit_x = sub_width;
    let crop_unit_y = sub_height * (2 - frame_mbs_only as u32);

    let width = width_in_mbs * 16 - crop_left * crop_unit_x - crop_right * crop_unit_x;
    let height = height_in_map_units * 16 * (2 - frame_mbs_only as u32)
        - crop_top * crop_unit_y
        - crop_bottom * crop_unit_y;

    Some((width, height))
}

fn skip_scaling_list(reader: &mut BitReader, size: u32) -> Option<()> {
    let mut last_scale = 8i32;
    let mut next_scale = 8i32;
    for _ in 0..size {
        if next_scale != 0 {
            let delta = reader.read_se()?;
            next_scale = (last_scale + delta + 256) % 256;
        }
        last_scale = if next_scale == 0 { last_scale } else { next_scale };
    }
    Some(())
}

/// Builds the `onMetaData` script message an RTMP player expects.
///
/// The message is the classic `@setDataFrame` + `onMetaData` + ECMA array triple.
pub fn build_on_meta_data(meta: &StreamMetadata) -> bytes::BytesMut {
    let mut out = bytes::BytesMut::new();
    rtmp_amf::encode(&AmfValue::String("@setDataFrame".to_string()), &mut out);
    rtmp_amf::encode(&AmfValue::String("onMetaData".to_string()), &mut out);
    rtmp_amf::encode(&AmfValue::EcmaArray(metadata_entries(meta)), &mut out);
    out
}

/// Builds the `onMetaData` script tag body of an FLV stream.
///
/// The FLV container does not use the `@setDataFrame` wrapper.
pub fn build_flv_metadata(meta: &StreamMetadata) -> bytes::BytesMut {
    let mut out = bytes::BytesMut::new();
    rtmp_amf::encode(&AmfValue::String("onMetaData".to_string()), &mut out);
    rtmp_amf::encode(&AmfValue::EcmaArray(metadata_entries(meta)), &mut out);
    out
}

/// The properties of the metadata of a stream.
fn metadata_entries(meta: &StreamMetadata) -> Vec<(String, AmfValue)> {
    vec![
        ("duration".to_string(), AmfValue::Number(0.0)),
        (
            "width".to_string(),
            AmfValue::Number(meta.width as f64),
        ),
        (
            "height".to_string(),
            AmfValue::Number(meta.height as f64),
        ),
        (
            "framerate".to_string(),
            AmfValue::Number(if meta.frame_rate > 0.0 {
                meta.frame_rate
            } else {
                25.0
            }),
        ),
        (
            "videocodecid".to_string(),
            AmfValue::Number(match meta.video_codec.as_deref() {
                Some("h264") => 7.0,
                Some("h265") => 12.0,
                _ => 0.0,
            }),
        ),
        (
            "audiocodecid".to_string(),
            AmfValue::Number(match meta.audio_codec.as_deref() {
                Some("aac") => 10.0,
                _ => 0.0,
            }),
        ),
        (
            "audiosamplerate".to_string(),
            AmfValue::Number(meta.sample_rate as f64),
        ),
        (
            "audiosamplesize".to_string(),
            AmfValue::Number(16.0),
        ),
        (
            "audioschannels".to_string(),
            AmfValue::Number(meta.channels as f64),
        ),
        (
            "stereo".to_string(),
            AmfValue::Boolean(meta.channels > 1),
        ),
        ("encoding".to_string(), AmfValue::String("avc1".to_string())),
        ("videodatarate".to_string(), AmfValue::Number(0.0)),
        ("audiodatarate".to_string(), AmfValue::Number(0.0)),
        ("filesize".to_string(), AmfValue::Number(0.0)),
    ]
}

/// A bit reader for the RBSP bit stream of an H.264 parameter set.
struct BitReader<'a> {
    data: &'a [u8],
    bit_position: usize,
}

impl<'a> BitReader<'a> {
    fn new(data: &'a [u8]) -> Self {
        Self {
            data,
            bit_position: 0,
        }
    }

    fn read_bit(&mut self) -> Option<bool> {
        let byte = self.bit_position / 8;
        if byte >= self.data.len() {
            return None;
        }
        let bit = (self.data[byte] >> (7 - (self.bit_position % 8))) & 1;
        self.bit_position += 1;
        Some(bit == 1)
    }

    fn read_bits(&mut self, count: u32) -> Option<u32> {
        let mut value = 0u32;
        for _ in 0..count {
            value = (value << 1) | u32::from(self.read_bit()?);
        }
        Some(value)
    }

    /// Reads an unsigned Exp-Golomb code.
    fn read_ue(&mut self) -> Option<u32> {
        let mut leading_zeros = 0u32;
        while !self.read_bit()? {
            leading_zeros += 1;
            if leading_zeros > 32 {
                return None;
            }
        }
        if leading_zeros == 0 {
            return Some(0);
        }
        let suffix = self.read_bits(leading_zeros)?;
        Some((1u32 << leading_zeros) - 1 + suffix)
    }

    /// Reads a signed Exp-Golomb code.
    fn read_se(&mut self) -> Option<i32> {
        let value = self.read_ue()?;
        let magnitude = ((value + 1) / 2) as i32;
        if value % 2 == 1 {
            Some(magnitude)
        } else {
            Some(-magnitude)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// The AVC sequence header ffmpeg produces for a 640x360 stream.
    ///
    /// Frame type and codec id, packet type, composition time and then the
    /// AVCDecoderConfigurationRecord.
    const AVC_SEQUENCE_HEADER_640X360: [u8; 50] = [
        0x17, 0x00, 0x00, 0x00, 0x00, 0x01, 0xf4, 0x00, 0x1e, 0xff, 0xe1, 0x00, 0x19, 0x67,
        0xf4, 0x00, 0x1e, 0x91, 0x96, 0x80, 0xa0, 0x2f, 0xf8, 0x9c, 0x04, 0x40, 0x00, 0x00,
        0x03, 0x00, 0x40, 0x00, 0x00, 0x0c, 0x83, 0xc5, 0x8b, 0xa8, 0x01, 0x00, 0x05, 0x68,
        0xce, 0x0f, 0x19, 0x20, 0xff, 0xf8, 0xf8, 0x00,
    ];

    /// The AAC sequence header ffmpeg produces for a 44100 Hz stream.
    const AAC_SEQUENCE_HEADER: [u8; 7] = [0xaf, 0x00, 0x12, 0x08, 0x56, 0xe5, 0x00];

    #[test]
    fn reads_the_dimensions_out_of_an_avc_sequence_header() {
        let mut meta = StreamMetadata::default();
        parse_video_tag(&AVC_SEQUENCE_HEADER_640X360, &mut meta);

        assert_eq!(meta.video_codec.as_deref(), Some("h264"));
        assert_eq!(meta.width, 640);
        assert_eq!(meta.height, 360);
    }

    #[test]
    fn reads_the_sample_rate_out_of_an_aac_sequence_header() {
        let mut meta = StreamMetadata::default();
        parse_audio_tag(&AAC_SEQUENCE_HEADER, &mut meta);

        assert_eq!(meta.audio_codec.as_deref(), Some("aac"));
        assert_eq!(meta.sample_rate, 44100);
        // The channel configuration of the config record is 1, so the stream is mono.
        assert_eq!(meta.channels, 1);
    }

    #[test]
    fn ignores_media_frames_that_are_no_sequence_headers() {
        let mut meta = StreamMetadata::default();
        // A regular AVC frame, not the sequence header.
        parse_video_tag(&[0x27, 0x01, 0x00, 0x00, 0x00, 0xaa, 0xbb], &mut meta);
        assert_eq!(meta.width, 0);
        assert_eq!(meta.height, 0);
        assert_eq!(meta.video_codec.as_deref(), Some("h264"));
    }

    #[test]
    fn builds_an_on_meta_data_message() {
        let meta = StreamMetadata {
            video_codec: Some("h264".to_string()),
            audio_codec: Some("aac".to_string()),
            width: 640,
            height: 360,
            sample_rate: 44100,
            channels: 2,
            frame_rate: 25.0,
        };

        let payload = build_on_meta_data(&meta);

        let mut offset = 0;
        let set_data_frame = crate::rtmp_amf::parse(&payload, &mut offset).unwrap();
        assert_eq!(set_data_frame.as_str(), Some("@setDataFrame"));

        let name = crate::rtmp_amf::parse(&payload, &mut offset).unwrap();
        assert_eq!(name.as_str(), Some("onMetaData"));

        let entries = crate::rtmp_amf::parse(&payload, &mut offset).unwrap();
        assert_eq!(entries.get("width").and_then(|v| v.as_number()), Some(640.0));
        assert_eq!(entries.get("height").and_then(|v| v.as_number()), Some(360.0));
        assert_eq!(
            entries.get("videocodecid").and_then(|v| v.as_number()),
            Some(7.0)
        );
        assert_eq!(
            entries.get("audiocodecid").and_then(|v| v.as_number()),
            Some(10.0)
        );
        assert_eq!(
            entries.get("audiosamplerate").and_then(|v| v.as_number()),
            Some(44100.0)
        );
    }
}
