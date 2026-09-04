/**
 * @file metadata_shard.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=6, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/metadata_shard.h"
#include "sharding/metadata_wal.h"
#include "sharding/metadata_snapshot.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace themisdb {
namespace sharding {

/** @brief Build stable cache key as "partition:key". */
static std::string makeCacheKey(MetadataPartitionKey partition, const std::string& key) {
    return std::to_string(static_cast<int>(partition)) + ":" + key;
}

/** @brief Construct metadata shard and optional cache/persistence backends. */
MetadataShard::MetadataShard(
    const MetadataShardConfig& config,
    std::shared_ptr<ConsensusModule> consensus
)
    : config_(config)
    , consensus_(consensus)
    , running_(false)
    , total_reads_(0)
    , total_writes_(0)
    , cache_hits_(0)
    , cache_misses_(0)
    , operations_since_snapshot_(0)
    , last_applied_lsn_(LSN(0, 0)) {
    
    // Initialize bounded cache
    if (config_.enable_cache) {
        themis::cache::BoundedLRUCache::Config cache_config;
        cache_config.max_entries = config_.cache_size;
        cache_config.ttl = config_.cache_ttl;
        cache_config.enable_statistics = true;
        
        cache_ = std::make_unique<themis::cache::BoundedLRUCache>(cache_config);
        
        spdlog::info("MetadataShard {} initialized with bounded cache (max: {} entries, ttl: {}s)",
                    config_.shard_id, cache_config.max_entries, cache_config.ttl.count());
    }
    
    // Phase 2.2: Initialize WAL and snapshot if persistence is enabled
    if (config_.enable_persistence && !config_.data_dir.empty()) {
        try {
            // Initialize WAL
            MetadataWALConfig wal_config;
            wal_config.wal_directory = config_.data_dir + "/wal";
            wal_config.snapshot_directory = config_.data_dir + "/snapshots";
            wal_config.snapshot_interval = config_.snapshot_interval;
            wal_config.max_snapshots = config_.max_snapshots;
            
            wal_ = std::make_unique<MetadataWAL>(wal_config);
            
            // Initialize snapshot manager
            snapshot_manager_ = std::make_unique<MetadataSnapshotManager>(
                wal_config.snapshot_directory,
                config_.max_snapshots
            );
            
            spdlog::info("MetadataShard {} initialized with persistence: wal_dir={}, snapshot_dir={}",
                        config_.shard_id,
                        wal_config.wal_directory,
                        wal_config.snapshot_directory);
        } catch (const std::exception& e) {
            spdlog::error("Failed to initialize MetadataShard persistence: {}", e.what());
            // Continue without persistence
            wal_.reset();
            snapshot_manager_.reset();
        }
    }
}

/** @brief Stop shard on destruction. */
MetadataShard::~MetadataShard() {
    stop();
}

/** @brief Initialize shard storage and optional WAL/snapshot recovery. */
bool MetadataShard::initialize() {
    spdlog::info("Initializing MetadataShard {}", config_.shard_id);
    
    // Phase 2.2: Initialize WAL if persistence enabled
    if (wal_ && !wal_->initialize()) {
        spdlog::warn("Failed to initialize MetadataShard WAL, continuing without persistence");
        wal_.reset();
        snapshot_manager_.reset();
    }
    
    // Phase 2.2: Attempt recovery from WAL
    if (wal_ && snapshot_manager_) {
        if (!recoverFromWAL()) {
            spdlog::warn("MetadataShard recovery failed, starting fresh");
        }
    }
    
    // Initialize storage for each partition
    for (auto partition : config_.partitions) {
        storage_[partition] = std::map<std::string, MetadataEntry>();
    }
    
    return true;
}

/** @brief Transition shard to running state. */
bool MetadataShard::start() {
    if (running_.load()) {
        spdlog::warn("MetadataShard {} already running", config_.shard_id);
        return false;
    }
    
    spdlog::info("Starting MetadataShard {}", config_.shard_id);
    running_.store(true);
    
    return true;
}

/** @brief Stop shard execution and clear cache state. */
void MetadataShard::stop() {
    if (!running_.load()) {
        return;
    }
    
    spdlog::info("Stopping MetadataShard {}", config_.shard_id);
    running_.store(false);
    
    // Clear cache
    if (cache_) {
        cache_->clear();
    }
}

