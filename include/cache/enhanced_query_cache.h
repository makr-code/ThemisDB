/**
 * @file enhanced_query_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <atomic>
#include <functional>
#include <optional>
#include <algorithm>
#include "utils/tbb_compat.h"

namespace themis {
namespace cache {

/**
 * @brief Enhanced Query Result Cache with advanced metrics
 * 
 * Provides:
 * - Lock-free concurrent access via TBB concurrent_hash_map
 * - TTL-based expiration
 * - LRU eviction policy
 * - Detailed hit/miss metrics
 * - Cache warming support
 * - Query pattern analysis
 * 
 * Performance Gains:
 * - 50-90% latency reduction for repeated queries
 * - 2-5x throughput improvement for read-heavy workloads
 * - Reduced CPU and I/O load
 * 
 * Sources:
 * - Benchmark Analysis: benchmarks/BENCHMARK_ANALYSIS_20251210.md
 * - Quick Wins: docs/de/performance/OPTIMIZATION_QUICK_WINS.md
 * 
 * @tparam KeyType Type of cache key (must be hashable)
 * @tparam ValueType Type of cached value (must be copyable)
 */
template<typename KeyType, typename ValueType>
class EnhancedQueryCache {
public:
    struct Config {
        size_t max_entries = 10000;                     ///< Maximum cache entries
        std::chrono::seconds default_ttl{300};          ///< Default TTL (5 minutes)
        bool enable_metrics = true;                     ///< Enable detailed metrics
        bool enable_warming = false;                    ///< Enable cache warming
        size_t max_memory_mb = 512;                     ///< Max memory usage (MB)
    };
    
    explicit EnhancedQueryCache(const Config& config);
    ~EnhancedQueryCache() = default;
    
    /**
     * @brief Get value from cache
     * @param key Cache key
     * @return Cached value if found and not expired
     */
    std::optional<ValueType> get(const KeyType& key);
    
    /**
     * @brief Put value into cache
     * @param key Cache key
     * @param value Value to cache
     * @param ttl Time-to-live (optional, uses default if not specified)
     */
    void put(const KeyType& key, const ValueType& value, 
             std::optional<std::chrono::seconds> ttl = std::nullopt);
    
    /**
     * @brief Check if key exists in cache
     * @param key Cache key
     * @return true if key exists and not expired
     */
    bool contains(const KeyType& key) const;
    
    /**
     * @brief Remove entry from cache
     * @param key Cache key
     */
    void remove(const KeyType& key);
    
    /**
     * @brief Clear all entries
     */
    void clear();
    
    /**
     * @brief Get cache statistics
     */
    struct Stats {
        size_t entries = 0;
        size_t hits = 0;
        size_t misses = 0;
        size_t evictions = 0;
        size_t expirations = 0;
        double hit_rate = 0.0;
        size_t memory_usage_bytes = 0;
        double avg_query_time_ms = 0.0;
        
        // Advanced metrics
        size_t warm_hits = 0;              ///< Hits on warmed entries
        size_t cold_misses = 0;            ///< Misses that were never cached
        size_t hot_entries = 0;            ///< Entries accessed >10 times
        double avg_entry_age_seconds = 0.0;
    };
    
    Stats getStats() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStats();
    
    /**
     * @brief Prune expired entries
     * @return Number of entries removed
     */
    size_t pruneExpired();
    
    /**
     * @brief Warm cache with provided entries
     * @param entries Entries to warm cache with
     */
    void warm(const std::vector<std::pair<KeyType, ValueType>>& entries);
    
    /**
     * @brief Get top N most accessed keys
     * @param n Number of keys to return
     * @return Vector of keys sorted by access count
     */
    std::vector<KeyType> getHotKeys(size_t n) const;
    
private:
    struct CacheEntry {
        ValueType value;
        std::chrono::steady_clock::time_point expiry;
        std::chrono::steady_clock::time_point created;
        std::atomic<size_t> access_count{0};
        std::atomic<std::chrono::steady_clock::time_point::rep> last_access;
        bool warmed = false;
        
        CacheEntry() = default;
        CacheEntry(const CacheEntry& other) 
            : value(other.value)
            , expiry(other.expiry)
            , created(other.created)
            , access_count(other.access_count.load())
            , last_access(other.last_access.load())
            , warmed(other.warmed)
        {}
        
        bool isExpired() const {
            return std::chrono::steady_clock::now() >= expiry;
        }
        
        void touch() {
            access_count.fetch_add(1);
            last_access.store(std::chrono::steady_clock::now().time_since_epoch().count());
        }
    };
    
    /**
     * @brief Check if cache is full and evict if needed
     */
    void evictIfNeeded();
    
    /**
     * @brief Estimate memory usage of entry
     */
    size_t estimateEntrySize(const ValueType& value) const;
    
