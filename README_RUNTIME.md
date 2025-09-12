# AuroraStream 运行时依赖部署指南

## 问题描述

编译后的可执行文件需要以下运行时依赖才能正常运行：
- Qt6 核心DLLs (Qt6Core.dll, Qt6Gui.dll, Qt6Widgets.dll)
- Qt6 平台插件 (platforms/qwindows.dll)
- FFmpeg DLLs (avcodec-62.dll, avformat-62.dll, avutil-60.dll, swscale-9.dll, swresample-4.dll)
- SDL2.dll

## 自动部署方案

### 方法一：使用部署脚本 (推荐)

1. 在项目根目录运行部署脚本：
   ```bash
   scripts\deploy_dependencies.bat
   ```

2. 按照提示选择要部署的依赖项

3. 脚本会自动检测Qt安装路径并复制所需的DLL文件

4. 将部署目录中的文件复制到构建目录的bin文件夹

### 方法二：使用CMake自动部署

1. 配置CMake时添加自动部署选项：
   ```bash
   cmake -B cmake-build-debug-mingw-msys2 -S . -DCMAKE_PREFIX_PATH="C:/Qt/6.6.0/mingw_64"
   ```

2. 构建项目：
   ```bash
   cmake --build cmake-build-debug-mingw-msys2
   ```

3. CMake会在构建后自动尝试复制Qt运行时依赖

## 手动部署步骤

### 1. 获取依赖文件

**Qt6 DLLs:**
- 从Qt安装目录复制 (通常位于 `C:\Qt\6.6.0\mingw_64\bin`)
- 需要的文件：Qt6Core.dll, Qt6Gui.dll, Qt6Widgets.dll

**Qt6 平台插件:**
- 从Qt安装目录复制 (通常位于 `C:\Qt\6.6.0\mingw_64\plugins\platforms\qwindows.dll`)
- 在bin目录下创建platforms文件夹，将qwindows.dll放入其中

**MinGW 运行时 DLLs (关键):**
- 从MSYS2安装目录复制 (通常位于 `D:\MSYS2\mingw64\bin`)
- 需要的文件：libstdc++-6.dll, libgcc_s_seh-1.dll, libwinpthread-1.dll
- 这些文件是MinGW编译器运行时必需的，缺少会导致应用程序无法启动

**FFmpeg DLLs:**
- 从FFm官网下载Windows版本：https://ffmpeg.org/download.html
- 需要的文件：avcodec-62.dll, avformat-62.dll, avutil-60.dll, swscale-9.dll, swresample-4.dll

**SDL2.dll:**
- 从SDL2官网下载开发库：https://www.libsdl.org/download-2.0.php
- 从lib目录复制SDL2.dll

### 2. 部署到构建目录

将所有DLL文件复制到构建目录的bin文件夹：
```
cmake-build-debug-mingw-msys2/
└── bin/
    ├── AuroraStream.exe
    ├── Qt6Core.dll
    ├── Qt6Gui.dll
    ├── Qt6Widgets.dll
    ├── libstdc++-6.dll          # MinGW运行时（必需）
    ├── libgcc_s_seh-1.dll       # MinGW运行时（必需）
    ├── libwinpthread-1.dll      # MinGW运行时（必需）
    ├── avcodec-62.dll
    ├── avformat-62.dll
    ├── avutil-60.dll
    ├── swscale-9.dll
    ├── swresample-4.dll
    ├── SDL2.dll
    └── platforms/
        └── qwindows.dll
```

## 验证部署

部署完成后，可以使用验证工具检查依赖是否完整：

```bash
# 使用Python验证工具
python scripts/verify_dependencies.py

# 或者指定构建目录
python scripts/verify_dependencies.py cmake-build-debug-mingw-msys2
```

验证通过后，双击 `AuroraStream.exe` 应该能够正常启动播放器界面。

如果遇到缺失DLL的错误，可以使用 Dependency Walker 或 Process Monitor 工具来检查缺失的依赖。

## 常见问题

1. **找不到Qt DLLs**: 确保设置了正确的CMAKE_PREFIX_PATH指向Qt安装目录
2. **平台插件错误**: 确保platforms文件夹和qwindows.dll正确放置
3. **FFmpeg版本不匹配**: 使用与编译时相同版本的FFmpeg DLLs

## 开发环境设置建议

对于开发环境，建议将依赖库的bin目录添加到系统PATH环境变量中，这样就不需要每次构建都复制DLL文件。