/** @brief Read metadata entry with cache-first lookup strategy. */
std::optional<MetadataEntry> MetadataShard::get(
    MetadataPartitionKey partition,
    const std::string& key
) const {
    total_reads_.fetch_add(1, std::memory_order_relaxed);
    
    // Try cache first
    if (cache_) {
        std::string cache_key = makeCacheKey(partition, key);
        auto cached = cache_->get(cache_key);
        if (cached.has_value()) {
            cache_hits_.fetch_add(1, std::memory_order_relaxed);
            return MetadataEntry::fromJson(*cached);
        }
        cache_misses_.fetch_add(1, std::memory_order_relaxed);
    }
    
    // Check storage
    std::lock_guard<std::mutex> lock(storage_mutex_);
    
    auto partition_it = storage_.find(partition);
    if (partition_it == storage_.end()) {
        return std::nullopt;
    }
    
    auto entry_it = partition_it->second.find(key);
    if (entry_it == partition_it->second.end()) {
        return std::nullopt;
    }
    
    // Cache the entry
    if (cache_) {
        std::string cache_key = makeCacheKey(partition, key);
        cache_->put(cache_key, entry_it->second.toJson());
    }
    
    return entry_it->second;
}

/** @brief Insert or update metadata entry with optional WAL/consensus path. */
bool MetadataShard::put(
    MetadataPartitionKey partition,
    const std::string& key,
    const nlohmann::json& value
) {
    total_writes_.fetch_add(1, std::memory_order_relaxed);
    
    // Create metadata entry
    MetadataEntry entry;
    entry.key = key;
    entry.value = value;
    entry.partition = partition;
    entry.version = 1;
    entry.created_at = std::chrono::system_clock::now();
    entry.updated_at = entry.created_at;
    
    // Phase 2.2: Log to WAL if persistence enabled
    if (wal_) {
        try {
            wal_->logPut(partition, key, value, entry.version);
            uint64_t ops = ++operations_since_snapshot_;
            
            // Check if we should create a snapshot
            if (wal_->shouldCreateSnapshot(ops)) {
                spdlog::info("Triggering metadata snapshot creation after {} operations", ops);
                createPeriodicSnapshot();
            }
        } catch (const std::exception& e) {
            spdlog::warn("Failed to log metadata PUT to WAL: {}", e.what());
            // Continue operation despite WAL failure (graceful degradation)
        }
    }
    
    // Apply via consensus if available
    if (consensus_ && config_.enforce_strong_consistency) {
        if (!applyChange("PUT", partition, key, value)) {
            return false;
        }
    }
    
    // Update storage: determine final version and write atomically in one lock scope
    // to eliminate the TOCTOU window that exists when version-read and write happen
    // under separate locks.
    {
        std::lock_guard<std::mutex> lock(storage_mutex_);
        auto& partition_map = storage_[partition];
        auto it = partition_map.find(key);
        if (it != partition_map.end()) {
            entry.version = it->second.version + 1;
            entry.created_at = it->second.created_at;
        }
        partition_map[key] = entry;
    }
    
    // Update cache
    if (cache_) {
        std::string cache_key = makeCacheKey(partition, key);
        cache_->put(cache_key, entry.toJson());
    }
    
    // Notify subscribers
    {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        auto sub_it = subscriptions_.find(partition);
        if (sub_it != subscriptions_.end()) {
            for ([[maybe_unused]] auto& callback : sub_it->second) {
                callback([[maybe_unused]] entry);
            }
        }
    }
    
    return true;
}

/** @brief Remove metadata entry with optional WAL/consensus path. */
bool MetadataShard::remove(
    MetadataPartitionKey partition,
    const std::string& key
) {
    // Get version before deletion (for WAL)
    uint64_t version = 0;
    {
        std::lock_guard<std::mutex> lock(storage_mutex_);
        auto partition_it = storage_.find(partition);
        if (partition_it != storage_.end()) {
            auto entry_it = partition_it->second.find(key);
            if (entry_it != partition_it->second.end()) {
                version = entry_it->second.version;
            }
        }
    }
    
    // Phase 2.2: Log to WAL if persistence enabled
    if (wal_ && version > 0) {
        try {
            wal_->logDelete(partition, key, version);
            operations_since_snapshot_++;
        } catch (const std::exception& e) {
            spdlog::warn("Failed to log metadata DELETE to WAL: {}", e.what());
            // Continue operation despite WAL failure (graceful degradation)
        }
    }
    
    // Apply via consensus if available
    if (consensus_ && config_.enforce_strong_consistency) {
        if (!applyChange("DELETE", partition, key, nullptr)) {
            return false;
        }
    }
    
    // Remove from storage
    {
        std::lock_guard<std::mutex> lock(storage_mutex_);
        
        auto partition_it = storage_.find(partition);
        if (partition_it != storage_.end()) {
            partition_it->second.erase(key);
        }
    }
    
    // Invalidate cache
    if (cache_) {
        invalidateCache(partition, key);
    }
    
    return true;
}

