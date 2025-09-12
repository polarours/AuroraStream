#include "aurorastream/modules/media/renderer/AudioRenderer.h"
#include <SDL2/SDL.h>
#include <QDebug>
#include <mutex>

namespace aurorastream {
namespace modules {
namespace media {
namespace renderer {

class SDLAudioRenderer : public AudioRenderer {
public:
    SDLAudioRenderer(QObject* parent = nullptr);
    ~SDLAudioRenderer() override;

    bool initialize(int sampleRate, int channels, int format) override;
    void play() override;
    void pause() override;
    void stop() override;
    void queueAudio(const decoder::AudioFrame& frame) override;
    void cleanup() override;
    bool isInitialized() const override;

private:
    static void audioCallback(void* userdata, Uint8* stream, int len);

    SDL_AudioDeviceID m_audioDevice = 0;
    std::mutex m_audioMutex;
    std::vector<uint8_t> m_audioBuffer;
};

SDLAudioRenderer::SDLAudioRenderer(QObject* parent) :
    AudioRenderer(parent)
{
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        qCritical() << "SDL audio init failed:" << SDL_GetError();
    }
}

SDLAudioRenderer::~SDLAudioRenderer() {
    cleanup();
}

bool SDLAudioRenderer::initialize(int sampleRate, int channels, int format) {
    if (m_initialized) return true;

    SDL_AudioSpec desired, obtained;
    SDL_zero(desired);

    desired.freq = sampleRate;
    desired.channels = channels;
    desired.format = AUDIO_S16SYS;
    desired.samples = 4096;
    desired.callback = audioCallback;
    desired.userdata = this;

    m_audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (m_audioDevice == 0) {
        qCritical() << "Open audio device failed:" << SDL_GetError();
        return false;
    }

    m_sampleRate = obtained.freq;
    m_channels = obtained.channels;
    m_format = obtained.format;
    m_initialized = true;
    return true;
}

void SDLAudioRenderer::play() {
    if (!m_initialized) return;
    SDL_PauseAudioDevice(m_audioDevice, 0);
    m_state = State::Playing;
    emit stateChanged(m_state);
}

void SDLAudioRenderer::pause() {
    if (!m_initialized) return;
    SDL_PauseAudioDevice(m_audioDevice, 1);
    m_state = State::Paused;
    emit stateChanged(m_state);
}

void SDLAudioRenderer::stop() {
    if (!m_initialized) return;
    SDL_PauseAudioDevice(m_audioDevice, 1);
    std::lock_guard<std::mutex> lock(m_audioMutex);
    m_audioBuffer.clear();
    m_state = State::Stopped;
    emit stateChanged(m_state);
}

void SDLAudioRenderer::queueAudio(const decoder::AudioFrame& frame) {
    if (!m_initialized) return;

    std::lock_guard<std::mutex> lock(m_audioMutex);
    // 修复向量插入语法
    size_t dataSize = frame.samples * m_channels * 2; // 假设16位样本
    m_audioBuffer.insert(m_audioBuffer.end(),
                        frame.data[0],
                        frame.data[0] + dataSize);
}

void SDLAudioRenderer::cleanup() {
    if (m_audioDevice) {
        SDL_CloseAudioDevice(m_audioDevice);
        m_audioDevice = 0;
    }
    m_initialized = false;
}

bool SDLAudioRenderer::isInitialized() const {
    return m_initialized;
}

void SDLAudioRenderer::audioCallback(void* userdata, Uint8* stream, int len) {
    SDLAudioRenderer* renderer = static_cast<SDLAudioRenderer*>(userdata);
    std::lock_guard<std::mutex> lock(renderer->m_audioMutex);

    size_t copySize = std::min(renderer->m_audioBuffer.size(), static_cast<size_t>(len));
    if (copySize > 0) {
        memcpy(stream, renderer->m_audioBuffer.data(), copySize);
        renderer->m_audioBuffer.erase(renderer->m_audioBuffer.begin(),
                                   renderer->m_audioBuffer.begin() + copySize);
    } else {
        memset(stream, 0, len);
    }
}

} // namespace renderer
} // namespace media
} // namespace modules
} // namespace aurorastream