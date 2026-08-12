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
 * @brief Resolve URNs and partition keys to physical shard placement.
 *
 * The resolver combines consistent hashing with topology lookups to map
 * logical identifiers to concrete shard endpoints for reads and writes.
 */
class URNResolver {
public:
    /**
     * @brief Construct resolver over topology and hash ring services.
     * @param topology Shard topology manager.
     * @param hash_ring Consistent hash ring for shard routing.
     * @param local_shard_id Optional local shard identifier for locality checks.
     */
    URNResolver(
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<ConsistentHashRing> hash_ring,
        const std::string& local_shard_id = ""
    );
    
    /**
        * @brief Resolve primary owner shard for a URN.
        * @param urn URN to resolve.
        * @return Primary shard info, or std::nullopt when ring/topology has no mapping.
     */
    std::optional<ShardInfo> resolvePrimary(const URN& urn) const;
    
    /**
        * @brief Resolve primary plus healthy replica targets.
        * @param urn URN to resolve.
        * @param replica_count Number of replica entries requested in addition to primary.
        * @return Vector with primary first, followed by healthy replicas up to request.
     */
    std::vector<ShardInfo> resolveReplicas(const URN& urn, size_t replica_count = 2) const;
    
    /**
        * @brief Check whether URN is owned by the configured local shard.
        * @param urn URN to check.
        * @return true when local_shard_id_ matches the current primary shard.
     */
    bool isLocal(const URN& urn) const;
    
    /**
        * @brief Resolve only the shard identifier for a URN.
        * @param urn URN to resolve.
        * @return Shard ID string, or empty string when no ring mapping exists.
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
    
    /** @brief Return all shards currently known in topology. */
    std::vector<ShardInfo> getAllShards() const;
    
    /** @brief Return currently healthy shards from topology state. */
    std::vector<ShardInfo> getHealthyShards() const;
    
    /**
     * @brief Refresh topology view from metadata backend.
     */
    void refreshTopology();
    
    /**
     * @brief Update local shard identifier used by isLocal.
     * @param shard_id This node's shard identifier.
     */
    void setLocalShardId(const std::string& shard_id) {
        local_shard_id_ = shard_id;
    }
    
    /**
     * @brief Return configured local shard identifier.
     * @return Local shard identifier string.
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
