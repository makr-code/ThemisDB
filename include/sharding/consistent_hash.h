/**
 * @file consistent_hash.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "sharding/urn.h"
#include "sharding/shard_topology.h"
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <mutex>
#include <optional>

namespace themis::sharding {

/** @brief Key identity used when validating an existing shard placement. */
enum class RoutingKeyType {
    PartitionKey,
    UrnUuid
};

/** @brief Versioned hash algorithms already used by the corresponding route. */
enum class RoutingHashAlgorithm {
    Fnv1aMix64V1,
    XxHash64Seed0V1
};

/**
 * @brief Typed input to the migration-safe routing token resolver.
 *
 * @note This descriptor does not change ordinary routing. `key_bytes` must
 *       use the exact bytes that were used by the existing placement, and
 *       `placement_epoch` must carry the nonzero epoch from the durable assignment.
 */
struct RoutingKeyDescriptor {
    RoutingKeyType key_type;
    RoutingHashAlgorithm algorithm;
    std::string_view key_bytes;
    uint64_t placement_epoch;
};

/** @brief Exclusive upper boundary for a normalized unsigned 64-bit ring interval. */
struct RingTokenBoundary {
    uint64_t value = 0;
    bool at_ring_end = false;  ///< Represents 2^64; never encoded by uint64_t overflow.
};

/** @brief One exact owner interval in normalized half-open form [lower, upper). */
struct RingOwnershipSegment {
    uint64_t lower_inclusive = 0;
    RingTokenBoundary upper_exclusive;
    std::string owner;
    bool full_ring = false;
};

/** @brief Immutable view of actual inserted ring entries and exact owner segments. */
class RingOwnershipSnapshot {
public:
    struct Entry {
        uint64_t token;
        std::string owner;
    };

    /** @brief Return the topology epoch supplied for this immutable capture. */
    [[nodiscard]] uint64_t topologyEpoch() const { return topology_epoch_; }

    /** @brief Return the canonical token-ordered actual ring entries. */
    [[nodiscard]] const std::vector<Entry>& entries() const { return entries_; }

    /** @brief Return exact normalized owner segments. */
    [[nodiscard]] const std::vector<RingOwnershipSegment>& segments() const {
        return segments_;
    }

    /** @brief Return SHA-256 of the epoch and canonical actual ring entries. */
    [[nodiscard]] const std::string& fingerprint() const { return fingerprint_; }

    /**
     * @brief Build normalized segment geometry from already captured ring entries.
     * @param topology_epoch Nonzero epoch associated with the caller-supplied entries.
     * @param entries Strictly token-ordered unique entries; empty entries represent an empty ring.
     * @return Snapshot, or std::nullopt for a zero epoch, empty owner, or invalid token ordering.
     * @note This pure builder does not establish authority; production captures
     *       must come from ConsistentHashRing::snapshotOwnership().
     */
    [[nodiscard]] static std::optional<RingOwnershipSnapshot> fromEntries(
        uint64_t topology_epoch,
        std::vector<Entry> entries
    );

    /**
     * @brief Check whether the snapshot assigns a token to the requested owner.
     * @param token Token to test; all 64-bit values, including 0 and UINT64_MAX, are valid.
     * @param owner Expected shard identifier.
     * @return True only when a captured exact segment assigns the token to owner.
     */
    [[nodiscard]] bool ownsToken(uint64_t token, std::string_view owner) const;

private:
    uint64_t topology_epoch_ = 0;
    std::vector<Entry> entries_;
    std::vector<RingOwnershipSegment> segments_;
    std::string fingerprint_;
};

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
     * @brief Resolve a migration token only when its typed, versioned contract is supported.
     * @param descriptor Exact key bytes, key type, existing algorithm version and nonzero epoch.
     * @return Stable token, or std::nullopt for empty/unsupported/inconsistent descriptors.
     * @note URN UUID tokens are available only in xxHash-enabled builds; the
     *       std::hash placement fallback is intentionally not migration-portable.
     */
    [[nodiscard]] std::optional<uint64_t> resolveRoutingToken(
        const RoutingKeyDescriptor& descriptor
    ) const;

    /**
     * @brief Validate typed-key ownership against one captured ring snapshot.
     * @param snapshot Immutable ownership snapshot.
     * @param descriptor Typed key with the same nonzero placement epoch.
     * @param shard_id Expected owner.
     * @return True only when the supported token resolves within an exact segment owned by shard_id.
     */
    [[nodiscard]] bool ownsKeyInSnapshot(
        const RingOwnershipSnapshot& snapshot,
        const RoutingKeyDescriptor& descriptor,
        std::string_view shard_id
    ) const;

    /**
     * @brief Capture actual inserted ring entries and exact owner segments.
     * @param topology_epoch Nonzero topology epoch supplied by the caller.
     * @return Snapshot, or std::nullopt when the epoch is zero or its SHA-256 fingerprint fails.
     * @note This does not establish authority or fence writers; callers must
     *       validate the epoch against their durable assignment.
     */
    [[nodiscard]] std::optional<RingOwnershipSnapshot> snapshotOwnership(
        uint64_t topology_epoch
    ) const;

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
