#pragma once

#include "Types.h"
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>

namespace lst {

/**
 * Configuration - Simple JSON-like configuration system
 *
 * Supports loading and saving application settings to a config file.
 * Uses a simple key-value format for compatibility without external JSON libs.
 */

class Configuration {
public:
    Configuration() = default;
    ~Configuration() = default;

    // Load/save from file
    bool load(const std::string& filepath);
    bool save(const std::string& filepath) const;

    // String values
    void setString(const std::string& key, const std::string& value);
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;

    // Integer values
    void setInt(const std::string& key, int value);
    int getInt(const std::string& key, int defaultValue = 0) const;

    // Float values
    void setFloat(const std::string& key, float value);
    float getFloat(const std::string& key, float defaultValue = 0.0f) const;

    // Boolean values
    void setBool(const std::string& key, bool value);
    bool getBool(const std::string& key, bool defaultValue = false) const;

    // Check if key exists
    bool hasKey(const std::string& key) const;

    // Remove a key
    void remove(const std::string& key);

    // Clear all settings
    void clear();

    // Get all keys
    std::vector<std::string> getKeys() const;

    // Create default configuration
    static Configuration createDefault();

    // Common configuration keys
    struct Keys {
        // General
        static constexpr const char* APP_NAME = "app.name";
        static constexpr const char* PLAYER_NAME = "player.name";
        static constexpr const char* PROFILE_PATH = "player.profile_path";

        // Session settings
        static constexpr const char* DEFAULT_MODE = "session.default_mode";
        static constexpr const char* AUTO_START_SESSION = "session.auto_start";
        static constexpr const char* EXPORT_ON_EXIT = "session.export_on_exit";
        static constexpr const char* EXPORT_PATH = "session.export_path";

        // Overlay settings
        static constexpr const char* OVERLAY_ENABLED = "overlay.enabled";
        static constexpr const char* OVERLAY_PRESET = "overlay.preset";
        static constexpr const char* OVERLAY_OPACITY = "overlay.opacity";
        static constexpr const char* SHOW_TRAILS = "overlay.show_trails";
        static constexpr const char* SHOW_COVERAGE = "overlay.show_coverage";
        static constexpr const char* SHOW_HUD = "overlay.show_hud";
        static constexpr const char* TRAIL_LENGTH = "overlay.trail_length";

        // Analytics settings
        static constexpr const char* VOXEL_RESOLUTION = "analytics.voxel_resolution";
        static constexpr const char* SAMPLE_RATE = "analytics.sample_rate";
        static constexpr const char* HISTORY_DURATION = "analytics.history_duration";

        // Haptics settings
        static constexpr const char* HAPTICS_ENABLED = "haptics.enabled";
        static constexpr const char* HAPTICS_INTENSITY = "haptics.intensity";
        static constexpr const char* HAPTICS_ON_DISCOVERY = "haptics.on_discovery";

        // Display settings
        static constexpr const char* RENDER_SCALE = "display.render_scale";
        static constexpr const char* SHOW_STATS = "display.show_stats";
        static constexpr const char* STATS_INTERVAL = "display.stats_interval";
    };

private:
    std::map<std::string, std::string> m_values;

    static std::string trim(const std::string& str);
    static bool parseKeyValue(const std::string& line, std::string& key, std::string& value);
};

// ============================================================================
// Implementation (header-only for simplicity)
// ============================================================================

inline bool Configuration::load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    m_values.clear();
    std::string line;

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::string key, value;
        if (parseKeyValue(line, key, value)) {
            m_values[key] = value;
        }
    }

    return true;
}

inline bool Configuration::save(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "# Movement Dojo Configuration\n";
    file << "# Auto-generated - edit with care\n\n";

    std::string lastSection;
    for (const auto& [key, value] : m_values) {
        // Add section comments
        size_t dotPos = key.find('.');
        if (dotPos != std::string::npos) {
            std::string section = key.substr(0, dotPos);
            if (section != lastSection) {
                if (!lastSection.empty()) file << "\n";
                file << "# " << section << " settings\n";
                lastSection = section;
            }
        }
        file << key << " = " << value << "\n";
    }

    return true;
}

