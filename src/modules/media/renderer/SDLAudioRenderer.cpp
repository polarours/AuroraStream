/********************************************************************************
 * @file   : SDLAudioRenderer.cpp
 * @brief  : AuroraStream SDL音频渲染器模块的实现
 *
 * 本文件实现了 SDLAudioRenderer 类，它提供了SDL音频渲染器的基本功能
 * 包括初始化、播放控制、状态管理等
 *
 * @author : polarours
 * @date   : 2025/08/25
 ********************************************************************************/

#include "aurorastream/modules/media/renderer/SDLAudioRenderer.h"
#include <SDL.h>
#include <QDebug>

namespace aurorastream {
namespace modules {
namespace media {
namespace renderer {

SDLAudioRenderer::SDLAudioRenderer(QObject *parent) : AudioRenderer(parent),
    m_audioDeviceID(nullptr),
    m_audioSpec(nullptr),
    m_internalState(InternalState::Uninitialized),
    m_sampleRate(0),
    m_channels(0),
    m_format(0),
    m_volume(1.0f),
    m_muted(false) {
    qDebug() << "SDLAudioRenderer created";
}

SDLAudioRenderer::~SDLAudioRenderer() {
    cleanup();
    qDebug() << "SDLAudioRenderer destroyed";
}

bool SDLAudioRenderer::initialize(int sampleRate, int channels, int format) {
    if (m_internalState != InternalState::Uninitialized) {
        qWarning() << "SDLAudioRenderer already initialized";
        return false;
    }

    m_sampleRate = sampleRate;
    m_channels = channels;
    m_format = format;

    if (!createOrUpdateAudioDevice(sampleRate, channels, format)) {
        setInternalState(InternalState::Error);
        return false;
    }

    setInternalState(InternalState::Initialized);
    qDebug() << "SDLAudioRenderer initialized";
    return true;
}

void SDLAudioRenderer::play() {
    if (m_internalState != InternalState::Initialized && m_internalState != InternalState::Paused) {
        qWarning() << "SDLAudioRenderer not ready for playback";
        return;
    }

    setInternalState(InternalState::Starting);
    SDL_PauseAudioDevice(m_audioDeviceID, 0);
    setInternalState(InternalState::Playing);
    qDebug() << "SDLAudioRenderer playback started";
}

void SDLAudioRenderer::pause() {
    if (m_internalState != InternalState::Playing) {
        qWarning() << "SDLAudioRenderer not playing";
        return;
    }

    setInternalState(InternalState::Pausing);
    SDL_PauseAudioDevice(m_audioDeviceID, 1);
    setInternalState(InternalState::Paused);
    qDebug() << "SDLAudioRenderer playback paused";
}

void SDLAudioRenderer::stop() {
    if (m_internalState == InternalState::Stopped) {
        return;
    }

    setInternalState(InternalState::Stopping);
    SDL_PauseAudioDevice(m_audioDeviceID, 1);
    destroyAudioDevice();
    setInternalState(InternalState::Stopped);
    qDebug() << "SDLAudioRenderer playback stopped";
}

void SDLAudioRenderer::setVolume(float volume) {
    m_volume = qBound(0.0f, volume, 1.0f);
    qDebug() << "SDLAudioRenderer volume set to" << m_volume;
}

float SDLAudioRenderer::getVolume() const {
    return m_volume;
}

void SDLAudioRenderer::setMute(bool mute) {
    m_muted = mute;
    qDebug() << "SDLAudioRenderer mute set to" << m_muted;
}

bool SDLAudioRenderer::isMute() const {
    return m_muted;
}

void SDLAudioRenderer::queueAudio(const aurorastream::modules::media::decoder::AudioFrame& frame) {
    if (m_internalState != InternalState::Playing) {
        qWarning() << "SDLAudioRenderer not playing";
        return;
    }

    if (!pushAudioFrame(frame)) {
        qWarning() << "Failed to queue audio frame";
    }
}

void SDLAudioRenderer::cleanup() {
    stop();
    qDebug() << "SDLAudioRenderer cleaned up";
}

bool SDLAudioRenderer::isInitialized() const {
    return m_internalState != InternalState::Uninitialized && m_internalState != InternalState::Error;
}

bool SDLAudioRenderer::createOrUpdateAudioDevice(int sampleRate, int channels, int format) {
    if (m_audioDeviceID) {
        destroyAudioDevice();
    }

    SDL_AudioSpec desiredSpec;
    SDL_zero(desiredSpec);
    desiredSpec.freq = sampleRate;
    desiredSpec.channels = channels;
    desiredSpec.format = format;
    desiredSpec.callback = audioCallback;
    desiredSpec.userdata = this;

    m_audioDeviceID = SDL_OpenAudioDevice(nullptr, 0, &desiredSpec, &m_audioSpec, 0);
    if (!m_audioDeviceID) {
        qCritical() << "Failed to open SDL audio device:" << SDL_GetError();
        return false;
    }

    return true;
}

void SDLAudioRenderer::destroyAudioDevice() {
    if (m_audioDeviceID) {
        SDL_CloseAudioDevice(m_audioDeviceID);
        m_audioDeviceID = nullptr;
    }

    if (m_audioSpec) {
        delete m_audioSpec;
        m_audioSpec = nullptr;
    }
}

bool SDLAudioRenderer::pushAudioFrame(const aurorastream::modules::media::decoder::AudioFrame& frame) {
    if (!m_audioDeviceID || !m_audioSpec) {
        return false;
    }

    // Implement audio frame processing logic here
    return true;
}

void SDLAudioRenderer::updateAudioParams(int sampleRate, int channels, int format) {
    m_sampleRate = sampleRate;
    m_channels = channels;
    m_format = format;
}

void SDLAudioRenderer::setInternalState(InternalState newState) {
    if (m_internalState != newState) {
        m_internalState = newState;
        emit stateChanged(static_cast<int>(newState));
    }
}

void SDLAudioRenderer::audioCallback(void* userdata, unsigned char* stream, int len) {
    SDLAudioRenderer* renderer = static_cast<SDLAudioRenderer*>(userdata);
    if (!renderer) {
        return;
    }

    // Implement audio callback logic here
}

} // namespace renderer
} // namespace media
} // namespace modules
} // namespace aurorastream