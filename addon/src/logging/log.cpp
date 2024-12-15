#include "log.hpp"
#include "logger.hpp"

namespace Log {
    void debug(const std::string& message, const char* file, int line) {
        Logger::Debug(message, file, line);
    }

    void info(const std::string& message, const char* file, int line) {
        Logger::Info(message, file, line);
    }

    void warn(const std::string& message, const char* file, int line) {
        Logger::Warn(message, file, line);
    }

    void error(const std::string& message, const char* file, int line) {
        Logger::Error(message, file, line);
    }
}
