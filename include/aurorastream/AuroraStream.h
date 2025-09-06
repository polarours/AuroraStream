/********************************************************************************
 * @file   : aurorastream.h
 * @brief  : aurorastream 媒体框架库的主包含文件。
 *
 * 此头文件是 aurorastream 库公共 API 的主要入口点。
 * 包含此文件即可访问库的核心功能和公共接口
 * 它定义了必要的宏，包含了关键模块的头文件，并声明了主命名空间。
 *
 * @Author : polarours
 * @Date   : 2025/08/21
 ********************************************************************************/

// include/aurorastream/aurorastream.h
#ifndef AURORASTREAM_H
#define AURORASTREAM_H

/**
 * @defgroup versioning aurorastream 版本信息
 * @{
 */

/**
 * @def AURORASTREAM_VERSION_MAJOR
 * @brief aurorastream 库主版本号。
 * 重大变更，不兼容的 API 修改。
 */
#define AURORASTREAM_VERSION_MAJOR 1

/**
 * @def AURORASTREAM_VERSION_MINOR
 * @brief aurorastream 库次版本号。
 * 向后兼容的功能性新增。
 */
#define AURORASTREAM_VERSION_MINOR 0

/**
 * @def AURORASTREAM_VERSION_PATCH
 * @brief aurorastream 库修订版本号。
 * 向后兼容的问题修正。
 */
#define AURORASTREAM_VERSION_PATCH 0

/**
 * @def AURORASTREAM_VERSION_STR
 * @brief aurorastream 库版本号字符串。
 * 格式为 "MAJOR.MINOR.PATCH"。
 */
#define AURORASTREAM_VERSION_STR "1.0.0"

/** @} */ // end of versioning group

/**
 * @defgroup build_config aurorastream 构建配置
 * @{
 */

// --- 库导出/导入宏定义 ---
// 用于支持 Windows DLL 构建。对于静态库或跨平台库，这是标准做法。

// 如果定义了 AURORASTREAM_STATIC_LIB，则所有符号都是静态链接的。
#if defined(AURORASTREAM_STATIC_LIB)
#  define AURORASTREAM_API // 静态库，无需特殊声明
#else
    // 动态库构建
#  if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__CYGWIN__)
    // Windows 平台
#    if defined(AURORASTREAM_LIBRARY) // 在构建 aurorastream 库本身时定义
#      define AURORASTREAM_API __declspec(dllexport) // 导出符号
#    else
#      define AURORASTREAM_API __declspec(dllimport) // 导入符号
#    endif
#  else
    // Unix-like 平台 (Linux, macOS) 使用 GCC/Clang 的 visibility 属性
#    if defined(__GNUC__) || defined(__clang__)
#      define AURORASTREAM_API __attribute__((visibility("default")))
#    else
#      define AURORASTREAM_API // 其他编译器默认可见
#    endif
#  endif
#endif

/** @} */ // end of build_config group

#include "core/MediaPlayer.h" // 媒体播放器核心接口
#include "utils/Logger.h"     // 日志系统
#include "utils/ConfigManager.h" // 配置管理

/**
 * @namespace aurorastream
 * @brief aurorastream 库的根命名空间。
 *
 * 所有 aurorastream 库的公共类、函数和类型都应位于此命名空间或其子命名空间内，
 * 以防止与用户代码或其他库发生命名冲突。
 */
namespace aurorastream {

} // namespace aurorastream

#endif  // AURORASTREAM_H