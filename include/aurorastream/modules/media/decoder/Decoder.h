/********************************************************************************
 * @file   : Decoder.h
 * @brief  : 声明 AuroraStream 媒体解码器抽象基类。
 *
 * 此文件定义了 aurorastream::modules::media::decoder::Decoder 类，
 * 它是所有具体解码器实现（例如，基于 FFmpeg 的解码器）的抽象基类。
 * 它规定了解码器模块必须提供的公共接口和行为。
 *
 * @author : polarours
 * @date   : 2025/08/22
 ********************************************************************************/

#ifndef AURORASTREAM_MODULES_MEDIA_DECODER_DECODER_H
#define AURORASTREAM_MODULES_MEDIA_DECODER_DECODER_H

#include <QObject>
#include <QString>

#include "AuroraStream/AuroraStream.h"

struct AVCodecContext;
struct AVCodecContext;
struct AVFrame;

namespace aurorastream {
namespace modules {
namespace media {
namespace decoder {

/**
 * @brief 视频帧数据结构，包含视频帧数据和相关信息
 */
struct AURORASTREAM_API AudioFrame {
    unsinged char* data[8] = { nullptr }; ///< 音频帧数据数组
    int linesize[8] = { 0 };              ///< 音频帧数据行大小数组
    int nb_samples =  0;                  ///< 音频帧采样数
    int sample_rate = 0;                  ///< 音频帧采样率
    int channels = 0;                     ///< 音频帧通道数
    int format = 0;                       ///< 音频帧格式
    int64_t pts = 0;                      ///< 音频帧显示时间戳
};

class AURORASTREAM_API Decoder : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent QObject 父对象
     */
    explicit Decoder(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~Decoder() override;

    /**
     * @brief 禁用拷贝和移动构造函数
     */
    Decoder(const Decoder&) = delete;

    /**
     * @brief 禁用拷贝和移动赋值运算符
     */
    Decoder& operator=(const Decoder&) = delete;

    /**
     * @brief 禁用移动构造函数
     */
    Decoder(Decoder&&) = delete;

    /**
     * @brief 禁用移动赋值运算符
     */
    Decoder& operator=(Decoder&&) = delete;

    /**
     * @brief 打开媒体文件
     * @param uri 媒体文件路径
     * @return 打开成功返回 true，失败返回 false
     */
    virtual bool open(const QString& uri) = 0;

    virtual void close() = 0;

    virtual void start() = 0;

    virtual void pause() = 0;

    virtual void stop() = 0;

    virtual void seek(qint64 position) = 0;

    virtual bool isOpen() const = 0;

    virtual bool isPlaying() const = 0;

    virtual bool isPaused() const = 0;

    virtual bool isStopped() const = 0;

    virtual qint64 getDuration() const = 0;

    virtual qint64 getPosition() const = 0;

    virtual int getVideoWidth() const = 0;

    virtual int getVideoHeight() const = 0;

    virtual int getVideoFrameRate() const = 0;

    virtual int getAudioSampleRate() const = 0;

    virtual int getAudioChannels() const = 0;

signals:
    void videoFrameReady(const aurorastream::modules::media::decoder::VideoFrame& frame);

    void audioFrameReady(const aurorastream::modules::media::decoder::AudioFrame& frame);

    void durationChanged(qint64 duration);

    void positionChanged(qint64 position);

    void error(const QString& message);

    void finished();

protected:
    QString m_uri;
    bool m_isOpen = false;
    qint64 m_duration = 0;
    qint64 m_position = 0;

};

}
}
}
}

#endif // AURORASTREAM_MODULES_MEDIA_DECODER_DECODER_H