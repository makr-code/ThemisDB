/**
 * @file lru_cache.h
 * @brief LRU cache with move semantics and container support
 * @version 0.1.0
 * @note Maturity: 🟡 BETA
 * @note Gap Categories: CWE-457 (uninitialized variable), CWE-672 (use-after-free)
 * 
 * Provides:
 * - Efficient LRU eviction policy
 * - Move-enabled container support (std::move in insertion)
 * - Moved-from state validation
 * - Access tracking for cache analytics
 * 
 * @see ThemisDB Remediation Roadmap: Sprint 8 Phase 1C
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <list>
#include <optional>
#include <functional>
#include <cstdint>

namespace themis {
namespace cache {

/**
 * @brief Least Recently Used (LRU) cache with move semantics
 * 
 * Template-based cache implementation featuring:
 * - O(1) insert, retrieve, delete
 * - LRU eviction when capacity exceeded
 * - Move constructor/assignment for container transfer
 * - Moved-from state tracking
 * 
 * @tparam Key Key type (must be hashable and comparable)
 * @tparam Value Value type (must support move semantics)
 */
template <typename Key, typename Value>
class LRUCache {
public:
    /**
     * @brief Cache entry with metadata
     */
    struct Entry {
        Key key;                   ///< Cache key
        Value value;               ///< Cached value
        uint64_t access_count = 0; ///< Number of accesses
        int64_t timestamp_us = 0;  ///< Last access time in microseconds
    };

    /**
     * @brief Cache hit/miss callback
     */
    using HitCallback = std::function<void(const Key&, const Value&)>;
    using MissCallback = std::function<void(const Key&)>;
    using EvictionCallback = std::function<void(const Key&, const Value&)>;

    /**
     * @brief Create LRU cache with capacity
     * 
     * @param max_entries Maximum number of entries to store
     * @param max_bytes Optional maximum total size in bytes (0 = no limit)
     * @throws std::invalid_argument If max_entries is 0
     */
    explicit LRUCache(size_t max_entries, size_t max_bytes = 0);

    /**
     * @brief Destructor - releases all entries
     */
    ~LRUCache() noexcept = default;

    // Move semantics
    /**
     * @brief Move constructor
     * 
     * @param other Cache to move from
     * 
     * Transfers all entries, configuration, and callbacks to this cache.
     * `other` becomes moved-from state (safe for destruction/reassignment).
     * 
     * @post other.is_moved_from() == true
     */
    LRUCache(LRUCache&& other) noexcept;

    /**
     * @brief Move assignment operator
     * 
     * @param other Cache to move from
     * @return Reference to this cache
     * 
     * Release-and-acquire:
     * - Clears current cache contents
     * - Acquires all entries from `other`
     * - `other` becomes moved-from state
     * 
     * @post other.is_moved_from() == true
     */
    LRUCache& operator=(LRUCache&& other) noexcept;

    // No copy
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;

    // --- Cache operations ---

    /**
     * @brief Insert or update cache entry
     * 
     * @param key Cache key
     * @param value Value to cache (moved if LRUCache owns Value)
     * @return true if value inserted, false if eviction occurred
     * @throws std::logic_error If called on moved-from cache
     * 
     * If cache is full, least-recently-used entry is evicted.
     * Insertion updates LRU tracking.
     * 
     * @pre !is_moved_from()
     */
    template<typename V>
    bool insert(const Key& key, V&& value);

    /**
     * @brief Retrieve cached value
     * 
     * @param key Cache key
     * @return Cached value if found, std::nullopt otherwise
     * @throws std::logic_error If called on moved-from cache
     * 
     * Updates access tracking (moves entry to MRU position).
     * 
     * @pre !is_moved_from()
     */
    std::optional<Value> get(const Key& key);

    /**
     * @brief Retrieve without updating LRU order (peek)
     * 
     * @param key Cache key
     * @return Cached value if found, std::nullopt otherwise
     * @throws std::logic_error If called on moved-from cache
     * 
     * Does NOT update access tracking.
     */
    std::optional<const Value> peek(const Key& key) const;

