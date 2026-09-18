/**
 * @file cache_replication.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

/**
 * Cache Replication for High-Availability Deployments
 *
 * Provides an interface for propagating cache writes, invalidations, and
 * full snapshots to standby replica nodes so that a replica can serve
 * cached data immediately after a primary failure.
 *
 * Design principles:
 * - Transport-agnostic: the caller supplies a concrete ICacheReplicationListener
 *   implementation (in-process, TCP, gRPC, …).
 * - Graceful degradation: a failing replica is marked UNHEALTHY and skipped
 *   until it recovers; the primary continues serving requests.
 * - Consistent with the existing themisdb::replication::IReplicationListener
 *   interface style used in include/replication/replication_manager.h.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <chrono>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace themis {
namespace cache {

// ---------------------------------------------------------------------------
// CacheReplicationEvent – payload for a single replication message
// ---------------------------------------------------------------------------

enum class CacheReplicationEventType {
    WRITE,         ///< A new entry was written (put)
    INVALIDATE,    ///< One or more entries were invalidated by pattern
    INVALIDATE_TENANT, ///< All entries for a tenant were invalidated
    SNAPSHOT,      ///< Full snapshot for bootstrapping a new replica
};

struct CacheReplicationEvent {
    CacheReplicationEventType type = CacheReplicationEventType::WRITE;
    std::string key;            ///< Cache key (WRITE)
    std::string pattern;        ///< Invalidation pattern (INVALIDATE)
    std::string tenant_id;      ///< Tenant ID (WRITE / INVALIDATE_TENANT)
    std::string payload;        ///< Serialised entry or snapshot data
    int ttl_seconds = 0;        ///< Remaining TTL in seconds (WRITE)
    int64_t timestamp_ms = 0;   ///< Event wall-clock time (ms since epoch)
    uint64_t sequence = 0;      ///< Monotonically increasing sequence number
};

// ---------------------------------------------------------------------------
// ICacheReplicationListener – implement this to receive events
// ---------------------------------------------------------------------------

class ICacheReplicationListener {
public:
    /**
     * @brief ICache Replication Listener.
     * @return Return value.
     */
    virtual ~ICacheReplicationListener() = default;

    [[nodiscard]] virtual bool onReplicationEvent(const CacheReplicationEvent& event) = 0;

    /**
     * @brief Ping.
     * @return True when the operation succeeds.
     * @details Implements ping without additional internal calls.
     */
    virtual bool ping() { return true; }

    [[nodiscard]] virtual std::string replicaId() const = 0;
};

// ---------------------------------------------------------------------------
// CacheReplicaState – runtime health of a single replica
// ---------------------------------------------------------------------------

enum class CacheReplicaHealth {
    HEALTHY,   ///< Replica is reachable and receiving events
    DEGRADED,  ///< Some events were dropped; replica may be lagging
    UNHEALTHY, ///< Replica is not responding; events are being discarded
};

struct CacheReplicaState {
    std::shared_ptr<ICacheReplicationListener> listener;
    CacheReplicaHealth health = CacheReplicaHealth::HEALTHY;
    uint32_t consecutive_failures = 0;
    std::chrono::steady_clock::time_point last_success;
    std::chrono::steady_clock::time_point last_failure;
    uint64_t events_sent = 0;
    uint64_t events_failed = 0;
};

// ---------------------------------------------------------------------------
// CacheReplicationConfig
// ---------------------------------------------------------------------------

struct CacheReplicationConfig {
    uint32_t max_consecutive_failures = 3;

    uint32_t health_probe_interval_ms = 5000;

    bool semi_sync = false;

    bool enabled = true;
};

// ---------------------------------------------------------------------------
// CacheReplicationStats
// ---------------------------------------------------------------------------

struct CacheReplicationStats {
    std::atomic<uint64_t> events_dispatched{0};
    std::atomic<uint64_t> events_failed{0};
    std::atomic<uint64_t> replicas_unhealthy{0};
    std::atomic<uint64_t> snapshots_sent{0};

    nlohmann::json toJson() const {
        return {
            {"events_dispatched", events_dispatched.load()},
            {"events_failed",     events_failed.load()},
            {"replicas_unhealthy", replicas_unhealthy.load()},
            {"snapshots_sent",    snapshots_sent.load()},
        };
    }
};

// ---------------------------------------------------------------------------
// CacheReplicationManager
// ---------------------------------------------------------------------------

class CacheReplicationManager : public ICacheReplicationListener {
public:
    explicit CacheReplicationManager(const CacheReplicationConfig& config = {});
    ~CacheReplicationManager() override = default;

    // Non-copyable
    CacheReplicationManager(const CacheReplicationManager&) = delete;
    CacheReplicationManager& operator=(const CacheReplicationManager&) = delete;

    // ---------------------------------------------------------------------------
    // Replica registration
    // ---------------------------------------------------------------------------

    void addReplica(std::shared_ptr<ICacheReplicationListener> listener,
                    const std::string& snapshot_ndjson = "");

    /**
     * @brief Remove Replica.
     * @param[in] replica_id Identifier of the replica.
     */
    void removeReplica(const std::string& replica_id);

    /**
     * @brief Replica Count.
     * @return Return value.
     */
    size_t replicaCount() const;

    /**
     * @brief Probe Unhealthy Replicas.
     */
    void probeUnhealthyReplicas();

    // ---------------------------------------------------------------------------
    // ICacheReplicationListener implementation (used when this manager itself
    // is nested inside another manager or used as a listener)
    // ---------------------------------------------------------------------------

    bool onReplicationEvent(const CacheReplicationEvent& event) override;
    bool ping() override;
    std::string replicaId() const override { return "CacheReplicationManager"; }

    /**
     * @brief --------------------------------------------------------------------------- Convenience helpers called by AdaptiveQueryCache hooks ---------------------------------------------------------------------------
     * @param[in] key Input parameter.
     * @param[in] payload Input parameter.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] ttl_seconds Input parameter.
     */

    void notifyWrite(const std::string& key,
                     const std::string& payload,
                     const std::string& tenant_id,
                     int ttl_seconds);

    /**
     * @brief Notify Invalidate.
     * @param[in] pattern Input parameter.
     */
    void notifyInvalidate(const std::string& pattern);

    /**
     * @brief Notify Invalidate Tenant.
     * @param[in] tenant_id Identifier of the tenant.
     */
    void notifyInvalidateTenant(const std::string& tenant_id);

    // ---------------------------------------------------------------------------
    // Observability
    // ---------------------------------------------------------------------------

    /**
     * @brief Get Stats.
     * @return Return value.
     */
    nlohmann::json getStats() const;

    /**
     * @brief Get Replica Health.
     * @return Return value.
     */
    nlohmann::json getReplicaHealth() const;

    const CacheReplicationStats& stats() const { return stats_; }

private:
    CacheReplicationConfig config_;
    mutable std::mutex replicas_mutex_;
    std::vector<CacheReplicaState> replicas_;
    mutable CacheReplicationStats stats_;
    mutable std::atomic<uint64_t> sequence_{0};

    /**
     * @brief Dispatch.
     * @param[in] event Input parameter.
     */
    void dispatch(const CacheReplicationEvent& event);

    /**
     * @brief Make Event.
     * @param[in] type Input parameter.
     * @return Return value.
     */
    CacheReplicationEvent makeEvent(CacheReplicationEventType type) const;

    /**
     * @brief Health To String.
     * @param[in] h Input parameter.
     * @return Pointer to the result.
     */
    static const char* healthToString(CacheReplicaHealth h);
};

} // namespace cache
} // namespace themis
