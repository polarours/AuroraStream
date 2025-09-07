/********************************************************************************
 * @file   : Decoder.cpp
 * @brief  :
 *
 *
 *
 * @Author : polarours
 * @Date   : 2025/08/25
 ********************************************************************************/

#include "aurorastream/modules/media/decoder/Decoder.h"
#include <QDebug>

namespace aurorastream {
namespace modules {
namespace media {
namespace decoder {

Decoder::Decoder(QObject *parent) : QObject(parent),
    m_isOpen(false),
    m_duration(0),
    m_position(0) {
    qDebug() << "Decoder initialized";
}

Decoder::~Decoder() {
    qDebug() << "Decoder destroyed";
}

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

void Decoder::close() {
    if (!m_isOpen) {
        qWarning() << "Decoder is not open";
        return;
    }

    m_isOpen = false;
    m_uri.clear();
    m_duration = 0;
    m_position = 0;
    qDebug() << "Closed media file";
}

void Decoder::start() {
    if (!m_isOpen) {
        qWarning() << "Decoder is not open";
        return;
    }

    qDebug() << "Playback started";
}

void Decoder::pause() {
    if (!m_isOpen) {
        qWarning() << "Decoder is not open";
        return;
    }

    qDebug() << "Playback paused";
}

void Decoder::stop() {
    if (!m_isOpen) {
        qWarning() << "Decoder is not open";
        return;
    }

    qDebug() << "Playback stopped";
}

void Decoder::seek(qint64 position) {
    if (!m_isOpen) {
        qWarning() << "Decoder is not open";
        return;
    }

    m_position = position;
    qDebug() << "Seek to position:" << position;
}

bool Decoder::isOpen() const {
    return m_isOpen;
}

bool Decoder::isPlaying() const {
    return m_isOpen && m_position > 0;
}

bool Decoder::isPaused() const {
    return m_isOpen && m_position > 0;
}

bool Decoder::isStopped() const {
    return !m_isOpen;
}

qint64 Decoder::getDuration() const {
    return m_duration;
}

qint64 Decoder::getPosition() const {
    return m_position;
}

int Decoder::getVideoWidth() const {
    return 0; // Default implementation, should be overridden
}

int Decoder::getVideoHeight() const {
    return 0; // Default implementation, should be overridden
}

int Decoder::getVideoFrameRate() const {
    return 0; // Default implementation, should be overridden
}

int Decoder::getAudioSampleRate() const {
    return 0; // Default implementation, should be overridden
}

int Decoder::getAudioChannels() const {
    return 0; // Default implementation, should be overridden
}

} // namespace decoder
} // namespace media
} // namespace modules
} // namespace aurorastream