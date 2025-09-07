/********************************************************************************
 * @file   : FFmpegDecoder.h
 * @brief  : 声明基于 FFmpeg 的具体媒体解码器类。
 *
 * 此文件定义了 aurorastream::modules::media::decoder::FFmpegDecoder 类，
 * 它是 Decoder 抽象基类的具体实现，使用 FFmpeg 库进行媒体解码。
 *
 * @author : polarours
 * @date   : 2025/08/22
 ********************************************************************************/

#ifdef AURORASTREAM_MEDIA_DECODER_FFMPEGDECODER_H
#define AURORASTREAM_MEDIA_DECODER_FFMPEGDECODER_H

#include <QObject> // FFmpegDecoder 继承 QObject
#include <QString> // 使用 QString 处理文件路径和错误信息
#include <QThread> // 可能需要用于解码线程 (如果直接继承 QThread)
#include <memory>  // 使用智能指针管理资源

#include "AuroraStream/AuroraStream.h" // 获取 AURORASTREAM_API 宏

#include "aurorastream/modules/media/decoder/Decoder.h" // 包含抽象基类 Decoder

extern "C" {
    struct AVFormatContext;
    struct AVCodecContext;
    struct AVFrame;
    struct SwsContext;
    struct SwrContext;
}

namespace aurorastream {
namespace modules {
namespace media {
namespace decoder {

class AURORASTREAM_API FFmpegDecoder : public Decoder
{
    Q_OBJECT

public:
    explicit FFmpegDecoder(QObject *parent = nullptr);

    ~FFmpegDecoder() override;

    FFmpegDecoder(const FFmpegDecoder &) = delete;
    FFmpegDecoder &operator=(const FFmpegDecoder &) = delete;
    FFmpegDecoder(FFmpegDecoder &&) = delete;
    FFmpegDecoder &operator=(FFmpegDecoder &&) = delete;

    bool open(const QString &url) override;
    void close() override;
    void start() override;
    bool pause() override;
    void stop() override;
    bool seek(qint64 position) override;
    bool isOpen() const override;
    bool isPlaying() const override;
    bool isPaused() const override;
    bool isStopped() const override;
    qint64 getDuration() const override;
    qint64 getPosition() const override;
    void getVideoWidth() const override;
    void getVideoHeight() const override;
    void getVideoFrameRate() const override;
    void getAudioSampleRate() const override;
    void getAudioChannels() const override;

private slots:

protected:
    AVFormatContext*  m_formatContext;
    AVCodecContext*   m_videoCodecContext;
    AVCodecContext*   m_audioCodecContext;
    AVFrame*         m_videoFrame;
    AVFrame*         m_audioFrame;
    SwsContext*      m_swsContext;
    SwrContext*      m_swrContext;

    int m_videoStreamIndex;
    int m_audioStreamIndex;

    enum class InternalState {
        Stopped = 0,      ///< 完全停止，无媒体打开
        Opening = 1,      ///< 正在打开媒体文件
        Opened = 2,       ///< 媒体已打开，但未开始播放
        Starting = 3,     ///< 正在启动解码过程
        Playing = 4,      ///< 正在解码和播放
        Pausing = 5,      ///< 正在暂停解码过程
        Paused = 6,       ///< 已暂停
        Stopping = 7,     ///< 正在停止解码过程
        Seeking = 8,      ///< 正在执行跳转操作
        Closing = 9,      ///< 正在关闭媒体
        Error = 10        ///< 发生错误
    };

    InternalState m_internalState;

    bool initFFmpeg(const QString& uri);
    void deinitFFmpeg();
    void decodeLoop();
    void processVideoFrame();
    void processAudioFrame();
    void updatePosition();
    void setInternalState(InternalState newState);
};

}
}
}
}

#endif