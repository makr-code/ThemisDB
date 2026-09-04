/**
 * @file lru_cache.hpp
 * @brief LRU cache template implementation
 * @version 0.1.0
 */

#ifndef THEMIS_CACHE_LRU_CACHE_HPP
#define THEMIS_CACHE_LRU_CACHE_HPP

#include "lru_cache.h"
#include <stdexcept>
#include <chrono>
#include <algorithm>

namespace themis {
namespace cache {

// =============================================================================
// LRUCache Template Implementation
// =============================================================================

template <typename Key, typename Value>
LRUCache<Key, Value>::LRUCache(size_t max_entries, size_t max_bytes)
    : max_entries_(max_entries), 
      max_bytes_(max_bytes),
      is_moved_from_(false) {
    
    if (max_entries == 0) {
        throw std::invalid_argument("max_entries must be > 0");
    }
}

template <typename Key, typename Value>
LRUCache<Key, Value>::LRUCache(LRUCache&& other) noexcept
    : lru_list_(std::move(other.lru_list_)),
      map_(std::move(other.map_)),
      max_entries_(other.max_entries_),
      max_bytes_(other.max_bytes_),
      bytes_used_(other.bytes_used_),
      stats_(other.stats_),
      hit_callback_(std::move(other.hit_callback_)),
      miss_callback_(std::move(other.miss_callback_)),
      eviction_callback_(std::move(other.eviction_callback_)),
      is_moved_from_(false) {
    
    other.is_moved_from_ = true;
    other.bytes_used_ = 0;
    other.stats_ = Stats{};
}

template <typename Key, typename Value>
LRUCache<Key, Value>& LRUCache<Key, Value>::operator=(LRUCache&& other) noexcept {
    if (this == &other || other.is_moved_from_) {
        return *this;
    }

    clear();

    lru_list_ = std::move(other.lru_list_);
    map_ = std::move(other.map_);
    max_entries_ = other.max_entries_;
    max_bytes_ = other.max_bytes_;
    bytes_used_ = other.bytes_used_;
    stats_ = other.stats_;
    hit_callback_ = std::move(other.hit_callback_);
    miss_callback_ = std::move(other.miss_callback_);
    eviction_callback_ = std::move(other.eviction_callback_);
    is_moved_from_ = false;

    other.is_moved_from_ = true;
    other.bytes_used_ = 0;
    other.stats_ = Stats{};

    return *this;
}

template <typename Key, typename Value>
template <typename V>
bool LRUCache<Key, Value>::insert(const Key& key, V&& value) {
    if (is_moved_from_) {
        throw std::logic_error("Cannot insert into moved-from cache");
    }

    auto it = map_.find(key);
    if (it != map_.end()) {
        // Update existing entry
        mark_accessed(key);
        return true;
    }

    // Check if new entry would exceed limits
    if (map_.size() >= max_entries_) {
        if (!evict_lru()) {
            return false;
        }
    }

    // Insert new entry
    lru_list_.push_front(key);
    map_[key] = {lru_list_.begin(), std::forward<V>(value)};

    return true;
}

template <typename Key, typename Value>
std::optional<Value> LRUCache<Key, Value>::get(const Key& key) {
    if (is_moved_from_) {
        throw std::logic_error("Cannot get from moved-from cache");
    }

    auto it = map_.find(key);
    if (it != map_.end()) {
        if (hit_callback_) {
            hit_callback_(key, it->second.second);
        }
        stats_.hits++;
        mark_accessed(key);
        return it->second.second;
    }

    if (miss_callback_) {
        miss_callback_(key);
    }
    stats_.misses++;
    return std::nullopt;
}

template <typename Key, typename Value>
std::optional<const Value> LRUCache<Key, Value>::peek(const Key& key) const {
    auto it = map_.find(key);
    if (it != map_.end()) {
        return it->second.second;
    }
    return std::nullopt;
}

template <typename Key, typename Value>
bool LRUCache<Key, Value>::erase(const Key& key) {
    if (is_moved_from_) {
        throw std::logic_error("Cannot erase from moved-from cache");
    }

    auto it = map_.find(key);
    if (it != map_.end()) {
        lru_list_.erase(it->second.first);
        map_.erase(it);
        return true;
    }
    return false;
}

template <typename Key, typename Value>
bool LRUCache<Key, Value>::contains(const Key& key) const {
    return map_.find(key) != map_.end();
}

template <typename Key, typename Value>
void LRUCache<Key, Value>::clear() {
    if (is_moved_from_) {
        throw std::logic_error("Cannot clear moved-from cache");
    }

    lru_list_.clear();
    map_.clear();
    bytes_used_ = 0;
}

template <typename Key, typename Value>
bool LRUCache<Key, Value>::evict_lru() {
    if (is_moved_from_) {
        throw std::logic_error("Cannot evict from moved-from cache");
    }

    if (lru_list_.empty()) {
        return false;
    }

    const Key& lru_key = lru_list_.back();
    auto it = map_.find(lru_key);
    if (it != map_.end()) {
        if (eviction_callback_) {
            eviction_callback_(lru_key, it->second.second);
        }
        stats_.evictions++;
        lru_list_.pop_back();
        map_.erase(it);
        return true;
    }

    return false;
}

template <typename Key, typename Value>
size_t LRUCache<Key, Value>::size() const noexcept {
    return is_moved_from_ ? 0 : map_.size();
}

template <typename Key, typename Value>
size_t LRUCache<Key, Value>::capacity() const noexcept {
    return is_moved_from_ ? 0 : max_entries_;
}

template <typename Key, typename Value>
double LRUCache<Key, Value>::hit_rate() const noexcept {
    uint64_t total = stats_.hits + stats_.misses;
    if (total == 0) {
      return 0.0;
    }
    return static_cast<double>(stats_.hits) / static_cast<double>(total);
}

template <typename Key, typename Value>
std::optional<typename LRUCache<Key, Value>::Entry> LRUCache<Key, Value>::get_stats(const Key& key) const {
    auto it = map_.find(key);
    if (it != map_.end()) {
        Entry entry;
        entry.key = key;
        entry.value = it->second.second;
        // Would need to track access_count and timestamp separately
        return entry;
    }
    return std::nullopt;
}

template <typename Key, typename Value>
void LRUCache<Key, Value>::mark_accessed(const Key& key) {
    auto it = map_.find(key);
    if (it != map_.end()) {
        // Move to front (MRU position)
        lru_list_.splice(lru_list_.begin(), lru_list_, it->second.first);
    }
}

template <typename Key, typename Value>
bool LRUCache<Key, Value>::would_exceed_limits(size_t value_size) const noexcept {
    if (map_.size() >= max_entries_) {
        return true;
    }
    if (max_bytes_ > 0 && (bytes_used_ + value_size) > max_bytes_) {
        return true;
    }
    return false;
}

} // namespace cache
} // namespace themis

#endif  // THEMIS_CACHE_LRU_CACHE_HPP
