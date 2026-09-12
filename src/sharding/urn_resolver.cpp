/**
 * @file urn_resolver.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/urn_resolver.h"
#include <spdlog/spdlog.h>

namespace themis::sharding {

/**
 * @brief Construct resolver with topology, ring and optional local shard id.
 */
URNResolver::URNResolver(
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<ConsistentHashRing> hash_ring,
    const std::string& local_shard_id
) : topology_(topology), 
    hash_ring_(hash_ring),
    local_shard_id_(local_shard_id) {
}

/**
 * @brief Resolve the primary shard for a URN.
 * @param urn Parsed URN value.
 * @return Primary shard info or std::nullopt when mapping/topology lookup fails.
 */
std::optional<ShardInfo> URNResolver::resolvePrimary(const URN& urn) const {
    // Get shard ID from consistent hash ring
    std::string shard_id = hash_ring_->getShardForURN(urn);
    
    if (shard_id.empty()) {
        return std::nullopt;
    }
    
    // Look up shard info from topology
    return topology_->getShard(shard_id);
}

/**
 * @brief Resolve primary plus healthy replica shards for a URN.
 * @param urn Parsed URN value.
 * @param replica_count Requested replica count in addition to primary.
 * @return Vector with primary first followed by healthy successors.
 */
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

/**
 * @brief Check whether URN resolves to current local shard id.
 * @param urn Parsed URN value.
 * @return true when local shard id is configured and matches primary owner.
 */
bool URNResolver::isLocal(const URN& urn) const {
    if (local_shard_id_.empty()) {
        return false; // No local shard configured
    }
    
    std::string shard_id = hash_ring_->getShardForURN(urn);
    return shard_id == local_shard_id_;
}

/**
 * @brief Resolve shard identifier only.
 * @param urn Parsed URN value.
 * @return Shard id string or empty when no mapping exists.
 */
std::string URNResolver::getShardId(const URN& urn) const {
    return hash_ring_->getShardForURN(urn);
}

/**
 * @brief Resolve owning shard for an arbitrary partition key.
 * @param key Non-empty partition key.
 * @return Shard id or empty string for invalid/unknown mapping.
 */
std::string URNResolver::getShardForKey(
    const std::string& /*collection*/,
    const std::string& key
) const {
    // Fail-closed guard: reject empty key
    if (key.empty()) {
        spdlog::error("URNResolver::getShardForKey: key is empty");
        return std::string{};
    }
    auto node = hash_ring_->getNode(key);
    return node.value_or(std::string{});
}

/**
 * @brief Resolve candidate shards for a key range.
 * @param min_key Inclusive lower key bound.
 * @param max_key Inclusive upper key bound.
 * @return Deduplicated shard id list; falls back to healthy shards when ring range is empty.
 */
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

/** @brief Return all shards from topology cache. */
std::vector<ShardInfo> URNResolver::getAllShards() const {
    return topology_->getAllShards();
}

/** @brief Return healthy shard subset from topology cache. */
std::vector<ShardInfo> URNResolver::getHealthyShards() const {
    return topology_->getHealthyShards();
}

/** @brief Refresh topology state from backing metadata store. */
void URNResolver::refreshTopology() {
    topology_->refresh();
}

} // namespace themis::sharding
