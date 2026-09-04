/**
 * @file blob_redundancy_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Blob-Level Redundancy Manager
 * 
 * Provides granular redundancy control at the binary blob level,
 * including RocksDB SST files, WAL segments, indexes, and large objects.
 * 
 * Features:
 * - Per-blob-type redundancy configuration
 * - Tiered storage with automatic transitions
 * - RocksDB EventListener integration
 * - Erasure coding for storage efficiency
 * - Geo-replication for disaster recovery
 * - Background scrubbing and repair
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <chrono>
#include <optional>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <condition_variable>

#include <rocksdb/listener.h>
#include "utils/expected.h"

// Forward declaration for RocksDB
namespace rocksdb {
    class DB;
    class EventListener;
    struct FlushJobInfo;
    struct CompactionJobInfo;
    struct TableFileDeletionInfo;
}

namespace themisdb {
namespace storage {

// Make themis::Result available in this namespace
using themis::Result;

/**
 * Blob Type Classification
 */
enum class BlobType {
    // RocksDB SST Files
    SST_L0,             // Level 0 (freshly flushed memtables)
    SST_L1,             // Level 1
    SST_L2_PLUS,        // Level 2 and above
    
    // Write-Ahead Logs
    WAL,                // WAL segments
    
    // RocksDB Metadata
    MANIFEST,           // Database manifest
    CURRENT,            // Current manifest pointer
    OPTIONS,            // Database options
    
    // Indexes
    INDEX_VECTOR,       // HNSW vector index
    INDEX_GRAPH,        // Graph adjacency index
    INDEX_FTS,          // Full-text search index
    INDEX_SPATIAL,      // R-tree spatial index
    
    // Binary Objects
    BLOB_SMALL,         // < 1MB
    BLOB_MEDIUM,        // 1MB - 100MB
    BLOB_LARGE,         // > 100MB
    
    // System
    METADATA,           // System metadata
    SCHEMA,             // Collection schemas
    
    CUSTOM              // Application-defined
};

/**
 * Storage Tier
 */
enum class StorageTier {
    HOT,                // Local SSD - fastest
    WARM,               // Local HDD or network SSD
    COLD,               // Object storage (S3-compatible)
    ARCHIVE             // Glacier-like - very slow retrieval
};

/**
 * Blob Priority
 */
enum class BlobPriority {
    CRITICAL,           // Must never lose, sync replication
    HIGH,               // Important, async replication OK
    NORMAL,             // Standard redundancy
    LOW,                // Can be regenerated
    EPHEMERAL           // No redundancy needed
};

/**
 * Redundancy Mode
 */
enum class RedundancyMode {
    NONE,               // No redundancy
    MIRROR,             // Full replication (RAID-1)
    STRIPE,             // Striping for throughput (RAID-0)
    STRIPE_MIRROR,      // Striping + mirroring (RAID-10)
    PARITY,             // Erasure coding (RAID-5/6)
    GEO_MIRROR          // Geo-distributed replication
};

/**
 * Erasure Coding Algorithm
 */
enum class ErasureCodingAlgorithm {
    REED_SOLOMON,       // Classic Reed-Solomon
    CAUCHY,             // Cauchy Reed-Solomon (faster)
    LRC                 // Local Reconstruction Codes (Azure-style)
};

/**
 * Checksum Algorithm
 */
enum class ChecksumAlgorithm {
    CRC32,              // Fast, good for integrity
    SHA256,             // Cryptographic, slower
    XXHASH64            // Very fast, good distribution
};

/**
 * Erasure Coding Configuration
 */
struct ErasureCodingConfig {
    uint32_t data_shards = 4;
    uint32_t parity_shards = 2;
    ErasureCodingAlgorithm algorithm = ErasureCodingAlgorithm::REED_SOLOMON;
    
    uint32_t totalShards() const { return data_shards + parity_shards; }
    double storageEfficiency() const { 
        return static_cast<double>(data_shards) / totalShards(); 
    }
};

/**
 * Geo-Replication Target
 */
struct GeoTarget {
    std::string datacenter;
    std::string endpoint;
    int32_t priority = 0;
    bool is_async = true;
    uint32_t max_lag_ms = 5000;
};

