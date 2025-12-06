#pragma once

#include "acceleration/compute_backend.h"
#include <string>
#include <optional>

namespace themis {
namespace acceleration {

/**
 * @brief Utility class for loading acceleration configuration from YAML files
 */
class ConfigLoader {
public:
    /**
     * @brief Load acceleration configuration from a YAML file
     * @param configPath Path to the acceleration.yaml file
     * @return Loaded configuration or default if file not found
     */
    static AccelerationConfig loadFromYAML(const std::string& configPath);
    
    /**
     * @brief Load acceleration configuration from JSON object
     * @param json JSON object containing acceleration config
     * @return Loaded configuration
     */
    static AccelerationConfig loadFromJSON(const void* json);
    
    /**
     * @brief Try to load configuration from default locations
     * @return Loaded configuration or default
     */
    static AccelerationConfig loadDefault();
    
private:
    static AccelerationPreference parsePreference(const std::string& prefer);
};

} // namespace acceleration
} // namespace themis
