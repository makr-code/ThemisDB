/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            index_manager_migration_example.cpp                ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     273                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file index_manager_migration_example.cpp
 * @brief Example migration from nullptr pattern to Result<T> for IndexManager
 * 
 * This file demonstrates the migration pattern for converting IndexManager
 * methods from returning nullptr to using Result<T*>.
 * 
 * NOTE: This is an EXAMPLE file showing migration patterns. It is not meant
 * to be compiled or used directly. Actual migration would modify the real
 * index_manager.cpp and index_manager.h files.
 */

#include "utils/expected.h"
#include "utils/error_registry.h"
#include <string>
#include <string_view>
#include <mutex>
#include <unordered_map>

namespace themis {
namespace migration_example {

// Forward declarations (simplified for example)
class ISecondaryIndex;
class SecondaryIndexManager;

/**
 * BEFORE: Original implementation returning nullptr
 * 
 * Problems:
 * - Caller cannot distinguish between different error types
 * - No context about why the operation failed
 * - Requires checking logs to understand failures
 */
class IndexManager_Old {
private:
    std::shared_ptr<SecondaryIndexManager> secondary_manager_;
    std::unordered_map<std::string, ISecondaryIndex*> secondary_indices_;
    std::mutex registry_mutex_ = {};
    
public:
    ISecondaryIndex* createSecondaryIndex(
        std::string_view name,
        std::string_view field_name,
        const std::string& config) {
        
        std::lock_guard<std::mutex> lock(registry_mutex_);
        
        // Error case 1: Not initialized
        if (!secondary_manager_) {
            THEMIS_ERROR("IndexManager::createSecondaryIndex: Secondary manager not initialized");
            return nullptr;  // ❌ Lost error context!
        }
        
        std::string name_str(name);
        
        // Check if index already exists (this is success, not error)
        if (secondary_indices_.find(name_str) != secondary_indices_.end()) {
            THEMIS_WARN("IndexManager::createSecondaryIndex: Index '{}' already exists", name_str);
            return secondary_indices_[name_str];
        }
        
        // Error case 2: Invalid name
        if (name.empty()) {
            THEMIS_ERROR("IndexManager::createSecondaryIndex: Empty index name");
            return nullptr;  // ❌ Lost error context!
        }
        
        // Simulate creation that might fail
        ISecondaryIndex* index = nullptr; // would call secondary_manager_->create()
        if (!index) {
            THEMIS_ERROR("IndexManager::createSecondaryIndex: Failed to create index '{}'", name_str);
            return nullptr;  // ❌ Lost error context!
        }
        
        secondary_indices_[name_str] = index;
        return index;
    }
};

/**
 * AFTER: Migrated implementation using Result<T*>
 * 
 * Benefits:
 * - Type-safe error handling with structured error codes
 * - Context information embedded in error
 * - Compiler-enforced error checking
 * - Composable with monadic operations
 * - Zero-overhead compared to nullptr (no exceptions)
 */
class IndexManager_New {
private:
    std::shared_ptr<SecondaryIndexManager> secondary_manager_;
    std::unordered_map<std::string, ISecondaryIndex*> secondary_indices_;
    std::mutex registry_mutex_;
    
public:
    /**
     * Create a secondary index with type-safe error handling
     * 
     * @param name Index name
     * @param field_name Field to index
     * @param config Configuration JSON
     * @return Result containing pointer to created index or structured error
     * 
     * Possible errors:
     * - ERR_INDEX_NOT_INITIALIZED: Secondary manager not initialized
     * - ERR_INDEX_CREATION_FAILED: Failed to create index
     * - ERR_API_INVALID_REQUEST: Empty or invalid index name
     */
    Result<ISecondaryIndex*> createSecondaryIndex(
        std::string_view name,
        std::string_view field_name,
        const std::string& config) {
        
        std::lock_guard<std::mutex> lock(registry_mutex_);
        
        // Error case 1: Not initialized
        if (!secondary_manager_) {
            THEMIS_ERROR("IndexManager::createSecondaryIndex: Secondary manager not initialized");
            return Err<ISecondaryIndex*>(
                errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED,
                "Secondary manager not initialized"
            );
        }
        
        std::string name_str(name);
        
        // Check if index already exists (success case)
        if (secondary_indices_.find(name_str) != secondary_indices_.end()) {
            THEMIS_WARN("IndexManager::createSecondaryIndex: Index '{}' already exists", name_str);
            return Ok(secondary_indices_[name_str]);
        }
        
        // Error case 2: Invalid name
        if (name.empty()) {
            THEMIS_ERROR("IndexManager::createSecondaryIndex: Empty index name");
            return Err<ISecondaryIndex*>(
                errors::ErrorCode::ERR_API_INVALID_REQUEST,
                "Index name cannot be empty"
            );
        }
        
        // Simulate creation that might fail
        ISecondaryIndex* index = nullptr; // would call secondary_manager_->create()
        if (!index) {
            THEMIS_ERROR("IndexManager::createSecondaryIndex: Failed to create index '{}'", name_str);
            return Err<ISecondaryIndex*>(
                errors::ErrorCode::ERR_INDEX_CREATION_FAILED,
                fmt::format("Failed to create index '{}'", name_str)
            );
        }
        
        secondary_indices_[name_str] = index;
        return Ok(index);
    }
    
