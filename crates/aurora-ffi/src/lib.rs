pub mod codec;
pub mod filter;
pub mod muxer;

pub use codec::{CodecParams, Decoder, Encoder, Frame, PixelFormat};
pub use filter::Filter;
pub use muxer::{Muxer, MuxerParams};

pub fn version() -> String {
    unsafe {
        let ptr = aurora_media_sys::aurora_version();
        if ptr.is_null() {
            return "unknown".to_string();
        }
        std::ffi::CStr::from_ptr(ptr).to_string_lossy().into_owned()
    }
}

#[allow(non_upper_case_globals)]
mod aurora_media_sys {
    use std::os::raw::{c_char, c_int};

    #[repr(C)]
    #[derive(Debug, Default, Clone, Copy)]
    pub struct AuroraCodecParams {
        pub codec_id: c_int,
        pub media_type: c_int,
        pub width: c_int,
        pub height: c_int,
        pub sample_rate: c_int,
        pub channels: c_int,
        pub bit_rate: c_int,
    }

    #[repr(C)]
    #[derive(Debug, Default)]
    pub struct AuroraFrame {
        pub pixel_format: c_int,
        pub width: c_int,
        pub height: c_int,
        pub linesize: [c_int; 4],
        pub data: [*mut u8; 4],
        pub pts: i64,
    }

    extern "C" {
        pub fn aurora_decoder_create(params: *const AuroraCodecParams) -> *mut std::ffi::c_void;
        pub fn aurora_decoder_decode(
            handle: *mut std::ffi::c_void,
            data: *const u8,
            size: c_int,
            frame: *mut AuroraFrame,
        ) -> c_int;
        pub fn aurora_decoder_destroy(handle: *mut std::ffi::c_void);

        pub fn aurora_encoder_create(params: *const AuroraCodecParams) -> *mut std::ffi::c_void;
        pub fn aurora_encoder_encode(
            handle: *mut std::ffi::c_void,
            frame: *const AuroraFrame,
            out_data: *mut *mut u8,
            out_size: *mut c_int,
            keyframe: *mut c_int,
        ) -> c_int;
        pub fn aurora_encoder_destroy(handle: *mut std::ffi::c_void);

        pub fn aurora_muxer_create(
            format_name: *const c_char,
            url: *const c_char,
        ) -> *mut std::ffi::c_void;
        pub fn aurora_muxer_write_header(
            handle: *mut std::ffi::c_void,
            params: *const AuroraCodecParams,
            stream_index: c_int,
        ) -> c_int;
        pub fn aurora_muxer_write_packet(
            handle: *mut std::ffi::c_void,
            data: *const u8,
            size: c_int,
            pts: i64,
            dts: i64,
            keyframe: c_int,
            stream_index: c_int,
        ) -> c_int;
        pub fn aurora_muxer_write_trailer(handle: *mut std::ffi::c_void);
        pub fn aurora_muxer_destroy(handle: *mut std::ffi::c_void);

        pub fn aurora_filter_create(filter_spec: *const c_char) -> *mut std::ffi::c_void;
        pub fn aurora_filter_apply(
            handle: *mut std::ffi::c_void,
            in_frame: *const AuroraFrame,
            out_frame: *mut AuroraFrame,
        ) -> c_int;
        pub fn aurora_filter_destroy(handle: *mut std::ffi::c_void);

        pub fn aurora_version() -> *const c_char;
    }
}
