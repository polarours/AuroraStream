#ifndef AURORA_MEDIA_H
#define AURORA_MEDIA_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AURORA_PIX_FMT_YUV420P = 0,
    AURORA_PIX_FMT_NV12 = 1,
    AURORA_PIX_FMT_RGB24 = 2,
    AURORA_PIX_FMT_RGBA = 3,
} AuroraPixelFormat;

typedef enum {
    AURORA_CODEC_ID_H264 = 0,
    AURORA_CODEC_ID_H265 = 1,
    AURORA_CODEC_ID_VP8 = 2,
    AURORA_CODEC_ID_VP9 = 3,
    AURORA_CODEC_ID_AV1 = 4,
    AURORA_CODEC_ID_AAC = 100,
    AURORA_CODEC_ID_OPUS = 101,
    AURORA_CODEC_ID_MP3 = 102,
} AuroraCodecID;

typedef enum {
    AURORA_MEDIA_TYPE_VIDEO = 0,
    AURORA_MEDIA_TYPE_AUDIO = 1,
} AuroraMediaType;

typedef struct {
    AuroraCodecID codec_id;
    AuroraMediaType media_type;
    int width;
    int height;
    int sample_rate;
    int channels;
    int bit_rate;
} AuroraCodecParams;

typedef struct {
    AuroraPixelFormat pixel_format;
    int width;
    int height;
    int linesize[4];
    uint8_t *data[4];
    int64_t pts;
} AuroraFrame;

typedef struct {
    AuroraCodecID codec_id;
    AuroraMediaType media_type;
    uint8_t *data;
    int size;
    int64_t pts;
    int64_t dts;
    int keyframe;
} AuroraPacket;

typedef void* AuroraDecoderHandle;
typedef void* AuroraEncoderHandle;
typedef void* AuroraMuxerHandle;
typedef void* AuroraFilterHandle;

AuroraDecoderHandle aurora_decoder_create(const AuroraCodecParams *params);
int aurora_decoder_decode(AuroraDecoderHandle handle,
                          const uint8_t *data, int size,
                          AuroraFrame *frame);
void aurora_decoder_destroy(AuroraDecoderHandle handle);

AuroraEncoderHandle aurora_encoder_create(const AuroraCodecParams *params);
int aurora_encoder_encode(AuroraEncoderHandle handle,
                          const AuroraFrame *frame,
                          uint8_t **out_data, int *out_size,
                          int *keyframe);
void aurora_encoder_destroy(AuroraEncoderHandle handle);

AuroraMuxerHandle aurora_muxer_create(const char *format_name, const char *url);
int aurora_muxer_write_header(AuroraMuxerHandle handle,
                              const AuroraCodecParams *params, int stream_index);
int aurora_muxer_write_packet(AuroraMuxerHandle handle,
                              const uint8_t *data, int size,
                              int64_t pts, int64_t dts,
                              int keyframe, int stream_index);
void aurora_muxer_write_trailer(AuroraMuxerHandle handle);
void aurora_muxer_destroy(AuroraMuxerHandle handle);

AuroraFilterHandle aurora_filter_create(const char *filter_spec);
int aurora_filter_apply(AuroraFilterHandle handle,
                        const AuroraFrame *in_frame,
                        AuroraFrame *out_frame);
void aurora_filter_destroy(AuroraFilterHandle handle);

int aurora_frame_alloc(AuroraFrame *frame, AuroraPixelFormat fmt,
                       int width, int height);
void aurora_frame_free(AuroraFrame *frame);
int aurora_frame_get_buffer_size(AuroraPixelFormat fmt, int width, int height);

const char* aurora_version(void);

#ifdef __cplusplus
}
#endif

#endif // AURORA_MEDIA_H
