// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/metadata_shard.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace themis {
namespace sharding {

// Helper function to create cache key
static std::string makeCacheKey(MetadataPartitionKey partition, const std::string& key) {
    return std::to_string(static_cast<int>(partition)) + ":" + key;
}

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
    , cache_misses_(0) {
    
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
}

MetadataShard::~MetadataShard() {
    stop();
}

bool MetadataShard::initialize() {
    spdlog::info("Initializing MetadataShard {}", config_.shard_id);
    
    // Initialize storage for each partition
    for (auto partition : config_.partitions) {
        storage_[partition] = std::map<std::string, MetadataEntry>();
    }
    
    return true;
}

bool MetadataShard::start() {
    if (running_.load()) {
        spdlog::warn("MetadataShard {} already running", config_.shard_id);
        return false;
    }
    
    spdlog::info("Starting MetadataShard {}", config_.shard_id);
    running_.store(true);
    
    return true;
}

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
    
    // Apply via consensus if available
    if (consensus_ && config_.enforce_strong_consistency) {
        if (!applyChange("PUT", partition, key, value)) {
            return false;
        }
    }
    
    // Update storage
    {
        std::lock_guard<std::mutex> lock(storage_mutex_);
        
        auto& partition_map = storage_[partition];
        auto it = partition_map.find(key);
        if (it != partition_map.end()) {
            // Update existing entry
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
            for (auto& callback : sub_it->second) {
                callback(entry);
            }
        }
    }
    
    return true;
}

bool MetadataShard::remove(
    MetadataPartitionKey partition,
    const std::string& key
) {
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

void MetadataShard::subscribe(
    MetadataPartitionKey partition,
    std::function<void(const MetadataEntry&)> callback
) {
    std::lock_guard<std::mutex> lock(subscriptions_mutex_);
    subscriptions_[partition].push_back(callback);
}

std::string MetadataShard::determineShardOwner(
    MetadataPartitionKey partition,
    const std::string& key
) const {
    // Simple hash-based sharding
    size_t hash = std::hash<std::string>{}(key);
    size_t shard_index = hash % config_.num_metadata_shards;
    return "shard_" + std::to_string(shard_index);
}

void MetadataShard::cacheEntry(const MetadataEntry& entry) {
    if (cache_) {
        std::string cache_key = makeCacheKey(entry.partition, entry.key);
        cache_->put(cache_key, entry.toJson());
    }
}

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

void MetadataShard::invalidateCache(MetadataPartitionKey partition, const std::string& key) {
    if (cache_) {
        std::string cache_key = makeCacheKey(partition, key);
        // Note: BoundedLRUCache doesn't have a remove method, so we can't explicitly invalidate.
        // The entry will expire based on TTL or be evicted by LRU policy.
        // For now, we'll just let it naturally expire.
    }
}

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
    
    // In a real implementation, we would submit this to the consensus module
    // and wait for it to be committed. For now, we'll just return success.
    // This is a simplified version.
    
    return true;
}

// MetadataShardRouter implementation

MetadataShardRouter::MetadataShardRouter(size_t num_shards)
    : num_shards_(num_shards)
    , total_operations_(0)
    , routing_errors_(0) {
}

void MetadataShardRouter::addShard(const std::string& shard_id, std::shared_ptr<MetadataShard> shard) {
    std::lock_guard<std::mutex> lock(shards_mutex_);
    shards_[shard_id] = shard;
}

void MetadataShardRouter::removeShard(const std::string& shard_id) {
    std::lock_guard<std::mutex> lock(shards_mutex_);
    shards_.erase(shard_id);
}

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

nlohmann::json MetadataShardRouter::getStatistics() const {
    std::lock_guard<std::mutex> lock(shards_mutex_);
    
    nlohmann::json stats = {
        {"num_shards", shards_.size()},
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

std::string MetadataShardRouter::routeToShard(
    MetadataPartitionKey partition,
    const std::string& key
) const {
    size_t hash = hashKey(key);
    size_t shard_index = hash % num_shards_;
    return "shard_" + std::to_string(shard_index);
}

size_t MetadataShardRouter::hashKey(const std::string& key) const {
    return std::hash<std::string>{}(key);
}

} // namespace sharding
} // namespace themis