/**
 * Blob Redundancy Configuration
 */
struct BlobRedundancyConfig {
    // Basic redundancy
    RedundancyMode mode = RedundancyMode::MIRROR;
    uint32_t replication_factor = 2;
    
    // Erasure coding (for PARITY mode)
    ErasureCodingConfig erasure_coding;
    
    // Storage
    StorageTier tier = StorageTier::HOT;
    bool sync_write = false;
    
    // Priority
    BlobPriority priority = BlobPriority::NORMAL;
    
    // Geo-replication
    bool geo_replicate = false;
    bool geo_replicate_async = true;
    std::vector<GeoTarget> geo_targets;
    
    // Lifecycle
    bool auto_tier_down = false;
    uint32_t tier_down_after_days = 30;
    StorageTier tier_down_target = StorageTier::WARM;
    uint32_t archive_after_days = 0;  // 0 = never
    uint32_t retention_days = 0;      // 0 = forever
    
    // Recovery
    bool rebuild_on_loss = false;     // For indexes
    bool backup_on_change = false;
    uint32_t version_history = 0;     // Versions to keep
    
    // Striping
    bool stripe_enabled = false;
    uint32_t stripe_size_kb = 64;
    
    // Size limits
    uint32_t min_size_mb = 0;
    uint32_t max_size_mb = 0;  // 0 = no limit
    
    // Compression
    std::string compression = "NONE";  // NONE, LZ4, ZSTD
    int32_t compression_level = 0;
};

/**
 * Storage Tier Configuration
 */
struct TierConfig {
    StorageTier tier;
    std::string type;           // LOCAL_SSD, LOCAL_HDD, OBJECT_STORAGE, GLACIER
    std::string path;           // Local path or endpoint URL
    std::string region;         // For cloud storage
    uint64_t max_capacity_gb = 0;
    uint32_t min_free_space_percent = 10;
    
    // Default redundancy for this tier
    RedundancyMode default_mode = RedundancyMode::MIRROR;
    uint32_t default_replication_factor = 2;
    ErasureCodingConfig default_erasure_coding;
    
    // Performance settings
    bool direct_io = false;
    bool sync_writes = false;
    std::string compression = "NONE";
};

/**
 * Blob Location Information
 */
struct BlobLocation {
    std::string shard_id;
    std::string path;
    StorageTier tier;
    std::string checksum;
    uint64_t size_bytes = 0;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_verified;
    bool is_parity = false;
    uint32_t chunk_index = 0;
    bool is_healthy = true;
    std::string datacenter;
};

/**
 * Blob Metadata
 */
struct BlobMetadata {
    std::string blob_id;
    BlobType type;
    std::string collection;
    std::string document_id;    // Optional, for document-specific blobs
    
    // Locations
    std::vector<BlobLocation> locations;
    
    // Configuration
    BlobRedundancyConfig config;
    
    // Lifecycle
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_accessed;
    std::chrono::system_clock::time_point last_modified;
    std::chrono::system_clock::time_point scheduled_tier_down;
    
    // Stripe info (if striped)
    uint32_t total_chunks = 1;
    uint64_t total_size = 0;
    
    // Health check methods
    bool isHealthy() const;
    uint32_t healthyLocationCount() const;
    uint32_t requiredLocationCount() const;
    bool canRecover() const;
    std::vector<std::string> getMissingShards() const;
    
    // Serialization
    std::string toJson() const;
    static std::optional<BlobMetadata> fromJson(const std::string& json);
};

/**
 * Blob Redundancy Statistics
 */
struct BlobRedundancyStats {
    // Counts
    uint64_t total_blobs = 0;
    uint64_t healthy_blobs = 0;
    uint64_t degraded_blobs = 0;
    uint64_t critical_blobs = 0;
    
    // Storage
    uint64_t logical_bytes = 0;       // Actual data size
    uint64_t physical_bytes = 0;      // Storage used (with redundancy)
    uint64_t redundant_bytes = 0;     // Overhead from redundancy
    double storage_efficiency = 0.0;
    
    // By type
    std::map<BlobType, uint64_t> blobs_by_type;
    std::map<BlobType, uint64_t> bytes_by_type;
    
