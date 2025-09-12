@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion
echo ====================================
echo AuroraStream 运行时依赖部署工具
echo ====================================
echo.

REM 检查是否在项目根目录中运行
if not exist "CMakeLists.txt" (
    echo 错误：请在项目根目录运行此脚本
    echo 当前目录：%CD%
    pause
    exit /b 1
)

REM 自动检测构建目录
set BUILD_DIR=
if exist "build\bin" (
    set BUILD_DIR=build
    echo 检测到构建目录: build
) else if exist "cmake-build-debug\bin" (
    set BUILD_DIR=cmake-build-debug
    echo 检测到构建目录: cmake-build-debug
) else if exist "cmake-build-release\bin" (
    set BUILD_DIR=cmake-build-release
    echo 检测到构建目录: cmake-build-release
) else if exist "cmake-build-debug-mingw-msys2\bin" (
    set BUILD_DIR=cmake-build-debug-mingw-msys2
    echo 检测到构建目录: cmake-build-debug-mingw-msys2
) else (
    echo 错误：未找到构建目录，请先编译项目
    echo 支持的构建目录: build, cmake-build-debug, cmake-build-release, cmake-build-debug-mingw-msys2
    pause
    exit /b 1
)

set BIN_DIR=%BUILD_DIR%\bin
echo 目标部署目录: %BIN_DIR%
echo.

REM 检查可执行文件是否存在
if not exist "%BIN_DIR%\AuroraStream.exe" (
    echo 警告：未找到 AuroraStream.exe，请确认项目已成功编译
    echo 继续部署依赖文件...
    echo.
)

echo 开始自动部署运行时依赖...
echo.

REM 1. 部署 FFmpeg DLL 文件
echo [1/4] 部署 FFmpeg 依赖...
if exist "external\ffmpeg\bin" (
    copy "external\ffmpeg\bin\*.dll" "%BIN_DIR%\" >nul 2>&1
    if !errorlevel! equ 0 (
        echo ✓ FFmpeg DLL 文件已复制
    ) else (
        echo ✗ FFmpeg DLL 复制失败
    )
) else (
    echo ✗ 未找到 external\ffmpeg\bin 目录
)

REM 2. 部署 SDL2 DLL 文件
echo [2/4] 部署 SDL2 依赖...
if exist "external\sdl2\bin\SDL2.dll" (
    copy "external\sdl2\bin\SDL2.dll" "%BIN_DIR%\" >nul 2>&1
    if !errorlevel! equ 0 (
        echo ✓ SDL2.dll 已复制
    ) else (
        echo ✗ SDL2.dll 复制失败
    )
) else (
    echo ✗ 未找到 external\sdl2\bin\SDL2.dll
)

REM 3. 自动检测并部署 Qt 运行时
echo [3/4] 部署 Qt 运行时...
call :deploy_qt_auto

REM 4. 部署 MinGW 运行时
echo [4/4] 部署 MinGW 运行时...
call :deploy_mingw_runtime

goto deploy_complete

:deploy_qt_auto
REM 尝试多种方式检测Qt安装路径
set QT_BIN=

REM 方法1: 通过qt-cmake检测
for /f "tokens=*" %%i in ('where qt-cmake 2^>nul') do (
    for %%j in ("%%i") do (
        set QT_CMAKE_DIR=%%~dpj
        set QT_BIN=!QT_CMAKE_DIR!..\bin
        if exist "!QT_BIN!\Qt6Core.dll" (
            echo 通过 qt-cmake 检测到Qt路径: !QT_BIN!
            goto deploy_qt_files
        )
    )
)

REM 方法2: 检查常见Qt安装路径
set QT_COMMON_PATHS=(
    "C:\Qt\6.8.0\mingw_64\bin"
    "C:\Qt\6.7.0\mingw_64\bin"
    "C:\Qt\6.6.0\mingw_64\bin"
    "C:\Qt\6.5.0\mingw_64\bin"
    "D:\Qt\6.8.0\mingw_64\bin"
    "D:\Qt\6.7.0\mingw_64\bin"
    "D:\Qt\6.6.0\mingw_64\bin"
    "D:\Qt\6.5.0\mingw_64\bin"
)

