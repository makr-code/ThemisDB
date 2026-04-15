/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            distributed_eviction.h                             ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-15 07:06:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     217                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f741f92339  2026-04-12  feat(cache): Phase 6 distribution headers — IDistributedE... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * @file distributed_eviction.h
 * @brief Cross-node coordinated eviction interface for clustered deployments.
 *
 * `IDistributedEviction` allows a cache node to broadcast eviction decisions
 * to all peers in a cluster.  This is complementary to `IRemoteCachePeer`
 * (which handles data-change invalidation) — distributed eviction is
 * triggered by capacity pressure, not by data mutation.
 *
 * Design constraints:
 *   - All methods are thread-safe; implementations serialise access internally.
 *   - `evict()` is fire-and-forget; callers must not block on peer ACKs.
 *   - Implementations must degrade gracefully when some peers are unreachable.
 *   - Listeners registered via `registerEvictionListener()` are called on a
 *     background delivery thread; they must not perform blocking I/O.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace themis {
namespace cache {

// ============================================================================
// DistributedEvictionEvent — payload for eviction notifications
// ============================================================================

/**
 * @brief Reason a distributed eviction was triggered.
 */
enum class DistributedEvictionReason : uint8_t {
    CAPACITY_PRESSURE, ///< Local tier reached capacity; LRU/LFU victim selected.
    TTL_EXPIRED,       ///< Entry TTL elapsed on the originating node.
    EXPLICIT_EVICT,    ///< Caller explicitly requested eviction via the API.
    PATTERN_EVICT,     ///< Glob/regex pattern eviction was requested.
    TENANT_EVICT,      ///< All entries for a tenant were evicted.
    FLUSH,             ///< Entire cache (or tenant partition) was flushed.
};

/**
 * @brief Payload broadcast to remote peers when a local eviction occurs.
 */
struct DistributedEvictionEvent {
    std::string key;           ///< Affected cache key; empty for FLUSH/TENANT_EVICT.
    std::string tenant_id;     ///< Tenant scope; empty = global.
    std::string pattern;       ///< Glob/regex; non-empty only for PATTERN_EVICT.
    DistributedEvictionReason reason = DistributedEvictionReason::CAPACITY_PRESSURE;
    int64_t     timestamp_utc_ms = 0; ///< Wall-clock time of the originating eviction (ms since epoch).
    std::string origin_node_id;       ///< Identifier of the node that triggered the eviction.
};

// ============================================================================
// DistributedEvictionStats — observable counters
// ============================================================================

/**
 * @brief Snapshot of distributed eviction statistics.
 */
struct DistributedEvictionStats {
    uint64_t evictions_sent     = 0; ///< Eviction events broadcast to peers.
    uint64_t evictions_received = 0; ///< Eviction events received from peers.
    uint64_t evictions_dropped  = 0; ///< Events dropped (peer unreachable or queue full).
    uint64_t peer_count         = 0; ///< Number of currently registered peers.
    uint64_t healthy_peer_count = 0; ///< Peers that reported healthy on last probe.
};

// ============================================================================
// IDistributedEviction — cross-node coordinated eviction interface
// ============================================================================

/**
 * @brief Callback type for incoming eviction events from remote peers.
 *
 * Called on a background delivery thread.  Implementations must be
 * non-blocking and must not throw.
 */
using DistributedEvictionListener =
    std::function<void(const DistributedEvictionEvent& event)>;

/**
 * @brief Pure-virtual interface for cross-node cache eviction coordination.
 *
 * Each node in a cluster holds one `IDistributedEviction` implementation.
 * When a local eviction decision is made (capacity pressure, TTL expiry, or
 * explicit request), the node calls `evict()` / `evictByPattern()` /
 * `evictByTenant()` / `flush()` to propagate the decision to all peers.
 *
 * Peer implementations receive the notification and apply the eviction to
 * their local cache tier, ensuring cluster-wide consistency.
 *
 * Thread-safety: all public methods are thread-safe.
 */
struct IDistributedEviction {
    virtual ~IDistributedEviction() = default;

    // -----------------------------------------------------------------------
    // Eviction broadcast API
    // -----------------------------------------------------------------------

    /**
     * @brief Broadcast eviction of a single key to all peers.
     *
     * Fire-and-forget; callers are not blocked on peer acknowledgements.
     *
     * @param key       Cache key to evict on all peers.
     * @param tenant_id Optional tenant scope; empty = global key-space.
     * @param reason    Why this eviction was triggered (for metrics/logging).
     */
    virtual void evict(const std::string& key,
                       const std::string& tenant_id = "",
                       DistributedEvictionReason reason =
                           DistributedEvictionReason::CAPACITY_PRESSURE) = 0;

    /**
     * @brief Broadcast eviction of all keys matching a glob/regex pattern.
     *
     * @param pattern   Glob or regex pattern matching cache keys to evict.
     * @param tenant_id Optional tenant scope; empty = global key-space.
     */
    virtual void evictByPattern(const std::string& pattern,
                                const std::string& tenant_id = "") = 0;

    /**
     * @brief Broadcast eviction of all keys belonging to a tenant.
     *
     * @param tenant_id Tenant identifier; must not be empty.
     */
    virtual void evictByTenant(const std::string& tenant_id) = 0;

    /**
     * @brief Broadcast a full cache flush to all peers.
     *
     * If @p tenant_id is non-empty, only that tenant's partition is flushed.
     * If @p tenant_id is empty, all entries in the cache are flushed.
     *
     * @param tenant_id Optional tenant scope for a partial flush.
     */
    virtual void flush(const std::string& tenant_id = "") = 0;

    // -----------------------------------------------------------------------
    // Listener registration
    // -----------------------------------------------------------------------

    /**
     * @brief Register a listener that is called when a peer broadcasts an
     *        eviction event to this node.
     *
     * Multiple listeners may be registered; all are called in registration
     * order on the delivery thread.  Returns an opaque handle that can be
     * passed to `unregisterEvictionListener()` to remove the listener.
     *
     * @param listener  Callback invoked with the inbound event (non-blocking).
     * @return          Opaque registration handle.
     */
    virtual uint64_t registerEvictionListener(DistributedEvictionListener listener) = 0;

    /**
     * @brief Remove a previously registered listener.
     *
     * No-op if @p handle is unknown or was already removed.
     *
     * @param handle  Handle returned by `registerEvictionListener()`.
     */
    virtual void unregisterEvictionListener(uint64_t handle) = 0;

    // -----------------------------------------------------------------------
    // Observability
    // -----------------------------------------------------------------------

    /**
     * @brief Return a point-in-time snapshot of eviction statistics.
     */
    virtual DistributedEvictionStats stats() const = 0;

    /**
     * @brief Return the number of peer nodes currently registered.
     */
    virtual uint64_t peerCount() const = 0;

    /**
     * @brief Return true when at least one peer is believed to be healthy.
     */
    virtual bool isHealthy() const = 0;
};

} // namespace cache
} // namespace themis
