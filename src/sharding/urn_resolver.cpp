/*
 * ThemisDB | File: urn_resolver.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:27:23
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 116
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=1, M=4, L=0
 * PR History (last 5): #3489 docs: Add comprehensive RAI... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "sharding/urn_resolver.h"
#include <spdlog/spdlog.h>

namespace themis::sharding {

URNResolver::URNResolver(
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<ConsistentHashRing> hash_ring,
    const std::string& local_shard_id
) : topology_(topology), 
    hash_ring_(hash_ring),
    local_shard_id_(local_shard_id) {
}

std::optional<ShardInfo> URNResolver::resolvePrimary(const URN& urn) const {
    // Get shard ID from consistent hash ring
    std::string shard_id = hash_ring_->getShardForURN(urn);
    
    if (shard_id.empty()) {
        return std::nullopt;
    }
    
    // Look up shard info from topology
    return topology_->getShard(shard_id);
}

std::vector<ShardInfo> URNResolver::resolveReplicas(const URN& urn, size_t replica_count) const {
    std::vector<ShardInfo> result;
    
    // Get primary shard
    auto primary = resolvePrimary(urn);
    if (!primary) {
        return result; // Empty result
    }
    
    result.push_back(*primary);
    
    // Get successor shards from the hash ring (for replication)
    uint64_t hash = urn.hash();
    std::vector<std::string> successor_ids = hash_ring_->getSuccessors(hash, replica_count + 1);
    
    // Skip the first one (it's the primary), add the rest
    for (size_t i = 1; i < successor_ids.size() && result.size() <= replica_count; ++i) {
        auto replica = topology_->getShard(successor_ids[i]);
        if (replica && replica->is_healthy) {
            result.push_back(*replica);
        }
    }
    
    return result;
}

bool URNResolver::isLocal(const URN& urn) const {
    if (local_shard_id_.empty()) {
        return false; // No local shard configured
    }
    
    std::string shard_id = hash_ring_->getShardForURN(urn);
    return shard_id == local_shard_id_;
}

std::string URNResolver::getShardId(const URN& urn) const {
    return hash_ring_->getShardForURN(urn);
}

std::string URNResolver::getShardForKey(
    const std::string& /*collection*/,
    const std::string& key
) const {
    auto node = hash_ring_->getNode(key);
    return node.value_or(std::string{});
}

std::vector<std::string> URNResolver::getShardsForKeyRange(
    const std::string& /*collection*/,
    const std::string& min_key,
    const std::string& max_key
) const {
    uint64_t h_min = hash_ring_->hashKey(min_key);
    uint64_t h_max = hash_ring_->hashKey(max_key);
    auto shards = hash_ring_->getShardsInRange(h_min, h_max);
    if (shards.empty()) {
        spdlog::warn("URNResolver::getShardsForKeyRange: ring returned no shards for "
                     "range [{}, {}] (h_min={:#x}, h_max={:#x}); falling back to all "
                     "healthy shards — check ring configuration",
                     min_key, max_key, h_min, h_max);
        auto all = getHealthyShards();
        for (const auto& s : all) {
            shards.push_back(s.shard_id);
        }
    }
    return shards;
}

std::vector<ShardInfo> URNResolver::getAllShards() const {
    return topology_->getAllShards();
}

std::vector<ShardInfo> URNResolver::getHealthyShards() const {
    return topology_->getHealthyShards();
}

void URNResolver::refreshTopology() {
    topology_->refresh();
}

} // namespace themis::sharding
