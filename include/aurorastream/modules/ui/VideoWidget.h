/********************************************************************************
 * @file   : VideoWidget.h
 * @brief  : 声明 AuroraStream 视频显示部件类。
 *
 * VideoWidget 是一个用于显示解码后视频帧的自定义 QWidget。
 * 它通常与渲染器 (Renderer) 模块协同工作。
 *
 * @Author : polarours
 * @Date   : 2025/8/22
 ********************************************************************************/

// include/aurorastream/modules/ui/VideoWidget.h
#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget> // Qt 基础窗口部件类
#include <memory> // 智能指针支持

/**
 * @namespace aurorastream
 * @brief AuroraStream 应用程序命名空间。
 */
namespace aurorastream {

/**
 * @namespace aurorastream::modules
 * @brief AuroraStream 应用程序模块命名空间。
 */ 
namespace core {
    class MediaPlayer;
}

/**
 * @namespace aurorastream::modules::media
 * @brief AuroraStream 应用程序媒体模块命名空间。
 */
namespace modules {

/**
 * @namespace aurorastream::modules::media
 * @brief AuroraStream 应用程序媒体模块命名空间。
 */
namespace media {

/**
 * @namespace aurorastream::modules::media::renderer
 * @brief AuroraStream 应用程序媒体渲染器模块命名空间。
 */  
namespace renderer {
    class VideoRenderer;
}
}

/**
 * @namespace aurorastream::modules::ui
 * @brief AuroraStream 应用程序 UI 模块命名空间。
 */  
namespace ui {

/**
* @class VideoWidget
* @brief 用于显示视频内容的自定义 QWidget。
*
* VideoWidget 提供一个视频显示区域。它本身不直接解码或渲染，
* 而是作为一个画布，接收来自渲染器的数据进行显示。
*/
class AURORASTREAM_API VideoWidget : public QWidget {
    Q_OBJECT

public:
    /**
     *@brief VideoWidget 构造函数。
     *@param parent 父部件指针。
     */
    explicit VideoWidget(QWidget *parent = nullptr);

    /**
     * @brief VideoWidget 析构函数。
     */
    ~VideoWidget() override;

    /**
     * @brief 设置关联的媒体播放器。
     * @param player 指向 MediaPlayer 的指针。
     */
    void setMediaPlayer(aurorastream::core::MediaPlayer* player);

    /**
     * @brief 设置关联的视频渲染器。
     * @param renderer 指向 Renderer 的指针。
     */
    void setRenderer(aurorastream::modules::media::renderer::Renderer* renderer);

protected:
    /**
     * @brief 重写 QWidget 的 paintEvent 方法，用于绘制视频内容。
     * @param event 绘制事件。
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief 重写 QWidget 的 resizeEvent 方法，处理窗口大小变化事件。
     * @param event 大小变化事件。
     */
    void resizeEvent(QResizeEvent *event) override;

private:
    aurorastream::core::MediaPlayer*                    m_mediaPlayer; ///< 关联的媒体播放器
    aurorastream::modules::media::renderer::Renderer*   m_renderer;    ///< 关联的视频渲染器
};

} // namespace ui
} // namespace modules
} // namespace aurorastream

#endif // VIDEOWIDGET_H