/** @brief List all keys currently present in one partition. */
std::vector<std::string> MetadataShard::listKeys(MetadataPartitionKey partition) const {
    std::lock_guard<std::mutex> lock(storage_mutex_);
    
    std::vector<std::string> keys;
    
    auto partition_it = storage_.find(partition);
    if (partition_it != storage_.end()) {
        for (const auto& pair : partition_it->second) {
            keys.push_back(pair.first);
        }
    }
    
    return keys;
}

/** @brief Return entry-count statistics for one partition. */
nlohmann::json MetadataShard::getPartitionStats(MetadataPartitionKey partition) const {
    std::lock_guard<std::mutex> lock(storage_mutex_);
    
    auto partition_it = storage_.find(partition);
    if (partition_it == storage_.end()) {
        return {
            {"partition", static_cast<int>(partition)},
            {"entry_count", 0}
        };
    }
    
    return {
        {"partition", static_cast<int>(partition)},
        {"entry_count", partition_it->second.size()}
    };
}

/** @brief Return aggregated shard/cache/partition statistics snapshot. */
nlohmann::json MetadataShard::getStatistics() const {
    nlohmann::json stats = {
        {"shard_id", config_.shard_id},
        {"total_reads", total_reads_.load()},
        {"total_writes", total_writes_.load()},
        {"cache_hits", cache_hits_.load()},
        {"cache_misses", cache_misses_.load()}
    };
    
    // Add cache statistics if cache is enabled
    if (cache_) {
        auto cache_stats = cache_->getStatistics();
        stats["cache_size"] = cache_stats.current_size;
        stats["cache_hit_ratio"] = cache_stats.hit_ratio();
    }
    
    // Add partition statistics
    nlohmann::json partition_stats = nlohmann::json::array();
    for (auto partition : config_.partitions) {
        partition_stats.push_back(getPartitionStats(partition));
    }
    stats["partitions"] = partition_stats;
    
    return stats;
}

/** @brief Register callback subscriber for partition-level metadata changes. */
void MetadataShard::subscribe(
    MetadataPartitionKey partition,
    std::function<void(const MetadataEntry&)> callback
) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    subscriptions_[partition].push_back([[maybe_unused]] callback);
}

/** @brief Determine shard owner id by hashing key into metadata shard space. */
std::string MetadataShard::determineShardOwner(
    [[maybe_unused]] MetadataPartitionKey partition,
    const std::string& key
) const {
    // Simple hash-based sharding
    size_t hash = std::hash<std::string>{}(key);
    size_t shard_index = hash % config_.num_metadata_shards;
    return "shard_" + std::to_string(shard_index);
}

/** @brief Put metadata entry into cache when enabled. */
void MetadataShard::cacheEntry(const MetadataEntry& entry) {
    if (cache_) {
        std::string cache_key = makeCacheKey(entry.partition, entry.key);
        cache_->put(cache_key, entry.toJson());
    }
}

/** @brief Get metadata entry from cache by partition/key. */
std::optional<MetadataEntry> MetadataShard::getCachedEntry(
    MetadataPartitionKey partition,
    const std::string& key
) const {
    if (!cache_) {
        return std::nullopt;
    }
    
    std::string cache_key = makeCacheKey(partition, key);
    auto cached = cache_->get(cache_key);
    if (cached.has_value()) {
        return MetadataEntry::fromJson(*cached);
    }
    
    return std::nullopt;
}

/** @brief Remove cache entry by partition/key. */
void MetadataShard::invalidateCache(MetadataPartitionKey partition, const std::string& key) {
    if (cache_) {
        std::string cache_key = makeCacheKey(partition, key);
        cache_->remove(cache_key);
    }
}

