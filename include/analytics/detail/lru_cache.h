/**
 * @file lru_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB Analytics – Generic O(1) LRU Cache
 *
 * A doubly-linked list + unordered_map implementation providing O(1)
 * amortised get, put, and eviction.  Suitable for use in any analytics
 * sub-component (OLAP result caching, LLM-response caching, etc.).
 *
 * Design:
 *   - The doubly-linked list records access order: MRU at the front, LRU at
 *     the back.
 *   - The hash map provides O(1) lookup of list iterators by key.
 *   - On every successful get() the accessed node is spliced to the front.
 *   - On put() when at capacity, the back node (LRU) is evicted first.
 *
 * Thread-safety: NOT thread-safe.  Protect with an external mutex when used
 * from multiple threads (see LLMProcessAnalyzer for an example that wraps
 * this type behind a std::mutex).
 *
 * Template parameters:
 *   K – key type (must be hashable and equality-comparable).
 *   V – value type (must be move-constructible).
 *
 * Usage:
 *   LRUCache<std::string, OLAPResult> cache(1000 /\* max entries \*/);
 *   cache.put("key", result);
 *   if (auto* v = cache.get("key")) { ... use *v ... }
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>
#include <list>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace themis::analytics::detail {

template <typename K, typename V>
class LRUCache {
public:
    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief LRUCache.
     * @param[in] max_entries Input parameter.
     * @return Return value.
     */
    explicit LRUCache(std::size_t max_entries)
        : max_entries_(max_entries) {
        if (max_entries == 0)
            throw std::invalid_argument("LRUCache: max_entries must be >= 1");
        map_.reserve(max_entries + 1); // avoid rehash on insert-then-evict
    }

    // -----------------------------------------------------------------------
    // Non-copyable, moveable
    // -----------------------------------------------------------------------
    LRUCache(const LRUCache&)            = delete;
    LRUCache& operator=(const LRUCache&) = delete;
    LRUCache(LRUCache&&)                 noexcept = default;
    LRUCache& operator=(LRUCache&&)      noexcept = default;

    // -----------------------------------------------------------------------
    // Core API
    // -----------------------------------------------------------------------

    /**
     * @brief Get.
     * @param[in] key Input parameter.
     * @return Pointer to the result.
     * @details Calls: find(), end(), splice(), begin().
     */
    V* get(const K& key) {
        auto it = map_.find(key);
        if (it == map_.end()) {
          return nullptr;
        }
        // Splice to front (O(1) for std::list iterators).
        list_.splice(list_.begin(), list_, it->second);
        return &it->second->second;
    }

    const V* peek(const K& key) const {
        auto it = map_.find(key);
        if (it == map_.end()) {
          return nullptr;
        }
        return &it->second->second;
    }

    /**
     * @brief Put.
     * @param[in] key Input parameter.
     * @param[in] value Input parameter.
     * @details Calls: find(), end(), std::move(), splice(), begin(), size(), erase(), back().
     */
    void put(K key, V value) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            // Update existing entry and promote to MRU.
            it->second->second = std::move(value);
            list_.splice(list_.begin(), list_, it->second);
            return;
        }
        // Evict LRU entry if at capacity.
        if (map_.size() >= max_entries_) {
            map_.erase(list_.back().first);
            list_.pop_back();
        }
        // Insert at front (MRU position).
        list_.emplace_front(key, std::move(value));
        map_.emplace(std::move(key), list_.begin());
    }

    /**
     * @brief Erase.
     * @param[in] key Input parameter.
     * @return True when the operation succeeds.
     * @details Calls: find(), end().
     */
    bool erase(const K& key) {
        auto it = map_.find(key);
        if (it == map_.end()) {
          return false;
        }
        list_.erase(it->second);
        map_.erase(it);
        return true;
    }

    /**
     * @brief Clear.
     * @details Implements clear without additional internal calls.
     */
    void clear() {
        list_.clear();
        map_.clear();
    }

    // -----------------------------------------------------------------------
    // Capacity / size queries
    // -----------------------------------------------------------------------

    [[nodiscard]] std::size_t size()  const noexcept { return map_.size(); }

    [[nodiscard]] std::size_t capacity() const noexcept { return max_entries_; }

    [[nodiscard]] bool empty() const noexcept { return map_.empty(); }

    [[nodiscard]] bool full()  const noexcept { return map_.size() >= max_entries_; }

private:
    // List node: pair<key, value>; front = MRU, back = LRU.
    using ListType = std::list<std::pair<K, V>>;
    using IterType = typename ListType::iterator;

    const std::size_t                       max_entries_;
    ListType                                list_;
    std::unordered_map<K, IterType>         map_;
};

} // namespace themis::analytics::detail
