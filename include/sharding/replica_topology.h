/*
 * ThemisDB | File: replica_topology.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <cstdint>
#include <unordered_set>
#include <nlohmann/json.hpp>

#include "sharding/redundancy_strategy.h"

namespace themis::sharding {

/**
 * Replica Topology Mapping
 *
 * Maps shard IDs to replica sets for mirror and stripe-mirror replication.
 */

/**
 * Shard replica set (e.g., [primary, replica1, replica2])
 */
struct ShardReplicaSet {
    std::string shard_id;
    std::string primary_id;
    std::vector<std::string> replicas;  // replica_ids (excludes primary)
    RedundancyMode redundancy = RedundancyMode::MIRROR;
    uint64_t stripe_key = 0;            // For STRIPE_MIRROR: stripe group identifier
    bool is_healthy = true;

    // Geo placement metadata (used for GEO_MIRROR and Raft placement)
    std::string region;   // e.g. "us-east", "eu-west"
    std::string zone;     // e.g. "us-east-1a", "eu-west-1b"
    
    size_t quorum_size() const {
        // Quorum = majority of all members (primary + replicas)
        return (1 + replicas.size()) / 2 + 1;
    }
    
    bool contains_replica(const std::string& replica_id) const {
        return replica_id == primary_id || 
               std::find(replicas.begin(), replicas.end(), replica_id) != replicas.end();
    }
};

/**
 * Replica Topology Manager
 * 
 * Manages shard-to-replica mappings for distributed writes.
 */
class ReplicaTopology {
public:
    ReplicaTopology() = default;
    
    /**
     * Define a shard replica set
     */
    void defineReplicaSet(const ShardReplicaSet& replica_set) {
        std::lock_guard<std::mutex> lock(mutex_);
        replica_sets_[replica_set.shard_id] = replica_set;
    }
    
    /**
     * Get replica set for a shard
     */
    std::shared_ptr<const ShardReplicaSet> getReplicaSet(const std::string& shard_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = replica_sets_.find(shard_id);
        if (it != replica_sets_.end()) {
            return std::make_shared<const ShardReplicaSet>(it->second);
        }
        return nullptr;
    }
    
    /**
     * Find shard by replica ID
     */
    std::vector<std::string> findShardsByReplica(const std::string& replica_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> shards;
        for (const auto& [shard_id, replica_set] : replica_sets_) {
            if (replica_set.contains_replica(replica_id)) {
                shards.push_back(shard_id);
            }
        }
        return shards;
    }
    
    /**
     * Get all shard IDs
     */
    std::vector<std::string> getAllShards() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> shards;
        for (const auto& [shard_id, _] : replica_sets_) {
            shards.push_back(shard_id);
        }
        return shards;
    }
    
    /**
     * Update replica health status
     */
    void setReplicaHealth(const std::string& shard_id, const std::string& replica_id, bool is_healthy) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = replica_sets_.find(shard_id);
        if (it != replica_sets_.end()) {
            // Mark entire shard unhealthy if primary is down
            if (replica_id == it->second.primary_id) {
                it->second.is_healthy = is_healthy;
            }
        }
    }
    
    /**
     * Get number of shards
     */
    size_t getShardCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return replica_sets_.size();
    }

    /**
     * Get all replica sets whose primary is in a specific region
     * @param region Region name (e.g. "us-east")
     * @return Vector of matching ShardReplicaSet values
     */
    std::vector<ShardReplicaSet> getReplicaSetsInRegion(const std::string& region) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<ShardReplicaSet> result;
        for (const auto& [shard_id, rs] : replica_sets_) {
            if (rs.region == region) {
                result.push_back(rs);
            }
        }
        return result;
    }

    /**
     * Get all distinct regions present in the replica topology
     * @return Sorted list of unique region names
     */
    std::vector<std::string> getRegions() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::unordered_set<std::string> seen;
        for (const auto& [shard_id, rs] : replica_sets_) {
            if (!rs.region.empty()) {
                seen.insert(rs.region);
            }
        }
        std::vector<std::string> regions(seen.begin(), seen.end());
        std::sort(regions.begin(), regions.end());
        return regions;
    }
    
    /**
     * Load replica topology from JSON config (example)
     */
    bool loadFromJson(const nlohmann::json& config);
    
private:
    mutable std::mutex mutex_;
    std::map<std::string, ShardReplicaSet> replica_sets_;
};

} // namespace themis::sharding
