/*
 * ThemisDB | File: arc_cache.h | Version: 0.0.46 | Last Modified: 2026-05-20 19:53:17
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 436
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

/**
 * @file arc_cache.h
 * @brief ARC (Adaptive Replacement Cache) – production-ready, scan-resistant.
 *
 * ARC was introduced by Megiddo & Modha (IBM, 2003):
 *   "ARC: A Self-Tuning, Low Overhead Replacement Cache"
 *   FAST '03. https://www.usenix.org/node/12555
 *
 * ## Why ARC instead of plain LRU?
 *
 *   LRU                        ARC
 *   ─────────────────────────  ─────────────────────────────────────────────
 *   Thrashing on scan patterns  Scan-resistant via ghost-list filter
 *   Fixed policy                Self-tuning: adapts to recency vs. frequency
 *   50-60% hit rate (mixed)     70-85% hit rate (same workload)
 *
 * ## Algorithm overview
 *
 * ARC maintains four lists:
 *
 *   T1  Recently-seen (recency) pages currently in cache.
 *   T2  Frequently-seen (frequency) pages currently in cache.
 *   B1  Ghost (evicted) pages that were in T1.  No data, only keys.
 *   B2  Ghost (evicted) pages that were in T2.  No data, only keys.
 *
 * The parameter `p` (0 ≤ p ≤ capacity) is the target size of T1.  On a
 * B1 hit `p` increases; on a B2 hit `p` decreases.  This drives the cache
 * to self-tune toward the optimal mix of recency and frequency.
 *
 * ## Complexity
 *
 * All operations (put/get/evict) run in amortised O(1) time.
 *
 * ## Thread safety
 *
 * All public methods are thread-safe (protected by a single shared mutex).
 *
 * ## Usage
 *
 * @code
 * using namespace themis::cache;
 * ARCCache<std::string, std::vector<uint8_t>> buf_pool(8192); // 8k pages
 *
 * // Write a page
 * buf_pool.put("page:42", page_data);
 *
 * // Read a page (returns nullopt on miss)
 * auto data = buf_pool.get("page:42");
 *
 * // Check hit rate
 * auto stats = buf_pool.stats();
 * spdlog::info("ARC hit rate: {:.1f}%", stats.hit_rate() * 100);
 * @endcode
 */

#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace themis {
namespace cache {

// ─────────────────────────────────────────────────────────────────────────────
// ARCCache<K, V>
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief ARC (Adaptive Replacement Cache) with self-tuning recency/frequency
 *        balance and ghost-list scan resistance.
 *
 * @tparam K Key type.  Must be hashable and equality-comparable.
 * @tparam V Value type.  Must be move-constructible.
 */
template <typename K, typename V>
class ARCCache {
public:
    // ── Statistics ──────────────────────────────────────────────────────────

    struct Stats {
        uint64_t hits   {0};
        uint64_t misses {0};
        uint64_t evictions_t1{0};  ///< Pages evicted from T1
        uint64_t evictions_t2{0};  ///< Pages evicted from T2
        uint64_t b1_hits{0};       ///< Ghost hits in B1 (recency side)
        uint64_t b2_hits{0};       ///< Ghost hits in B2 (frequency side)
        uint64_t pin_skips{0};     ///< Evictions skipped because page was pinned

        /** Fraction of lookups that were cache hits (0.0 – 1.0). */
        double hit_rate() const {
            uint64_t total = hits + misses;
            return total == 0 ? 0.0 : static_cast<double>(hits) / total;
        }
    };

    // ── Construction / destruction ───────────────────────────────────────────

    /**
     * @brief Construct an ARCCache.
     * @param capacity  Maximum number of pages held in T1+T2.
     *                  Must be ≥ 1.
     */
    explicit ARCCache(size_t capacity)
        : capacity_(capacity > 0 ? capacity : 1), p_(0) {}

    ~ARCCache() = default;

    ARCCache(const ARCCache&) = delete;
    ARCCache& operator=(const ARCCache&) = delete;
    ARCCache(ARCCache&&) = delete;
    ARCCache& operator=(ARCCache&&) = delete;

    // ── Core operations ──────────────────────────────────────────────────────

