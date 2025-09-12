# Core Module Configuration

set(CORE_MODULE_HEADERS
        ${ROOT_DIR}/include/aurorastream/core/MediaPlayer.h
)

set(CORE_MODULE_SOURCES
        MediaPlayer.cpp
)

# 创建 CoreModule 库
add_library(CoreModule STATIC
        ${CORE_MODULE_HEADERS}
        ${CORE_MODULE_SOURCES}
)

# 设置包含目录
target_include_directories(CoreModule PUBLIC
        $<BUILD_INTERFACE:${ROOT_DIR}/include/aurorastream/core>
        ${Qt6Core_INCLUDE_DIRS}
)

target_include_directories(CoreModule PRIVATE
        ${ROOT_DIR}/include
        ${FFMPEG_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(CoreModule
        PRIVATE
        Qt6::Core
        ${FFMPEG_LIBRARIES}
)