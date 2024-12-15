#ifndef HPX_CONFIG_HPP
#define HPX_CONFIG_HPP

#include <string>
#include <cstddef>
#include <napi.h>

struct HPXUserConfig {
    // HPX Runtime Configurations
    std::string executionPolicy = "par"; // "seq", "par", "par_unseq"
    size_t threshold = 10000;
    size_t threadCount = 2; // Default thread count (less than 2 makes no sense btw.)

    // Addon-specific Configurations
    bool loggingEnabled = true;          // Enable or disable logging
    std::string logLevel = "INFO";       // Logging level: INFO, DEBUG, WARN, ERROR
    std::string addonName = "hpxaddon";  // Addon name, default is "hpxaddon"
};

// Called by addon.cpp when user provides a config object.
void SetUserConfigFromNapiObject(const Napi::Object& configObj);

// Retrieve current config
const HPXUserConfig& GetUserConfig();

// Optionally, provide a setter for threadCount if needed elsewhere
void SetThreadCount(size_t count);

#endif // HPX_CONFIG_HPP
