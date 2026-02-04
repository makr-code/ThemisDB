// Copyright 2025 ThemisDB
// Licensed under MIT License

#ifndef THEMISDB_SHARDING_BOUNDED_LRU_CACHE_H
#define THEMISDB_SHARDING_BOUNDED_LRU_CACHE_H

#include <map>
#include <deque>
#include <mutex>
#include <optional>
#include <functional>
#include <chrono>

namespace themisdb {
namespace sharding {

/**
 * @brief Bounded LRU Cache with TTL and Size Limits
 * 
 * Thread-safe LRU cache with:
 * - Maximum entry count limit
 * - Maximum memory size limit
 * - Time-to-Live (TTL) expiration
 * - LRU eviction policy
 * - Metrics tracking
 * 
 * @tparam K Key type
 * @tparam V Value type
 */
template<typename K, typename V>
class BoundedLRUCache {
public:
    struct Config {
        size_t max_entries;
        size_t max_bytes;
        std::chrono::milliseconds ttl;
        std::function<size_t(const V&)> size_estimator;
    };
    
    struct Stats {
        size_t entries;
        size_t bytes_used;
        size_t hits;
        size_t misses;
        size_t evictions;
    };
    
    explicit BoundedLRUCache(const Config& config)
        : config_(config), total_bytes_used_(0), hits_(0), misses_(0), evictions_(0) {}
    
    /**
     * @brief Get value from cache
     * @param key Key to lookup
     * @return Value if found and not expired, nullopt otherwise
     */
    std::optional<V> get(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = data_.find(key);
        if (it == data_.end()) {
            ++misses_;
            return std::nullopt;
        }
        
        // Check TTL
        auto ts_it = timestamps_.find(key);
        if (ts_it != timestamps_.end()) {
            auto age = std::chrono::system_clock::now() - ts_it->second;
            if (age > config_.ttl) {
                // Expired
                evictKey(key);
                ++misses_;
                return std::nullopt;
            }
        }
        
        // Update LRU order
        updateLRU(key);
        ++hits_;
        return it->second;
    }
    
    /**
     * @brief Put value into cache
     * @param key Key to store
     * @param value Value to store
     */
    void put(const K& key, const V& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Remove old value if exists
        auto it = data_.find(key);
        if (it != data_.end()) {
            size_t old_size = config_.size_estimator ? config_.size_estimator(it->second) : 0;
            total_bytes_used_ -= old_size;
        }
        
        // Calculate new size
        size_t new_size = config_.size_estimator ? config_.size_estimator(value) : 0;
        
        // Evict entries if necessary
        while (needsEviction(new_size)) {
            evictOne();
        }
        
        // Insert new entry
        data_[key] = value;
        timestamps_[key] = std::chrono::system_clock::now();
        total_bytes_used_ += new_size;
        updateLRU(key);
    }
    
    /**
     * @brief Evict least recently used entry
     */
    void evictOne() {
        if (lru_order_.empty()) {
            return;
        }
        
        K key = lru_order_.front();
        lru_order_.pop_front();
        evictKey(key);
    }
    
    /**
     * @brief Evict all expired entries
     * @return Number of entries evicted
     */
    size_t evictExpired() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::system_clock::now();
        size_t evicted = 0;
        
        std::vector<K> expired_keys;
        for (const auto& [key, ts] : timestamps_) {
            auto age = now - ts;
            if (age > config_.ttl) {
                expired_keys.push_back(key);
            }
        }
        
        for (const K& key : expired_keys) {
            evictKey(key);
            ++evicted;
        }
        
        return evicted;
    }
    
    /**
     * @brief Get cache statistics
     * @return Current stats
     */
    Stats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return Stats{
            data_.size(),
            total_bytes_used_,
            hits_,
            misses_,
            evictions_
        };
    }
    
    /**
     * @brief Clear all entries
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        data_.clear();
        lru_order_.clear();
        timestamps_.clear();
        total_bytes_used_ = 0;
    }

private:
    Config config_;
    std::map<K, V> data_;
    std::deque<K> lru_order_;
    std::map<K, std::chrono::system_clock::time_point> timestamps_;
    mutable std::mutex mutex_;
    size_t total_bytes_used_;
    size_t hits_;
    size_t misses_;
    size_t evictions_;
    
    /**
     * @brief Check if eviction is needed
     */
    bool needsEviction(size_t new_size) const {
        if (data_.size() >= config_.max_entries) {
            return true;
        }
        if (total_bytes_used_ + new_size > config_.max_bytes) {
            return true;
        }
        return false;
    }
    
    /**
     * @brief Update LRU order for key
     */
    void updateLRU(const K& key) {
        // Remove from current position
        lru_order_.erase(
            std::remove(lru_order_.begin(), lru_order_.end(), key),
            lru_order_.end()
        );
        // Add to back (most recently used)
        lru_order_.push_back(key);
    }
    
    /**
     * @brief Evict specific key
     */
    void evictKey(const K& key) {
        auto it = data_.find(key);
        if (it != data_.end()) {
            size_t size = config_.size_estimator ? config_.size_estimator(it->second) : 0;
            total_bytes_used_ -= size;
            data_.erase(it);
            ++evictions_;
        }
        timestamps_.erase(key);
        lru_order_.erase(
            std::remove(lru_order_.begin(), lru_order_.end(), key),
            lru_order_.end()
        );
    }
};

} // namespace sharding
} // namespace themisdb

#endif // THEMISDB_SHARDING_BOUNDED_LRU_CACHE_H
