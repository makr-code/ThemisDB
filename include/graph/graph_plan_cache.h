/**
 * @file graph_plan_cache.h
 * @brief Phase-3 extended plan-cache and cost-model utilities for the graph module.
 *
 * Provides:
 *  - @ref themis::graph::GraphLRUPlanCache  – thread-safe, generic LRU cache with TTL
 *  - @ref themis::graph::GraphCostHistogram – fixed-bucket latency histogram
 *  - @ref themis::graph::GraphAdvancedCostModel – extended per-algorithm cost model
 *
 * @version 1.9.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace graph {

// ─────────────────────────────────────────────────────────────────────────────
// GraphLRUPlanCache
// ─────────────────────────────────────────────────────────────────────────────

template <typename K, typename V>
class GraphLRUPlanCache {
public:
    using Clock     = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    struct Metrics {
        uint64_t hits      = 0; ///< Total cache hits
        uint64_t misses    = 0; ///< Total cache misses
        uint64_t evictions = 0; ///< Entries evicted due to LRU or TTL
        uint64_t inserts   = 0; ///< Total entries inserted

        double hitRatio() const {
            uint64_t total = hits + misses;
            return total > 0 ? static_cast<double>(hits) / static_cast<double>(total) : 0.0;
        }
    };

    explicit GraphLRUPlanCache(size_t max_size = 0,
                               std::chrono::milliseconds ttl = std::chrono::milliseconds{0})
        : max_size_(max_size), ttl_(ttl) {}

    /**
     * @brief Put.
     * @param[in] key Input parameter.
     * @param[in] value Input parameter.
     * @details Calls: lock(), find(), end(), erase(), push_front(), std::move(), Clock::now(), begin().
     */
    void put(const K& key, V value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end()) {
            // Update existing entry: move to front of LRU list.
            lru_.erase(it->second.lru_it);
            lru_.push_front(key);
            it->second.value   = std::move(value);
            it->second.inserted = Clock::now();
            it->second.lru_it  = lru_.begin();
        } else {
            // Evict LRU entry if at capacity.
            if (max_size_ > 0 && map_.size() >= max_size_) {
                evictLRU();
            }
            lru_.push_front(key);
            map_.emplace(key, Entry{std::move(value), Clock::now(), lru_.begin()});
            ++metrics_.inserts;
        }
    }

    /**
     * @brief Get.
     * @param[in] key Input parameter.
     * @return Return value.
     * @details Calls: lock(), find(), end(), isExpired(), evictEntry(), erase(), push_front(), begin().
     */
    std::optional<V> get(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it == map_.end()) {
            ++metrics_.misses;
            return std::nullopt;
        }
        // TTL check
        if (isExpired(it->second)) {
            evictEntry(it);
            ++metrics_.misses;
            return std::nullopt;
        }
        // Promote to MRU
        lru_.erase(it->second.lru_it);
        lru_.push_front(key);
        it->second.lru_it = lru_.begin();
        ++metrics_.hits;
        return it->second.value;
    }

    /**
     * @brief Remove.
     * @param[in] key Input parameter.
     * @return True when the operation succeeds.
     * @details Calls: lock(), find(), end(), erase().
     */
    bool remove(const K& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it == map_.end()) {
          return false;
        }
        lru_.erase(it->second.lru_it);
        map_.erase(it);
        return true;
    }

    /**
     * @brief Clear.
     * @details Calls: lock().
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.clear();
        lru_.clear();
    }

    size_t size() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.size();
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
     * @brief Purge Expired.
     * @return Return value.
     * @details Calls: lock(), begin(), end(), isExpired(), erase().
     */
    size_t purgeExpired() {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t count = 0;
        auto it = map_.begin();
        while (it != map_.end()) {
            if (isExpired(it->second)) {
                lru_.erase(it->second.lru_it);
                it = map_.erase(it);
                ++metrics_.evictions;
                ++count;
            } else {
                ++it;
            }
        }
        return count;
    }

    /**
     * @brief Set Max Size.
     * @param[in] max_size Input parameter.
     * @details Calls: lock(), size(), evictLRU().
     */
    void setMaxSize(size_t max_size) {
        std::lock_guard<std::mutex> lock(mutex_);
        max_size_ = max_size;
        while (max_size_ > 0 && map_.size() > max_size_) {
            evictLRU();
        }
    }

    /**
     * @brief Set TTL.
     * @param[in] ttl Input parameter.
     * @details Calls: lock().
     */
    void setTTL(std::chrono::milliseconds ttl) {
        std::lock_guard<std::mutex> lock(mutex_);
        ttl_ = ttl;
    }

    size_t maxSize() const { return max_size_; }

    std::chrono::milliseconds ttl() const { return ttl_; }

private:
    struct Entry {
        V           value;
        TimePoint   inserted;
        typename std::list<K>::iterator lru_it;
    };

    bool isExpired(const Entry& e) const {
        if (ttl_.count() == 0) {
          return false;
        }
        return Clock::now() - e.inserted >= ttl_;
    }

    /**
     * @brief Must be called under lock.
     * @details Calls: empty(), back(), erase(), pop_back().
     */
    void evictLRU() {
        if (lru_.empty()) {
          return;
        }
        const K& victim = lru_.back();
        map_.erase(victim);
        lru_.pop_back();
        ++metrics_.evictions;
    }

    // Must be called under lock.
    void evictEntry(typename std::unordered_map<K, Entry>::iterator it) {
        lru_.erase(it->second.lru_it);
        map_.erase(it);
        ++metrics_.evictions;
    }

    mutable std::mutex mutex_;
    size_t             max_size_;
    std::chrono::milliseconds ttl_;
    std::list<K> lru_;
    std::unordered_map<K, Entry> map_;
    Metrics metrics_;
};

