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

/**
 * @brief Thread-safe generic LRU cache with optional TTL expiry.
 *
 * Uses a combination of a doubly-linked list (access order) and a hash map
 * for O(1) average-case insert, lookup, and eviction.
 *
 * @tparam K Key type (must be hashable).
 * @tparam V Value type (must be movable).
 */
template <typename K, typename V>
class GraphLRUPlanCache {
public:
    using Clock     = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    /**
     * @brief Metrics snapshot for this cache instance.
     */
    struct Metrics {
        uint64_t hits      = 0; ///< Total cache hits
        uint64_t misses    = 0; ///< Total cache misses
        uint64_t evictions = 0; ///< Entries evicted due to LRU or TTL
        uint64_t inserts   = 0; ///< Total entries inserted

        /**
         * @brief Compute the hit ratio in [0.0, 1.0].
         * @return Hit ratio, or 0.0 if no lookups have been performed.
         */
        double hitRatio() const {
            uint64_t total = hits + misses;
            return total > 0 ? static_cast<double>(hits) / static_cast<double>(total) : 0.0;
        }
    };

    /**
     * @brief Construct a cache with the given capacity and TTL.
     *
     * @param max_size Maximum number of entries (0 = unlimited).
     * @param ttl      Time-to-live per entry (zero = no expiry).
     */
    explicit GraphLRUPlanCache(size_t max_size = 0,
                               std::chrono::milliseconds ttl = std::chrono::milliseconds{0})
        : max_size_(max_size), ttl_(ttl) {}

    /**
     * @brief Insert or update a key-value pair.
     *
     * If the key already exists it is updated in place and promoted to MRU
     * position.  When the cache is full the LRU entry is evicted.
     *
     * @param key   Cache key.
     * @param value Value to store.
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
     * @brief Look up a value by key.
     *
     * On a hit the entry is promoted to MRU position.  Expired entries
     * (TTL-based) are treated as misses and lazily evicted.
     *
     * @param key Cache key.
     * @return The cached value, or std::nullopt on miss/expiry.
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
     * @brief Remove a specific key from the cache.
     * @param key Cache key to remove.
     * @return true if the key was present and removed.
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

    /// Clear all entries.
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.clear();
        lru_.clear();
    }

    /**
     * @brief Return the current number of live entries.
     * @return Entry count.
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.size();
    }

    /**
     * @brief Return a copy of the current metrics.
     * @return Metrics snapshot.
     */
    Metrics metrics() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return metrics_;
    }

    /**
     * @brief Evict all TTL-expired entries.
     * @return Number of entries evicted.
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
     * @brief Set or update the maximum cache size.
     * @param max_size New maximum (0 = unlimited).
     */
    void setMaxSize(size_t max_size) {
        std::lock_guard<std::mutex> lock(mutex_);
        max_size_ = max_size;
        while (max_size_ > 0 && map_.size() > max_size_) {
            evictLRU();
        }
    }

    /**
     * @brief Set or update the TTL for new and existing lookups.
     * @param ttl New TTL (zero = no expiry).
     */
    void setTTL(std::chrono::milliseconds ttl) {
        std::lock_guard<std::mutex> lock(mutex_);
        ttl_ = ttl;
    }

    /**
     * @brief Return configured maximum size (0 = unlimited).
     * @return Max size.
     */
    size_t maxSize() const { return max_size_; }

    /**
     * @brief Return configured TTL (zero = no expiry).
     * @return TTL.
     */
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

    // Must be called under lock.
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

/**
 * @brief Fixed-bucket latency histogram for query cost tracking.
 *
 * Provides O(1) recording and approximate percentile computation.
 * Bucket upper bounds (ms): 1, 5, 10, 25, 50, 100, 250, 500, 1000, +Inf.
 */
class GraphCostHistogram {
public:
    static constexpr size_t   kBucketCount = 10;
    static constexpr uint64_t kBounds[9]   = {1, 5, 10, 25, 50, 100, 250, 500, 1000};

