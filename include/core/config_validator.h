/*
 * ThemisDB | File: config_validator.h | Version: 0.0.47 | Last Modified: 2026-05-20 19:53:17
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 93/100 | Lines: 276
 * Gap Summary: total=13; TODO=1, Stub=11, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #4481 feat(core): implement IHeal... (2026-04-09) | #3899 feat(auth): Mandatory JWT I... (2026-03-12) | #3570 feat(core): dynamic log lev... (2026-03-12) | #2841 [WIP] Add plugin-based adap... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
            
            if (!config.expected_issuer.has_value()) {
                result.addError("JWT expected_issuer is required in production mode");
            }
            
            if (!config.expected_audience.has_value()) {
                if (config.require_audience_validation) {
                    result.addError("JWT expected_audience is required in production mode");
                } else {
                    result.addWarning("JWT expected_audience not set - audience validation disabled");
                }
            }
        } else {
            // In development, just warnings
            if (config.jwks_url.empty()) {
                result.addWarning("JWT jwks_url not set");
            }
            
            if (!config.expected_issuer.has_value()) {
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
     * @brief Validate adapter type configuration
     * 
     * Validates that the adapter type strings in ConcernsContext::Config refer
     * to known, supported adapters.
     *
     * @param logger_adapter           Value of Config::loggerAdapter
     * @param tracer_adapter           Value of Config::tracerAdapter (empty = auto)
     * @param metrics_adapter          Value of Config::metricsAdapter (empty = auto)
     * @param cache_adapter            Value of Config::cacheAdapter
     * @param circuit_breaker_adapter  Value of Config::circuitBreakerAdapter
     * @param feature_flags_adapter    Value of Config::featureFlagsAdapter
     * @param audit_adapter            Value of Config::auditAdapter
     * @param secrets_adapter          Value of Config::secretsAdapter
     */
    static ValidationResult validateAdapterConfig(
        const std::string& logger_adapter,
        const std::string& tracer_adapter,
        const std::string& metrics_adapter,
        const std::string& cache_adapter,
        const std::string& circuit_breaker_adapter = "default",
        const std::string& feature_flags_adapter   = "inmemory",
        const std::string& audit_adapter            = "noop",
        const std::string& secrets_adapter          = "noop",
        const std::string& cache_redis_url          = "")
    {
        ValidationResult result;

        const std::vector<std::string> valid_logger_adapters          = {"spdlog", "noop"};
        const std::vector<std::string> valid_tracer_adapters          = {"otel", "jaeger", "zipkin", "noop", ""};
        const std::vector<std::string> valid_metrics_adapters         = {"prometheus", "noop", ""};
        const std::vector<std::string> valid_cache_adapters           = {"inmemory", "noop", "redis"};
        const std::vector<std::string> valid_circuit_breaker_adapters = {"default", "noop"};
        const std::vector<std::string> valid_feature_flags_adapters   = {"inmemory", "noop"};
        const std::vector<std::string> valid_audit_adapters           = {"inmemory", "noop"};
        const std::vector<std::string> valid_secrets_adapters         = {"noop", "inmemory", "env"};

        auto check = [&](const std::string& value,
                         const std::vector<std::string>& valid_values,
                         const std::string& field_name) {
            for (const auto& v : valid_values) {
                if (value == v) return;
            }
            std::string allowed;
            for (size_t i = 0; i < valid_values.size(); ++i) {
                if (i > 0) allowed += ", ";
                allowed += valid_values[i].empty() ? "(empty)" : valid_values[i];
            }
            result.addError("Unknown " + field_name + " adapter: '" + value +
                            "'. Supported values: " + allowed);
        };

        check(logger_adapter,          valid_logger_adapters,          "loggerAdapter");
        check(tracer_adapter,          valid_tracer_adapters,          "tracerAdapter");
        check(metrics_adapter,         valid_metrics_adapters,         "metricsAdapter");
        check(cache_adapter,           valid_cache_adapters,           "cacheAdapter");
        check(circuit_breaker_adapter, valid_circuit_breaker_adapters, "circuitBreakerAdapter");
        check(feature_flags_adapter,   valid_feature_flags_adapters,   "featureFlagsAdapter");
        check(audit_adapter,           valid_audit_adapters,           "auditAdapter");
        check(secrets_adapter,         valid_secrets_adapters,         "secretsAdapter");

        // Redis cache adapter requires an explicit endpoint to avoid
        // silently falling back to in-memory cache behavior.
        if (cache_adapter == "redis" && cache_redis_url.empty()) {
            result.addError("cacheAdapter 'redis' requires a non-empty cacheRedisUrl");
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
