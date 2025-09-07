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

#include <QTimer>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QDateTime>

#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <functional>

#include "AuroraStream/core/MediaPlayer.h"

// ---  FFmpeg 相关头文件 ---
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

namespace aurorastream{
namespace core{

/**
 * @brief 构造函数，初始化播放器状态
 * @param parent QObject指针，用于Qt对象树管理
 * @note 该构造函数会初始化 FFmpeg 网络模块
 */
MediaPlayer::MediaPlayer(QObject *parent)
    : QObject(parent)               // 调用基类 QObject 的构造函数
    // --- 初始化状态和信息 ---
    , m_state(State::Stopped)       // 初始状态为停止
    , m_duration(0)                 // 初始媒体时长为0毫秒
    , m_position(0)                 // 初始播放位置为0毫秒
    // --- 初始化 FFmpeg 相关指针 ---
    , m_formatContext(nullptr)      // FFmpeg 格式上下文指针初始化为空
    , m_videoCodecContext(nullptr)  // FFmpeg 视频解码器上下文指针初始化为空
    , m_audioCodecContext(nullptr)  // FFmpeg 音频解码器上下文指针初始化为空
    , m_videoStreamIndex(-1)        // -1 表示未找到有效的视频流
    , m_audioStreamIndex(-1)        // -1 表示未找到有效的音频流

{
	avformat_network_init(); //  初始化 FFmpeg 网络模块

    qDebug() << "MediaPlayer created.";
}

/**
 * @brief 析构函数，负责释放资源
 * 负责释放 FFmpeg 相关资源，并停止播放线程
 */
MediaPlayer::~MediaPlayer()
{
	stop(); //  确保停止播放

    // 释放解码器上下文
	if  (m_videoCodecContext) {
           avcodec_free_context(&m_videoCodecContext);
	}

    // 释放解码器上下文
	if (m_audioCodecContext) {
	       avcodec_free_context(&m_audioCodecContext);
	}

	// 释放格式上下文
	if (m_formatContext) {
	       avformat_free_context(m_formatContext);
	}

	qDebug() << "MediaPlayer destroyed."; // 生成销毁日志

}

/**
 * @brief 开始播放当前加载的媒体
 * @note 如果当前没有加载媒体，则不会执行任何操作
 */
void MediaPlayer::play()
{
	qDebug() << "MediaPlayer::play() called. Current state:" << m_state; ";

	//  状态检查：如果当前状态为停止且未加载媒体，则发出警告并返回
	if (m_state == State::Stopped && m_currentMedia.isEmpty()) {
        qWarning() << "MediaPlayer::play() called, but no media is loaded.";
        return;
	}

	//  状态切换：如果当前状态为暂停或停止，则切换为播放状态
    if (m_state == State::Paused ||  m_state == State::Stopped) {
		m_state = State::Playing;
        qDebug() << "MediaPlayer: Playing.";

        emit stateChanged(m_state);
	}

	// 输出已经是播放状态的情况
	qDebug() << "MediaPlayer::play(): Already playing.";
}

/**
 * @brief 暂停当前播放的媒体
 * 根据当前状态决定是否暂停播放
 * - 如果当前状态为播放状态，则切换为暂停状态
 * - 如果当前状态不是播放状态，则不会执行任何操作
 */
void  MediaPlayer::pause()
{
	qDebug() << "MediaPlayer::pause() called. Current state:" << m_state;

  	//  状态检查：如果当前状态为播放状态，则切换为暂停状态
    if (m_state == State::Playing) {
        m_state = State::Paused;
        qDebug() << "MediaPlayer: Paused.";
        emit stateChanged(m_state);
	}

	//  输出非播放状态的情况
	qDebug() << "MediaPlayer::pause(): Not playing,  cannot pause."
}

/**
 * @brief 停止当前播放的媒体
 * 将播放器状态设置为停止，并重置播放位置
 * 如果当前状态不是停止状态，则切换为停止状态
 * 如果当前状态已经是停止状态，则不会执行任何操作
 */
void MediaPlayer::stop()
{
	qDebug() << "MediaPlayer::stop() called. Current state:" << m_state;

	//  状态检查：如果当前状态不是停止状态，则切换为停止状态
	if (m_state != State::Stopped) {
        m_state = State::Stopped;
        qint64 oldPosition = m_position;
        m_position = 0;
        qDebug() << "MediaPlayer: Stopped.";
        emit stateChanged(m_state);

		if (oldPosition != m_position) {
			emit positionChanged(m_position);
		}

        qDebug() << "MediaPlayer::stop(): Already stopped.";
	}
}

/**
 * @brief 加载媒体文件
 * @param filename 媒体文件路径
 * @return 成功加载返回 true，否则返回 false
 */
bool MediaPlayer::openFile(const QString &filename)
{
	qDebug() <<  "MediaPlayer::openFile() called with filename:" << filename;

	QFileInfo fileInfo(filename);

	// 检查文件是否存在
	if (!fileInfo.exists() && !fileInfo.isFile()) {
		QString errorMessage = QString("MediaPlayer::openFile() failed. File does not exist or is not a regular file: %1").arg(filename);
		qWarning() << errorMessage;
		emit error(errorMessage);
		return false;
	}

	stop(); // 停止当前播放

	// 释放之前加载的媒体资源
	if (m_formatContext) {
		avformat_close_input(&m_formatContext);
        avformat_free_context(m_formatContext);
		m_formatContext = nullptr;
		m_videoCodecContext = nullptr;
		m_audioCodecContext = nullptr;
		m_videoStreamIndex = -1;
		m_audioStreamIndex = -1;
		m_duration = 0;
		m_currentMedia.clear();
	}

	AVFormatContext* formatContext = nullptr;
	int ret = avformat_open_input(&formatContext, filename.toStdString().c_str(), nullptr, nullptr);

	// 检查文件是否成功打开
	if (ret < 0) {
		QString errorMessage = QString("MediaPlayer::openFile() failed. Could not open file: %1").arg(filename);
		char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		QString detailMessage = QString("FFmpeg error: %1 (%2)").arg(errorMessage).arg(errbuf);
	    qWarning() << detailMessage;
		emit error(detailMessage);
		return false;
	}

    ret = avformat_find_stream_info(formatContext, nullptr);

	// 如果未找到流信息，则返回 false
	if (ret < 0) {
		QString errorMessage = QString("MediaPlayer::openFile() failed. Could not find stream information in file: %1").arg(filename);
	   	char  errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		QString detailMessage = QString("FFmpeg error: %1 (%2)").arg(errorMessage).arg(errbuf);
		qWarning() << detailMessage;
		avformat_close_input(&formatContext);
		emit error(detailMessage);
		return false;
	}

	int videoStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);// 查找视频流
	int audioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);// 查找音频流