    Config config_;
    
    // Lock-free concurrent hash map
    tbb::concurrent_hash_map<KeyType, CacheEntry> cache_;
    
    // Statistics
    std::atomic<size_t> hits_{0};
    std::atomic<size_t> misses_{0};
    std::atomic<size_t> evictions_{0};
    std::atomic<size_t> expirations_{0};
    std::atomic<size_t> warm_hits_{0};
    std::atomic<size_t> cold_misses_{0};
    std::atomic<size_t> total_query_time_ns_{0};
    std::atomic<size_t> estimated_memory_bytes_{0};
};

// Template implementation

template<typename KeyType, typename ValueType>
EnhancedQueryCache<KeyType, ValueType>::EnhancedQueryCache(const Config& config)
    : config_(config)
{
}

template<typename KeyType, typename ValueType>
std::optional<ValueType> EnhancedQueryCache<KeyType, ValueType>::get(const KeyType& key) {
    auto start = std::chrono::steady_clock::now();
    
    typename tbb::concurrent_hash_map<KeyType, CacheEntry>::const_accessor acc;
    if (cache_.find(acc, key)) {
        // Found entry, check if expired
        if (acc->second.isExpired()) {
            acc.release();
            
            // Remove expired entry
            typename tbb::concurrent_hash_map<KeyType, CacheEntry>::accessor write_acc;
            if (cache_.find(write_acc, key)) {
                size_t entry_size = estimateEntrySize(write_acc->second.value);
                cache_.erase(write_acc);
                estimated_memory_bytes_.fetch_sub(entry_size);
                expirations_.fetch_add(1);
            }
            
            misses_.fetch_add(1);
            cold_misses_.fetch_add(1);
            return std::nullopt;
        }
        
        // Valid entry, update access stats
        const_cast<CacheEntry&>(acc->second).touch();
        
        hits_.fetch_add(1);
        if (acc->second.warmed) {
            warm_hits_.fetch_add(1);
        }
        
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        total_query_time_ns_.fetch_add(duration.count());
        
        return acc->second.value;
    }
    
    misses_.fetch_add(1);
    cold_misses_.fetch_add(1);
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    total_query_time_ns_.fetch_add(duration.count());
    
    return std::nullopt;
}

template<typename KeyType, typename ValueType>
void EnhancedQueryCache<KeyType, ValueType>::put(
    const KeyType& key,
    const ValueType& value,
    std::optional<std::chrono::seconds> ttl
) {
    evictIfNeeded();
    
    CacheEntry entry;
    entry.value = value;
    entry.created = std::chrono::steady_clock::now();
    entry.expiry = entry.created + (ttl.value_or(config_.default_ttl));
    entry.warmed = false;
    
    size_t entry_size = estimateEntrySize(value);
    
    typename tbb::concurrent_hash_map<KeyType, CacheEntry>::accessor acc;
    if (cache_.insert(acc, key)) {
        // New entry
        acc->second = std::move(entry);
        estimated_memory_bytes_.fetch_add(entry_size);
    } else {
        // Update existing entry
        size_t old_size = estimateEntrySize(acc->second.value);
        acc->second = std::move(entry);
        estimated_memory_bytes_.fetch_add(entry_size);
        estimated_memory_bytes_.fetch_sub(old_size);
    }
}

template<typename KeyType, typename ValueType>
bool EnhancedQueryCache<KeyType, ValueType>::contains(const KeyType& key) const {
    typename tbb::concurrent_hash_map<KeyType, CacheEntry>::const_accessor acc;
    if (cache_.find(acc, key)) {
        return !acc->second.isExpired();
    }
    return false;
}

template<typename KeyType, typename ValueType>
void EnhancedQueryCache<KeyType, ValueType>::remove(const KeyType& key) {
    typename tbb::concurrent_hash_map<KeyType, CacheEntry>::accessor acc;
    if (cache_.find(acc, key)) {
        size_t entry_size = estimateEntrySize(acc->second.value);
        cache_.erase(acc);
        estimated_memory_bytes_.fetch_sub(entry_size);
    }
}

template<typename KeyType, typename ValueType>
void EnhancedQueryCache<KeyType, ValueType>::clear() {
    cache_.clear();
    estimated_memory_bytes_.store(0);
}

