/********************************************************************************
 * @file   :
 * @brief  :
 *
 *
 *
 * @author : polarours
 * @date   : 2025/08/29
 ********************************************************************************/

#ifndef AURORASTREAM_UTILS_LOGGER_H
#define AURORASTREAM_UTILS_LOGGER_H

#include <string>
#include <fstream>
#include <memory>
#include <mutex>

namespace aurorastream {
namespace utils {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
public:
    static Logger& instance();

    void init(const std::string& logFilePath);
    void log(LogLevel level, const std::string& message);

    // 便捷方法
    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warning(const std::string& msg);
    void error(const std::string& msg);
    void critical(const std::string& msg);

private:
    Logger() = default;
    std::ofstream logFile_;
    std::mutex logMutex_;
    std::string getLevelString(LogLevel level);
};

} // namespace utils
} // namespace aurorastream

#endif