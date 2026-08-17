/**
 * @file observability.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Replication Observability API
 *
 * Provides high-level monitoring, diagnostics, and health-scoring for the
 * replication subsystem.  ReplicationObserver wraps a ReplicationManager
 * instance and the embedded ReplicationAnalytics data to surface
 * actionable insight without exposing internal implementation details.
 *
 * Target: v1.7.0
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "replication/replication_manager.h"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace themisdb {
namespace replication {

// ============================================================================
// ReplicationObserver configuration (at namespace scope to avoid GCC issues
// with default member initializers and default arguments in the same class)
// ============================================================================

struct ReplicationObserverConfig {
    int64_t critical_lag_threshold_ms   = 10000; ///< Lag above this → is_critical = true
    int64_t bottleneck_lag_threshold_ms =  5000; ///< Lag above this → network bottleneck
    int64_t high_lag_threshold_ms       =  2000; ///< Lag above this degrades lag_score
};

/**
 * ReplicationObserver
 *
 * A read-only façade over ReplicationManager that exposes:
 *   - Per-replica lag snapshots with critical-lag detection
 *   - Cluster topology as a structured graph
 *   - Bottleneck detection (network / disk / CPU)
 *   - An overall health score (0 – 100)
 *
 * Thread-safety: All methods are safe to call concurrently.
 *
 * Example:
 * @code
 *   ReplicationObserver observer(repl_mgr);
 *   auto lags = observer.getLagSnapshots(std::chrono::minutes(5));
 *   for (const auto& lag : lags)
 *       if (lag.is_critical) alert("Critical lag on " + lag.replica_id);
 *
 *   auto health = observer.calculateHealthScore();
 *   std::cout << "Replication health: " << health.overall_score << "/100\n";
 * @endcode
 */
class ReplicationObserver {
public:
    // -----------------------------------------------------------------------
    // Data types
    // -----------------------------------------------------------------------

    /**
     * Point-in-time lag measurement for a single replica.
     */
    struct LagSnapshot {
        std::string replica_id;
        int64_t     lag_ms        = 0;    ///< Current replication lag in ms
        uint64_t    lag_sequences = 0;    ///< Number of WAL entries behind leader
        std::chrono::system_clock::time_point captured_at;
        bool        is_critical   = false; ///< True when lag exceeds critical_lag_threshold_ms
    };

    /**
     * Node in the replication topology graph.
     */
    struct TopologyNode {
        std::string              node_id;
        ReplicationRole          role;
        std::vector<std::string> downstream_replicas; ///< Direct followers of this node
        std::string              upstream_source;      ///< Empty string if this is the leader
        bool                     is_healthy = true;
        HealthStatus             health_status = HealthStatus::UNKNOWN;
    };

    /**
     * Detected performance bottleneck in the replication pipeline.
     */
    struct Bottleneck {
        enum class Type { NETWORK, DISK_IO, CPU, MEMORY };

        Type                     type;
        std::string              affected_replica;
        double                   severity = 0.0;  ///< 0.0 (minor) – 1.0 (critical)
        std::string              description;
        std::vector<std::string> recommendations;
    };

    /**
     * Composite health score for the replication cluster.
     */
    struct HealthScore {
        int                      overall_score      = 100; ///< Weighted composite, 0–100
        int                      lag_score          = 100; ///< 0–100 (100 = no lag)
        int                      throughput_score   = 100; ///< 0–100 (100 = healthy throughput)
        int                      availability_score = 100; ///< 0–100 (100 = all replicas healthy)
        std::vector<std::string> issues;                   ///< Human-readable descriptions of problems
    };

    // Convenience alias for the configuration type
    using ObserverConfig = ReplicationObserverConfig;

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * Construct an observer backed by the given ReplicationManager.
     * The observer holds a shared_ptr to the manager so it remains valid for
     * the observer's lifetime.
     */
    explicit ReplicationObserver(
        std::shared_ptr<ReplicationManager> manager,
        const ObserverConfig& config = ObserverConfig{}
    );

    ~ReplicationObserver() = default;

    // Non-copyable, movable
    ReplicationObserver(const ReplicationObserver&) = delete;
    ReplicationObserver& operator=(const ReplicationObserver&) = delete;
    ReplicationObserver(ReplicationObserver&&) noexcept = default;
    ReplicationObserver& operator=(ReplicationObserver&&) noexcept = default;

    // -----------------------------------------------------------------------
    // Observability API
    // -----------------------------------------------------------------------

    /**
     * Return a lag snapshot for every known replica.
     * The `window` parameter is currently informational; lag is measured at
     * the time of the call from the heartbeat timestamps in ReplicaInfo.
     *
     * Performance: O(N) in the number of replicas, ≤ 1 ms.
     */
    std::vector<LagSnapshot> getLagSnapshots(
        std::chrono::seconds window = std::chrono::seconds(60)
    ) const;

    /**
     * Return the current replication topology as a list of TopologyNode
     * records, one per known cluster member (including the local node).
     *
     * Performance: O(N), ≤ 1 ms.
     */
    std::vector<TopologyNode> getTopology() const;

    /**
     * Analyse replica lag and health metrics to identify bottlenecks.
     * Returns an empty vector when the cluster is healthy.
     *
     * Performance: O(N), ≤ 5 ms.
     */
    std::vector<Bottleneck> detectBottlenecks() const;

    /**
     * Compute an overall health score for the replication cluster.
     * Scores degrade based on: replica lag, failed health checks, and
     * recent failover events.
     *
     * Performance: O(N), ≤ 5 ms.
     */
    HealthScore calculateHealthScore() const;

private:
    std::shared_ptr<ReplicationManager> manager_;
    ObserverConfig config_;
};

} // namespace replication
} // namespace themisdb
