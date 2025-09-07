/********************************************************************************
 * @file   : AudioRenderer.cpp
 * @brief  :
 *
 *
 *
 * @Author : polarours
 * @Date   : 2025/08/25
 ********************************************************************************/

#include "aurorastream/modules/media/renderer/AudioRenderer.h"

namespace aurorastream {
namespace modules {
namespace media {
namespace renderer {

AudioRenderer::AudioRenderer() : m_initialized(false), m_sampleRate(0), m_channels(0) {}

AudioRenderer::~AudioRenderer() {}

bool AudioRenderer::initialize(int sampleRate, int channels) {
    m_sampleRate = sampleRate;
    m_channels = channels;
    m_initialized = true;
    return true;
}

void AudioRenderer::play() {
    // Default implementation, should be overridden
}

void AudioRenderer::pause() {
    // Default implementation, should be overridden
}

void AudioRenderer::stop() {
    // Default implementation, should be overridden
}

void AudioRenderer::setVolume(float volume) {
    // Default implementation, should be overridden
}

float AudioRenderer::getVolume() const {
    return 1.0f; // Default implementation, should be overridden
}

void AudioRenderer::setMute(bool mute) {
    // Default implementation, should be overridden
}

bool AudioRenderer::isMute() const {
    return false; // Default implementation, should be overridden
}

void AudioRenderer::render(const aurorastream::modules::media::decoder::AudioFrame& frame) {
    // Default implementation, should be overridden
}

void AudioRenderer::cleanup() {
    m_initialized = false;
}

bool AudioRenderer::isInitialized() const {
    return m_initialized;
}

} // namespace renderer
} // namespace media
} // namespace modules
} // namespace aurorastream