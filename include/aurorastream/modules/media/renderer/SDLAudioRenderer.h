/********************************************************************************
 * @file   : SDLAudioRenderer.h
 * @brief  : 声明基于 SDL2 的具体音频渲染器类。
 *
 * 此文件定义了 aurorastream::modules::media::renderer::SDLAudioRenderer 类，
 * 它是 AudioRenderer 抽象基类的具体实现，使用 SDL2 库进行音频播放。
 *
 * @author : polarours
 * @date   : 2025/8/24
 ********************************************************************************/

#ifndef AURORASTREAM_MODULES_MEDIA_RENDERER_SDLAUDIORENDERER_H
#define AURORASTREAM_MODULES_MEDIA_RENDERER_SDLAUDIORENDERER_H

#include <memory>
#include <cstdint>

#include "aurorastream/AudioRenderer.h"
#include "aurorastream/modules/media/renderer/AudioRenderer.h"

struct SDL_AudioDeviceID;
struct SDL_AudioSpec;

namespace aurorastream {
namespace modules {
namespace media {

namespace decoer {
    class AudioFrame;
}

namespace renderer {

class AURORASTREAM_API SDLAudioRenderer : public AudioRenderer
{
    Q_OBJECT

public:
    explicit SDLAudioRenderer(QObject *parent = nullptr);
    ~SDLAudioRenderer() override;

    SDLAudioRenderer(const SDLAudioRenderer &) = delete;
    SDLAudioRenderer &operator=(const SDLAudioRenderer &) = delete;
    SDLAudioRenderer(SDLAudioRenderer &&) = delete;
    SDLAudioRenderer &operator=(SDLAudioRenderer &&) = delete;

    bool initialize(int sampleRate, int channels, int foramt) override;
    void play() override;
    void pause() override;
    void stop() override;
    void setVolume(float volume) override;
    float getVolume() const override;
    void setMute(bool mute) override;
    bool isMute() const override;
    void queueAudio(const aurorastream::modules::media::decoer::AudioFrame& frame) override;
    void cleanup() override;
    bool isInitialized() const override;

private slots:

protected:
    SDL_AudioDeviceID* m_audioDeviceID = nullptr;
    SDL_AudioSpec* m_audioSpec = nullptr;

    enum class InternalState {
        Uninitialized = 0, ///< 未初始化
        Initialized = 1,   ///< 已初始化，但未打开设备
        Opening = 2,       ///< 正在打开音频设备
        Opened = 3,        ///< 音频设备已打开
        Starting = 4,      ///< 正在启动音频播放
        Playing = 5,       ///< 正在播放音频
        Pausing = 6,       ///< 正在暂停音频播放
        Paused = 7,        ///< 已暂停音频播放
        Stopping = 8,      ///< 正在停止音频播放
        Stopped = 9,       ///< 已停止音频播放
        Closing = 10,      ///< 正在关闭音频设备
        Error = 11         ///< 发生错误
    };

    InternalState m_internalState; ///< 当前内部状态

    // --- 音频参数缓存 ---
    int m_sampleRate;     ///< 缓存的音频采样率
    int m_channels;       ///< 缓存的音频声道数
    int m_format;         ///< 缓存的音频样本格式
    float m_volume;       ///< 缓存的音量 (0.0 to 1.0)
    bool m_muted;         ///< 缓存的静音状态

    bool createOrUpdateAudioDevice(int sampleRate, int channels, int format);
    void destroyAudioDevice();
    bool pushAudioFrame(const aurorastream::modules::media::decoder::AudioFrame& frame);
    void updateAudioParams(int sampleRate, int channels, int format);
    void setInternalState(InternalState newState);
    static void audioCallback(void* userdata, unsigned char* stream, int len);

};

} // namespace renderer
} // namespace media
} // namespace modules
} // namespace aurorastream

#endif // AURORASTREAM_MODULES_MEDIA_RENDERER_SDLAUDIORENDERER_H
