#ifndef AURORASTREAM_H
#define AURORASTREAM_H

#include <string>
#include <memory>
#include <cstdint>
#include <QtCore/QString>
#include <QtCore/QDebug>

// 版本定义
#define AURORASTREAM_VERSION_MAJOR 1
#define AURORASTREAM_VERSION_MINOR 0
#define AURORASTREAM_VERSION_PATCH 0
#define AURORASTREAM_VERSION_STR "1.0.0"

// 跨平台导出宏
#if defined(AURORASTREAM_SHARED_LIB)
    #if defined(_WIN32) || defined(__WIN32__)
        #ifdef AURORASTREAM_EXPORTS
            #define AURORASTREAM_API __declspec(dllexport)
        #else
            #define AURORASTREAM_API __declspec(dllimport)
        #endif
    #else
        #define AURORASTREAM_API __attribute__((visibility("default")))
    #endif
#else
    #define AURORASTREAM_API
#endif

namespace aurorastream {

// 日志级别
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

// 媒体状态
enum class MediaState {
    IDLE,
    PLAYING,
    PAUSED,
    STOPPED,
    BUFFERING,
    ERROR
};

// 媒体类型
enum class MediaType {
    VIDEO,
    AUDIO,
    IMAGE,
    STREAM
};

// 播放质量
enum class QualityLevel {
    LOW,
    MEDIUM,
    HIGH,
    ULTRA_HD,
    ADAPTIVE
};

// 错误代码
enum class ErrorCode {
    SUCCESS = 0,
    FILE_NOT_FOUND = 100,
    NETWORK_ERROR = 200,
    DECODE_ERROR = 300,
    INVALID_STATE = 400,
    UNSUPPORTED_FORMAT = 500
};

// 核心媒体控制接口
class AURORASTREAM_API MediaController {
public:
    virtual ~MediaController() = default;

    // 播放控制
    virtual bool load(const std::string& uri) = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void seek(int64_t position) = 0;

    // 状态查询
    virtual MediaState getState() const = 0;
    virtual int64_t getPosition() const = 0;
    virtual int64_t getDuration() const = 0;
    virtual QString getCurrentMedia() const = 0;
    virtual std::string getCurrentUrl() const = 0;

    // 播放设置
    virtual void setVolume(float volume) = 0;
    virtual float getVolume() const = 0;
    virtual void setLoop(bool loop) = 0;
    virtual bool getLoop() const = 0;
};

// 工具方法
class AURORASTREAM_API Utils {
public:
    static std::string getVersion() {
        return AURORASTREAM_VERSION_STR;
    }

    static bool isFormatSupported(const std::string& format);
    static bool isFeatureAvailable(const std::string& feature);
};

// 创建媒体控制器工厂方法
AURORASTREAM_API std::unique_ptr<MediaController> createMediaController();

} // namespace aurorastream

#endif // AURORASTREAM_H