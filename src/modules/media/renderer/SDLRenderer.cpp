/********************************************************************************
 * @file   : SDLRenderer.cpp
 * @brief  :
 *
 *
 *
 * @author : polarours
 * @date   : 2025/08/25
 ********************************************************************************/

#include "aurorastream/modules/media/renderer/SDLRenderer.h"
#include <SDL.h>
#include <QDebug>

namespace aurorastream {
namespace modules {
namespace media {
namespace renderer {

SDLRenderer::SDLRenderer() : m_window(nullptr), m_renderer(nullptr), m_texture(nullptr) {
    qDebug() << "SDLRenderer created";
}

SDLRenderer::~SDLRenderer() {
    cleanup();
    qDebug() << "SDLRenderer destroyed";
}

bool SDLRenderer::initialize(int width, int height, void* windowHandle) {
    if (m_window) {
        qWarning() << "SDLRenderer already initialized";
        return false;
    }

    if (!windowHandle) {
        m_window = SDL_CreateWindow("AuroraStream", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    } else {
        m_window = SDL_CreateWindowFrom(windowHandle);
    }

    if (!m_window) {
        qCritical() << "Failed to create SDL window:" << SDL_GetError();
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) {
        qCritical() << "Failed to create SDL renderer:" << SDL_GetError();
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
        return false;
    }

    qDebug() << "SDLRenderer initialized";
    return true;
}

void SDLRenderer::resize(int width, int height) {
    if (!m_window) {
        qWarning() << "SDLRenderer not initialized";
        return;
    }

    SDL_SetWindowSize(m_window, width, height);
    qDebug() << "SDLRenderer resized to" << width << "x" << height;
}

void SDLRenderer::renderVideo(const aurorastream::modules::media::decoder::VideoFrame& frame) {
    if (!m_renderer || !m_texture) {
        qWarning() << "SDLRenderer not ready for rendering";
        return;
    }

    if (!createOrUpdateTexture(frame)) {
        qWarning() << "Failed to update texture";
        return;
    }

    presentTexture();
}

void SDLRenderer::cleanup() {
    if (m_texture) {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }

    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }

    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    qDebug() << "SDLRenderer cleaned up";
}

bool SDLRenderer::isInitialized() const {
    return m_window && m_renderer;
}

bool SDLRenderer::createOrUpdateTexture(const aurorastream::modules::media::decoder::VideoFrame& frame) {
    if (!m_renderer) {
        return false;
    }

    if (!m_texture) {
        m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, frame.width, frame.height);
        if (!m_texture) {
            qCritical() << "Failed to create SDL texture:" << SDL_GetError();
            return false;
        }
    }

    return updateTexture(frame);
}

bool SDLRenderer::updateTexture(const aurorastream::modules::media::decoder::VideoFrame& frame) {
    if (!m_texture) {
        return false;
    }

    if (SDL_UpdateYUVTexture(m_texture, nullptr,
                            frame.data[0], frame.linesize[0],
                            frame.data[1], frame.linesize[1],
                            frame.data[2], frame.linesize[2]) != 0) {
        qCritical() << "Failed to update SDL texture:" << SDL_GetError();
        return false;
    }

    return true;
}

void SDLRenderer::presentTexture() {
    if (!m_renderer || !m_texture) {
        return;
    }

    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);
    SDL_RenderPresent(m_renderer);
}

} // namespace renderer
} // namespace media
} // namespace modules
} // namespace aurorastream