// ─────────────────────────────────────────────────────────────────────────────
// GraphCostHistogram
// ─────────────────────────────────────────────────────────────────────────────

class GraphCostHistogram {
public:
    static constexpr size_t   kBucketCount = 10;
    static constexpr uint64_t kBounds[9]   = {1, 5, 10, 25, 50, 100, 250, 500, 1000};

    /**
     * @brief Record.
     * @param[in] latency_ms Input parameter.
     * @details Calls: fetch_add().
     */
    void record(uint64_t latency_ms) {
        for (size_t i = 0; i < 9; ++i) {
            if (latency_ms <= kBounds[i]) {
                counts_[i].fetch_add(1, std::memory_order_relaxed);
                return;
            }
        }
        counts_[9].fetch_add(1, std::memory_order_relaxed);
    }

    uint64_t total() const {
        uint64_t sum = 0;
        for (size_t i = 0; i < kBucketCount; ++i)
            sum += counts_[i].load(std::memory_order_relaxed);
        return sum;
    }

    double percentileMs(double p) const {
        const uint64_t n = total();
        if (n == 0) {
          return 0.0;
        }
        const uint64_t target     = static_cast<uint64_t>(p * static_cast<double>(n));
        uint64_t       cumulative = 0;
        for (size_t i = 0; i < kBucketCount; ++i) {
            uint64_t bc = counts_[i].load(std::memory_order_relaxed);
            if (cumulative + bc > target) {
                const double lower = (i == 0) ? 0.0 : static_cast<double>(kBounds[i - 1]);
                const double upper = (i < 9)  ? static_cast<double>(kBounds[i])
                                              : static_cast<double>(kBounds[8]) * 2.0;
                if (bc == 0) {
                  return lower;
                }
                const double frac = static_cast<double>(target - cumulative) /
                                    static_cast<double>(bc);
                return lower + frac * (upper - lower);
            }
            cumulative += bc;
        }
        return static_cast<double>(kBounds[8]) * 2.0;
    }

    /**
     * @brief Reset the modification detection flag.
     * @details Calls: store().
     */
    void reset() {
        for (size_t i = 0; i < kBucketCount; ++i)
            counts_[i].store(0, std::memory_order_relaxed);
    }

    std::vector<uint64_t> bucketCounts() const {
        /**
         * @brief Out.
         * @param[in] kBucketCount Input parameter.
         * @return Return value.
         */
        std::vector<uint64_t> out(kBucketCount);
        for (size_t i = 0; i < kBucketCount; ++i)
            out[i] = counts_[i].load(std::memory_order_relaxed);
        return out;
    }

private:
    std::atomic<uint64_t> counts_[kBucketCount]{};
};

// ─────────────────────────────────────────────────────────────────────────────
// GraphAdvancedCostModel
// ─────────────────────────────────────────────────────────────────────────────

class GraphAdvancedCostModel {
public:
    static constexpr double kDefaultAlpha = 0.1; ///< Default EMA learning rate

    explicit GraphAdvancedCostModel(double alpha = kDefaultAlpha)
        : alpha_(alpha) {}

    /**
     * @brief Observe.
     * @param[in] observed_ms Input parameter.
     * @details Calls: std::min(), record(), std::max().
     */
    void observe(double observed_ms) {
        if (exec_count_ == 0) {
            ema_cost_ms_ = observed_ms;
        } else {
            ema_cost_ms_ = alpha_ * observed_ms + (1.0 - alpha_) * ema_cost_ms_;
        }
        ++exec_count_;
        confidence_ = std::min(1.0, static_cast<double>(exec_count_) / kMaxConfidenceObs);
        histogram_.record(static_cast<uint64_t>(std::max(0.0, observed_ms)));
    }

    double emaCostMs() const { return ema_cost_ms_; }

    double confidence() const { return confidence_; }

    uint32_t execCount() const { return exec_count_; }

    double p99Ms() const { return histogram_.percentileMs(0.99); }

    double p95Ms() const { return histogram_.percentileMs(0.95); }

    double p50Ms() const { return histogram_.percentileMs(0.50); }

    double blendedEstimate(double theoretical_ms) const {
        return confidence_ * ema_cost_ms_ + (1.0 - confidence_) * theoretical_ms;
    }

    /**
     * @brief Reset the modification detection flag.
     * @details Implements reset without additional internal calls.
     */
    void reset() {
        ema_cost_ms_ = 0.0;
        exec_count_  = 0;
        confidence_  = 0.0;
        histogram_.reset();
    }

    const GraphCostHistogram& histogram() const { return histogram_; }

private:
    static constexpr uint32_t kMaxConfidenceObs = 100;

    double   alpha_;
    double   ema_cost_ms_ = 0.0;
    uint32_t exec_count_  = 0;
    double   confidence_  = 0.0;
    GraphCostHistogram histogram_;
};

} // namespace graph
} // namespace themis