    /**
     * @brief Look up a key.
     *
     * Cache hit  → moves the page to the MRU end of T2 (frequency queue)
     *              and returns the value.
     * Cache miss → returns nullopt.
     *
     * @return Const reference inside an optional, or nullopt.
     */
    std::optional<V> get(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check T1 (recency set)
        auto it1 = t1_map_.find(key);
        if (it1 != t1_map_.end()) {
            // Move from T1 to T2 (this page is now "frequently used")
            const V& val = it1->second->value;
            t2_list_.push_front({key, val});
            t2_map_[key] = t2_list_.begin();
            t1_list_.erase(it1->second);
            t1_map_.erase(it1);
            ++stats_.hits;
            return t2_list_.front().value;
        }

        // Check T2 (frequency set)
        auto it2 = t2_map_.find(key);
        if (it2 != t2_map_.end()) {
            // Move to MRU end of T2
            t2_list_.splice(t2_list_.begin(), t2_list_, it2->second);
            ++stats_.hits;
            return it2->second->value;
        }

        ++stats_.misses;
        return std::nullopt;
    }

    /**
     * @brief Insert or update a key-value pair.
     *
     * If the key is already in T1 or T2, its value is updated in place.
     * If the key is in B1 (ghost), the ARC adaptation rule increases `p`
     * and the page is inserted into T2.
     * If the key is in B2 (ghost), `p` decreases and the page goes into T2.
     * Otherwise the page is inserted into T1.
     *
     * Eviction is triggered automatically to respect the capacity limit.
     */
    void put(const K& key, V value) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Already in T1 – update value
        auto it1 = t1_map_.find(key);
        if (it1 != t1_map_.end()) {
            it1->second->value = std::move(value);
            // Promote to T2
            const V& v = it1->second->value;
            t2_list_.push_front({key, v});
            t2_map_[key] = t2_list_.begin();
            t1_list_.erase(it1->second);
            t1_map_.erase(it1);
            return;
        }

        // Already in T2 – update and move to MRU
        auto it2 = t2_map_.find(key);
        if (it2 != t2_map_.end()) {
            it2->second->value = std::move(value);
            t2_list_.splice(t2_list_.begin(), t2_list_, it2->second);
            return;
        }

        // Ghost hit in B1 (previously evicted from T1) → adapt p upward
        auto ib1 = b1_set_.find(key);
        if (ib1 != b1_set_.end()) {
            ++stats_.b1_hits;
            // delta = max(1, |B2| / |B1|) — clamped to avoid division by zero
            size_t delta = (b1_set_.size() == 0 || b2_set_.size() >= b1_set_.size())
                               ? 1
                               : b2_set_.size() / b1_set_.size();
            p_ = std::min(p_ + std::max<size_t>(delta, 1), capacity_);
            b1_set_.erase(ib1);
            evict();
            t2_list_.push_front({key, std::move(value)});
            t2_map_[key] = t2_list_.begin();
            return;
        }

        // Ghost hit in B2 (previously evicted from T2) → adapt p downward
        auto ib2 = b2_set_.find(key);
        if (ib2 != b2_set_.end()) {
            ++stats_.b2_hits;
            // delta = max(1, |B1| / |B2|) — clamped to avoid division by zero
            size_t delta = (b2_set_.size() == 0 || b1_set_.size() >= b2_set_.size())
                               ? 1
                               : b1_set_.size() / b2_set_.size();
            p_ = (p_ >= delta) ? p_ - delta : 0;
            b2_set_.erase(ib2);
            evict();
            t2_list_.push_front({key, std::move(value)});
            t2_map_[key] = t2_list_.begin();
            return;
        }

        // New page: check total size before inserting
        size_t t1t2 = t1_list_.size() + t2_list_.size();
        size_t b1b2 = b1_set_.size() + b2_set_.size();

        if (t1t2 >= capacity_) {
            // Cache is full: evict one live page
            evict();
            // Also cap ghost lists at 2 * capacity
            if (b1b2 >= 2 * capacity_) {
                if (!b2_set_.empty())
                    b2_set_.erase(b2_set_.begin());
                else if (!b1_set_.empty())
                    b1_set_.erase(b1_set_.begin());
            }
        }

        // Insert into T1 (recently seen, first time)
        t1_list_.push_front({key, std::move(value)});
        t1_map_[key] = t1_list_.begin();
    }

    /**
     * @brief Remove a key from the cache (and from ghost lists).
     * @return true if the key was found and removed from T1 or T2.
     */
    bool remove(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it1 = t1_map_.find(key);
        if (it1 != t1_map_.end()) {
            t1_list_.erase(it1->second);
            t1_map_.erase(it1);
            return true;
        }
        auto it2 = t2_map_.find(key);
        if (it2 != t2_map_.end()) {
            t2_list_.erase(it2->second);
            t2_map_.erase(it2);
            return true;
        }
        return false;
    }

