/**
 * @file metadata_shard.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "sharding/consensus_module.h"
#include "sharding/wal_manager.h"
#include "cache/bounded_lru_cache.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <atomic>
#include <nlohmann/json.hpp>

// Forward declarations for Phase 2.2
namespace themisdb::sharding {
    class MetadataWAL;
    class MetadataSnapshotManager;
}

namespace themisdb {
namespace sharding {

using LSN = themis::sharding::LSN;

/** @brief Logical metadata partitions distributed across metadata shards. */
enum class MetadataPartitionKey {
    SCHEMA,          // Schema definitions
    INDEX,           // Index metadata
    SHARD_MAP,       // Shard mapping information
    TRANSACTION_LOG, // Transaction log entries
    STATISTICS,      // Statistics and metrics
    CONFIGURATION    // Configuration data
};

/** @brief Versioned metadata key/value record stored in a partition. */
struct MetadataEntry {
    /** @brief Logical key inside partition. */
    std::string key;                           // Metadata key
    /** @brief JSON payload for metadata value. */
    nlohmann::json value;                      // Metadata value
    /** @brief Monotonic version for conflict resolution. */
    uint64_t version;                          // Version number
    /** @brief Owning metadata partition. */
    MetadataPartitionKey partition;            // Partition this belongs to
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    
    /** @brief Serialize entry to JSON representation for cache/persistence paths. */
    nlohmann::json toJson() const {
        return {
            {"key", key},
            {"value", value},
            {"version", version},
            {"partition", static_cast<int>(partition)},
            {"created_at", std::chrono::duration_cast<std::chrono::milliseconds>(
                created_at.time_since_epoch()).count()},
            {"updated_at", std::chrono::duration_cast<std::chrono::milliseconds>(
                updated_at.time_since_epoch()).count()}
        };
    }
    
    /** @brief Deserialize metadata entry from JSON representation. */
    static MetadataEntry fromJson(const nlohmann::json& j) {
        MetadataEntry entry;
        entry.key = j["key"];
        entry.value = j["value"];
        entry.version = j["version"];
        entry.partition = static_cast<MetadataPartitionKey>(j["partition"]);
        entry.created_at = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(j["created_at"]));
        entry.updated_at = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(j["updated_at"]));
        return entry;
    }
};

/** @brief Runtime configuration for one metadata shard instance. */
struct MetadataShardConfig {
    /** @brief Shard identifier of this metadata shard instance. */
    std::string shard_id;                      // This shard's ID
    /** @brief Partitions managed locally by this shard. */
    std::vector<MetadataPartitionKey> partitions;  // Partitions managed by this shard
    /** @brief Total shard count used for ownership/routing hash decisions. */
    size_t num_metadata_shards = 3;            // Total number of metadata shards
    
    // Cache settings
    /** @brief Enable bounded LRU cache for metadata reads. */
    bool enable_cache = true;
    /** @brief Maximum cache entry count. */
    size_t cache_size = 10000;
    /** @brief Cache entry time-to-live duration. */
    std::chrono::seconds cache_ttl{300};
    
    // Replication settings
    /** @brief Desired replication factor for metadata changes. */
    uint32_t replication_factor = 3;
    
    // Consistency settings
    /** @brief Route write operations through consensus before local commit. */
    bool enforce_strong_consistency = true;
    
    // Phase 2.2: Persistence settings
    /** @brief Enable WAL + snapshot persistence for crash recovery. */
    bool enable_persistence = false;           // Enable WAL and snapshots
    /** @brief Base directory for WAL/snapshot files. */
    std::string data_dir;                      // Data directory for WAL and snapshots
    /** @brief Snapshot trigger interval in number of metadata operations. */
    uint64_t snapshot_interval = 10000;        // Snapshot every N operations
    /** @brief Maximum retained snapshot files. */
    size_t max_snapshots = 10;                 // Keep last N snapshots
};

/**
 * @brief Metadata Shard
 * 
 * Manages a horizontally partitioned subset of cluster metadata.
 * Provides:
 * - Distributed metadata storage
 * - Versioning and conflict resolution
 * - Caching for performance
 * - Consistency via consensus module
 */
class MetadataShard {
public:
    /** @brief Construct metadata shard with routing/config and optional consensus module. */
    explicit MetadataShard(
        const MetadataShardConfig& config,
        std::shared_ptr<ConsensusModule> consensus
    );
    
    /** @brief Destructor stops shard and releases cache/persistence resources. */
    ~MetadataShard();
    
    /** @brief Initialize partitions, persistence backends, and recovery state. */
    bool initialize();
    
    /** @brief Mark shard as running and ready to serve operations. */
    bool start();
    
    /** @brief Stop shard and clear volatile cache state. */
    void stop();
    
    /**
     * @brief Get metadata entry
     * @param partition Partition to read from
     * @param key Metadata key
     * @return Metadata entry or nullopt if not found
     */
    std::optional<MetadataEntry> get(
        MetadataPartitionKey partition,
        const std::string& key
    ) const;
    
    /**
     * @brief Put metadata entry
     * @param partition Partition to write to
     * @param key Metadata key
     * @param value Metadata value
     * @return true if successful
     */
    bool put(
        MetadataPartitionKey partition,
        const std::string& key,
        const nlohmann::json& value
    );
    
