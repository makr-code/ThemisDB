/**
 * @file judge_config.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/judge_config.h"
#include <stdexcept>
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>

using json = nlohmann::json;

namespace themis::rag::judge {

bool JudgeConfigManager::loadFromYAML(const std::string& filepath) {
    // Note: Simplified YAML parsing for basic key-value pairs
    // For complex YAML structures, use yaml-cpp library
    // This implementation supports:
    // - Simple key: value pairs
    // - Single-level nesting (section headers)
    // - Comments (lines starting with #)
    // Limitations:
    // - Does not support multi-level nesting
    // - Does not support arrays or complex YAML features
    // - Indentation-based section detection only
    
    THEMIS_INFO("Loading judge configuration from YAML: {}", filepath);
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        THEMIS_ERROR("Failed to open configuration file: {}", filepath);
        return false;
    }
    
    // Simple YAML-like parsing (key: value format)
    std::string line = {};
    std::string current_section = {};
    
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Detect section headers (indentation-based)
        size_t indent = line.find_first_not_of(" \t");
        if (indent == std::string::npos) {
          continue;
        }
        
        line = line.substr(indent);
        
        // Parse key: value
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            
            // Trim whitespace
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            if (indent == 0) {
                current_section = key;
            } else {
                std::string full_key = current_section.empty() ? key : current_section + "." + key;
                config_[full_key] = value;
            }
        }
    }
    
    THEMIS_INFO("Loaded {} configuration entries",static_cast<int>(config_.size()));
    return validate();
}

bool JudgeConfigManager::loadFromJSON(const std::string& filepath) {
    THEMIS_INFO("Loading judge configuration from JSON: {}", filepath);
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        THEMIS_ERROR("Failed to open configuration file: {}", filepath);
        return false;
    }
    
    try {
        json j;
        file >> j;
        return loadFromJSONString(j.dump());
    } catch (const json::exception& e) {
        THEMIS_ERROR("Failed to parse JSON configuration: {}", e.what());
        return false;
    }
}

bool JudgeConfigManager::loadFromJSONString(const std::string& json_str) {
    try {
        json j = json::parse(json_str);
        config_.clear();
        
        // Flatten JSON to dotted keys
        std::function<void(const json&, const std::string&)> flatten;
        flatten = [&](const json& obj, const std::string& prefix) {
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                std::string key = prefix.empty() ? it.key() : prefix + "." + it.key();
                
                if (it.value().is_object()) {
                    flatten(it.value(), key);
                } else if (it.value().is_string()) {
                    config_[key] = it.value().get<std::string>();
                } else if (it.value().is_number()) {
                    config_[key] = std::to_string(it.value().get<double>());
                } else if (it.value().is_boolean()) {
                    config_[key] = it.value().get<bool>() ? "true" : "false";
                }
            }
        };
        
        if (j.is_object()) {
            flatten(j, "");
        }
        
        THEMIS_INFO("Loaded {} configuration entries from JSON",static_cast<int>(config_.size()));
        return validate();
    } catch (const json::exception& e) {
        THEMIS_ERROR("Failed to parse JSON configuration: {}", e.what());
        return false;
    }
}

bool JudgeConfigManager::updateConfig(const std::string& key, const std::string& value) {
    config_[key] = value;
    THEMIS_DEBUG("Updated configuration: {} = {}", key, value);
    return true;
}

bool JudgeConfigManager::validate() {
    // Validate required keys and value ranges
    bool valid = true;
    
    // Check scoring weights sum to ~1.0
    double total_weight = 
        getDouble("scoring.faithfulness_weight", 0.4) +
        getDouble("scoring.relevance_weight", 0.3) +
        getDouble("scoring.completeness_weight", 0.2) +
        getDouble("scoring.coherence_weight", 0.1);
    
    if (std::abs(total_weight - 1.0) > 0.01) {
        THEMIS_WARN("Scoring weights sum to {}, expected 1.0", total_weight);
        valid = false;
    }
    
    // Validate thresholds are in [0, 1]
    double quality_threshold = getDouble("quality_threshold", 0.7);
    if (quality_threshold < 0.0 || quality_threshold > 1.0) {
        THEMIS_ERROR("quality_threshold must be in [0, 1], got {}", quality_threshold);
        valid = false;
    }
    
    return valid;
}

std::optional<std::string> JudgeConfigManager::get(const std::string& key) const {
    auto it = config_.find(key);
    if (it != config_.end()) {
        return it->second;
    }
    return std::nullopt;
}

double JudgeConfigManager::getDouble(const std::string& key, double default_value) const {
    auto value = get(key);
    if (value) {
        try {
            return std::stod(*value);
        } catch (...) {
            THEMIS_WARN("Failed to parse '{}' as double, using default: {}", key, default_value);
        }
    }
    return default_value;
}

int JudgeConfigManager::getInt(const std::string& key, int default_value) const {
    auto value = get(key);
    if (value) {
        try {
            return std::stoi(*value);
        } catch (...) {
            THEMIS_WARN("Failed to parse '{}' as int, using default: {}", key, default_value);
        }
    }
    return default_value;
}

bool JudgeConfigManager::getBool(const std::string& key, bool default_value) const {
    auto value = get(key);
    if (value) {
        std::string v = *value;
        std::transform(v.begin(), v.end(), v.begin(), ::tolower);
        return v == "true" || v == "1" || v == "yes";
    }
    return default_value;
}

std::string JudgeConfigManager::getString(const std::string& key, const std::string& default_value) const {
    auto value = get(key);
    return value.value_or(default_value);
}

std::string JudgeConfigManager::toJSON() const {
    json j;
    
    for (const auto& [key, value] : config_) {
        // Convert dotted keys back to nested JSON
        std::vector<std::string> parts = splitKey(key);
        json* current = &j;
        
        for (size_t i = 0; i < parts.size() - 1; ++i) {
            if (!(*current).contains(parts[i])) {
                (*current)[parts[i]] = json::object();
            }
            current = &(*current)[parts[i]];
        }
        
        (*current)[parts.back()] = value;
    }
    
    return j.dump(2);
}

bool JudgeConfigManager::has(const std::string& key) const {
    return config_.find(key) != config_.end();
}

void JudgeConfigManager::clear() {
    config_.clear();
}

std::vector<std::string> JudgeConfigManager::splitKey(const std::string& key) const {
    std::vector<std::string> parts;
    std::istringstream stream(key);
    std::string part = {};
    
    while (std::getline(stream, part, '.')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    
    return parts;
}

} // namespace themis::rag::judge

