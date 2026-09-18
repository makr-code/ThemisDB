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
 * @see ThemisDB Cache Module Roadmap: src/cache/ROADMAP.md
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

template <typename Key, typename Value>
class LRUCache {
public:
    struct Entry {
        Key key;                   ///< Cache key
        Value value;               ///< Cached value
        uint64_t access_count = 0; ///< Number of accesses
        int64_t timestamp_us = 0;  ///< Last access time in microseconds
    };

    using HitCallback = std::function<void(const Key&, const Value&)>;
    using MissCallback = std::function<void(const Key&)>;
    using EvictionCallback = std::function<void(const Key&, const Value&)>;

    explicit LRUCache(size_t max_entries, size_t max_bytes = 0);

    ~LRUCache() noexcept = default;

    // Move semantics
    LRUCache(LRUCache&& other) noexcept;

    LRUCache& operator=(LRUCache&& other) noexcept;

    // No copy
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;

    // --- Cache operations ---

    template<typename V>
    /**
     * @brief Insert.
     * @param[in] key Input parameter.
     * @param[in] value Input parameter.
     * @return True when the operation succeeds.
     */
    bool insert(const Key& key, V&& value);

    /**
     * @brief Get.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    std::optional<Value> get(const Key& key);

    /**
     * @brief Peek.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    std::optional<const Value> peek(const Key& key) const;

    /**
     * @brief Erase.
     * @param[in] key Input parameter.
     * @return True when the operation succeeds.
     */
    bool erase(const Key& key);

    /**
     * @brief Contains.
     * @param[in] key Input parameter.
     * @return True when the operation succeeds.
     */
    bool contains(const Key& key) const;

    /**
     * @brief Clear.
     */
    void clear();

    /**
     * @brief Evict lru.
     * @return True when the operation succeeds.
     */
    bool evict_lru();

    /**
     * @brief --- Callbacks ---
     * @param[in] callback Input parameter.
     * @details Implements on_hit without additional internal calls.
     */

    void on_hit(HitCallback callback) { hit_callback_ = callback; }

    /**
     * @brief On miss.
     * @param[in] callback Input parameter.
     * @details Implements on_miss without additional internal calls.
     */
    void on_miss(MissCallback callback) { miss_callback_ = callback; }

    /**
     * @brief On eviction.
     * @param[in] callback Input parameter.
     * @details Implements on_eviction without additional internal calls.
     */
    void on_eviction(EvictionCallback callback) { eviction_callback_ = callback; }

    /**
     * @brief --- Statistics ---
     * @return Return value.
     * @note Exception safety: noexcept.
     */

    size_t size() const noexcept;

    /**
     * @brief Capacity.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    size_t capacity() const noexcept;

    uint64_t hits() const noexcept { return stats_.hits; }

    uint64_t misses() const noexcept { return stats_.misses; }

    /**
     * @brief Hit rate.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    double hit_rate() const noexcept;

    uint64_t evictions() const noexcept { return stats_.evictions; }

    /**
     * @brief Get stats.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    std::optional<Entry> get_stats(const Key& key) const;

    // --- State ---

    bool is_moved_from() const noexcept { return is_moved_from_; }

    bool is_valid() const noexcept { return !is_moved_from_; }

private:
    struct Stats {
        uint64_t hits = 0;
        uint64_t misses = 0;
        uint64_t evictions = 0;
    };

    std::list<Key> lru_list_;

    std::unordered_map<Key, std::pair<typename std::list<Key>::iterator, Value>> map_;

    size_t max_entries_;
    size_t max_bytes_;
    size_t bytes_used_ = 0;

    Stats stats_;
    HitCallback hit_callback_;
    MissCallback miss_callback_;
    EvictionCallback eviction_callback_;

    bool is_moved_from_ = false;

    /**
     * @brief Mark accessed.
     * @param[in] key Input parameter.
     */
    void mark_accessed(const Key& key);

    /**
     * @brief Would exceed limits.
     * @param[in] value_size Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool would_exceed_limits(size_t value_size) const noexcept;
};

} // namespace cache
} // namespace themis

#include "lru_cache.hpp"  // Template implementation
