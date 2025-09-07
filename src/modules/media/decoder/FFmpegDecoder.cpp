/********************************************************************************
 * @file   : FFmpegDecoder.cpp
 * @brief  :
 *
 *
 *
 * @Author : polarours
 * @Date   : 2025/08/25
 ********************************************************************************/

#include "aurorastream/modules/media/decoder/FFmpegDecoder.h"

#include <QDebug>
#include <QThread>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

namespace aurorastream {
namespace modules {
namespace media {
namespace decoder {

FFmpegDecoder::FFmpegDecoder(QObject *parent) : Decoder(parent),
    m_formatContext(nullptr),
    m_videoCodecContext(nullptr),
    m_audioCodecContext(nullptr),
    m_videoFrame(nullptr),
    m_audioFrame(nullptr),
    m_swsContext(nullptr),
    m_swrContext(nullptr),
    m_videoStreamIndex(-1),
    m_audioStreamIndex(-1),
    m_internalState(InternalState::Stopped) {}

FFmpegDecoder::~FFmpegDecoder() {
    deinitFFmpeg();
}

bool FFmpegDecoder::open(const QString &url) {
    if (m_internalState != InternalState::Stopped) {
        qWarning() << "Decoder is not in stopped state";
        return false;
    }

    setInternalState(InternalState::Opening);
    if (!initFFmpeg(url)) {
        setInternalState(InternalState::Error);
        emit error("Failed to initialize FFmpeg");
        return false;
    }

    setInternalState(InternalState::Opened);
    emit durationChanged(m_duration);
    return true;
}

void FFmpegDecoder::close() {
    if (m_internalState == InternalState::Stopped) {
        return;
    }

    setInternalState(InternalState::Closing);
    deinitFFmpeg();
    setInternalState(InternalState::Stopped);
    emit finished();
}

void FFmpegDecoder::start() {
    if (m_internalState != InternalState::Opened && m_internalState != InternalState::Paused) {
        qWarning() << "Decoder is not in opened or paused state";
        return;
    }

    setInternalState(InternalState::Starting);
    QThread *thread = new QThread();
    moveToThread(thread);
    connect(thread, &QThread::started, this, &FFmpegDecoder::decodeLoop);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
    setInternalState(InternalState::Playing);
}

bool FFmpegDecoder::pause() {
    if (m_internalState != InternalState::Playing) {
        qWarning() << "Decoder is not in playing state";
        return false;
    }

    setInternalState(InternalState::Pausing);
    setInternalState(InternalState::Paused);
    return true;
}

void FFmpegDecoder::stop() {
    if (m_internalState == InternalState::Stopped) {
        return;
    }

    setInternalState(InternalState::Stopping);
    deinitFFmpeg();
    setInternalState(InternalState::Stopped);
    emit finished();
}

bool FFmpegDecoder::seek(qint64 position) {
    if (m_internalState != InternalState::Playing && m_internalState != InternalState::Paused) {
        qWarning() << "Decoder is not in playing or paused state";
        return false;
    }

    setInternalState(InternalState::Seeking);
    if (av_seek_frame(m_formatContext, -1, position, AVSEEK_FLAG_BACKWARD) < 0) {
        setInternalState(InternalState::Error);
        emit error("Failed to seek");
        return false;
    }

    updatePosition(position);
    setInternalState(m_internalState == InternalState::Playing ? InternalState::Playing : InternalState::Paused);
    return true;
}

bool FFmpegDecoder::isOpen() const {
    return m_internalState != InternalState::Stopped && m_internalState != InternalState::Error;
}

bool FFmpegDecoder::isPlaying() const {
    return m_internalState == InternalState::Playing;
}

bool FFmpegDecoder::isPaused() const {
    return m_internalState == InternalState::Paused;
}

bool FFmpegDecoder::isStopped() const {
    return m_internalState == InternalState::Stopped;
}

qint64 FFmpegDecoder::getDuration() const {
    return m_duration;
}

qint64 FFmpegDecoder::getPosition() const {
    return m_position;
}

int FFmpegDecoder::getVideoWidth() const {
    return m_videoCodecContext ? m_videoCodecContext->width : 0;
}

int FFmpegDecoder::getVideoHeight() const {
    return m_videoCodecContext ? m_videoCodecContext->height : 0;
}

double FFmpegDecoder::getVideoFrameRate() const {
    return m_videoCodecContext ? av_q2d(m_videoCodecContext->framerate) : 0.0;
}

int FFmpegDecoder::getAudioSampleRate() const {
    return m_audioCodecContext ? m_audioCodecContext->sample_rate : 0;
}

int FFmpegDecoder::getAudioChannels() const {
    return m_audioCodecContext ? m_audioCodecContext->channels : 0;
}

bool FFmpegDecoder::initFFmpeg(const QString &uri) {
    // Initialize FFmpeg resources
    if (avformat_open_input(&m_formatContext, uri.toStdString().c_str(), nullptr, nullptr) < 0) {
        qWarning() << "Failed to open input file";
        return false;
    }

    if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
        qWarning() << "Failed to find stream info";
        return false;
    }