for %%P in %QT_COMMON_PATHS% do (
    if exist "%%P\Qt6Core.dll" (
        set QT_BIN=%%P
        echo 在常见路径检测到Qt: !QT_BIN!
        goto deploy_qt_files
    )
)

REM 方法3: 通过环境变量检测
if defined QTDIR (
    if exist "%QTDIR%\bin\Qt6Core.dll" (
        set QT_BIN=%QTDIR%\bin
        echo 通过环境变量检测到Qt: !QT_BIN!
        goto deploy_qt_files
    )
)

echo ✗ 未能自动检测到Qt安装路径
echo   请手动复制以下Qt文件到 %BIN_DIR%:
echo   - Qt6Core.dll
echo   - Qt6Gui.dll
echo   - Qt6Widgets.dll
echo   - platforms\qwindows.dll (需要创建platforms文件夹)
goto :eof

:deploy_qt_files
if not defined QT_BIN goto :eof

REM 复制Qt核心DLL
copy "!QT_BIN!\Qt6Core.dll" "%BIN_DIR%\" >nul 2>&1 && echo ✓ Qt6Core.dll 已复制
copy "!QT_BIN!\Qt6Gui.dll" "%BIN_DIR%\" >nul 2>&1 && echo ✓ Qt6Gui.dll 已复制
copy "!QT_BIN!\Qt6Widgets.dll" "%BIN_DIR%\" >nul 2>&1 && echo ✓ Qt6Widgets.dll 已复制

REM 复制Qt平台插件
for %%j in ("!QT_BIN!") do set QT_PLUGINS=%%~dpj..\plugins
if exist "!QT_PLUGINS!\platforms\qwindows.dll" (
    if not exist "%BIN_DIR%\platforms" mkdir "%BIN_DIR%\platforms"
    copy "!QT_PLUGINS!\platforms\qwindows.dll" "%BIN_DIR%\platforms\" >nul 2>&1 && echo ✓ platforms\qwindows.dll 已复制
) else (
    echo ✗ 未找到Qt平台插件 qwindows.dll
)
goto :eof

:deploy_mingw_runtime
REM 尝试从多个位置复制MinGW运行时
set MINGW_PATHS=(
    "D:\MSYS2\mingw64\bin"
    "C:\msys64\mingw64\bin"
    "C:\MSYS2\mingw64\bin"
)

if defined MSYSTEM_PREFIX (
    set MINGW_PATHS=("%MSYSTEM_PREFIX%\bin" %MINGW_PATHS%)
)

for %%P in %MINGW_PATHS% do (
    if exist "%%P\libstdc++-6.dll" (
        echo 检测到MinGW运行时目录: %%P
        copy "%%P\libstdc++-6.dll" "%BIN_DIR%\" >nul 2>&1 && echo ✓ libstdc++-6.dll 已复制
        copy "%%P\libgcc_s_seh-1.dll" "%BIN_DIR%\" >nul 2>&1 && echo ✓ libgcc_s_seh-1.dll 已复制
        copy "%%P\libwinpthread-1.dll" "%BIN_DIR%\" >nul 2>&1 && echo ✓ libwinpthread-1.dll 已复制
        goto :eof
    )
)

echo ✗ 未找到MinGW运行时DLL
echo   请手动复制以下文件到 %BIN_DIR%:
echo   - libstdc++-6.dll
echo   - libgcc_s_seh-1.dll
echo   - libwinpthread-1.dll
echo   这些文件通常位于 MSYS2\mingw64\bin 目录
goto :eof

:deploy_complete
echo.
echo ====================================
echo 🎉 部署完成！
echo ====================================
echo 部署目录: %BIN_DIR%
echo.
echo 现在可以运行应用程序:
echo   cd %BIN_DIR%
echo   AuroraStream.exe
echo.
echo 或者直接双击: %BIN_DIR%\AuroraStream.exe
echo.
echo 如果遇到缺少DLL的错误，请检查上述部署日志
echo 详细指南请参考 README_RUNTIME.md
echo ====================================
echo.
echo 按任意键退出...
pause >nul