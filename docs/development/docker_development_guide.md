# Docker 开发指南

## 概述

本文档介绍如何使用 Docker 进行 AuroraStream 的开发和测试，提供一致的开发环境。

## 快速开始

### 前提条件
- Docker Desktop 或 Docker Engine
- Git

### 克隆项目
```shell
git clone https://github.com/polarours/AuroraStream.git
cd AuroraStream
```

## 开发环境

### 开发镜像构建
```dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential cmake git \
    libglfw3-dev libglew-dev libopenal-dev \
    libavcodec-dev libavformat-dev libavutil-dev libswscale-dev

WORKDIR /app
COPY . .

RUN mkdir build && cd build && \
    cmake .. && make -j$(nproc)
```

### 构建和运行
```shell
# 构建镜像
docker build -t aurorastream-dev .

# 运行开发容器
docker run -it --rm \
  -v $(pwd):/app \
  -w /app \
  aurorastream-dev \
  /bin/bash
```

## 开发工作流

### 交互式开发
```shell
# 在容器内构建
cd build
cmake ..
make -j4

# 运行程序
./AuroraStream
```

### 测试执行
```shell
# 运行单元测试
./test/unit_tests

# 运行集成测试
./test/integration_tests
```

## Docker Compose

### docker-compose.yml
```yaml
version: '3.8'

services:
  dev:
    build: .
    volumes:
      - .:/app
    working_dir: /app
    command: sleep infinity
```

### 使用 Compose
```shell
# 启动环境
docker-compose up -d

# 进入容器
docker-compose exec dev bash
```

## 调试和分析

### GDB 调试
```shell
docker run -it --rm \
  -v $(pwd):/app \
  --cap-add=SYS_PTRACE \
  aurorastream-dev \
  gdb ./build/AuroraStream
```

### 性能分析
```shell
# 使用 perf 分析
docker run --rm \
  --privileged \
  -v $(pwd):/app \
  aurorastream-dev \
  perf record -g ./build/AuroraStream
```

## 生产构建

### 多阶段构建
```dockerfile
FROM ubuntu:22.04 AS builder
# 构建步骤同上

FROM ubuntu:22.04
RUN apt-get update && apt-get install -y \
    libglfw3 libglew2.1 libopenal1 \
    libavcodec58 libavformat58 libavutil56 libswscale5

COPY --from=builder /app/build/AuroraStream /app/
WORKDIR /app
CMD ["./AuroraStream"]
```

## 网络配置

### 开发网络
```yaml
networks:
  aurora-net:
    driver: bridge

services:
  app:
    networks:
      - aurora-net
    ports:
      - "8080:8080"
```

## 数据管理

### 持久化存储
```shell
# 创建数据卷
docker volume create aurora-data

# 使用数据卷
docker run -v aurora-data:/app/data aurorastream-dev
```

## 最佳实践

### 开发建议
1. 使用 .dockerignore 排除不必要的文件
2. 定期清理未使用的镜像和容器
3. 使用 Docker Compose 管理多服务环境

### 性能优化
- 使用构建缓存减少重建时间
- 合理使用多阶段构建减小镜像大小
- 配置适当的资源限制

## 故障排除

### 常见问题
1. **权限问题**: 使用适当的用户权限
2. **网络问题**: 检查防火墙和端口设置
3. **性能问题**: 调整资源分配

### 获取帮助
- 查看 Docker 文档
- 检查项目 Wiki
- 提交 GitHub Issue

## 更新日志

### 版本记录
- v1.0.0: 初始版本发布
- v1.1.0: 添加多架构支持
- v1.2.0: 优化构建性能

这个指南提供了完整的 Docker 开发环境配置，支持高效的开发和测试工作流。