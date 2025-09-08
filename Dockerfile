# Dockerfile
# ----------------------------------------------------------------------------
# Dockerfile - AuroraStream 开发环境 Docker 配置文件
# ----------------------------------------------------------------------------

# --- 1. 选择基础镜像 ---
FROM ubuntu:22.04

# --- 2. 设置非交互式环境 ---
ARG DEBIAN_FRONTEND=noninteractive

# --- 3. 设置环境变量 ---
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

# --- 4. 更新包列表并安装依赖 ---
# 将所有 apt-get 命令合并到一个 RUN 指令中，以减少镜像层数。
# 注意：反斜杠 \ 必须在每一行的末尾（除了最后一行），并且注释不能直接跟在 \ 后面。
# --- 4.1 更换为阿里云镜像源以提高在中国大陆的下载速度 ---
RUN sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list && \
    sed -i 's/security.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list && \
    apt-get clean && \
    apt-get update

# --- 4.2 安装依赖项 ---
RUN apt-get install -y --no-install-recommends \
    # --- 基础构建工具 ---
    build-essential \
    cmake \
    pkg-config \
    # --- Qt6 开发库 ---
    qt6-base-dev \
    qt6-base-dev-tools \
    # --- FFmpeg 开发库 ---
    libavformat-dev \
    libavcodec-dev \
    libavutil-dev \
    libswscale-dev \
    libswresample-dev \
    # --- SDL2 开发库 ---
    libsdl2-dev \
    # --- 调试和开发工具 ---
    gdb \
    vim \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# --- 5. 设置工作目录 ---
WORKDIR /workspace

# --- 6. 定义容器启动时默认执行的命令 ---
CMD ["/bin/bash"]