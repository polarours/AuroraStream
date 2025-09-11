# AuroraStream 模块设计

## 架构概述

AuroraStream 是一个基于 Qt、FFmpeg 和 SDL 的跨平台媒体播放器。采用模块化设计，整体架构分为核心层、媒体处理层和用户界面层。

## 核心模块

### 1. 媒体播放器核心 (MediaPlayer)

#### 职责
- 媒体文件加载和解析
- FFmpeg 格式上下文管理
- 音视频流识别和解码器初始化
- 播放状态管理

#### 主要功能
```cpp
class MediaPlayer : public QObject {
    Q_OBJECT
    
public:
    enum class State { Stopped, Playing, Paused };
    
    // 播放控制
    void play();
    void pause();
    void stop();
    void seek(qint64 position);
    
    // 媒体文件操作
    bool openFile(const QString& fileName);
    
    // 状态查询
    State getState() const;
    QString getCurrentMedia() const;
    qint64 getDuration() const;
    qint64 getPosition() const;
    
signals:
    void stateChanged(State state);
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void mediaOpened(const QString& fileName);
    void error(const QString& errorMessage);
};
```

### 2. 播放器模块 (Player)

#### 职责
- 高层播放器状态机管理
- 解码器和渲染器协调
- 播放进度和时长管理

#### 状态机设计
```cpp
class Player : public QObject {
    Q_OBJECT
    
public:
    enum class State {
        Stopped, Opening, Opened, 
        Starting, Playing, Pausing, Paused,
        Stopping, Seeking, Closing, Error
    };
    
    // 组件设置
    void setDecoder(std::unique_ptr<Decoder> decoder);
    void setRenderer(std::unique_ptr<Renderer> renderer);
    
    // 播放控制
    bool open(const QString& uri);
    bool play();
    bool pause();
    bool stop();
    bool seek(qint64 position);
    bool close();
};
```

## 媒体处理模块

### 3. 解码器模块 (Decoder)

#### 职责
- FFmpeg 解码器封装
- 音视频帧解码
- 媒体信息提取

#### 接口设计
```cpp
class Decoder {
public:
    virtual ~Decoder() = default;
    
    virtual bool open(const QString& uri) = 0;
    virtual bool play() = 0;
    virtual bool pause() = 0;
    virtual bool stop() = 0;
    virtual bool seek(qint64 position) = 0;
    virtual bool close() = 0;
    
    virtual int getVideoWidth() const = 0;
    virtual int getVideoHeight() const = 0;
    virtual double getVideoFrameRate() const = 0;
    virtual int getAudioSampleRate() const = 0;
    virtual int getAudioChannels() const = 0;
};
```

### 4. 渲染器模块 (Renderer)

#### 职责
- SDL 视频渲染
- SDL 音频输出
- 帧同步和显示

#### 接口设计
```cpp
class Renderer {
public:
    virtual ~Renderer() = default;
    
    virtual void renderVideoFrame(const VideoFrame& frame) = 0;
    virtual void renderAudioFrame(const AudioFrame& frame) = 0;
};
```

### 5. SDL 渲染器实现

#### 视频渲染器
```cpp
class SDLVideoRenderer : public VideoRenderer {
public:
    bool initialize(void* windowHandle) override;
    void renderFrame(const VideoFrame& frame) override;
    void resize(int width, int height) override;
};
```

#### 音频渲染器
```cpp
class SDLAudioRenderer : public AudioRenderer {
public:
    bool initialize(const AudioConfig& config) override;
    void playSamples(const AudioSamples& samples) override;
    void pause() override;
    void resume() override;
    void setVolume(float volume) override;
};
```

## 用户界面模块

### 6. 主窗口 (MainWindow)

#### 职责
- 应用程序主界面
- 菜单和工具栏管理
- 窗口布局控制

#### 类设计
```cpp
class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    MainWindow(QWidget* parent = nullptr);
    
private:
    void setupUi();
    void setupConnections();
    
    MediaPlayer* m_mediaPlayer;
    VideoWidget* m_videoWidget;
    ControlPanel* m_controlPanel;
};
```

### 7. 视频控件 (VideoWidget)

#### 职责
- 视频显示区域
- 鼠标事件处理
- 渲染表面提供

### 8. 控制面板 (ControlPanel)

#### 职责
- 播放控制按钮
- 进度条显示和控制
- 音量调节

## 数据流设计

### 媒体处理流水线

```
媒体文件 → FFmpeg 解复用 → 音视频分离 → FFmpeg 解码 → SDL 渲染
    │           │              │             │           │
    │           │              │             │           └→ 视频显示 (SDL)
    │           │              │             └→ 音频解码 (FFmpeg)
    │           │              └→ 流信息提取
    │           └→ 格式探测 (FFmpeg)
    └→ 文件/网络 IO
```

### 线程模型

#### 主线程 (GUI 线程)
- Qt 事件处理
- 用户界面更新
- 播放状态管理

#### 解码线程
- FFmpeg 音视频解码
- 帧队列管理
- 同步控制

#### 渲染线程
- SDL 视频渲染
- SDL 音频输出
- 显示刷新

## 依赖管理

### 核心依赖
- **Qt 6**: 图形用户界面框架
- **FFmpeg**: 媒体解复用和解码
- **SDL2**: 音视频渲染输出

### 平台支持
- Windows: Win32 API + DirectX 支持
- Linux: X11/Wayland + OpenGL 支持
- macOS: Cocoa + Metal 支持

## 错误处理

### 错误类型
```cpp
enum class ErrorType {
    FileNotFound,
    DecoderInitFailed,
    RendererInitFailed,
    NetworkTimeout,
    UnsupportedFormat
};
```

### 异常处理
- Qt 信号槽机制用于错误通知
- FFmpeg 错误码转换和本地化
- 资源泄漏防护和自动清理

## 性能优化

### 内存管理
- FFmpeg 帧对象重用
- 智能指针资源管理
- 零拷贝帧传递

### 渲染优化
- SDL 纹理缓存
- 硬件加速支持
- 异步渲染流水线

这个模块设计基于实际的 AuroraStream 项目结构，提供了清晰的职责划分和可扩展的架构设计。