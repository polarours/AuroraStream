# src/modules/media/CMakeLists.txt

set(MEDIA_MODULE_HEADERS
        ${ROOT_DIR}/include/aurorastream/modules/media/decoder/Decoder.h
        ${ROOT_DIR}/include/aurorastream/modules/media/player/Player.h
        ${ROOT_DIR}/include/aurorastream/modules/media/renderer/AudioRenderer.h
        ${ROOT_DIR}/include/aurorastream/modules/media/renderer/VideoRenderer.h
)

set(MEDIA_MODULE_SOURCES
        decoder/Decoder.cpp
        player/Player.cpp
        renderer/AudioRenderer.cpp
        renderer/VideoRenderer.cpp
)

# 创建 MediaModule 模块库
add_library(MediaModule STATIC
        ${MEDIA_MODULE_HEADERS}
        ${MEDIA_MODULE_SOURCES}
)

# 设置 Media 模块的包含目录
target_include_directories(MediaModule PUBLIC
        $<BUILD_INTERFACE:${ROOT_DIR}/include/aurorastream/modules/media>
)

# 设置 Media 模块的链接库
target_link_libraries(MediaModule
        PRIVATE
        ${FFMPEG_LIBRARIES}
        ${SDL2_LIBRARIES}
        Qt6::Core
        CoreModule
)

# 设置包含目录
target_include_directories(MediaModule PRIVATE
        ${FFMPEG_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
        ${ROOT_DIR}/include
)