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

template <typename K, typename V>
class GraphMultiTierCache {
public:
    struct Config {
        size_t hot_capacity  = 64;   ///< Maximum Hot-tier entries
        size_t warm_capacity = 256;  ///< Maximum Warm-tier entries
        size_t cold_capacity = 1024; ///< Maximum Cold-tier entries
    };

    struct Metrics {
        uint64_t hot_hits   = 0; ///< Hits served from Hot tier
        uint64_t warm_hits  = 0; ///< Hits served from Warm tier
        uint64_t cold_hits  = 0; ///< Hits served from Cold tier
        uint64_t misses     = 0; ///< Total cache misses
        uint64_t evictions  = 0; ///< Entries evicted from Cold tier
        uint64_t inserts    = 0; ///< Total entries inserted

        double hitRatio() const {
            const uint64_t hits  = hot_hits + warm_hits + cold_hits;
            const uint64_t total = hits + misses;
            return total > 0 ? static_cast<double>(hits) / static_cast<double>(total) : 0.0;
        }

        uint64_t totalLookups() const { return hot_hits + warm_hits + cold_hits + misses; }
    };

    explicit GraphMultiTierCache(Config cfg = Config{}) : cfg_(cfg) {}

    /**
     * @brief Get.
     * @param[in] key Input parameter.
     * @return Return value.
     * @details Calls: lock(), find(), end(), promote(), evictFromTier(), insertToTier().
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
     * @brief Put.
     * @param[in] key Input parameter.
     * @param[in] value Input parameter.
     * @details Calls: lock(), removeFromAll(), insertToTier(), std::move().
     */
    void put(const K& key, V value) {
        std::lock_guard<std::mutex> lock(mutex_);
        removeFromAll(key);
        insertToTier(cold_, cfg_.cold_capacity, key, std::move(value), nullptr);
        ++metrics_.inserts;
    }

    /**
     * @brief Remove.
     * @param[in] key Input parameter.
     * @return True when the operation succeeds.
     * @details Calls: lock(), removeFromAll().
     */
    bool remove(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        return removeFromAll(key);
    }

    /**
     * @brief Clear.
     * @details Calls: lock().
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        hot_.map.clear();  hot_.lru.clear();
        warm_.map.clear(); warm_.lru.clear();
        cold_.map.clear(); cold_.lru.clear();
    }

    size_t size() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return hot_.map.size() + warm_.map.size() + cold_.map.size();
    }

    size_t hotSize() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return hot_.map.size();
    }

    size_t warmSize() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return warm_.map.size();
    }

    size_t coldSize() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return cold_.map.size();
    }

    Metrics metrics() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return metrics_;
    }

    /**
     * @brief Reconfigure.
     * @param[in] cfg Input parameter.
     * @details Calls: lock(), trimTier().
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

    /**
     * @brief Insert into a tier, demoting the LRU entry to `demote_to` if at capacity.
     * @param[in,out] tier Input/output parameter.
     * @param[in] capacity Input parameter.
     * @param[in] key Input parameter.
     * @param[in] value Input parameter.
     * @param[in,out] demote_to Input/output parameter.
     * @details If `demote_to` is nullptr the evicted entry is discarded. Calls: size(), back(), std::move(), at(), push_front(), emplace(), begin(), erase().
     */
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

    /**
     * @brief Remove key from all tiers.
     * @param[in] key Input parameter.
     * @return True when the operation succeeds.
     * @details Returns true if found anywhere. Calls: find(), end(), erase().
     */
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

    /**
     * @brief Trim a tier to at most `capacity` entries by evicting LRU entries.
     * @param[in,out] tier Input/output parameter.
     * @param[in] capacity Input parameter.
     * @details Calls: size(), back(), erase(), pop_back().
     */
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

class GraphTraversalResultCache {
public:
    using ResultType = std::vector<std::string>;

    explicit GraphTraversalResultCache(size_t hot_cap  = 64,
                                       size_t warm_cap = 256,
                                       size_t cold_cap = 1024)
        : cache_(GraphMultiTierCache<std::string, ResultType>::Config{
              hot_cap, warm_cap, cold_cap}) {}

    static std::string bfsKey(const std::string& start_vertex,
                               int                max_depth,
                               const std::string& edge_type = "") {
        return "bfs:" + start_vertex + ":" + std::to_string(max_depth) +
               (edge_type.empty() ? "" : ":" + edge_type);
    }

    /**
     * @brief Shortest Path Key.
     * @param[in] from Input parameter.
     * @param[in] to Input parameter.
     * @return Return value.
     * @details Implements shortestPathKey without additional internal calls.
     */
    static std::string shortestPathKey(const std::string& from,
                                        const std::string& to) {
        return "sp:" + from + "->" + to;
    }

    /**
     * @brief Put.
     * @param[in] key Input parameter.
     * @param[in] result Input parameter.
     * @details Calls: std::move().
     */
    void put(const std::string& key, ResultType result) {
        cache_.put(key, std::move(result));
    }

    /**
     * @brief Get.
     * @param[in] key Input parameter.
     * @return Return value.
     * @details Implements get without additional internal calls.
     */
    std::optional<ResultType> get(const std::string& key) {
        return cache_.get(key);
    }

    /**
     * @brief Invalidate.
     * @param[in] key Input parameter.
     * @return True when the operation succeeds.
     * @details Calls: remove().
     */
    bool invalidate(const std::string& key) { return cache_.remove(key); }

    /**
     * @brief Clear.
     * @details Implements clear without additional internal calls.
     */
    void clear() { cache_.clear(); }

    size_t size() const { return cache_.size(); }

    auto metrics() const { return cache_.metrics(); }

private:
    GraphMultiTierCache<std::string, ResultType> cache_;
};

} // namespace graph
} // namespace themis
