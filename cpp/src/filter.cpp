#include "aurora_media.h"

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

int aurora_frame_alloc(AuroraFrame *frame, AuroraPixelFormat fmt,
                       int width, int height) {
    if (!frame) return -1;

    enum AVPixelFormat av_fmt;
    switch (fmt) {
        case AURORA_PIX_FMT_YUV420P: av_fmt = AV_PIX_FMT_YUV420P; break;
        case AURORA_PIX_FMT_NV12:    av_fmt = AV_PIX_FMT_NV12;    break;
        case AURORA_PIX_FMT_RGB24:   av_fmt = AV_PIX_FMT_RGB24;   break;
        case AURORA_PIX_FMT_RGBA:    av_fmt = AV_PIX_FMT_RGBA;    break;
        default: return -1;
    }

    frame->pixel_format = fmt;
    frame->width = width;
    frame->height = height;

    int ret = av_image_alloc(frame->data, frame->linesize,
                             width, height, av_fmt, 32);
    if (ret < 0) return -1;

    return 0;
}

void aurora_frame_free(AuroraFrame *frame) {
    if (!frame) return;
    for (int i = 0; i < 4; i++) {
        if (frame->data[i]) {
            av_free(frame->data[i]);
            frame->data[i] = nullptr;
        }
    }
}

int aurora_frame_get_buffer_size(AuroraPixelFormat fmt, int width, int height) {
    enum AVPixelFormat av_fmt;
    switch (fmt) {
        case AURORA_PIX_FMT_YUV420P: av_fmt = AV_PIX_FMT_YUV420P; break;
        case AURORA_PIX_FMT_NV12:    av_fmt = AV_PIX_FMT_NV12;    break;
        case AURORA_PIX_FMT_RGB24:   av_fmt = AV_PIX_FMT_RGB24;   break;
        case AURORA_PIX_FMT_RGBA:    av_fmt = AV_PIX_FMT_RGBA;    break;
        default: return -1;
    }
    return av_image_get_buffer_size(av_fmt, width, height, 32);
}

struct FilterContext {
    AVFilterGraph *graph;
    AVFilterContext *buffersrc_ctx;
    AVFilterContext *buffersink_ctx;
    char spec[256];
};

AuroraFilterHandle aurora_filter_create(const char *filter_spec) {
    auto ctx = new FilterContext();
    strncpy(ctx->spec, filter_spec, sizeof(ctx->spec) - 1);
    ctx->spec[sizeof(ctx->spec) - 1] = '\0';
    ctx->graph = nullptr;
    ctx->buffersrc_ctx = nullptr;
    ctx->buffersink_ctx = nullptr;
    return static_cast<AuroraFilterHandle>(ctx);
}

int aurora_filter_apply(AuroraFilterHandle handle,
                        const AuroraFrame *in_frame,
                        AuroraFrame *out_frame) {
    if (!handle || !in_frame || !out_frame) return -1;

    auto ctx = static_cast<FilterContext*>(handle);

    if (!ctx->graph) {
        ctx->graph = avfilter_graph_alloc();
        if (!ctx->graph) return -1;

        const AVFilter *buffersrc = avfilter_get_by_name("buffer");
        const AVFilter *buffersink = avfilter_get_by_name("buffersink");
        if (!buffersrc || !buffersink) {
            avfilter_graph_free(&ctx->graph);
            return -1;
        }

        char args[512];
        snprintf(args, sizeof(args),
            "video_size=%dx%d:pix_fmt=%d:time_base=1/30:pixel_aspect=1/1",
            in_frame->width, in_frame->height, in_frame->pixel_format);

        if (avfilter_graph_create_filter(&ctx->buffersrc_ctx, buffersrc, "in", args, nullptr, ctx->graph) < 0) {
            avfilter_graph_free(&ctx->graph);
            return -1;
        }

        if (avfilter_graph_create_filter(&ctx->buffersink_ctx, buffersink, "out", nullptr, nullptr, ctx->graph) < 0) {
            avfilter_graph_free(&ctx->graph);
            return -1;
        }

        AVFilterInOut *outputs = avfilter_inout_alloc();
        AVFilterInOut *inputs = avfilter_inout_alloc();
        outputs->name = av_strdup("in");
        outputs->filter_ctx = ctx->buffersrc_ctx;
        outputs->pad_idx = 0;
        outputs->next = nullptr;
        inputs->name = av_strdup("out");
        inputs->filter_ctx = ctx->buffersink_ctx;
        inputs->pad_idx = 0;
        inputs->next = nullptr;

        if (avfilter_graph_parse_ptr(ctx->graph, ctx->spec, &inputs, &outputs, nullptr) < 0) {
            avfilter_inout_free(&inputs);
            avfilter_inout_free(&outputs);
            avfilter_graph_free(&ctx->graph);
            return -1;
        }

        if (avfilter_graph_config(ctx->graph, nullptr) < 0) {
            avfilter_inout_free(&inputs);
            avfilter_inout_free(&outputs);
            avfilter_graph_free(&ctx->graph);
            return -1;
        }

        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
    }

    AVFrame *frame = av_frame_alloc();
    frame->format = in_frame->pixel_format;
    frame->width = in_frame->width;
    frame->height = in_frame->height;
    frame->pts = in_frame->pts;
    av_frame_get_buffer(frame, 0);

    for (int i = 0; i < 4 && in_frame->data[i]; i++) {
        int h = (i < 3) ? in_frame->height : 1;
        for (int y = 0; y < h; y++) {
            memcpy(frame->data[i] + y * frame->linesize[i],
                   in_frame->data[i] + y * in_frame->linesize[i],
                   in_frame->linesize[i]);
        }
    }

    if (av_buffersrc_add_frame_flags(ctx->buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        av_frame_free(&frame);
        return -1;
    }
    av_frame_free(&frame);

    AVFrame *out_avframe = av_frame_alloc();
    int ret = av_buffersink_get_frame(ctx->buffersink_ctx, out_avframe);
    if (ret < 0) {
        av_frame_free(&out_avframe);
        return -1;
    }

    out_frame->pixel_format = static_cast<AuroraPixelFormat>(out_avframe->format);
    out_frame->width = out_avframe->width;
    out_frame->height = out_avframe->height;
    out_frame->pts = out_avframe->pts;
    for (int i = 0; i < 4; i++) {
        out_frame->linesize[i] = out_avframe->linesize[i];
        out_frame->data[i] = out_avframe->data[i];
    }

    av_frame_unref(out_avframe);
    av_frame_free(&out_avframe);

    return 0;
}

void aurora_filter_destroy(AuroraFilterHandle handle) {
    auto ctx = static_cast<FilterContext*>(handle);
    if (!ctx) return;
    if (ctx->graph) avfilter_graph_free(&ctx->graph);
    delete ctx;
}

const char* aurora_version(void) {
    return "AuroraStream Media Core 0.1.0";
}
