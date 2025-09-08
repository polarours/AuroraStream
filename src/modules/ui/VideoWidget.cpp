/********************************************************************************
 * @file   : VideoWidget.cpp
 * @brief  :
 *
 *
 *
 * @Author : polarours
 * @Date   : 2025/08/25
 ********************************************************************************/

#include "aurorastream/modules/ui/VideoWidget.h"

namespace aurorastream {
namespace modules {
namespace ui {

VideoWidget::VideoWidget(QWidget* parent) : QWidget(parent) {
    m_videoPlayer = new QMediaPlayer(this);
    m_videoWidget = new QVideoWidget(this);
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_videoWidget);
    setLayout(layout);
    
    m_videoPlayer->setVideoOutput(m_videoWidget);
    m_videoPlayer->setNotifyInterval(50);
}

VideoWidget::~VideoWidget() {
    // Cleanup resources
}

void VideoWidget::setVideoSource(const QString& source) {
    if(QFile::exists(source)) {
        m_videoPlayer->setMedia(QUrl::fromLocalFile(source));
    } else {
        m_videoPlayer->setMedia(QUrl(source));
    }
}

void VideoWidget::play() {
    if(m_videoPlayer->mediaStatus() != QMediaPlayer::NoMedia) {
        m_videoPlayer->play();
    }
}

void VideoWidget::pause() {
    if(m_videoPlayer->state() == QMediaPlayer::PlayingState) {
        m_videoPlayer->pause();
    }
}

void VideoWidget::stop() {
    m_videoPlayer->stop();
}

void VideoWidget::resizeEvent(QResizeEvent* event) {
    // Handle resize event
    QWidget::resizeEvent(event);
}

} // namespace ui
} // namespace modules
} // namespace aurorastream