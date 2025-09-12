#pragma once

#include <QObject>
#include <thread>
#include <atomic>
#include <mutex>
#include <QString>
#include <QProperty>
#include <memory>
#include "aurorastream/AuroraStream.h"
#include "aurorastream/modules/media/decoder/Decoder.h"
#include "aurorastream/modules/media/renderer/VideoRenderer.h"
#include "aurorastream/modules/media/renderer/AudioRenderer.h"

namespace aurorastream {
namespace modules {
namespace media {

struct VideoFrame {
    // 视频帧数据结构定义
};

struct AudioFrame {
    // 音频帧数据结构定义
};
namespace player {

class AURORASTREAM_API Player : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)

public:
    enum class State {
        Stopped = 0,
        Opening = 1,
        Opened = 2,
        Playing = 3,
        Paused = 4,
        Seeking = 5,
        Error = 6
    };
    Q_ENUM(State)

    explicit Player(QObject* parent = nullptr);
    ~Player() override;

    // 播放控制
    Q_INVOKABLE bool open(const QString& uri);
    Q_INVOKABLE bool play();
    Q_INVOKABLE bool pause();
    Q_INVOKABLE bool stop();
    Q_INVOKABLE bool seek(qint64 position);

    // 状态访问
    State state() const;
    qint64 position() const;
    qint64 duration() const;
    QString currentUri() const;

signals:
    void stateChanged(State newState);
    void positionChanged(qint64 newPosition);
    void durationChanged(qint64 newDuration);
    void error(const QString& error);
    void mediaOpened(const QString& uri);
    void finished();
    void videoFrameReady(const VideoFrame& frame);
    void audioFrameReady(const AudioFrame& frame);

private:
    QProperty<State> m_state;
    QProperty<qint64> m_position;
    QProperty<qint64> m_duration;
    QString m_currentUri;
    std::unique_ptr<decoder::Decoder> m_decoder;
    std::unique_ptr<renderer::VideoRenderer> m_videoRenderer;
    std::unique_ptr<renderer::AudioRenderer> m_audioRenderer;

    // 渲染器工厂方法
    std::unique_ptr<renderer::VideoRenderer> createVideoRenderer();
    std::unique_ptr<renderer::AudioRenderer> createAudioRenderer();

    // 线程相关
    std::thread m_decodeThread;
    std::thread m_audioThread;
    std::atomic<bool> m_running {false};
    std::mutex m_mutex;

    // SDL窗口相关
    void* m_sdlWindow {nullptr};
    void* m_sdlRenderer {nullptr};
    void* m_sdlTexture {nullptr};
};

} // namespace player
} // namespace media
} // namespace modules
} // namespace aurorastream