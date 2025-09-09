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
/**
 * @brief 媒体渲染器抽象基类。
 */
class AURORASTREAM_API Renderer : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数。
     */
    explicit Renderer(QObject* parent = nullptr);

    /**
     * @brief 析构函数。
     */
    ~Renderer() override;

    // ---- 禁止拷贝和移动 ----
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    /**
     * @brief 初始化渲染器。
     * @param width 渲染区域的宽度（像素）。
     * @param height 渲染区域的高度（像素）。
     * @param windowHandle 可选参数，用于指定渲染的目标窗口句柄（例如 SDL_Window*）。
     * @return 初始化成功返回 true，否则返回 false。
     */
    virtual bool initialize(int width, int height, void* windowHandle = nullptr) = 0;

    /**
     * @brief 调整渲染区域大小。
     * @param width 新的宽度（像素）。
     * @param height 新的高度（像素）。
     */
    virtual void resize(int width, int height) = 0;

    /**
     * @brief 渲染视频帧。
     * @param frame 要渲染的视频帧数据。
     */
    virtual void renderVideo(const aurorastream::modules::meida::decoder::VideoFrame& frame) = 0;

    /**
     * @brief 渲染音频帧。
     * @param frame 要渲染的音频帧数据。
     */
    virtual void renderAudio(const aurorastream::modules::meida::decoder::AudioFrame& frame) = 0;

    /**
     * @brief 清理渲染器资源。
     */
    virtual void cleanup() = 0;

    /**
     * @brief 检查渲染器是否已初始化。
     * @return 已初始化返回 true，否则返回 false。
     */
    virtual bool isInitialized() const = 0;

protected:
    int m_width;          ///< 当前渲染区域的宽度（像素）。
    int m_height;         ///< 当前渲染区域的高度（像素）。
    bool m_initialized;   ///< 标记渲染器是否已初始化。
    void* m_windowHandle; ///< 用于渲染的目标窗口句柄（由具体实现使用）。
};

} // namespace renderer
} // namespace media
} // namespace modules
} // namespace aurorastream

#endif // AURORASTREAM_MODULES_MEDIA_RENDERER_RENDERER_H