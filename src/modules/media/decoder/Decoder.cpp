/********************************************************************************
 * @file   : Decoder.cpp
 * @brief  : AuroraStream 媒体解码器模块的实现
 *
 * 本文件实现了 Decoder 类，它提供了媒体解码器的基本功能
 * 包括初始化、解码控制、状态管理等
 *
 * @Author : polarours
 * @Date   : 2025/08/25
 ********************************************************************************/

#include <QDebug>

#include "aurorastream/modules/media/decoder/Decoder.h"

namespace aurorastream {
namespace modules {
namespace media {
namespace decoder {

/**
 * @brief Decoder 类的构造函数
 * @param parent 父对象
 * @note 初始化解码器状态
 */
Decoder::Decoder(QObject *parent)
	: QObject(parent),
    m_isOpen(false),
    m_duration(0),
    m_position(0)
{
	qDebug() << "Decoder initialized";
}

/**
 * @brief Decoder 类的析构函数
 * @note 释放解码器资源
 */
Decoder::~Decoder() {
    qDebug() << "Decoder destroyed";
}

/**
 * @brief 打开媒体文件
 * @param uri 媒体文件的URI
 * @return 是否成功打开
 */
bool Decoder::open(const QString &uri) {
    if (m_isOpen) {
        qWarning() << "Decoder is already open";
        return false;
    }

    m_uri = uri;
    m_isOpen = true;
    qDebug() << "Opened media file:" << uri;

    return true;
}

/**
 * @brief 关闭媒体文件
 * @note 关闭媒体文件后，解码器将进入 Stopped 状态
 */
void Decoder::close() {
    if (!m_isOpen) {
        qWarning() << "Decoder is not open";
        return;
    }

    m_isOpen = false;
    m_duration = 0;
    m_position = 0;
    m_uri.clear();

	qDebug() << "Closed media file";
}

/**
 * @brief 开始解码
 * @note 开始解码后，解码器将进入 Playing 状态
 */
void Decoder::start() {
    if (!m_isOpen) {
        qWarning() << "Decoder is not open";
        return;
    }

    qDebug() << "Playback started";
}

/**
 * @brief 暂停解码
 * @note 暂停解码后，解码器将进入 Paused 状态
 */
void Decoder::pause() {
    if (!m_isOpen) {
        qWarning() << "Decoder is not open";
        return;
    }

    qDebug() << "Playback paused";
}

/**
 * @brief 停止解码
 * @note 停止解码后，解码器将进入 Stopped 状态
 */
void Decoder::stop() {
    if (!m_isOpen) {
        qWarning() << "Decoder is not open";
        return;
    }

    qDebug() << "Playback stopped";
}


/**
 * @brief 定位到指定位置
 * @param position 定位位置
 */
void Decoder::seek(qint64 position) {
    if (!m_isOpen) {
        qWarning() << "Decoder is not open";
        return;
    }

    m_position = position;

    qDebug() << "Seek to position:" << position;
}

/**
 * @brief 获取当前媒体是否已经打开
 * @return 当前媒体是否已经打开
 */
bool Decoder::isOpen() const {
    return m_isOpen;
}

/**
 * @brief 获取当前媒体是否正在播放
 * @return 当前媒体是否正在播放
 */
bool Decoder::isPlaying() const {
    return m_isOpen && m_position > 0;
}

/**
 * @brief 获取当前媒体是否已经暂停
 * @return 当前媒体是否已经暂停
 */
bool Decoder::isPaused() const {
    return m_isOpen && m_position > 0;
}

/**
 * @brief 获取当前媒体是否已经停止
 * @return 当前媒体是否已经停止
 */
bool Decoder::isStopped() const {
    return !m_isOpen;
}

/**
 * @brief 获取当前媒体的总时长
 * @return 当前媒体的总时长
 */
qint64 Decoder::getDuration() const {
    return m_duration;
}

/**
 * @brief 获取当前媒体的当前位置
 * @return 当前媒体的当前位置
 */
qint64 Decoder::getPosition() const {
    return m_position;
}

/**
 * @brief 获取当前媒体的视频宽度
 * @return 当前媒体的视频宽度
 */
int Decoder::getVideoWidth() const {
    return m_videoWidth;
}

/**
 * @brief 获取当前媒体的视频高度
 * @return 当前媒体的视频高度
 */
int Decoder::getVideoHeight() const {
    return m_videoHeight;
}

/**
 * @brief 获取当前媒体的视频帧率
 * @return 当前媒体的视频帧率
 */
double Decoder::getVideoFrameRate() const {
    return m_videoFrameRate;
}

/**
 * @brief 获取当前媒体的音频采样率
 * @return 当前媒体的音频采样率
 */
int Decoder::getAudioSampleRate() const {
    return m_audioSampleRate;
}

/**
 * @brief 获取当前媒体的音频通道数
 * @return 当前媒体的音频通道数
 */
int Decoder::getAudioChannels() const {
    return m_audioChannels;
}

} // namespace decoder
} // namespace media
} // namespace modules
} // namespace aurorastream