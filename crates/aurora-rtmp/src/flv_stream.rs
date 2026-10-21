//! The streaming body of an HTTP-FLV response.

use std::pin::Pin;
use std::task::{Context, Poll};

use bytes::Bytes;
use futures_core::Stream;

use crate::flv_muxer::{flv_tag, TAG_AUDIO, TAG_VIDEO};
use crate::stream_registry::StreamEvent;

/// The body of an HTTP-FLV response.
///
/// It starts with the FLV header, the metadata and the cached sequence headers, and
/// continues with the live frames of the stream.
pub struct FlvStream {
    receiver: tokio::sync::mpsc::UnboundedReceiver<StreamEvent>,
    prefix: Option<Bytes>,
}

impl FlvStream {
    pub fn new(
        receiver: tokio::sync::mpsc::UnboundedReceiver<StreamEvent>,
        prefix: Bytes,
    ) -> Self {
        Self {
            receiver,
            prefix: Some(prefix),
        }
    }
}

impl Stream for FlvStream {
    type Item = Result<Bytes, std::io::Error>;

    fn poll_next(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Option<Self::Item>> {
        let this = self.get_mut();

        if let Some(prefix) = this.prefix.take() {
            return Poll::Ready(Some(Ok(prefix)));
        }

        match this.receiver.poll_recv(cx) {
            Poll::Ready(Some(StreamEvent::Packet(packet))) => {
                let tag_type = if packet.msg_type_id == TAG_AUDIO {
                    TAG_AUDIO
                } else {
                    TAG_VIDEO
                };
                Poll::Ready(Some(Ok(flv_tag(
                    tag_type,
                    packet.timestamp,
                    &packet.payload,
                ))))
            }
            // The publisher went away or the client disconnected.
            Poll::Ready(Some(StreamEvent::Ended)) | Poll::Ready(None) => Poll::Ready(None),
            Poll::Pending => Poll::Pending,
        }
    }
}