    /**
     * @brief Record one observation.
     * @param latency_ms Observed latency in milliseconds.
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

    /**
     * @brief Return the total number of observations.
     * @return Observation count.
     */
    uint64_t total() const {
        uint64_t sum = 0;
        for (size_t i = 0; i < kBucketCount; ++i)
            sum += counts_[i].load(std::memory_order_relaxed);
        return sum;
    }

    /**
     * @brief Compute approximate p-th percentile latency.
     * @param p Percentile in [0.0, 1.0] (e.g. 0.99 for p99).
     * @return Approximate latency in ms, or 0.0 if no data.
     */
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

    /// Reset all counters.
    void reset() {
        for (size_t i = 0; i < kBucketCount; ++i)
            counts_[i].store(0, std::memory_order_relaxed);
    }

    /**
     * @brief Return a snapshot of raw bucket counts.
     * @return Vector of counts indexed by bucket.
     */
    std::vector<uint64_t> bucketCounts() const {
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

/**
 * @brief Extended per-algorithm cost model for graph query planning.
 *
 * Combines an exponential moving average (EMA) of observed execution times
 * with a fixed-bucket histogram for percentile analysis.  Optionally
 * supports a graph-size cost multiplier for scaling estimates.
 */
class GraphAdvancedCostModel {
public:
    static constexpr double kDefaultAlpha = 0.1; ///< Default EMA learning rate

    /**
     * @brief Construct with configurable EMA learning rate.
     * @param alpha EMA alpha in (0.0, 1.0]; smaller = slower adaptation.
     */
    explicit GraphAdvancedCostModel(double alpha = kDefaultAlpha)
        : alpha_(alpha) {}

    /**
     * @brief Record an observed execution duration.
     *
     * Updates the EMA and histogram.  Thread-safe for histogram updates;
     * EMA update requires external synchronisation if called concurrently.
     *
     * @param observed_ms Observed execution time in milliseconds.
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

    /**
     * @brief Return the current EMA cost estimate.
     * @return EMA cost in ms, or 0.0 before any observations.
     */
    double emaCostMs() const { return ema_cost_ms_; }

    /**
     * @brief Return the confidence level in [0.0, 1.0].
     * @return Confidence (saturates at 1.0 after kMaxConfidenceObs observations).
     */
    double confidence() const { return confidence_; }

    /**
     * @brief Return the number of observations recorded.
     * @return Observation count.
     */
    uint32_t execCount() const { return exec_count_; }

    /**
     * @brief Return the p99 latency estimate from the histogram.
     * @return Approximate p99 latency in ms.
     */
    double p99Ms() const { return histogram_.percentileMs(0.99); }

    /**
     * @brief Return the p95 latency estimate from the histogram.
     * @return Approximate p95 latency in ms.
     */
    double p95Ms() const { return histogram_.percentileMs(0.95); }

    /**
     * @brief Return the p50 latency estimate from the histogram.
     * @return Approximate p50 latency in ms.
     */
    double p50Ms() const { return histogram_.percentileMs(0.50); }

    /**
     * @brief Compute a cost estimate blending static and learned data.
     *
     * When confidence is high the learned EMA dominates; when low the
     * theoretical estimate is preferred.
     *
     * @param theoretical_ms Static/theory-based cost estimate in ms.
     * @return Blended estimate in ms.
     */
    double blendedEstimate(double theoretical_ms) const {
        return confidence_ * ema_cost_ms_ + (1.0 - confidence_) * theoretical_ms;
    }

    /**
     * @brief Reset all learned state.
     */
    void reset() {
        ema_cost_ms_ = 0.0;
        exec_count_  = 0;
        confidence_  = 0.0;
        histogram_.reset();
    }

    /**
     * @brief Access the underlying histogram for raw bucket data.
     * @return Const reference to the histogram.
     */
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
