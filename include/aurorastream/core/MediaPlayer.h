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
#include <unordered_set>

#include "aurorastream/AuroraStream.h"

// FFmpeg 头文件
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

namespace aurorastream {
namespace core {

class AURORASTREAM_API MediaPlayer : public QObject, public MediaController
{
    Q_OBJECT
public:

    explicit MediaPlayer(QObject *parent = nullptr);

    ~MediaPlayer() override;

    void play() override;
    void pause() override;
    void stop() override;
    bool load(const std::string& uri) override;
    void seek(int64_t position) override;

    MediaState getState() const override;

    bool isPlaying() const;

    qint64 getPosition() const override;
    qint64 getDuration() const override;
    QString getCurrentMedia() const override;
    std::string getCurrentUrl() const override;

    void setVolume(float volume) override;
    float getVolume() const override;
    void setLoop(bool loop) override;
    bool getLoop() const override;

    Q_INVOKABLE bool openFile(const QString& fileName);
    Q_INVOKABLE bool setSource(const QString& source);

signals:
    void stateChanged(MediaState state);
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void mediaOpened(const QString& fileName);
    void error(const QString& message);
    void volumeChanged(float volume);
    void loopChanged(bool loop);

private:
    MediaState m_state;
    qint64 m_duration;
    qint64 m_position;
    QString m_currentMedia;
    int m_videoStreamIndex;
    int m_audioStreamIndex;
    AVFormatContext* m_formatContext;
    AVCodecContext* m_videoCodecContext;
    AVCodecContext* m_audioCodecContext;
    float m_volume;
    bool m_loop;
};

} // namespace core
} // namespace aurorastream

#endif // AURORASTREAM_CORE_MEDIAPLAYER_H