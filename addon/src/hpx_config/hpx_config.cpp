#include "hpx_config.hpp"
#include "logger.hpp"
#include "data_conversion.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <algorithm>

// Global user config
static HPXUserConfig g_user_config;

// Function to set thread count
void SetThreadCount(size_t count) {
    g_user_config.threadCount = count;
}
// Convert Napi::Object to nlohmann::json internally
void SetUserConfigFromNapiObject(const Napi::Object& configObj) {
    nlohmann::json j = nlohmann::json::object();

    Napi::Array keys = configObj.GetPropertyNames();
    for (uint32_t i = 0; i < keys.Length(); i++) {
        std::string key = keys.Get(i).As<Napi::String>().Utf8Value();
        Napi::Value val = configObj.Get(key);
        if (val.IsString()) {
            j[key] = val.As<Napi::String>().Utf8Value();
        } else if (val.IsNumber()) {
            double num = val.As<Napi::Number>().DoubleValue();
            // If integral is needed, cast to int64_t if appropriate
            j[key] = num;
        } else if (val.IsBoolean()) {
            j[key] = val.As<Napi::Boolean>().Value();
        }
        // Other types can be handled here
    }

    // Parse fields from j into g_user_config
    if (j.contains("executionPolicy")) {
        std::string policy = j["executionPolicy"].get<std::string>();
        if (policy == "seq" || policy == "par" || policy == "par_unseq") {
            g_user_config.executionPolicy = policy;
        }
    }

    if (j.contains("threshold")) {
        int64_t t = j["threshold"].get<int64_t>();
        if (t > 0) {
            g_user_config.threshold = static_cast<size_t>(t);
        }
    }

    if (j.contains("threadCount")) {
        int64_t tc = j["threadCount"].get<int64_t>();
        if (tc > 0) {
            g_user_config.threadCount = static_cast<size_t>(tc);
        }
    }

    // Parse logging configurations
    if (j.contains("loggingEnabled")) {
        g_user_config.loggingEnabled = j["loggingEnabled"].get<bool>();
    }

    if (j.contains("logLevel")) {
        std::string l = j["logLevel"].get<std::string>();
        std::string level = ToUpperCase(l);

        if (level == "DEBUG" || level == "INFO" || level == "WARN" || level == "ERROR") {
            g_user_config.logLevel = level;
        } else {
            g_user_config.logLevel = "INFO";
        }
    }

    // Parse addonName if provided
    if (j.contains("addonName")) {
        std::string name = j["addonName"].get<std::string>();
        if (!name.empty()) {
            g_user_config.addonName = name;
        }
    }

    // Initialize Logger based on the configuration
    Logger::getInstance().initialize(
        g_user_config.loggingEnabled, 
        g_user_config.logLevel == "DEBUG" ? LogLevel::Debug :
        g_user_config.logLevel == "INFO" ? LogLevel::Info :
        g_user_config.logLevel == "WARN" ? LogLevel::Warn :
        g_user_config.logLevel == "ERROR" ? LogLevel::Error : LogLevel::Info
    );
}

const HPXUserConfig& GetUserConfig() {
    return g_user_config;
}