    // Find video and audio streams
    for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
        AVStream *stream = m_formatContext->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && m_videoStreamIndex == -1) {
            m_videoStreamIndex = i;
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && m_audioStreamIndex == -1) {
            m_audioStreamIndex = i;
        }
    }

    // Initialize codec contexts
    if (m_videoStreamIndex != -1) {
        AVStream *videoStream = m_formatContext->streams[m_videoStreamIndex];
        const AVCodec *videoCodec = avcodec_find_decoder(videoStream->codecpar->codec_id);
        if (!videoCodec) {
            qWarning() << "Unsupported video codec";
            return false;
        }

        m_videoCodecContext = avcodec_alloc_context3(videoCodec);
        if (avcodec_parameters_to_context(m_videoCodecContext, videoStream->codecpar) < 0) {
            qWarning() << "Failed to initialize video codec context";
            return false;
        }

        if (avcodec_open2(m_videoCodecContext, videoCodec, nullptr) < 0) {
            qWarning() << "Failed to open video codec";
            return false;
        }

        m_videoFrame = av_frame_alloc();
    }

    if (m_audioStreamIndex != -1) {
        AVStream *audioStream = m_formatContext->streams[m_audioStreamIndex];
        const AVCodec *audioCodec = avcodec_find_decoder(audioStream->codecpar->codec_id);
        if (!audioCodec) {
            qWarning() << "Unsupported audio codec";
            return false;
        }

        m_audioCodecContext = avcodec_alloc_context3(audioCodec);
        if (avcodec_parameters_to_context(m_audioCodecContext, audioStream->codecpar) < 0) {
            qWarning() << "Failed to initialize audio codec context";
            return false;
        }

        if (avcodec_open2(m_audioCodecContext, audioCodec, nullptr) < 0) {
            qWarning() << "Failed to open audio codec";
            return false;
        }

        m_audioFrame = av_frame_alloc();
    }

    m_duration = m_formatContext->duration / AV_TIME_BASE;
    return true;
}

void FFmpegDecoder::deinitFFmpeg() {
    if (m_videoFrame) {
        av_frame_free(&m_videoFrame);
        m_videoFrame = nullptr;
    }

    if (m_audioFrame) {
        av_frame_free(&m_audioFrame);
        m_audioFrame = nullptr;
    }

    if (m_videoCodecContext) {
        avcodec_free_context(&m_videoCodecContext);
        m_videoCodecContext = nullptr;
    }

    if (m_audioCodecContext) {
        avcodec_free_context(&m_audioCodecContext);
        m_audioCodecContext = nullptr;
    }

    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;
    }

    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }

    if (m_swrContext) {
        swr_free(&m_swrContext);
        m_swrContext = nullptr;
    }

    m_videoStreamIndex = -1;
    m_audioStreamIndex = -1;
    m_duration = 0;
    m_position = 0;
}

void FFmpegDecoder::decodeLoop() {
    AVPacket packet;
    while (m_internalState == InternalState::Playing) {
        if (av_read_frame(m_formatContext, &packet) < 0) {
            break;
        }

        if (packet.stream_index == m_videoStreamIndex) {
            processVideoFrame(&packet);
        } else if (packet.stream_index == m_audioStreamIndex) {
            processAudioFrame(&packet);
        }

        av_packet_unref(&packet);
    }

    if (m_internalState == InternalState::Playing) {
        setInternalState(InternalState::Stopped);
        emit finished();
    }
}

void FFmpegDecoder::processVideoFrame(AVPacket *packet) {
    if (avcodec_send_packet(m_videoCodecContext, packet) < 0) {
        qWarning() << "Failed to send video packet";
        return;
    }

    while (avcodec_receive_frame(m_videoCodecContext, m_videoFrame) >= 0) {
        // Process video frame (e.g., convert to RGB, emit signal)
        emit videoFrameReady(*m_videoFrame);
    }
}

void FFmpegDecoder::processAudioFrame(AVPacket *packet) {
    if (avcodec_send_packet(m_audioCodecContext, packet) < 0) {
        qWarning() << "Failed to send audio packet";
        return;
    }

    while (avcodec_receive_frame(m_audioCodecContext, m_audioFrame) >= 0) {
        // Process audio frame (e.g., resample, emit signal)
        emit audioFrameReady(*m_audioFrame);
    }
}

void FFmpegDecoder::updatePosition() {
    if (m_formatContext) {
        m_position = av_rescale_q(m_formatContext->pb->pos, AV_TIME_BASE_Q, m_formatContext->streams[0]->time_base);
        emit positionChanged(m_position);
    }
}

void FFmpegDecoder::setInternalState(InternalState newState) {
    if (m_internalState != newState) {
        m_internalState = newState;
        emit stateChanged(static_cast<int>(newState));
    }
}

} // namespace decoder
} // namespace media
} // namespace modules
} // namespace aurorastream