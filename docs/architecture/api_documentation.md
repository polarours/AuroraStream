# AuroraStream API 文档

## 概述

AuroraStream 提供了一套完整的 C++ API，用于媒体播放、流处理和用户界面控制。本文档详细描述了主要的 API 接口和使用方法。

## 核心 API

### 初始化与销毁

#### 初始化库
```cpp
/**
 * 初始化 AuroraStream 库
 * @return 成功返回 true，失败返回 false
 */
bool AuroraStream_Initialize();
```

#### 清理资源
```cpp
/**
 * 清理 AuroraStream 库资源
 */
void AuroraStream_Shutdown();
```

### 播放器控制

#### 创建播放器实例
```cpp
/**
 * 创建新的播放器实例
 * @return 播放器句柄，失败返回 nullptr
 */
AuroraPlayer* AuroraPlayer_Create();
```

#### 加载媒体文件
```cpp
/**
 * 加载媒体文件
 * @param player 播放器实例
 * @param file_path 文件路径
 * @return 成功返回 true，失败返回 false
 */
bool AuroraPlayer_LoadFile(AuroraPlayer* player, const char* file_path);
```

#### 播放控制
```cpp
// 开始播放
void AuroraPlayer_Play(AuroraPlayer* player);

// 暂停播放
void AuroraPlayer_Pause(AuroraPlayer* player);

// 停止播放
void AuroraPlayer_Stop(AuroraPlayer* player);

// 跳转到指定时间
void AuroraPlayer_Seek(AuroraPlayer* player, double timestamp);
```

### 状态查询

#### 获取播放状态
```cpp
/**
 * 获取当前播放状态
 * @return 状态枚举值
 */
enum PlayerState {
    STATE_STOPPED,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_BUFFERING
};

PlayerState AuroraPlayer_GetState(AuroraPlayer* player);
```

#### 获取媒体信息
```cpp
typedef struct {
    double duration;          // 总时长（秒）
    int width;               // 视频宽度
    int height;              // 视频高度
    double frame_rate;       // 帧率
    int audio_channels;      // 音频声道数
    int audio_sample_rate;   // 音频采样率
} MediaInfo;

MediaInfo AuroraPlayer_GetMediaInfo(AuroraPlayer* player);
```

### 音频控制

#### 音量控制
```cpp
// 设置音量 (0.0 - 1.0)
void AuroraPlayer_SetVolume(AuroraPlayer* player, float volume);

// 获取当前音量
float AuroraPlayer_GetVolume(AuroraPlayer* player);

// 静音切换
void AuroraPlayer_SetMute(AuroraPlayer* player, bool mute);
bool AuroraPlayer_IsMuted(AuroraPlayer* player);
```

### 视频控制

#### 画面调整
```cpp
// 设置显示比例
void AuroraPlayer_SetAspectRatio(AuroraPlayer* player, const char* ratio);

// 设置旋转角度
void AuroraPlayer_SetRotation(AuroraPlayer* player, int degrees);

// 设置缩放模式
enum ScaleMode {
    SCALE_NONE,
    SCALE_STRETCH,
    SCALE_FIT,
    SCALE_FILL
};
void AuroraPlayer_SetScaleMode(AuroraPlayer* player, ScaleMode mode);
```

### 字幕控制

#### 字幕管理
```cpp
// 加载外部字幕文件
bool AuroraPlayer_LoadSubtitle(AuroraPlayer* player, const char* subtitle_path);

// 设置字幕显示状态
void AuroraPlayer_EnableSubtitle(AuroraPlayer* player, bool enable);

// 设置字幕延迟
void AuroraPlayer_SetSubtitleDelay(AuroraPlayer* player, double delay);
```

### 事件回调

#### 事件注册
```cpp
typedef void (*EventCallback)(AuroraPlayer* player, int event_type, void* user_data);

// 事件类型定义
enum EventType {
    EVENT_PLAYBACK_STARTED,
    EVENT_PLAYBACK_ENDED,
    EVENT_BUFFERING_START,
    EVENT_BUFFERING_END,
    EVENT_ERROR_OCCURRED,
    EVENT_SEEK_COMPLETED
};

// 注册事件回调
void AuroraPlayer_SetEventCallback(AuroraPlayer* player, EventCallback callback, void* user_data);
```

