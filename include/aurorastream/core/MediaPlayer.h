/********************************************************************************
 * @file   : MediaPlayer.h
 * @brief  : 媒体播放器类定义
 *
 * 此文件定义了 aurorastream::core::MediaPlayer 类，它是 AuroraStream 框架用于
 * 控制媒体播放流程的主要接口。它提供了播放控制（播放、暂停、停止、Seek）、
 * 状态查询（播放状态、时长、位置）以及文件打开等核心功能。
 *
 * @author : polarours
 * @date   : 2025/08/21
 ********************************************************************************/

#ifndef AURORASTREAM_CORE_MEDIAPLAYER_H
#define AURORASTREAM_CORE_MEDIAPLAYER_H

#include <QObject>
#include <QString>
#include <QUrl>

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
 */
class AURORASTREAM_API MediaPlayer : public QObject // 使用 AURORASTREAM_API 宏
{
    Q_OBJECT // 启用 Qt 的元对象系统

public:
    /**
     * @enum State
     * @brief 定义播放器可能的状态。
     */
    enum State {
        Stopped = 0, ///< 停止状态
        Playing = 1, ///< 播放状态
        Paused = 2,  ///< 暂停状态
        Error = 3    ///< 错误状态
    };
    Q_ENUM(State) // 注册枚举到 Qt 元对象系统

    /**
     * @brief MediaPlayer 构造函数。
     * @param parent QObject 父对象指针。
     */
    explicit MediaPlayer(QObject *parent = nullptr);

    /**
     * @brief MediaPlayer 析构函数。
     */
    ~MediaPlayer() override;

    // --- 播放控制方法 ---

    /**
     * @brief 开始播放。
     */
    Q_INVOKABLE void play();

    /**
     * @brief 暂停播放。
     */
    Q_INVOKABLE void pause();

    /**
     * @brief 停止播放。
     */
    Q_INVOKABLE void stop();

    /**
     * @brief 打开一个媒体文件。
     * @param fileName 文件路径。
     * @return 是否成功打开。
     */
    Q_INVOKABLE bool openFile(const QString& fileName);

    /**
     * @brief 跳转到指定位置。
     * @param position 目标位置 (毫秒)。
     */
    Q_INVOKABLE void seek(qint64 position);

    // --- 状态查询方法 ---

    /**
     * @brief 获取当前状态。
     * @return 当前 State。
     */
    State getState() const;

    /**
     * @brief 检查是否正在播放。
     * @return 如果正在播放返回 true。
     */
    bool isPlaying() const;

    /**
     * @brief 获取当前媒体文件路径。
     * @return 文件路径。
     */
    QString getCurrentMedia() const;

    /**
     * @brief 获取媒体总时长。
     * @return 总时长 (毫秒)。
     */
    qint64 getDuration() const;

    /**
     * @brief 获取当前播放位置。
     * @return 当前位置 (毫秒)。
     */
    qint64 getPosition() const;

signals:
    // --- Qt 信号：用于通知状态和事件变化 ---

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
	// --- 私有成员变量 ---
    State m_state;             ///< 当前播放状态
    QString m_currentMedia;    ///< 当前媒体文件路径
    qint64 m_duration;         ///< 媒体总时长 (毫秒)
    qint64 m_position;         ///< 当前播放位置 (毫秒)

	// --- FFmpeg 相关成员 (使用 void* 隐藏具体类型) ---
    void* m_formatContext;     ///< FFmpeg 格式上下文指针 (实际类型是 AVFormatContext*)
    void* m_videoCodecContext; ///< FFmpeg 视频解码器上下文指针
    void* m_audioCodecContext; ///< FFmpeg 音频解码器上下文指针
    int m_videoStreamIndex;    ///< 视频流索引
    int m_audioStreamIndex;    ///< 音频流索引
};

} // namespace core
} // namespace aurorastream

#endif // AURORASTREAM_CORE_MEDIAPLAYER_H