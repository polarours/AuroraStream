# Utils Module Configuration

set(UTILS_MODULE_HEADERS
        ${ROOT_DIR}/include/aurorastream/utils/Logger.h
        ${ROOT_DIR}/include/aurorastream/utils/ConfigManager.h
)

set(UTILS_MODULE_SOURCES
        Logger.cpp
        ConfigManager.cpp
)

# 创建 UtilsModule 库
add_library(UtilsModule STATIC
        ${UTILS_MODULE_HEADERS}
        ${UTILS_MODULE_SOURCES}
)

# 设置包含目录
target_include_directories(UtilsModule PUBLIC
        $<BUILD_INTERFACE:${ROOT_DIR}/include>
        ${Qt6Core_INCLUDE_DIRS}
)

target_include_directories(UtilsModule PRIVATE
        ${ROOT_DIR}/include
)

# 链接库
target_link_libraries(UtilsModule
        PRIVATE
        Qt6::Core
)