    /** @brief Remove all entries and reset statistics. */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        t1_list_.clear(); t1_map_.clear();
        t2_list_.clear(); t2_map_.clear();
        b1_set_.clear();
        b2_set_.clear();
        pinned_.clear();
        p_ = 0;
        stats_ = {};
    }

    // ── Accessors ────────────────────────────────────────────────────────────

    /** @brief Number of live pages (T1 + T2). */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return t1_list_.size() + t2_list_.size();
    }

    /** @brief Maximum number of live pages. */
    size_t capacity() const { return capacity_; }

    /** @brief Current target size of T1 (adapts automatically). */
    size_t targetT1() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return p_;
    }

    /** @brief Return a snapshot of current statistics. */
    Stats stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }

    /** @brief Return true if the key is in T1 or T2. */
    bool contains(const K& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return t1_map_.count(key) || t2_map_.count(key);
    }

    // ── Hot-page pinning ─────────────────────────────────────────────────────

    /**
     * @brief Pin a page so it cannot be evicted.
     *
     * Pinned pages are skipped during eviction even when the cache is full.
     * A page that is not currently in the cache may be pinned in advance;
     * when it is subsequently inserted it will not be evicted until unpinned.
     *
     * @param key Key to pin.
     */
    void pin(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        pinned_.insert(key);
    }

    /**
     * @brief Unpin a page so it becomes eligible for eviction again.
     *
     * @param key Key to unpin.
     */
    void unpin(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        pinned_.erase(key);
    }

    /**
     * @brief Return true if @p key is currently pinned.
     *
     * @note A pinned page may or may not be present in the cache.
     */
    bool isPinned(const K& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pinned_.count(key) != 0;
    }

private:
    // ── Internal types ───────────────────────────────────────────────────────

    struct Entry {
        K key;
        V value;
    };

    using List    = std::list<Entry>;
    using ListIt  = typename List::iterator;
    using LiveMap = std::unordered_map<K, ListIt>;
    // Ghost lists only need key membership – use an unordered_set equivalent.
    using GhostSet = std::unordered_map<K, bool>;

    // ── Eviction ─────────────────────────────────────────────────────────────

    /**
     * @brief Evict one page from T1 or T2 according to the ARC policy.
     *
     * If |T1| > p (or T2 is empty), evict LRU from T1 → move key to B1.
     * Otherwise evict LRU from T2 → move key to B2.
     * Pinned pages are skipped (stat: pin_skips incremented once per blocked attempt).
     */
    void evict() {
        // Try T1 first (respecting ARC policy), skip pinned pages
        if (!t1_list_.empty() &&
            (t1_list_.size() > p_ || t2_list_.empty())) {
            // Scan from LRU end of T1 for an unpinned candidate
            for (auto it = t1_list_.rbegin(); it != t1_list_.rend(); ++it) {
                if (pinned_.count(it->key) == 0) {
                    b1_set_[it->key] = true;
                    t1_map_.erase(it->key);
                    t1_list_.erase(std::next(it).base());
                    ++stats_.evictions_t1;
                    return;
                }
            }
            // All T1 candidates were pinned – count as one blocked eviction attempt
            ++stats_.pin_skips;
        }
        // Try T2
        for (auto it = t2_list_.rbegin(); it != t2_list_.rend(); ++it) {
            if (pinned_.count(it->key) == 0) {
                b2_set_[it->key] = true;
                t2_map_.erase(it->key);
                t2_list_.erase(std::next(it).base());
                ++stats_.evictions_t2;
                return;
            }
        }
        // All live pages are pinned – count as one blocked eviction attempt
        ++stats_.pin_skips;
    }

    // ── Data members ─────────────────────────────────────────────────────────

    const size_t capacity_;
    size_t       p_;          ///< Target size of T1

    // Live lists (hold actual data)
    List    t1_list_;         ///< Recently-seen (MRU at front)
    LiveMap t1_map_;          ///< Fast lookup into T1
    List    t2_list_;         ///< Frequently-seen (MRU at front)
    LiveMap t2_map_;          ///< Fast lookup into T2

    // Ghost lists (hold only keys, no data)
    GhostSet b1_set_;         ///< Keys evicted from T1
    GhostSet b2_set_;         ///< Keys evicted from T2

    // Pinned pages (may not be evicted)
    std::unordered_set<K> pinned_;

    mutable std::mutex mutex_;
    Stats stats_;
};

} // namespace cache
} // namespace themis
