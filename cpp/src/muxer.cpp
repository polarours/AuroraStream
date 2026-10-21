#include "aurora_media.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

struct MuxerContext {
    AVFormatContext *fmt_ctx;
    AVStream *stream;
    int stream_index;
    char url[1024];
};

AuroraMuxerHandle aurora_muxer_create(const char *format_name, const char *url) {
    auto ctx = new MuxerContext();
    ctx->fmt_ctx = nullptr;
    ctx->stream = nullptr;
    ctx->stream_index = 0;

    const AVOutputFormat *oformat = nullptr;
    if (format_name) {
        oformat = av_guess_format(format_name, url, nullptr);
    }
    if (!oformat) {
        oformat = av_guess_format(nullptr, url, nullptr);
    }
    if (!oformat) {
        delete ctx;
        return nullptr;
    }

    int ret = avformat_alloc_output_context2(&ctx->fmt_ctx, oformat, nullptr, url);
    if (ret < 0 || !ctx->fmt_ctx) {
        delete ctx;
        return nullptr;
    }

    strncpy(ctx->url, url, sizeof(ctx->url) - 1);
    ctx->url[sizeof(ctx->url) - 1] = '\0';

    return static_cast<AuroraMuxerHandle>(ctx);
}

int aurora_muxer_write_header(AuroraMuxerHandle handle,
                              const AuroraCodecParams *params, int stream_index) {
    auto ctx = static_cast<MuxerContext*>(handle);
    if (!ctx || !ctx->fmt_ctx) return -1;

    AVStream *st = avformat_new_stream(ctx->fmt_ctx, nullptr);
    if (!st) return -1;

    AVCodecParameters *par = st->codecpar;
    if (params->media_type == AURORA_MEDIA_TYPE_VIDEO) {
        par->codec_type = AVMEDIA_TYPE_VIDEO;
        switch (params->codec_id) {
            case AURORA_CODEC_ID_H264: par->codec_id = AV_CODEC_ID_H264; break;
            case AURORA_CODEC_ID_H265: par->codec_id = AV_CODEC_ID_H265; break;
            case AURORA_CODEC_ID_VP8:  par->codec_id = AV_CODEC_ID_VP8;  break;
            case AURORA_CODEC_ID_VP9:  par->codec_id = AV_CODEC_ID_VP9;  break;
            default: par->codec_id = AV_CODEC_ID_H264; break;
        }
        par->width = params->width;
        par->height = params->height;
    } else {
        par->codec_type = AVMEDIA_TYPE_AUDIO;
        switch (params->codec_id) {
            case AURORA_CODEC_ID_AAC:  par->codec_id = AV_CODEC_ID_AAC;  break;
            case AURORA_CODEC_ID_OPUS: par->codec_id = AV_CODEC_ID_OPUS; break;
            case AURORA_CODEC_ID_MP3:  par->codec_id = AV_CODEC_ID_MP3;  break;
            default: par->codec_id = AV_CODEC_ID_AAC; break;
        }
        par->sample_rate = params->sample_rate;
        av_channel_layout_default(&par->ch_layout, params->channels);
    }

    ctx->stream = st;
    ctx->stream_index = st->index;

    if (!(ctx->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        int ret = avio_open(&ctx->fmt_ctx->pb, ctx->url, AVIO_FLAG_WRITE);
        if (ret < 0) return -1;
    }

    int ret = avformat_write_header(ctx->fmt_ctx, nullptr);
    if (ret < 0) return -1;

    return 0;
}

int aurora_muxer_write_packet(AuroraMuxerHandle handle,
                              const uint8_t *data, int size,
                              int64_t pts, int64_t dts,
                              int keyframe, int stream_index) {
    auto ctx = static_cast<MuxerContext*>(handle);
    if (!ctx || !ctx->fmt_ctx || !ctx->stream) return -1;

    AVPacket *pkt = av_packet_alloc();
    pkt->data = const_cast<uint8_t*>(data);
    pkt->size = size;
    pkt->pts = pts;
    pkt->dts = dts;
    pkt->stream_index = ctx->stream->index;
    if (keyframe) pkt->flags |= AV_PKT_FLAG_KEY;

    int ret = av_interleaved_write_frame(ctx->fmt_ctx, pkt);
    av_packet_free(&pkt);

    return (ret < 0) ? -1 : 0;
}

void aurora_muxer_write_trailer(AuroraMuxerHandle handle) {
    auto ctx = static_cast<MuxerContext*>(handle);
    if (!ctx || !ctx->fmt_ctx) return;
    av_write_trailer(ctx->fmt_ctx);
}

void aurora_muxer_destroy(AuroraMuxerHandle handle) {
    auto ctx = static_cast<MuxerContext*>(handle);
    if (!ctx) return;
    if (ctx->fmt_ctx) {
        if (!(ctx->fmt_ctx->oformat->flags & AVFMT_NOFILE) && ctx->fmt_ctx->pb) {
            avio_closep(&ctx->fmt_ctx->pb);
        }
        avformat_free_context(ctx->fmt_ctx);
    }
    delete ctx;
}
