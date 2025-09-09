 /********************************************************************************
 * @file   : Player.h
 * @brief  : 声明 AuroraStream 媒体播放器核心逻辑类。
 *
 * 此文件定义了 aurorastream::modules::media::player::Player 类。
 * Player 类负责管理整个媒体播放流程，包括状态控制、解码协调、
 * 渲染调度以及与用户界面的交互。
 *
 * @author : polarours
 * @date   : 2025/08/22
 ********************************************************************************/

#ifndef AURORASTREAM_MODULES_MEDIA_PLAYER_PLAYER_H
#define AURORASTREAM_MODULES_MEDIA_PLAYER_PLAYER_H

#include <QObject>
#include <QString>
#include <memory>

#include "aurorastream/AuroraStream.h"

namespace aurorastream {
namespace modules {
namespace media {
namespace decoder {
class Decoder;
}
namespace render {
class Renderer;
}
namespace player {
/**
 * @brief 媒体播放器核心类
 * 本类负责协调解码器、渲染器和播放状态，提供完整的媒体播放功能。
 */
class AURORASTREAM_API Player : public QObject
{
    Q_OBJECT
public:
    // --- 状态枚举 ---
    enum class State {
        Stopped = 0,    ///< 完全停止，无媒体加载
        Opening = 1,    ///< 正在打开媒体文件
        Opened = 2,     ///< 媒体已打开，但未开始播放
        Starting = 3,   ///< 正在启动播放 (解码线程启动中)
        Playing = 4,    ///< 正在播放 (解码和渲染进行中)
        Pausing = 5,    ///< 正在暂停播放
        Paused = 6,     ///< 已暂停播放
        Stopping = 7,   ///< 正在停止播放 (解码线程停止中)
        Seeking = 8,    ///< 正在执行跳转操作
        Closing = 9,    ///< 正在关闭媒体文件
        Error = 10      ///< 发生错误
    };

    Q_ENUM(State) // 注册枚举到元对象系统

