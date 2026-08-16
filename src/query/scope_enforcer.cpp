/**
 * @file scope_enforcer.cpp
 * @brief Scope enforcement implementation (Phase 2 Agent 3)
 * @version 0.1.0
 * @note Status: Phase 2 Executor Scope Enforcement
 */

#include "query/scope_enforcer.h"
#include "utils/logger.h"
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <mutex>

namespace themis {
namespace query {

// ============================================================================
// ScopeEnforcerImpl Implementation
// ============================================================================

ScopeEnforcerImpl::ScopeEnforcerImpl() = default;

Result<void> ScopeEnforcerImpl::validateResultScope(
    const std::string& result_data,
    const QueryScope& expected_scope) const
{
    if (result_data.empty()) {
        return OkVoid();
    }
    
    try {
        const auto result_scope = extractResultScope(result_data);
        
        // Validate collection match
        if (!expected_scope.collection_name.empty() &&
            !result_scope.collection_name.empty() &&
            result_scope.collection_name != expected_scope.collection_name) {
            return ErrVoid(
                errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                "Scope mismatch: expected collection '" + expected_scope.collection_name +
                "' but result from '" + result_scope.collection_name + "'");
        }
        
        // Validate shard match for federated queries
        if (expected_scope.is_federated &&
            !expected_scope.shard_id.empty() &&
            !result_scope.shard_id.empty() &&
            result_scope.shard_id != expected_scope.shard_id) {
            return ErrVoid(
                errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                "Federated scope mismatch: expected shard '" + expected_scope.shard_id +
                "' but result from '" + result_scope.shard_id + "'");
        }
        
        return OkVoid();
    } catch (const std::exception& e) {
        THEMIS_WARN("validateResultScope exception: {}", e.what());
        return ErrVoid(
            errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            std::string("Scope validation failed: ") + e.what());
    }
}

QueryScope ScopeEnforcerImpl::extractResultScope(
    const std::string& result_data) const
{
    QueryScope scope;
    
    try {
        if (result_data.empty() || result_data[0] != '{') {
            return scope;
        }
        
        auto json = nlohmann::json::parse(result_data);
        
        // Try to extract scope metadata from result JSON
        if (json.is_object()) {
            if (json.contains("_scope")) {
                const auto& scope_obj = json["_scope"];
                if (scope_obj.is_object()) {
                    scope.collection_name = scope_obj.value("collection", "");
                    scope.shard_id = scope_obj.value("shard", "");
                    scope.scope_generation = scope_obj.value("generation", 0);
                    scope.is_federated = scope_obj.value("is_federated", false);
                }
            }
        }
    } catch (const std::exception& e) {
        THEMIS_DEBUG("extractResultScope parse error (non-fatal): {}", e.what());
    }
    
    return scope;
}

Result<void> ScopeEnforcerImpl::enforceAccumulatedScopeBounds(
    const std::string& scope_key,
    uint64_t new_bytes,
    uint64_t max_bytes_per_scope)
{
    std::lock_guard<std::mutex> lock(accumulators_mutex_);
    
    auto it = scope_accumulators_.find(scope_key);
    if (it == scope_accumulators_.end()) {
        // First entry for this scope
        if (new_bytes > max_bytes_per_scope) {
            return ErrVoid(
                errors::ErrorCode::ERR_QUERY_RESOURCE_EXHAUSTED,
                "Scope '" + scope_key + "' single result (" +
                std::to_string(new_bytes) + " bytes) exceeds per-scope limit (" +
                std::to_string(max_bytes_per_scope) + " bytes)");
        }
        scope_accumulators_[scope_key] = new_bytes;
    } else {
        // Accumulate and check limit
        uint64_t accumulated = it->second;
        
        // Check for overflow before addition
        if (new_bytes > max_bytes_per_scope - accumulated) {
            return ErrVoid(
                errors::ErrorCode::ERR_QUERY_RESOURCE_EXHAUSTED,
                "Scope '" + scope_key + "' accumulated bytes (" +
                std::to_string(accumulated + new_bytes) + ") exceeds per-scope limit (" +
                std::to_string(max_bytes_per_scope) + " bytes)");
        }
        
        it->second += new_bytes;
    }
    
    return OkVoid();
}

void ScopeEnforcerImpl::resetScopeAccumulation(const std::string& scope_key)
{
    std::lock_guard<std::mutex> lock(accumulators_mutex_);
    scope_accumulators_.erase(scope_key);
}

uint64_t ScopeEnforcerImpl::getScopeAccumulatedBytes(const std::string& scope_key) const
{
    std::lock_guard<std::mutex> lock(accumulators_mutex_);
    auto it = scope_accumulators_.find(scope_key);
    return it != scope_accumulators_.end() ? it->second : 0;
}

Result<void> ScopeEnforcerImpl::validatePageScope(
    size_t begin_offset,
    size_t end_offset,
    size_t total_size,
    const QueryScope& /*expected_scope*/) const
{
    // Validate page boundaries don't cross total size
    if (begin_offset > total_size) {
        return ErrVoid(
            errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "Page begin_offset (" + std::to_string(begin_offset) +
            ") exceeds total_size (" + std::to_string(total_size) + ")");
    }
    
    if (end_offset > total_size) {
        return ErrVoid(
            errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "Page end_offset (" + std::to_string(end_offset) +
            ") exceeds total_size (" + std::to_string(total_size) + ")");
    }
    
    if (begin_offset > end_offset) {
        return ErrVoid(
            errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "Page begin_offset (" + std::to_string(begin_offset) +
            ") exceeds end_offset (" + std::to_string(end_offset) + ")");
    }
    
    return OkVoid();
}

} // namespace query
} // namespace themis
