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

#ifndef AURORASTREAM_MEDIA_DECODER_FFMPEGDECODER_H
#define AURORASTREAM_MEDIA_DECODER_FFMPEGDECODER_H

#include <QObject>
#include <QString>
#include <QThread>
#include <memory>

#include "AuroraStream/AuroraStream.h"

#include "aurorastream/modules/media/decoder/Decoder.h"

// --- FFmpeg 前向声明 ---
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
/**
 * @brief FFmpeg 是基于 FFmpeg 库实现的媒体解码器，继承自 Decoder 抽象基类。
 */
class AURORASTREAM_API FFmpegDecoder : public Decoder
{
    Q_OBJECT

public:
	/**
     * @brief 构造函数
     * @param parent QObject 父对象指针
     */
    explicit FFmpegDecoder(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     * @note 会自动释放所有 FFmpeg 相关资源
     */
    ~FFmpegDecoder() override;

	// --- 禁止拷贝和移动 ---
    FFmpegDecoder(const FFmpegDecoder &) = delete;
    FFmpegDecoder &operator=(const FFmpegDecoder &) = delete;
    FFmpegDecoder(FFmpegDecoder &&) = delete;
    FFmpegDecoder &operator=(FFmpegDecoder &&) = delete;

    /**
     * @brief 打开媒体资源
     * @param uri 媒体资源路径
     * @return 成功返回 true，失败返回 false
	 */
    bool open(const QString &uri) override;

    /**
     * @brief 关闭当前媒体资源
     */
    void close() override;

    /**
     * @brief 开始解码并播放媒体
     */
	void start() override;

    /**
     * @brief 暂停解码和播放
     */
    bool pause() override;

    /**
     * @brief 停止解码和播放
     */
 	void stop() override;

    /**
     * @brief 跳转到指定位置
     * @param position 目标位置（毫秒）
     */
 	bool seek(qint64 position) override;

    /**
     * @brief 检查当前是否已打开媒体
     * @return 已打开返回 true，否则返回 false
     */
	bool isOpen() const override;

    /**
     * @brief 检查当前是否正在播放
     * @return 正在播放返回 true，否则返回 false
     */
	bool isPlaying() const override;

    /**
     * @brief 检查当前是否已暂停
     * @return 已暂停返回 true，否则返回 false
     */
 	bool isPaused() const override;

    /**
     * @brief 检查当前是否已停止
     * @return 已停止返回 true，否则返回 false
     */
 	bool isStopped() const override;

    /**
     * @brief 获取当前视频窗口宽度
     * @return 视频窗口宽度
     */
    int getVideoWidth() const override;

    /**
     * @brief 获取当前视频窗口高度
     * @return 视频窗口高度
     */
	int getVideoHeight() const override;

    /**
     * @brief 获取当前视频帧率
     * @return 视频帧率
     */
	int getVideoFrameRate() const override;

    /**
     * @brief 获取当前音频采样率
     * @return 音频采样率
     */
	int getAudioSampleRate() const override;

    /**
     * @brief 获取当前音频通道数
     * @return 音频通道数
     */
	int getAudioChannels() const override;

    /**
     * @brief 获取媒体总时长
     * @return 媒体总时长（毫秒）
     */
	qint64 getDuration() const override;

    /**
     * @brief 获取当前播放位置
     * @return 当前播放位置（毫秒）
     */
	qint64 getPosition() const override;

private slots:

protected:

    // ---  FFmpeg 相关资源 ---
	AVFrame*         m_videoFrame;         ///< 视频帧
    AVFrame*         m_audioFrame;         ///< 音频帧
    SwsContext*      m_swsContext;         ///< 视频转换上下文
    SwrContext*      m_swrContext;         ///< 音频转换上下文
    AVFormatContext*  m_formatContext;     ///< 媒体格式上下文
    AVCodecContext*   m_videoCodecContext; ///< 视频解码器上下文
    AVCodecContext*   m_audioCodecContext; ///< 音频解码器上下文

	// ---  媒体流索引 ---
    int m_videoStreamIndex = -1; ///< 视频流索引
    int m_audioStreamIndex = -1; ///< 音频流索引

	// --- 内部状态 ---
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

    InternalState m_internalState; ///< 当前内部状态

	// --- FFmpeg 相关方法 ---

	/**
     * @brief 初始化 FFmpeg 相关资源
     * @param uri 媒体资源路径
     * @return 成功返回 true，失败返回 false
     */
    bool initFFmpeg(const QString& uri);

    /**
     * @brief 释放 FFmpeg 相关资源
     */
	void deinitFFmpeg();

    /**
     * @brief 解码循环
     */
	void decodeLoop();

    /**
     * @brief 处理视频帧
     */
	void processVideoFrame();

    /**
     * @brief 处理音频帧
     */
	void processAudioFrame();

    /**
     * @brief 更新当前播放位置
     */
	void updatePosition();

    /**
     * @brief 设置内部状态
     * @param newState 新状态
     */
	void setInternalState(InternalState newState);
};

} // namespace decoder
} // namespace media
} // namespace modules
} // namespace aurorastream

#endif // AURORASTREAM_MEDIA_DECODER_FFMPEGDECODER_H