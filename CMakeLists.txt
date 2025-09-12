cmake_minimum_required(VERSION 3.22)
project(AuroraStream VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 针对MSVC编译器的特殊设置
if(MSVC)
    add_compile_options(/Zc:__cplusplus)
    add_compile_options(/utf-8)
    add_compile_options(/permissive-)
    add_compile_options(/std:c++17)
endif()

# 设置输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 设置根目录变量
set(ROOT_DIR ${CMAKE_SOURCE_DIR})

find_package(PkgConfig REQUIRED)

# 容器环境特殊处理
if(DEFINED ENV{DOCKER_BUILD})
    message(STATUS "Running in Docker build environment")
    set(CMAKE_PREFIX_PATH "/usr/local/qt6" ${CMAKE_PREFIX_PATH})
    set(FFMPEG_DIR "/usr/local/ffmpeg")
endif()

# --- Qt ---
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# --- Vulkan ---
option(ENABLE_VULKAN "Enable Vulkan GPU acceleration" OFF)
if(ENABLE_VULKAN)
    find_package(Vulkan)
    if(NOT Vulkan_FOUND)
        message(WARNING "Vulkan not found - GPU acceleration disabled")
    endif()
endif()

# --- GTest ---
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    find_package(GTest)
    if(NOT GTest_FOUND)
        message(WARNING "GoogleTest not found - unit tests disabled")
    endif()
endif()

# --- FFmpeg ---
set(FFMPEG_COMPONENTS
        libavformat
        libavcodec
        libavutil
        libswscale
        libswresample
)

# 首先检查本地externals目录
set(LOCAL_FFMPEG_DIR "${CMAKE_SOURCE_DIR}/external/ffmpeg")
if(EXISTS ${LOCAL_FFMPEG_DIR})
    set(FFMPEG_INCLUDE_DIRS ${LOCAL_FFMPEG_DIR}/include)
    if(WIN32)
        set(FFMPEG_LIBRARIES
                ${LOCAL_FFMPEG_DIR}/lib/avformat.lib
                ${LOCAL_FFMPEG_DIR}/lib/avcodec.lib
                ${LOCAL_FFMPEG_DIR}/lib/avutil.lib
                ${LOCAL_FFMPEG_DIR}/lib/swscale.lib
                ${LOCAL_FFMPEG_DIR}/lib/swresample.lib
        )
    else()
        set(FFMPEG_LIBRARIES
                ${LOCAL_FFMPEG_DIR}/lib/libavformat.a
                ${LOCAL_FFMPEG_DIR}/lib/libavcodec.a
                ${LOCAL_FFMPEG_DIR}/lib/libavutil.a
                ${LOCAL_FFMPEG_DIR}/lib/libswscale.a
                ${LOCAL_FFMPEG_DIR}/lib/libswresample.a
        )
    endif()
    set(FFMPEG_FOUND TRUE)
    message(STATUS "Found local FFmpeg in externals directory")
else()
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
endif()

# 设置FFmpeg包含目录
if(FFMPEG_FOUND AND FFMPEG_INCLUDE_DIRS)
    include_directories(${FFMPEG_INCLUDE_DIRS})
endif()

# --- SDL2 ---
set(LOCAL_SDL2_DIR "${CMAKE_SOURCE_DIR}/external/sdl2")
if(EXISTS ${LOCAL_SDL2_DIR})
    set(SDL2_INCLUDE_DIRS ${LOCAL_SDL2_DIR}/include)
    if(WIN32)
        set(SDL2_LIBRARIES ${LOCAL_SDL2_DIR}/lib/libSDL2.dll.a ${LOCAL_SDL2_DIR}/lib/libSDL2main.a)
    else()
        set(SDL2_LIBRARIES ${LOCAL_SDL2_DIR}/lib/libSDL2.a)
    endif()
    set(SDL2_FOUND TRUE)
    message(STATUS "Found local SDL2 in externals directory")
else()
    find_package(SDL2)
    if(NOT SDL2_FOUND)
        find_library(SDL2_LIBRARY SDL2)
        if(SDL2_LIBRARY)
            set(SDL2_FOUND TRUE)
            set(SDL2_LIBRARIES ${SDL2_LIBRARY})
            message(STATUS "Found SDL2 library: ${SDL2_LIBRARIES}")
        else()
            message(WARNING "SDL2 library not found. Some features may be disabled.")
        endif()
    endif()
endif()

# 设置SDL2包含目录
if(SDL2_FOUND AND SDL2_INCLUDE_DIRS)
    include_directories(${SDL2_INCLUDE_DIRS})
endif()

# 启用自动MOC
set(CMAKE_AUTOMOC ON)

# 添加子目录
add_subdirectory(src)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# 创建可执行文件
add_executable(AuroraStream src/main.cpp)

# 在容器中设置rpath
if(DEFINED ENV{DOCKER_BUILD})
    set_target_properties(AuroraStream PROPERTIES
            INSTALL_RPATH "/usr/local/lib"
            BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

# 链接库
target_link_libraries(AuroraStream
        PRIVATE
        CoreModule
        UtilsModule
        MediaModule
        UIModule
        Qt6::Widgets
        Qt6::Core
        ${FFMPEG_LIBRARIES}
        ${SDL2_LIBRARIES}
)

# 确保所有Qt相关目标都启用了MOC
set_target_properties(CoreModule UtilsModule MediaModule UIModule
        PROPERTIES
        AUTOMOC ON
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

# 自动复制运行时依赖文件到构建目录
if(WIN32)
    # 设置部署选项
    option(AUTO_DEPLOY_RUNTIME "Automatically deploy runtime dependencies after build" ON)

    if(AUTO_DEPLOY_RUNTIME)
        # 创建部署脚本
        configure_file(
                ${CMAKE_SOURCE_DIR}/scripts/deploy_runtime_dependencies.cmake.in
                ${CMAKE_BINARY_DIR}/deploy_runtime_dependencies.cmake
                @ONLY
        )

        # 添加自定义目标来部署运行时依赖
        add_custom_target(deploy_runtime_dependencies
                COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/deploy_runtime_dependencies.cmake
                COMMENT "Deploying runtime dependencies to output directory"
                VERBATIM
        )

        # 在构建后自动部署依赖
        add_custom_command(TARGET AuroraStream POST_BUILD
                COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/deploy_runtime_dependencies.cmake
                COMMENT "Auto-deploying runtime dependencies"
        )

        message(STATUS "Runtime dependency auto-deployment enabled")
    else()
        message(STATUS "Runtime dependency auto-deployment disabled")
    endif()
endif()