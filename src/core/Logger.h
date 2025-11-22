/**
 * Logger.h - Unified Debug Logging System
 *
 * Provides consistent logging across all engine components with:
 * - Console output with timestamps
 * - File output to debug-log.txt
 * - Log levels (DEBUG, INFO, WARN, ERROR)
 * - Thread-safe operation
 *
 * Usage:
 *   Logger::init("debug-log.txt");
 *   LOG_INFO("Engine") << "Initialized successfully";
 *   LOG_ERROR("Physics") << "Collision failed: " << errorCode;
 */

#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace lst {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

class Logger {
public:
    // Initialize logging to file
    static bool init(const std::string& filepath = "debug-log.txt") {
        std::lock_guard<std::mutex> lock(getMutex());
        getFile().open(filepath, std::ios::out | std::ios::trunc);
        if (!getFile().is_open()) {
            std::cerr << "[Logger] Failed to open log file: " << filepath << std::endl;
            return false;
        }

        // Write header
        getFile() << "========================================\n";
        getFile() << "Movement Dojo Debug Log\n";
        getFile() << "Started: " << getTimestamp() << "\n";
        getFile() << "========================================\n\n";
        getFile().flush();

        getInitialized() = true;
        return true;
    }

    // Shutdown logging
    static void shutdown() {
        std::lock_guard<std::mutex> lock(getMutex());
        if (getFile().is_open()) {
            getFile() << "\n========================================\n";
            getFile() << "Log ended: " << getTimestamp() << "\n";
            getFile() << "========================================\n";
            getFile().close();
        }
        getInitialized() = false;
    }

    // Set minimum log level
    static void setLevel(LogLevel level) {
        getMinLevel() = level;
    }

    // Log a message
    static void log(LogLevel level, const std::string& component, const std::string& message) {
        if (level < getMinLevel()) return;

        std::lock_guard<std::mutex> lock(getMutex());

        std::string levelStr;
        std::string colorCode;

        switch (level) {
            case LogLevel::DEBUG:
                levelStr = "DEBUG";
                colorCode = "\033[36m";  // Cyan
                break;
            case LogLevel::INFO:
                levelStr = "INFO ";
                colorCode = "\033[32m";  // Green
                break;
            case LogLevel::WARN:
                levelStr = "WARN ";
                colorCode = "\033[33m";  // Yellow
                break;
            case LogLevel::ERROR:
                levelStr = "ERROR";
                colorCode = "\033[31m";  // Red
                break;
        }

        std::string timestamp = getTimestamp();
        std::string formatted = "[" + timestamp + "] [" + levelStr + "] [" + component + "] " + message;

        // Console output with color
        std::cout << colorCode << formatted << "\033[0m" << std::endl;

        // File output (no color codes)
        if (getInitialized() && getFile().is_open()) {
            getFile() << formatted << std::endl;
            getFile().flush();  // Ensure immediate write
        }
    }

    // Check if initialized
    static bool isInitialized() { return getInitialized(); }

private:
    static std::string getTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::tm* tm = std::localtime(&time);

        std::ostringstream oss;
        oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }

    // Static members using Meyer's singleton pattern
    static std::mutex& getMutex() {
        static std::mutex mutex;
        return mutex;
    }

    static std::ofstream& getFile() {
        static std::ofstream file;
        return file;
    }

    static bool& getInitialized() {
        static bool initialized = false;
        return initialized;
    }

    static LogLevel& getMinLevel() {
        static LogLevel level = LogLevel::DEBUG;
        return level;
    }
};

// Log stream helper for << syntax
class LogStream {
public:
    LogStream(LogLevel level, const std::string& component)
        : m_level(level), m_component(component) {}

    ~LogStream() {
        Logger::log(m_level, m_component, m_stream.str());
    }

    template<typename T>
    LogStream& operator<<(const T& value) {
        m_stream << value;
        return *this;
    }

private:
    LogLevel m_level;
    std::string m_component;
    std::ostringstream m_stream;
};

// Convenience macros
#define LOG_DEBUG(component) lst::LogStream(lst::LogLevel::DEBUG, component)
#define LOG_INFO(component)  lst::LogStream(lst::LogLevel::INFO, component)
#define LOG_WARN(component)  lst::LogStream(lst::LogLevel::WARN, component)
#define LOG_ERROR(component) lst::LogStream(lst::LogLevel::ERROR, component)

// XR result logging helper
inline std::string xrResultToString(int result) {
    switch (result) {
        case 0: return "XR_SUCCESS";
        case 1: return "XR_TIMEOUT_EXPIRED";
        case -1: return "XR_ERROR_VALIDATION_FAILURE";
        case -2: return "XR_ERROR_RUNTIME_FAILURE";
        case -3: return "XR_ERROR_OUT_OF_MEMORY";
        case -4: return "XR_ERROR_API_VERSION_UNSUPPORTED";
        case -6: return "XR_ERROR_HANDLE_INVALID";
        case -7: return "XR_ERROR_INSTANCE_LOST";
        case -8: return "XR_ERROR_SESSION_RUNNING";
        case -9: return "XR_ERROR_SESSION_NOT_RUNNING";
        case -10: return "XR_ERROR_SESSION_LOST";
        case -11: return "XR_ERROR_SYSTEM_INVALID";
        case -12: return "XR_ERROR_PATH_INVALID";
        case -13: return "XR_ERROR_PATH_COUNT_EXCEEDED";
        case -14: return "XR_ERROR_PATH_FORMAT_INVALID";
        case -51: return "XR_ERROR_FORM_FACTOR_UNSUPPORTED";
        case -52: return "XR_ERROR_FORM_FACTOR_UNAVAILABLE";
        default: return "XR_ERROR_UNKNOWN (" + std::to_string(result) + ")";
    }
}

#define LOG_XR_RESULT(component, result, context) \
    if ((result) < 0) { \
        LOG_ERROR(component) << context << ": " << lst::xrResultToString(result); \
    } else if ((result) > 0) { \
        LOG_WARN(component) << context << ": " << lst::xrResultToString(result); \
    }

} // namespace lst
