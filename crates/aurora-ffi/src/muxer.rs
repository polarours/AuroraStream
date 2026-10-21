use std::ffi::CString;
use std::os::raw::c_int;

use crate::aurora_media_sys;
use crate::codec::CodecParams;

pub struct Muxer {
    handle: *mut std::ffi::c_void,
}

unsafe impl Send for Muxer {}

pub struct MuxerParams {
    pub format: String,
    pub url: String,
}

impl Muxer {
    pub fn new(params: &MuxerParams) -> Option<Self> {
        let format = CString::new(params.format.clone()).ok()?;
        let url = CString::new(params.url.clone()).ok()?;

        unsafe {
            let handle = aurora_media_sys::aurora_muxer_create(format.as_ptr(), url.as_ptr());
            if handle.is_null() {
                None
            } else {
                Some(Self { handle })
            }
        }
    }

    pub fn write_header(&self, codec_params: &CodecParams, stream_index: u32) -> bool {
        let c_params = aurora_media_sys::AuroraCodecParams {
            codec_id: codec_params.codec_id.as_c_int(),
            media_type: codec_params.media_type.as_c_int(),
            width: codec_params.width.unwrap_or(0) as c_int,
            height: codec_params.height.unwrap_or(0) as c_int,
            sample_rate: codec_params.sample_rate.unwrap_or(0) as c_int,
            channels: codec_params.channels.unwrap_or(0) as c_int,
            bit_rate: codec_params.bit_rate.unwrap_or(0) as c_int,
        };

        unsafe {
            let ret = aurora_media_sys::aurora_muxer_write_header(
                self.handle,
                &c_params,
                stream_index as c_int,
            );
            ret >= 0
        }
    }

    pub fn write_packet(
        &self,
        data: &[u8],
        pts: i64,
        dts: i64,
        keyframe: bool,
        stream_index: u32,
    ) -> bool {
        unsafe {
            let ret = aurora_media_sys::aurora_muxer_write_packet(
                self.handle,
                data.as_ptr(),
                data.len() as c_int,
                pts,
                dts,
                if keyframe { 1 } else { 0 },
                stream_index as c_int,
            );
            ret >= 0
        }
    }

    pub fn write_trailer(&self) {
        unsafe {
            aurora_media_sys::aurora_muxer_write_trailer(self.handle);
        }
    }
}

impl Drop for Muxer {
    fn drop(&mut self) {
        unsafe {
            aurora_media_sys::aurora_muxer_destroy(self.handle);
        }
    }
}
