/**
 * @file cache_replication.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Type of cache replication event
 */
enum class CacheReplicationEventType {
    WRITE,         ///< A new entry was written (put)
    INVALIDATE,    ///< One or more entries were invalidated by pattern
    INVALIDATE_TENANT, ///< All entries for a tenant were invalidated
    SNAPSHOT,      ///< Full snapshot for bootstrapping a new replica
};

/**
 * @brief A single replication event transmitted to replica nodes.
 *
 * For WRITE events:
 *   - key:        cache key (fingerprint, possibly tenant-scoped)
 *   - payload:    serialised JSON of the cache entry
 *   - tenant_id:  tenant identifier (empty if isolation is disabled)
 *   - ttl_seconds: remaining TTL for the entry
 *
 * For INVALIDATE events:
 *   - pattern:    regex pattern used in AdaptiveQueryCache::invalidate()
 *
 * For INVALIDATE_TENANT events:
 *   - tenant_id:  tenant whose entries must be evicted
 *
 * For SNAPSHOT events:
 *   - payload:    newline-delimited JSON (same format as exportSnapshot())
 */
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

/**
 * @brief Interface that replica transport adapters must implement.
 *
 * Each registered listener receives every WRITE, INVALIDATE, and SNAPSHOT
 * event produced by the primary AdaptiveQueryCache. If a listener throws
 * or returns false, the CacheReplicationManager marks the replica as
 * UNHEALTHY and retries on the next heartbeat interval.
 */
class ICacheReplicationListener {
public:
    virtual ~ICacheReplicationListener() = default;

    /**
     * @brief Called on every cache mutation that must be replicated.
     *
     * @note This callback is invoked outside the cache tier's internal mutexes.
     *       It is safe to call blocking operations here, although for
     *       network-bound transports dispatching to a background queue is
     *       recommended to keep cache write latency low.
     *       Implementations must not call back into the originating
     *       AdaptiveQueryCache to avoid potential re-entrancy issues.
     *
     * @param event  Structured replication event.
     * @return true on success; false signals a transient failure.
     */
    [[nodiscard]] virtual bool onReplicationEvent(const CacheReplicationEvent& event) = 0;

    /**
     * @brief Called periodically so the listener can report its liveness.
     * @return true if the replica is reachable and healthy.
     */
    virtual bool ping() { return true; }

    /**
     * @brief Human-readable replica identifier (host:port, node ID, …).
     */
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
    /// Maximum consecutive delivery failures before marking a replica UNHEALTHY.
    uint32_t max_consecutive_failures = 3;

    /// Interval at which unhealthy replicas are probed for recovery (ms).
    uint32_t health_probe_interval_ms = 5000;

    /// If true, the primary blocks on WRITE events until at least one replica
    /// acknowledges (semi-sync).  If false, events are fire-and-forget (async).
    bool semi_sync = false;

    /// Enable replication (can be set to false to disable without removing listeners).
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

    /// Return a JSON snapshot of the stats (copy atomics to avoid races).
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

/**
 * @brief Manages a set of replica listeners and fan-outs cache events to them.
 *
 * Usage:
 * @code
 *   auto mgr = std::make_shared<CacheReplicationManager>(config);
 *   mgr->addReplica(std::make_shared<MyTransportListener>("node2:7001"));
 *   cache.setReplicationListener(mgr);
 * @endcode
 *
 * Thread-safety: all public methods are thread-safe.
 */
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

    /**
     * @brief Add a replica listener.
     *
     * Immediately sends a SNAPSHOT event so the new replica bootstraps its
     * cache state from the provided snapshot data (may be empty).
     *
     * @param listener       Transport adapter for the replica.
     * @param snapshot_ndjson NDJSON snapshot string (from exportSnapshot).
     *                        Pass empty string to skip bootstrap snapshot.
     */
    void addReplica(std::shared_ptr<ICacheReplicationListener> listener,
                    const std::string& snapshot_ndjson = "");

    /**
     * @brief Remove a replica by its replicaId().
     */
    void removeReplica(const std::string& replica_id);

    /**
     * @brief Number of registered replicas (healthy + unhealthy).
     */
    size_t replicaCount() const;

    /**
     * @brief Probe all unhealthy replicas to check whether they have recovered.
     *
     * Intended to be called from a periodic background timer; also safe to
     * call from a test.
     */
    void probeUnhealthyReplicas();

    // ---------------------------------------------------------------------------
    // ICacheReplicationListener implementation (used when this manager itself
    // is nested inside another manager or used as a listener)
    // ---------------------------------------------------------------------------

    bool onReplicationEvent(const CacheReplicationEvent& event) override;
    bool ping() override;
    std::string replicaId() const override { return "CacheReplicationManager"; }

    // ---------------------------------------------------------------------------
    // Convenience helpers called by AdaptiveQueryCache hooks
    // ---------------------------------------------------------------------------

    /**
     * @brief Fan-out a WRITE event to all healthy replicas.
     *
     * @param key         Cache key (fingerprint, possibly tenant-scoped).
     * @param payload     Serialised JSON of the cache entry.
     * @param tenant_id   Tenant identifier (empty when isolation is disabled).
     * @param ttl_seconds Remaining TTL for the entry.
     */
    void notifyWrite(const std::string& key,
                     const std::string& payload,
                     const std::string& tenant_id,
                     int ttl_seconds);

    /**
     * @brief Fan-out an INVALIDATE event to all healthy replicas.
     *
     * @param pattern  Regex pattern used in AdaptiveQueryCache::invalidate().
     */
    void notifyInvalidate(const std::string& pattern);

    /**
     * @brief Fan-out an INVALIDATE_TENANT event to all healthy replicas.
     *
     * @param tenant_id  Tenant whose entries must be evicted on replicas.
     */
    void notifyInvalidateTenant(const std::string& tenant_id);

    // ---------------------------------------------------------------------------
    // Observability
    // ---------------------------------------------------------------------------

    /**
     * @brief Return replication statistics as JSON.
     */
    nlohmann::json getStats() const;

    /**
     * @brief Return per-replica health as JSON array.
     */
    nlohmann::json getReplicaHealth() const;

    /**
     * @brief Access raw stats counters.
     */
    const CacheReplicationStats& stats() const { return stats_; }

private:
    CacheReplicationConfig config_;
    mutable std::mutex replicas_mutex_;
    std::vector<CacheReplicaState> replicas_;
    mutable CacheReplicationStats stats_;
    mutable std::atomic<uint64_t> sequence_{0};

    /// Dispatch an event to all healthy replicas, updating health state.
    void dispatch(const CacheReplicationEvent& event);

    /// Build a CacheReplicationEvent with a fresh sequence number and timestamp.
    CacheReplicationEvent makeEvent(CacheReplicationEventType type) const;

    static const char* healthToString(CacheReplicaHealth h);
};

} // namespace cache
} // namespace themis
