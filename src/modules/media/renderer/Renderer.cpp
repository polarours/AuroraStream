/********************************************************************************
 * @file   : Renderer.cpp
 * @brief  :
 *
 *
 *
 * @Author : polarours
 * @Date   : 2025/08/25
 ********************************************************************************/

#include "aurorastream/modules/media/renderer/Renderer.h"

namespace aurorastream {
namespace modules {
namespace media {
namespace renderer {

Renderer::Renderer() : m_width(0), m_height(0), m_initialized(false), m_windowHandle(nullptr) {}

Renderer::~Renderer() {}

bool Renderer::initialize(int width, int height, void* windowHandle) {
    return false; // Default implementation, should be overridden
}

void Renderer::resize(int width, int height) {
    // Default implementation, should be overridden
}

void Renderer::renderVideo(const aurorastream::modules::media::decoder::VideoFrame& frame) {
    // Default implementation, should be overridden
}

void Renderer::renderAudio(const aurorastream::modules::media::decoder::AudioFrame& frame) {
    // Default implementation, should be overridden
}

void Renderer::cleanup() {
    // Default implementation, should be overridden
}

bool Renderer::isInitialized() const {
    return m_initialized;
}

} // namespace renderer
} // namespace media
} // namespace modules
} // namespace aurorastream