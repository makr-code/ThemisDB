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

struct ScopeAccumulator {
    std::string scope_key;           ///< Unique key for scope (collection:shard)
    uint64_t accumulated_bytes = 0;  ///< Total bytes accumulated in this scope
    size_t result_count = 0;         ///< Number of results from this scope
};

class ScopeEnforcer {
public:
    /**
     * @brief Scope Enforcer.
     * @return Return value.
     */
    virtual ~ScopeEnforcer() = default;
    
    /**
     * @brief Validate Result Scope.
     * @param[in] result_data Input parameter.
     * @param[in] expected_scope Input parameter.
     * @return Return value.
     */
    virtual Result<void> validateResultScope(
        const std::string& result_data,
        const QueryScope& expected_scope) const = 0;
    
    /**
     * @brief Extract Result Scope.
     * @param[in] result_data Input parameter.
     * @return Return value.
     */
    virtual QueryScope extractResultScope(
        const std::string& result_data) const = 0;
    
    /**
     * @brief Enforce Accumulated Scope Bounds.
     * @param[in] scope_key Input parameter.
     * @param[in] new_bytes Input parameter.
     * @param[in] max_bytes_per_scope Input parameter.
     * @return Return value.
     */
    virtual Result<void> enforceAccumulatedScopeBounds(
        const std::string& scope_key,
        uint64_t new_bytes,
        uint64_t max_bytes_per_scope) = 0;
    
    /**
     * @brief Reset Scope Accumulation.
     * @param[in] scope_key Input parameter.
     */
    virtual void resetScopeAccumulation(const std::string& scope_key) = 0;
    
    /**
     * @brief Get Scope Accumulated Bytes.
     * @param[in] scope_key Input parameter.
     * @return Return value.
     */
    virtual uint64_t getScopeAccumulatedBytes(const std::string& scope_key) const = 0;
    
    /**
     * @brief Validate Page Scope.
     * @param[in] begin_offset Input parameter.
     * @param[in] end_offset Input parameter.
     * @param[in] total_size Input parameter.
     * @param[in] expected_scope Input parameter.
     * @return Return value.
     */
    virtual Result<void> validatePageScope(
        size_t begin_offset,
        size_t end_offset,
        size_t total_size,
        const QueryScope& expected_scope) const = 0;
};

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
    std::unordered_map<std::string, uint64_t> scope_accumulators_;
    mutable std::mutex accumulators_mutex_;
};

} // namespace query
} // namespace themis
