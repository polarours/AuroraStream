cmake_minimum_required(VERSION 3.22)
project(AuroraStream VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 设置根目录变量
set(ROOT_DIR ${CMAKE_SOURCE_DIR})

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

# 尝试使用pkg-config查找FFmpeg
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(FFMPEG ${FFMPEG_COMPONENTS})
endif()

# 如果pkg-config找不到，尝试使用find_library
if(NOT FFMPEG_FOUND)
    find_library(AVFORMAT_LIBRARY avformat)
    find_library(AVCODEC_LIBRARY avcodec)
    find_library(AVUTIL_LIBRARY avutil)
    find_library(SWSCALE_LIBRARY swscale)
    find_library(SWRESAMPLE_LIBRARY swresample)

    if(AVFORMAT_LIBRARY AND AVCODEC_LIBRARY AND AVUTIL_LIBRARY)
        set(FFMPEG_FOUND TRUE)
        set(FFMPEG_LIBRARIES
                ${AVFORMAT_LIBRARY}
                ${AVCODEC_LIBRARY}
                ${AVUTIL_LIBRARY}
                ${SWSCALE_LIBRARY}
                ${SWRESAMPLE_LIBRARY}
        )
        message(STATUS "Found FFmpeg libraries: ${FFMPEG_LIBRARIES}")
    else()
        message(WARNING "FFmpeg libraries not found. Some features may be disabled.")
    endif()
endif()

# 设置FFmpeg包含目录
if(FFMPEG_FOUND AND FFMPEG_INCLUDE_DIRS)
    include_directories(${FFMPEG_INCLUDE_DIRS})
endif()

# --- SDL2 ---
find_package(SDL2)
if(NOT SDL2_FOUND)
    find_library(SDL2_LIBRARY SDL2)
    if(SDL2_LIBRARY)
        set(SDL2_FOUND TRUE)
        set(SDL2_LIBRARIES ${SDL2_LIBRARY})
        message(STATUS "Found SDL2 library: ${SDL2_LIBRARY}")
    else()
        message(WARNING "SDL2 library not found. Some features may be disabled.")
    endif()
endif()

# 设置SDL2包含目录
if(SDL2_FOUND AND SDL2_INCLUDE_DIRS)
    include_directories(${SDL2_INCLUDE_DIRS})
endif()

# 添加子目录
add_subdirectory(src)
add_subdirectory(tests)

# 创建可执行文件
add_executable(AuroraStream src/main.cpp)

# 链接库
target_link_libraries(AuroraStream
        PRIVATE
        UIModule
        MediaModule
        UtilsModule
        Qt6::Widgets
        Qt6::Core
        ${FFMPEG_LIBRARIES}
        ${SDL2_LIBRARIES}
)

# 设置包含目录
target_include_directories(AuroraStream PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${FFMPEG_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
        ${Qt6Core_INCLUDE_DIRS}
        ${Qt6Widgets_INCLUDE_DIRS}
)

# 安装目标
install(TARGETS AuroraStream
        RUNTIME DESTINATION bin
)

# 安装资源文件
install(DIRECTORY resources/ DESTINATION share/aurorastream/resources)