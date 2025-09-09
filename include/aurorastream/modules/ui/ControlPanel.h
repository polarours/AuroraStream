 /********************************************************************************
 * @file   : ControlPanel.h
 * @brief  : 控制面板组件
 *
 * 此文件定义了 aurorastream::modules::ui::ControlPanel 类，它是 AuroraStream 框架用于
 * 媒体播放的 UI 控件，提供播放/暂停、停止、进度条、音量控制等交互功能。
 *
 * @Author : polarours
 * @Date   : 2025/08/21
 ********************************************************************************/

#ifndef AURORASTREAM_MODULES_UI_CONTROLPANEL_H
#define AURORASTREAM_MODULES_UI_CONTROLPANEL_H

#include <QWidget>

// --- 前向声明 ---
QT_BEGIN_NAMESPACE
class QPushButton;
class QSlider;
class QLabel;
QT_END_NAMESPACE

namespace aurorastream {
namespace core {
    class MediaPlayer;
    enum class MediaPlayerState;
}

namespace modules {
namespace ui {
/**
 * @class ControlPanel
 * @brief 媒体控制面板类。
 * @details ControlPanel 提供播放控制按钮和进度条，用于与 MediaPlayer 交互。
 */
class AURORASTREAM_API ControlPanel : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief ControlPanel 构造函数。
     * @param parent 父 QWidget 指针。
     */
    explicit ControlPanel(QWidget *parent = nullptr);

    /**
     * @brief ControlPanel 析构函数。
     */
    ~ControlPanel() override;

    /**
     * @brief 设置关联的媒体播放器。
     * @param player 指向 MediaPlayer 对象的指针。
     */
    void setMediaPlayer(aurorastream::core::MediaPlayer* player);

private slots:
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
     * @brief 打开按钮点击槽函数。
     */
    void onOpenClicked();

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
     * @brief 媒体状态改变槽函数。
     * @param state 当前媒体状态。
     */
    void onMediaStateChanged(aurorastream::core::MediaPlayer::State state);

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

    /**
     * @brief 媒体错误槽函数。
     * @param message 错误信息。
     */
    void onError(const QString& message);

private:
    void setupUI();
    void updateButtons();
    QString formatTime(qint64 milliseconds);

    QPushButton* m_openButton;                      ///< 打开按钮
    QPushButton* m_playButton;                      ///< 播放按钮
    QPushButton* m_pauseButton;                     ///< 暂停按钮
    QPushButton* m_stopButton;                      ///< 停止按钮
    QSlider*     m_seekSlider;                      ///< 进度条
    QSlider*     m_volumeSlider;                    ///< 音量条
    QLabel*      m_timeLabel;                       ///< 时间标签
    QLabel*      m_durationLabel;                   ///< 时长标签

    qint64       m_duration;                        ///< 当前媒体总时长

    aurorastream::core::MediaPlayer* m_mediaPlayer; ///< 关联的媒体播放器
};

} // namespace ui
} // namespace modules
} // namespace aurorastream

#endif // AURORASTREAM_MODULES_UI_CONTROLPANEL_H