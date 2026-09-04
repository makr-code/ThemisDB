/**
 * @file graph_cache_manager.h
 * @brief Phase-3 multi-tier cache manager for graph traversal results.
 *
 * Provides:
 *  - @ref themis::graph::GraphMultiTierCache   – Hot/Warm/Cold three-tier cache
 *  - @ref themis::graph::GraphTraversalResultCache – Typed cache for BFS/DFS results
 *
 * Design goals:
 *  - Cache hit ratio ≥ 85% on repeated traversal workloads.
 *  - LRU eviction within each tier.
 *  - Automatic promotion (Cold → Warm → Hot) on repeated access.
 *  - Thread-safe for concurrent readers/writers.
 *
 * @version 1.9.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace graph {

// ─────────────────────────────────────────────────────────────────────────────
// GraphMultiTierCache
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Three-tier LRU cache implementing Hot / Warm / Cold storage layers.
 *
 * On lookup:
 *  - Hot hit   → promote to MRU in Hot tier.
 *  - Warm hit  → promote to Hot tier (if space permits).
 *  - Cold hit  → promote to Warm tier.
 *
 * On insert, all new entries enter the Cold tier.  When a tier reaches
 * capacity the LRU entry is demoted to the next-colder tier (or evicted if
 * already in Cold).
 *
 * @tparam K Key type (must be hashable and equality-comparable).
 * @tparam V Value type (must be copyable and movable).
 */
template <typename K, typename V>
class GraphMultiTierCache {
public:
    /**
     * @brief Per-tier size limits and overall metrics.
     */
    struct Config {
        size_t hot_capacity  = 64;   ///< Maximum Hot-tier entries
        size_t warm_capacity = 256;  ///< Maximum Warm-tier entries
        size_t cold_capacity = 1024; ///< Maximum Cold-tier entries
    };

    /**
     * @brief Cumulative access counters.
     */
    struct Metrics {
        uint64_t hot_hits   = 0; ///< Hits served from Hot tier
        uint64_t warm_hits  = 0; ///< Hits served from Warm tier
        uint64_t cold_hits  = 0; ///< Hits served from Cold tier
        uint64_t misses     = 0; ///< Total cache misses
        uint64_t evictions  = 0; ///< Entries evicted from Cold tier
        uint64_t inserts    = 0; ///< Total entries inserted

        /**
         * @brief Overall hit ratio across all tiers.
         * @return Hit ratio in [0.0, 1.0].
         */
        double hitRatio() const {
            const uint64_t hits  = hot_hits + warm_hits + cold_hits;
            const uint64_t total = hits + misses;
            return total > 0 ? static_cast<double>(hits) / static_cast<double>(total) : 0.0;
        }

        /**
         * @brief Total lookups (hits + misses).
         * @return Lookup count.
         */
        uint64_t totalLookups() const { return hot_hits + warm_hits + cold_hits + misses; }
    };

    /**
     * @brief Construct a multi-tier cache with the given configuration.
     * @param cfg Per-tier capacity limits.
     */
    explicit GraphMultiTierCache(Config cfg = Config{}) : cfg_(cfg) {}

    /**
     * @brief Look up a value by key.
     *
     * Promotes the entry to a warmer tier on hit.
     *
     * @param key Cache key.
     * @return Cached value, or std::nullopt on miss.
     */
    std::optional<V> get(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        // Hot tier
        auto it = hot_.map.find(key);
        if (it != hot_.map.end()) {
            promote(hot_, key, it);
            ++metrics_.hot_hits;
            return it->second.value;
        }
        // Warm tier
        auto wit = warm_.map.find(key);
        if (wit != warm_.map.end()) {
            V value = wit->second.value;
            evictFromTier(warm_, wit);
            insertToTier(hot_, cfg_.hot_capacity, key, value, &warm_);
            ++metrics_.warm_hits;
            return value;
        }
        // Cold tier
        auto cit = cold_.map.find(key);
        if (cit != cold_.map.end()) {
            V value = cit->second.value;
            evictFromTier(cold_, cit);
            insertToTier(warm_, cfg_.warm_capacity, key, value, &cold_);
            ++metrics_.cold_hits;
            return value;
        }
        ++metrics_.misses;
        return std::nullopt;
    }

