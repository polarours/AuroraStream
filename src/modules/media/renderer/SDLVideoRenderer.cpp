/********************************************************************************
 * @file   : SDLVideoRenderer.cpp
 * @brief  :
 *
 *
 *
 * @Author : polarours
 * @Date   : 2025/08/25
 ********************************************************************************/

#include "aurorastream/modules/media/renderer/SDLVideoRenderer.h"
#include <SDL.h>
#include <QDebug>

namespace aurorastream {
namespace modules {
namespace media {
namespace renderer {

SDLVideoRenderer::SDLVideoRenderer() : m_window(nullptr), m_renderer(nullptr), m_texture(nullptr) {
    qDebug() << "SDLVideoRenderer created";
}

SDLVideoRenderer::~SDLVideoRenderer() {
    cleanup();
    qDebug() << "SDLVideoRenderer destroyed";
}

bool SDLVideoRenderer::initialize(void* hwnd) {
    if (m_window) {
        qWarning() << "SDLVideoRenderer already initialized";
        return false;
    }

    m_window = SDL_CreateWindowFrom(hwnd);
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

    qDebug() << "SDLVideoRenderer initialized";
    return true;
}

void SDLVideoRenderer::resize(int width, int height) {
    if (!m_window) {
        qWarning() << "SDLVideoRenderer not initialized";
        return;
    }

    SDL_SetWindowSize(m_window, width, height);
    qDebug() << "SDLVideoRenderer resized to" << width << "x" << height;
}

bool SDLVideoRenderer::render(const aurorastream::modules::media::decoder::VideoFrame* frame) {
    if (!m_renderer || !m_texture) {
        qWarning() << "SDLVideoRenderer not ready for rendering";
        return false;
    }

    if (!createOrUpdateTexture(frame)) {
        qWarning() << "Failed to update texture";
        return false;
    }

    presentTexture();
    return true;
}

void SDLVideoRenderer::cleanup() {
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

    qDebug() << "SDLVideoRenderer cleaned up";
}

bool SDLVideoRenderer::isInitialized() {
    return m_window && m_renderer;
}

bool SDLVideoRenderer::createOrUpdateTexture(const aurorastream::modules::media::decoder::VideoFrame* frame) {
    if (!m_renderer || !frame) {
        return false;
    }

    if (!m_texture) {
        m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, frame->width, frame->height);
        if (!m_texture) {
            qCritical() << "Failed to create SDL texture:" << SDL_GetError();
            return false;
        }
    }

    return updateTexture(frame);
}

bool SDLVideoRenderer::updateTexture(const aurorastream::modules::media::decoder::VideoFrame* frame) {
    if (!m_texture || !frame) {
        return false;
    }

    if (SDL_UpdateYUVTexture(m_texture, nullptr,
                            frame->data[0], frame->linesize[0],
                            frame->data[1], frame->linesize[1],
                            frame->data[2], frame->linesize[2]) != 0) {
        qCritical() << "Failed to update SDL texture:" << SDL_GetError();
        return false;
    }

    return true;
}

void SDLVideoRenderer::presentTexture() {
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