    // By tier
    std::map<StorageTier, uint64_t> blobs_by_tier;
    std::map<StorageTier, uint64_t> bytes_by_tier;
    
    // Operations
    uint64_t repair_operations = 0;
    uint64_t tier_transitions = 0;
    uint64_t scrub_operations = 0;
    uint64_t scrub_errors_found = 0;
    
    // Latency
    std::chrono::microseconds avg_write_latency{0};
    std::chrono::microseconds avg_read_latency{0};
    std::chrono::microseconds avg_repair_time{0};
};

/**
 * Blob Redundancy Manager
 * 
 * Central manager for blob-level redundancy, integrating with RocksDB
 * and providing fine-grained control over storage and replication.
 */
class BlobRedundancyManager {
public:
    /**
     * Configuration
     */
    struct Config {
        // Config file path (YAML)
        std::string config_path = "config/storage_redundancy.yaml";
        
        // Metadata store
        std::string metadata_store = "etcd";
        std::string metadata_endpoint = "http://localhost:2379";
        
        // Enable blob tracking
        bool enable_blob_tracking = true;
        
        // Checksum algorithm
        ChecksumAlgorithm checksum_algorithm = ChecksumAlgorithm::SHA256;
        
        // Background maintenance
        uint32_t maintenance_interval_seconds = 300;
        uint32_t repair_parallelism = 4;
        
        // Hot reload
        bool hot_reload_enabled = true;
        uint32_t hot_reload_check_seconds = 30;
    };
    
    using WriteHandler = std::function<bool(
        const std::string& shard_id,
        const std::string& path,
        const std::vector<uint8_t>& data
    )>;
    
    using ReadHandler = std::function<std::optional<std::vector<uint8_t>>(
        const std::string& shard_id,
        const std::string& path
    )>;
    
    using DeleteHandler = std::function<bool(
        const std::string& shard_id,
        const std::string& path
    )>;
    
    explicit BlobRedundancyManager(const Config& config);
    ~BlobRedundancyManager();
    
    // Lifecycle
    bool start();
    void stop();
    bool isRunning() const;
    
    // Configuration Management
    bool loadConfig(const std::string& path);
    bool reloadConfig();
    BlobRedundancyConfig getConfigForBlob(BlobType type, const std::string& collection = "");
    void setCollectionOverride(const std::string& collection, const BlobRedundancyConfig& config);
    void setDocumentOverride(const std::string& collection, const std::string& doc_id, 
                            const BlobRedundancyConfig& config);
    
    // Blob Registration (called when blobs are created)
    std::string registerBlob(
        BlobType type,
        const std::string& local_path,
        uint64_t size_bytes,
        const std::string& collection = "",
        const std::string& document_id = ""
    );
    
    // Blob Unregistration (called when blobs are deleted)
    void unregisterBlob(const std::string& blob_id);
    
    // Redundancy Operations
    Result<void> ensureRedundancy(const std::string& blob_id);
    Result<void> repairBlob(const std::string& blob_id);
    bool verifyBlob(const std::string& blob_id);
    
    // Read/Write with redundancy
    Result<void> writeBlob(
        const std::string& blob_id,
        const std::vector<uint8_t>& data,
        WriteHandler handler
    );
    
    Result<std::vector<uint8_t>> readBlob(
        const std::string& blob_id,
        ReadHandler handler
    );
    
    Result<void> deleteBlob(
        const std::string& blob_id,
        DeleteHandler handler
    );
    
    // Tier Management
    Result<void> tierDown(const std::string& blob_id, StorageTier target);
    Result<void> tierUp(const std::string& blob_id, StorageTier target);
    std::vector<std::string> getBlobsForTierDown() const;
    
    // Health and Monitoring
    BlobMetadata getBlobMetadata(const std::string& blob_id) const;
    std::vector<std::string> getDegradedBlobs() const;
    std::vector<std::string> getCriticalBlobs() const;
    BlobRedundancyStats getStats() const;
    
    // Maintenance
    void runMaintenanceCycle();
    void runScrub(bool full = false);
    void runRepairQueue();
    
