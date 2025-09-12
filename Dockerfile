# 基于官方Ubuntu镜像
FROM ubuntu:22.04

# 安装基础工具和构建依赖
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    cmake \
    git \
    libvulkan-dev \
    vulkan-validationlayers \
    libgtest-dev \
    qt6-base-dev \
    qt6-tools-dev \
    qt6-l10n-tools \
    && rm -rf /var/lib/apt/lists/*

# 手动安装GTest
RUN cd /usr/src/gtest && \
    cmake CMakeLists.txt && \
    make && \
    cp *.a /usr/lib

# 设置工作目录
WORKDIR /app

# 复制项目文件
COPY . .

# 配置构建目录
RUN mkdir -p build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Debug ..

# 构建项目
CMD cd build && make -j$(nproc)