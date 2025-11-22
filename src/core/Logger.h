/**
 * Logger.h - Unified Debug Logging System for Movement Dojo
 *
 * Designed for high-intensity VR combat debugging with:
 * - Configurable file output (auto-creates directories)
 * - Optional console output with colors
 * - Log levels: TRACE, DEBUG, INFO, WARN, ERROR, FATAL
 * - Thread-safe operation
 * - Subsystem tagging for filtering (XR, Input, Combat, etc.)
 * - Immediate flush for crash safety
 *
 * Usage:
 *   // Initialize at startup
 *   LogConfig config;
 *   config.logFilePath = "./logs/engine.log";
 *   config.fileLogLevel = LogLevel::DEBUG;
 *   config.consoleLogLevel = LogLevel::INFO;
 *   Logger::init(config);
 *
 *   // Log messages
 *   LOG_INFO("Engine") << "Initialized successfully";
 *   LOG_ERROR("XR") << "xrCreateSession failed: " << result;
 *   LOG_TRACE("Combat") << "Drone " << droneId << " position: " << pos;
 *
 *   // Shutdown
 *   Logger::shutdown();
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
#include <cstdlib>
#include <sys/stat.h>
#include <cstring>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#define MKDIR(path) mkdir(path, 0755)
#endif

namespace lst {

// =============================================================================
// Log Levels
// =============================================================================

enum class LogLevel {
    TRACE = 0,   // Extremely verbose, per-frame data (poses, velocities)
    DEBUG = 1,   // Detailed debugging info
    INFO  = 2,   // General information (startup, mode changes)
    WARN  = 3,   // Warnings (degraded performance, fallbacks)
    ERROR = 4,   // Errors (recoverable failures)
    FATAL = 5,   // Fatal errors (unrecoverable, will exit)
    OFF   = 6    // Disable logging
};

inline const char* logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        case LogLevel::OFF:   return "OFF  ";
        default:              return "?????";
    }
}

inline const char* logLevelToColor(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "\033[90m";  // Dark gray
        case LogLevel::DEBUG: return "\033[36m";  // Cyan
        case LogLevel::INFO:  return "\033[32m";  // Green
        case LogLevel::WARN:  return "\033[33m";  // Yellow
        case LogLevel::ERROR: return "\033[31m";  // Red
        case LogLevel::FATAL: return "\033[35m";  // Magenta
        default:              return "\033[0m";   // Reset
    }
}

// =============================================================================
// Log Configuration
// =============================================================================

struct LogConfig {
    std::string logFilePath = "./logs/engine.log";  // Log file path
    LogLevel fileLogLevel = LogLevel::DEBUG;        // Min level for file output
    LogLevel consoleLogLevel = LogLevel::INFO;      // Min level for console output
    bool enableConsole = true;                      // Enable console output
    bool enableFile = true;                         // Enable file output
    bool enableColors = true;                       // Enable ANSI colors in console
    bool flushEveryLine = true;                     // Flush after every log (crash safety)
    std::string appName = "Movement Dojo";          // App name for log header
};

// =============================================================================
// Logger Class
// =============================================================================

class Logger {
public:
    /**
     * Initialize the logging system
     * Must be called at the very start of main() before any logging
     */
    static bool init(const LogConfig& config = LogConfig()) {
        std::lock_guard<std::mutex> lock(getMutex());

        // Store configuration
        getConfig() = config;

        // Create log directory if needed
        if (config.enableFile) {
            if (!createDirectoryForPath(config.logFilePath)) {
                std::cerr << "[Logger] Failed to create log directory for: "
                          << config.logFilePath << std::endl;
                // Continue without file logging
                getConfig().enableFile = false;
            } else {
                // Open log file
                getFile().open(config.logFilePath, std::ios::out | std::ios::trunc);
                if (!getFile().is_open()) {
                    std::cerr << "[Logger] Failed to open log file: "
                              << config.logFilePath << std::endl;
                    getConfig().enableFile = false;
                } else {
                    // Write header
                    writeHeader();
                }
            }
        }

        getInitialized() = true;
        return true;
    }

    /**
     * Initialize with simple path (backward compatible)
     */
    static bool init(const std::string& filepath) {
        LogConfig config;
        config.logFilePath = filepath;
        return init(config);
    }

    /**
     * Shutdown the logging system
     * Ensures all logs are flushed to disk
     */
    static void shutdown() {
        std::lock_guard<std::mutex> lock(getMutex());

        if (getFile().is_open()) {
            // Write footer
            getFile() << "\n";
            getFile() << "================================================================================\n";
            getFile() << "Log ended: " << getTimestamp() << "\n";
            getFile() << "================================================================================\n";
            getFile().flush();
            getFile().close();
        }

        getInitialized() = false;
    }

    /**
     * Log a message
     */
    static void log(LogLevel level, const std::string& component, const std::string& message) {
        std::lock_guard<std::mutex> lock(getMutex());

        const LogConfig& config = getConfig();

        // Check if we should log to console
        bool logToConsole = config.enableConsole && level >= config.consoleLogLevel;

        // Check if we should log to file
        bool logToFile = config.enableFile && getFile().is_open() && level >= config.fileLogLevel;

        if (!logToConsole && !logToFile) {
            return;  // Nothing to do
        }

        // Format the message
        std::string timestamp = getTimestamp();
        std::string levelStr = logLevelToString(level);

        std::ostringstream formatted;
        formatted << "[" << timestamp << "] [" << levelStr << "] [" << component << "] " << message;
        std::string line = formatted.str();

        // Console output
        if (logToConsole) {
            if (config.enableColors) {
                std::cout << logLevelToColor(level) << line << "\033[0m" << std::endl;
            } else {
                std::cout << line << std::endl;
            }
        }

        // File output
        if (logToFile) {
            getFile() << line << std::endl;
            if (config.flushEveryLine) {
                getFile().flush();
            }
        }
    }

    /**
     * Set minimum log level for console output
     */
    static void setConsoleLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(getMutex());
        getConfig().consoleLogLevel = level;
    }

    /**
     * Set minimum log level for file output
     */
    static void setFileLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(getMutex());
        getConfig().fileLogLevel = level;
    }

    /**
     * Legacy: Set both levels (backward compatible)
     */
    static void setLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(getMutex());
        getConfig().consoleLogLevel = level;
        getConfig().fileLogLevel = level;
    }

    /**
     * Check if logger is initialized
     */
    static bool isInitialized() {
        return getInitialized();
    }

    /**
     * Get current log file path
     */
    static std::string getLogFilePath() {
        return getConfig().logFilePath;
    }

    /**
     * Force flush all pending log data to disk
     */
    static void flush() {
        std::lock_guard<std::mutex> lock(getMutex());
        if (getFile().is_open()) {
            getFile().flush();
        }
    }

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

    static bool createDirectoryForPath(const std::string& filepath) {
        // Find the directory portion of the path
        size_t lastSlash = filepath.find_last_of("/\\");
        if (lastSlash == std::string::npos) {
            return true;  // No directory to create
        }

        std::string dirPath = filepath.substr(0, lastSlash);
        if (dirPath.empty()) {
            return true;
        }

        // Create directories recursively
        std::string currentPath;
        for (size_t i = 0; i < dirPath.length(); i++) {
            char c = dirPath[i];
            currentPath += c;

            if (c == '/' || c == '\\' || i == dirPath.length() - 1) {
                if (!currentPath.empty() && currentPath != "." && currentPath != "..") {
                    // Try to create this directory
                    struct stat st;
                    if (stat(currentPath.c_str(), &st) != 0) {
                        // Directory doesn't exist, try to create it
                        if (MKDIR(currentPath.c_str()) != 0 && errno != EEXIST) {
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }

    static void writeHeader() {
        const LogConfig& config = getConfig();
        auto& file = getFile();

        file << "================================================================================\n";
        file << config.appName << " - Debug Log\n";
        file << "================================================================================\n";
        file << "Started:      " << getTimestamp() << "\n";
        file << "Log file:     " << config.logFilePath << "\n";
        file << "File level:   " << logLevelToString(config.fileLogLevel) << "\n";
        file << "Console level:" << logLevelToString(config.consoleLogLevel) << "\n";
        file << "================================================================================\n\n";
        file.flush();
    }

    // Static members using Meyer's singleton pattern (thread-safe initialization)
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

    static LogConfig& getConfig() {
        static LogConfig config;
        return config;
    }
};

// =============================================================================
// Log Stream Helper (for << syntax)
// =============================================================================

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

    // Prevent copying
    LogStream(const LogStream&) = delete;
    LogStream& operator=(const LogStream&) = delete;

    // Allow moving
    LogStream(LogStream&& other) noexcept
        : m_level(other.m_level)
        , m_component(std::move(other.m_component))
        , m_stream(std::move(other.m_stream)) {}

private:
    LogLevel m_level;
    std::string m_component;
    std::ostringstream m_stream;
};

// =============================================================================
// Convenience Macros
// =============================================================================

#define LOG_TRACE(component) lst::LogStream(lst::LogLevel::TRACE, component)
#define LOG_DEBUG(component) lst::LogStream(lst::LogLevel::DEBUG, component)
#define LOG_INFO(component)  lst::LogStream(lst::LogLevel::INFO, component)
#define LOG_WARN(component)  lst::LogStream(lst::LogLevel::WARN, component)
#define LOG_ERROR(component) lst::LogStream(lst::LogLevel::ERROR, component)
#define LOG_FATAL(component) lst::LogStream(lst::LogLevel::FATAL, component)

// =============================================================================
// OpenXR Result Helpers
// =============================================================================

/**
 * Convert OpenXR result code to human-readable string
 * Includes common error codes for debugging SteamVR/Quest issues
 */
inline std::string xrResultToString(int result) {
    switch (result) {
        // Success codes
        case 0:   return "XR_SUCCESS";
        case 1:   return "XR_TIMEOUT_EXPIRED";
        case 3:   return "XR_SESSION_LOSS_PENDING";

        // Generic errors
        case -1:  return "XR_ERROR_VALIDATION_FAILURE";
        case -2:  return "XR_ERROR_RUNTIME_FAILURE";
        case -3:  return "XR_ERROR_OUT_OF_MEMORY";
        case -4:  return "XR_ERROR_API_VERSION_UNSUPPORTED";
        case -5:  return "XR_ERROR_INITIALIZATION_FAILED";
        case -6:  return "XR_ERROR_FUNCTION_UNSUPPORTED";
        case -7:  return "XR_ERROR_FEATURE_UNSUPPORTED";
        case -8:  return "XR_ERROR_EXTENSION_NOT_PRESENT";
        case -9:  return "XR_ERROR_LIMIT_REACHED";
        case -10: return "XR_ERROR_SIZE_INSUFFICIENT";
        case -11: return "XR_ERROR_HANDLE_INVALID";

        // Instance errors
        case -12: return "XR_ERROR_INSTANCE_LOST";

        // Session errors
        case -13: return "XR_ERROR_SESSION_RUNNING";
        case -14: return "XR_ERROR_SESSION_NOT_RUNNING";
        case -15: return "XR_ERROR_SESSION_LOST";

        // System errors
        case -16: return "XR_ERROR_SYSTEM_INVALID";

        // Path errors
        case -17: return "XR_ERROR_PATH_INVALID";
        case -18: return "XR_ERROR_PATH_COUNT_EXCEEDED";
        case -19: return "XR_ERROR_PATH_FORMAT_INVALID";
        case -20: return "XR_ERROR_PATH_UNSUPPORTED";

        // Layer errors
        case -21: return "XR_ERROR_LAYER_INVALID";
        case -22: return "XR_ERROR_LAYER_LIMIT_EXCEEDED";

        // Swapchain errors
        case -23: return "XR_ERROR_SWAPCHAIN_RECT_INVALID";
        case -24: return "XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED";

        // Action errors
        case -25: return "XR_ERROR_ACTION_TYPE_MISMATCH";
        case -26: return "XR_ERROR_SESSION_NOT_FOCUSED";
        case -27: return "XR_ERROR_ACTION_SET_NOT_ATTACHED";

        // Space/Pose errors
        case -28: return "XR_ERROR_REFERENCE_SPACE_UNSUPPORTED";
        case -29: return "XR_ERROR_FILE_ACCESS_ERROR";
        case -30: return "XR_ERROR_FILE_CONTENTS_INVALID";

        // Form factor errors (Quest/HMD detection)
        case -31: return "XR_ERROR_FORM_FACTOR_UNSUPPORTED";
        case -32: return "XR_ERROR_FORM_FACTOR_UNAVAILABLE";

        // View errors
        case -33: return "XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED";

        // Environment blend mode
        case -34: return "XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED";

        // Name errors
        case -37: return "XR_ERROR_NAME_DUPLICATED";
        case -38: return "XR_ERROR_NAME_INVALID";

        // Actionset errors
        case -39: return "XR_ERROR_ACTIONSET_NOT_ATTACHED";
        case -40: return "XR_ERROR_ACTIONSETS_ALREADY_ATTACHED";

        // Binding errors
        case -41: return "XR_ERROR_LOCALIZED_NAME_DUPLICATED";
        case -42: return "XR_ERROR_LOCALIZED_NAME_INVALID";

        // Graphics errors
        case -43: return "XR_ERROR_GRAPHICS_DEVICE_INVALID";
        case -44: return "XR_ERROR_POSE_INVALID";

        // Index errors
        case -45: return "XR_ERROR_INDEX_OUT_OF_RANGE";

        // Composition errors
        case -46: return "XR_ERROR_COMPOSITION_LAYER_INVALID";

        // Runtime errors
        case -50: return "XR_ERROR_RUNTIME_UNAVAILABLE";

        // Extension-specific (estimate ranges)
        case -1000000000: return "XR_ERROR_EXTENSION (Vulkan)";

        default:
            if (result < -1000) {
                return "XR_ERROR_EXTENSION_SPECIFIC (" + std::to_string(result) + ")";
            }
            return "XR_UNKNOWN (" + std::to_string(result) + ")";
    }
}

/**
 * Get troubleshooting hint for common OpenXR errors
 * Tailored for Quest 3 + Virtual Desktop + SteamVR setup
 */
inline std::string xrErrorHint(int result) {
    switch (result) {
        case -2:  // XR_ERROR_RUNTIME_FAILURE
            return "Runtime crashed or is unresponsive. Restart SteamVR and try again.";
        case -5:  // XR_ERROR_INITIALIZATION_FAILED
            return "OpenXR initialization failed. Check: (1) SteamVR is running, "
                   "(2) SteamVR is set as default OpenXR runtime, (3) HMD is connected.";
        case -8:  // XR_ERROR_EXTENSION_NOT_PRESENT
            return "Required extension not available. Update SteamVR/graphics drivers.";
        case -12: // XR_ERROR_INSTANCE_LOST
            return "OpenXR instance lost. SteamVR may have crashed - restart it.";
        case -15: // XR_ERROR_SESSION_LOST
            return "VR session lost. Reconnect headset or restart SteamVR.";
        case -16: // XR_ERROR_SYSTEM_INVALID
            return "Invalid system. HMD may be disconnected or sleeping.";
        case -31: // XR_ERROR_FORM_FACTOR_UNSUPPORTED
            return "HMD form factor not supported by this runtime.";
        case -32: // XR_ERROR_FORM_FACTOR_UNAVAILABLE
            return "No HMD detected. For Quest 3 + Virtual Desktop: (1) Ensure Virtual Desktop "
                   "streamer is running on PC, (2) Quest is connected and streaming, "
                   "(3) SteamVR is running and detects the headset.";
        case -43: // XR_ERROR_GRAPHICS_DEVICE_INVALID
            return "Graphics device invalid. Check: (1) GPU drivers are up to date, "
                   "(2) Vulkan/D3D is working, (3) No GPU resource conflicts.";
        case -50: // XR_ERROR_RUNTIME_UNAVAILABLE
            return "No OpenXR runtime available. Start SteamVR before launching the app.";
        default:
            return "";
    }
}

/**
 * Log an OpenXR result with appropriate level and troubleshooting hint
 */
#define LOG_XR_RESULT(component, result, context) \
    do { \
        int _xr_res = (result); \
        if (_xr_res < 0) { \
            LOG_ERROR(component) << context << ": " << lst::xrResultToString(_xr_res); \
            std::string _hint = lst::xrErrorHint(_xr_res); \
            if (!_hint.empty()) { \
                LOG_ERROR(component) << "  Hint: " << _hint; \
            } \
        } else if (_xr_res > 0) { \
            LOG_WARN(component) << context << ": " << lst::xrResultToString(_xr_res); \
        } \
    } while(0)

/**
 * Log an OpenXR result and return false on failure (with troubleshooting hint)
 */
#define LOG_XR_CHECK(component, result, context) \
    do { \
        int _xr_res = (result); \
        if (_xr_res < 0) { \
            LOG_ERROR(component) << context << " FAILED: " << lst::xrResultToString(_xr_res); \
            std::string _hint = lst::xrErrorHint(_xr_res); \
            if (!_hint.empty()) { \
                LOG_ERROR(component) << "  Hint: " << _hint; \
            } \
            return false; \
        } else if (_xr_res > 0) { \
            LOG_WARN(component) << context << ": " << lst::xrResultToString(_xr_res); \
        } \
    } while(0)

/**
 * Execute OpenXR call, store result, log on failure with hint
 * Usage: XR_CHECK(result, xrCreateInstance(...), "Creating XR instance")
 */
#define XR_CHECK(resultVar, xrCall, context) \
    do { \
        resultVar = (xrCall); \
        if (resultVar < 0) { \
            LOG_ERROR(LOG_TAG_XR) << context << " FAILED: " << lst::xrResultToString(resultVar); \
            std::string _hint = lst::xrErrorHint(resultVar); \
            if (!_hint.empty()) { \
                LOG_ERROR(LOG_TAG_XR) << "  Hint: " << _hint; \
            } \
            return false; \
        } else if (resultVar > 0) { \
            LOG_WARN(LOG_TAG_XR) << context << ": " << lst::xrResultToString(resultVar); \
        } \
    } while(0)

/**
 * Execute OpenXR call, log on failure but don't return (non-fatal)
 */
#define XR_WARN_ON_FAIL(resultVar, xrCall, context) \
    do { \
        resultVar = (xrCall); \
        if (resultVar < 0) { \
            LOG_WARN(LOG_TAG_XR) << context << ": " << lst::xrResultToString(resultVar); \
        } \
    } while(0)

// =============================================================================
// Subsystem Tags (for consistent filtering)
// =============================================================================

// Core subsystems
#define LOG_TAG_ENGINE   "Engine"
#define LOG_TAG_XR       "XR"
#define LOG_TAG_RENDER   "Render"
#define LOG_TAG_INPUT    "Input"
#define LOG_TAG_PHYSICS  "Physics"

// Combat subsystems (for future dojo)
#define LOG_TAG_COMBAT   "Combat"
#define LOG_TAG_DRONE    "Drone"
#define LOG_TAG_SABER    "Saber"
#define LOG_TAG_BLASTER  "Blaster"
#define LOG_TAG_HAPTICS  "Haptics"

// Analytics
#define LOG_TAG_ANALYTICS "Analytics"
#define LOG_TAG_PERF      "Perf"

// Diagnostics
#define LOG_TAG_DIAG      "Diag"

} // namespace lst