    // Prometheus Metrics
    std::string exportPrometheusMetrics() const;
    
    // RocksDB Integration
    Result<std::shared_ptr<rocksdb::EventListener>> createRocksDBListener();
    
    // Called by RocksDBBlobListener when an SST file is deleted by RocksDB.
    // Marks all blob locations backed by the deleted file as unhealthy and
    // queues the affected blobs for re-replication.
    void notifySSTFileDeleted(const std::string& file_path);
    
private:
    Config config_;
    std::atomic<bool> running_{false};
    
    // Configuration
    std::map<BlobType, BlobRedundancyConfig> blob_type_configs_;
    std::map<StorageTier, TierConfig> tier_configs_;
    std::map<std::string, BlobRedundancyConfig> collection_overrides_;
    std::map<std::string, BlobRedundancyConfig> document_overrides_;
    mutable std::shared_mutex config_mutex_;
    
    // Blob tracking
    std::map<std::string, BlobMetadata> blobs_;
    mutable std::shared_mutex blobs_mutex_;
    
    // Repair queue
    std::queue<std::string> repair_queue_;
    std::mutex repair_mutex_;
    std::condition_variable repair_cv_;
    mutable std::mutex shutdown_mutex_;
    std::condition_variable shutdown_cv_;
    
    // Background threads
    std::thread maintenance_thread_;
    std::thread repair_thread_;
    std::thread config_reload_thread_;
    
    // Erasure coder
    // Statistics
    std::atomic<uint64_t> stats_total_blobs_{0};
    std::atomic<uint64_t> stats_repairs_{0};
    std::atomic<uint64_t> stats_tier_transitions_{0};
    
    // Internal methods
    void maintenanceLoop();
    void repairLoop();
    void configReloadLoop();
    
    std::string generateBlobId();
    std::string calculateChecksum(const std::vector<uint8_t>& data);
    BlobType classifyBlobType(const std::string& path, uint64_t size);
    
    bool replicateToShard(const std::string& shard_id, const BlobMetadata& blob, 
                          const std::vector<uint8_t>& data, WriteHandler handler);
    bool deleteFromShard(const std::string& shard_id, const std::string& path,
                        DeleteHandler handler);
    
    std::vector<std::string> selectTargetShards(const BlobMetadata& blob);
    std::string selectReadShard(const BlobMetadata& blob);
    
    void updateMetadataStore(const BlobMetadata& blob);
    void removeFromMetadataStore(const std::string& blob_id);
    void loadFromMetadataStore();
};

/**
 * RocksDB Event Listener for Blob Tracking
 * 
 * Integrates with RocksDB to automatically track SST file lifecycle
 */
class RocksDBBlobListener : public rocksdb::EventListener {
public:
    explicit RocksDBBlobListener(BlobRedundancyManager& manager, 
                                  const std::string& collection = "");
    
    // Memtable flush completed - new L0 SST file created
    void OnFlushCompleted(
        rocksdb::DB* db,
        const rocksdb::FlushJobInfo& info
    ) override;
    
    // Compaction completed - new SST files created
    void OnCompactionCompleted(
        rocksdb::DB* db,
        const rocksdb::CompactionJobInfo& info
    ) override;
    
    // SST file deleted
    void OnTableFileDeleted(
        const rocksdb::TableFileDeletionInfo& info
    ) override;
    
private:
    BlobRedundancyManager& manager_;
    std::string collection_;
    
    BlobType levelToBlobType(int level);
};

/**
 * Collection Redundancy Configuration
 * For per-collection YAML configuration
 */
struct CollectionRedundancyConfig {
    std::string collection = {};
    std::string description;
    
    // Collection-level defaults
    BlobRedundancyConfig defaults;
    
    // Field-level overrides
    std::map<std::string, BlobRedundancyConfig> field_overrides;
    
    // Blob-type overrides for this collection
    std::map<BlobType, BlobRedundancyConfig> blob_overrides;
    
    // Load from YAML
    static std::optional<CollectionRedundancyConfig> loadFromYaml(const std::string& path);
    
    // Save to YAML
    bool saveToYaml(const std::string& path) const;
};

} // namespace storage
} // namespace themisdb
