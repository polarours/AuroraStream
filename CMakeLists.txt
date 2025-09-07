cmake_minimum_required(VERSION 3.5)

project(AuroraStream VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia)

add_executable(AuroraStream
    src/main.cpp
    src/modules/ui/MainWindow.cpp
    src/modules/ui/MainWindow.h
    src/modules/ui/VideoWidget.cpp
    src/modules/ui/VideoWidget.h
    src/modules/ui/ControlPanel.cpp
    src/modules/ui/ControlPanel.h
)

target_link_libraries(AuroraStream
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
)