# AuroraStream

> ⚠️ **Under Development**:
> This project is currently under active development. Features may be incomplete, APIs may change, and documentation may be lacking or inaccurate. Use at your own risk.

---

A modular, enterprise-grade media streaming framework.

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

## 🌟 Overview

**AuroraStream** is a powerful and flexible C++ media streaming framework designed for building robust audio and video applications. Inspired by the beauty and dynamism of the Aurora Borealis, it aims to deliver smooth, high-quality media experiences across platforms.

Built with modern C++ (C++17), Qt6 for the GUI, and leveraging the power of FFmpeg and SDL2, AuroraStream provides a solid foundation for developers looking to integrate advanced media capabilities into their projects.

## 🎯 Key Features

*   **Modular Architecture**: Clean separation of concerns between UI, Core Logic, Media Decoding, and Rendering.
*   **Cross-Platform**: Developed with Qt and standard libraries for broad compatibility (Windows, Linux, macOS planned).
*   **Modern C++ Design**: Utilizes C++17 features for safety, performance, and expressiveness.
*   **Extensible Core**: Easily add new decoders, renderers, or UI components.
*   **Qt Integration**: Native Qt signals and slots for seamless GUI interaction.
*   **FFmpeg Backend**: Robust support for a vast array of media formats and codecs.
*   **SDL2 Rendering (Planned)**: Flexible rendering pipeline (implementation in progress).

## 📦 Dependencies

AuroraStream relies on several powerful open-source libraries:

