/*
 * ThemisDB | File: adaptive_ttl_policy.h | Version: 0.0.10
 * Maturity: 🟢 PRODUCTION-READY | Score: 94/100
 * Gap Summary: total=1; TODO=0, Stub=0, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * @file adaptive_ttl_policy.h
 * @brief Stateful adaptive TTL policy interface for workload-driven TTL tuning.
 *
 * `IAdaptiveTTLPolicy` extends `ITTLAdapter` (see `cache_interfaces.h`) with
 * persistent, stateful access-pattern learning.  Unlike `ITTLAdapter`, which
 * performs a stateless computation on each call, `IAdaptiveTTLPolicy` tracks
 * per-key access history across calls and adjusts TTLs based on observed
 * temporal locality.
 *
 * Typical usage:
 *   1. Call `recordAccess(key, now_ms)` on every cache hit or put.
 *   2. Call `computeTTL(key)` when inserting a new entry or refreshing TTL.
 *   3. Call `evict(key)` when an entry is removed to free its history.
 *   4. Periodically call `pruneHistory(max_age_ms)` to discard stale records.
 *
 * Design constraints:
 *   - `computeTTL()` must never return a value exceeding `maxTTL` from the
 *     active configuration.
 *   - `recordAccess()` must be callable concurrently without external locking.
 *   - `pruneHistory()` may be called on a background thread; it must be
 *     thread-safe but is not required to be lock-free.
 *   - Implementations must not throw from `recordAccess()` or `computeTTL()`.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace cache {

// ============================================================================
// AdaptiveTTLPolicyConfig — policy configuration
// ============================================================================

/**
 * @brief Configuration for an `IAdaptiveTTLPolicy` instance.
 */
struct AdaptiveTTLPolicyConfig {
    /// Minimum TTL that `computeTTL()` may return (default: 1 second).
    std::chrono::milliseconds minTTL{1'000};

    /// Hard upper bound for `computeTTL()` (default: 1 hour).
    std::chrono::milliseconds maxTTL{3'600'000};

    /// Number of recent accesses tracked per key.  Older accesses beyond this
    /// window are discarded (default: 64).
    uint32_t access_window_size = 64;

    /// Scaling factor applied to inter-access interval estimates.  Higher
    /// values produce longer TTLs for frequently accessed keys (default: 2.0).
    double aggressiveness = 2.0;

    /// Discount factor applied to older access intervals (0 < decay ≤ 1).
    /// Values closer to 1.0 give equal weight to all accesses; values closer
    /// to 0.0 prioritise recent accesses (default: 0.9).
    double decay_factor = 0.9;

    /// Maximum age (ms) of access records retained by `pruneHistory()`.
    /// Records older than this threshold are discarded (default: 24 hours).
    int64_t max_history_age_ms = 86'400'000LL;
};

// ============================================================================
// AccessRecord — single recorded access event
// ============================================================================

/**
 * @brief A single access event recorded for a cache key.
 */
struct AccessRecord {
    int64_t timestamp_ms = 0; ///< Wall-clock time of the access (ms since epoch).
    bool    is_hit       = true;  ///< true = cache hit; false = cache miss (new put).
};

// ============================================================================
// AdaptiveTTLSuggestion — output from computeTTL
// ============================================================================

/**
 * @brief Result of `IAdaptiveTTLPolicy::computeTTL()`.
 */
struct AdaptiveTTLSuggestion {
    /// Suggested TTL, clamped to [minTTL, maxTTL].
    std::chrono::milliseconds ttl{0};

    /// Estimated mean inter-access interval used to derive the TTL.
    /// Zero if fewer than two access records exist.
    std::chrono::milliseconds mean_access_interval{0};

    /// Number of access records that contributed to this suggestion.
    uint32_t sample_count = 0;