inline void Configuration::setString(const std::string& key, const std::string& value) {
    m_values[key] = value;
}

inline std::string Configuration::getString(const std::string& key, const std::string& defaultValue) const {
    auto it = m_values.find(key);
    return (it != m_values.end()) ? it->second : defaultValue;
}

inline void Configuration::setInt(const std::string& key, int value) {
    m_values[key] = std::to_string(value);
}

inline int Configuration::getInt(const std::string& key, int defaultValue) const {
    auto it = m_values.find(key);
    if (it == m_values.end()) return defaultValue;
    try {
        return std::stoi(it->second);
    } catch (...) {
        return defaultValue;
    }
}

inline void Configuration::setFloat(const std::string& key, float value) {
    std::ostringstream oss;
    oss << value;
    m_values[key] = oss.str();
}

inline float Configuration::getFloat(const std::string& key, float defaultValue) const {
    auto it = m_values.find(key);
    if (it == m_values.end()) return defaultValue;
    try {
        return std::stof(it->second);
    } catch (...) {
        return defaultValue;
    }
}

inline void Configuration::setBool(const std::string& key, bool value) {
    m_values[key] = value ? "true" : "false";
}

inline bool Configuration::getBool(const std::string& key, bool defaultValue) const {
    auto it = m_values.find(key);
    if (it == m_values.end()) return defaultValue;
    return (it->second == "true" || it->second == "1" || it->second == "yes");
}

inline bool Configuration::hasKey(const std::string& key) const {
    return m_values.find(key) != m_values.end();
}

inline void Configuration::remove(const std::string& key) {
    m_values.erase(key);
}

inline void Configuration::clear() {
    m_values.clear();
}

inline std::vector<std::string> Configuration::getKeys() const {
    std::vector<std::string> keys;
    keys.reserve(m_values.size());
    for (const auto& [key, value] : m_values) {
        keys.push_back(key);
    }
    return keys;
}

inline Configuration Configuration::createDefault() {
    Configuration config;

    // General
    config.setString(Keys::APP_NAME, "Movement Dojo");
    config.setString(Keys::PLAYER_NAME, "Player");
    config.setString(Keys::PROFILE_PATH, "profile.dat");

    // Session
    config.setString(Keys::DEFAULT_MODE, "free");
    config.setBool(Keys::AUTO_START_SESSION, true);
    config.setBool(Keys::EXPORT_ON_EXIT, true);
    config.setString(Keys::EXPORT_PATH, "movement_data");

    // Overlay
    config.setBool(Keys::OVERLAY_ENABLED, true);
    config.setString(Keys::OVERLAY_PRESET, "standard");
    config.setFloat(Keys::OVERLAY_OPACITY, 0.7f);
    config.setBool(Keys::SHOW_TRAILS, true);
    config.setBool(Keys::SHOW_COVERAGE, true);
    config.setBool(Keys::SHOW_HUD, true);
    config.setFloat(Keys::TRAIL_LENGTH, 2.0f);

    // Analytics
    config.setFloat(Keys::VOXEL_RESOLUTION, 0.1f);
    config.setInt(Keys::SAMPLE_RATE, 90);
    config.setFloat(Keys::HISTORY_DURATION, 3600.0f);

    // Haptics
    config.setBool(Keys::HAPTICS_ENABLED, true);
    config.setFloat(Keys::HAPTICS_INTENSITY, 1.0f);
    config.setBool(Keys::HAPTICS_ON_DISCOVERY, true);

    // Display
    config.setFloat(Keys::RENDER_SCALE, 1.0f);
    config.setBool(Keys::SHOW_STATS, true);
    config.setFloat(Keys::STATS_INTERVAL, 10.0f);

    return config;
}

inline std::string Configuration::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

inline bool Configuration::parseKeyValue(const std::string& line, std::string& key, std::string& value) {
    size_t eqPos = line.find('=');
    if (eqPos == std::string::npos) return false;

    key = trim(line.substr(0, eqPos));
    value = trim(line.substr(eqPos + 1));

    // Remove quotes if present
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.size() - 2);
    }

    return !key.empty();
}

} // namespace lst
