use std::os::raw::c_int;
use std::ptr;

use crate::aurora_media_sys;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PixelFormat {
    Yuv420p,
    Nv12,
    Rgb24,
    Rgba,
}

impl PixelFormat {
    pub fn as_c_int(self) -> c_int {
        match self {
            Self::Yuv420p => 0,
            Self::Nv12 => 1,
            Self::Rgb24 => 2,
            Self::Rgba => 3,
        }
    }
}

#[derive(Debug, Clone)]
pub struct CodecParams {
    pub codec_id: CodecId,
    pub media_type: MediaType,
    pub width: Option<u32>,
    pub height: Option<u32>,
    pub sample_rate: Option<u32>,
    pub channels: Option<u32>,
    pub bit_rate: Option<u32>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CodecId {
    H264,
    H265,
    Vp8,
    Vp9,
    Aac,
    Opus,
    Mp3,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MediaType {
    Video,
    Audio,
}

impl CodecId {
    pub fn as_c_int(self) -> c_int {
        match self {
            Self::H264 => 0,
            Self::H265 => 1,
            Self::Vp8 => 2,
            Self::Vp9 => 3,
            Self::Aac => 100,
            Self::Opus => 101,
            Self::Mp3 => 102,
        }
    }
}

impl MediaType {
    pub fn as_c_int(self) -> c_int {
        match self {
            Self::Video => 0,
            Self::Audio => 1,
        }
    }
}

pub struct Decoder {
    handle: *mut std::ffi::c_void,
}

unsafe impl Send for Decoder {}
unsafe impl Sync for Decoder {}

impl Decoder {
    pub fn new(params: &CodecParams) -> Option<Self> {
        let c_params = aurora_media_sys::AuroraCodecParams {
            codec_id: params.codec_id.as_c_int(),
            media_type: params.media_type.as_c_int(),
            width: params.width.unwrap_or(0) as c_int,
            height: params.height.unwrap_or(0) as c_int,
            sample_rate: params.sample_rate.unwrap_or(0) as c_int,
            channels: params.channels.unwrap_or(0) as c_int,
            bit_rate: params.bit_rate.unwrap_or(0) as c_int,
        };

        unsafe {
            let handle = aurora_media_sys::aurora_decoder_create(&c_params);
            if handle.is_null() {
                None
            } else {
                Some(Self { handle })
            }
        }
    }

    pub fn decode(&self, data: &[u8]) -> Option<Frame> {
        let mut frame = aurora_media_sys::AuroraFrame::default();

        unsafe {
            let ret = aurora_media_sys::aurora_decoder_decode(
                self.handle,
                data.as_ptr(),
                data.len() as c_int,
                &mut frame,
            );

            if ret < 0 {
                return None;
            }

            Some(Frame::from_raw(&frame))
        }
    }
}

impl Drop for Decoder {
    fn drop(&mut self) {
        unsafe {
            aurora_media_sys::aurora_decoder_destroy(self.handle);
        }
    }
}

pub struct Encoder {
    handle: *mut std::ffi::c_void,
}

unsafe impl Send for Encoder {}
unsafe impl Sync for Encoder {}

impl Encoder {
    pub fn new(params: &CodecParams) -> Option<Self> {
        let c_params = aurora_media_sys::AuroraCodecParams {
            codec_id: params.codec_id.as_c_int(),
            media_type: params.media_type.as_c_int(),
            width: params.width.unwrap_or(0) as c_int,
            height: params.height.unwrap_or(0) as c_int,
            sample_rate: params.sample_rate.unwrap_or(0) as c_int,
            channels: params.channels.unwrap_or(0) as c_int,
            bit_rate: params.bit_rate.unwrap_or(0) as c_int,
        };

        unsafe {
            let handle = aurora_media_sys::aurora_encoder_create(&c_params);
            if handle.is_null() {
                None
            } else {
                Some(Self { handle })
            }
        }
    }

    pub fn encode(&self, frame: &Frame) -> Option<(Vec<u8>, bool)> {
        let c_frame = frame.to_raw();

        unsafe {
            let mut out_data: *mut u8 = ptr::null_mut();
            let mut out_size: c_int = 0;
            let mut keyframe: c_int = 0;

            let ret = aurora_media_sys::aurora_encoder_encode(
                self.handle,
                &c_frame,
                &mut out_data,
                &mut out_size,
                &mut keyframe,
            );

            if ret < 0 || out_data.is_null() {
                return None;
            }

            let data = std::slice::from_raw_parts(out_data, out_size as usize).to_vec();
            Some((data, keyframe != 0))
        }
    }
}

impl Drop for Encoder {
    fn drop(&mut self) {
        unsafe {
            aurora_media_sys::aurora_encoder_destroy(self.handle);
        }
    }
}

#[derive(Debug, Clone)]
pub struct Frame {
    pub pixel_format: PixelFormat,
    pub width: u32,
    pub height: u32,
    pub data: Vec<Vec<u8>>,
    pub linesize: [i32; 4],
    pub pts: i64,
}

impl Frame {
    pub fn new(pixel_format: PixelFormat, width: u32, height: u32, pts: i64) -> Self {
        Self {
            pixel_format,
            width,
            height,
            data: vec![vec![0u8; (width * height) as usize * 4]],
            linesize: [0; 4],
            pts,
        }
    }

    pub unsafe fn from_raw(frame: &aurora_media_sys::AuroraFrame) -> Self {
        let fmt = match frame.pixel_format {
            0 => PixelFormat::Yuv420p,
            1 => PixelFormat::Nv12,
            2 => PixelFormat::Rgb24,
            3 => PixelFormat::Rgba,
            _ => PixelFormat::Yuv420p,
        };

        let mut data = Vec::new();
        for i in 0..4 {
            if !frame.data[i].is_null() && frame.linesize[i] > 0 {
                let size = (frame.linesize[i] * frame.height) as usize;
                let slice = std::slice::from_raw_parts(frame.data[i], size);
                data.push(slice.to_vec());
            } else {
                data.push(Vec::new());
            }
        }

        Self {
            pixel_format: fmt,
            width: frame.width as u32,
            height: frame.height as u32,
            data,
            linesize: frame.linesize,
            pts: frame.pts,
        }
    }

    pub(crate) fn to_raw(&self) -> aurora_media_sys::AuroraFrame {
        let mut frame = aurora_media_sys::AuroraFrame::default();
        frame.pixel_format = self.pixel_format.as_c_int();
        frame.width = self.width as c_int;
        frame.height = self.height as c_int;
        frame.linesize = self.linesize;
        frame.pts = self.pts;

        for (i, d) in self.data.iter().enumerate() {
            if i < 4 && !d.is_empty() {
                frame.data[i] = d.as_ptr() as *mut u8;
            }
        }

        frame
    }
}
