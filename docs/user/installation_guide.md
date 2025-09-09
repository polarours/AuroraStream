# AuroraStream 播放器部署指南

## 本地部署

### 1、从GitHub上克隆本项目
```shell
git clone https://github.com/polarours/AuroraStream.git
```

### 2、编译
```shell
cd AuroraStream
mkdir build && cd build
make .
```

### 3、运行
```shell
./AuroraStream
```

## Docker容器化部署（推荐）

### 1、从GitHub上克隆本项目
```shell
git clone https://github.com/polarours/AuroraStream.git
```

### 2、创建Docker镜像
```shell
docker build -t aurorastream-dev:latest .
```

### 3、