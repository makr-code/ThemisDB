#include "acceleration/config_loader.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <filesystem>

namespace themis {
namespace acceleration {

using json = nlohmann::json;

AccelerationPreference ConfigLoader::parsePreference(const std::string& prefer) {
    if (prefer == "gpu") {
        return AccelerationPreference::GPU;
    } else if (prefer == "cpu") {
        return AccelerationPreference::CPU;
    } else {
        return AccelerationPreference::AUTO;
    }
}

AccelerationConfig ConfigLoader::loadFromYAML(const std::string& configPath) {
    AccelerationConfig config;
    
    try {
        if (!std::filesystem::exists(configPath)) {
            THEMIS_WARN("Acceleration config not found at {}, using defaults", configPath);
            return config;
        }
        
        YAML::Node root = YAML::LoadFile(configPath);
        if (!root["acceleration"]) {
            THEMIS_WARN("No 'acceleration' section in {}, using defaults", configPath);
            return config;
        }
        
        YAML::Node accel = root["acceleration"];
        
        // Parse 'prefer' option (new)
        if (accel["prefer"]) {
            config.prefer = parsePreference(accel["prefer"].as<std::string>());
        }
        
        // Parse 'gpu_fallback' option (new)
        if (accel["gpu_fallback"]) {
            config.gpuFallback = accel["gpu_fallback"].as<bool>();
        }
        
        // Parse 'min_batch_size' option (new)
        if (accel["min_batch_size"]) {
            config.minBatchSize = accel["min_batch_size"].as<size_t>();
        }
        
        // Legacy: mode=DISABLED means CPU preference
        if (accel["mode"]) {
            std::string mode = accel["mode"].as<std::string>();
            if (mode == "DISABLED") {
                config.prefer = AccelerationPreference::CPU;
            }
        }
        
        THEMIS_INFO("Loaded acceleration config from {}: prefer={}, gpu_fallback={}, min_batch_size={}", 
            configPath, static_cast<int>(config.prefer), config.gpuFallback, config.minBatchSize);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to load acceleration config from {}: {}", configPath, e.what());
    }
    
    return config;
}

AccelerationConfig ConfigLoader::loadFromJSON(const void* jsonPtr) {
    AccelerationConfig config;
    
    try {
        const json* j = static_cast<const json*>(jsonPtr);
        if (!j || !j->contains("acceleration")) {
            return config;
        }
        
        const json& accel = (*j)["acceleration"];
        
        // Parse 'prefer' option
        if (accel.contains("prefer")) {
            config.prefer = parsePreference(accel["prefer"].get<std::string>());
        }
        
        // Parse 'gpu_fallback' option
        if (accel.contains("gpu_fallback")) {
            config.gpuFallback = accel["gpu_fallback"].get<bool>();
        }
        
        // Parse 'min_batch_size' option
        if (accel.contains("min_batch_size")) {
            config.minBatchSize = accel["min_batch_size"].get<size_t>();
        }
        
        // Legacy: mode=DISABLED means CPU preference
        if (accel.contains("mode")) {
            std::string mode = accel["mode"].get<std::string>();
            if (mode == "DISABLED") {
                config.prefer = AccelerationPreference::CPU;
            }
        }
        
        THEMIS_INFO("Loaded acceleration config from JSON: prefer={}, gpu_fallback={}, min_batch_size={}", 
            static_cast<int>(config.prefer), config.gpuFallback, config.minBatchSize);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to load acceleration config from JSON: {}", e.what());
    }
    
    return config;
}

AccelerationConfig ConfigLoader::loadDefault() {
    // Try standard locations
    std::vector<std::string> searchPaths = {
        "./config/acceleration.yaml",
        "./acceleration.yaml",
        "/etc/themisdb/acceleration.yaml",
        "/etc/vccdb/acceleration.yaml"
    };
    
    for (const auto& path : searchPaths) {
        if (std::filesystem::exists(path)) {
            THEMIS_INFO("Found acceleration config at {}", path);
            return loadFromYAML(path);
        }
    }
    
    THEMIS_INFO("No acceleration config found, using defaults");
    return AccelerationConfig();
}

} // namespace acceleration
} // namespace themis
