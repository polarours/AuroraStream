 /********************************************************************************
 * @file Player.h
 * @brief 声明 AuroraStream 媒体播放器核心逻辑类。
 *
 * 此文件定义了 aurorastream::modules::media::player::Player 类。
 * Player 类负责管理整个媒体播放流程，包括状态控制、解码协调、
 * 渲染调度以及与用户界面的交互。
 *
 * @author : polarours
 * @date   : 2025/08/22
 ********************************************************************************/

#ifdef AURORASTREAM_MODULES_MEDIA_PLAYER_H
#define AURORASTREAM_MODULES_MEDIA_PLAYER_H

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

class AURORASTREAM_API Player : public QObject
{
    Q_OBJECT

public:
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
    Q_ENUM(State)

    explicit Player(QObject *parent = nullptr);
    ~Player() override;

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;
    Player(Player&&) = delete;
    Player& operator=(Player&&) = delete;

    void setDecoder(std::unique_ptr<aurorastream::modules::media::decoder::Decoder> decoder);

    void setRenderer(std::unique_ptr<aurorastream::modules::media::render::Renderer> renderer);

    Q_INVOKABLE bool open(const QString& uri);

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void seek(qint64 position);
    Q_INVOKABLE void close();
    State getState() const;
    QString getCurrentUri() const;
    qint64 getDuration() const;
    qint64 getPosition() const;
    int getVideoWidth() const;
    int getVideoHeight() const;
    double getVideoFrameRate() const;
    int getAudioSampleRate() const;
    int getAudioChannels() const;

signals:
    void stateChanged(State state);
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void mediaOpened(const QString& uri);
    void errorOccurred(const QString& errorMessage);
    void finished();
    void videoFrameReady(const aurorastream::modules::media::decoder::VideoFrame& frame);
    void audioFrameReady(const aurorastream::modules::media::decoder::AudioFrame& frame);

private slots:
    void onDecoderVideoFrameReady(const aurorastream::modules::media::decoder::VideoFrame& frame);
    void onDecoderAudioFrameReady(const aurorastream::modules::media::decoder::AudioFrame& frame);
    void onDecoderDurationChanged(qint64 duration);
    void onDecoderPositionChanged(qint64 position);
    void onDecoderErrorOccurred(const QString& errorMessage);
    void onDecoderMediaOpened(const QString& uri);
    void onDecoderFinished();

private:
    State m_state;

    std::unique_ptr<aurorastream::modules::media::decoder::Decoder> m_decoder;
    std::unique_ptr<aurorastream::modules::media::render::Renderer> m_renderer;

    QString m_currentUri;
    qint64 m_duration;
    qint64 m_position;

    void setState(State newState);
    void updatePosition(qint64 position);
    void updateDuration(qint64 duration);
    bool doOpen(const QString& uri);
    bool doPlay();
    bool doPause();
    bool doStop();
    bool doSeek(qint64 position);
    bool doClose();
    void handleVideoFrame(const aurorastream::modules::media::decoder::VideoFrame& frame);
    void handleAudioFrame(const aurorastream::modules::media::decoder::AudioFrame& frame);

};

}
}
}
}

#endif