/**
 * @file config_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=13; TODO=1, Stub=11, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
     * @brief Validation result returned by configuration checks.
     *
     * The result is considered valid only when valid is true and errors is
     * empty. Warnings do not invalidate the configuration.
     */
    struct ValidationResult {
        bool valid = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        
        /**
         * @brief Record a validation error and mark the result invalid.
         *
         * @param error Human-readable error message.
         */
        void addError(const std::string& error) {
            valid = false;
            errors.push_back(error);
        }
        
        /**
         * @brief Record a non-fatal validation warning.
         *
         * @param warning Human-readable warning message.
         */
        void addWarning(const std::string& warning) {
            warnings.push_back(warning);
        }
        
        /**
         * @brief Format all validation messages into a single text block.
         *
         * Errors are emitted before warnings so callers can present the most
         * important failures first.
         *
         * @return Multi-line summary string suitable for exception messages or
         *         log output.
         */
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
     *
     * Requires `vault_addr` and `vault_token`. The address must use HTTP or
     * HTTPS. Missing optional keys are reported as warnings when they imply a
     * production-hardened default.
     *
     * @param config JSON configuration object to validate.
     * @return ValidationResult with errors for invalid input and warnings for
     *         risky defaults.
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
     *
     * In production mode the validator requires a JWKS URL and issuer, and it
     * requires an audience when audience validation is enabled. In development
     * mode the same gaps are downgraded to warnings so the caller can bootstrap
     * local test environments.
     *
     * @param config JWT validator configuration to validate.
     * @param production_mode Whether production enforcement rules should apply.
     * @return ValidationResult capturing hard failures and soft warnings.
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
     *
     * Accepts only the canonical log levels used by the runtime logging stack.
     * An empty pattern is allowed but reported as a warning so callers can fall
     * back to the default formatter explicitly.
     *
     * @param log_level   Requested log level string.
     * @param log_pattern  Pattern string for the logger backend.
     * @return ValidationResult with an error for an unknown level and warnings
     *         for empty or risky settings.
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
     *
     * Tracing is only valid when the feature is enabled and the endpoint is
     * provided. Missing service names are treated as warnings because the
     * backend can often inject a fallback identifier.
     *
     * @param enabled      Whether tracing is enabled.
     * @param endpoint     OpenTelemetry or collector endpoint.
     * @param service_name  Logical service name used in spans.
     * @return ValidationResult describing configuration issues.
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
     * @param cache_redis_url          Redis endpoint required when cache is set
     *                                 to redis.
     * @return ValidationResult with explicit adapter-name errors and missing
     *         endpoint errors.
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
                if (value == v) {
                  return;
                }
            }
            std::string allowed;
            for (size_t i = 0; i < valid_values.size(); ++i) {
                if (i > 0) {
                  allowed += ", ";
                }
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
     *
     * Zero size disables the cache and very large capacities are reported as
     * warnings because they can increase memory pressure.
     *
     * @param max_size     Maximum number of entries retained by the cache.
     * @param default_ttl  Default time-to-live in seconds.
     * @return ValidationResult with warnings for degenerate or risky cache
     *         settings.
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
