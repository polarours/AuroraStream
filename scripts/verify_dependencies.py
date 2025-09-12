#!/usr/bin/env python3
"""
AuroraStream 运行时依赖验证工具
用于检查构建目录中的DLL依赖是否完整
"""

import os
import sys
import glob
from pathlib import Path

def check_dependencies(build_dir):
    """检查运行时依赖是否完整"""
    print("=" * 60)
    print("AuroraStream 运行时依赖验证")
    print("=" * 60)

    # 确定构建目录
    if not build_dir:
        # 自动检测常见的构建目录
        possible_dirs = [
            "cmake-build-debug-mingw-msys2",
            "cmake-build-release-mingw-qt",
            "build",
            "cmake-build-debug",
            "cmake-build-release"
        ]

        for dir_name in possible_dirs:
            if os.path.exists(dir_name) and os.path.isdir(dir_name):
                build_dir = dir_name
                break
        else:
            print("错误：未找到构建目录，请手动指定")
            return False

    bin_dir = os.path.join(build_dir, "bin")
    if not os.path.exists(bin_dir):
        print(f"错误：构建目录 {build_dir} 中未找到 bin 文件夹")
        return False

    print(f"检查目录: {os.path.abspath(bin_dir)}")
    print()

    # 检查必要的DLL文件
    required_dlls = {
        "Qt6 Core": ["Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll"],
        "Qt6 Platform": ["platforms/qwindows.dll"],
        "MinGW Runtime": ["libstdc++-6.dll", "libgcc_s_seh-1.dll", "libwinpthread-1.dll"],
        "FFmpeg": ["avcodec-62.dll", "avformat-62.dll", "avutil-60.dll",
                   "swscale-9.dll", "swresample-4.dll"],
        "SDL2": ["SDL2.dll"]
    }

    all_missing = []
    found_count = 0
    total_count = 0

    for category, dlls in required_dlls.items():
        print(f"检查 {category}:")
        missing_in_category = []

        for dll in dlls:
            total_count += 1
            dll_path = os.path.join(bin_dir, dll)

            if os.path.exists(dll_path):
                print(f"  ✓ {dll}")
                found_count += 1
            else:
                print(f"  ✗ {dll} (缺失)")
                missing_in_category.append(dll)
                all_missing.append(dll)

        if missing_in_category:
            print(f"  缺失文件: {', '.join(missing_in_category)}")
        print()

    # 输出总结
    print("=" * 60)
    print("依赖检查结果:")
    print(f"找到文件: {found_count}/{total_count}")

    if all_missing:
        print("缺失文件:")
        for dll in all_missing:
            print(f"  - {dll}")

        print("\n解决方案:")
        print("1. 运行部署脚本: scripts/deploy_dependencies.bat")
        print("2. 手动复制缺失的DLL文件到构建目录的bin文件夹")
        print("3. 参考 README_RUNTIME.md 获取详细部署指南")
        return False
    else:
        print("✓ 所有运行时依赖都已就位！")
        print("应用程序应该可以正常运行。")
        return True

def main():
    """主函数"""
    build_dir = None
    if len(sys.argv) > 1:
        build_dir = sys.argv[1]

    success = check_dependencies(build_dir)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()