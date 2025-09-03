# AuroraStream

> ⚠️ **开发中**:
> 本项目目前正在积极开发中。功能可能不完整，API 可能会发生变化，文档可能缺失或不准确。使用时请自行承担风险。

---

A modular, enterprise-grade media streaming framework.

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

## 🌟 概述

**AuroraStream** 是一个功能强大且灵活的 C++ 媒体流框架，专为构建稳健的音视频应用程序而设计。其灵感来源于极光的美丽与动感，旨在跨平台提供流畅、高质量的媒体体验。

该项目采用现代 C++ (C++17)、Qt6 用于图形用户界面，并利用 FFmpeg 和 SDL2 的强大功能，为希望在其项目中集成高级媒体功能的开发人员提供了坚实的基础。

## 🎯 主要特性

*   **模块化架构**: UI、核心逻辑、媒体解码和渲染之间职责清晰分离。
*   **跨平台**: 使用 Qt 和标准库开发，具有广泛的兼容性（支持 Windows、Linux、macOS）。
*   **现代 C++ 设计**: 利用 C++17 特性，确保安全性、性能和表现力。
*   **可扩展核心**: 可轻松添加新的解码器、渲染器或 UI 组件。
*   **Qt 集成**: 原生 Qt 信号与槽机制，实现无缝的 GUI 交互。
*   **FFmpeg 后端**: 对各种媒体格式和编解码器提供强大的支持。
*   **SDL2 渲染 (计划中)**: 灵活的渲染管道（实现正在进行中）。

## 📦 依赖项

AuroraStream 依赖于几个强大的开源库：

