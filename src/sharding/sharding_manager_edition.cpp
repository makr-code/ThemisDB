/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            sharding_manager_edition.cpp                       ║
  Version:         0.0.35                                             ║
  Last Modified:   2026-03-16 04:19:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     163                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c9b143394  2026-03-15  feat(server): inject live ShardingManager into HttpServer... ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * Sharding Manager with Edition-Specific Node Limits
 * ====================================================
 * Enforces maximum shard node count based on edition at runtime.
 */

#include <memory>
#include <algorithm>
#include "sharding/sharding_manager.h"
#include "themis/runtime_license_gate.h"

namespace themis {
namespace sharding {

void ShardingManager::AddShardNode(const ShardNodeInfo& node) {
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

size_t ShardingManager::GetNodeCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return shard_nodes_.size();
}

int ShardingManager::GetRemainingNodeCapacity() const {
    std::lock_guard<std::mutex> lock(mutex_);
    int max_nodes = GetMaxShardNodes();
    int current_nodes = static_cast<int>(shard_nodes_.size());
    return std::max(0, max_nodes - current_nodes);
}

void ShardingManager::ValidateNodeCount(size_t requested_nodes) {
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

std::vector<ShardNodeInfo> ShardingManager::GetAllNodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return shard_nodes_;
}

int ShardingManager::GetHealthyNodeCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    int count = 0;
    for (const auto& node : shard_nodes_) {
        if (node.is_healthy) count++;
    }
    return count;
}

bool ShardingManager::RemoveShardNode(uint32_t node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = shard_nodes_.begin(); it != shard_nodes_.end(); ++it) {
        if (it->node_id == node_id) {
            shard_nodes_.erase(it);
            return true;
        }
    }
    return false;
}

std::string ShardingManager::GetShardingCapabilityInfo() const {
    const auto info = edition::EditionInfo::Get();
    std::string result = "Edition: ";
    result += std::string(info.name);
    result += " | Max Shard Nodes: ";
    result += std::to_string(info.sharding_max_nodes);
    result += " | Current Nodes: ";
    result += std::to_string(GetNodeCount());
    return result;
}

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
