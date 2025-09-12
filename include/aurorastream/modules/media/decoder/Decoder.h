/********************************************************************************
 * @file   : Decoder.cpp
 * @brief  : 实现 AuroraStream 媒体解码器模块。
 *
 * 本文件实现了 aurorastream::modules::media::decoder::Decoder 类，
 * 它提供了媒体解码器的基本功能，包括初始化、解码控制、状态管理等。
 *
 * @author : polarours
 * @date   : 2025/08/25
 ********************************************************************************/

#pragma once

#include <memory>
#include <string>
#include <functional>
#include "../../../AuroraStream.h"
#include <QtCore/QString>
#include <QtCore/QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace aurorastream {
namespace modules {
namespace media {
namespace decoder {

// 帧类型枚举
enum class FrameType {
    VIDEO,
    AUDIO
};

// 视频帧结构体
struct VideoFrame {
    FrameType type = FrameType::VIDEO;
    uint8_t* data[4];     // YUV数据指针
    int linesize[4];      // 每行字节数
    int width;
    int height;
    int64_t pts;          // 播放时间戳
    double duration;
};

// 音频帧结构体
struct AudioFrame {
    FrameType type = FrameType::AUDIO;
    uint8_t* data[8];     // 音频数据指针
    int samples;          // 样本数
    int channels;         // 声道数
    int sampleRate;       // 采样率
    int64_t pts;          // 播放时间戳
    double duration;
};

class Decoder {
public:
    enum class Type {
        VIDEO,
        AUDIO
    };

    explicit Decoder(Type type);
    virtual ~Decoder();

    // 初始化解码器
    bool init(AVCodecParameters* codecParams);

    // 发送数据包到解码器
    bool sendPacket(AVPacket* packet);

    // 接收解码后的帧
    bool receiveFrame(AVFrame* frame);

    // 刷新解码器缓冲区
    void flush();

    // 硬件加速支持
    bool enableHardwareAcceleration(const std::string& deviceType = "auto");

    // 获取解码器统计信息
    struct Statistics {
        uint64_t framesDecoded;
        uint64_t packetsReceived;
        double averageDecodeTime;
    };

    Statistics getStatistics() const;

    // 媒体控制接口
    bool open(const QString& uri);
    void close();
    void start();
    void pause();
    void stop();

    bool seek(qint64 position);

    // 状态查询
    bool isOpen() const;
    bool isPlaying() const;
    bool isPaused() const;
    bool isStopped() const;
    qint64 getDuration() const;
    qint64 getPosition() const;

    // 媒体信息查询
    bool hasVideo() const;
    bool hasAudio() const;
    
    // 获取下一帧数据
    std::shared_ptr<VideoFrame> getNextFrame();
    
    // 媒体信息
    int getVideoWidth() const;
    int getVideoHeight() const;
    double getVideoFrameRate() const;
    int getAudioSampleRate() const;
    int getAudioChannels() const;

protected:
    virtual bool initDecoder(AVCodecParameters* codecParams);
    virtual void cleanup();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace decoder
} // namespace media
} // namespace modules
} // namespace aurorastream