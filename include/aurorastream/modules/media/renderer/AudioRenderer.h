/********************************************************************************
 * @file   : AudioRenderer.h
 * @brief  : 声明 AuroraStream 音频渲染器抽象基类。
 *
 * 此文件定义了 aurorastream::modules::media::renderer::AudioRenderer 类，
 * 它是所有具体音频渲染器实现（例如，基于 SDL2 的音频渲染器）的抽象基类。
 * 它规定了音频渲染器模块必须提供的公共接口和行为，并集成了 Qt 的信号与槽机制
 * 以支持事件通知。
 *
 * @author : polarours
 * @date   : 2025/8/24
 ********************************************************************************/

#ifndef AURORASTREAM_MODULES_MEDIA_RENDERER_AUDIOOUTPUT_H
#define AURORASTREAM_MODULES_MEDIA_RENDERER_AUDIOOUTPUT_H

#include <QObject>
#include <cstdint>
#include <memory>

#include "aurorastream/AuroraStream.h"

namespace aurorastream {
namespace modules {
namespace media {
namespace decoder {
struct AudioFrame;
}

namespace renderer {

/**
 * @brief AudioRenderer 是音频渲染器模块的抽象基类
 * 定义了音频渲染器的接口
 */
class  AURORASTREAM_API AudioRenderer  : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit AudioRenderer(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~AudioRenderer() override;

    // --- 禁用拷贝和移动 ---
    AudioRenderer(const AudioRenderer&) = delete;
    AudioRenderer& operator=(const AudioRenderer&) = delete;
    AudioRenderer(AudioRenderer&&) = delete;
    AudioRenderer& operator=(AudioRenderer&&) = delete;

    /**
     * @brief 初始化音频渲染器
     * @param sampleRate 采样率
     * @param channels 声道数
     * @param format 音频格式
     * @return 初始化是否成功
     */
    virtual bool initialize(int sampleRate, int channels, int format) = 0;

    /**
     * @brief 开始播放音频
     */
    virtual void play() = 0;

    /**
     * @brief 暂停播放音频
     */
    virtual void pause() = 0;

    /**
     * @brief 停止播放音频
     */
    virtual void stop() = 0;

    /**
     * @brief 加入音频帧到播放队列
     * @param frame 音频帧
     */
    virtual void queueAudio(const aurorastream::modules::media::decoder::AudioFrame& frame) = 0;

    /**
     * @brief 设置音量
     * @param volume 音量值
     */
    virtual void setVolume(float volume) = 0;

    /**
     * @brief 设置静音
     * @param mute 是否静音
     */
    virtual void setMute(bool mute) = 0;

    /**
     * @brief 清理音频渲染器
     */
    virtual void cleanup() = 0;

    /**
     * @brief 检查音频渲染器是否已初始化
     * @return 是否已初始化, true 表示已初始化
     */
    virtual bool isInitialized() const = 0;

signals:
    /**
     * @brief 音频渲染器状态发生变化时发出的信号
     * @param newState 新的状态
     */
    void stateChanged(aurorastream::modules::media::renderer::AudioRenderer::State newState);

    /**
     * @brief 音频渲染器发生错误时发出的信号
     * @param error 错误信息
     */
    void errorOccurred(const QString& error);
    void positionChanged(int64_t position);
    void finished();
    void playing();
    void paused();
    void stopped();
    void initialized();

protected:
    bool m_initialized{false};
    int m_sampleRate{0};
    int m_channels{0};
    int m_format{0};
    float m_volume{1.0f};
    bool m_mute{false};

};

} // namespace renderer
} // namespace media
} // namespace modules
} // namespace aurorastream

#endif // AURORASTREAM_MODULES_MEDIA_RENDERER_AUDIOOUTPUT_H