    /**
     * Get existing index by name
     * 
     * @param name Index name to find
     * @return Result containing pointer to index or ERR_INDEX_NOT_FOUND
     */
    Result<ISecondaryIndex*> getSecondaryIndex(std::string_view name) {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        
        std::string name_str(name);
        auto it = secondary_indices_.find(name_str);
        
        if (it == secondary_indices_.end()) {
            return Err<ISecondaryIndex*>(
                errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                fmt::format("Index '{}' not found", name_str)
            );
        }
        
        return Ok(it->second);
    }
};

/**
 * Example usage showing caller-side improvements
 */
void example_usage() {
    IndexManager_New manager;
    
    // Example 1: Check result and handle error
    auto result = manager.createSecondaryIndex("user_id_idx", "user_id", "{}");
    if (result) {
        // Success path
        ISecondaryIndex* index = *result;
        // Use index...
    } else {
        // Error path - get structured error information
        const Error& err = result.error();
        
        // Log with full context
        spdlog::error("Failed to create index: {} (code: {})",
                     err.message(),
                     static_cast<int>(err.code()));
        
        // Get solution from error metadata
        auto metadata = err.metadata();
        spdlog::info("Suggested solution:\n{}", metadata.solution);
        
        // Handle different error types
        switch (err.code()) {
            case errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED:
                // Initialization error - fatal
                throw std::runtime_error("System not initialized");
                
            case errors::ErrorCode::ERR_INDEX_CREATION_FAILED:
                // Creation failed - might retry or fall back
                spdlog::warn("Index creation failed, continuing without index");
                break;
                
            case errors::ErrorCode::ERR_API_INVALID_REQUEST:
                // Invalid request - client error
                // Return HTTP 400 Bad Request
                break;
                
            default:
                break;
        }
    }
    
    // Example 2: Use value_or for default handling
    auto index_or_null = manager.getSecondaryIndex("some_index")
        .value_or(nullptr);
    
    // Example 3: Chain operations with and_then
    auto final_result = manager.getSecondaryIndex("user_id_idx")
        .and_then([](ISecondaryIndex* index) -> Result<int> {
            // Perform operation on index
            if (index) {
                return Ok(42);  // success
            }
            return Err<int>(errors::ErrorCode::ERR_INDEX_INVALID_TYPE, "Invalid index");
        });
    
    // Example 4: Convert old nullable API to new Result API
    ISecondaryIndex* old_api_result = nullptr; // from legacy code
    auto converted = fromNullable(
        old_api_result,
        errors::ErrorCode::ERR_INDEX_NOT_FOUND,
        "Index not found in legacy system"
    );
}

} // namespace migration_example
} // namespace themis
