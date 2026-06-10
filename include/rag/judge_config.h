/**
 * @file judge_config.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <vector>

namespace themis::rag::judge {

/**
 * @brief Configuration loader and manager for RAG Judge
 * 
 * Supports YAML/JSON configuration loading with runtime updates
 * and validation according to the schema.
 */
class JudgeConfigManager {
public:
    /**
     * @brief Load configuration from YAML file
     * @param filepath Path to YAML configuration file
     * @return true if loaded successfully
     */
    bool loadFromYAML(const std::string& filepath);
    
    /**
     * @brief Load configuration from JSON file
     * @param filepath Path to JSON configuration file
     * @return true if loaded successfully
     */
    bool loadFromJSON(const std::string& filepath);
    
    /**
     * @brief Load configuration from JSON string
     * @param json_str JSON string content
     * @return true if loaded successfully
     */
    bool loadFromJSONString(const std::string& json_str);
    
    /**
     * @brief Update configuration at runtime
     * @param key Configuration key (supports dot notation: "scoring.faithfulness_weight")
     * @param value New value
     * @return true if updated successfully
     */
    bool updateConfig(const std::string& key, const std::string& value);
    
    /**
     * @brief Validate configuration against schema
     * @return true if configuration is valid
     */
    bool validate();
    
    /**
     * @brief Get configuration value
     * @param key Configuration key
     * @return Optional value string
     */
    std::optional<std::string> get(const std::string& key) const;
    
    /**
     * @brief Get configuration value as double
     * @param key Configuration key
     * @param default_value Default value if key not found
     * @return Configuration value
     */
    double getDouble(const std::string& key, double default_value = 0.0) const;
    
    /**
     * @brief Get configuration value as int
     * @param key Configuration key
     * @param default_value Default value if key not found
     * @return Configuration value
     */
    int getInt(const std::string& key, int default_value = 0) const;
    
    /**
     * @brief Get configuration value as bool
     * @param key Configuration key
     * @param default_value Default value if key not found
     * @return Configuration value
     */
    bool getBool(const std::string& key, bool default_value = false) const;
    
    /**
     * @brief Get configuration value as string
     * @param key Configuration key
     * @param default_value Default value if key not found
     * @return Configuration value
     */
    std::string getString(const std::string& key, const std::string& default_value = "") const;
    
    /**
     * @brief Get all configuration as JSON string
     * @return JSON representation of configuration
     */
    std::string toJSON() const;
    
    /**
     * @brief Check if configuration has key
     * @param key Configuration key
     * @return true if key exists
     */
    bool has(const std::string& key) const;
    
    /**
     * @brief Clear all configuration
     */
    void clear();

private:
    std::unordered_map<std::string, std::string> config_;
    
    // Helper to parse nested keys
    std::vector<std::string> splitKey(const std::string& key) const;
};

} // namespace themis::rag::judge
