/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            smart_routing.h                                    ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 05:37:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     331                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d153089bc2  2026-03-13  fix(server): eliminate data race in SmartRouter::refreshS... ║
    • 830284816f  2026-03-13  feat: implement RequestCoalescingManager and SmartRouter ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file smart_routing.h
 * @brief ML-inspired smart routing for the API Gateway.
 *
 * SmartRouter learns query patterns and per-backend latency/load statistics
 * at runtime and uses them to make routing decisions that minimise tail
 * latency and backend load.
 *
 * Routing strategy (applied in order):
 *  1. **Cache-hit prediction** – if a resource key has been accessed on a
 *     particular backend significantly more than others, route there first
 *     (higher chance of a warm cache hit).
 *  2. **High tail-latency avoidance** – skip backends whose rolling p99
 *     latency exceeds the configured threshold.
 *  3. **Least-loaded** – among remaining backends, pick the one with the
 *     fewest active connections; break ties with lowest average latency.
 *
 * Expected improvement: 20–40% latency reduction vs. random/round-robin.
 *
 * Thread safety: all public methods are safe to call concurrently.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <atomic>
#include <deque>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace themis::server {

// ---------------------------------------------------------------------------
// BackendEndpoint – descriptor for a routable backend
// ---------------------------------------------------------------------------

/**
 * @brief Identifies a single backend service endpoint.
 */
struct BackendEndpoint {
    std::string backend_id;   ///< Unique identifier (e.g. "shard-0")
    std::string address;      ///< Host name or IP
    uint16_t    port{8080};   ///< HTTP port

    bool operator==(const BackendEndpoint& o) const noexcept {
        return backend_id == o.backend_id;
    }
};

// ---------------------------------------------------------------------------
// SmartRouter
// ---------------------------------------------------------------------------

/**
 * @brief Runtime-learning router for API Gateway backend selection.
 *
 * The router maintains a rolling latency window and access-frequency map for
 * every registered backend.  These are updated via `record*` calls after each
 * request and used to inform future routing decisions.
 */
class SmartRouter {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        /// Number of recent latency samples to keep per backend.
        uint32_t latency_window_size{100};

        /// p99 latency (ms) above which a backend is considered high-tail and
        /// is excluded from the primary candidate set unless it is the only
        /// option.
        double tail_latency_threshold_ms{500.0};

        /// Minimum number of accesses a backend must have seen for a given
        /// resource key before it is selected via cache-hit prediction.
        uint32_t min_cache_prediction_hits{3};

