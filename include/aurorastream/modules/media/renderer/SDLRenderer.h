 /********************************************************************************
 * @file   : SDLRenderer.h
 * @brief  : 声明基于 SDL2 的具体媒体渲染器类。
 *
 * 此文件定义了 aurorastream::modules::media::renderer::SDLRenderer 类，
 * 它是 Renderer 抽象基类的具体实现，使用 SDL2 库进行视频渲染。
 *
 * @author : polarours
 * @date   : 2025/08/23
 ********************************************************************************/

#ifndef AURORASTREAM_MODULE_MEDIA_RENDERER_SDLRENDERER_H
#define AURORASTREAM_MODULE_MEDIA_RENDERER_SDLRENDERER_H

#include <cstdint>
#include "aurorastream/AuroraStream.h"
#include "aurorastream/modules/media/renderer/Renderer.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace aurorastream {
namespace modules {
namespace media {
namespace decoder {
struct VideoFrame;
}
namespace renderer {
/**
 * @brief SDL2 媒体渲染器
 */
class AURORASTREAM_API SDLRenderer : public Renderer
{
    Q_OBJECT
public:

    explicit SDLRenderer(QObject *parent = nullptr);

    ~SDLRenderer() override;

    SDLRenderer(const SDLRenderer&) = delete;
    SDLRenderer& operator=(const SDLRenderer&) = delete;
    SDLRenderer(SDLRenderer&&) = delete;
    SDLRenderer& operator=(SDLRenderer&&) = delete;

    bool initialize(int width, int height, void* windowHandle = nullptr) override;
    void resize(int width, int height) override;
    void renderVideo(const aurorastream::modules::media::decoder::VideoFrame& frame) override;
    void cleanup() override;
    bool isInitialized() const override;

private:
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    SDL_Texture* m_texture;

    bool createOrUpdateTexture(const aurorastream::modules::media::decoder::VideoFrame& frame);
    bool updateTexture(const aurorastream::modules::media::decoder::VideoFrame& frame);
    void presentTexture();

};

} // namespace renderer
} // namespace media
} // namespace modules
} // namespace aurorastream

#endif // AURORASTREAM_MODULE_MEDIA_RENDERER_SDLRENDERER_H