    /// Confidence in [0.0, 1.0]: 0.0 = no data, 1.0 = fully converged.
    double confidence = 0.0;
};

// ============================================================================
// IAdaptiveTTLPolicy — stateful adaptive TTL policy interface
// ============================================================================

/**
 * @brief Pure-virtual stateful adaptive TTL policy.
 *
 * Implementations maintain per-key access history across calls and use it
 * to derive TTL suggestions that reflect observed access frequency and
 * temporal locality.
 *
 * Thread-safety: `recordAccess()` and `computeTTL()` must be thread-safe and
 * callable concurrently from multiple cache-get/put threads.
 */
struct IAdaptiveTTLPolicy {
    virtual ~IAdaptiveTTLPolicy() = default;

    // -----------------------------------------------------------------------
    // Access recording
    // -----------------------------------------------------------------------

    /**
     * @brief Record an access event for @p key.
     *
     * Must be called on every cache hit (is_hit=true) and on every put that
     * inserts a new entry (is_hit=false).  Thread-safe; must not throw.
     *
     * @param key           Cache key that was accessed or inserted.
     * @param timestamp_ms  Wall-clock time of the access (ms since epoch).
     * @param is_hit        true = cache hit; false = new entry inserted.
     */
    virtual void recordAccess(const std::string& key,
                              int64_t            timestamp_ms,
                              bool               is_hit = true) noexcept = 0;

    // -----------------------------------------------------------------------
    // TTL computation
    // -----------------------------------------------------------------------

    /**
     * @brief Compute a TTL suggestion for @p key based on recorded history.
     *
     * If no access records exist for @p key, returns a suggestion based on
     * the configured `minTTL` with `confidence = 0.0`.
     *
     * The returned `ttl` is always in [minTTL, maxTTL].
     *
     * @param key           Cache key for which to compute a TTL suggestion.
     * @param now_ms        Current wall-clock time (ms since epoch).
     * @return AdaptiveTTLSuggestion carrying the suggested TTL and metadata.
     */
    virtual AdaptiveTTLSuggestion computeTTL(const std::string& key,
                                             int64_t            now_ms) const = 0;

    // -----------------------------------------------------------------------
    // History management
    // -----------------------------------------------------------------------

    /**
     * @brief Return all recorded access events for @p key.
     *
     * Returns an empty vector if no history exists.  Returned records are in
     * chronological order (oldest first).
     *
     * @param key  Cache key to query.
     */
    virtual std::vector<AccessRecord> getHistory(
        const std::string& key) const = 0;

    /**
     * @brief Remove all access history for @p key.
     *
     * Called when an entry is evicted or explicitly removed from the cache.
     * No-op if @p key has no history.
     *
     * @param key  Cache key whose history is to be removed.
     */
    virtual void evict(const std::string& key) = 0;

    /**
     * @brief Discard all access records older than @p max_age_ms milliseconds.
     *
     * @param now_ms     Current wall-clock time (ms since epoch).
     * @param max_age_ms Records older than `now_ms - max_age_ms` are discarded.
     * @return Number of individual access records pruned.
     */
    virtual uint64_t pruneHistory(int64_t now_ms, int64_t max_age_ms) = 0;

    /**
     * @brief Flush all accumulated access history.
     *
     * After this call `getHistory()` returns an empty vector for every key.
     */
    virtual void flushHistory() = 0;

    // -----------------------------------------------------------------------
    // Configuration & observability
    // -----------------------------------------------------------------------

    /**
     * @brief Update the policy configuration at runtime.
     *
     * Takes effect on the next call to `computeTTL()` and `pruneHistory()`.
     *
     * @param config  New configuration to apply.
     */
    virtual void configure(const AdaptiveTTLPolicyConfig& config) = 0;

    /**
     * @brief Return the active configuration.
     */
    virtual AdaptiveTTLPolicyConfig getConfig() const = 0;

    /**
     * @brief Return the number of keys for which history is currently tracked.
     */
    virtual size_t trackedKeyCount() const = 0;
};

} // namespace cache
} // namespace themis