*   **[Qt 6.x](https://www.qt.io/)** (Core, Widgets): 用于跨平台图形用户界面。
*   **[FFmpeg](https://ffmpeg.org/)** (libavformat, libavcodec, libavutil, libswscale, libswresample): 用于全面的媒体解复用、解码和过滤。
*   **[SDL2](https://www.libsdl.org/)**: (未来渲染/音频输出计划) 一个跨平台开发库，旨在提供对音频、键盘、鼠标、操纵杆和图形硬件的低级访问。

## 🛠️ 入门指南

以下说明将帮助您在本地计算机上获取并运行该项目的副本。

### 先决条件

请确保您已安装以下工具：

*   **C++ 编译器**: 符合 C++17 标准的编译器 (GCC 7+, Clang 5+, MSVC 2017+)。
*   **CMake** (>= 3.16): 构建系统生成器。
    *   安装地址: [https://cmake.org/download/](https://cmake.org/download/)
*   **Qt 6.x**: 下载并安装 Qt 库。
    *   官方安装程序: [https://www.qt.io/download](https://www.qt.io/download) (选择开源版本)
    *   选择组件: `Qt` > `Qt 6.x.x` > `MSVC 20xx 64-bit` (Windows) 或 `GCC 64-bit` (Linux) 或 `macOS`。
*   **FFmpeg 开发库**: 安装 FFmpeg 的开发头文件和库。
    *   **Linux (Ubuntu/Debian)**: `sudo apt install libavformat-dev libavcodec-dev libavutil-dev libswscale-dev libswresample-dev`
    *   **macOS (Homebrew)**: `brew install ffmpeg`
    *   **Windows**: 下载预编译的开发包 (例如，从 [https://github.com/BtbN/FFmpeg-Builds/releases](https://github.com/BtbN/FFmpeg-Builds/releases) 下载)。将其解压到一个已知位置 (例如 `C:\ffmpeg`)。
*   **SDL2 开发库**: 安装 SDL2 的开发头文件和库。
    *   **Linux (Ubuntu/Debian)**: `sudo apt install libsdl2-dev`
    *   **macOS (Homebrew)**: `brew install sdl2`
    *   **Windows**: 下载 MinGW 开发归档文件 (例如，从 [https://www.libsdl.org/download-2.0.php](https://www.libsdl.org/download-2.0.php) 下载)。将其解压到一个已知位置 (例如 `C:\SDL2`)。

### 构建

AuroraStream 使用 CMake 进行构建。以下是配置和构建的方法。

#### 方式一：命令行

1.  **克隆仓库 (如果使用 git):**
    ```bash
    git clone <repository_url> AuroraStream
    cd AuroraStream
    ```
    *(或者，如果不使用 git，请直接导航到您的项目目录)*

2.  **创建构建目录:**
    在源代码树之外进行构建是一种良好的实践。
    ```bash
    mkdir build
    cd build
    ```

3.  **使用 CMake 进行配置:**
    如果 Qt、FFmpeg 和 SDL2 没有安装在标准系统路径中，您可能需要告诉 CMake 它们的位置。请根据需要调整以下路径。

    *   **Linux/macOS:**
        如果库是通过包管理器安装的，CMake 通常会自动找到它们。
        ```bash
        # 基本配置 (如果依赖项在标准路径中)
        cmake ..

        # 或者，如果您需要显式指定路径:
        # PKG_CONFIG_PATH=/path/to/ffmpeg/lib/pkgconfig cmake .. # 对于 Linux/macOS 上的 FFmpeg
        # cmake .. -DCMAKE_PREFIX_PATH="/path/to/Qt/6.x.x/gcc_64;/path/to/SDL2" # 对于 Qt 和 SDL2
        ```

    *   **Windows (在 VS/MSVC 环境的开发者命令提示符或 MinGW/MSYS2 shell 中使用命令提示符或 PowerShell):**
        您通常需要将 CMake 指向安装位置。
        ```bash
        # 请将路径替换为您实际的安装目录
        cmake .. -G "MinGW Makefiles" ^
                 -DCMAKE_PREFIX_PATH="D:/Qt/6.5.0/mingw_64;D:/SDL2/SDL2-2.30.8/x86_64-w64-mingw32" ^
                 -DPKG_CONFIG_PATH="D:/ffmpeg/lib/pkgconfig"
        # 注意: 在 Windows 上的 CMake 中使用正斜杠 (/) 或双反斜杠 (\\)。
        # 对于 Visual Studio 生成器，使用 -G "Visual Studio 17 2022" (或类似) 代替 -G "MinGW Makefiles"
        ```

4.  **构建项目:**
    ```bash
    cmake --build . --config Release
    # 或者用于并行构建 (例如，使用 4 个核心):
    # cmake --build . --config Release --parallel 4
    ```
    可执行文件 (`AuroraStream` 或 `AuroraStream.exe`) 将在 `build` 目录中生成。

#### 方式二：使用 IDE (例如 CLion, Qt Creator, Visual Studio)

1.  **打开项目:** 使用您喜欢的 IDE 打开项目根目录下的 `CMakeLists.txt` 文件。
2.  **配置 CMake:** IDE 可能会提示您配置 CMake 项目。请确保工具链（编译器、调试器）设置正确。
3.  **设置 CMake 选项:** 在 IDE 的 CMake 设置/配置文件中，配置 Qt、FFmpeg 和 SDL2 的路径，类似于上述命令行方法。这通常涉及设置 `CMAKE_PREFIX_PATH` 和/或 `PKG_CONFIG_PATH`。
    *   **CLion 示例:**
        *   转到 `File` -> `Settings` -> `Build, Execution, Deployment` -> `CMake`。
        *   在您的 CMake 配置文件中，找到 `Environment` 或 `Cache variables`。
        *   添加/修改变量，例如:
            *   `CMAKE_PREFIX_PATH`: `D:/Qt/6.5.0/mingw_64;D:/SDL2/SDL2-2.30.8/x86_64-w64-mingw32` (Windows 示例)
            *   `PKG_CONFIG_PATH`: `D:/ffmpeg/lib/pkgconfig` (Windows 示例)
4.  **构建:** 使用 IDE 的标准构建命令 (通常是 `Build` -> `Build Project` 或锤子图标)。

### 运行

构建成功后，您可以从 `build` 目录运行 `AuroraStream` 可执行文件。

*   **Linux/macOS:**
    ```bash
    ./AuroraStream
    ```
*   **Windows:**
    ```cmd
    AuroraStream.exe
    ```

要播放媒体文件，您通常可以使用应用程序 GUI 中的“打开”按钮，或者如果已实现，可以将其作为命令行参数传递（请检查 `main.cpp`）。

## 📖 文档

*   **架构:** 请参阅 `docs/architecture/system_architecture.md` 了解项目设计的概述。
*   **API 参考:** (未来) 生成的 Doxygen 文档将放在 `docs/api/` 中。
*   **开发指南:** 请参阅 `docs/development/` 了解编码标准和贡献指南。

## 🧪 测试

(单元测试计划在未来的开发中。一旦测试框架集成完毕，说明将添加到这里。)

## 🤝 贡献

欢迎贡献！请阅读 `docs/development/coding_standards.md` 和 `docs/development/contributing.md` (待创建) 了解我们的行为准则以及提交拉取请求的流程。

## 📄 许可证

该项目根据 MIT 许可证授权 - 有关详细信息，请参阅 `LICENSE` 文件。

## 🙏 致谢

*   感谢 Qt、FFmpeg 和 SDL2 等社区提供了这些 essential 工具。