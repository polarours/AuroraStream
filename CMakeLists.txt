cmake_minimum_required(VERSION 3.22)
project(AuroraStream)

find_package(PkgConfig REQUIRED)

# --- Qt ---
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# --- FFmpeg ---
set(FFMPEG_COMPONENTS
        libavformat
        libavcodec
        libavutil
        libswscale
        libswresample
)
pkg_check_modules(FFMPEG REQUIRED ${FFMPEG_COMPONENTS})

# --- SDL2 ---
find_package(SDL2 REQUIRED)