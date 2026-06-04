/**
 * @file urn_resolver.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: urn_resolver.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "sharding/urn.h"
#include "sharding/shard_topology.h"
#include "sharding/consistent_hash.h"
#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <memory>

namespace themis::sharding {

/**
 * URN Resolver - Maps URNs to Shard Locations
 * 
 * The URN Resolver is responsible for:
 * 1. Parsing URNs into structured format
 * 2. Using consistent hashing to determine which shard owns the data
 * 3. Resolving shard IDs to network endpoints
 * 4. Finding replica shards for read scaling
 * 
 * This provides location transparency - clients don't need to know
 * which physical shard holds their data.
 */
class URNResolver {
public:
    /**
     * Initialize resolver with shard topology and hash ring
     * @param topology Shard topology manager
     * @param hash_ring Consistent hash ring for shard routing
     * @param local_shard_id Optional: this node's shard ID (for locality checks)
     */
    URNResolver(
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<ConsistentHashRing> hash_ring,
        const std::string& local_shard_id = ""
    );
    
    /**
     * Resolve URN to Shard Info (Primary)
     * @param urn URN to resolve
     * @return ShardInfo for primary shard, or nullopt if not found
     */
    std::optional<ShardInfo> resolvePrimary(const URN& urn) const;
    
    /**
     * Resolve URN to all Replicas (for read scaling)
     * Returns the primary shard plus replica shards
     * @param urn URN to resolve
     * @param replica_count Number of replicas to return (default 2)
     * @return Vector of ShardInfo (primary + replicas)
     */
    std::vector<ShardInfo> resolveReplicas(const URN& urn, size_t replica_count = 2) const;
    
    /**
     * Check if URN is local to this node
     * @param urn URN to check
     * @return true if this node is the primary for this URN
     */
    bool isLocal(const URN& urn) const;
    
    /**
     * Get Shard ID for URN (without full ShardInfo)
     * Faster than resolvePrimary if you only need the shard ID
     * @param urn URN to resolve
     * @return Shard ID string, or empty if ring is empty
     */
    std::string getShardId(const URN& urn) const;

    /**
     * Get the single shard responsible for an arbitrary partition key.
     *
     * The key is hashed with the same FNV-1a + mix64 function used by the
     * ring, then looked up with a clockwise walk.
     *
     * @param collection  Collection name (for logging / future topology hints)
     * @param key         Partition key value (e.g. document _key; non-empty required)
     * @return Shard ID, or empty string if the ring is empty or key is empty
     * @note Rejects empty keys fail-closed, returns empty string without processing
     */
    std::string getShardForKey(const std::string& collection, const std::string& key) const;

    /**
     * Get all shards that could own keys in the range [min_key, max_key].
     *
     * Hashes both endpoints and collects every shard whose virtual-node
     * token falls within that hash range (clockwise walk).  For wrap-around
     * ranges every shard is returned.
     *
     * @param collection  Collection name
     * @param min_key     Inclusive lower bound of the key range
     * @param max_key     Inclusive upper bound of the key range
     * @return Deduplicated list of shard IDs; never empty if the ring is not empty
     */
    std::vector<std::string> getShardsForKeyRange(
        const std::string& collection,
        const std::string& min_key,
        const std::string& max_key
    ) const;
    
    /**
     * Get all Shards in cluster
     * @return Vector of all shard information
     */
    std::vector<ShardInfo> getAllShards() const;
    
    /**
     * Get all healthy shards
     * @return Vector of healthy shard information
     */
    std::vector<ShardInfo> getHealthyShards() const;
    
    /**
     * Reload topology from metadata store (etcd)
     * Call this periodically or when topology changes are detected
     */
    void refreshTopology();
    
    /**
     * Set local shard ID
     * @param shard_id This node's shard identifier
     */
    void setLocalShardId(const std::string& shard_id) {
        local_shard_id_ = shard_id;
    }
    
    /**
     * Get local shard ID
     * @return This node's shard identifier
     */
    const std::string& getLocalShardId() const {
        return local_shard_id_;
    }
    
private:
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<ConsistentHashRing> hash_ring_;
    std::string local_shard_id_;
};

} // namespace themis::sharding
