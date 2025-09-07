/********************************************************************************
 * @file   : SDLVideoRenderer.h
 * @brief  : 声明基于 SDL2 的具体视频渲染器类。
 *
 * 此文件定义了 aurorastream::modules::media::renderer::SDLVideoRenderer 类，
 * 它是 VideoRenderer 抽象基类的具体实现，使用 SDL2 库进行视频渲染。
 *
 * @author : polarours
 * @date   : 2025/8/23
 ********************************************************************************/

#ifndef AURORASTREAM_MODULES_MEDIA_VIDEO_SDLVIDEORENDERER_H
#define AURORASTREAM_MODULES_MEDIA_VIDEO_SDLVIDEORENDERER_H

#include <cstdint>
#include "aurorastream/AuroraStream.h"
#include "aurorastream/modules/media/video/VideoRenderer.h"

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

class AURORASTREAM_API SDLVideoRenderer : public VideoRenderer
{

public:
    SDLVideoRenderer();
    ~SDLVideoRenderer();

    SDLVideoRenderer(const SDLVideoRenderer&) = delete;
    SDLVideoRenderer& operator=(const SDLVideoRenderer&) = delete;
    SDLVideoRenderer(SDLVideoRenderer&&) = delete;
    SDLVideoRenderer& operator=(SDLVideoRenderer&&) = delete;

    bool initialize(void* hwnd) override;
    void resize(int width, int height) override;
    bool render(const aurorastream::modules::media::decoder::VideoFrame* frame) override;
    void cleanup() override;
    bool isInitialized() override;

private:
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    SDL_Texture* m_texture;

    bool createOrUpdateTexture(const aurorastream::modules::media::decoder::VideoFrame* frame);
    bool updateTexture(const aurorastream::modules::media::decoder::VideoFrame* frame);
    void presentTexture();

};

}
}
}
}


#endif // AURORASTREAM_MODULES_MEDIA_VIDEO_SDLVIDEORENDERER_H
