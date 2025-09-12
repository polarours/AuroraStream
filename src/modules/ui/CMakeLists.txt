# UI Module Configuration

set(UI_MODULE_HEADERS
        ${ROOT_DIR}/include/aurorastream/modules/ui/MainWindow.h
)

set(UI_MODULE_SOURCES
        MainWindow.cpp
)

# 添加 UIModule 模块库
add_library(UIModule STATIC
        ${UI_MODULE_HEADERS}
        ${UI_MODULE_SOURCES}
)

# 设置 UIModule 模块库的包含路径和链接
target_include_directories(UIModule
        PUBLIC
        $<BUILD_INTERFACE:${ROOT_DIR}/include/aurorastream/modules/ui>
        PRIVATE
        ${ROOT_DIR}/include
        ${Qt6Widgets_INCLUDE_DIRS}
        ${Qt6Core_INCLUDE_DIRS}
)

target_link_libraries(UIModule
        PRIVATE
        Qt6::Widgets
        Qt6::Core
        CoreModule
        MediaModule
)