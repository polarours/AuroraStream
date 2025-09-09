/********************************************************************************
 * @file   : VideoRenderer.cpp
 * @brief  :
 *
 *
 *
 * @author : polarours
 * @date   : 2025/08/25
 ********************************************************************************/

#include "aurorastream/modules/media/renderer/VideoRenderer.h"

namespace aurorastream {
namespace modules {
namespace media {
namespace renderer {

VideoRenderer::VideoRenderer() : m_width(0), m_height(0), m_initialized(false) {}

VideoRenderer::~VideoRenderer() {}

bool VideoRenderer::initialize(int width, int height, void* windowHandle) {
    m_width = width;
    m_height = height;
    m_windowHandle = windowHandle;
    m_initialized = true;
    return true;
}

void VideoRenderer::resize(int width, int height) {
    m_width = width;
    m_height = height;
}

void VideoRenderer::render(const aurorastream::modules::media::decoder::VideoFrame& frame) {
    // Default implementation, should be overridden
}

void VideoRenderer::cleanup() {
    m_initialized = false;
}

bool VideoRenderer::isInitialized() const {
    return m_initialized;
}

} // namespace renderer
} // namespace media
} // namespace modules
} // namespace aurorastream