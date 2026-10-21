cmake_minimum_required(VERSION 3.16)
project(aurora_media LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(SWRESAMPLE REQUIRED libswresample)
pkg_check_modules(AVFILTER REQUIRED libavfilter)

add_library(aurora_media SHARED
    src/codec.cpp
    src/muxer.cpp
    src/filter.cpp
    src/pool.cpp
)

target_include_directories(aurora_media PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${AVCODEC_INCLUDE_DIRS}
    ${AVFORMAT_INCLUDE_DIRS}
    ${AVUTIL_INCLUDE_DIRS}
    ${SWRESAMPLE_INCLUDE_DIRS}
    ${AVFILTER_INCLUDE_DIRS}
)

target_link_libraries(aurora_media
    ${AVCODEC_LIBRARIES}
    ${AVFORMAT_LIBRARIES}
    ${AVUTIL_LIBRARIES}
    ${SWRESAMPLE_LIBRARIES}
    ${AVFILTER_LIBRARIES}
)

set_target_properties(aurora_media PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

install(TARGETS aurora_media
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES include/aurora_media.h DESTINATION include)
