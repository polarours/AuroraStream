/********************************************************************************
 * @file   : VideoWidget.cpp
 * @brief  : AuroraStream 视频播放器模块的实现
 *
 * 本文件实现了 VideoWidget 类，它提供了一个视频播放器界面
 * 包括播放、暂停、停止等基本功能
 *
 * @Author : polarours
 * @Date   : 2025/08/25
 ********************************************************************************/

#include "aurorastream/modules/ui/VideoWidget.h"

namespace aurorastream {
namespace modules {
namespace ui {

VideoWidget::VideoWidget(QWidget* parent)
    : QWidget(parent)
    // TODO: 初始化播放器
{

}

VideoWidget::~VideoWidget() {
    // TODO: 释放播放器资源
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