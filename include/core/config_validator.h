/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            config_validator.h                                 ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     219                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "auth/jwt_validator.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace themis {
namespace core {

/**
 * @brief Configuration validation utilities
 * 
 * Provides schema validation for security and observability configurations
 */
class ConfigValidator {
public:
    /**
     * @brief Validation result
     */
    struct ValidationResult {
        bool valid = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        
        void addError(const std::string& error) {
            valid = false;
            errors.push_back(error);
        }
        
        void addWarning(const std::string& warning) {
            warnings.push_back(warning);
        }
        
        std::string formatErrors() const {
            std::string result;
            for (const auto& error : errors) {
                result += "ERROR: " + error + "\n";
            }
            for (const auto& warning : warnings) {
                result += "WARNING: " + warning + "\n";
            }
            return result;
        }
    };
    
    /**
     * @brief Validate Vault key provider configuration
     */
    static ValidationResult validateVaultConfig(const nlohmann::json& config) {
        ValidationResult result;
        
        // Required fields
        if (!config.contains("vault_addr") || config["vault_addr"].get<std::string>().empty()) {
            result.addError("vault_addr is required and cannot be empty");
        } else {
            const std::string& addr = config["vault_addr"].get<std::string>();
            if (addr.find("http://") != 0 && addr.find("https://") != 0) {
                result.addError("vault_addr must start with http:// or https://");
            }
        }
        
        if (!config.contains("vault_token") || config["vault_token"].get<std::string>().empty()) {
            result.addError("vault_token is required and cannot be empty");
        }
        
        // Optional but recommended
        if (!config.contains("kv_mount_path")) {
            result.addWarning("kv_mount_path not specified, using default 'secret'");
        }
        
        // TLS verification
        if (config.contains("tls_skip_verify") && config["tls_skip_verify"].get<bool>()) {
            result.addWarning("TLS verification disabled - not recommended for production");
        }
        
        return result;
    }
    
    /**
     * @brief Validate JWT configuration
     */
    static ValidationResult validateJWTConfig(const auth::JWTValidatorConfig& config, bool production_mode) {
        ValidationResult result;
        
        if (production_mode) {
            // In production, strict validation
            if (config.jwks_url.empty()) {
                result.addError("JWT jwks_url is required in production mode");
            }
            
            if (config.expected_issuer.empty()) {
                result.addError("JWT expected_issuer is required in production mode");
            }
            
            if (config.expected_audience.empty()) {
                result.addWarning("JWT expected_audience not set - audience validation disabled");
            }
        } else {
            // In development, just warnings
            if (config.jwks_url.empty()) {
                result.addWarning("JWT jwks_url not set");
            }
            
            if (config.expected_issuer.empty()) {
                result.addWarning("JWT expected_issuer not set - issuer validation disabled");
            }
        }
        
        // Check reasonable cache TTL
        if (config.cache_ttl.count() < 60) {
            result.addWarning("JWT cache_ttl is very short (< 60s) - may impact performance");
        }
        
        if (config.cache_ttl.count() > 3600) {
            result.addWarning("JWT cache_ttl is very long (> 1h) - may delay key rotation");
        }
        
        return result;
    }
    
    /**
     * @brief Validate logging configuration
     */
    static ValidationResult validateLogConfig(const std::string& log_level, const std::string& log_pattern) {
        ValidationResult result;
        
        // Validate log level
        const std::vector<std::string> valid_levels = {"trace", "debug", "info", "warn", "error", "critical", "off"};
        bool level_valid = false;
        for (const auto& level : valid_levels) {
            if (log_level == level) {
                level_valid = true;
                break;
            }
        }
        
        if (!level_valid) {
            result.addError("Invalid log_level: '" + log_level + "'. Must be one of: trace, debug, info, warn, error, critical, off");
        }
        
        // Pattern validation (basic check)
        if (log_pattern.empty()) {
            result.addWarning("log_pattern is empty - using default format");
        }
        
        return result;
    }
    
    /**
     * @brief Validate tracing configuration
     */
    static ValidationResult validateTracingConfig(bool enabled, const std::string& endpoint, const std::string& service_name) {
        ValidationResult result;
        
        if (enabled) {
            if (endpoint.empty()) {
                result.addError("tracing_endpoint is required when tracing is enabled");
            }
            
            if (service_name.empty()) {
                result.addWarning("tracing_service_name is empty - using default");
            }
        }
        
        return result;
    }
    
    /**
     * @brief Validate cache configuration
     */
    static ValidationResult validateCacheConfig(size_t max_size, uint64_t default_ttl) {
        ValidationResult result;
        
        if (max_size == 0) {
            result.addWarning("cache_max_size is 0 - cache effectively disabled");
        }
        
        if (max_size > 1000000) {
            result.addWarning("cache_max_size is very large (> 1M entries) - may consume significant memory");
        }
        
        if (default_ttl > 0 && default_ttl < 60) {
            result.addWarning("cache_default_ttl is very short (< 60s) - may impact performance");
        }
        
        return result;
    }
};

} // namespace core
} // namespace themis
