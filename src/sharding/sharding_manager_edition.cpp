/**
 * @file sharding_manager_edition.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * Sharding Manager with Edition-Specific Node Limits
 * ====================================================
 * Enforces maximum shard node count based on edition at runtime.
 */

#include <memory>
#include <algorithm>
#include <fmt/format.h>
#include "sharding/sharding_manager.h"
#include "themis/runtime_license_gate.h"

namespace themis {
namespace sharding {

void ShardingManager::AddShardNode(const ShardNodeInfo& node) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Edition constraint check
    if (static_cast<int>(shard_nodes_.size()) >= static_cast<size_t>(GetMaxShardNodes())) {
        throw std::runtime_error(fmt::format(
            "Cannot add shard node. Edition limit reached: {} nodes maximum ({} edition)",
            GetMaxShardNodes(), edition::EDITION_STRING));
    }

    shard_nodes_.push_back(node);
    ring_.addShard(node.node_address);
}

size_t ShardingManager::GetNodeCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(shard_nodes_.size());
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
        throw std::runtime_error(fmt::format(
            "Requested node count ({}) exceeds edition limit ({}). Edition: {}",
            requested_nodes, max_nodes, edition::EDITION_STRING));
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
        if (node.is_healthy) {
          count++;
        }
    }
    return count;
}

bool ShardingManager::RemoveShardNode(uint32_t node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = shard_nodes_.begin(); it != shard_nodes_.end(); ++it) {
        if (it->node_id == node_id) {
            ring_.removeShard(it->node_address);
            shard_nodes_.erase(it);
            return true;
        }
    }
    return false;
}

std::string ShardingManager::GetShardingCapabilityInfo() const {
    const auto info = edition::EditionInfo::Get();
    return fmt::format("Edition: {} | Max Shard Nodes: {} | Current Nodes: {}",
        info.name, info.sharding_max_nodes, GetNodeCount());
}

std::string ShardingManager::GetShardForKey(
    const std::string& collection,
    const std::string& key) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto result = ring_.getNode(collection + "/" + key);
    return result.value_or(std::string{});
}

std::vector<std::string> ShardingManager::GetShardsForKeyRange(
    const std::string& collection,
    const std::string& min_key,
    const std::string& max_key) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (shard_nodes_.empty()) {
        return {};
    }

    // Resolve the shard responsible for each bound.
    auto start_opt = ring_.getNode(collection + "/" + min_key);
    auto end_opt   = ring_.getNode(collection + "/" + max_key);

    if (!start_opt || !end_opt) {
        return {};
    }

    const std::string& start_shard = *start_opt;
    const std::string& end_shard   = *end_opt;

    // Fast path: both bounds land on the same shard.
    if (start_shard == end_shard) {
        return {start_shard};
    }

    // Walk the ring clockwise from start_shard until we reach end_shard,
    // collecting every unique shard we encounter.  getAllShards() returns
    // the list of unique shard identifiers; we build an ordered traversal
    // by iterating the sorted virtual-node ring.
    //
    // Strategy:
    //   1. Get all shards sorted by their *minimum* hash token (first
    //      virtual node position), forming a clockwise traversal order.
    //   2. Starting from start_shard, walk until end_shard is included.
    //
    // Note: this approximation is accurate for ranges that are small
    // relative to the keyspace.  For very large ranges the caller
    // should prefer a SCATTER_GATHER.

    std::vector<std::string> all_shards = ring_.getAllShards();
    if (all_shards.empty()) {
        return {};
    }

    // Find indices of start and end in the all_shards list.
    int start_idx = -1, end_idx = -1;
    for (size_t i = 0; i < all_shards.size(); ++i) {
        if (all_shards[i] == start_shard) {
          start_idx = i;
        }
        if (all_shards[i] == end_shard) {
          end_idx   = i;
        }
    }

    if (start_idx < 0 || end_idx < 0) {
        // Defensive fallback — return both boundary shards.
        return {start_shard, end_shard};
    }

    std::vector<std::string> result = {};

    const int n = static_cast<int>(all_shards.size());

    // Walk clockwise from start_idx to end_idx (inclusive), wrapping around.
    int idx = start_idx;
    for (int steps = 0; steps < n; ++steps) {
        result.push_back(all_shards[idx]);
        if (idx == end_idx) {
          break;
        }
        idx = (idx + 1) % n;
    }

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

