# CMakeLists.txt
cmake_minimum_required(VERSION 3.22)
project(AuroraStream)

# --- 查找依赖项 ---
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
find_package(PkgConfig REQUIRED)

# --- FFmpeg ---
# 在 Docker 容器中，这些库安装在标准路径，pkg_check_modules 应该能自动找到
set(FFMPEG_COMPONENTS
        libavformat
        libavcodec
        libavutil
        libswscale
        libswresample
)
pkg_check_modules(FFMPEG REQUIRED ${FFMPEG_COMPONENTS})
# 移除 FFMPEG_SEARCH_PATHS 设置

# --- SDL2 ---
# 在 Docker 容器中，SDL2 也安装在标准路径，find_package 应该能自动找到
find_package(SDL2 REQUIRED)
# 移除 SDL2_SEARCH_PATHS 设置