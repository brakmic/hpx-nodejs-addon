#ifndef LOG_CONFIG_HPP
#define LOG_CONFIG_HPP

#include "log_levels.hpp"

// Set the current log level based on defined macros
#if defined(HPX_ADDON_RELEASE)
    #define CURRENT_LOG_LEVEL LOG_LEVEL_INFO
#elif defined(HPX_ADDON_DEBUG)
    #define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG
#elif defined(HPX_ADDON_WARN_ONLY)
    #define CURRENT_LOG_LEVEL LOG_LEVEL_WARN
#else
    // Default log level if none is defined
    #define CURRENT_LOG_LEVEL LOG_LEVEL_INFO
#endif

#endif // LOG_CONFIG_HPP
