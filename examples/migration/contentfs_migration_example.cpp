/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            contentfs_migration_example.cpp                    ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   94.0/100                                       ║
    • Total Lines:     383                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file contentfs_migration_example.cpp
 * @brief Example migration from Status{ok, message} pattern to Result<T>
 * 
 * This file demonstrates the migration pattern for converting ContentFS
 * methods from using Status struct to using Result<T>.
 * 
 * NOTE: This is an EXAMPLE file showing migration patterns. It is not meant
 * to be compiled or used directly. Actual migration would modify the real
 * content_fs.cpp and content_fs.h files.
 */

#include "utils/expected.h"
#include "utils/error_registry.h"
#include <string>
#include <vector>
#include <optional>

namespace themis {
namespace migration_example {

/**
 * BEFORE: Original implementation using Status struct
 * 
 * Problems:
 * - Status struct duplicated across multiple modules
 * - No machine-readable error codes
 * - Limited error context
 * - Cannot distinguish error types programmatically
 */
class ContentFS_Old {
public:
    // Old Status struct (local to this module)
    struct Status {
        bool ok = true;
        std::string message;
        
        static Status OK() { return {true, {}}; }
        static Status Error(std::string msg) { return {false, std::move(msg)}; }
        operator bool() const { return ok; }
    };
    
private:
    // Simplified storage simulation
    bool storage_initialized_ = true;
    
public:
    /**
     * Store content blob
     * 
     * Returns: Status indicating success or failure with message
     */
    Status put(const std::string& pk,
               const std::vector<uint8_t>& data,
               const std::string& mime,
               const std::optional<std::string>& sha256_expected_hex = std::nullopt) {
        
        // Validation error
        if (pk.empty()) {
            return Status::Error("put: pk must not be empty");  // ❌ No error code!
        }
        
        if (data.empty()) {
            return Status::Error("put: data must not be empty");  // ❌ No error code!
        }
        
        // Storage error
        if (!storage_initialized_) {
            return Status::Error("put: storage not initialized");  // ❌ No error code!
        }
        
        // Simulate write failure
        bool write_success = true; // would actually write to storage
        if (!write_success) {
            return Status::Error("put: failed to write blob");  // ❌ No error code!
        }
        
        return Status::OK();
    }
    
    /**
     * Retrieve content blob
     * 
     * Returns: Pair of Status and data (data empty on error)
     */
    std::pair<Status, std::vector<uint8_t>> get(const std::string& pk) {
        if (pk.empty()) {
            return {Status::Error("get: pk must not be empty"), {}};  // ❌ No error code!
        }
        
        if (!storage_initialized_) {
            return {Status::Error("get: storage not initialized"), {}};  // ❌ No error code!
        }
        
        // Simulate not found
        bool found = false;
        if (!found) {
            return {Status::Error("get: not found"), {}};  // ❌ No error code!
        }
        
        std::vector<uint8_t> data = {1, 2, 3};  // would read from storage
        return {Status::OK(), data};
    }
    
    /**
     * Delete content blob
     */
    Status del(const std::string& pk) {
        if (pk.empty()) {
            return Status::Error("del: pk must not be empty");  // ❌ No error code!
        }
        
        if (!storage_initialized_) {
            return Status::Error("del: storage not initialized");  // ❌ No error code!
        }
        
        return Status::OK();
    }
};

/**
 * AFTER: Migrated implementation using Result<T>
 * 
 * Benefits:
 * - Unified error handling across all modules
 * - Machine-readable error codes with metadata
 * - Type-safe error propagation
 * - Rich error context and solutions
 * - Composable with monadic operations
 */
class ContentFS_New {
private:
    bool storage_initialized_ = true;
    
public:
    /**
     * Store content blob
     * 
     * @param pk Primary key (blob identifier)
     * @param data Blob data to store
     * @param mime MIME type
     * @param sha256_expected_hex Optional expected SHA-256 hash for verification
     * @return Result<void> indicating success or structured error
     * 
     * Possible errors:
     * - ERR_API_INVALID_REQUEST: Empty pk or data
     * - ERR_STORAGE_PERMISSION_DENIED: Storage not initialized
     * - ERR_STORAGE_DISK_FULL: Insufficient disk space
     * - ERR_STORAGE_CORRUPTION: Data integrity check failed
     */
    Result<void> put(const std::string& pk,
                     const std::vector<uint8_t>& data,
                     const std::string& mime,
                     const std::optional<std::string>& sha256_expected_hex = std::nullopt) {
        
        // Validation errors
        if (pk.empty()) {
            return ErrVoid(
                errors::ErrorCode::ERR_API_INVALID_REQUEST,
                "Primary key (pk) must not be empty"
            );
        }
        
        if (data.empty()) {
            return ErrVoid(
                errors::ErrorCode::ERR_API_INVALID_REQUEST,
                "Data must not be empty"
            );
        }
        
        // Storage initialization error
        if (!storage_initialized_) {
            return ErrVoid(
                errors::ErrorCode::ERR_STORAGE_PERMISSION_DENIED,
                "Storage backend not initialized"
            );
        }
        
        // Simulate write failure (could be disk full, permission denied, etc.)
        bool write_success = true; // would actually write to storage
        if (!write_success) {
            return ErrVoid(
                errors::ErrorCode::ERR_STORAGE_DISK_FULL,
                fmt::format("Failed to write blob '{}'", pk)
            );
        }
        
        // Simulate checksum verification failure
        if (sha256_expected_hex.has_value()) {
            bool checksum_match = true; // would verify actual checksum
            if (!checksum_match) {
                return ErrVoid(
                    errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                    fmt::format("Checksum mismatch for blob '{}'", pk)
                );
            }
        }
        
        return OkVoid();
    }
    