        /// When true, enable cache-hit prediction (Phase 1 of routing).
        bool enable_cache_prediction{true};
    };

    // -----------------------------------------------------------------------
    // Per-backend runtime statistics
    // -----------------------------------------------------------------------

    struct BackendStats {
        std::string backend_id;
        double      avg_latency_ms{0.0};     ///< Rolling average over the window
        double      p99_latency_ms{0.0};     ///< 99th percentile over the window
        uint64_t    total_requests{0};        ///< All requests routed here
        uint32_t    active_connections{0};    ///< Currently in-flight requests
        uint64_t    cache_hits{0};            ///< Recorded cache hits
        uint64_t    cache_misses{0};          ///< Recorded cache misses
        uint32_t    latency_samples{0};       ///< Samples in current window

        nlohmann::json toJson() const {
            return {
                {"backend_id",        backend_id},
                {"avg_latency_ms",    avg_latency_ms},
                {"p99_latency_ms",    p99_latency_ms},
                {"total_requests",    total_requests},
                {"active_connections", active_connections},
                {"cache_hits",        cache_hits},
                {"cache_misses",      cache_misses},
                {"latency_samples",   latency_samples},
            };
        }
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    SmartRouter();
    explicit SmartRouter(const Config& config);

    SmartRouter(const SmartRouter&)            = delete;
    SmartRouter& operator=(const SmartRouter&) = delete;
    SmartRouter(SmartRouter&&)                 = default;
    SmartRouter& operator=(SmartRouter&&)      = default;

    ~SmartRouter() = default;

    // -----------------------------------------------------------------------
    // Backend registry
    // -----------------------------------------------------------------------

    /**
     * @brief Register a backend endpoint.
     * @param endpoint Backend to add; no-op if already registered.
     */
    void addBackend(const BackendEndpoint& endpoint);

    /**
     * @brief Remove a backend endpoint.
     * @param backend_id Backend to remove.
     */
    void removeBackend(const std::string& backend_id);

    /**
     * @brief Return all currently registered backends.
     */
    std::vector<BackendEndpoint> listBackends() const;

    // -----------------------------------------------------------------------
    // Feedback – called after every request to update runtime state
    // -----------------------------------------------------------------------

    /**
     * @brief Record an observed request latency for a backend.
     * @param backend_id  Backend that served the request.
     * @param latency_ms  Round-trip latency in milliseconds.
     */
    void recordLatency(const std::string& backend_id, double latency_ms);

    /**
     * @brief Record a cache hit on a backend for a resource key.
     * @param backend_id  Backend that produced the cache hit.
     * @param resource_key Resource identifier (e.g. entity URN or path).
     */
    void recordCacheHit(const std::string& backend_id,
                        const std::string& resource_key);

    /**
     * @brief Record a cache miss on a backend for a resource key.
     */
    void recordCacheMiss(const std::string& backend_id,
                         const std::string& resource_key);

    /**
     * @brief Increment the active-connection counter for a backend.
     *
     * Call this immediately before dispatching a request.
     */
    void incrementActiveConnections(const std::string& backend_id);

    /**
     * @brief Decrement the active-connection counter for a backend.
     *
     * Call this after a response is received (including on error).
     */
    void decrementActiveConnections(const std::string& backend_id);

    // -----------------------------------------------------------------------
    // Routing decisions
    // -----------------------------------------------------------------------

    /**
     * @brief Select the best backend for a given resource key.
     *
     * Applies cache-hit prediction, tail-latency filtering, and
     * least-loaded selection in that order.
     *
     * @param resource_key  Resource identifier (e.g. entity URN or path).
     * @return Selected backend, or nullopt if no backends are registered.
     */
    std::optional<BackendEndpoint> route(const std::string& resource_key) const;

    /**
     * @brief Return the backend with the fewest active connections.
     *
     * Tie-breaks by lowest average latency.  Ignores high-tail-latency
     * backends when alternatives exist.
     *
     * @return Least-loaded backend, or nullopt if no backends are registered.
     */
    std::optional<BackendEndpoint> routeLeastLoaded() const;

    /**
     * @brief Return the backend most likely to have `resource_key` in its cache.
     *
     * Returns nullopt when there is insufficient history.
     *
     * @param resource_key  Resource to look up.
     * @return Backend with highest cache-hit count for the key, or nullopt.
     */
    std::optional<BackendEndpoint> predictCachedBackend(
        const std::string& resource_key) const;

    // -----------------------------------------------------------------------
    // Observability
    // -----------------------------------------------------------------------

    /**
     * @brief Return statistics for all registered backends.
     */
    std::vector<BackendStats> getAllStats() const;

    /**
     * @brief Return statistics for a specific backend.
     * @throws std::out_of_range if backend_id is not registered.
     */
    BackendStats getBackendStats(const std::string& backend_id) const;

private:
    // -----------------------------------------------------------------------
    // Internal state
    // -----------------------------------------------------------------------

    struct BackendState {
        BackendEndpoint endpoint;

        // Rolling latency window (newest at back)
        std::deque<double> latency_window;

        std::atomic<uint32_t> active_connections{0};
        uint64_t total_requests{0};
        uint64_t cache_hits{0};
        uint64_t cache_misses{0};

        // Per-resource-key access counts for cache prediction
        // key → number of cache hits on this backend
        std::unordered_map<std::string, uint32_t> key_hit_counts;

        // Cached statistics – updated eagerly in recordLatency() while the
        // unique_lock is held.  Safe to read under shared_lock.
        double cached_avg_latency{0.0};
        double cached_p99_latency{0.0};
    };

    // -----------------------------------------------------------------------
    // Private helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Compute the average of a latency window.
     */
    static double computeAvg(const std::deque<double>& window) noexcept;

    /**
     * @brief Compute the p99 of a latency window (nearest-rank method).
     */
    static double computeP99(const std::deque<double>& window);

    /**
     * @brief Return true when a backend is high-tail (p99 > threshold) AND
     *        there is at least one other non-high-tail backend available.
     */
    bool isHighTail(const BackendState& state,
                    bool has_other_candidates) const noexcept;

    /**
     * @brief Recompute cached avg/p99 from the current latency window.
     *        MUST be called while holding a unique_lock on mutex_.
     */
    static void refreshStats(BackendState& state) noexcept;

    // -----------------------------------------------------------------------
    // Data members
    // -----------------------------------------------------------------------

    Config config_;

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, BackendState> backends_;
};

} // namespace themis::server
