 /********************************************************************************
 * @file   : Renderer.h
 * @brief  : 声明 AuroraStream 媒体渲染器抽象基类。
 *
 * 此文件定义了 aurorastream::modules::media::renderer::Renderer 类，
 * 它是所有具体渲染器实现（例如，基于 SDL2 的渲染器）的抽象基类。
 * 它规定了渲染器模块必须提供的公共接口和行为。
 *
 * @Author : polarours
 * @Date   : 2025/08/22
 ********************************************************************************/

#ifndef AURORASTREAM_MODULES_MEDIA_RENDERER_RENDERER_H
#define AURORASTREAM_MODULES_MEDIA_RENDERER_RENDERER_H

#include <cstdint>

#include "aurorastream/AuroraStream.h"

namespace aurorastream {
namespace modules {
namespace media {

namespace decoder {
    struct VideoFrame;
    struct AudioFrame;
}

namespace renderer {

class AURORASTREAM_API Renderer : public QObject
{
    Q_OBJECT

public:
    enum class RendererType {
        VideoRenderer,
        AudioRenderer
    };

    Renderer();
    virtual ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    virtual bool initialize(int width, int height, void* windowHandle = nullptr) = 0;
    virtual void resize(int width, int height) = 0;
    virtual void renderVideo(const aurorastream::modules::meida::decoder::VideoFrame& frame) = 0;
    virtual void renderAudio(const aurorastream::modules::meida::decoder::AudioFrame& frame) = 0;
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

#endif // AURORASTREAM_MODULES_MEDIA_RENDERER_RENDERER_H