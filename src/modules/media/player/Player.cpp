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

/**
 * @brief Player 类的构造函数
 * @param parent 父对象
 * @note 初始化播放器状态
 */
Player::Player(QObject *parent) : QObject(parent),
    m_state(State::Stopped),
    m_duration(0),
    m_position(0)
{
    // 生成创建日志
    qDebug() << "Player initialized";
}

Player::~Player()
{
    // 生成销毁日志
    qDebug() << "Player destroyed";
}

/**
 * @brief 设置解码器
 * @param decoder 解码器对象
 * @note 解码器对象的所有权将被转移到 Player 对象
 */
void Player::setDecoder(std::unique_ptr<aurorastream::modules::media::decoder::Decoder> decoder) {
    m_decoder = std::move(decoder);
}

/**
 * @brief 设置渲染器
 * @param renderer 渲染器对象
 * @note 渲染器对象的所有权将被转移到 Player 对象
 */
void Player::setRenderer(std::unique_ptr<aurorastream::modules::media::render::Renderer> renderer) {
    m_renderer = std::move(renderer);
}

/**
 * @brief 打开媒体文件
 * @param uri 媒体文件的URI
 * @return 是否成功打开
 */
bool Player::open(const QString &uri) {
    return doOpen(uri);
}

/**
 * @brief 播放媒体文件
 */
void Player::play() {
    doPlay();
}

/**
 * @brief 暂停播放
 */
void Player::pause() {
    doPause();
}

/**
 * @brief 停止播放
 */
void Player::stop() {
    doStop();
}

/**
 * @brief 跳转到指定位置
 * @param position 位置（毫秒）
 */
void Player::seek(qint64 position) {
    doSeek(position);
}

/**
 * @brief 关闭媒体文件
 */
void Player::close() {
    doClose();
}

/**
 * @brief 获取当前播放状态
 * @return 当前播放状态
 */
Player::State Player::getState() const {
    return m_state;
}

/**
 * @brief 获取当前播放的媒体URI
 * @return 当前播放的媒体URI
 */
QString Player::getCurrentUri() const {
    return m_currentUri;
}

/**
 * @brief 获取当前播放的媒体时长
 * @return 当前播放的媒体时长
 */
qint64 Player::getDuration() const {
    return m_duration;
}

/**
 * @brief 获取当前播放的媒体位置
 * @return 当前播放的媒体位置
 */
qint64 Player::getPosition() const {
    return m_position;
}

/**
 * @brief 获取当前播放的媒体视频宽度
 * @return 当前播放的媒体视频宽度
 */
int Player::getVideoWidth() const {
    return m_decoder ? m_decoder->getVideoWidth() : 0;
}

/**
 * @brief 获取当前播放的媒体视频高度
 * @return 当前播放的媒体视频高度
 */
int Player::getVideoHeight() const {
    return m_decoder ? m_decoder->getVideoHeight() : 0;
}

/**
 * @brief 获取当前播放的媒体视频帧率
 * @return 当前播放的媒体视频帧率
 */
double Player::getVideoFrameRate() const {
    return m_decoder ? m_decoder->getVideoFrameRate() : 0.0;
}

/**
 * @brief 获取当前播放的媒体音频采样率
 * @return 当前播放的媒体音频采样率
 */
int Player::getAudioSampleRate() const {
    return m_decoder ? m_decoder->getAudioSampleRate() : 0;
}

/**
 * @brief 获取当前播放的媒体音频通道数
 * @return 当前播放的媒体音频通道数
 */
int Player::getAudioChannels() const {
    return m_decoder ? m_decoder->getAudioChannels() : 0;
}

/**
 * @brief 设置播放状态
 * @param newState 新的播放状态
 * @note 如果新的状态与当前状态不同，则发出 stateChanged 信号
 */
void Player::setState(State newState) {
    if (m_state != newState) {
        m_state = newState;
        emit stateChanged(m_state);
    }else {
        qDebug() << "Player::setState(): Already in state:" << m_state;
    }
}

/**
 * @brief 更新当前播放位置
 * @param newPosition 新的播放位置
 * @note 如果新的位置与当前位置不同，则发出 positionChanged 信号
 */
void Player::updatePosition(qint64 newPosition) {
    if (m_position != newPosition) {
        m_position = newPosition;
        emit positionChanged(m_position);
    }else {
        qDebug() << "Player::updatePosition(): Already at position:" << m_position;
    }
}

/**
 * @brief 更新当前播放时长
 * @param newDuration 新的播放时长
 * @note 如果新的时长与当前时长不同，则发出 durationChanged 信号
 */
