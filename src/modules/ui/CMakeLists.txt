# UI Module Configuration

set(UI_MODULE_HEADERS
    include/aurorastream/modules/ui/MainWindow.h
    include/aurorastream/modules/ui/VideoWidget.h
    include/aurorastream/modules/ui/ControlPanel.h
)

set(UI_MODULE_SOURCES
    src/modules/ui/MainWindow.cpp
    src/modules/ui/VideoWidget.cpp
    src/modules/ui/ControlPanel.cpp
)

# 添加 UIModule 模块库
add_library(UIModule STATIC
    ${UI_MODULE_HEADERS}
    ${UI_MODULE_SOURCES}
)

# 设置 UIModule 模块库的包含路径
target_include_directories(UIModule
    $<BUILD_INTERFACE:${ROOT_DIR}/include/aurorastream/modules/ui>
)

# 链接 UIModule 模块库到 AuroraStream 应用程序
target_link_libraries(UIModule

)