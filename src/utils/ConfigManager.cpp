/********************************************************************************
 * @file   : ConfigManager.cpp
 * @brief  : 
 *
 * 
 *
 * @author : polarours
 * @date   : 2025/8/28
 ********************************************************************************/

#include "aurorastream/utils/ConfigManager.h"

#include <fstream>
#include <sstream>

namespace aurorastream {
    namespace utils {

        ConfigManager& ConfigManager::instance() {
            static ConfigManager instance;
            return instance;
        }

        void ConfigManager::load(const std::string& configPath) {
            std::lock_guard<std::mutex> lock(configMutex_);
            std::ifstream file(configPath);
            std::string line;

            while (std::getline(file, line)) {
                size_t delimiter = line.find('=');
                if (delimiter != std::string::npos) {
                    std::string key = line.substr(0, delimiter);
                    std::string value = line.substr(delimiter + 1);
                    configMap_[key] = value;
                }
            }
        }

        void ConfigManager::save(const std::string& configPath) {
            std::lock_guard<std::mutex> lock(configMutex_);
            std::ofstream file(configPath);

            for (const auto& [key, value] : configMap_) {
                file << key << "=" << value << "\n";
            }
        }

        std::string ConfigManager::get(const std::string& key, const std::string& defaultValue) {
            std::lock_guard<std::mutex> lock(configMutex_);
            auto it = configMap_.find(key);
            return it != configMap_.end() ? it->second : defaultValue;
        }

        void ConfigManager::set(const std::string& key, const std::string& value) {
            std::lock_guard<std::mutex> lock(configMutex_);
            configMap_[key] = value;
        }

        int ConfigManager::getInt(const std::string& key, int defaultValue) {
            std::string value = get(key);
            return value.empty() ? defaultValue : std::stoi(value);
        }

        void ConfigManager::setInt(const std::string& key, int value) {
            set(key, std::to_string(value));
        }

        bool ConfigManager::getBool(const std::string& key, bool defaultValue) {
            std::string value = get(key);
            if (value.empty()) return defaultValue;
            return value == "true" || value == "1";
        }

        void ConfigManager::setBool(const std::string& key, bool value) {
            set(key, value ? "true" : "false");
        }

    } // namespace utils
} // namespace aurorastream