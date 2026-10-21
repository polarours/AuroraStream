#include "aurora_media.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

struct DecoderContext {
    AVCodecContext *codec_ctx;
    AVFrame *frame;
    AVPacket *packet;
};

AuroraDecoderHandle aurora_decoder_create(const AuroraCodecParams *params) {
    auto ctx = new DecoderContext();
    ctx->codec_ctx = nullptr;
    ctx->frame = av_frame_alloc();
    ctx->packet = av_packet_alloc();

    const AVCodec *codec = nullptr;
    switch (params->codec_id) {
        case AURORA_CODEC_ID_H264: codec = avcodec_find_decoder(AV_CODEC_ID_H264); break;
        case AURORA_CODEC_ID_H265: codec = avcodec_find_decoder(AV_CODEC_ID_H265); break;
        case AURORA_CODEC_ID_VP8:  codec = avcodec_find_decoder(AV_CODEC_ID_VP8);  break;
        case AURORA_CODEC_ID_VP9:  codec = avcodec_find_decoder(AV_CODEC_ID_VP9);  break;
        case AURORA_CODEC_ID_AAC:  codec = avcodec_find_decoder(AV_CODEC_ID_AAC);  break;
        case AURORA_CODEC_ID_OPUS: codec = avcodec_find_decoder(AV_CODEC_ID_OPUS); break;
        case AURORA_CODEC_ID_MP3:  codec = avcodec_find_decoder(AV_CODEC_ID_MP3);  break;
        default: return nullptr;
    }
    if (!codec) { delete ctx; return nullptr; }

    ctx->codec_ctx = avcodec_alloc_context3(codec);
    if (params->media_type == AURORA_MEDIA_TYPE_VIDEO) {
        ctx->codec_ctx->width = params->width;
        ctx->codec_ctx->height = params->height;
    } else {
        ctx->codec_ctx->sample_rate = params->sample_rate;
        ctx->codec_ctx->ch_layout.nb_channels = params->channels;
    }

    if (avcodec_open2(ctx->codec_ctx, codec, nullptr) < 0) {
        avcodec_free_context(&ctx->codec_ctx);
        av_frame_free(&ctx->frame);
        av_packet_free(&ctx->packet);
        delete ctx;
        return nullptr;
    }

    return static_cast<AuroraDecoderHandle>(ctx);
}

int aurora_decoder_decode(AuroraDecoderHandle handle,
                          const uint8_t *data, int size,
                          AuroraFrame *frame) {
    auto ctx = static_cast<DecoderContext*>(handle);
    if (!ctx || !ctx->codec_ctx) return -1;

    ctx->packet->data = const_cast<uint8_t*>(data);
    ctx->packet->size = size;

    int ret = avcodec_send_packet(ctx->codec_ctx, ctx->packet);
    if (ret < 0) return -1;

    ret = avcodec_receive_frame(ctx->codec_ctx, ctx->frame);
    if (ret < 0) return -1;

    if (ctx->frame->format == AV_PIX_FMT_YUV420P ||
        ctx->frame->format == AV_PIX_FMT_NV12) {
        frame->pixel_format = (ctx->frame->format == AV_PIX_FMT_YUV420P)
            ? AURORA_PIX_FMT_YUV420P : AURORA_PIX_FMT_NV12;
    } else {
        frame->pixel_format = AURORA_PIX_FMT_YUV420P;
    }
    frame->width = ctx->frame->width;
    frame->height = ctx->frame->height;
    frame->pts = ctx->frame->pts;

    for (int i = 0; i < 4; i++) {
        frame->linesize[i] = ctx->frame->linesize[i];
        frame->data[i] = ctx->frame->data[i];
    }

    return 0;
}

void aurora_decoder_destroy(AuroraDecoderHandle handle) {
    auto ctx = static_cast<DecoderContext*>(handle);
    if (!ctx) return;
    if (ctx->codec_ctx) avcodec_free_context(&ctx->codec_ctx);
    if (ctx->frame) av_frame_free(&ctx->frame);
    if (ctx->packet) av_packet_free(&ctx->packet);
    delete ctx;
}

struct EncoderContext {
    AVCodecContext *codec_ctx;
    AVFrame *frame;
    AVPacket *packet;
};

