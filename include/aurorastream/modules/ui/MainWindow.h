/********************************************************************************
 * @file   : MainWindow.h
 * @brief  : 声明 AuroraStream 主窗口类。
 *
 * MainWindow 是 AuroraStream 应用程序的主用户界面窗口。
 * 它通常包含视频显示区域 (VideoWidget)、控制面板 (ControlPanel)，
 * 并处理应用程序级别的菜单、工具栏和状态栏。
 *
 * @Author : polarours
 * @Date   : 2025/08/21
 ********************************************************************************/

// include/aurorastream/modules/ui/MainWindow.h
#ifndef AURORASTREAM_MODULES_UI_MAINWINDOW_H
#define AURORASTREAM_MODULES_UI_MAINWINDOW_H

#include <QMainWindow> // 继承自 QMainWindow
#include <memory> // 智能指针

// 前置声明
QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

/**
 * @namespace aurorastream
 * @brief AuroraStream 应用程序命名空间。
 */
namespace aurorastream {

/**
 * @namespace aurorastream::core
 * @brief AuroraStream 核心功能命名空间。
 */
namespace core {
    class MediaPlayer; // 媒体播放器核心类
}

/**
 * @namespace aurorastream::modules
 * @brief AuroraStream 模块命名空间。
 */
namespace modules {

/**
 * @namespace aurorastream::modules::ui
 * @brief AuroraStream UI 模块命名空间。
 */
namespace ui {

class VideoWidget; // 视频显示部件
class ControlPanel; // 控制面板部件

/**
* @class MainWindow
* @brief AuroraStream 应用程序的主窗口类。
*
* 继承自 QMainWindow，是应用程序 GUI 的核心容器。
*/
class AURORASTREAM_API MainWindow : public QMainWindow {
    Q_OBJECT // Qt 宏，启用信号/槽机制

public:
    /**
     * @brief MainWindow 构造函数。
     * @param parent 父 QObject 指针。
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /**
     * @brief MainWindow 析构函数。
     */
    ~MainWindow() override;

    /**
     * @brief 设置关联的核心媒体播放器。
     * @param player 指向 MediaPlayer 对象的指针。
     */
    void setMediaPlayer(aurorastream::core::MediaPlayer* player);

private slots:
    /**
     * @brief 处理打开文件动作。
     */
    void onOpenFile();

    /**
     * @brief 处理媒体打开事件。
     * @param filePath 媒体文件路径。
     */
    void onMediaOpened(const QString& filePath);

    /**
     * @brief 处理错误事件。
     * @param message 错误信息。
     */
    void onError(const QString& message);

protected:
    /**
     * @brief 重写dragEnterEvent事件处理函数。
     * @param event 事件对象。
     */
    void dragEnterEvent(QDragEnterEvent *event) override;

    /**
     * @brief 重写dropEvent事件处理函数。
     * @param event 事件对象。
     */
    void dropEvent(QDropEvent *event) override;

private:
    /**
     * @brief 初始化窗口 UI。
     */
    void setupUI();

    /**
     * @brief 初始化窗口菜单。
     */
    void setupMenus();

    /**
     * @brief 初始化窗口工具栏。
     */
    void setupStatusBar();

    /**
     * @brief 更新窗口标题。
     */
    void updateWindowTitle();

    aurorastream::core::MediaPlayer* m_mediaPlayer; ///< 核心媒体播放器指针
    VideoWidget*       m_videoWidget;               ///< 视频显示区域
    ControlPanel*      m_controlPanel;              ///< 控制面板
    QString            m_currentFile;               ///< 当前打开的媒体文件路径

};

} // namespace ui
} // namespace modules
} // namespace aurorastream

#endef // AURORASTREAM_MODULES_UI_MAINWINDOW_H