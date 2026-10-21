use std::ffi::CString;

use crate::aurora_media_sys;
use crate::codec::Frame;

pub struct Filter {
    handle: *mut std::ffi::c_void,
}

unsafe impl Send for Filter {}

impl Filter {
    pub fn new(filter_spec: &str) -> Option<Self> {
        let spec = CString::new(filter_spec).ok()?;

        unsafe {
            let handle = aurora_media_sys::aurora_filter_create(spec.as_ptr());
            if handle.is_null() {
                None
            } else {
                Some(Self { handle })
            }
        }
    }

    pub fn apply(&self, input: &Frame) -> Option<Frame> {
        let c_in = input.to_raw();
        let mut c_out = aurora_media_sys::AuroraFrame::default();

        unsafe {
            let ret = aurora_media_sys::aurora_filter_apply(self.handle, &c_in, &mut c_out);

            if ret < 0 {
                return None;
            }

            Some(Frame::from_raw(&c_out))
        }
    }
}

impl Drop for Filter {
    fn drop(&mut self) {
        unsafe {
            aurora_media_sys::aurora_filter_destroy(self.handle);
        }
    }
}
