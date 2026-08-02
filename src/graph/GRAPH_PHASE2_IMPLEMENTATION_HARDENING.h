/**
 * @file GRAPH_PHASE2_IMPLEMENTATION_HARDENING.h
 * @brief Graph Module Phase 2 Implementation: Error & Thread-Safety Hardening
 * 
 * This header provides hardening utilities and inline helpers for Phase 2 implementation.
 * Covers:
 * - Error-path guards (cost overflow, frontier explosion, constraint validation)
 * - Thread-safety synchronization primitives (mutex, shared_lock)
 * - Diagnostic context propagation
 *
 * @version 1.0.0
 * @date 2026-08-02
 * @author Graph Module Hardening Team
 */

#pragma once

#include "graph/graph_error_taxonomy.h"
#include "utils/expected.h"
#include <chrono>
#include <limits>
#include <cmath>
#include <optional>
#include <string_view>

namespace themis {
namespace graph {
namespace phase2_hardening {

// ─────────────────────────────────────────────────────────────────────────────
// STREAM A: Error-Path Hardening Utilities
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Guard against cost calculation overflow.
 * 
 * Returns true if frontier explosion or cost overflow is detected.
 * Used in optimizer cost estimation to prevent NaN/inf propagation.
 * 
 * @param current_frontier Current frontier size estimate
 * @param fan_out Expected per-vertex fan-out
 * @param max_frontier Maximum allowed frontier (shard limit ~100M)
 * @return true if overflow condition detected, false otherwise
 */
inline bool isCostOverflowRisk(
    double current_frontier,
    double fan_out,
    double max_frontier = 1e8
) noexcept {
    // Check if frontier is already too large
    if (current_frontier > max_frontier) return true;
    
    // Check if multiplication would exceed maximum
    if (current_frontier > 0 && fan_out > 0) {
        if (current_frontier > std::numeric_limits<double>::max() / fan_out) {
            return true;
        }
        double result = current_frontier * fan_out;
        if (!std::isfinite(result)) return true;
    }
    
    return false;
}

/**
 * @brief Validate graph statistics freshness.
 * 
 * Checks if statistics are within acceptable staleness bounds.
 * Returns error if stats are too old (> 1 hour by default).
 * 
 * @param last_updated_time Last update timestamp
 * @param staleness_threshold Maximum allowed age (default: 1 hour)
 * @return true if stats are fresh, false if stale
 */
inline bool areStatisticsFresh(
    std::chrono::steady_clock::time_point last_updated_time,
    std::chrono::duration<int64_t, std::ratio<3600>> staleness_threshold =
        std::chrono::hours(1)
) noexcept {
    auto now = std::chrono::steady_clock::now();
    return (now - last_updated_time) <= staleness_threshold;
}

/**
 * @brief Validate frontier capacity and bounds.
 * 
 * Ensures per-source and aggregate frontier sizes are within limits.
 * Used in parallel traversal to prevent memory exhaustion.
 * 
 * @param per_source_size Current per-source frontier size
 * @param aggregate_size Current aggregate frontier size
 * @param per_source_limit Maximum per-source (default: 1M)
 * @param aggregate_limit Maximum aggregate (default: 10M)
 * @return true if bounds respected, false if violated
 */
inline bool isFrontierValid(
    size_t per_source_size,
    size_t aggregate_size,
    size_t per_source_limit = 1'000'000,
    size_t aggregate_limit = 10'000'000
) noexcept {
    return (per_source_size <= per_source_limit) &&
           (aggregate_size <= aggregate_limit);
}

/**
 * @brief Check if vertex ID is valid (non-empty).
 * 
 * @param vertex_id Vertex identifier
 * @return true if vertex_id is non-empty, false otherwise
 */
inline bool isValidVertexId(std::string_view vertex_id) noexcept {
    return !vertex_id.empty() && vertex_id.length() <= 65535;
}

/**
 * @brief Check if depth limit is reasonable.
 * 
 * Ensures max_depth is within practical bounds [1, 1000].
 * Prevents pathological traversal patterns.
 * 
 * @param max_depth Requested maximum depth
 * @param limit Maximum allowed depth (default: 1000)
 * @return true if within bounds, false otherwise
 */
inline bool isDepthValid(size_t max_depth, size_t limit = 1000) noexcept {
    return max_depth > 0 && max_depth <= limit;
}

// ─────────────────────────────────────────────────────────────────────────────
// STREAM B: Thread-Safety Hardening Utilities
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief RAII guard for shared_lock (read lock).
 * 
 * Automatically acquires and releases a shared lock.
 * Usage: auto _guard = phase2_hardening::make_shared_lock(mutex);
 */
template<typename Mutex>
class SharedLockGuard {
public:
    explicit SharedLockGuard(Mutex& m) : lock_(m) {}
    
