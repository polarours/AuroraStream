# 使用Ubuntu 22.04作为基础镜像
FROM ubuntu:22.04

# 安装必要的依赖项
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    qt6-base-dev \
    qt6-multimedia-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    && rm -rf /var/lib/apt/lists/*

# 设置工作目录
WORKDIR /app

# 复制项目文件到容器中
COPY . .

# 构建项目
RUN mkdir build && cd build && \
    cmake .. && \
    cmake --build . --config Release

# 运行应用程序
CMD ["./build/AuroraStream"]