*   **[Qt 6.x](https://www.qt.io/)** (Core, Widgets): For the cross-platform Graphical User Interface.
*   **[FFmpeg](https://ffmpeg.org/)** (libavformat, libavcodec, libavutil, libswscale, libswresample): For comprehensive media demuxing, decoding, and filtering.
*   **[SDL2](https://www.libsdl.org/)**: (Planned for future rendering/audio output) A cross-platform development library designed to provide low level access to audio, keyboard, mouse, joystick, and graphics hardware.

## 🛠️ Getting Started

These instructions will get you a copy of the project up and running on your local machine.

### Prerequisites

Ensure you have the following tools installed:

*   **C++ Compiler**: A C++17 compliant compiler (GCC 7+, Clang 5+, MSVC 2017+).
*   **CMake** (>= 3.16): The build system generator.
    *   Install from: [https://cmake.org/download/](https://cmake.org/download/)
*   **Qt 6.x**: Download and install the Qt libraries.
    *   Official installer: [https://www.qt.io/download](https://www.qt.io/download) (Choose Open Source)
    *   Select components: `Qt` > `Qt 6.x.x` > `MSVC 20xx 64-bit` (Windows) or `GCC 64-bit` (Linux) or `macOS`.
*   **FFmpeg Development Libraries**: Install the development headers and libraries for FFmpeg.
    *   **Linux (Ubuntu/Debian)**: `sudo apt install libavformat-dev libavcodec-dev libavutil-dev libswscale-dev libswresample-dev`
    *   **macOS (Homebrew)**: `brew install ffmpeg`
    *   **Windows**: Download a pre-built development package (e.g., from [https://github.com/BtbN/FFmpeg-Builds/releases](https://github.com/BtbN/FFmpeg-Builds/releases)). Extract it to a known location (e.g., `C:\ffmpeg`).
*   **SDL2 Development Libraries**: Install the development headers and libraries for SDL2.
    *   **Linux (Ubuntu/Debian)**: `sudo apt install libsdl2-dev`
    *   **macOS (Homebrew)**: `brew install sdl2`
    *   **Windows**: Download the MinGW development archive (e.g., from [https://www.libsdl.org/download-2.0.php](https://www.libsdl.org/download-2.0.php)). Extract it to a known location (e.g., `C:\SDL2`).

### Building

AuroraStream uses CMake for its build process. Here's how to configure and build it.

#### Option 1: Command Line

1.  **Clone the Repository (if using git):**
    ```bash
    git clone <repository_url> AuroraStream
    cd AuroraStream
    ```
    *(Or simply navigate to your project directory if not using git)*

2.  **Create a Build Directory:**
    It's good practice to build outside the source tree.
    ```bash
    mkdir build
    cd build
    ```

3.  **Configure with CMake:**
    You may need to tell CMake where to find Qt, FFmpeg, and SDL2 if they are not in standard system locations. Adjust the paths below accordingly.

    *   **Linux/macOS:**
        If libraries are installed via package manager, CMake often finds them automatically.
        ```bash
        # Basic configuration (if dependencies are in standard paths)
        cmake ..

        # Or, if you need to specify paths explicitly:
        # PKG_CONFIG_PATH=/path/to/ffmpeg/lib/pkgconfig cmake .. # For FFmpeg on Linux/macOS
        # cmake .. -DCMAKE_PREFIX_PATH="/path/to/Qt/6.x.x/gcc_64;/path/to/SDL2" # For Qt and SDL2
        ```

    *   **Windows (using Command Prompt or PowerShell within the Developer Command Prompt for VS/MSVC environment, or MinGW/MSYS2 shell):**
        You typically need to point CMake to the installations.
        ```bash
        # Replace paths with your actual installation directories
        cmake .. -G "MinGW Makefiles" ^
                 -DCMAKE_PREFIX_PATH="D:/Qt/6.5.0/mingw_64;D:/SDL2/SDL2-2.30.8/x86_64-w64-mingw32" ^
                 -DPKG_CONFIG_PATH="D:/ffmpeg/lib/pkgconfig"
        # Note: Use forward slashes (/) or double backslashes (\\) in paths for CMake on Windows.
        # For Visual Studio generator, use -G "Visual Studio 17 2022" (or similar) instead of -G "MinGW Makefiles"
        ```

4.  **Build the Project:**
    ```bash
    cmake --build . --config Release
    # Or for parallel builds (e.g., using 4 cores):
    # cmake --build . --config Release --parallel 4
    ```
    The executable (`AuroraStream` or `AuroraStream.exe`) will be generated in the `build` directory.

#### Option 2: Using an IDE (e.g., CLion, Qt Creator, Visual Studio)

1.  **Open Project:** Open the `CMakeLists.txt` file located in the project root directory using your preferred IDE.
2.  **Configure CMake:** The IDE will likely prompt you to configure the CMake project. Ensure the toolchain (compiler, debugger) is correctly set.
3.  **Set CMake Options:** In your IDE's CMake settings/profile, configure the paths for Qt, FFmpeg, and SDL2, similar to the command-line method above. This often involves setting `CMAKE_PREFIX_PATH` and/or `PKG_CONFIG_PATH`.
    *   **CLion Example:**
        *   Go to `File` -> `Settings` -> `Build, Execution, Deployment` -> `CMake`.
        *   In your CMake profile, find `Environment` or `Cache variables`.
        *   Add/modify variables like:
            *   `CMAKE_PREFIX_PATH`: `D:/Qt/6.5.0/mingw_64;D:/SDL2/SDL2-2.30.8/x86_64-w64-mingw32` (Windows example)
            *   `PKG_CONFIG_PATH`: `D:/ffmpeg/lib/pkgconfig` (Windows example)
4.  **Build:** Use the IDE's standard build command (usually `Build` -> `Build Project` or a hammer icon).

### Running

After a successful build, you can run the `AuroraStream` executable from the `build` directory.

*   **Linux/macOS:**
    ```bash
    ./AuroraStream
    ```
*   **Windows:**
    ```cmd
    AuroraStream.exe
    ```

To play a media file, you can typically use the "Open" button within the application's GUI or potentially pass it as a command-line argument if implemented (check `main.cpp`).

## 📖 Documentation

*   **Architecture:** See `docs/architecture/system_architecture.md` for an overview of the project's design.
*   **API Reference:** (Future) Generated Doxygen documentation will be placed in `docs/api/`.
*   **Development Guides:** See `docs/development/` for coding standards and contribution guidelines.

## 🧪 Testing

(Unit tests are planned for future development. Instructions will be added here once the testing framework is integrated.)

## 🤝 Contributing

Contributions are welcome! Please read `docs/development/coding_standards.md` and `docs/development/contributing.md` (to be created) for details on our code of conduct and the process for submitting pull requests.

## 📄 License

This project is licensed under the MIT License - see the `LICENSE` file for details.

## 🙏 Acknowledgments

*   Thanks to the amazing communities behind Qt, FFmpeg, and SDL2 for providing these essential tools.