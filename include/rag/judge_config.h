/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            judge_config.h                                     ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:04:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     146                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 67965456c8  2026-03-22  Add constructors with default config for various classes ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file judge_config.h
 * @brief Configuration management for RAG Judge
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