	AVCodecContext* videoCodecContext = nullptr;// 视频解码器上下文
	AVCodecContext* audioCodecContext = nullptr;// 音频解码器上下文

	// 查找并初始化视频解码器
	if (videoStreamIndex >= 0) {
		AVStream* videoStream = formatContext->streams[videoStreamIndex];
		const AVCodec* videoCodec = acvodec_find_decoder(videoStream->codecpar->codec_id);
		if (videoCodec) {
			videoCodecContext = avcodec_alloc_context3(videoCodec);
			if (videoCodecContext && avcodec_parameters_to_context(videoCodecContext, videoStream->codecpar) >= 0) {
				if (avcodec_open2(videoCodecContext, videoCodec, nullptr) >= 0) {
					qDebug() << "MediaPlayer::openFile(): Video stream found and decoder initialized.";
				}else {
					qWarning() << "MediaPlayer::openFile(): Could not open video codec.";
					avcodec_free_context(&videoCodecContext);
					videoStreamIndex = -1;
				}
			}
		}else {
			qWarning() << "MediaPlayer::openFile(): Could not find video decoder.";
			avcodec_free_context(&videoCodecContext);
			videoStreamIndex = -1;
		}else {
			qWarning() << "MediaPlayer::openFile(): Could not allocate video codec context.";
			videoStreamIndex = -1;
		}
	}

