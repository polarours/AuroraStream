#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PixelFormat {
    Yuv420p,
    Nv12,
    Rgb24,
    Rgba,
}

#[derive(Debug, Clone)]
pub struct VideoFrame {
    pub pixel_format: PixelFormat,
    pub width: u32,
    pub height: u32,
    pub data: Vec<u8>,
    pub pts: i64,
}

impl VideoFrame {
    pub fn new(pixel_format: PixelFormat, width: u32, height: u32, data: Vec<u8>, pts: i64) -> Self {
        Self {
            pixel_format,
            width,
            height,
            data,
            pts,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SampleFormat {
    F32Planar,
    S16,
    F32,
}

#[derive(Debug, Clone)]
pub struct AudioFrame {
    pub sample_format: SampleFormat,
    pub sample_rate: u32,
    pub channels: u32,
    pub data: Vec<u8>,
    pub pts: i64,
}

impl AudioFrame {
    pub fn new(
        sample_format: SampleFormat,
        sample_rate: u32,
        channels: u32,
        data: Vec<u8>,
        pts: i64,
    ) -> Self {
        Self {
            sample_format,
            sample_rate,
            channels,
            data,
            pts,
        }
    }
}

#[derive(Debug, Clone)]
pub enum MediaFrame {
    Video(VideoFrame),
    Audio(AudioFrame),
}
