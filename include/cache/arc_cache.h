/**
 * @file arc_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.46
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

        double hit_rate() const {
            uint64_t total = hits + misses;
            return total == 0 ? 0.0 : static_cast<double>(hits) / total;
        }
    };

    /**
     * @brief ── Construction / destruction ───────────────────────────────────────────
     * @param[in] capacity Input parameter.
     * @return Return value.
     */

    explicit ARCCache(size_t capacity)
        : capacity_(capacity > 0 ? capacity : 1), p_(0) {}

    ~ARCCache() = default;

    ARCCache(const ARCCache&) = delete;
    ARCCache& operator=(const ARCCache&) = delete;
    ARCCache(ARCCache&&) = delete;
    ARCCache& operator=(ARCCache&&) = delete;

    /**
     * @brief ── Core operations ──────────────────────────────────────────────────────
     * @param[in] key Input parameter.
     * @return Return value.
     * @details Calls: lock(), find(), end(), push_front(), begin(), erase(), front(), splice().
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
     * @brief Put.
     * @param[in] key Input parameter.
     * @param[in] value Input parameter.
     * @details Calls: lock(), find(), end(), std::move(), push_front(), begin(), erase(), splice().
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
     * @brief Remove.
     * @param[in] key Input parameter.
     * @return True when the operation succeeds.
     * @details Calls: lock(), find(), end(), erase().
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

    /**
     * @brief Clear.
     * @details Calls: lock().
     */
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

    size_t size() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return t1_list_.size() + t2_list_.size();
    }

    size_t capacity() const { return capacity_; }

    size_t targetT1() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return p_;
    }

    Stats stats() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }

    bool contains(const K& key) const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return t1_map_.count(key) || t2_map_.count(key);
    }

    /**
     * @brief ── Hot-page pinning ─────────────────────────────────────────────────────
     * @param[in] key Input parameter.
     * @details Calls: lock(), insert().
     */

    void pin(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        pinned_.insert(key);
    }

    /**
     * @brief Unpin.
     * @param[in] key Input parameter.
     * @details Calls: lock(), erase().
     */
    void unpin(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        pinned_.erase(key);
    }

    bool isPinned(const K& key) const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
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

    /**
     * @brief ── Eviction ─────────────────────────────────────────────────────────────
     * @details Calls: empty(), size(), rbegin(), rend(), count(), erase(), std::next(), base().
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
