/********************************************************************************
 * @file   : MainWindow.h
 * @brief  : 定义了 aurorastream::modules::ui::MainWindow 主窗口类。
 *
 * MainWindow 是 AuroraStream 应用程序的主用户界面窗口。
 * 它通常包含视频显示区域 (VideoWidget)、控制面板 (ControlPanel)，
 * 并处理应用程序级别的菜单、工具栏和状态栏。
 *
 * @author : polarours
 * @date   : 2025/08/21
 ********************************************************************************/

#ifndef AURORASTREAM_MODULES_UI_MAINWINDOW_H
#define AURORASTREAM_MODULES_UI_MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <memory>

// --- 前置声明 ---
QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class QSlider;
class QLabel;
QT_END_NAMESPACE

namespace aurorastream {
namespace core {
class MediaPlayer; // 媒体播放器核心类
enum class MediaPlayerState;
}
namespace modules {
namespace media {
namespace renderer {
class VideoRenderer;
}
}
namespace ui {
/**
* @class MainWindow
* @brief AuroraStream 应用程序的主窗口类。
* 继承自 QMainWindow，是应用程序 GUI 的核心容器。
*/
class AURORASTREAM_API MainWindow : public QMainWindow {
    Q_OBJECT
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
     * @brief 播放按钮点击槽函数。
     */
    void onPlayClicked();

    /**
     * @brief 暂停按钮点击槽函数。
     */
    void onPauseClicked();

    /**
     * @brief 停止按钮点击槽函数。
     */
    void onStopClicked();

    /**
     * @brief 进度条拖动槽函数。
     * @param value 进度条当前值。
     */
    void onSeekSliderMoved(int value);

    /**
     * @brief 音量条拖动槽函数。
     * @param value 音量条当前值。
     */
    void onVolumeChanged(int value);

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

    /**
     * @brief 媒体状态改变槽函数。
     * @param state 当前媒体状态。
     */
    void onMediaStateChanged(aurorastream::core::MediaPlayerState state);

    /**
     * @brief 媒体总时长改变槽函数。
     * @param duration 当前媒体总时长。
     */
    void onDurationChanged(qint64 duration);

    /**
     * @brief 媒体位置改变槽函数。
     * @param position 当前媒体位置。
     */
    void onPositionChanged(qint64 position);

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

    /**
     * @brief 更新控制按钮状态。
     */
    void updateButtons();

    /**
     * @brief 格式化时间显示。
     * @param milliseconds 毫秒数。
     * @return 格式化后的时间字符串。
     */
    QString formatTime(qint64 milliseconds);

    aurorastream::core::MediaPlayer* m_mediaPlayer; ///< 核心媒体播放器指针
    QString            m_currentFile;               ///< 当前打开的媒体文件路径

    // 控制面板组件
    QPushButton*       m_openButton;                ///< 打开按钮
    QPushButton*       m_playButton;                ///< 播放按钮
    QPushButton*       m_pauseButton;               ///< 暂停按钮
    QPushButton*       m_stopButton;                ///< 停止按钮
    QSlider*           m_seekSlider;                ///< 进度条
    QSlider*           m_volumeSlider;              ///< 音量条
    QLabel*           m_timeLabel;                 ///< 时间标签
    QLabel*           m_durationLabel;             ///< 时长标签

    qint64            m_duration;                  ///< 当前媒体总时长
    QWidget*          m_videoContainer;            ///< 视频显示容器

};

} // namespace ui
} // namespace modules
} // namespace aurorastream

#endef // AURORASTREAM_MODULES_UI_MAINWINDOW_H