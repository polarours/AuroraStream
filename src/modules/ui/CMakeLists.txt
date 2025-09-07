# UI Module Configuration

set(UI_SOURCES
    MainWindow.cpp
    VideoWidget.cpp
    ControlPanel.cpp
)

set(UI_HEADERS
    MainWindow.h
    VideoWidget.h
    ControlPanel.h
)

# Add library or module target
add_library(UIModule STATIC
    ${UI_SOURCES}
    ${UI_HEADERS}
)

# Link dependencies
target_link_libraries(UIModule
    PRIVATE
        Qt6::Widgets
        Qt6::Multimedia
)

# Include directories
target_include_directories(UIModule
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)