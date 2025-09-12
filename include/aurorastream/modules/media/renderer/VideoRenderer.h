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
#include <QObject>

#include "aurorastream/AuroraStream.h"
#include "../decoder/Decoder.h"

namespace aurorastream {
namespace modules {
namespace media {
namespace decoder {
struct VideoFrame;
}
namespace renderer {

class AURORASTREAM_API VideoRenderer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool initialized READ isInitialized NOTIFY initializedChanged)

public:
    explicit VideoRenderer(QObject* parent = nullptr);
    ~VideoRenderer() override;

    // 禁用拷贝和移动
    VideoRenderer(const VideoRenderer&) = delete;
    VideoRenderer& operator=(const VideoRenderer&) = delete;
    VideoRenderer(VideoRenderer&&) = delete;
    VideoRenderer& operator=(VideoRenderer&&) = delete;

    // 核心接口
    /**
     * @brief 初始化视频渲染器
     * @param width 初始宽度
     * @param height 初始高度
     * @param windowHandle 平台相关窗口句柄
     * @return 初始化成功返回true
     */
    virtual bool initialize(int width, int height, void* windowHandle = nullptr) = 0;

    /**
     * @brief 调整渲染尺寸
     * @param width 新宽度
     * @param height 新高度
     */
    virtual void resize(int width, int height) = 0;

    /**
     * @brief 渲染视频帧
     * @param frame YUV420P格式的视频帧
     */
    virtual void render(const aurorastream::modules::media::decoder::VideoFrame& frame) = 0;

    /// 释放所有渲染资源
    virtual void cleanup() = 0;

    /// 检查渲染器是否已初始化
    virtual bool isInitialized() const = 0;

    /// 支持的像素格式枚举
    enum PixelFormat {
        PIXELFORMAT_IYUV = 0x56555949, // SDL_PIXELFORMAT_IYUV
        PIXELFORMAT_YV12 = 0x32315659  // SDL_PIXELFORMAT_YV12
    };
    Q_ENUM(PixelFormat)

    // 硬件加速支持
    enum class HardwareAccel {
        None,
        DXVA2,
        CUDA,
        VAAPI
    };
    Q_ENUM(HardwareAccel)

    virtual void setHardwareAccel(HardwareAccel accel);

signals:
    void initializedChanged(bool initialized);
    void errorOccurred(const QString& error);

protected:
    int m_width {0};
    int m_height {0};
    bool m_initialized {false};
    void* m_windowHandle {nullptr};
    HardwareAccel m_hwAccel {HardwareAccel::None};
};

} // namespace renderer
} // namespace media
} // namespace modules
} // namespace aurorastream


#endif // AURORASTREAM_MODULES_MEDIA_RENDERER_VIDEORENDERER_H