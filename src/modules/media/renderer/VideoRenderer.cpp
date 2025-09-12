#include "aurorastream/modules/media/renderer/VideoRenderer.h"
#include <SDL2/SDL.h>
#include <QDebug>

namespace aurorastream {
namespace modules {
namespace media {
namespace renderer {

class SDLVideoRenderer : public VideoRenderer {
public:
    SDLVideoRenderer(QObject* parent = nullptr);
    ~SDLVideoRenderer() override;

    bool initialize(int width, int height, void* windowHandle) override;
    void resize(int width, int height) override;
    void render(const decoder::VideoFrame& frame) override;
    void cleanup() override;
    bool isInitialized() const override;

private:
    SDL_Window* m_sdlWindow = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    SDL_Texture* m_texture = nullptr;
};

SDLVideoRenderer::SDLVideoRenderer(QObject* parent) :
    VideoRenderer(parent)
{
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        qCritical() << "SDL video init failed:" << SDL_GetError();
    }
}

SDLVideoRenderer::~SDLVideoRenderer() {
    cleanup();
}

bool SDLVideoRenderer::initialize(int width, int height, void* windowHandle) {
    if (m_initialized) return true;

    m_sdlWindow = SDL_CreateWindowFrom(windowHandle);
    if (!m_sdlWindow) {
        qCritical() << "Create SDL window failed:" << SDL_GetError();
        return false;
    }

    Uint32 renderFlags = SDL_RENDERER_ACCELERATED;
    if (m_hwAccel != HardwareAccel::None) {
        renderFlags |= SDL_RENDERER_PRESENTVSYNC;
    }

    m_renderer = SDL_CreateRenderer(m_sdlWindow, -1, renderFlags);
    if (!m_renderer) {
        qCritical() << "Create renderer failed:" << SDL_GetError();
        return false;
    }

    m_texture = SDL_CreateTexture(m_renderer,
                                SDL_PIXELFORMAT_IYUV,
                                SDL_TEXTUREACCESS_STREAMING,
                                width, height);
    if (!m_texture) {
        qCritical() << "Create texture failed:" << SDL_GetError();
        return false;
    }

    m_width = width;
    m_height = height;
    m_initialized = true;
    return true;
}

void SDLVideoRenderer::resize(int width, int height) {
    if (!m_initialized) return;

    if (width <= 0 || height <= 0) {
        qWarning() << "Invalid size:" << width << "x" << height;
        return;
    }

    if (m_texture) {
        SDL_DestroyTexture(m_texture);
        m_texture = SDL_CreateTexture(m_renderer,
                                    SDL_PIXELFORMAT_IYUV,
                                    SDL_TEXTUREACCESS_STREAMING,
                                    width, height);
    }
    m_width = width;
    m_height = height;
}

void SDLVideoRenderer::render(const decoder::VideoFrame& frame) {
    if (!m_initialized) return;

    // 修夏SDL_UpdateYUVTexture调用
    SDL_UpdateYUVTexture(m_texture, nullptr,
                        frame.data[0], frame.linesize[0],
                        frame.data[1], frame.linesize[1],
                        frame.data[2], frame.linesize[2]);

    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);
    SDL_RenderPresent(m_renderer);
}

void SDLVideoRenderer::cleanup() {
    if (m_texture) SDL_DestroyTexture(m_texture);
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_sdlWindow) SDL_DestroyWindow(m_sdlWindow);

    m_texture = nullptr;
    m_renderer = nullptr;
    m_sdlWindow = nullptr;
    m_initialized = false;
}

bool SDLVideoRenderer::isInitialized() const {
    return m_initialized;
}

} // namespace renderer
} // namespace media
} // namespace modules
} // namespace aurorastream