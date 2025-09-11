# AuroraStream 播放器安装指南

## 系统要求

- **操作系统**: Windows 10/11, Linux (Ubuntu 18.04+), macOS 10.15+
- **处理器**: 双核 2.0 GHz 或更高
- **内存**: 4GB RAM (推荐 8GB)
- **显卡**: 支持 OpenGL 3.3+
- **存储空间**: 至少 500MB 可用空间

## 本地部署

### 1. 从GitHub克隆项目
```shell
git clone https://github.com/polarours/AuroraStream.git
cd AuroraStream
```

### 2. 安装依赖
```shell
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential cmake libglfw3-dev libglew-dev libopenal-dev

# macOS
brew install cmake glfw glew openal-soft

# Windows
# 需要安装 Visual Studio 2019+ 和 vcpkg
```

### 3. 编译项目
```shell
mkdir build && cd build
cmake ..
make -j4
```

### 4. 运行程序
```shell
./AuroraStream
```

## Docker容器化部署（推荐）

### 1. 克隆项目
```shell
git clone https://github.com/polarours/AuroraStream.git
cd AuroraStream
```

### 2. 构建Docker镜像
```shell
docker build -t aurorastream:latest .
```

### 3. 运行容器
```shell
docker run -d \
  --name aurorastream \
  -p 8080:8080 \
  -v /path/to/media:/app/media \
  aurorastream:latest
```

### 4. 访问应用
打开浏览器访问: http://localhost:8080

## 包管理器安装

### Ubuntu Snap
```shell
sudo snap install aurorastream
```

### macOS Homebrew
```shell
brew install aurorastream
```

### Windows Chocolatey
```shell
choco install aurorastream
```

## 验证安装

安装完成后，运行以下命令验证：
```shell
AuroraStream --version
```

应该显示类似：`AuroraStream v1.0.0`

## 故障排除

### 常见问题
1. **编译错误**: 确保所有依赖库已正确安装
2. **运行时报错**: 检查显卡驱动是否支持 OpenGL
3. **Docker容器无法启动**: 检查端口是否被占用

### 获取帮助
- 查看详细日志: `AuroraStream --verbose`
- 访问项目Wiki: https://github.com/polarours/AuroraStream/wiki
- 提交Issue: https://github.com/polarours/AuroraStream/issues