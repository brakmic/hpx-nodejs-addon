#include "logger.hpp"
#include "hpx_config.hpp" // To access HPXUserConfig
#include "data_conversion.hpp"
#include <algorithm>      // For std::transform
#include <cctype>         // For std::toupper

// Initialize the singleton instance
Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

// Private constructor
Logger::Logger()
    : currentLevel_(LogLevel::Info), enabled_(true) {}

// Helper method to convert string to LogLevel
LogLevel Logger::stringToLogLevel(const std::string& levelStr) {
    std::string upperStr = ToUpperCase(levelStr);
    if (upperStr == "DEBUG") return LogLevel::Debug;
    if (upperStr == "INFO") return LogLevel::Info;
    if (upperStr == "WARN") return LogLevel::Warn;
    if (upperStr == "ERROR") return LogLevel::Error;
    return LogLevel::Info; // Default
}

// Initialize the logger with configuration
void Logger::initialize(bool enabled, LogLevel level) {
    std::lock_guard<std::mutex> lock(mtx_);
    enabled_ = enabled;
    currentLevel_ = level;
}

// Set log level at runtime
void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mtx_);
    currentLevel_ = level;
}

// Enable or disable logging at runtime
void Logger::setEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mtx_);
    enabled_ = enabled;
}

// Format the log message with timestamp, level, and optional file/line info
std::string Logger::formatMessage(const std::string& levelStr, const std::string& message, const char* file, int line) {
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto tm_now = std::localtime(&time_t_now);

    // Format timestamp
    char timeBuffer[20];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", tm_now);

    // Construct the log message
    std::string formatted = "[" + std::string(levelStr) + "] " + std::string(timeBuffer) + " - " + message;

    // Append file and line info if provided
    if (file != nullptr && std::string(file).length() > 0 && line > 0) {
        formatted += " (" + std::string(file) + ":" + std::to_string(line) + ")";
    }

    return formatted;
}

// Logging methods
void Logger::debug(const std::string& message, const char* file, int line) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (enabled_ && currentLevel_ <= LogLevel::Debug) {
        std::cerr << formatMessage("DEBUG", message, file, line) << std::endl;
    }
}

void Logger::info(const std::string& message, const char* file, int line) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (enabled_ && currentLevel_ <= LogLevel::Info) {
        std::cerr << formatMessage("INFO", message, file, line) << std::endl;
    }
}

void Logger::warn(const std::string& message, const char* file, int line) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (enabled_ && currentLevel_ <= LogLevel::Warn) {
        std::cerr << formatMessage("WARN", message, file, line) << std::endl;
    }
}

void Logger::error(const std::string& message, const char* file, int line) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (enabled_ && currentLevel_ <= LogLevel::Error) {
        std::cerr << formatMessage("ERROR", message, file, line) << std::endl;
    }
}
