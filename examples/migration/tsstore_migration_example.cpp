/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tsstore_migration_example.cpp                      ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:22:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     372                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tsstore_migration_example.cpp
 * @brief Example migration from std::optional<T> pattern to Result<T>
 * 
 * This file demonstrates the migration pattern for converting TSStore
 * methods from using std::optional to using Result<T>.
 * 
 * NOTE: This is an EXAMPLE file showing migration patterns. It is not meant
 * to be compiled or used directly. Actual migration would modify the real
 * tsstore.cpp and tsstore.h files.
 */

#include "utils/expected.h"
#include "utils/error_registry.h"
#include <string>
#include <optional>
#include <cstdint>

namespace themis {
namespace migration_example {

/**
 * BEFORE: Original implementation using std::optional<T>
 * 
 * Problems:
 * - No error information when parsing fails
 * - Caller cannot distinguish between different failure reasons
 * - No context about what went wrong
 * - Difficult to debug parse failures
 */
class TSStore_Old {
public:
    struct KeyComponents {
        std::string table;
        int64_t timestamp;
        std::string field;
    };
    
private:
    static constexpr const char* KEY_PREFIX = "ts:";
    
public:
    /**
     * Parse a time series key into components
     * 
     * Returns: std::optional with value on success, nullopt on failure
     */
    std::optional<KeyComponents> parseKey(const std::string& key) const {
        // Check prefix
        if (key.compare(0, strlen(KEY_PREFIX), KEY_PREFIX) != 0) {
            return std::nullopt;  // ❌ Why did it fail? Wrong prefix? Empty key?
        }
        
        // Find delimiters
        size_t first_colon = key.find(':', strlen(KEY_PREFIX));
        if (first_colon == std::string::npos) {
            return std::nullopt;  // ❌ No context about malformed key!
        }
        
        size_t second_colon = key.find(':', first_colon + 1);
        if (second_colon == std::string::npos) {
            return std::nullopt;  // ❌ No context about malformed key!
        }
        
        // Extract components
        KeyComponents comp;
        comp.table = key.substr(strlen(KEY_PREFIX), first_colon - strlen(KEY_PREFIX));
        
        // Parse timestamp
        std::string ts_str = key.substr(first_colon + 1, second_colon - first_colon - 1);
        try {
            comp.timestamp = std::stoll(ts_str);
        } catch (...) {
            return std::nullopt;  // ❌ No context about invalid timestamp!
        }
        
        comp.field = key.substr(second_colon + 1);
        
        // Validation
        if (comp.table.empty() || comp.field.empty()) {
            return std::nullopt;  // ❌ Which component was empty?
        }
        
        return comp;
    }
    
    /**
     * Get timestamp from key
     * 
     * Returns: std::optional with timestamp on success, nullopt on failure
     */
    std::optional<int64_t> extractTimestamp(const std::string& key) const {
        auto components = parseKey(key);
        if (!components) {
            return std::nullopt;  // ❌ Cascading nullopts lose error context!
        }
        return components->timestamp;
    }
};

/**
 * AFTER: Migrated implementation using Result<T>
 * 
 * Benefits:
 * - Detailed error information for each failure case
 * - Structured error codes for programmatic handling
 * - Rich context about what went wrong and how to fix it
 * - Error propagation preserves context
 * - Type-safe and composable
 */
class TSStore_New {
public:
    struct KeyComponents {
        std::string table;
        int64_t timestamp;
        std::string field;
    };
    
private:
    static constexpr const char* KEY_PREFIX = "ts:";
    
public:
    /**
     * Parse a time series key into components
     * 
     * @param key Time series key in format "ts:table:timestamp:field"
     * @return Result containing KeyComponents or structured error
     * 
     * Possible errors:
     * - ERR_API_INVALID_REQUEST: Empty key or missing prefix
     * - ERR_QUERY_PARSE_FAILED: Malformed key structure
     * - ERR_SCHEMA_INVALID_TYPE: Invalid timestamp format
     * - ERR_QUERY_INVALID_SYNTAX: Empty table or field name
     */
    Result<KeyComponents> parseKey(const std::string& key) const {
        // Check empty key
        if (key.empty()) {
            return Err<KeyComponents>(
                errors::ErrorCode::ERR_API_INVALID_REQUEST,
                "Time series key is empty"
            );
        }
        
        // Check prefix
        if (key.compare(0, strlen(KEY_PREFIX), KEY_PREFIX) != 0) {
            return Err<KeyComponents>(
                errors::ErrorCode::ERR_API_INVALID_REQUEST,
                fmt::format("Key must start with '{}' prefix, got: '{}'", 
                           KEY_PREFIX, key.substr(0, 10))
            );
        }
        
        // Find delimiters
        size_t first_colon = key.find(':', strlen(KEY_PREFIX));
        if (first_colon == std::string::npos) {
            return Err<KeyComponents>(
                errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
                fmt::format("Malformed key: missing first delimiter in '{}'", key)
            );
        }
        
        size_t second_colon = key.find(':', first_colon + 1);
        if (second_colon == std::string::npos) {
            return Err<KeyComponents>(
                errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
                fmt::format("Malformed key: missing second delimiter in '{}'", key)
            );
        }
        
        // Extract components
        KeyComponents comp;
        comp.table = key.substr(strlen(KEY_PREFIX), first_colon - strlen(KEY_PREFIX));
        
        // Parse timestamp
        std::string ts_str = key.substr(first_colon + 1, second_colon - first_colon - 1);
        try {
            comp.timestamp = std::stoll(ts_str);
        } catch (const std::exception& e) {
            return Err<KeyComponents>(
                errors::ErrorCode::ERR_SCHEMA_INVALID_TYPE,
                fmt::format("Invalid timestamp '{}' in key: {}", ts_str, e.what())
            );
        }
        
        comp.field = key.substr(second_colon + 1);
        
        // Validation
        if (comp.table.empty()) {
            return Err<KeyComponents>(
                errors::ErrorCode::ERR_QUERY_INVALID_SYNTAX,
                "Table name cannot be empty in time series key"
            );
        }
        
        if (comp.field.empty()) {
            return Err<KeyComponents>(
                errors::ErrorCode::ERR_QUERY_INVALID_SYNTAX,
                "Field name cannot be empty in time series key"
            );
        }
        
        return Ok(std::move(comp));
    }
    
