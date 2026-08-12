/**
 * @file lru_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * O(1) LRU cache with a configurable capacity.
 *
 * @tparam K  Key type — must satisfy std::hash and operator==.
 * @tparam V  Value type — must be move-constructible.
 */
template <typename K, typename V>
class LRUCache {
public:
    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @param max_entries  Maximum number of entries.  Must be ≥ 1.
     * @throws std::invalid_argument if max_entries == 0.
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
    LRUCache(LRUCache&&)                 = default;
    LRUCache& operator=(LRUCache&&)      = default;

    // -----------------------------------------------------------------------
    // Core API
    // -----------------------------------------------------------------------

    /**
     * Look up @p key.
     *
     * On a cache hit the entry is promoted to MRU position.
     *
     * @return Pointer to the cached value, or nullptr on a miss.
     *         The pointer is valid until the next non-const operation.
     */
    V* get(const K& key) {
        auto it = map_.find(key);
        if (it == map_.end()) return nullptr;
        // Splice to front (O(1) for std::list iterators).
        list_.splice(list_.begin(), list_, it->second);
        return &it->second->second;
    }

    /** Const overload — does NOT update the access order. */
    const V* peek(const K& key) const {
        auto it = map_.find(key);
        if (it == map_.end()) return nullptr;
        return &it->second->second;
    }

    /**
     * Insert or update a key-value pair.
     *
     * If @p key is already present the value is updated in-place and the
     * entry is promoted to MRU.  If the cache is at capacity the LRU entry
     * is evicted first.
     *
     * @param key    Cache key.
     * @param value  Value to cache (moved in).
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
     * Remove the entry for @p key if it exists.
     *
     * @return true if the entry was found and removed, false otherwise.
     */
    bool erase(const K& key) {
        auto it = map_.find(key);
        if (it == map_.end()) return false;
        list_.erase(it->second);
        map_.erase(it);
        return true;
    }

    /** Remove all entries. */
    void clear() {
        list_.clear();
        map_.clear();
    }

    // -----------------------------------------------------------------------
    // Capacity / size queries
    // -----------------------------------------------------------------------

    /** Number of entries currently in the cache. */
    [[nodiscard]] std::size_t size()  const noexcept { return map_.size(); }

    /** Maximum number of entries (supplied at construction). */
    [[nodiscard]] std::size_t capacity() const noexcept { return max_entries_; }

    /** Returns true iff the cache contains no entries. */
    [[nodiscard]] bool empty() const noexcept { return map_.empty(); }

    /** Returns true iff the cache is at capacity. */
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
