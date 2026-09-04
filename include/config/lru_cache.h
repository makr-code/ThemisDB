/**
 * @file lru_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <unordered_map>
#include <list>
#include <chrono>
#include <mutex>
#include <optional>

namespace themis {
namespace config {

/**
 * Thread-safe LRU cache with TTL (Time-To-Live) support.
 * 
 * This cache stores resolved config paths to avoid repeated filesystem lookups.
 * Each entry has a TTL and the cache has a maximum size limit (LRU eviction).
 * 
 * Thread-safety: All operations are protected by a mutex.
 */
template<typename Key, typename Value>
class LRUCacheWithTTL {
public:
    using TimePoint = std::chrono::steady_clock::time_point;
    
    /**
     * Create a cache with specified capacity and default TTL.
     * 
     * @param capacity Maximum number of entries (default: 1000)
     * @param default_ttl_seconds Default TTL in seconds (default: 300 = 5 minutes)
     */
    explicit LRUCacheWithTTL(size_t capacity = 1000, 
                             int default_ttl_seconds = 300)
        : capacity_(capacity),
          default_ttl_(std::chrono::seconds(default_ttl_seconds)) {}
    
    /**
     * Insert or update a cache entry.
     * 
     * @param key The cache key
     * @param value The value to cache
     * @param ttl_seconds Optional TTL in seconds (uses default if not specified)
     */
    void put(const Key& key, const Value& value, 
             std::optional<int> ttl_seconds = std::nullopt) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::steady_clock::now();
        auto ttl = ttl_seconds.has_value() 
            ? std::chrono::seconds(*ttl_seconds) 
            : default_ttl_;
        auto expires_at = now + ttl;
        
        // Check if key already exists
        auto it = map_.find(key);
        if (it != map_.end()) {
            // Update existing entry
            list_.erase(it->second.list_it);
            list_.push_front({key, value, expires_at});
            it->second = {value, expires_at, list_.begin()};
        } else {
            // Add new entry
            if (map_.size() >= capacity_) {
                // Evict least recently used
                evictLRU();
            }
            list_.push_front({key, value, expires_at});
            map_[key] = {value, expires_at, list_.begin()};
        }
    }
    
    /**
     * Retrieve a value from the cache.
     * 
     * @param key The cache key
     * @return The cached value if found and not expired, std::nullopt otherwise
     */
    std::optional<Value> get(const Key& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = map_.find(key);
        if (it == map_.end()) {
            misses_++;
            return std::nullopt;
        }
        
        // Check if expired
        auto now = std::chrono::steady_clock::now();
        if (now >= it->second.expires_at) {
            // Expired - remove and return nullopt
            list_.erase(it->second.list_it);
            map_.erase(it);
            expirations_++;
            misses_++;
            return std::nullopt;
        }
        
        // Move to front (most recently used)
        list_.splice(list_.begin(), list_, it->second.list_it);
        
        hits_++;
        return it->second.value;
    }
    
    /**
     * Remove a specific entry from the cache.
     * 
     * @param key The cache key to remove
     * @return true if the entry was found and removed
     */
    bool invalidate(const Key& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = map_.find(key);
        if (it == map_.end()) {
            return false;
        }
        
        list_.erase(it->second.list_it);
        map_.erase(it);
        return true;
    }
    
    /**
     * Clear all entries from the cache.
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        list_.clear();
        map_.clear();
    }
    
    /**
     * Get current cache size.
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.size();
    }
    
    /**
     * Check if cache is empty.
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.empty();
    }
    
    /**
     * Get cache statistics.
     */
    struct Stats {
        uint64_t hits = 0;
        uint64_t misses;
        uint64_t evictions;
        uint64_t expirations;
        size_t size;
        size_t capacity;
        double hit_rate;
    };
    
    Stats stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        uint64_t total = hits_ + misses_;
        double hit_rate = total > 0 ? static_cast<double>(hits_) / total : 0.0;
        
        return {
            hits_,
            misses_,
            evictions_,
            expirations_,
            map_.size(),
            capacity_,
            hit_rate
        };
    }
    
    /**
     * Remove all expired entries.
     * This is automatically done during get() operations, but can be called
     * explicitly for maintenance.
     * 
     * Note: This iterates from the back of the LRU list (least recently used).
     * Due to varying custom TTLs, expired entries may be scattered throughout
     * the list, so this may not catch all expired entries in a single pass.
     */
    void removeExpired() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::steady_clock::now();
        auto it = list_.rbegin();
        
        while (it != list_.rend()) {
            if (now < it->expires_at) {
                // Move to next entry
                ++it;
            } else {
                // Entry is expired, remove it
                map_.erase(it->key);
                it = decltype(it)(list_.erase(std::next(it).base()));
                expirations_++;
            }
        }
    }

private:
    struct ListEntry {
        Key key;
        Value value;
        TimePoint expires_at;
    };
    
    struct MapEntry {
        Value value;
        TimePoint expires_at;
        typename std::list<ListEntry>::iterator list_it;
    };
    
    void evictLRU() {
        if (list_.empty()) {
            return;
        }
        
        // Remove least recently used (back of list)
        auto& back = list_.back();
        map_.erase(back.key);
        list_.pop_back();
        evictions_++;
    }
    
    size_t capacity_;
    std::chrono::seconds default_ttl_;
    mutable std::mutex mutex_;
    
    std::list<ListEntry> list_;
    std::unordered_map<Key, MapEntry> map_;
    
    // Statistics
    uint64_t hits_ = 0;
    uint64_t misses_ = 0;
    uint64_t evictions_ = 0;
    uint64_t expirations_ = 0;
};

} // namespace config
} // namespace themis

