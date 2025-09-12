/********************************************************************************
 * @file   : Logger.cpp
 * @brief  : 
 *
 * 
 *
 * @author : polarours
 * @date   : 2025/8/28
 ********************************************************************************/

#include "aurorastream/utils/Logger.h"

namespace aurorastream {
    namespace utils {

        Logger& Logger::instance() {
            static Logger instance;
            return instance;
        }

        void Logger::init(const std::string& logFilePath) {
            logFile_.open(logFilePath, std::ios::out | std::ios::app);
        }

        void Logger::log(LogLevel level, const std::string& message) {
            std::lock_guard<std::mutex> lock(logMutex_);
            if (logFile_.is_open()) {
                logFile_ << "[" << getLevelString(level) << "] " << message << std::endl;
            }
        }

        std::string Logger::getLevelString(LogLevel level) {
            switch(level) {
                case LogLevel::DEBUG: return "DEBUG";
                case LogLevel::INFO: return "INFO";
                case LogLevel::WARNING: return "WARNING";
                case LogLevel::ERROR: return "ERROR";
                case LogLevel::CRITICAL: return "CRITICAL";
                default: return "UNKNOWN";
            }
        }

        // 便捷方法实现
        void Logger::debug(const std::string& msg) { log(LogLevel::DEBUG, msg); }
        void Logger::info(const std::string& msg) { log(LogLevel::INFO, msg); }
        void Logger::warning(const std::string& msg) { log(LogLevel::WARNING, msg); }
        void Logger::error(const std::string& msg) { log(LogLevel::ERROR, msg); }
        void Logger::critical(const std::string& msg) { log(LogLevel::CRITICAL, msg); }

    } // namespace utils
} // namespace aurorastream