## 高级功能 API

### 硬件加速

#### 硬件加速控制
```cpp
// 启用/禁用硬件加速
void AuroraPlayer_EnableHardwareAcceleration(AuroraPlayer* player, bool enable);
bool AuroraPlayer_IsHardwareAccelerationEnabled(AuroraPlayer* player);

// 获取支持的硬件加速器列表
const char** AuroraPlayer_GetSupportedAccelerators(int* count);
```

### 流处理

#### 网络流支持
```cpp
// 加载网络流
bool AuroraPlayer_LoadStream(AuroraPlayer* player, const char* stream_url);

// 设置缓冲大小
void AuroraPlayer_SetBufferSize(AuroraPlayer* player, size_t buffer_size);
```

### 滤镜效果

#### 视频滤镜
```cpp
// 应用视频滤镜
enum VideoFilter {
    FILTER_NONE,
    FILTER_GRAYSCALE,
    FILTER_SEPIA,
    FILTER_INVERT
};
void AuroraPlayer_ApplyVideoFilter(AuroraPlayer* player, VideoFilter filter);
```

#### 音频均衡器
```cpp
// 设置均衡器参数
typedef struct {
    float preamp;
    float bands[10]; // 10-band equalizer
} EqualizerSettings;

void AuroraPlayer_SetEqualizer(AuroraPlayer* player, const EqualizerSettings* settings);
```

## 工具函数

### 版本信息
```cpp
// 获取库版本
const char* AuroraStream_GetVersion();

// 获取编译信息
const char* AuroraStream_GetBuildInfo();
```

### 错误处理
```cpp
// 获取错误信息
const char* AuroraPlayer_GetLastError(AuroraPlayer* player);

// 清除错误状态
void AuroraPlayer_ClearError(AuroraPlayer* player);
```

## 使用示例

### 基本播放示例
```cpp
#include <aurorastream/player.h>

int main() {
    // 初始化库
    if (!AuroraStream_Initialize()) {
        return -1;
    }

    // 创建播放器
    AuroraPlayer* player = AuroraPlayer_Create();
    if (!player) {
        AuroraStream_Shutdown();
        return -1;
    }

    // 加载文件
    if (!AuroraPlayer_LoadFile(player, "video.mp4")) {
        printf("Error: %s\n", AuroraPlayer_GetLastError(player));
        AuroraPlayer_Destroy(player);
        AuroraStream_Shutdown();
        return -1;
    }

    // 开始播放
    AuroraPlayer_Play(player);

    // 等待播放完成
    while (AuroraPlayer_GetState(player) != STATE_STOPPED) {
        // 处理事件或等待
    }

    // 清理资源
    AuroraPlayer_Destroy(player);
    AuroraStream_Shutdown();
    return 0;
}
```

### 事件处理示例
```cpp
void event_callback(AuroraPlayer* player, int event_type, void* user_data) {
    switch (event_type) {
        case EVENT_PLAYBACK_STARTED:
            printf("Playback started\n");
            break;
        case EVENT_PLAYBACK_ENDED:
            printf("Playback ended\n");
            break;
        case EVENT_ERROR_OCCURRED:
            printf("Error: %s\n", AuroraPlayer_GetLastError(player));
            break;
    }
}

// 注册回调
AuroraPlayer_SetEventCallback(player, event_callback, nullptr);
```

## 编译链接

### 编译选项
```cmake
find_package(AuroraStream REQUIRED)
target_link_libraries(your_target AuroraStream::AuroraStream)
```

### 依赖库
- pthread
- dl
- GLFW3
- GLEW
- OpenAL
- avcodec
- avformat
- avutil
- swscale

## 许可证

AuroraStream API 遵循 MIT 许可证。详细条款请参考 LICENSE 文件。

## 支持与反馈

- 文档问题: 提交 GitHub Issue
- API 建议: 使用功能请求模板
- Bug 报告: 提供详细的重现步骤