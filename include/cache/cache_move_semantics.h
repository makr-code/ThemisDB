/**
 * @file cache_move_semantics.h
 * @brief Cache module classes with explicit move semantics
 * @version 1.0.0
 * @date 2026-07-05
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <utility>
#include <chrono>
#include <functional>
#include <mutex>

namespace themis {
namespace cache {

/**
 * @brief Cache entry template with explicit move semantics
 * 
 * Generic cache entry storing key-value pairs with expiration and access tracking.
 * Designed for efficient move-based cache operations.
 * 
 * Template Parameters:
 * - K: Key type (must be movable)
 * - V: Value type (must be movable)
 * 
 * Thread-safety:
 * - NOT thread-safe for concurrent moves
 * - Only move during initialization/teardown
 * 
 * Move Semantics:
 * - Explicit move constructor transfers key and value
 * - Explicit move assignment transfers ownership
 * - Copy semantics are deleted
 * - All operations marked noexcept
 * 
 * @code
 * CacheEntry<std::string, int> entry1("key", 42);
 * CacheEntry<std::string, int> entry2 = std::move(entry1);  // ✅ Move
 * @endcode
 */
template<typename K, typename V>
class CacheEntry {
private:
    K key_;
    V value_;
    std::chrono::system_clock::time_point expiry_;
    size_t access_count_ = 0;

public:
    /// Default constructor
    CacheEntry() = default;

    /// Constructor with key and value
    CacheEntry(const K& key, const V& value)
        : key_(key), value_(value) {}

    /**
     * @brief Move constructor - transfers key and value
     * 
     * @param[in,out] other Source entry (will be empty after move)
     * 
     * @post this->key_ = std::move(other.key_)
     * @post this->value_ = std::move(other.value_)
     * @post other.access_count_ = 0
     * 
     * Exception safety: noexcept
     */
    CacheEntry(CacheEntry&& other) noexcept
        : key_(std::move(other.key_)),
          value_(std::move(other.value_)),
          expiry_(std::move(other.expiry_)),
          access_count_(other.access_count_) {
        other.access_count_ = 0;
    }

    /**
     * @brief Move assignment operator - transfers key and value
     * 
     * @param[in,out] other Source entry (will be empty after move)
     * @return Reference to this
     * 
     * @post this->key_ = std::move(other.key_)
     * @post this->value_ = std::move(other.value_)
     * @post other.access_count_ = 0
     * 
     * Exception safety: noexcept
     */
    CacheEntry& operator=(CacheEntry&& other) noexcept {
        if (this != &other) {
            key_ = std::move(other.key_);
            value_ = std::move(other.value_);
            expiry_ = std::move(other.expiry_);
            access_count_ = other.access_count_;
            other.access_count_ = 0;
        }
        return *this;
    }

    /// Delete copy constructor
    CacheEntry(const CacheEntry&) = delete;
    
    /// Delete copy assignment operator
    CacheEntry& operator=(const CacheEntry&) = delete;

    /// Destructor
    ~CacheEntry() = default;

    // Accessors

    const K& getKey() const { return key_; }
    V& getValue() { return value_; }
    const V& getValue() const { return value_; }

    std::chrono::system_clock::time_point getExpiry() const { return expiry_; }
    void setExpiry(std::chrono::system_clock::time_point exp) { expiry_ = exp; }

    size_t getAccessCount() const noexcept { return access_count_; }
    void incrementAccessCount() noexcept { ++access_count_; }
    void resetAccessCount() noexcept { access_count_ = 0; }

    /// Check if entry has expired
    bool isExpired() const {
        return std::chrono::system_clock::now() > expiry_;
    }
};

/**
 * @brief LRU eviction policy with explicit move semantics
 */
class LRUEvictionPolicy {
private:
    struct Entry {
        std::string key;
        size_t last_accessed = 0;
        