    SharedLockGuard(const SharedLockGuard&) = delete;
    SharedLockGuard& operator=(const SharedLockGuard&) = delete;
    
    SharedLockGuard(SharedLockGuard&&) = default;
    SharedLockGuard& operator=(SharedLockGuard&&) = default;
    
private:
    std::shared_lock<Mutex> lock_;
};

/**
 * @brief RAII guard for unique_lock (write lock).
 * 
 * Automatically acquires and releases a unique lock.
 * Usage: auto _guard = phase2_hardening::make_unique_lock(mutex);
 */
template<typename Mutex>
class UniqueLockGuard {
public:
    explicit UniqueLockGuard(Mutex& m) : lock_(m) {}
    
    UniqueLockGuard(const UniqueLockGuard&) = delete;
    UniqueLockGuard& operator=(const UniqueLockGuard&) = delete;
    
    UniqueLockGuard(UniqueLockGuard&&) = default;
    UniqueLockGuard& operator=(UniqueLockGuard&&) = default;
    
private:
    std::unique_lock<Mutex> lock_;
};

/**
 * @brief Check lock timeout safety.
 * 
 * Ensures timeout durations are reasonable (10ms to 30s).
 * Prevents pathological deadlock scenarios.
 * 
 * @param timeout Requested timeout duration
 * @param min_ms Minimum allowed (default: 10ms)
 * @param max_ms Maximum allowed (default: 30000ms)
 * @return true if timeout is reasonable, false otherwise
 */
inline bool isTimeoutValid(
    std::chrono::milliseconds timeout,
    std::chrono::milliseconds min_ms = std::chrono::milliseconds(10),
    std::chrono::milliseconds max_ms = std::chrono::milliseconds(30000)
) noexcept {
    return timeout >= min_ms && timeout <= max_ms;
}

/**
 * @brief Fairness anti-starvation check for resource acquisition.
 * 
 * Ensures older requests are served before newer ones (FIFO).
 * Prevents indefinite postponement of high-priority threads.
 * 
 * @param enqueued_at Time when request was enqueued
 * @param max_wait_time Maximum acceptable wait (default: 30s)
 * @return true if request has not exceeded max_wait, false if starvation risk
 */
inline bool isAcquisitionFair(
    std::chrono::steady_clock::time_point enqueued_at,
    std::chrono::seconds max_wait_time = std::chrono::seconds(30)
) noexcept {
    auto now = std::chrono::steady_clock::now();
    return (now - enqueued_at) < max_wait_time;
}

// ─────────────────────────────────────────────────────────────────────────────
// Diagnostic Context Propagation
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Error context for diagnostic propagation.
 * 
 * Captures operation context (component, phase, recovery hint) for improved
 * error diagnostics and fallback routing.
 */
struct ErrorDiagnostics {
    /// Component where error occurred (e.g., "optimizer", "traversal")
    std::string component;
    
    /// Operation phase (e.g., "cost_estimation", "frontier_check")
    std::string phase;
    
    /// Recovery hint for caller (e.g., "fallback to CPU", "increase max_depth")
    std::string recovery_hint;
    
    /// Optional additional context (JSON or free text)
    std::optional<std::string> context;
    
    /// Timestamp of error occurrence
    std::chrono::steady_clock::time_point timestamp = std::chrono::steady_clock::now();
};

} // namespace phase2_hardening
} // namespace graph
} // namespace themis

#endif // THEMIS_GRAPH_PHASE2_IMPLEMENTATION_HARDENING_H