template<typename KeyType, typename ValueType>
typename EnhancedQueryCache<KeyType, ValueType>::Stats 
EnhancedQueryCache<KeyType, ValueType>::getStats() const {
    Stats stats;
    stats.entries = cache_.size();
    stats.hits = hits_.load();
    stats.misses = misses_.load();
    stats.evictions = evictions_.load();
    stats.expirations = expirations_.load();
    stats.warm_hits = warm_hits_.load();
    stats.cold_misses = cold_misses_.load();
    stats.memory_usage_bytes = estimated_memory_bytes_.load();
    
    size_t total_queries = stats.hits + stats.misses;
    if (total_queries > 0) {
        stats.hit_rate = static_cast<double>(stats.hits) / total_queries;
        
        size_t total_time_ns = total_query_time_ns_.load();
        stats.avg_query_time_ms = (total_time_ns / static_cast<double>(total_queries)) / 1000000.0;
    }
    
    // Count hot entries
    for (typename tbb::concurrent_hash_map<KeyType, CacheEntry>::const_iterator it = cache_.begin();
         it != cache_.end(); ++it) {
        if (it->second.access_count.load() > 10) {
            stats.hot_entries++;
        }
    }
    
    return stats;
}

template<typename KeyType, typename ValueType>
void EnhancedQueryCache<KeyType, ValueType>::resetStats() {
    hits_.store(0);
    misses_.store(0);
    evictions_.store(0);
    expirations_.store(0);
    warm_hits_.store(0);
    cold_misses_.store(0);
    total_query_time_ns_.store(0);
}

template<typename KeyType, typename ValueType>
size_t EnhancedQueryCache<KeyType, ValueType>::pruneExpired() {
    size_t removed = 0;
    auto now = std::chrono::steady_clock::now();
    
    std::vector<KeyType> expired_keys;
    
    for (typename tbb::concurrent_hash_map<KeyType, CacheEntry>::iterator it = cache_.begin();
         it != cache_.end(); ++it) {
        if (now >= it->second.expiry) {
            expired_keys.push_back(it->first);
        }
    }
    
    for (const auto& key : expired_keys) {
        remove(key);
        removed++;
    }
    
    expirations_.fetch_add(removed);
    return removed;
}

template<typename KeyType, typename ValueType>
void EnhancedQueryCache<KeyType, ValueType>::warm(
    const std::vector<std::pair<KeyType, ValueType>>& entries
) {
    if (!config_.enable_warming) {
        return;
    }
    
    for (const auto& [key, value] : entries) {
        CacheEntry entry;
        entry.value = value;
        entry.created = std::chrono::steady_clock::now();
        entry.expiry = entry.created + config_.default_ttl;
        entry.warmed = true;
        
        typename tbb::concurrent_hash_map<KeyType, CacheEntry>::accessor acc;
        if (cache_.insert(acc, key)) {
            acc->second = std::move(entry);
            size_t entry_size = estimateEntrySize(value);
            estimated_memory_bytes_.fetch_add(entry_size);
        }
    }
}

template<typename KeyType, typename ValueType>
std::vector<KeyType> EnhancedQueryCache<KeyType, ValueType>::getHotKeys(size_t n) const {
    std::vector<std::pair<KeyType, size_t>> entries;
    
    for (typename tbb::concurrent_hash_map<KeyType, CacheEntry>::const_iterator it = cache_.begin();
         it != cache_.end(); ++it) {
        entries.emplace_back(it->first, it->second.access_count.load());
    }
    
    std::partial_sort(entries.begin(), 
                     entries.begin() + std::min(n, entries.size()),
                     entries.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<KeyType> hot_keys = {};

    for (size_t i = 0; i < std::min(n, entries.size()); ++i) {
        hot_keys.push_back(entries[i].first);
    }
    
    return hot_keys;
}

template<typename KeyType, typename ValueType>
void EnhancedQueryCache<KeyType, ValueType>::evictIfNeeded() {
    // Check memory limit
    size_t current_memory = estimated_memory_bytes_.load();
    size_t max_memory = config_.max_memory_mb * 1024 * 1024;
    
    if (current_memory < max_memory && cache_.size() < config_.max_entries) {
        return;
    }
    
    // Need to evict - use LRU (least recently used)
    std::vector<std::pair<KeyType, std::chrono::steady_clock::time_point::rep>> entries;
    
    for (typename tbb::concurrent_hash_map<KeyType, CacheEntry>::const_iterator it = cache_.begin();
         it != cache_.end(); ++it) {
        entries.emplace_back(it->first, it->second.last_access.load());
    }
    
    if (entries.empty()) {
        return;
    }
    
    // Sort by last access time (oldest first)
    std::sort(entries.begin(), entries.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    // Evict oldest 10%
    size_t to_evict = std::max(size_t(1), entries.size() / 10);
    
    for (size_t i = 0; i < to_evict && i < entries.size(); ++i) {
        remove(entries[i].first);
        evictions_.fetch_add(1);
    }
}

template<typename KeyType, typename ValueType>
size_t EnhancedQueryCache<KeyType, ValueType>::estimateEntrySize(const ValueType& value) const {
    // Simple estimation - can be specialized for different types
    return sizeof(ValueType) + sizeof(CacheEntry);
}

} // namespace cache
} // namespace themis