/** @brief Propose metadata mutation through consensus and wait for commit. */
bool MetadataShard::applyChange(
    const std::string& operation,
    MetadataPartitionKey partition,
    const std::string& key,
    const nlohmann::json& value
) {
    if (!consensus_) {
        return true;  // No consensus module, proceed without replication
    }
    
    // Create log entry for consensus
    nlohmann::json log_data = {
        {"operation", operation},
        {"partition", static_cast<int>(partition)},
        {"key", key},
        {"value", value}
    };
    
    // Propose the entry to the consensus module and wait for quorum commit.
    auto log_index = consensus_->propose(operation, log_data);
    if (!log_index.has_value()) {
        spdlog::warn("MetadataShard::applyChange: consensus propose failed "
                     "(op={}, partition={}, key={})",
                     operation, static_cast<int>(partition), key);
        return false;
    }

    constexpr auto kCommitTimeout = std::chrono::milliseconds{5000};
    if (!consensus_->waitForCommit(*log_index, kCommitTimeout)) {
        spdlog::warn("MetadataShard::applyChange: consensus commit timeout "
                     "(op={}, partition={}, key={}, log_index={})",
                     operation, static_cast<int>(partition), key, *log_index);
        return false;
    }

    return true;
}

// MetadataShardRouter implementation

/** @brief Construct metadata router with fixed shard-count hashing domain. */
MetadataShardRouter::MetadataShardRouter(size_t num_shards)
    : num_shards_(num_shards)
    , total_operations_(0)
    , routing_errors_(0) {
}

/** @brief Register shard instance for routing. */
void MetadataShardRouter::addShard(const std::string& shard_id, std::shared_ptr<MetadataShard> shard) {
    std::lock_guard<std::mutex> lock(shards_mutex_);
    shards_[shard_id] = shard;
}

/** @brief Remove shard instance from routing table. */
void MetadataShardRouter::removeShard(const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(shards_mutex_);
    shards_.erase(shard_id);
}

/** @brief Route metadata read operation to owning shard. */
std::optional<MetadataEntry> MetadataShardRouter::get(
    MetadataPartitionKey partition,
    const std::string& key
) const {
    total_operations_.fetch_add(1, std::memory_order_relaxed);
    
    std::string target_shard = routeToShard(partition, key);
    
    std::lock_guard<std::mutex> lock(shards_mutex_);
    auto it = shards_.find(target_shard);
    if (it == shards_.end()) {
        routing_errors_.fetch_add(1, std::memory_order_relaxed);
        return std::nullopt;
    }
    
    return it->second->get(partition, key);
}

