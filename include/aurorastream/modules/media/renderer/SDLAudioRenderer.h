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
/**
 * @brief 基于 SDL2 的具体音频渲染器实现类
 */
class AURORASTREAM_API SDLAudioRenderer : public AudioRenderer
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent QObject 父对象指针
     */
    explicit SDLAudioRenderer(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~SDLAudioRenderer() override;

    // --- 禁用拷贝和移动 ---
    SDLAudioRenderer(const SDLAudioRenderer &) = delete;
    SDLAudioRenderer &operator=(const SDLAudioRenderer &) = delete;
    SDLAudioRenderer(SDLAudioRenderer &&) = delete;
    SDLAudioRenderer &operator=(SDLAudioRenderer &&) = delete;

    /**
     * @brief 初始化 SDL2 音频渲染器
     * @return  true 初始化成功，false 初始化失败
     */
    bool initialize(int sampleRate, int channels, int foramt) override;

    /**
     * @brief 开始播放音频
     */
    void play() override;

    /**
     * @brief 暂停播放音频
     */
    void pause() override;

    /**
     * @brief 停止播放音频
     */
    void stop() override;

    /**
     * @brief 设置音量
     * @param volume 音量值
     */
    void setVolume(float volume) override;

    /**
     * @brief 获取当前音量
     * @return 当前音量值
     */
    float getVolume() const override;

    /**
     * @brief 设置静音状态
     * @param mute  true 静音，false 非静音
     */
    void setMute(bool mute) override;

    /**
     * @brief 获取当前静音状态
     * @return true 静音，false 非静音
     */
    bool isMute() const override;

    /**
     * @brief 将音频帧加入播放队列
     * @param frame 音频帧
     */
    void queueAudio(const aurorastream::modules::media::decoer::AudioFrame& frame) override;

    /**
     * @brief 清空播放队列
     */
    void cleanup() override;

    /**
     * @brief 检查 SDL2 音频渲染器是否已初始化
     * @return true 已初始化，false 未初始化
     */
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

    // --- SDL2 音频相关私有方法 ---
    /**
     * @brief 创建或更新 SDL2 音频设备
     * @param sampleRate 音频采样率
     * @param channels 音频声道数
     * @param format 音频样本格式
     * @return true 成功，false 失败
     */
    bool createOrUpdateAudioDevice(int sampleRate, int channels, int format);

    /**
     * @brief 销毁 SDL2 音频设备
     */
    void destroyAudioDevice();

    /**
     * @brief 将音频帧推入 SDL2 音频设备
     * @param frame 音频帧
     * @return true 成功，false 失败
     */
    bool pushAudioFrame(const aurorastream::modules::media::decoder::AudioFrame& frame);

    /**
     * @brief 更新音频参数
     * @param sampleRate 音频采样率
     * @param channels 音频声道数
     * @param format 音频样本格式
     */
    void updateAudioParams(int sampleRate, int channels, int format);

    /**
     * @brief 更新内部状态
     * @param newState 新的内部状态
     */
    void setInternalState(InternalState newState);

    /**
     * @brief SDL2 音频回调函数
     * @param userdata 用户数据指针
     * @param stream 音频流缓冲区
     * @param len 音频流缓冲区长度
     */
    static void audioCallback(void* userdata, unsigned char* stream, int len);
};

} // namespace renderer
} // namespace media
} // namespace modules
} // namespace aurorastream

#endif // AURORASTREAM_MODULES_MEDIA_RENDERER_SDLAUDIORENDERER_H
