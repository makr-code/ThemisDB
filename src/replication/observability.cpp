/**
 * @file observability.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Replication Observability Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "replication/observability.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace themisdb {
namespace replication {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ReplicationObserver::ReplicationObserver(
    std::shared_ptr<ReplicationManager> manager,
    const ObserverConfig& config
)
    : manager_(std::move(manager))
    , config_(config)
{}

// ---------------------------------------------------------------------------
// getLagSnapshots
// ---------------------------------------------------------------------------

std::vector<ReplicationObserver::LagSnapshot>
ReplicationObserver::getLagSnapshots(std::chrono::seconds /*window*/) const
{
    std::vector<LagSnapshot> result;
    const auto replicas = manager_->getReplicas();
    result.reserve(replicas.size());

    const auto now = std::chrono::system_clock::now();
    for (const auto& replica : replicas) {
        LagSnapshot snap;
        snap.replica_id   = replica.node_id;
        snap.lag_ms       = replica.replicationLagMs();
        snap.lag_sequences =
            (snap.lag_ms > 0)
            ? static_cast<uint64_t>(snap.lag_ms / 10) // approximate
            : 0;
        snap.captured_at  = now;
        snap.is_critical  = (snap.lag_ms > config_.critical_lag_threshold_ms);
        result.push_back(snap);
    }
    return result;
}

// ---------------------------------------------------------------------------
// getTopology
// ---------------------------------------------------------------------------

std::vector<ReplicationObserver::TopologyNode>
ReplicationObserver::getTopology() const
{
    std::vector<TopologyNode> nodes;
    const auto replicas = manager_->getReplicas();
    nodes.reserve(replicas.size());  // Pre-allocate to avoid reallocations
    
    // Build a map: node_id → role for downstream lookup
    std::map<std::string, ReplicationRole> role_map;
    for (const auto& r : replicas) {
        role_map[r.node_id] = r.role;
    }

    // Identify the leader
    const std::string leader_endpoint = manager_->getLeaderEndpoint();

    for (const auto& replica : replicas) {
        TopologyNode node;
        node.node_id       = replica.node_id;
        node.role          = replica.role;
        node.health_status = replica.health_status;
        node.is_healthy    = (replica.health_status == HealthStatus::HEALTHY ||
                              replica.health_status == HealthStatus::UNKNOWN);

        // For followers, upstream_source is the leader
        if (replica.role == ReplicationRole::FOLLOWER ||
            replica.role == ReplicationRole::OBSERVER)
        {
            node.upstream_source = leader_endpoint;
        }
        // Leader has no upstream
        // Downstream replicas are all followers/observers
        // (populated after iterating all replicas)

        nodes.push_back(node);
    }

    // Populate downstream_replicas for the leader node
    for (auto& node : nodes) {
        if (node.role == ReplicationRole::LEADER) {
            for (const auto& r : replicas) {
                if (r.role == ReplicationRole::FOLLOWER ||
                    r.role == ReplicationRole::OBSERVER ||
                    r.role == ReplicationRole::WITNESS)
                {
                    node.downstream_replicas.push_back(r.node_id);
                }
            }
        }
    }

    return nodes;
}

// ---------------------------------------------------------------------------
// detectBottlenecks
// ---------------------------------------------------------------------------

