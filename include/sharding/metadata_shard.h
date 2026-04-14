/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            metadata_shard.h                                   ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:26:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     375                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#ifndef THEMISDB_SHARDING_METADATA_SHARD_H
#define THEMISDB_SHARDING_METADATA_SHARD_H

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

/**
 * @brief Metadata partition key types
 */
enum class MetadataPartitionKey {
    SCHEMA,          // Schema definitions
    INDEX,           // Index metadata
    SHARD_MAP,       // Shard mapping information
    TRANSACTION_LOG, // Transaction log entries
    STATISTICS,      // Statistics and metrics
    CONFIGURATION    // Configuration data
};

/**
 * @brief Metadata entry
 */
struct MetadataEntry {
    std::string key;                           // Metadata key
    nlohmann::json value;                      // Metadata value
    uint64_t version;                          // Version number
    MetadataPartitionKey partition;            // Partition this belongs to
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    
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

/**
 * @brief Metadata shard configuration
 */
struct MetadataShardConfig {
    std::string shard_id;                      // This shard's ID
    std::vector<MetadataPartitionKey> partitions;  // Partitions managed by this shard
    size_t num_metadata_shards = 3;            // Total number of metadata shards
    
    // Cache settings
    bool enable_cache = true;
    size_t cache_size = 10000;
    std::chrono::seconds cache_ttl{300};
    
    // Replication settings
    uint32_t replication_factor = 3;
    
    // Consistency settings
    bool enforce_strong_consistency = true;
    
    // Phase 2.2: Persistence settings
    bool enable_persistence = false;           // Enable WAL and snapshots
    std::string data_dir;                      // Data directory for WAL and snapshots
    uint64_t snapshot_interval = 10000;        // Snapshot every N operations
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
    explicit MetadataShard(
        const MetadataShardConfig& config,
        std::shared_ptr<ConsensusModule> consensus
    );
    
    ~MetadataShard();
    
    /**
     * @brief Initialize the metadata shard
     */
    bool initialize();
    
    /**
     * @brief Start the metadata shard
     */
    bool start();
    
    /**
     * @brief Stop the metadata shard
     */
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
    
    /**
     * @brief Get partition statistics
     */
    nlohmann::json getPartitionStats(MetadataPartitionKey partition) const;
    
    /**
     * @brief Get all statistics
     */
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
    
    /**
     * @brief Phase 2.2: Create periodic snapshot
     * @return true if successful
     */
    bool createPeriodicSnapshot();
    
    /**
     * @brief Phase 2.2: Recover from WAL
     * @return true if successful
     */
    bool recoverFromWAL();

private:
    /**
     * @brief Determine which shard owns a key
     */
    std::string determineShardOwner(
        MetadataPartitionKey partition,
        const std::string& key
    ) const;
    
    /**
     * @brief Cache management
     */
    void cacheEntry(const MetadataEntry& entry);
    std::optional<MetadataEntry> getCachedEntry(
        MetadataPartitionKey partition,
        const std::string& key
    ) const;
    void invalidateCache(MetadataPartitionKey partition, const std::string& key);
    
    /**
     * @brief Apply metadata change via consensus
     */
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
    explicit MetadataShardRouter(size_t num_shards);
    
    /**
     * @brief Add a metadata shard
     */
    void addShard(const std::string& shard_id, std::shared_ptr<MetadataShard> shard);
    
    /**
     * @brief Remove a metadata shard
     */
    void removeShard(const std::string& shard_id);
    
    /**
     * @brief Get metadata entry
     */
    std::optional<MetadataEntry> get(
        MetadataPartitionKey partition,
        const std::string& key
    ) const;
    
    /**
     * @brief Put metadata entry
     */
    bool put(
        MetadataPartitionKey partition,
        const std::string& key,
        const nlohmann::json& value
    );
    
    /**
     * @brief Delete metadata entry
     */
    bool remove(
        MetadataPartitionKey partition,
        const std::string& key
    );
    
    /**
     * @brief List all keys in a partition (scatter-gather)
     */
    std::vector<std::string> listKeys(MetadataPartitionKey partition) const;
    
    /**
     * @brief Get routing statistics
     */
    nlohmann::json getStatistics() const;
    
private:
    /**
     * @brief Determine which shard to route to
     */
    std::string routeToShard(
        MetadataPartitionKey partition,
        const std::string& key
    ) const;
    
    /**
     * @brief Hash function for consistent hashing
     */
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

#endif // THEMISDB_SHARDING_METADATA_SHARD_H
