/********************************************************************************
 * @file   : MediaPlayer.h
 * @brief  : 定义了 aurorastream::core::MediaPlayer 类。
 *
 * 该类是 AuroraStream 框架的核心媒体播放器主要接口，负责管理媒体播放流程，
 * 功能包括：播放、暂停、停止、快进、快退、音量控制、播放进度控制等。
 *
 * @author : polarours
 * @date   : 2025/08/21
 ********************************************************************************/

#ifndef AURORASTREAM_CORE_MEDIAPLAYER_H
#define AURORASTREAM_CORE_MEDIAPLAYER_H

#include <QUrl>
#include <QObject>
#include <QString>

#include "aurorastream/AuroraStream.h"

// --- 前向声明 FFmpeg 结构体 ---
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct SwsContext;
struct SwrContext;

namespace aurorastream {
namespace core {
/**
 * @class MediaPlayer
 * @brief AuroraStream 框架的核心媒体播放器类。
 *
 * MediaPlayer 负责管理媒体播放的整个流程。
 * 它提供了播放控制（播放、暂停、停止）、文件打开、位置跳转等功能。
 * 它通过 Qt 的信号与槽机制与其他部分（如用户界面）进行通信。
 *
 */
class AURORASTREAM_API MediaPlayer : public QObject
{
    Q_OBJECT
public:
    /**
     * @enum State
     * @brief 定义播放器可能的状态
     */
    enum State {
        Stopped = 0, ///< 停止状态
        Playing = 1, ///< 播放状态
        Paused = 2,  ///< 暂停状态
        Error = 3    ///< 错误状态
    };
    Q_ENUM(State) // 注册枚举到 Qt 元对象系统

    /**
     * @brief MediaPlayer 构造函数
     * @param parent QObject 父对象指针
     */
    explicit MediaPlayer(QObject *parent = nullptr);

    /**
     * @brief MediaPlayer 析构函数
     */
    ~MediaPlayer() override;

    /**
     * @brief 开始播放
     */
    Q_INVOKABLE void play();

    /**
     * @brief 暂停播放
     */
    Q_INVOKABLE void pause();

    /**
     * @brief 停止播放。
     */
    Q_INVOKABLE void stop();

    /**
 	 * @brief 加载媒体文件
 	 * @param fileName 媒体文件路径
 	 * @return 成功加载返回 true，否则返回 false
 	 */
    Q_INVOKABLE bool openFile(const QString& fileName);

    /**
     * @brief 跳转到指定位置
     * @param position 目标位置 (毫秒)
     */
    Q_INVOKABLE void seek(qint64 position);

    /**
     * @brief 获取当前状态
     * @return 当前状态
     */
    State getState() const;

    /**
     * @brief 检查是否正在播放
     * @return 是否正在播放
     */
    bool isPlaying() const;

    /**
     * @brief 获取当前媒体文件路径
     * @return 当前媒体文件路径
     */
    QString getCurrentMedia() const;

    /**
     * @brief 获取当前媒体文件总时长
     * @return 当前媒体文件总时长（毫秒）
     */
    qint64 getDuration() const;

    /**
 	 * @brief 获取当前播放位置
 	 * @return 当前播放位置（毫秒）
     */
    qint64 getPosition() const;

signals:
    /**
     * @brief 状态改变时发出。
     * @param state 新状态。
     */
    void stateChanged(aurorastream::core::MediaPlayer::State state);

    /**
     * @brief 位置更新时发出。
     * @param position 新位置 (毫秒)。
     */
    void positionChanged(qint64 position);

    /**
     * @brief 时长改变时发出。
     * @param duration 媒体总时长 (毫秒)。
     */
    void durationChanged(qint64 duration);

    /**
     * @brief 媒体文件成功打开时发出。
     * @param fileName 已打开的文件名。
     */
    void mediaOpened(const QString& fileName);

    /**
     * @brief 发生错误时发出。
     * @param message 错误信息。
     */
    void error(const QString& message);

private:
    State m_state;             ///< 当前播放状态
    qint64 m_duration;         ///< 媒体总时长 (毫秒)
    qint64 m_position;         ///< 当前播放位置 (毫秒)
	QString m_currentMedia;    ///< 当前媒体文件路径
	int m_videoStreamIndex;    ///< 视频流索引
    int m_audioStreamIndex;    ///< 音频流索引
    void* m_formatContext;     ///< FFmpeg 格式上下文指针
    void* m_videoCodecContext; ///< FFmpeg 视频解码器上下文指针
    void* m_audioCodecContext; ///< FFmpeg 音频解码器上下文指针
};

} // namespace core
} // namespace aurorastream

#endif // AURORASTREAM_CORE_MEDIAPLAYER_H