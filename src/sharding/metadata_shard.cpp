// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/metadata_shard.h"
#include "sharding/bounded_lru_cache.h"
#include <spdlog/spdlog.h>

namespace themisdb {
namespace sharding {

// Helper function to estimate metadata entry size
static size_t estimateMetadataEntrySize(const MetadataEntry& entry) {
    return entry.key.size() + entry.value.dump().size() + sizeof(MetadataEntry);
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
    , cache_misses_(0)
{
    // Initialize bounded cache if enabled
    if (config_.enable_cache) {
        // Cache will be created on first use since it's a template
        spdlog::info("MetadataShard initialized with bounded cache: max_entries={}, ttl={}s",
                     config_.cache_size, config_.cache_ttl.count());
    }
}

MetadataShard::~MetadataShard() {
    stop();
}

bool MetadataShard::initialize() {
    if (!consensus_) {
        spdlog::error("Consensus module required for metadata shard");
        return false;
    }
    
    spdlog::info("Metadata shard {} initialized", config_.shard_id);
    return true;
}

bool MetadataShard::start() {
    if (running_.exchange(true)) {
        spdlog::warn("Metadata shard already running");
        return false;
    }
    
    spdlog::info("Metadata shard {} started", config_.shard_id);
    return true;
}

void MetadataShard::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    
    spdlog::info("Metadata shard {} stopped", config_.shard_id);
}

std::optional<MetadataEntry> MetadataShard::get(
    MetadataPartitionKey partition,
    const std::string& key
) const {
    ++total_reads_;
    
    // Try cache first
    auto cached = getCachedEntry(partition, key);
    if (cached.has_value()) {
        ++cache_hits_;
        return cached;
    }
    
    ++cache_misses_;
    
    // Read from storage
    std::lock_guard<std::mutex> lock(storage_mutex_);
    
    auto part_it = storage_.find(partition);
    if (part_it == storage_.end()) {
        return std::nullopt;
    }
    
    auto entry_it = part_it->second.find(key);
    if (entry_it == part_it->second.end()) {
        return std::nullopt;
    }
    
    // Cache the entry
    const auto& entry = entry_it->second;
    cacheEntry(entry);
    
    return entry;
}

bool MetadataShard::put(
    MetadataPartitionKey partition,
    const std::string& key,
    const nlohmann::json& value
) {
    ++total_writes_;
    
    // Apply change via consensus
    if (!applyChange("PUT", partition, key, value)) {
        return false;
    }
    
    // Update storage
    std::lock_guard<std::mutex> lock(storage_mutex_);
    
    MetadataEntry entry;
    entry.key = key;
    entry.value = value;
    entry.partition = partition;
    entry.created_at = std::chrono::system_clock::now();
    entry.updated_at = entry.created_at;
    
    auto& partition_map = storage_[partition];
    auto it = partition_map.find(key);
    if (it != partition_map.end()) {
        entry.version = it->second.version + 1;
        entry.created_at = it->second.created_at;
    } else {
        entry.version = 1;
    }
    
    partition_map[key] = entry;
    
    // Update cache
    cacheEntry(entry);
    
    // Notify subscribers
    std::lock_guard<std::mutex> sub_lock(subscriptions_mutex_);
    auto sub_it = subscriptions_.find(partition);
    if (sub_it != subscriptions_.end()) {
        for (const auto& callback : sub_it->second) {
            callback(entry);
        }
    }
    
    return true;
}

bool MetadataShard::remove(
    MetadataPartitionKey partition,
    const std::string& key
) {
    ++total_writes_;
    
    // Apply change via consensus
    if (!applyChange("DELETE", partition, key, nlohmann::json())) {
        return false;
    }
    
    // Remove from storage
    std::lock_guard<std::mutex> lock(storage_mutex_);
    
    auto part_it = storage_.find(partition);
    if (part_it != storage_.end()) {
        part_it->second.erase(key);
    }
    
    // Invalidate cache
    invalidateCache(partition, key);
    
    return true;
}

std::vector<std::string> MetadataShard::listKeys(MetadataPartitionKey partition) const {
    std::lock_guard<std::mutex> lock(storage_mutex_);
    
    std::vector<std::string> keys;
    auto part_it = storage_.find(partition);
    if (part_it != storage_.end()) {
        for (const auto& [key, entry] : part_it->second) {
            keys.push_back(key);
        }
    }
    
    return keys;
}

nlohmann::json MetadataShard::getPartitionStats(MetadataPartitionKey partition) const {
    std::lock_guard<std::mutex> lock(storage_mutex_);
    
    nlohmann::json stats;
    auto part_it = storage_.find(partition);
    if (part_it != storage_.end()) {
        stats["entries"] = part_it->second.size();
    } else {
        stats["entries"] = 0;
    }
    
    return stats;
}

nlohmann::json MetadataShard::getStatistics() const {
    return {
        {"shard_id", config_.shard_id},
        {"total_reads", total_reads_.load()},
        {"total_writes", total_writes_.load()},
        {"cache_hits", cache_hits_.load()},
        {"cache_misses", cache_misses_.load()},
        {"cache_hit_rate", cache_hits_.load() > 0 ? 
            static_cast<double>(cache_hits_) / (cache_hits_ + cache_misses_) : 0.0}
    };
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
    return "metadata_shard_" + std::to_string(shard_index);
}

void MetadataShard::cacheEntry(const MetadataEntry& entry) {
    if (!config_.enable_cache) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    std::string cache_key = std::to_string(static_cast<int>(entry.partition)) + ":" + entry.key;
    cache_[cache_key] = entry;
    
    // Simple size-based eviction (not true LRU, but simpler and sufficient)
    // For production, consider using a proper LRU cache structure
    if (cache_.size() > config_.cache_size) {
        // Remove first entry (arbitrary, not LRU semantic)
        cache_.erase(cache_.begin());
    }
}

std::optional<MetadataEntry> MetadataShard::getCachedEntry(
    MetadataPartitionKey partition,
    const std::string& key
) const {
    if (!config_.enable_cache) {
        return std::nullopt;
    }
    
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    std::string cache_key = std::to_string(static_cast<int>(partition)) + ":" + key;
    auto it = cache_.find(cache_key);
    if (it == cache_.end()) {
        return std::nullopt;
    }
    
    // Check TTL
    auto age = std::chrono::system_clock::now() - it->second.updated_at;
    if (age > config_.cache_ttl) {
        // Note: This modification in a const method is acceptable because:
        // 1. cache_mutex_ is mutable
        // 2. This is an implementation detail (cache cleanup)
        // 3. Logically const from caller's perspective
        cache_.erase(it);
        return std::nullopt;
    }
    
    return it->second;
}

void MetadataShard::invalidateCache(MetadataPartitionKey partition, const std::string& key) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    std::string cache_key = std::to_string(static_cast<int>(partition)) + ":" + key;
    cache_.erase(cache_key);
}

bool MetadataShard::applyChange(
    const std::string& operation,
    MetadataPartitionKey partition,
    const std::string& key,
    const nlohmann::json& value
) {
    if (!consensus_) {
        return true;  // No consensus required
    }
    
    nlohmann::json data = {
        {"operation", operation},
        {"partition", static_cast<int>(partition)},
        {"key", key},
        {"value", value}
    };
    
    return consensus_->propose("METADATA_CHANGE", data);
}

} // namespace sharding
} // namespace themisdb
