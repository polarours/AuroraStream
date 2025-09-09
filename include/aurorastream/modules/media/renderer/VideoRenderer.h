/********************************************************************************
 * @file   : VideoRenderer.h
 * @brief  : 声明 AuroraStream 视频渲染器抽象基类。
 *
 * 此文件定义了 aurorastream::modules::media::renderer::VideoRenderer 类，
 * 它是所有具体视频渲染器实现（例如，基于 SDL2 的视频渲染器）的抽象基类。
 * 它规定了视频渲染器模块必须提供的公共接口和行为。
 *
 * @author : polarours
 * @date   : 2025/8/23
 ********************************************************************************/

#ifndef AURORASTREAM_MODULES_MEDIA_RENDERER_VIDEORENDERER_H
#define AURORASTREAM_MODULES_MEDIA_RENDERER_VIDEORENDERER_H

#include <cstdint>
#include "aurorastream/AuroraStream.h"

namespace aurorastream {
namespace modules {
namespace media {
namespace decoder {
struct VideoFrame;
}
namespace renderer {

class AURORASTREAM_API VideoRenderer
{
public:

    explicit VideoRenderer(QObject* parent = nullptr);

    ~VideoRenderer() override;

    VideoRenderer(const VideoRenderer&) = delete;
    VideoRenderer& operator=(const VideoRenderer&) = delete;
    VideoRenderer(VideoRenderer&&) = delete;
    VideoRenderer& operator=(VideoRenderer&&) = delete;

    virtual bool initialize(int width, int height, void* windowHandle = nullptr) = 0;
    virtual void resize(int width, int height) = 0;
    virtual void render(const aurorastream::modules::media::decoder::VideoFrame& frame) = 0;
    virtual void cleanup() = 0;
    virtual bool isInitialized() const = 0;

protected:
    int m_width;
    int m_height;
    bool m_initialized;
    void* m_windowHandle;
};

} // namespace renderer
} // namespace media
} // namespace modules
} // namespace aurorastream


#endif // AURORASTREAM_MODULES_MEDIA_RENDERER_VIDEORENDERER_H