std::vector<ReplicationObserver::Bottleneck>
ReplicationObserver::detectBottlenecks() const
{
    std::vector<Bottleneck> bottlenecks;
    const auto replicas = manager_->getReplicas();
    bottlenecks.reserve(replicas.size());  // Pre-allocate to avoid reallocations

    for (const auto& replica : replicas) {
        const int64_t lag = replica.replicationLagMs();

        if (replica.health_status == HealthStatus::FAILED) {
            Bottleneck b;
            b.type             = Bottleneck::Type::NETWORK;
            b.affected_replica = replica.node_id;
            b.severity         = 1.0;
            b.description      = "Replica " + replica.node_id +
                                 " is in FAILED state – no heartbeat received.";
            b.recommendations  = {
                "Check network connectivity to " + replica.endpoint,
                "Inspect replica logs for crash or OOM events",
                "Consider triggering a manual failover if not auto-recovering"
            };
            bottlenecks.push_back(std::move(b));
        } else if (lag > config_.bottleneck_lag_threshold_ms) {
            Bottleneck b;
            b.affected_replica = replica.node_id;
            b.severity = std::min(1.0,
                static_cast<double>(lag) /
                static_cast<double>(config_.critical_lag_threshold_ms));

            if (replica.health_status == HealthStatus::DEGRADED) {
                b.type = Bottleneck::Type::NETWORK;
                b.description = "Replica " + replica.node_id +
                                " is DEGRADED with lag=" +
                                std::to_string(lag) + "ms – possible network issue.";
                b.recommendations = {
                    "Check bandwidth and packet loss between leader and " + replica.node_id,
                    "Consider enabling Zstd WAL compression to reduce bandwidth",
                    "Verify that the replica is not CPU or I/O saturated"
                };
            } else {
                b.type = Bottleneck::Type::DISK_IO;
                b.description = "Replica " + replica.node_id +
                                " has high lag=" +
                                std::to_string(lag) + "ms – possible disk I/O bottleneck.";
                b.recommendations = {
                    "Check disk I/O utilisation on " + replica.node_id,
                    "Increase WAL batch size to reduce fsync frequency",
                    "Consider using parallel replication to speed up WAL application"
                };
            }
            bottlenecks.push_back(std::move(b));
        }
    }
    return bottlenecks;
}

// ---------------------------------------------------------------------------
// calculateHealthScore
// ---------------------------------------------------------------------------

ReplicationObserver::HealthScore
ReplicationObserver::calculateHealthScore() const
{
    HealthScore score;
    const auto replicas = manager_->getReplicas();
    if (replicas.empty()) {
        score.overall_score      = 100;
        score.lag_score          = 100;
        score.throughput_score   = 100;
        score.availability_score = 100;
        return score;
    }

    // Availability: penalise for failed/degraded replicas
    int failed   = 0;
    int degraded = 0;
    for (const auto& r : replicas) {
        if (r.health_status == HealthStatus::FAILED)   ++failed;
        else if (r.health_status == HealthStatus::DEGRADED) ++degraded;
    }
    const int total = static_cast<int>(replicas.size());
    // Each failed replica costs 30 points; each degraded costs 10
    score.availability_score = std::max(0,
        100 - (failed * 30) - (degraded * 10));
    if (failed > 0)
        score.issues.push_back(std::to_string(failed) + " replica(s) FAILED");
    if (degraded > 0)
        score.issues.push_back(std::to_string(degraded) + " replica(s) DEGRADED");

    // Lag score: measure max lag across all replicas
    int64_t max_lag_ms = 0;
    for (const auto& r : replicas) {
        max_lag_ms = std::max(max_lag_ms, r.replicationLagMs());
    }
    if (max_lag_ms > config_.critical_lag_threshold_ms) {
        score.lag_score = 0;
        score.issues.push_back("Max replication lag " + std::to_string(max_lag_ms) +
                               "ms exceeds critical threshold " +
                               std::to_string(config_.critical_lag_threshold_ms) + "ms");
    } else if (max_lag_ms > config_.high_lag_threshold_ms) {
        score.lag_score = std::max(0,
            100 - static_cast<int>(
                80.0 * (max_lag_ms - config_.high_lag_threshold_ms) /
                (config_.critical_lag_threshold_ms - config_.high_lag_threshold_ms)));
        score.issues.push_back("Max replication lag " + std::to_string(max_lag_ms) + "ms is elevated");
    }

    // Throughput score: approximate from healthy replica count
    // A cluster with all replicas healthy gets 100.
    const int healthy = total - failed - degraded;
    score.throughput_score = (total > 0)
        ? std::max(0, 100 * healthy / total)
        : 100;

    // Overall: weighted average (lag 40%, availability 40%, throughput 20%)
    score.overall_score = (score.lag_score * 40 +
                           score.availability_score * 40 +
                           score.throughput_score * 20) / 100;

    return score;
}

} // namespace replication
} // namespace themisdb