void Player::updateDuration(qint64 newDuration) {
    if (m_duration != newDuration) {
        m_duration = newDuration;
        emit durationChanged(m_duration);
    }else {
        qDebug() << "Player::updateDuration(): Already at duration:" << m_duration;
    }
}

/**
 * @brief 打开媒体文件
 * @param uri 媒体文件的URI
 * @return 是否成功打开
 * @note 如果当前状态是停止状态，则切换为打开状态，否则直接返回false
 * @note 如果打开成功，则发出 mediaOpened 信号，否则发出 errorOccurred 信号
 */
bool Player::doOpen(const QString &uri) {
    if (m_state != State::Stopped) {
        return false; // 如果不是停止状态，则直接返回false
    }

    setState(State::Opening); // 切换为正在打开状态
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

/**
 * @brief 播放媒体
 * @return 是否成功播放
 * @note 如果当前状态是打开状态或者暂停状态，则切换为正在播放状态，否则直接返回false
 * @note 如果播放成功，则发出 mediaPlayed 信号，否则发出 errorOccurred 信号
 */
bool Player::doPlay() {
    if (m_state != State::Opened && m_state != State::Paused) {
        return false;
    }

    setState(State::Starting);
    if (m_decoder && m_decoder->play()) {
        setState(State::Playing);
        emit mediaPlayed();
        return true;
    } else {
        setState(State::Error);
        emit errorOccurred("Failed to start playback");
        return false;
    }
}

/**
 * @brief 暂停媒体
 * @return 是否成功暂停
 * @note 如果当前状态是播放状态，则切换为正在暂停状态，否则直接返回false
 * @note 如果暂停成功，则发出 mediaPaused 信号，否则发出 errorOccurred 信号
 */
bool Player::doPause() {
    if (m_state != State::Playing) {
        return false;
    }

    setState(State::Pausing);
    if (m_decoder && m_decoder->pause()) {
        setState(State::Paused);
        emit mediaPaused();
        return true;
    } else {
        setState(State::Error);
        emit errorOccurred("Failed to pause playback");
        return false;
    }
}

/**
 * @brief 停止媒体
 * @return 是否成功停止
 * @note 如果当前状态是停止状态，则直接返回false
 * @note 如果停止成功，则发出 finished 信号，否则发出 errorOccurred 信号
 */
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

/**
 * @brief 改变播放位置
 * @param newPosition 新的播放位置
 * @return 是否成功改变播放位置
 * @note 如果当前状态是播放状态或者暂停状态，则切换为正在改变播放位置状态，否则直接返回false
 * @note 如果改变播放位置成功，则发出 positionChanged 信号，否则发出 errorOccurred 信号
 */
bool Player::doSeek(qint64 newPosition) {
    if (m_state != State::Playing && m_state != State::Paused) {
        return false;
    }

    setState(State::Seeking);
    if (m_decoder && m_decoder->seek(newPosition)) {
        updatePosition(newPosition);
        emit positionChanged(newPosition);
        setState(m_state == State::Playing ? State::Playing : State::Paused);
        return true;
    } else {
        setState(State::Error);
        emit errorOccurred("Failed to seek");
        return false;
    }
}

/**
 * @brief 关闭媒体文件
 * @return 是否成功关闭
 * @note 如果当前状态是停止状态，则直接返回false
 * @note 如果关闭成功，则发出 mediaClosed 信号，否则发出 errorOccurred 信号
 */
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

/**
 * @brief 处理视频帧
 * @param frame 视频帧
 */
void Player::handleVideoFrame(const aurorastream::modules::media::decoder::VideoFrame &frame) {
    if (m_renderer) {
        m_renderer->renderVideoFrame(frame);
    }
    emit videoFrameReady(frame);
}

/**
 * @brief 处理音频帧
 * @param frame 音频帧
 */
void Player::handleAudioFrame(const aurorastream::modules::media::decoder::AudioFrame &frame) {
    if (m_renderer) {
        m_renderer->renderAudioFrame(frame);
    }
    emit audioFrameReady(frame);
}

/**
 * @brief 响应视频帧
 * @param frame 视频帧
 */
void Player::onVideoFrameReady(const aurorastream::modules::media::decoder::VideoFrame &frame) {
    handleVideoFrame(frame);
}

/**
 * @brief 响应音频帧
 * @param frame 音频帧
 */
void Player::onAudioFrameReady(const aurorastream::modules::media::decoder::AudioFrame &frame) {
    handleAudioFrame(frame);
}

/**
 * @brief 响应总时长变化
 * @param duration 总时长
 */
void Player::onDurationChanged(qint64 duration) {
    updateDuration(duration);
}

void Player::onPositionChanged(qint64 position) {
    updatePosition(position);
}

void Player::onErrorOccurred(const QString &errorMessage) {
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