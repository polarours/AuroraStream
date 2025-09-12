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
#include "../decoder/Decoder.h"

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
class AURORASTREAM_API AudioRenderer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool initialized READ isInitialized NOTIFY initializedChanged)
    Q_PROPERTY(float volume READ getVolume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ isMute WRITE setMute NOTIFY muteChanged)

public:
    enum class State {
        Stopped,
        Playing,
        Paused,
        Error
    };
    Q_ENUM(State)

    explicit AudioRenderer(QObject* parent = nullptr);
    ~AudioRenderer() override;

    // 禁用拷贝和移动
    AudioRenderer(const AudioRenderer&) = delete;
    AudioRenderer& operator=(const AudioRenderer&) = delete;
    AudioRenderer(AudioRenderer&&) = delete;
    AudioRenderer& operator=(AudioRenderer&&) = delete;

    // 核心接口
    /**
     * @brief 初始化音频设备
     * @param sampleRate 采样率 (48000, 44100等)
     * @param channels 声道数 (1-单声道, 2-立体声)
     * @param format 音频格式 (SDL_AUDIO_*格式常量)
     * @return 初始化成功返回true
     */
    virtual bool initialize(int sampleRate, int channels, int format) = 0;

    /// 开始播放音频数据
    virtual void play() = 0;

    /// 暂停播放(保留音频数据)
    virtual void pause() = 0;

    /// 停止播放(清空音频数据)
    virtual void stop() = 0;

    /**
     * @brief 添加音频帧到播放队列
     * @param frame 包含PCM数据的音频帧
     * @note 线程安全操作
     */
    virtual void queueAudio(const aurorastream::modules::media::decoder::AudioFrame& frame) = 0;

    /// 释放所有音频资源
    virtual void cleanup() = 0;

    /// 检查音频设备是否已初始化
    virtual bool isInitialized() const = 0;

    /// 支持的音频格式枚举
    enum AudioFormat {
        AUDIO_S8 = 0x8008,
        AUDIO_U8 = 0x0008,
        AUDIO_S16LSB = 0x8010,
        AUDIO_S16MSB = 0x9010,
        AUDIO_S32LSB = 0x8020
    };
    Q_ENUM(AudioFormat)

    // 音频控制
    virtual void setVolume(float volume);
    virtual float getVolume() const;
    virtual void setMute(bool mute);
    virtual bool isMute() const;

signals:
    void stateChanged(State newState);
    void errorOccurred(const QString& error);
    void positionChanged(int64_t position);
    void volumeChanged(float volume);
    void muteChanged(bool muted);
    void initializedChanged(bool initialized);

protected:
    int m_sampleRate {0};
    int m_channels {0};
    int m_format {0};
    float m_volume {1.0f};
    bool m_mute {false};
    bool m_initialized {false};
    State m_state {State::Stopped};
};

} // namespace renderer
} // namespace media
} // namespace modules
} // namespace aurorastream

#endif // AURORASTREAM_MODULES_MEDIA_RENDERER_AUDIOOUTPUT_H