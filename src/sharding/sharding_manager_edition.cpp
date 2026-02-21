/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            sharding_manager_edition.cpp                       ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     226                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * Sharding Manager with Edition-Specific Node Limits
 * ====================================================
 * Enforces maximum shard node count based on edition at runtime.
 */

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <stdexcept>
#include "themis/edition.h"
#include "themis/runtime_license_gate.h"

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
    void AddShardNode(const ShardNodeInfo& node) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Edition constraint check
        if (shard_nodes_.size() >= static_cast<size_t>(GetMaxShardNodes())) {
            std::string error = "Cannot add shard node. Edition limit reached: ";
            error += std::to_string(GetMaxShardNodes());
            error += " nodes maximum (";
            error += std::string(edition::EDITION_STRING);
            error += " edition)";
            throw std::runtime_error(error);
        }

        shard_nodes_.push_back(node);
    }

    // Get number of currently configured shard nodes
    size_t GetNodeCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return shard_nodes_.size();
    }

    // Get remaining node capacity in this edition
    int GetRemainingNodeCapacity() const {
        std::lock_guard<std::mutex> lock(mutex_);
        int max_nodes = GetMaxShardNodes();
        int current_nodes = static_cast<int>(shard_nodes_.size());
        return std::max(0, max_nodes - current_nodes);
    }

    // Validate node configuration before adding
    void ValidateNodeCount(size_t requested_nodes) {
        int max_nodes = GetMaxShardNodes();
        if (static_cast<int>(requested_nodes) > max_nodes) {
            std::string error = "Requested node count (";
            error += std::to_string(requested_nodes);
            error += ") exceeds edition limit (";
            error += std::to_string(max_nodes);
            error += "). Edition: ";
            error += std::string(edition::EDITION_STRING);
            throw std::runtime_error(error);
        }
    }

    // Get all configured shard nodes
    std::vector<ShardNodeInfo> GetAllNodes() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return shard_nodes_;
    }

    // Check health status of all nodes
    int GetHealthyNodeCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        int count = 0;
        for (const auto& node : shard_nodes_) {
            if (node.is_healthy) count++;
        }
        return count;
    }

    // Remove a shard node from configuration
    bool RemoveShardNode(uint32_t node_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = shard_nodes_.begin(); it != shard_nodes_.end(); ++it) {
            if (it->node_id == node_id) {
                shard_nodes_.erase(it);
                return true;
            }
        }
        return false;
    }

    // Get edition-specific sharding capability information
    std::string GetShardingCapabilityInfo() const {
        const auto info = edition::EditionInfo::Get();
        std::string result = "Edition: ";
        result += std::string(info.name);
        result += " | Max Shard Nodes: ";
        result += std::to_string(info.sharding_max_nodes);
        result += " | Current Nodes: ";
        result += std::to_string(GetNodeCount());
        return result;
    }

private:
    ShardingManager() = default;
    ~ShardingManager() = default;

    // Prevent copying
    ShardingManager(const ShardingManager&) = delete;
    ShardingManager& operator=(const ShardingManager&) = delete;

    mutable std::mutex mutex_;
    std::vector<ShardNodeInfo> shard_nodes_;
};

// ============================================================================
// SHARDING UTILITY FUNCTIONS - EDITION-AWARE
// ============================================================================

// Check if multi-node replication is available (compile-time + runtime license gate)
inline bool CanUseMultiNodeReplication() {
    return license::RuntimeLicenseGate::instance().isFeatureAllowed("multi_master");
}

// Get node replication strategy based on edition
inline std::string GetReplicationStrategy() {
    const auto info = edition::EditionInfo::Get();
    
    if (info.type == edition::EditionType::COMMUNITY) {
        // Community: Single-node only
        // Suggest application-level replication workaround
        return "COMMUNITY_SINGLE_NODE: "
               "Single-node deployment only. For high availability, use external "
               "replication tools (e.g., streaming replication to standby, "
               "application-level primary-standby pattern, or upgrade to Enterprise).";
    } else if (info.type == edition::EditionType::ENTERPRISE) {
        return "ENTERPRISE_MULTI_MASTER: Multi-master replication across "
               "up to 100 nodes with automatic failover";
    } else if (info.type == edition::EditionType::HYPERSCALER) {
        return "HYPERSCALER_UNLIMITED: Unlimited node clustering with "
               "advanced load balancing and custom replication topologies";
    }
    return "UNKNOWN_EDITION";
}

// Suggest upgrade path if node limit is exceeded
inline std::string SuggestUpgrade(size_t requested_nodes) {
    const auto info = edition::EditionInfo::Get();
    std::string suggestion = "Your deployment requires ";
    suggestion += std::to_string(requested_nodes);
    suggestion += " nodes. Current edition (";
    suggestion += std::string(info.name);
    suggestion += ") supports ";
    suggestion += std::to_string(info.sharding_max_nodes);
    suggestion += " nodes. ";
    
    if (info.type == edition::EditionType::COMMUNITY) {
        suggestion += "Please upgrade to Enterprise Edition for multi-node deployments.";
    } else if (info.type == edition::EditionType::ENTERPRISE) {
        suggestion += "Please upgrade to Hyperscaler Edition for larger deployments.";
    }
    return suggestion;
}

} // namespace sharding
} // namespace themis