    /**
     * @brief Delete metadata entry
     * @param partition Partition to delete from
     * @param key Metadata key
     * @return true if successful
     */
    bool remove(
        MetadataPartitionKey partition,
        const std::string& key
    );
    
    /**
     * @brief List all keys in a partition
     * @param partition Partition to list
     * @return Vector of keys
     */
    std::vector<std::string> listKeys(MetadataPartitionKey partition) const;
    
    /** @brief Return statistics for one partition. */
    nlohmann::json getPartitionStats(MetadataPartitionKey partition) const;
    
    /** @brief Return shard-wide statistics including cache and partitions. */
    nlohmann::json getStatistics() const;
    
    /**
     * @brief Subscribe to metadata changes
     * @param partition Partition to watch
     * @param callback Callback for changes
     */
    void subscribe(
        MetadataPartitionKey partition,
        std::function<void(const MetadataEntry&)> callback
    );
    
    /** @brief Create snapshot from current storage when persistence is enabled. */
    bool createPeriodicSnapshot();
    
    /** @brief Recover shard state from latest snapshot plus WAL replay. */
    bool recoverFromWAL();

private:
    /** @brief Determine owning shard id for given partition/key pair. */
    std::string determineShardOwner(
        MetadataPartitionKey partition,
        const std::string& key
    ) const;
    
    /** @brief Insert/update one entry in cache when cache is enabled. */
    void cacheEntry(const MetadataEntry& entry);
    /** @brief Lookup entry in cache by partition/key. */
    std::optional<MetadataEntry> getCachedEntry(
        MetadataPartitionKey partition,
        const std::string& key
    ) const;
    /** @brief Invalidate one cache key after mutation/removal. */
    void invalidateCache(MetadataPartitionKey partition, const std::string& key);
    
    /** @brief Propose and wait for consensus commit of metadata mutation. */
    bool applyChange(
        const std::string& operation,
        MetadataPartitionKey partition,
        const std::string& key,
        const nlohmann::json& value
    );
    
    MetadataShardConfig config_;
    std::shared_ptr<ConsensusModule> consensus_;
    
    // Storage
    mutable std::mutex storage_mutex_;
    std::map<MetadataPartitionKey, std::map<std::string, MetadataEntry>> storage_;
    
    // Cache - replaced with BoundedLRUCache
    std::unique_ptr<themis::cache::BoundedLRUCache> cache_;
    
    // Subscriptions
    mutable std::mutex subscriptions_mutex_;
    std::map<MetadataPartitionKey, 
             std::vector<std::function<void(const MetadataEntry&)>>> subscriptions_;
    
    // State
    std::atomic<bool> running_;
    
    // Statistics
    mutable std::atomic<uint64_t> total_reads_;
    mutable std::atomic<uint64_t> total_writes_;
    mutable std::atomic<uint64_t> cache_hits_;
    mutable std::atomic<uint64_t> cache_misses_;
    
    // Phase 2.2: Persistence
    std::unique_ptr<MetadataWAL> wal_;
    std::unique_ptr<MetadataSnapshotManager> snapshot_manager_;
    std::atomic<uint64_t> operations_since_snapshot_;
    LSN last_applied_lsn_;
};

/**
 * @brief Metadata Shard Router
 * 
 * Routes metadata operations to the appropriate metadata shard
 */
class MetadataShardRouter {
public:
    /** @brief Construct router with configured shard-count hashing domain. */
    explicit MetadataShardRouter(size_t num_shards);
    
    /** @brief Register shard instance under shard id. */
    void addShard(const std::string& shard_id, std::shared_ptr<MetadataShard> shard);
    
    /** @brief Unregister shard instance by shard id. */
    void removeShard(const std::string& shard_id);
    
    /** @brief Route and read metadata entry from owning shard. */
    std::optional<MetadataEntry> get(
        MetadataPartitionKey partition,
        const std::string& key
    ) const;
    
    /** @brief Route and write metadata entry to owning shard. */
    bool put(
        MetadataPartitionKey partition,
        const std::string& key,
        const nlohmann::json& value
    );
    
    /** @brief Route and delete metadata entry on owning shard. */
    bool remove(
        MetadataPartitionKey partition,
        const std::string& key
    );
    
    /**
     * @brief List all keys in a partition (scatter-gather)
     */
    std::vector<std::string> listKeys(MetadataPartitionKey partition) const;
    
    /** @brief Return router operation/error and shard-level statistics snapshot. */
    nlohmann::json getStatistics() const;
    
private:
    /** @brief Resolve target shard id for partition/key request. */
    std::string routeToShard(
        MetadataPartitionKey partition,
        const std::string& key
    ) const;
    
    /** @brief Hash key into shard-space index domain. */
    size_t hashKey(const std::string& key) const;
    
    size_t num_shards_;
    mutable std::mutex shards_mutex_;
    std::map<std::string, std::shared_ptr<MetadataShard>> shards_;
    
    // Statistics
    mutable std::atomic<uint64_t> total_operations_;
    mutable std::atomic<uint64_t> routing_errors_;
};

} // namespace sharding
} // namespace themisdb

