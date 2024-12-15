#ifndef LOG_HPP
#define LOG_HPP

#include <string>

namespace Log {
    /**
     * @brief Log a debug message.
     * @param message The message to log.
     * @param file The source file name (optional).
     * @param line The line number in the source file (optional).
     */
    void debug(const std::string& message, const char* file = "", int line = 0);

    /**
     * @brief Log an informational message.
     * @param message The message to log.
     * @param file The source file name (optional).
     * @param line The line number in the source file (optional).
     */
    void info(const std::string& message, const char* file = "", int line = 0);

    /**
     * @brief Log a warning message.
     * @param message The message to log.
     * @param file The source file name (optional).
     * @param line The line number in the source file (optional).
     */
    void warn(const std::string& message, const char* file = "", int line = 0);

    /**
     * @brief Log an error message.
     * @param message The message to log.
     * @param file The source file name (optional).
     * @param line The line number in the source file (optional).
     */
    void error(const std::string& message, const char* file = "", int line = 0);
}

#endif // LOG_HPP
