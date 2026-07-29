// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
#pragma once

/**
 * @file performance_api_contract.h
 * @brief Frozen API contract for the ThemisDB performance module.
 * @version 1.0.0
 *
 * @section purpose Purpose
 * This header documents the binding contracts for the performance module:
 * query compilation, adaptive caching, resource pooling, cost modelling,
 * and load balancing.  All public interfaces MUST adhere to the contracts
 * described here.  Any deviation is a regression.
 *
 * @section contracts API Contracts
 *
 * ### Query Compiler
 * - `AdaptiveQueryCompiler::compile()` returns a compiled plan within the
 *   configured timeout; timeout → PERF_COMPILE_TIMEOUT.
 * - Compiled plan is deterministic for the same query text + schema version.
 * - Plan cache invalidation is synchronous — stale plans are never executed.
 *
 * ### Cache Manager
 * - `AdvancedCacheManager::get()` returns nullopt on miss (never throws for
 *   missing keys).
 * - Eviction policy (LRU/LFU/ARC) is configurable at construction; policy
 *   cannot be changed after the cache is warm.
 * - Cache hit rate is observable via `cacheStats()`.
 *
 * ### Resource Pool
 * - `ResourcePool::acquire()` blocks until a resource is available or timeout
 *   expires → PERF_POOL_EXHAUSTED.
 * - Resources are returned to pool on RAII destructor; double-return is safe.
 *
 * ### Cost Model
 * - `CostModel::estimate()` returns a non-negative cost; zero cost is valid
 *   (in-memory table scan).
 * - Estimates are monotonically consistent: adding more predicates ≤ cost
 *   without predicates.
 *
 * ### Load Balancer
 * - `LoadBalancer::selectNode()` returns a healthy node or throws
 *   PERF_NO_HEALTHY_NODE if all nodes are degraded.
 *
 * @section error_taxonomy Error Taxonomy
 * | Code                      | Meaning                                      |
 * |---------------------------|----------------------------------------------|
 * | PERF_COMPILE_TIMEOUT      | Query compilation exceeded budget            |
 * | PERF_CACHE_EVICTION_FULL  | Cache eviction cannot free space             |
 * | PERF_POOL_EXHAUSTED       | Resource pool exhausted; timeout reached     |
 * | PERF_COST_MODEL_INVALID   | Cost model input out of valid range          |
 * | PERF_NO_HEALTHY_NODE      | Load balancer: no healthy node available     |
 * | PERF_PLAN_STALE           | Cached plan invalidated; recompilation needed|
 * | PERF_STATS_UNAVAILABLE    | Performance statistics not yet collected     |
 *
 * @section threading Threading Guarantees
 * - `AdvancedCacheManager` is thread-safe (internal mutex).
 * - `ResourcePool::acquire()` and `release()` are thread-safe.
 * - `AdaptiveQueryCompiler` is thread-safe for concurrent compilations.
 * - `LoadBalancer` read path is lock-free; health-update path acquires a
 *   shared mutex.
 *
 * @section contract_freeze Contract Freeze
 * This contract is frozen for ThemisDB v2.x.  Breaking changes require a
 * major version bump and deprecation notice in CHANGELOG.md.
 *
 * @see src/performance/ROADMAP.md — Phase 1 / Phase 5 gates
 * @see benchmarks/performance/bench_performance_release_gates.cpp
 * @see tests/performance/test_performance_contract_hardening_focused.cpp
 */

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace themis::performance {

/// @brief Error codes for the performance module.
enum class PerfError : int32_t {
    kCompileTimeout    = 7100, ///< Query compilation exceeded budget
    kCacheEvictionFull = 7101, ///< Cache eviction cannot free space
    kPoolExhausted     = 7102, ///< Resource pool exhausted
    kCostModelInvalid  = 7103, ///< Cost model input invalid
    kNoHealthyNode     = 7104, ///< No healthy load-balancer target
    kPlanStale         = 7105, ///< Cached plan has been invalidated
    kStatsUnavailable  = 7106, ///< Statistics not yet collected
};

/// @brief Resource pool acquisition result.
struct PoolAcquireResult {
    bool     acquired{false};  ///< true if resource was obtained
    PerfError error{PerfError::kPoolExhausted}; ///< error if !acquired
};

/// @brief Cache statistics snapshot.
struct CacheStats {
    uint64_t hits{0};
    uint64_t misses{0};
    uint64_t evictions{0};
    double   hitRatePercent{0.0}; ///< [0, 100]
};

/// @brief Query cost estimate (non-negative).
using CostEstimate = double;

} // namespace themis::performance
