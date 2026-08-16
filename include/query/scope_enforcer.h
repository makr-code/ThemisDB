/**
 * @file scope_enforcer.h
 * @brief Scope enforcement and validation for query execution (Phase 2 Agent 3)
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Phase 2 Executor Scope Enforcement
 */

#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "utils/expected.h"

namespace themis {
namespace query {

/**
 * @brief Scope identifier for query results.
 * 
 * Uniquely identifies a collection/scope in the query execution context.
 * Used to prevent cross-scope data leakage in federated queries,
 * materialized views, and result streaming.
 */
struct QueryScope {
    std::string collection_name;  ///< Collection/view name
    std::string shard_id;          ///< Shard ID (for federated queries, may be empty for local)
    uint64_t scope_generation;     ///< Generation counter for incremental tracking
    bool is_federated = false;     ///< True if result came from federated shard
    
    bool operator==(const QueryScope& other) const {
        return collection_name == other.collection_name &&
               shard_id == other.shard_id &&
               scope_generation == other.scope_generation &&
               is_federated == other.is_federated;
    }
};

/**
 * @brief Accumulated scope tracking for result merging.
 * 
 * Tracks per-scope byte accumulation during federated result merging
 * to prevent exceeding resource limits.
 */
struct ScopeAccumulator {
    std::string scope_key;           ///< Unique key for scope (collection:shard)
    uint64_t accumulated_bytes = 0;  ///< Total bytes accumulated in this scope
    size_t result_count = 0;         ///< Number of results from this scope
};

/**
 * @brief Scope enforcement and validation interface.
 * 
 * Provides mechanisms to:
 * - Validate result scope consistency
 * - Enforce scope boundaries in federated result merging
 * - Track accumulated bytes per scope
 * - Isolate cross-scope data access
 * 
 * All methods are exception-safe and thread-safe where noted.
 */
class ScopeEnforcer {
public:
    virtual ~ScopeEnforcer() = default;
    
    /**
     * @brief Validate that a result belongs to the expected scope.
     * 
     * @param result_data JSON-serializable result data
     * @param expected_scope Expected scope for this result
     * @return OkVoid on valid scope, error if scope mismatch
     * @exception std::invalid_argument if scope mismatch
     */
    virtual Result<void> validateResultScope(
        const std::string& result_data,
        const QueryScope& expected_scope) const = 0;
    
    /**
     * @brief Extract scope information from a result.
     * 
     * @param result_data Result to analyze
     * @return QueryScope extracted from result, or default if unavailable
     */
    virtual QueryScope extractResultScope(
        const std::string& result_data) const = 0;
    
    /**
     * @brief Enforce accumulated byte limits for a specific scope.
     * 
     * Tracks and validates that accumulated bytes in a scope do not exceed
     * the maximum per-scope limit. Used during federated result merging.
     * 
     * @param scope_key Unique key identifying the scope
     * @param new_bytes Additional bytes to accumulate
     * @param max_bytes_per_scope Maximum bytes allowed per scope
     * @return OkVoid if within limits, error if exceeded
     * @exception std::runtime_error if limit exceeded
     */
    virtual Result<void> enforceAccumulatedScopeBounds(
        const std::string& scope_key,
        uint64_t new_bytes,
        uint64_t max_bytes_per_scope) = 0;
    
    /**
     * @brief Reset accumulated tracking for a scope.
     * 
     * Called at the start of a new query or result batch to clear
     * accumulation counters.
     * 
     * @param scope_key Scope to reset
     */
    virtual void resetScopeAccumulation(const std::string& scope_key) = 0;
    
    /**
     * @brief Get current accumulated bytes for a scope.
     * 
     * @param scope_key Scope to query
     * @return Accumulated bytes, or 0 if scope not tracked
     */
    virtual uint64_t getScopeAccumulatedBytes(const std::string& scope_key) const = 0;
    
    /**
     * @brief Validate that an iterator range respects scope boundaries.
     * 
     * Used during result pagination to ensure page boundaries don't
     * inadvertently cross scope boundaries.
     * 
     * @param begin_offset Start offset in result set
     * @param end_offset End offset in result set
     * @param total_size Total size of result set
     * @param expected_scope Expected scope for this range
     * @return OkVoid if range is valid, error if crosses scope boundary
     */
    virtual Result<void> validatePageScope(
        size_t begin_offset,
        size_t end_offset,
        size_t total_size,
        const QueryScope& expected_scope) const = 0;
};

/**
 * @brief Standard implementation of scope enforcement.
 * 
 * Provides basic scope tracking and enforcement suitable for most
 * query execution scenarios. Thread-safe for concurrent result merging.
 */
class ScopeEnforcerImpl : public ScopeEnforcer {
public:
    ScopeEnforcerImpl();
    ~ScopeEnforcerImpl() override = default;
    
    Result<void> validateResultScope(
        const std::string& result_data,
        const QueryScope& expected_scope) const override;
    
    QueryScope extractResultScope(
        const std::string& result_data) const override;
    
    Result<void> enforceAccumulatedScopeBounds(
        const std::string& scope_key,
        uint64_t new_bytes,
        uint64_t max_bytes_per_scope) override;
    
    void resetScopeAccumulation(const std::string& scope_key) override;
    
    uint64_t getScopeAccumulatedBytes(const std::string& scope_key) const override;
    
    Result<void> validatePageScope(
        size_t begin_offset,
        size_t end_offset,
        size_t total_size,
        const QueryScope& expected_scope) const override;

private:
    /// Accumulated bytes tracking per scope (scope_key → bytes)
    std::unordered_map<std::string, uint64_t> scope_accumulators_;
    mutable std::mutex accumulators_mutex_;
};

} // namespace query
} // namespace themis
