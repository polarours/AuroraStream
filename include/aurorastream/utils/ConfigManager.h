/********************************************************************************
 * @file   :
 * @brief  :
 *
 *
 * @author : polarours
 * @date   : 2025/08/29
 ********************************************************************************/

#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace aurorastream {
namespace utils {

class ConfigManager {
public:
    static ConfigManager& instance();

    void load(const std::string& configPath);
    void save(const std::string& configPath);

    std::string get(const std::string& key, const std::string& defaultValue = "");
    void set(const std::string& key, const std::string& value);

    int getInt(const std::string& key, int defaultValue = 0);
    void setInt(const std::string& key, int value);

    bool getBool(const std::string& key, bool defaultValue = false);
    void setBool(const std::string& key, bool value);

private:
    ConfigManager() = default;
    std::unordered_map<std::string, std::string> configMap_;
    std::mutex configMutex_;
};

} // namespace utils
} // namespace aurorastream