    /**
     * @brief Insert a key-value pair (always enters Cold tier).
     *
     * If the key already exists in any tier it is removed first.
     *
     * @param key   Cache key.
     * @param value Value to store.
     */
    void put(const K& key, V value) {
        std::lock_guard<std::mutex> lock(mutex_);
        removeFromAll(key);
        insertToTier(cold_, cfg_.cold_capacity, key, std::move(value), nullptr);
        ++metrics_.inserts;
    }

    /**
     * @brief Remove a key from whichever tier it resides in.
     * @param key Cache key.
     * @return true if the key was found and removed.
     */
    bool remove(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        return removeFromAll(key);
    }

    /// Clear all three tiers.
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        hot_.map.clear();  hot_.lru.clear();
        warm_.map.clear(); warm_.lru.clear();
        cold_.map.clear(); cold_.lru.clear();
    }

    /**
     * @brief Return the total number of live entries across all tiers.
     * @return Entry count.
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return hot_.map.size() + warm_.map.size() + cold_.map.size();
    }

    /**
     * @brief Return the number of entries in the Hot tier.
     * @return Hot-tier entry count.
     */
    size_t hotSize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return hot_.map.size();
    }

    /**
     * @brief Return the number of entries in the Warm tier.
     * @return Warm-tier entry count.
     */
    size_t warmSize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return warm_.map.size();
    }

    /**
     * @brief Return the number of entries in the Cold tier.
     * @return Cold-tier entry count.
     */
    size_t coldSize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cold_.map.size();
    }

    /**
     * @brief Return a snapshot of current access metrics.
     * @return Metrics snapshot.
     */
    Metrics metrics() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return metrics_;
    }

    /**
     * @brief Update tier capacities at runtime.
     *
     * Existing entries that now exceed the new capacity are evicted (LRU).
     *
     * @param cfg New configuration.
     */
    void reconfigure(Config cfg) {
        std::lock_guard<std::mutex> lock(mutex_);
        cfg_ = cfg;
        trimTier(hot_,  cfg_.hot_capacity);
        trimTier(warm_, cfg_.warm_capacity);
        trimTier(cold_, cfg_.cold_capacity);
    }

private:
    struct Entry {
        V value;
        typename std::list<K>::iterator lru_it;
    };

    struct Tier {
        std::list<K> lru;
        std::unordered_map<K, Entry> map;
    };

    // Promote key to MRU within the same tier (already under lock).
    void promote(Tier& tier, const K& key,
                 typename std::unordered_map<K, Entry>::iterator it) {
        tier.lru.erase(it->second.lru_it);
        tier.lru.push_front(key);
        it->second.lru_it = tier.lru.begin();
    }

    // Evict a specific entry from a tier (already under lock).
    void evictFromTier(Tier& tier,
                       typename std::unordered_map<K, Entry>::iterator it) {
        tier.lru.erase(it->second.lru_it);
        tier.map.erase(it);
    }

    // Insert into a tier, demoting the LRU entry to `demote_to` if at
    // capacity.  If `demote_to` is nullptr the evicted entry is discarded.
    void insertToTier(Tier& tier, size_t capacity, const K& key, V value,
                      Tier* demote_to) {
        if (capacity > 0 && tier.map.size() >= capacity) {
            // Demote LRU entry
            const K& victim = tier.lru.back();
            if (demote_to) {
                // Move victim to cooler tier without recursing into insertToTier
                // to avoid double-lock; capacity limit for demote_to is soft here.
                V victim_val = std::move(tier.map.at(victim).value);
                demote_to->lru.push_front(victim);
                demote_to->map.emplace(victim, Entry{std::move(victim_val),
                                                     demote_to->lru.begin()});
            } else {
                ++metrics_.evictions;
            }
            tier.map.erase(victim);
            tier.lru.pop_back();
        }
        tier.lru.push_front(key);
        tier.map.emplace(key, Entry{std::move(value), tier.lru.begin()});
    }

    // Remove key from all tiers. Returns true if found anywhere.
    bool removeFromAll(const K& key) {
        for (Tier* t : {&hot_, &warm_, &cold_}) {
            auto it = t->map.find(key);
            if (it != t->map.end()) {
                t->lru.erase(it->second.lru_it);
                t->map.erase(it);
                return true;
            }
        }
        return false;
    }

    // Trim a tier to at most `capacity` entries by evicting LRU entries.
    void trimTier(Tier& tier, size_t capacity) {
        if (capacity == 0) {
          return;
        }
        while (tier.map.size() > capacity) {
            const K& victim = tier.lru.back();
            tier.map.erase(victim);
            tier.lru.pop_back();
            ++metrics_.evictions;
        }
    }

    mutable std::mutex mutex_;
    Config cfg_;
    Tier   hot_;
    Tier   warm_;
    Tier   cold_;
    Metrics metrics_;
};

