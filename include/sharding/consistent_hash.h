/**
 * @file consistent_hash.h
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
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <optional>

namespace themis::sharding {

/**
 * Consistent Hashing Ring for even data distribution
 * 
 * Uses virtual nodes to ensure balanced distribution even with uneven
 * number of shards. Each shard gets multiple positions on the hash ring
 * (virtual nodes) to improve balance.
 * 
 * Benefits:
 * - Minimal data movement on shard add/remove (only 1/N of data moves)
 * - Even distribution with virtual nodes
 * - Fast lookup O(log N) where N is number of virtual nodes
 */
class ConsistentHashRing {
public:
    explicit ConsistentHashRing(size_t /*virtual_nodes_hint*/ = 150) {}

    /**
     * Add a shard to the ring with virtual nodes
     * @param shard_id Unique shard identifier (e.g., "shard_001")
     * @param virtual_nodes Number of virtual nodes (higher = better balance, default 150)
     */
    void addShard(const std::string& shard_id, size_t virtual_nodes = 150);

    // Backward-compat alias
    void addNode(const std::string& shard_id, size_t virtual_nodes = 150) {
        addShard(shard_id, virtual_nodes);
    }
    
    /**
     * Remove a shard from the ring
     * @param shard_id Shard identifier to remove
     */
    void removeShard(const std::string& shard_id);
    
    /**
     * Get shard for a given key hash
     * Uses clockwise search on the ring to find the first shard
     * @param hash 64-bit hash value
     * @return Shard ID, or empty string if ring is empty
     */
    std::string getShardForHash(uint64_t hash) const;
    
    /**
     * Get shard for a URN
     * Convenience method that hashes the URN and finds the shard
     * @param urn URN to route
     * @return Shard ID, or empty string if ring is empty
     */
    std::string getShardForURN(const URN& urn) const;

    /**
     * Get shard for an arbitrary key (wrapper returning optional)
     */
    std::optional<std::string> getNode(const std::string& key) const;

    /**
     * Get replica shards following the primary for a key
     * @param key partition key
     * @param count number of replicas to return
     */
    std::vector<std::string> getReplicaNodes(const std::string& key, size_t count) const;
    
    /**
     * Get N successor shards (for replication)
     * Returns the next N distinct shards clockwise on the ring
     * @param hash Starting hash position
     * @param count Number of successors to find
     * @return List of shard IDs (may be less than count if fewer shards exist)
     */
    std::vector<std::string> getSuccessors(uint64_t hash, size_t count) const;
    
    /**
     * Get hash range for a shard (min, max)
     * Returns the minimum and maximum hash values this shard is responsible for
     * Note: In consistent hashing, a shard may have multiple ranges (one per virtual node)
     * This returns the overall min/max across all virtual nodes
     * @param shard_id Shard identifier
     * @return Pair of (min_hash, max_hash), or (0, 0) if shard not found
     */
    std::pair<uint64_t, uint64_t> getShardRange(const std::string& shard_id) const;
    
    /**
     * Get all unique shards in the ring
     * @return List of unique shard IDs
     */
    std::vector<std::string> getAllShards() const;

    /**
     * Get all shards responsible for any key whose hash lies in [hash_start, hash_end].
     *
     * Performs a clockwise ring walk from hash_start to hash_end, collecting every
     * distinct shard ID encountered.  When hash_start > hash_end the range wraps
     * around the ring and all shards are returned.
     *
     * @param hash_start  Lower bound of the hash range (inclusive)
     * @param hash_end    Upper bound of the hash range (inclusive)
     * @return Ordered, deduplicated list of shard IDs (may be all shards)
     */
    std::vector<std::string> getShardsInRange(uint64_t hash_start, uint64_t hash_end) const;

    /**
     * Hash an arbitrary string key using the internal FNV-1a + mix function.
     * Exposed so callers can compute consistent hash positions without duplicating
     * the hash logic.
     * @param key  String to hash
     * @return 64-bit hash value
     */
    uint64_t hashKey(const std::string& key) const { return hash(key); }
    
    /**
     * Calculate balance factor (standard deviation of virtual nodes per shard)
     * Lower is better. < 5% is considered well-balanced
     * @return Balance factor as percentage (0.0 to 100.0)
     */
    double getBalanceFactor() const;
    
    /**
     * Get total number of virtual nodes in the ring
     * @return Total virtual node count
     */
    size_t getVirtualNodeCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return ring_.size();
    }
    
    /**
     * Get number of unique shards
     * @return Number of unique shards
     */
    size_t getShardCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return shard_tokens_.size();
    }
    
    /**
     * Clear all shards from the ring
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        ring_.clear();
        shard_tokens_.clear();
    }
    
private:
    // Token (hash) → Shard ID mapping
    // The ring is represented as a sorted map where keys are hash positions
    std::map<uint64_t, std::string> ring_;
    
    // Shard ID → Virtual Node Tokens
    // Tracks which hash positions belong to each shard
    std::map<std::string, std::vector<uint64_t>> shard_tokens_;
    
    // Mutex for thread-safe operations
    mutable std::mutex mutex_;
    
    /**
     * Hash function for virtual node generation
     * Combines shard_id and virtual_node_index to generate unique positions
     * @param key String to hash
     * @return 64-bit hash value
     */
    uint64_t hash(const std::string& key) const;
};

/** @brief Consistent hash component. */
class ConsistentHash {
public:
    explicit ConsistentHash(int /*expected_shards*/) {}

    void addNode(const std::string& shard_id, const std::string& endpoint, size_t virtual_nodes = 150) {
        endpoints_[shard_id] = endpoint;
        ring_.addShard(shard_id, virtual_nodes);
    }

    void removeNode(const std::string& shard_id) {
        endpoints_.erase(shard_id);
        ring_.removeShard(shard_id);
    }

    std::optional<ShardInfo> getNode(const std::string& key) const {
        if (ring_.getShardCount() == 0) {
            return std::nullopt;
        }
        std::string shard = ring_.getNode(key).value_or(std::string{});
        if (shard.empty()) {
            return std::nullopt;
        }
        auto it = endpoints_.find(shard);
        std::string endpoint = (it != endpoints_.end()) ? it->second : std::string();
        ShardInfo info{};
        info.shard_id = shard;
        info.primary_endpoint = endpoint;
        return info;
    }

    size_t getShardCount() const { return ring_.getShardCount(); }
    size_t getVirtualNodeCount() const { return ring_.getVirtualNodeCount(); }

private:
    ConsistentHashRing ring_;
    std::map<std::string, std::string> endpoints_;
};

} // namespace themis::sharding
