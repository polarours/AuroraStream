

#include "aurorastream/modules/media/player/Player.h"

#include "aurorastream/AuroraStream.h"
#include "aurorastream/modules/media/decoder/Decoder.h"
#include "aurorastream/modules/media/renderer/VideoRenderer.h"
#include "aurorastream/modules/media/renderer/AudioRenderer.h"
#include <QtCore/QObject>
#include <QtCore/QDebug>
#include <QDebug>

#include "SDL2/SDL_audio.h"

namespace aurorastream {
namespace modules {
namespace media {
namespace player {

Player::Player(QObject* parent)
    : QObject(parent),
    m_state(State::Stopped),
    m_position(0),
    m_duration(0)
{
    qDebug() << "Player initialized";
}

Player::~Player()
{
    stop();
    qDebug() << "Player destroyed";
}

bool Player::open(const QString& uri)
{
    if (m_state != State::Stopped) {
        qWarning() << "Player is not in stopped state";
        return false;
    }

    m_state = State::Opening;
    emit stateChanged(m_state);

    // 初始化解码器（视频解码器）
    m_decoder = std::make_unique<decoder::Decoder>(decoder::Decoder::Type::VIDEO);

    // 延迟渲染器初始化
    m_videoRenderer = nullptr;
    m_audioRenderer = nullptr;

    // 打开媒体源
    if (m_decoder->open(uri)) {
        // 根据媒体类型初始化渲染器
        if (m_decoder->hasVideo()) {
            m_videoRenderer = createVideoRenderer(); // 需要实现此工厂方法
            if (!m_videoRenderer) {
                qWarning() << "Failed to create video renderer";
                return false;
            }
        }

        if (m_decoder->hasAudio()) {
            m_audioRenderer = createAudioRenderer(); // 需要实现此工厂方法
            if (!m_audioRenderer) {
                qWarning() << "Failed to create audio renderer";
                return false;
            }
        }
        m_currentUri = uri;
        m_duration = m_decoder->getDuration();
        m_state = State::Opened;
        emit stateChanged(m_state);
        emit mediaOpened(uri);
        emit durationChanged(m_duration);
        return true;
    }

    m_state = State::Error;
    emit stateChanged(m_state);
    emit error("Failed to open media");
    return false;
}

bool Player::play()
{
    if (m_state != State::Opened && m_state != State::Paused) {
        qWarning() << "Invalid state for play operation";
        return false;
    }

    m_state = State::Playing;
    emit stateChanged(m_state);

    // 启动解码线程
    m_decodeThread = std::thread([this]() {
        while (m_state == State::Playing) {
            auto frame = m_decoder->getNextFrame();
            if (frame) {
                if (frame->type == decoder::FrameType::VIDEO && m_videoRenderer) {
                    m_videoRenderer->render(*frame);
                } else if (frame->type == decoder::FrameType::AUDIO && m_audioRenderer) {
                    // 对于音频帧，需要转换为AudioFrame
                    decoder::AudioFrame audioFrame;
                    audioFrame.type = decoder::FrameType::AUDIO;
                    audioFrame.data[0] = frame->data[0];
                    audioFrame.samples = frame->width; // 使用width作为samples
                    audioFrame.channels = 2;
                    audioFrame.sampleRate = 44100;
                    audioFrame.pts = frame->pts;
                    audioFrame.duration = frame->duration;
                    m_audioRenderer->queueAudio(audioFrame);
                }
                m_position = frame->pts;
                emit positionChanged(m_position);
            }
        }
    });

    // 启动音频渲染线程
    if (m_audioRenderer) {
        m_audioThread = std::thread([this]() {
            m_audioRenderer->play();
        });
    }
    return true;
}

bool Player::pause()
{
    if (m_state != State::Playing) {
        qWarning() << "Invalid state for pause operation";
        return false;
    }

    m_state = State::Paused;
    emit stateChanged(m_state);
    return true;
}

bool Player::stop()
{
    if (m_state == State::Stopped) {
        return true;
    }

    // 停止解码线程
    if (m_decodeThread.joinable()) {
        m_decodeThread.join();
    }

    // 停止音频线程
    if (m_audioThread.joinable()) {
        m_audioRenderer->stop();
        m_audioThread.join();
    }

    // 清理渲染器
    if (m_videoRenderer) {
        m_videoRenderer->cleanup();
    }
    if (m_audioRenderer) {
        m_audioRenderer->cleanup();
    }

    m_state = State::Stopped;
    m_position = 0;
    m_currentUri.clear();
    emit stateChanged(m_state);
    emit positionChanged(m_position);
    emit finished();
    return true;
}

bool Player::seek(qint64 position)
{
    if (m_state != State::Playing && m_state != State::Paused) {
        qWarning() << "Invalid state for seek operation";
        return false;
    }

    State oldState = m_state;
    m_state = State::Seeking;
    emit stateChanged(m_state);

    // TODO: 实现精确跳转
    if (m_decoder->seek(position)) {
        m_position = position;
        m_state = oldState;
        emit stateChanged(m_state);
        emit positionChanged(m_position);
        return true;
    }

    m_state = State::Error;
    emit stateChanged(m_state);
    emit error("Seek failed");
    return false;
}

Player::State Player::state() const
{
    return m_state;
}

qint64 Player::position() const
{
    return m_position;
}

qint64 Player::duration() const
{
    return m_duration;
}

QString Player::currentUri() const
{
    return m_currentUri;
}

} // namespace player
} // namespace media
} // namespace modules
std::unique_ptr<modules::media::renderer::VideoRenderer> modules::media::player::Player::createVideoRenderer()
{
    // 简单实现，返回nullptr
    return nullptr;
}

std::unique_ptr<modules::media::renderer::AudioRenderer> modules::media::player::Player::createAudioRenderer()
{
    // 简单实现，返回nullptr
    return nullptr;
}

} // namespace aurorastream