//! Writes the FLV container format.
//!
//! The payloads that arrive over RTMP are already FLV tag bodies, so muxing them into
//! an FLV stream is a matter of wrapping every body in a tag header.

use bytes::{BufMut, Bytes, BytesMut};

/// Tag type of an audio tag.
pub const TAG_AUDIO: u8 = 8;
/// Tag type of a video tag.
pub const TAG_VIDEO: u8 = 9;
/// Tag type of a script data tag, used for the metadata.
pub const TAG_SCRIPT: u8 = 18;

/// The FLV file header.
pub fn flv_header() -> Bytes {
    let mut out = BytesMut::with_capacity(13);
    out.put_slice(b"FLV");
    out.put_u8(1); // version
    out.put_u8(0x05); // flags: audio and video present
    out.put_u32(9); // offset of the first tag header
    out.put_u32(0); // size of the previous tag
    out.freeze()
}

/// Wraps a tag body into an FLV tag.
pub fn flv_tag(tag_type: u8, timestamp: u32, data: &[u8]) -> Bytes {
    let mut out = BytesMut::with_capacity(data.len() + 16);

    out.put_u8(tag_type);
    put_u24(&mut out, data.len() as u32);
    put_u24(&mut out, timestamp & 0x00FF_FFFF);
    out.put_u8((timestamp >> 24) as u8); // extended timestamp
    put_u24(&mut out, 0); // stream id
    out.put_slice(data);
    out.put_u32((data.len() + 11) as u32); // size of the previous tag

    out.freeze()
}

fn put_u24(buf: &mut BytesMut, value: u32) {
    buf.put_u8((value >> 16) as u8);
    buf.put_u8((value >> 8) as u8);
    buf.put_u8(value as u8);
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn writes_the_file_header() {
        let header = flv_header();
        assert_eq!(&header[..3], b"FLV");
        assert_eq!(header[3], 1);
        assert_eq!(header[4], 0x05);
        assert_eq!(u32::from_be_bytes([header[5], header[6], header[7], header[8]]), 9);
    }

    /// The three bytes of the timestamp field.
    fn timestamp_of(tag: &[u8]) -> u32 {
        ((tag[4] as u32) << 16) | ((tag[5] as u32) << 8) | (tag[6] as u32)
    }

    #[test]
    fn writes_a_tag() {
        // The layout is: type (1), data size (3), timestamp (3), extended timestamp (1),
        // stream id (3), data, size of the previous tag (4).
        let tag = flv_tag(TAG_VIDEO, 0, b"abc");
        assert_eq!(tag[0], TAG_VIDEO);
        // The three byte size field, padded to a big endian u32.
        assert_eq!(u32::from_be_bytes([0, tag[1], tag[2], tag[3]]), 3);
        assert_eq!(timestamp_of(&tag), 0);
        assert_eq!(tag[7], 0);
        assert_eq!(&tag[8..11], &[0, 0, 0]); // stream id
        assert_eq!(&tag[11..14], b"abc");
        assert_eq!(u32::from_be_bytes([tag[14], tag[15], tag[16], tag[17]]), 14);
    }

    #[test]
    fn splits_a_large_timestamp() {
        let tag = flv_tag(TAG_AUDIO, 0x0102_0304, b"");
        // The lower 24 bits go into the timestamp, the rest into the extended byte.
        assert_eq!(timestamp_of(&tag), 0x020304);
        assert_eq!(tag[7], 0x01);
    }
}
