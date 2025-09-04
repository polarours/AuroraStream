/********************************************************************************
 * @file   : MediaPlayer.cpp
 * @brief  : 实现了 aurorastream::core::MediaPlayer 类。
 *
 * 该文件实现了 aurorastream::core::MediaPlayer 类，该类用于播放媒体文件。
 * 功能包括：播放、暂停、停止、快进、快退、音量控制、播放进度控制等。
 *
 * @Author : polarours
 * @Date   : 2025/08/22
 ********************************************************************************/

  #include "AuroraStream/core/MediaPlayer.h"

  #inlude <QTimer>
  #include <QFileInfo>
  #include <QDir>
  #include <QDebug>
  #include <QDateTime>

extern "C" {
  #include <libavformat/avformat.h>
  #include <libavcodec/avcodec.h>
  #include <libavutil/imgutils.h>
  #include <libswscale/swscale.h>
  #include <libswresample/swresample.h> 
  #include <libavutil/samplefmt.h>
  #include <libavutil/avstring.h>
  #include <libavutil/error.h>
  #include <libavutil/time.h>
}

#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <functional>

namespace aurorastream{
namespace core{

MediaPlayer::MediaPlayer(QObject *parent)
    : QObject(parent) // 调用基类 QObject 的构造函数
    // --- 初始化状态和信息 ---
    , m_state(State::Stopped) // 初始状态为停止
    , m_duration(0)          // 初始媒体时长为0毫秒
    , m_position(0)          // 初始播放位置为0毫秒
    // --- 初始化 FFmpeg 相关指针 ---
    , m_formatContext(nullptr)      // FFmpeg 格式上下文指针初始化为空
    , m_videoCodecContext(nullptr)  // FFmpeg 视频解码器上下文指针初始化为空
    , m_audioCodecContext(nullptr)  // FFmpeg 音频解码器上下文指针初始化为空
    , m_videoStreamIndex(-1)       // -1 表示未找到有效的视频流
    , m_audioStreamIndex(-1)       // -1 表示未找到有效的音频流

{
    avformat_network_init(); 

    qDebug() << "MediaPlayer created.";
}

}
}