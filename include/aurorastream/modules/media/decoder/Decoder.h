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

// --- 前向声明 ---
struct AVCodecContext;
struct AVCodecContext;
struct AVFrame;

namespace aurorastream {
namespace modules {
namespace media {
namespace decoder {
/**
 * @brief 视频帧数据结构，包含视频帧数据和相关信息。
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

/**
 * @brief Decoder 类是所有具体解码器实现的抽象基类。
 */
class AURORASTREAM_API Decoder : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数。
     * @param parent QObject 父对象。
     */
    explicit Decoder(QObject* parent = nullptr);

    /**
     * @brief 析构函数。
     */
    ~Decoder() override;

    /**
     * @brief 禁用拷贝和移动构造函数。
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

    /**
     * @brief 关闭媒体文件
	 */
    virtual void close() = 0;

    /**
	 * @brief 开始播放
	 */
    virtual void start() = 0;

	/**
	 * @brief 暂停播放
     */
    virtual void pause() = 0;

    /**
	 * @brief 停止播放
	 */
    virtual void stop() = 0;

    /**
     * @brief 跳转到指定位置
     * @param position 指定位置
     */
    virtual void seek(qint64 position) = 0;

    /**
     * @brief 检查媒体文件是否打开
     * @return 如果媒体文件已经打开则返回 true，否则返回 false
     */
    virtual bool isOpen() const = 0;

    /**
     * @brief 检查媒体文件是否正在播放
     * @return 如果媒体文件正在播放则返回 true，否则返回 false
     */
    virtual bool isPlaying() const = 0;

    /**
     * @brief 检查媒体文件是否已经暂停
     * @return 如果媒体文件已经暂停则返回 true，否则返回 false
     */
    virtual bool isPaused() const = 0;

    /**
     * @brief 检查媒体文件是否已经停止
     * @return 如果媒体文件已经停止则返回 true，否则返回 false
     */
    virtual bool isStopped() const = 0;

    /**
     * @brief 获取媒体文件的总时长
     * @return 媒体文件的总时长（毫秒）
     */
    virtual qint64 getDuration() const = 0;

    /**
     * @brief 获取媒体文件的当前播放位置
     * @return 媒体文件的当前播放位置（毫秒）
     */
    virtual qint64 getPosition() const = 0;

    /**
     * @brief 获取媒体文件的视频宽度
     * @return 媒体文件的视频宽度
     */
    virtual int getVideoWidth() const = 0;

    /**
     * @brief 获取媒体文件的视频高度
     * @return 媒体文件的视频高度
     */
    virtual int getVideoHeight() const = 0;

    /**
     * @brief 获取媒体文件的视频帧率
     * @return 媒体文件的视频帧率
     */
    virtual int getVideoFrameRate() const = 0;

    /**
     * @brief 获取媒体文件的音频采样率
     * @return 媒体文件的音频采样率
     */
    virtual int getAudioSampleRate() const = 0;

    /**
     * @brief 获取媒体文件的音频通道数
     * @return 媒体文件的音频通道数
     */
    virtual int getAudioChannels() const = 0;

signals:
	/**
     * @brief 视频帧就绪信号
     * @param frame 视频帧数据
     */
    void videoFrameReady(const aurorastream::modules::media::decoder::VideoFrame& frame);

    /**
     * @brief 音频帧就绪信号
     * @param frame 音频帧数据
     */
    void audioFrameReady(const aurorastream::modules::media::decoder::AudioFrame& frame);

    /**
     * @brief 媒体文件总时长改变信号
     * @param duration 媒体文件的总时长（毫秒）
     */
    void durationChanged(qint64 duration);

    /**
     * @brief 媒体文件当前播放位置改变信号
     * @param position 媒体文件的当前播放位置（毫秒）
     */
    void positionChanged(qint64 position);

    /**
     * @brief 媒体文件播放状态改变信号
     * @param state 媒体文件的播放状态
     */
    void error(const QString& message);

    /**
     * @brief 媒体文件播放结束信号
     */
    void finished();

protected:
    QString m_uri;         ///< 媒体文件路径
    qint64 m_duration = 0; ///< 总时长
    qint64 m_position = 0; ///< 当前播放位置
	bool m_isOpen = false; ///< 媒体文件是否已经打开
};

} // namespace decoder
} // namespace media
} // namespace modules
} // namespace aurorastream

#endif // AURORASTREAM_MODULES_MEDIA_DECODER_DECODER_H