// ─────────────────────────────────────────────────────────────────────────────
// GraphTraversalResultCache
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Typed multi-tier cache for graph traversal results.
 *
 * Stores lists of vertex IDs keyed by a query descriptor string.
 * Provides convenience methods for building canonical cache keys.
 */
class GraphTraversalResultCache {
public:
    using ResultType = std::vector<std::string>;

    /**
     * @brief Construct with the given per-tier capacity limits.
     *
     * @param hot_cap  Hot-tier capacity (default 64).
     * @param warm_cap Warm-tier capacity (default 256).
     * @param cold_cap Cold-tier capacity (default 1024).
     */
    explicit GraphTraversalResultCache(size_t hot_cap  = 64,
                                       size_t warm_cap = 256,
                                       size_t cold_cap = 1024)
        : cache_(GraphMultiTierCache<std::string, ResultType>::Config{
              hot_cap, warm_cap, cold_cap}) {}

    /**
     * @brief Build a canonical BFS cache key.
     *
     * @param start_vertex  BFS starting vertex.
     * @param max_depth     Maximum traversal depth.
     * @param edge_type     Optional edge type filter ("" = any).
     * @return Canonical string key.
     */
    static std::string bfsKey(const std::string& start_vertex,
                               int                max_depth,
                               const std::string& edge_type = "") {
        return "bfs:" + start_vertex + ":" + std::to_string(max_depth) +
               (edge_type.empty() ? "" : ":" + edge_type);
    }

    /**
     * @brief Build a canonical shortest-path cache key.
     *
     * @param from Source vertex.
     * @param to   Target vertex.
     * @return Canonical string key.
     */
    static std::string shortestPathKey(const std::string& from,
                                        const std::string& to) {
        return "sp:" + from + "->" + to;
    }

    /**
     * @brief Store a traversal result.
     *
     * @param key    Cache key (use bfsKey / shortestPathKey helpers).
     * @param result List of vertex IDs from the traversal.
     */
    void put(const std::string& key, ResultType result) {
        cache_.put(key, std::move(result));
    }

    /**
     * @brief Look up a cached traversal result.
     *
     * @param key Cache key.
     * @return Result if cached, std::nullopt otherwise.
     */
    std::optional<ResultType> get(const std::string& key) {
        return cache_.get(key);
    }

    /**
     * @brief Invalidate a specific cache entry.
     * @param key Cache key to remove.
     * @return true if the entry was present and removed.
     */
    bool invalidate(const std::string& key) { return cache_.remove(key); }

    /// Clear the entire cache.
    void clear() { cache_.clear(); }

    /**
     * @brief Return the total number of cached entries.
     * @return Entry count.
     */
    size_t size() const { return cache_.size(); }

    /**
     * @brief Return current access metrics.
     * @return Metrics snapshot.
     */
    auto metrics() const { return cache_.metrics(); }

private:
    GraphMultiTierCache<std::string, ResultType> cache_;
};

} // namespace graph
} // namespace themis
