#ifndef LOG_MACROS_HPP
#define LOG_MACROS_HPP

#include "log_config.hpp"
#include "log.hpp"
#include <sstream> // Include for std::ostringstream

// Define LOG_DEBUG
#if CURRENT_LOG_LEVEL <= LOG_LEVEL_DEBUG
    #define LOG_DEBUG(msg) \
        do { \
            std::ostringstream oss; \
            oss << msg; \
            Log::debug(oss.str(), __FILE__, __LINE__); \
        } while(0)
#else
    #define LOG_DEBUG(msg) do {} while(0)
#endif

// Define LOG_INFO
#if CURRENT_LOG_LEVEL <= LOG_LEVEL_INFO
    #define LOG_INFO(msg) \
        do { \
            std::ostringstream oss; \
            oss << msg; \
            Log::info(oss.str(), __FILE__, __LINE__); \
        } while(0)
#else
    #define LOG_INFO(msg) do {} while(0)
#endif

// Define LOG_WARN
#if CURRENT_LOG_LEVEL <= LOG_LEVEL_WARN
    #define LOG_WARN(msg) \
        do { \
            std::ostringstream oss; \
            oss << msg; \
            Log::warn(oss.str(), __FILE__, __LINE__); \
        } while(0)
#else
    #define LOG_WARN(msg) do {} while(0)
#endif

// Define LOG_ERROR
#if CURRENT_LOG_LEVEL <= LOG_LEVEL_ERROR
    #define LOG_ERROR(msg) \
        do { \
            std::ostringstream oss; \
            oss << msg; \
            Log::error(oss.str(), __FILE__, __LINE__); \
        } while(0)
#else
    #define LOG_ERROR(msg) do {} while(0)
#endif

#endif // LOG_MACROS_HPP