    /**
     * Get timestamp from key
     * 
     * @param key Time series key
     * @return Result containing timestamp or structured error (same errors as parseKey)
     */
    Result<int64_t> extractTimestamp(const std::string& key) const {
        // Error context is preserved through the chain!
        return parseKey(key)
            .and_then([](const KeyComponents& comp) -> Result<int64_t> {
                return Ok(comp.timestamp);
            });
    }
    
    /**
     * Validate key format without full parsing
     * 
     * @param key Time series key to validate
     * @return Result<void> indicating if key is valid
     */
    Result<void> validateKey(const std::string& key) const {
        return parseKey(key)
            .and_then([](const KeyComponents&) -> Result<void> {
                return OkVoid();
            });
    }
};

/**
 * Example usage showing caller-side improvements
 */
void example_usage() {
    TSStore_New ts_store;
    
    // Example 1: Parse valid key
    std::string valid_key = "ts:metrics:1234567890:cpu_usage";
    auto result1 = ts_store.parseKey(valid_key);
    
    if (result1) {
        const auto& comp = *result1;
        spdlog::info("Parsed key: table={}, timestamp={}, field={}",
                    comp.table, comp.timestamp, comp.field);
    } else {
        spdlog::error("Parse failed: {}", result1.error().message());
    }
    
    // Example 2: Parse invalid key with detailed error
    std::string invalid_key = "ts:metrics:not_a_number:cpu_usage";
    auto result2 = ts_store.parseKey(invalid_key);
    
    if (!result2) {
        const Error& err = result2.error();
        spdlog::error("Parse error: {}", err.message());
        spdlog::error("Error code: {}", static_cast<int>(err.code()));
        
        // Get solution from error registry
        auto metadata = err.metadata();
        spdlog::info("Solution:\n{}", metadata.solution);
        
        // Handle specific error types
        switch (err.code()) {
            case errors::ErrorCode::ERR_SCHEMA_INVALID_TYPE:
                // Invalid timestamp format - might be corrupted data
                spdlog::warn("Skipping corrupted key: {}", invalid_key);
                break;
                
            case errors::ErrorCode::ERR_API_INVALID_REQUEST:
                // Missing prefix - might be wrong key type
                spdlog::warn("Not a time series key: {}", invalid_key);
                break;
                
            default:
                break;
        }
    }
    
    // Example 3: Chain operations with error propagation
    auto timestamp_result = ts_store.extractTimestamp(valid_key);
    
    if (timestamp_result) {
        spdlog::info("Timestamp: {}", *timestamp_result);
    } else {
        // Error from parseKey is automatically propagated!
        spdlog::error("Failed to extract timestamp: {}", 
                     timestamp_result.error().message());
    }
    
    // Example 4: Validate key
    auto validation = ts_store.validateKey("ts:sensors:1000:temperature");
    if (validation) {
        spdlog::info("Key is valid");
    } else {
        spdlog::error("Invalid key: {}", validation.error().message());
    }
    
    // Example 5: Convert from old optional API to Result
    // Helper function for migration
    auto convert_optional = [](const std::optional<int64_t>& opt, 
                               const std::string& context) -> Result<int64_t> {
        if (opt.has_value()) {
            return Ok(*opt);
        }
        return Err<int64_t>(
            errors::ErrorCode::ERR_QUERY_PARSE_FAILED,
            context
        );
    };
    
    // Use conversion helper during gradual migration
    std::optional<int64_t> legacy_optional = std::nullopt;
    Result<int64_t> migrated = convert_optional(legacy_optional, "Legacy parse failed");
    
    // Example 6: Use value_or for default handling
    int64_t timestamp = ts_store.extractTimestamp("invalid")
        .value_or(0);  // Default to 0 if parse fails
    
    // Example 7: Multiple operations with error accumulation
    auto process_key = [&](const std::string& key) -> Result<std::string> {
        return ts_store.parseKey(key)
            .and_then([](const KeyComponents& comp) -> Result<KeyComponents> {
                // Validate timestamp is in range
                if (comp.timestamp < 0) {
                    return Err<KeyComponents>(
                        errors::ErrorCode::ERR_SCHEMA_INVALID_TYPE,
                        "Timestamp cannot be negative"
                    );
                }
                return Ok(comp);
            })
            .and_then([](const KeyComponents& comp) -> Result<std::string> {
                // Format result
                return Ok(fmt::format("{}:{}", comp.table, comp.field));
            });
    };
    
    auto formatted = process_key(valid_key);
    if (formatted) {
        spdlog::info("Processed: {}", *formatted);
    } else {
        spdlog::error("Processing failed: {}", formatted.error().message());
    }
}

} // namespace migration_example
} // namespace themis
