/**
 * @file sharding_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <mutex>
#include <stdexcept>
#include "themis/edition.h"
#include "sharding/consistent_hash.h"

namespace themis {
namespace sharding {

// ============================================================================
// SHARD NODE CONFIGURATION
// ============================================================================

struct ShardNodeInfo {
    uint32_t node_id;
    std::string node_address;
    std::string node_role;  // PRIMARY, REPLICA, ARBITER
    bool is_healthy;
};

// ============================================================================
// SHARDING MANAGER - EDITION-AWARE
// ============================================================================

/** @brief SHARDING MANAGER - EDITION-AWARE. */
class ShardingManager {
public:
    // Singleton instance
    static ShardingManager& GetInstance() {
        static ShardingManager instance;
        return instance;
    }

    // Get maximum number of shard nodes allowed in this edition
    static constexpr int GetMaxShardNodes() {
        return edition::SHARDING_MAX_NODES;
    }

    // Check if sharding is available in this edition
    static constexpr bool IsShardingAvailable() {
        return GetMaxShardNodes() > 1;
    }

    // Add a new shard node to the cluster
    // Throws exception if would exceed edition limit
    void AddShardNode(const ShardNodeInfo& node);

    // Get number of currently configured shard nodes
    size_t GetNodeCount() const;

    // Get remaining node capacity in this edition
    int GetRemainingNodeCapacity() const;

    // Validate node configuration before adding
    void ValidateNodeCount(size_t requested_nodes);

    // Get all configured shard nodes
    std::vector<ShardNodeInfo> GetAllNodes() const;

    // Check health status of all nodes
    int GetHealthyNodeCount() const;

    // Remove a shard node from configuration
    bool RemoveShardNode(uint32_t node_id);

    // Get edition-specific sharding capability information
    std::string GetShardingCapabilityInfo() const;

    // ----------------------------------------------------------------
    // Key-based shard routing (consistent-hashing)
    // ----------------------------------------------------------------

    /**
     * Return the shard responsible for a single document key.
     *
     * The routing key is formed as "<collection>/<key>" and resolved
     * via the consistent-hash ring that is kept in sync with the node
     * registry.  Returns an empty string when no nodes are registered.
     *
     * @param collection  Collection name (used to namespace the key)
     * @param key         Document / partition key
     * @return Shard identifier (node_address of the owning node), or ""
     */
    std::string GetShardForKey(const std::string& collection,
                               const std::string& key) const;

    /**
     * Return all shards that may hold keys in [min_key, max_key].
     *
     * Traverses the consistent-hash ring clockwise from the virtual
     * node responsible for min_key to the one responsible for max_key
     * and collects every unique shard encountered.  When min and max
     * map to the same shard, a single-element vector is returned.
     * When no nodes are registered an empty vector is returned.
     *
     * @param collection  Collection name
     * @param min_key     Lower bound of the key range (inclusive)
     * @param max_key     Upper bound of the key range (inclusive)
     * @return Ordered list of unique shard identifiers
     */
    std::vector<std::string> GetShardsForKeyRange(
        const std::string& collection,
        const std::string& min_key,
        const std::string& max_key) const;

private:
    ShardingManager() = default;
    ~ShardingManager() = default;

    // Prevent copying
    ShardingManager(const ShardingManager&) = delete;
    ShardingManager& operator=(const ShardingManager&) = delete;

    mutable std::mutex mutex_;
    std::vector<ShardNodeInfo> shard_nodes_;
    // Consistent-hash ring kept in sync with shard_nodes_; used for
    // GetShardForKey / GetShardsForKeyRange.  Ring entries use
    // node_address as the shard identifier so callers can route
    // directly to the appropriate endpoint.
    ConsistentHashRing ring_;
};

} // namespace sharding
} // namespace themis