    // 查找并初始化音频解码器
    if (audioStreamIndex >= 0) {
       AVStream* audioStream = formatContext->streams[audioStreamIndex];
       const AVCodec* audioCodec = avcodec_find_decoder(audioStream->codecpar->codec_id);
       if (audioCodec) {
           audioCodecContext = avcodec_alloc_context3(audioCodec);
           if (audioCodecContext && avcodec_parameters_to_context(audioCodecContext, audioStream->codecpar) >= 0) {
               if (avcodec_open2(audioCodecContext, audioCodec, nullptr) >= 0) {
                   qDebug() << "MediaPlayer::openFile(): Audio stream found and decoder initialized.";
               }else {
                   qWarning() << "MediaPlayer::openFile(): Could not open audio codec.";
                   avcodec_free_context(&audioCodecContext);
                   audioStreamIndex = -1;
               }
           }else {
               qWarning() << "MediaPlayer::openFile(): Could not allocate audio codec context.";
               audioStreamIndex = -1;
           }
       }else {
           qWarning() << "MediaPlayer::openFile(): Could not find audio decoder.";
           audioStreamIndex = -1;
       }
	}

	// 检查是否找到有效的流
    if (videoStreamIndex < 0 && audioStreamIndex < 0) {
       QString errorMessage = QString("MediaPlayer::openFile() failed. No valid video or audio streams found in file: %1").arg(filename);
       qWarning() << errorMessage;
       avformat_free_context(&videoCodecContext);
       avformat_free_context(&audioCodecContext);
       avformat_close_input(&formatContext);
       emit error(errorMessage);
       return false;
    }

	m_formatContext = formatContext;
    m_videoCodecContext = videoCodecContext;
    m_audioCodecContext = audioCodecContext;
    m_videoStreamIndex = videoStreamIndex;
    m_audioStreamIndex = audioStreamIndex;
    m_position = 0;
    m_currentMedia = filename;

    if (m_formatContext->duration != AV_NOPTS_VALUE) {
		m_duration = m_formatContext->duration / AV_TIME_BASE;
	}else {
		m_duration = 0;
	}

	qDebug() << "MediaPlayer::openFile(): Successfully opened file:" << filename;

	emit mediaOpened(filename);
	if (m_duration > 0) {
		emit durationChanged(m_duration);
	}

	return true;
}

void MediaPlayer::seek(qint64 position)
{
	qDebug() << "MediaPlayer::seek() called with position:" << position;
	if (!m_formatContext) {
		QString errorMessage = "MediaPlayer::seek() failed. No media is loaded.";
		qWarning() << errorMessage;
		emit error(errorMessage);
		return;
	}

    if (position < 0) {
		position = 0;
	}

	if (m_duration > 0 && position > m_duration) {
		position = m_duration;
	}

	int64_t timestamp_us = position * 1000;
	int ret =  av_seek_frame(m_formatContext, -1, timestamp_us, AVSEEK_FLAG_BACKWARD);
	if (ret < 0) {
		QString errorMessage = "MediaPlayer::seek() failed. Could not seek to position.";
		char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        QString detailMessage = QString("FFmpeg error: %1 (%2)").arg(errorMessage).arg(errbuf);
        qWarning() << detailMessage;
        emit error(detailMessage);
        return;
	}

	if (m_videoCodecContext) {
        avcodec_flush_buffers(m_videoCodecContext);
    }

    if (m_audioCodecContext) {
        avcodec_flush_buffers(m_audioCodecContext);
    }

    qint64 oldPosition = m_position;
    m_position = position;
    qDebug() << "MediaPlayer: Seeked to position:" << position;
    if (oldPosition != m_position) {
        emit positionChanged(m_position);
    }
}

MediaPlayer::State MediaPlayer::state() const {
	return m_state;
}

bool MediaPlayer::isPlaying() const {
    return m_state == State::Playing;
}

QString MediaPlayer::currentMedia() const {
	return  m_currentMedia;
}

qint64 MediaPlayer::duration() const {
    return  m_duration;
}

qint64 MediaPlayer::position() const {
    return  m_position;
}

}
}