/** @brief Route metadata write operation to owning shard. */
bool MetadataShardRouter::put(
    MetadataPartitionKey partition,
    const std::string& key,
    const nlohmann::json& value
) {
    total_operations_.fetch_add(1, std::memory_order_relaxed);
    
    std::string target_shard = routeToShard(partition, key);
    
    std::lock_guard<std::mutex> lock(shards_mutex_);
    auto it = shards_.find(target_shard);
    if (it == shards_.end()) {
        routing_errors_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
    return it->second->put(partition, key, value);
}

/** @brief Route metadata delete operation to owning shard. */
bool MetadataShardRouter::remove(
    MetadataPartitionKey partition,
    const std::string& key
) {
    total_operations_.fetch_add(1, std::memory_order_relaxed);
    
    std::string target_shard = routeToShard(partition, key);
    
    std::lock_guard<std::mutex> lock(shards_mutex_);
    auto it = shards_.find(target_shard);
    if (it == shards_.end()) {
        routing_errors_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
    return it->second->remove(partition, key);
}

/** @brief Gather keys for a partition across all shards (scatter-gather). */
std::vector<std::string> MetadataShardRouter::listKeys(MetadataPartitionKey partition) const {
    std::lock_guard<std::mutex> lock(shards_mutex_);
    
    std::vector<std::string> all_keys;
    
    // Scatter-gather across all shards
    for (const auto& pair : shards_) {
        auto shard_keys = pair.second->listKeys(partition);
        all_keys.insert(all_keys.end(), shard_keys.begin(), shard_keys.end());
    }
    
    return all_keys;
}

/** @brief Return router operation/error counters plus per-shard statistics. */
nlohmann::json MetadataShardRouter::getStatistics() const {
    std::lock_guard<std::mutex> lock(shards_mutex_);
    
    nlohmann::json stats = {
        {"num_shards",static_cast<int>(shards_.size())},
        {"total_operations", total_operations_.load()},
        {"routing_errors", routing_errors_.load()}
    };
    
    nlohmann::json shard_stats = nlohmann::json::array();
    for (const auto& pair : shards_) {
        shard_stats.push_back(pair.second->getStatistics());
    }
    stats["shards"] = shard_stats;
    
    return stats;
}

/** @brief Resolve target shard id for key using hash-based routing. */
std::string MetadataShardRouter::routeToShard(
    [[maybe_unused]] MetadataPartitionKey partition,
    const std::string& key
) const {
    size_t hash = hashKey(key);
    size_t shard_index = hash % num_shards_;
    return "shard_" + std::to_string(shard_index);
}

/** @brief Hash metadata key for shard routing. */
size_t MetadataShardRouter::hashKey(const std::string& key) const {
    return std::hash<std::string>{}(key);
}

// Phase 2.2: Recovery and Snapshot methods

/** @brief Create persisted snapshot from current in-memory metadata storage. */
bool MetadataShard::createPeriodicSnapshot() {
    if (!wal_ || !snapshot_manager_) {
        return false;
    }
    
    try {
        std::lock_guard<std::mutex> lock(storage_mutex_);
        
        // Create snapshot
        auto snapshot_id = snapshot_manager_->createSnapshot(
            config_.shard_id,
            last_applied_lsn_,
            storage_
        );
        
        if (snapshot_id.has_value()) {
            // Reset operations counter
            operations_since_snapshot_.store(0);
            spdlog::info("Created metadata snapshot: id={}", snapshot_id.value());
            return true;
        }
        
        return false;
    } catch (const std::exception& e) {
        spdlog::error("Exception creating metadata snapshot: {}", e.what());
        return false;
    }
}

/** @brief Recover metadata state from latest snapshot and WAL replay. */
bool MetadataShard::recoverFromWAL() {
    if (!wal_ || !snapshot_manager_) {
        return false;
    }
    
    try {
        spdlog::info("Starting metadata recovery for shard {}", config_.shard_id);
        
        // Step 1: Load latest snapshot
        auto snapshot = snapshot_manager_->loadLatestSnapshot();
        if (snapshot.has_value()) {
            spdlog::info("Loaded metadata snapshot: id={}, entries={}",
                        snapshot->snapshot_id,
                        snapshot->total_entries);
            
            // Restore state from snapshot
            std::lock_guard<std::mutex> lock(storage_mutex_);
            storage_.clear();
            
            for (const auto& [partition_key, entries] : snapshot->partitions) {
                std::map<std::string, MetadataEntry> partition_entries = {};

                for (const auto& [key, value_json] : entries) {
                    MetadataEntry entry = MetadataEntry::fromJson(value_json);
                    partition_entries[key] = entry;
                }
                storage_[partition_key] = partition_entries;
            }
            
            last_applied_lsn_ = snapshot->last_applied_lsn;
            spdlog::info("Restored {} partitions from snapshot",static_cast<int>(storage_.size()));
        } else {
            spdlog::info("No snapshot found, starting with empty metadata");
            last_applied_lsn_ = LSN(0, 0);
        }
        
        // Step 2: Replay WAL from last_applied_lsn
        auto wal_entries = wal_->readEntries(last_applied_lsn_);
        spdlog::info("Replaying {} WAL entries from LSN ({}, {})",
                    wal_entries.size(),
                    last_applied_lsn_.segment,
                    last_applied_lsn_.offset);
        
        std::lock_guard<std::mutex> lock(storage_mutex_);
        for (const auto& entry : wal_entries) {
            // Apply entry to storage
            if (entry.type == MetadataWALEntryType::PUT ||
                entry.type == MetadataWALEntryType::UPDATE) {
                
                MetadataEntry metadata_entry;
                metadata_entry.key = entry.key;
                metadata_entry.value = entry.value;
                metadata_entry.partition = entry.partition;
                metadata_entry.version = entry.version;
                metadata_entry.created_at = std::chrono::system_clock::now();
                metadata_entry.updated_at = metadata_entry.created_at;
                
                storage_[entry.partition][entry.key] = metadata_entry;
                
            } else if (entry.type == MetadataWALEntryType::DELETE_OP) {
                auto partition_it = storage_.find(entry.partition);
                if (partition_it != storage_.end()) {
                    partition_it->second.erase(entry.key);
                }
            }
            
            last_applied_lsn_ = entry.lsn;
        }
        
        spdlog::info("Metadata recovery complete: {} entries restored",
                    [this]() {
                        size_t total = 0;
                        for (const auto& [_, entries] : storage_) {
                            total += entries.size();
                        }
                        return total;
                    }());
        
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Exception during metadata recovery: {}", e.what());
        return false;
    }
}

} // namespace sharding
} // namespace themisdb