        Entry() = default;
        Entry(Entry&& other) noexcept
            : key(std::move(other.key)), last_accessed(other.last_accessed) {}
        Entry& operator=(Entry&& other) noexcept {
            if (this != &other) {
                key = std::move(other.key);
                last_accessed = other.last_accessed;
            }
            return *this;
        }
        Entry(const Entry&) = delete;
        Entry& operator=(const Entry&) = delete;
    };

    std::vector<Entry> lru_list_;
    size_t max_size_ = 1000;
    size_t current_size_ = 0;

public:
    LRUEvictionPolicy() = default;

    explicit LRUEvictionPolicy(size_t max_size) : max_size_(max_size) {}

    /**
     * @brief Move constructor - transfers LRU list
     */
    LRUEvictionPolicy(LRUEvictionPolicy&& other) noexcept
        : lru_list_(std::move(other.lru_list_)),
          max_size_(other.max_size_),
          current_size_(other.current_size_) {
        other.current_size_ = 0;
    }

    /**
     * @brief Move assignment operator
     */
    LRUEvictionPolicy& operator=(LRUEvictionPolicy&& other) noexcept {
        if (this != &other) {
            lru_list_ = std::move(other.lru_list_);
            max_size_ = other.max_size_;
            current_size_ = other.current_size_;
            other.current_size_ = 0;
        }
        return *this;
    }

    LRUEvictionPolicy(const LRUEvictionPolicy&) = delete;
    LRUEvictionPolicy& operator=(const LRUEvictionPolicy&) = delete;

    ~LRUEvictionPolicy() = default;

    /// Mark key as recently accessed
    void access(const std::string& key) noexcept;

    /// Get least recently used key for eviction
    std::string getEvictionCandidate() noexcept;

    /// Clear all entries
    void clear() noexcept {
        lru_list_.clear();
        current_size_ = 0;
    }

    size_t getCurrentSize() const noexcept { return current_size_; }
    size_t getMaxSize() const noexcept { return max_size_; }
};

/**
 * @brief Cache manager with explicit move semantics
 */
class CacheManager {
private:
    std::map<std::string, std::string> cache_entries_;
    LRUEvictionPolicy eviction_policy_;
    size_t hit_count_ = 0;
    size_t miss_count_ = 0;
    mutable std::mutex cache_lock_;

public:
    CacheManager() = default;

    /**
     * @brief Move constructor - transfers cache entries and policy
     */
    CacheManager(CacheManager&& other) noexcept
        : cache_entries_(std::move(other.cache_entries_)),
          eviction_policy_(std::move(other.eviction_policy_)),
          hit_count_(other.hit_count_),
          miss_count_(other.miss_count_) {
        other.hit_count_ = 0;
        other.miss_count_ = 0;
    }

    /**
     * @brief Move assignment operator
     */
    CacheManager& operator=(CacheManager&& other) noexcept {
        if (this != &other) {
            cache_entries_ = std::move(other.cache_entries_);
            eviction_policy_ = std::move(other.eviction_policy_);
            hit_count_ = other.hit_count_;
            miss_count_ = other.miss_count_;
            other.hit_count_ = 0;
            other.miss_count_ = 0;
        }
        return *this;
    }

    CacheManager(const CacheManager&) = delete;
    CacheManager& operator=(const CacheManager&) = delete;

    ~CacheManager() = default;

    /// Put value in cache
    void put(const std::string& key, const std::string& value);

    /// Get value from cache
    bool get(const std::string& key, std::string& value) noexcept;

    /// Remove value from cache
    bool remove(const std::string& key) noexcept;

    /// Clear cache
    void clear() noexcept;

    /// Get cache statistics
    size_t getHitCount() const noexcept { return hit_count_; }
    size_t getMissCount() const noexcept { return miss_count_; }
    double getHitRate() const noexcept {
        size_t total = hit_count_ + miss_count_;
        return total > 0 ? static_cast<double>(hit_count_) / total : 0.0;
    }
    size_t getSize() const noexcept { return cache_entries_.size(); }
};

}  // namespace cache
}  // namespace themis
