#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <mutex>
#include <memory>
#include <iostream>
#include <chrono>
#include <iomanip>

enum class LogLevel {
    Debug = 0,
    Info,
    Warn,
    Error,
    None // For disabling all logs
};

/**
 * @brief Singleton Logger class to handle log messages.
 */
class Logger {
public:
    /**
     * @brief Get the singleton instance of Logger.
     * @return Reference to the Logger instance.
     */
    static Logger& getInstance();

    /**
     * @brief Initialize the logger with configuration settings.
     * @param enabled Whether logging is enabled.
     * @param level The log level.
     */
    void initialize(bool enabled, LogLevel level);

    // Instance Methods
    void debug(const std::string& message, const char* file = "", int line = 0);
    void info(const std::string& message, const char* file = "", int line = 0);
    void warn(const std::string& message, const char* file = "", int line = 0);
    void error(const std::string& message, const char* file = "", int line = 0);

    /**
     * @brief Set the log level at runtime.
     * @param level The new log level.
     */
    void setLogLevel(LogLevel level);

    /**
     * @brief Enable or disable logging at runtime.
     * @param enabled True to enable, false to disable.
     */
    void setEnabled(bool enabled);

    // Static Wrapper Methods for Convenience
    static void Debug(const std::string& message, const char* file = "", int line = 0) {
        getInstance().debug(message, file, line);
    }

    static void Info(const std::string& message, const char* file = "", int line = 0) {
        getInstance().info(message, file, line);
    }

    static void Warn(const std::string& message, const char* file = "", int line = 0) {
        getInstance().warn(message, file, line);
    }

    static void Error(const std::string& message, const char* file = "", int line = 0) {
        getInstance().error(message, file, line);
    }

private:
    // Private constructor for Singleton pattern
    Logger();

    // Deleted copy constructor and assignment operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Convert string to LogLevel enum.
     * @param levelStr The log level string.
     * @return Corresponding LogLevel enum.
     */
    LogLevel stringToLogLevel(const std::string& levelStr);

    /**
     * @brief Format the log message with timestamp, level, and optional file/line info.
     * @param levelStr The log level as a string.
     * @param message The log message.
     * @param file The source file name.
     * @param line The line number in the source file.
     * @return Formatted log message string.
     */
    std::string formatMessage(const std::string& levelStr, const std::string& message, const char* file, int line);

    // Current log level
    LogLevel currentLevel_;

    // Flag to enable/disable logging
    bool enabled_;

    // Mutex for thread-safe logging
    std::mutex mtx_;
};

#endif // LOGGER_HPP