AuroraEncoderHandle aurora_encoder_create(const AuroraCodecParams *params) {
    auto ctx = new EncoderContext();
    ctx->codec_ctx = nullptr;
    ctx->frame = av_frame_alloc();
    ctx->packet = av_packet_alloc();

    const AVCodec *codec = nullptr;
    switch (params->codec_id) {
        case AURORA_CODEC_ID_H264: codec = avcodec_find_encoder(AV_CODEC_ID_H264); break;
        case AURORA_CODEC_ID_H265: codec = avcodec_find_encoder(AV_CODEC_ID_H265); break;
        case AURORA_CODEC_ID_AAC:  codec = avcodec_find_encoder(AV_CODEC_ID_AAC);  break;
        case AURORA_CODEC_ID_OPUS: codec = avcodec_find_encoder(AV_CODEC_ID_OPUS); break;
        default: return nullptr;
    }
    if (!codec) { delete ctx; return nullptr; }

    ctx->codec_ctx = avcodec_alloc_context3(codec);
    if (params->media_type == AURORA_MEDIA_TYPE_VIDEO) {
        ctx->codec_ctx->width = params->width;
        ctx->codec_ctx->height = params->height;
        ctx->codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
        ctx->codec_ctx->bit_rate = params->bit_rate > 0 ? params->bit_rate : 2000000;
        ctx->codec_ctx->time_base = {1, 30};
        ctx->codec_ctx->framerate = {30, 1};
        ctx->codec_ctx->gop_size = 30;
        ctx->codec_ctx->max_b_frames = 0;
        if (codec->id == AV_CODEC_ID_H264) {
            av_opt_set(ctx->codec_ctx->priv_data, "preset", "ultrafast", 0);
            av_opt_set(ctx->codec_ctx->priv_data, "tune", "zerolatency", 0);
        }
    } else {
        ctx->codec_ctx->sample_rate = params->sample_rate > 0 ? params->sample_rate : 44100;
        av_channel_layout_default(&ctx->codec_ctx->ch_layout,
                                  params->channels > 0 ? params->channels : 2);
        ctx->codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
        ctx->codec_ctx->bit_rate = params->bit_rate > 0 ? params->bit_rate : 128000;
    }

    if (avcodec_open2(ctx->codec_ctx, codec, nullptr) < 0) {
        avcodec_free_context(&ctx->codec_ctx);
        av_frame_free(&ctx->frame);
        av_packet_free(&ctx->packet);
        delete ctx;
        return nullptr;
    }

    return static_cast<AuroraEncoderHandle>(ctx);
}

int aurora_encoder_encode(AuroraEncoderHandle handle,
                          const AuroraFrame *frame,
                          uint8_t **out_data, int *out_size,
                          int *keyframe) {
    auto ctx = static_cast<EncoderContext*>(handle);
    if (!ctx || !ctx->codec_ctx) return -1;

    ctx->frame->format = ctx->codec_ctx->pix_fmt;
    ctx->frame->width = frame->width;
    ctx->frame->height = frame->height;
    ctx->frame->pts = frame->pts;

    if (ctx->codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        av_frame_get_buffer(ctx->frame, 0);
        for (int i = 0; i < 4 && frame->data[i]; i++) {
            int h = (i < 3) ? frame->height : 1;
            for (int y = 0; y < h; y++) {
                memcpy(ctx->frame->data[i] + y * ctx->frame->linesize[i],
                       frame->data[i] + y * frame->linesize[i],
                       frame->linesize[i]);
            }
        }
    }

    int ret = avcodec_send_frame(ctx->codec_ctx, ctx->frame);
    av_frame_unref(ctx->frame);
    if (ret < 0) return -1;

    ret = avcodec_receive_packet(ctx->codec_ctx, ctx->packet);
    if (ret < 0) return -1;

    *out_data = ctx->packet->data;
    *out_size = ctx->packet->size;
    *keyframe = (ctx->packet->flags & AV_PKT_FLAG_KEY) ? 1 : 0;

    return 0;
}

void aurora_encoder_destroy(AuroraEncoderHandle handle) {
    auto ctx = static_cast<EncoderContext*>(handle);
    if (!ctx) return;
    if (ctx->codec_ctx) avcodec_free_context(&ctx->codec_ctx);
    if (ctx->frame) av_frame_free(&ctx->frame);
    if (ctx->packet) av_packet_free(&ctx->packet);
    delete ctx;
}