    /**
     * Retrieve content blob
     * 
     * @param pk Primary key (blob identifier)
     * @return Result containing blob data or structured error
     * 
     * Possible errors:
     * - ERR_API_INVALID_REQUEST: Empty pk
     * - ERR_STORAGE_FILE_NOT_FOUND: Blob not found
     * - ERR_STORAGE_PERMISSION_DENIED: Storage not initialized
     * - ERR_STORAGE_CORRUPTION: Data corruption detected
     */
    Result<std::vector<uint8_t>> get(const std::string& pk) {
        if (pk.empty()) {
            return Err<std::vector<uint8_t>>(
                errors::ErrorCode::ERR_API_INVALID_REQUEST,
                "Primary key (pk) must not be empty"
            );
        }
        
        if (!storage_initialized_) {
            return Err<std::vector<uint8_t>>(
                errors::ErrorCode::ERR_STORAGE_PERMISSION_DENIED,
                "Storage backend not initialized"
            );
        }
        
        // Simulate not found
        bool found = false;
        if (!found) {
            return Err<std::vector<uint8_t>>(
                errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
                fmt::format("Blob '{}' not found", pk)
            );
        }
        
        std::vector<uint8_t> data = {1, 2, 3};  // would read from storage
        
        // Simulate corruption check
        bool data_valid = true;  // would verify checksum
        if (!data_valid) {
            return Err<std::vector<uint8_t>>(
                errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                fmt::format("Data corruption detected for blob '{}'", pk)
            );
        }
        
        return Ok(std::move(data));
    }
    
    /**
     * Delete content blob
     * 
     * @param pk Primary key (blob identifier)
     * @return Result<void> indicating success or structured error
     * 
     * Possible errors:
     * - ERR_API_INVALID_REQUEST: Empty pk
     * - ERR_STORAGE_PERMISSION_DENIED: Storage not initialized or permission denied
     * - ERR_STORAGE_FILE_NOT_FOUND: Blob not found (non-fatal)
     */
    Result<void> del(const std::string& pk) {
        if (pk.empty()) {
            return ErrVoid(
                errors::ErrorCode::ERR_API_INVALID_REQUEST,
                "Primary key (pk) must not be empty"
            );
        }
        
        if (!storage_initialized_) {
            return ErrVoid(
                errors::ErrorCode::ERR_STORAGE_PERMISSION_DENIED,
                "Storage backend not initialized"
            );
        }
        
        // Note: Delete of non-existent key could return success or error
        // depending on desired semantics
        return OkVoid();
    }
};

/**
 * Example usage showing caller-side improvements
 */
void example_usage() {
    ContentFS_New fs;
    
    // Example 1: Put operation with error handling
    std::vector<uint8_t> data = {0x48, 0x65, 0x6C, 0x6C, 0x6F};  // "Hello"
    auto put_result = fs.put("doc123", data, "text/plain");
    
    if (put_result) {
        spdlog::info("Successfully stored blob");
    } else {
        const Error& err = put_result.error();
        spdlog::error("Failed to store blob: {}", err.message());
        
        // Get detailed solution
        auto metadata = err.metadata();
        spdlog::info("Solution:\n{}", metadata.solution);
        
        // Handle specific error types
        if (err.code() == errors::ErrorCode::ERR_STORAGE_DISK_FULL) {
            // Cleanup old data and retry
            // ...
        }
    }
    
    // Example 2: Get operation with chaining
    auto get_result = fs.get("doc123")
        .and_then([](const std::vector<uint8_t>& data) -> Result<std::string> {
            // Convert bytes to string
            std::string str(data.begin(), data.end());
            if (str.empty()) {
                return Err<std::string>(
                    errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                    "Empty data in blob"
                );
            }
            return Ok(str);
        });
    
    if (get_result) {
        spdlog::info("Retrieved content: {}", *get_result);
    } else {
        spdlog::error("Failed to get content: {}", get_result.error().message());
    }
    
    // Example 3: Delete with value_or pattern
    auto del_result = fs.del("doc123");
    bool deleted = del_result.has_value();
    
    // Example 4: Migration from old Status API
    // Old code: if (status.ok) { ... }
    // New code: if (result) { ... }
    
    // Helper to convert old Status to Result<void>
    auto convert_status = [](bool ok, const std::string& msg) -> Result<void> {
        if (ok) {
            return OkVoid();
        }
        // Map generic message to appropriate error code
        if (msg.find("not found") != std::string::npos) {
            return ErrVoid(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND, msg);
        }
        if (msg.find("empty") != std::string::npos) {
            return ErrVoid(errors::ErrorCode::ERR_API_INVALID_REQUEST, msg);
        }
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_CORRUPTION, msg);
    };
    
    // Use conversion helper for gradual migration
    bool legacy_ok = true;
    std::string legacy_msg = "";
    Result<void> migrated = convert_status(legacy_ok, legacy_msg);
}

} // namespace migration_example
} // namespace themis
