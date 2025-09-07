
set(MEDIA_SOURCES
    MediaPlayer.cpp
    MediaSource.cpp
)

set(MEDIA_HEADERS
    MediaPlayer.h
    MediaSource.h
)

add_library(MediaModule STATIC
    ${MEDIA_SOURCES}
    ${MEDIA_HEADERS}
)

target_link_libraries(MediaModule
    PRIVATE
        Qt6::Multimedia
)

target_include_directories(MediaModule
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)