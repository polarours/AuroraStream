/********************************************************************************
 * @file   : Player.cpp
 * @brief  : AuroraStream 媒体播放器模块的实现
 *
 * 本文件实现了 Player 类，它提供了媒体播放器的基本功能
 * 包括初始化、播放控制、状态管理等
 *
 * @Author : polarours
 * @Date   : 2025/08/25
 ********************************************************************************/

#include "aurorastream/modules/media/player/Player.h"

namespace aurorastream {
namespace modules {
namespace media {
namespace player {

Player::Player(QObject *parent) : QObject(parent),
    m_state(State::Stopped),
    m_duration(0),
    m_position(0) {}

Player::~Player() {}

void Player::setDecoder(std::unique_ptr<aurorastream::modules::media::decoder::Decoder> decoder) {
    m_decoder = std::move(decoder);
}

void Player::setRenderer(std::unique_ptr<aurorastream::modules::media::render::Renderer> renderer) {
    m_renderer = std::move(renderer);
}

bool Player::open(const QString &uri) {
    return doOpen(uri);
}

void Player::play() {
    doPlay();
}

void Player::pause() {
    doPause();
}

void Player::stop() {
    doStop();
}

void Player::seek(qint64 position) {
    doSeek(position);
}

void Player::close() {
    doClose();
}

Player::State Player::getState() const {
    return m_state;
}

QString Player::getCurrentUri() const {
    return m_currentUri;
}

qint64 Player::getDuration() const {
    return m_duration;
}

qint64 Player::getPosition() const {
    return m_position;
}

int Player::getVideoWidth() const {
    return m_decoder ? m_decoder->getVideoWidth() : 0;
}

int Player::getVideoHeight() const {
    return m_decoder ? m_decoder->getVideoHeight() : 0;
}

double Player::getVideoFrameRate() const {
    return m_decoder ? m_decoder->getVideoFrameRate() : 0.0;
}

int Player::getAudioSampleRate() const {
    return m_decoder ? m_decoder->getAudioSampleRate() : 0;
}

int Player::getAudioChannels() const {
    return m_decoder ? m_decoder->getAudioChannels() : 0;
}

void Player::setState(State newState) {
    if (m_state != newState) {
        m_state = newState;
        emit stateChanged(m_state);
    }
}

void Player::updatePosition(qint64 position) {
    if (m_position != position) {
        m_position = position;
        emit positionChanged(m_position);
    }
}

void Player::updateDuration(qint64 duration) {
    if (m_duration != duration) {
        m_duration = duration;
        emit durationChanged(m_duration);
    }
}

bool Player::doOpen(const QString &uri) {
    if (m_state != State::Stopped) {
        return false;
    }

    setState(State::Opening);
    if (m_decoder && m_decoder->open(uri)) {
        m_currentUri = uri;
        setState(State::Opened);
        emit mediaOpened(uri);
        return true;
    } else {
        setState(State::Error);
        emit errorOccurred("Failed to open media file");
        return false;
    }
}

bool Player::doPlay() {
    if (m_state != State::Opened && m_state != State::Paused) {
        return false;
    }

    setState(State::Starting);
    if (m_decoder && m_decoder->play()) {
        setState(State::Playing);
        return true;
    } else {
        setState(State::Error);
        emit errorOccurred("Failed to start playback");
        return false;
    }
}

bool Player::doPause() {
    if (m_state != State::Playing) {
        return false;
    }

    setState(State::Pausing);
    if (m_decoder && m_decoder->pause()) {
        setState(State::Paused);
        return true;
    } else {
        setState(State::Error);
        emit errorOccurred("Failed to pause playback");
        return false;
    }
}

bool Player::doStop() {
    if (m_state == State::Stopped) {
        return false;
    }

    setState(State::Stopping);
    if (m_decoder && m_decoder->stop()) {
        setState(State::Stopped);
        m_currentUri.clear();
        m_duration = 0;
        m_position = 0;
        emit finished();
        return true;
    } else {
        setState(State::Error);
        emit errorOccurred("Failed to stop playback");
        return false;
    }
}

bool Player::doSeek(qint64 position) {
    if (m_state != State::Playing && m_state != State::Paused) {
        return false;
    }

    setState(State::Seeking);
    if (m_decoder && m_decoder->seek(position)) {
        updatePosition(position);
        setState(m_state == State::Playing ? State::Playing : State::Paused);
        return true;
    } else {
        setState(State::Error);
        emit errorOccurred("Failed to seek");
        return false;
    }
}

bool Player::doClose() {
    if (m_state == State::Stopped) {
        return false;
    }

    setState(State::Closing);
    if (m_decoder && m_decoder->close()) {
        setState(State::Stopped);
        m_currentUri.clear();
        m_duration = 0;
        m_position = 0;
        return true;
    } else {
        setState(State::Error);
        emit errorOccurred("Failed to close media file");
        return false;
    }
}

void Player::handleVideoFrame(const aurorastream::modules::media::decoder::VideoFrame &frame) {
    if (m_renderer) {
        m_renderer->renderVideoFrame(frame);
    }
    emit videoFrameReady(frame);
}

void Player::handleAudioFrame(const aurorastream::modules::media::decoder::AudioFrame &frame) {
    if (m_renderer) {
        m_renderer->renderAudioFrame(frame);
    }
    emit audioFrameReady(frame);
}

void Player::onDecoderVideoFrameReady(const aurorastream::modules::media::decoder::VideoFrame &frame) {
    handleVideoFrame(frame);
}

void Player::onDecoderAudioFrameReady(const aurorastream::modules::media::decoder::AudioFrame &frame) {
    handleAudioFrame(frame);
}

void Player::onDecoderDurationChanged(qint64 duration) {
    updateDuration(duration);
}

void Player::onDecoderPositionChanged(qint64 position) {
    updatePosition(position);
}

void Player::onDecoderErrorOccurred(const QString &errorMessage) {
    setState(State::Error);
    emit errorOccurred(errorMessage);
}

void Player::onDecoderMediaOpened(const QString &uri) {
    m_currentUri = uri;
    emit mediaOpened(uri);
}

void Player::onDecoderFinished() {
    setState(State::Stopped);
    emit finished();
}

} // namespace player
} // namespace media
} // namespace modules
} // namespace aurorastream