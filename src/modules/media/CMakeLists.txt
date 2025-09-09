# src/modules/media/CMakeLists.txt

set(MEDIA_MODULE_HEADERS
        include/aurorastream/modules/media/decoder/Decoder.h
        include/aurorastream/modules/media/decoder/FfmpegDecoder.h
        include/aurorastream/modules/media/player/Player.h
        include/aurorastream/modules/media/renderer/Renderer.h
        include/aurorastream/modules/media/renderer/AudioRenderer.h
        include/aurorastream/modules/media/renderer/VideoRenderer.h
        include/aurorastream/modules/media/renderer/SDLRenderer.h
        include/aurorastream/modules/media/renderer/SDLAudioRenderer.h
        include/aurorastream/modules/media/renderer/SDLVideoRenderer.h
)

set(MEDIA_MODULE_SOURCES
        src/modules/media/decoder/Decoder.cpp
        src/modules/media/decoder/FfmpegDecoder.cpp
        src/modules/media/player/Player.cpp
        src/modules/media/renderer/Renderer.cpp
        src/modules/media/renderer/AudioRenderer.cpp
        src/modules/media/renderer/VideoRenderer.cpp
        src/modules/media/renderer/SDLRenderer.cpp
        src/modules/media/renderer/SDLAudioRenderer.cpp
        src/modules/media/renderer/SDLVideoRenderer.cpp
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

# 设置 Media 模块的链接目录
target_link_libraries(MediaModule
        ${FFMPEG_LIBRARIES}
        ${SDL2_LIBRARIES}
        Qt6::Core
        CoreModule
)