	/**
     * @brief 构造函数
     * @param parent QObject父对象指针
     */
    explicit Player(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     * 负责释放所有资源并确保安全关闭播放器。
     */
	~Player() override;

	// --- 禁用拷贝和移动语义 ---
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;
    Player(Player&&) = delete;
    Player& operator=(Player&&) = delete;

	/**
     * @brief 设置解码器实现
     * @param decoder 解码器实现指针，Player 将接管其所有权
     * @note 调用此方法会清空当前解码器实例
     */
    void setDecoder(std::unique_ptr<aurorastream::modules::media::decoder::Decoder> decoder);

	/**
     * @brief 设置渲染器实现
     * @param renderer 渲染器实现指针，Player 将接管其所有权
     * @note 调用此方法会清空当前渲染器实例
     */
    void setRenderer(std::unique_ptr<aurorastream::modules::media::render::Renderer> renderer);

	/**
     * @brief 打开媒体资源
     * @param uri 媒体资源路径，可以是本地文件路径或网络URL
     * @return 成功返回true，失败返回false并设置错误状态
     */
    Q_INVOKABLE bool open(const QString& uri);

 	/**
     * @brief 开始播放
     * @note 必须在Opened状态下调用
     */
    Q_INVOKABLE void play();

    /**
     * @brief 暂停播放
     * @note 必须在Playing状态下调用
     */
 	Q_INVOKABLE void pause();

    /**
     * @brief 停止播放
     * @note 可以在任何状态下调用
     */
	Q_INVOKABLE void stop();

	/**
     * @brief 跳转到指定位置 (毫秒)
     * @param position 目标位置 (毫秒)
     * @note 必须在Opened/Paused/Playing状态下调用
     */
	Q_INVOKABLE void seek(qint64 position);

    /**
     * @brief 关闭当前媒体资源
     * @note 可以在任何状态下调用
     */
	Q_INVOKABLE void close();

    /**
     * @brief 获取当前播放状态
     * @return 当前播放状态
     */
	State getState() const;

    /**
     * @brief 获取当前播放媒体URI
     * @return 当前媒体URI
     */
	QString getCurrentUri() const;

    /**
     * @brief 获取当前播放时长 (毫秒)
     * @return 媒体总时长 (毫秒)
     */
	qint64 getDuration() const;

    /**
     * @brief 获取当前播放位置 (毫秒)
     * @return 当前播放位置 (毫秒)
     */
	qint64 getPosition() const;

	/**
     * @brief 获取当前视频宽度
     * @return 视频宽度
     */
	int getVideoWidth() const;

    /**
     * @brief 获取当前视频高度
     * @return 视频高度
     */
    int getVideoHeight() const;

    /**
     * @brief 获取当前音频采样率
     * @return 音频采样率
     */
	int getAudioSampleRate() const;

    /**
     * @brief 获取当前音频通道数
     * @return 音频通道数
     */
	int getAudioChannels() const;

	/**
     * @brief 获取当前视频帧率
     * @return 视频帧率
     */
    double getVideoFrameRate() const;

signals:
    /**
     * @brief 播放状态改变信号
     * @param newState 新的播放状态
     */
    void stateChanged(State newState);

    /**
     * @brief 播放位置改变信号
     * @param newPosition 新的播放位置 (毫秒)
     */
    void positionChanged(qint64 newPosition);

    /**
     * @brief 播放时长改变信号
     * @param newDuration 新的播放时长 (毫秒)
     */
	void durationChanged(qint64 newDuration);

    /**
     * @brief 媒体打开完成信号
     * @param uri 媒体URI
     */
	void mediaOpened(const QString& uri);

    /**
     * @brief 发生错误信号
     * @param errorMessage 错误描述
     */
    void errorOccurred(const QString& errorMessage);

    /**
     * @brief 播放完成信号
     * @note 仅在正常播放结束时触发
     */
	void finished();

    /**
     * @brief 视频帧就绪信号
     * @param frame 视频帧数据
     */
    void videoFrameReady(const aurorastream::modules::media::decoder::VideoFrame& frame);

    /**
     * @brief 音频帧就绪信号
     * @param frame 音频帧数据
     */
    void audioFrameReady(const aurorastream::modules::media::decoder::AudioFrame& frame);

private slots:
    /**
     * @brief 响应解码器音频帧就绪信号
     * @param frame 音频帧数据
     */
    void onVideoFrameReady(const aurorastream::modules::media::decoder::VideoFrame& frame);

    /**
     * @brief 响应解码器音频帧就绪信号
     * @param frame 音频帧数据
     */
	void onAudioFrameReady(const aurorastream::modules::media::decoder::AudioFrame& frame);

    /**
     * @brief 响应总时长变化信号
     * @param duration 新的总时长 (毫秒)
     */
	void onDurationChanged(qint64 duration);

    /**
     * @brief 响应播放位置变化信号
     * @param position 新的播放位置 (毫秒)
     */
    void onPositionChanged(qint64 position);

    /**
     * @brief 响应错误信号
     * @param errorMessage 错误描述
     */
	void onErrorOccurred(const QString& errorMessage);

    /**
     * @brief 响应媒体打开完成信号
     * @param uri 媒体URI
     */
	void onMediaOpened(const QString& uri);

    /**
     * @brief 响应播放完成信号
     */
	void onMediaFinished();

private:
	// --- 成员变量 ---
    State m_state;	 	                 										///< 当前状态
    qint64 m_duration;			                                                ///< 总时长 (毫秒)
    qint64 m_position;	    													///< 当前位置 (毫秒)
	QString m_currentUri;                                                       ///< 当前媒体URI
	std::unique_ptr<aurorastream::modules::media::decoder::Decoder> m_decoder;  ///< 解码器
    std::unique_ptr<aurorastream::modules::media::render::Renderer> m_renderer; ///< 渲染器

	// --- 私有方法 ---
    /**
     * @brief 更新播放状态
     * @param newState 新的播放状态
     */
    void setState(State newState);

    /**
     * @brief 更新播放位置
     * @param newPosition 新的播放位置
     */
	void updatePosition(qint64 newPosition);

    /**
     * @brief 更新总时长
     * @param newDuration 新的总时长
     */
	void updateDuration(qint64 newDuration);

    /**
     * @brief 打开媒体文件
     * @param uri 媒体URI
     * @return 成功返回true，失败返回false
     */
	bool doOpen(const QString& uri);

    /**
     * @brief 播放媒体
     */
	bool doPlay();

    /**
     * @brief 暂停播放
     */
    bool doPause();

    /**
     * @brief 停止播放
     */
	bool doStop();

    /**
     * @brief 关闭媒体
     */
    bool doClose();

    /**
     * @brief 定位到指定位置
     * @param position 新的播放位置 (毫秒)
     */
	bool doSeek(qint64 position)

    /**
     * @brief 处理视频帧
     * @param frame 视频帧数据
     */
    void handleVideoFrame(const aurorastream::modules::media::decoder::VideoFrame& frame);

    /**
     * @brief 处理音频帧
     * @param frame 音频帧数据
     */
	void handleAudioFrame(const aurorastream::modules::media::decoder::AudioFrame& frame);
};

} // namespace player
} // namespace media
} // namespace modules
} // namespace aurorastream

#endif // AURORASTREAM_MODULES_MEDIA_PLAYER_PLAYER_H