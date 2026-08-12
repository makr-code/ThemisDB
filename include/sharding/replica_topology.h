/**
 * @file replica_topology.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

/** @brief Replica-set definition for one shard primary and its replicas. */
struct ShardReplicaSet {
    /** @brief Logical shard identifier. */
    std::string shard_id;
    /** @brief Primary node identifier for this shard. */
    std::string primary_id;
    /** @brief Replica node identifiers (excluding primary). */
    std::vector<std::string> replicas;  // replica_ids (excludes primary)
    /** @brief Redundancy mode currently applied to this shard set. */
    RedundancyMode redundancy = RedundancyMode::MIRROR;
    /** @brief Stripe-group identifier (used by STRIPE_MIRROR layouts). */
    uint64_t stripe_key = 0;            // For STRIPE_MIRROR: stripe group identifier
    /** @brief Aggregate health flag for this shard set. */
    bool is_healthy = true;

    // Geo placement metadata (used for GEO_MIRROR and Raft placement)
    std::string region;   // e.g. "us-east", "eu-west"
    std::string zone;     // e.g. "us-east-1a", "eu-west-1b"
    
    /** @brief Return majority quorum size over primary + replicas. */
    size_t quorum_size() const {
        // Quorum = majority of all members (primary + replicas)
        return (1 + replicas.size()) / 2 + 1;
    }
    
    /** @brief Return true when replica_id is primary or one of replica members. */
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
    /** @brief Construct empty replica-topology manager. */
    ReplicaTopology() = default;
    
    /** @brief Insert or replace replica-set mapping for one shard id. */
    void defineReplicaSet(const ShardReplicaSet& replica_set) {
        std::lock_guard<std::mutex> lock(mutex_);
        replica_sets_[replica_set.shard_id] = replica_set;
    }
    
    /** @brief Return replica-set snapshot for shard id, or nullptr if unknown. */
    std::shared_ptr<const ShardReplicaSet> getReplicaSet(const std::string& shard_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = replica_sets_.find(shard_id);
        if (it != replica_sets_.end()) {
            return std::make_shared<const ShardReplicaSet>(it->second);
        }
        return nullptr;
    }
    
    /** @brief Return all shard ids containing the provided replica id. */
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
    
    /** @brief Return all known shard identifiers. */
    std::vector<std::string> getAllShards() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> shards;
        for (const auto& [shard_id, _] : replica_sets_) {
            shards.push_back(shard_id);
        }
        return shards;
    }
    
    /**
     * @brief Update health state for replica membership.
     * @note Current implementation marks shard health only when primary status changes.
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
    
    /** @brief Return number of shard mappings currently tracked. */
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
     * @brief Load replica topology mappings from JSON array configuration.
     * @param config JSON array of objects containing shard_id/primary_id/replicas fields.
     * @return true when at least one valid replica set was loaded.
     */
    bool loadFromJson(const nlohmann::json& config);
    
private:
    mutable std::mutex mutex_;
    std::map<std::string, ShardReplicaSet> replica_sets_;
};

} // namespace themis::sharding
