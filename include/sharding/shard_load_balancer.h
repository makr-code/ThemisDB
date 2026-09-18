/**
 * @file shard_load_balancer.h
 * @brief Phase 3 P3-04-B: Load-aware shard selection for ThemisDB.
 *
 * Provides @ref ShardLoadBalancer, which selects the optimal target shard for
 * an incoming query based on per-shard load metrics (CPU utilisation, pending
 * query count, and response-time p99).
 *
 * ### Load scoring
 * Each shard is assigned a composite load score in [0, 100]:
 * @code
 *   score = cpu_weight   * cpu_pct
 *         + queue_weight * min(pending / max_pending * 100, 100)
 *         + latency_weight * min(p99_ms / max_p99_ms * 100, 100)
 * @endcode
 * Default weights: cpu=0.30, queue=0.40, latency=0.30.  The shard with the
 * minimum score is selected; ties are broken by insertion order (earlier-registered
 * shard wins deterministically).
 *
 * ### Failover
 * If the preferred shard is marked unavailable, the balancer selects the
 * next-best candidate automatically.
 *
 * ### Thread safety
 * All public methods are thread-safe.
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Block B P3-04-B delivery
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::sharding {

// ============================================================================
// ShardMetrics
// ============================================================================

/**
 * @brief Live load metrics for a single shard.
 *
 * Updated by external observers (heartbeat or query-completion callbacks).
 * All fields are weakly consistent snapshots, not atomic.
 */
struct ShardMetrics {
    double      cpu_percent      = 0.0;  ///< CPU utilisation [0, 100].
    std::size_t pending_queries  = 0;    ///< Queries currently in-flight.
    double      p99_latency_ms   = 0.0;  ///< 99th-percentile response time (ms).
    bool        available        = true; ///< false = shard is down / draining.
    std::chrono::steady_clock::time_point last_updated{};
};

// ============================================================================
// ShardLoadBalancer
// ============================================================================

/**
 * @brief Selects the least-loaded shard for each incoming query.
 *
 * ### Usage
 * @code
 * ShardLoadBalancer lb({"shard-0", "shard-1", "shard-2"});
 *
 * // Update metrics (e.g., from heartbeat thread):
 * lb.updateMetrics("shard-1", {.cpu_percent=60, .pending_queries=80});
 *
 * // Select best shard for a query:
 * auto id = lb.selectShard();         // returns "shard-0" (lowest score)
 *
 * // Notify after completion:
 * lb.reportCompletion("shard-0", 12.4);  // records response time
 * @endcode
 */
class ShardLoadBalancer {
public:
    /**
     * @brief Weight configuration for the load score formula.
     */
    struct Weights {
        double cpu     = 0.30; ///< Weight for CPU utilisation.
        double queue   = 0.40; ///< Weight for pending query count.
        double latency = 0.30; ///< Weight for p99 latency.
    };

    /**
     * @brief Configuration for the load balancer.
     */
    struct Config {
        Weights     weights;
        double      max_pending = 200.0; ///< Normalisation baseline for pending count.
        double      max_p99_ms  = 200.0; ///< Normalisation baseline for p99 latency.
        /// Client-affinity: prefer same shard for same client hash.
        bool        sticky_sessions  = false;
        double      sticky_threshold = 80.0; ///< Abandon stickiness if score > this.
    };

    /**
     * @brief Per-shard statistics exported by @ref statistics().
     */
    struct ShardStatistics {
        std::string shard_id = {};
        ShardMetrics metrics;
        double       load_score     = 0.0;
        std::size_t  total_selected = 0;
    };

    /**
     * @brief Constructs the balancer with default weights.
     * @param shard_ids  List of shard identifiers.
     */
    explicit ShardLoadBalancer(std::vector<std::string> shard_ids);

    /**
     * @brief Constructs the balancer with custom configuration.
     * @param shard_ids  List of shard identifiers.
     * @param cfg        Weight configuration.
     */
    ShardLoadBalancer(std::vector<std::string> shard_ids, Config cfg);

    ~ShardLoadBalancer() = default;

    // Non-copyable.
    ShardLoadBalancer(const ShardLoadBalancer&)            = delete;
    ShardLoadBalancer& operator=(const ShardLoadBalancer&) = delete;

    /**
     * @brief Registers a new shard dynamically.
     * @param shard_id  Unique identifier for the shard.
     */
    void addShard(const std::string& shard_id);

    /**
     * @brief Removes a shard (graceful drain).
     *
     * The shard will not be selected for new queries.  Returns false if
     * the shard was not registered.
     */
    bool removeShard(const std::string& shard_id);

    /**
     * @brief Updates load metrics for a shard.
     *
     * @param shard_id  Shard to update.
     * @param metrics   New metrics.  @p available defaults to true.
     */
    void updateMetrics(const std::string& shard_id, const ShardMetrics& metrics);

    /**
     * @brief Marks a shard as available or unavailable.
     * @param shard_id   Shard to update.
     * @param available  New availability state.
     */
    void setAvailable(const std::string& shard_id, bool available);

    /**
     * @brief Selects the shard with the lowest current load score.
     *
     * @param client_hash  Optional client hash for sticky-session routing.
     * @return The shard ID to route to.
     * @throws std::runtime_error if no shards are available.
     */
    [[nodiscard]] std::string selectShard(
        std::optional<std::size_t> client_hash = std::nullopt) const;

    /**
     * @brief Records query completion for latency tracking.
     *
     * @param shard_id     Shard that processed the query.
     * @param latency_ms   Observed latency.
     */
    void reportCompletion(const std::string& shard_id, double latency_ms);

    /**
     * @brief Computes the load score for a shard.
     *
     * @param m  Metrics snapshot.
     * @return Score in [0, 100].
     */
    [[nodiscard]] double computeScore(const ShardMetrics& m) const noexcept;

    /// @brief Returns statistics for all registered shards.
    [[nodiscard]] std::vector<ShardStatistics> statistics() const;

    /// @brief Returns the number of registered (and available) shards.
    [[nodiscard]] std::size_t availableShardCount() const noexcept;

private:
    struct ShardState {
        ShardMetrics metrics;
        std::size_t  total_selected = 0;
    };

    mutable std::mutex                           mutex_;
    std::vector<std::string>                     shard_order_; ///< Insertion order.
    std::unordered_map<std::string, ShardState>  shards_;
    Config                                       cfg_;
};

}  // namespace themis::sharding