    /**
     * @brief Remove specific entry
     * 
     * @param key Cache key
     * @return true if entry was found and removed, false otherwise
     * @throws std::logic_error If called on moved-from cache
     */
    bool erase(const Key& key);

    /**
     * @brief Check if key exists in cache
     * 
     * @param key Cache key
     * @return true if entry exists (does not update LRU)
     * @throws std::logic_error If called on moved-from cache
     */
    bool contains(const Key& key) const;

    /**
     * @brief Clear all entries
     * 
     * @throws std::logic_error If called on moved-from cache
     */
    void clear();

    /**
     * @brief Evict least-recently-used entry
     * 
     * @return true if entry was evicted, false if cache empty
     * @throws std::logic_error If called on moved-from cache
     */
    bool evict_lru();

    // --- Callbacks ---

    /**
     * @brief Register hit callback (called on cache hit)
     * 
     * @param callback Function to invoke on cache hit
     */
    void on_hit(HitCallback callback) { hit_callback_ = callback; }

    /**
     * @brief Register miss callback (called on cache miss)
     * 
     * @param callback Function to invoke on cache miss
     */
    void on_miss(MissCallback callback) { miss_callback_ = callback; }

    /**
     * @brief Register eviction callback (called on entry eviction)
     * 
     * @param callback Function to invoke on eviction
     */
    void on_eviction(EvictionCallback callback) { eviction_callback_ = callback; }

    // --- Statistics ---

    /**
     * @brief Get cache size (number of entries)
     * 
     * @return Number of cached entries
     */
    size_t size() const noexcept;

    /**
     * @brief Get cache capacity (maximum entries)
     * 
     * @return Maximum number of entries
     */
    size_t capacity() const noexcept;

    /**
     * @brief Get number of cache hits
     * 
     * @return Total hit count
     */
    uint64_t hits() const noexcept { return stats_.hits; }

    /**
     * @brief Get number of cache misses
     * 
     * @return Total miss count
     */
    uint64_t misses() const noexcept { return stats_.misses; }

    /**
     * @brief Get cache hit rate
     * 
     * @return Hit rate as [0.0, 1.0]
     */
    double hit_rate() const noexcept;

    /**
     * @brief Get total evictions
     * 
     * @return Eviction count
     */
    uint64_t evictions() const noexcept { return stats_.evictions; }

    /**
     * @brief Get access statistics for key
     * 
     * @param key Cache key
     * @return Entry metadata if exists, std::nullopt otherwise
     */
    std::optional<Entry> get_stats(const Key& key) const;

    // --- State ---

    /**
     * @brief Check if cache is in moved-from state
     * 
     * @return true if all resources have been moved out
     */
    bool is_moved_from() const noexcept { return is_moved_from_; }

    /**
     * @brief Check if cache is valid (not moved-from)
     * 
     * @return true if cache is operational
     */
    bool is_valid() const noexcept { return !is_moved_from_; }

private:
    /**
     * @brief Internal statistics
     */
    struct Stats {
        uint64_t hits = 0;
        uint64_t misses = 0;
        uint64_t evictions = 0;
    };

    /// LRU list (front = MRU, back = LRU)
    std::list<Key> lru_list_;

    /// Map from key to list iterator and cached value
    std::unordered_map<Key, std::pair<typename std::list<Key>::iterator, Value>> map_;

    size_t max_entries_;
    size_t max_bytes_;
    size_t bytes_used_ = 0;

    Stats stats_;
    HitCallback hit_callback_;
    MissCallback miss_callback_;
    EvictionCallback eviction_callback_;

    bool is_moved_from_ = false;

    /// Move item to MRU position (front of LRU list)
    void mark_accessed(const Key& key);

    /// Check if adding value would exceed size limits
    bool would_exceed_limits(size_t value_size) const noexcept;
};

} // namespace cache
} // namespace themis

#include "lru_cache.hpp"  // Template implementation
