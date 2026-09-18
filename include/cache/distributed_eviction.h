/**
 * @file distributed_eviction.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

enum class DistributedEvictionReason : uint8_t {
    CAPACITY_PRESSURE, ///< Local tier reached capacity; LRU/LFU victim selected.
    TTL_EXPIRED,       ///< Entry TTL elapsed on the originating node.
    EXPLICIT_EVICT,    ///< Caller explicitly requested eviction via the API.
    PATTERN_EVICT,     ///< Glob/regex pattern eviction was requested.
    TENANT_EVICT,      ///< All entries for a tenant were evicted.
    FLUSH,             ///< Entire cache (or tenant partition) was flushed.
};

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

using DistributedEvictionListener =
    std::function<void(const DistributedEvictionEvent& event)>;

struct IDistributedEviction {
    /**
     * @brief IDistributed Eviction.
     * @return Return value.
     */
    virtual ~IDistributedEviction() = default;

    // -----------------------------------------------------------------------
    // Eviction broadcast API
    // -----------------------------------------------------------------------

    virtual void evict(const std::string& key,
                       const std::string& tenant_id = "",
                       DistributedEvictionReason reason =
                           DistributedEvictionReason::CAPACITY_PRESSURE) = 0;

    virtual void evictByPattern(const std::string& pattern,
                                const std::string& tenant_id = "") = 0;

    /**
     * @brief Evict By Tenant.
     * @param[in] tenant_id Identifier of the tenant.
     */
    virtual void evictByTenant(const std::string& tenant_id) = 0;

    virtual void flush(const std::string& tenant_id = "") = 0;

    // -----------------------------------------------------------------------
    // Listener registration
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual uint64_t registerEvictionListener(DistributedEvictionListener listener) = 0;

    /**
     * @brief Unregister Eviction Listener.
     * @param[in] handle Input parameter.
     */
    virtual void unregisterEvictionListener(uint64_t handle) = 0;

    // -----------------------------------------------------------------------
    // Observability
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual DistributedEvictionStats stats() const = 0;

    [[nodiscard]] virtual uint64_t peerCount() const = 0;

    [[nodiscard]] virtual bool isHealthy() const = 0;
};

} // namespace cache
} // namespace themis
