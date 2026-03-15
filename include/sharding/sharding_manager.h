#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <mutex>
#include <stdexcept>
#include "themis/edition.h"

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

private:
    ShardingManager() = default;
    ~ShardingManager() = default;

    // Prevent copying
    ShardingManager(const ShardingManager&) = delete;
    ShardingManager& operator=(const ShardingManager&) = delete;

    mutable std::mutex mutex_;
    std::vector<ShardNodeInfo> shard_nodes_;
};

} // namespace sharding
} // namespace themis
