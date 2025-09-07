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

class  AURORASTREAM_API AudioRenderer  : public QObject
{
    Q_OBJECT

public:
    explicit AudioRenderer(QObject *parent = nullptr);
    ~AudioRenderer() override;

    AudioRenderer(const AudioRenderer&) = delete;
    AudioRenderer& operator=(const AudioRenderer&) = delete;
    AudioRenderer(AudioRenderer&&) = delete;
    AudioRenderer& operator=(AudioRenderer&&) = delete;

    virtual bool initialize(int sampleRate, int channels, int format) = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void queueAudio(const aurorastream::modules::media::decoder::AudioFrame& frame) = 0;
    virtual void setVolume(float volume) = 0;
    virtual void setMute(bool mute) = 0;
    virtual void cleanup() = 0;
    virtual bool isInitialized() const = 0;

signals:
    void stateChanged(aurorastream::modules::media::renderer::AudioRenderer::State state);
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

}

}
}
}


#endif // AURORASTREAM_MODULES_MEDIA_RENDERER_AUDIOOUTPUT_H
