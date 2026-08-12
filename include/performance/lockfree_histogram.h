/**
 * @file lockfree_histogram.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB – Performance Module
 * File:  include/performance/lockfree_histogram.h
 * Brief: Lock-free fixed-bucket histogram for low-overhead latency tracking.
 *
 * Design goals:
 *   • `record(value)` ≤ 20 ns per call on modern x86-64 (single atomic fetch_add)
 *   • `percentile(p)` is wait-free from any thread
 *   • `reset()` resets all counters atomically per bucket (not snapshot-atomic
 *     overall — concurrent record() calls during reset are accepted)
 *   • No heap allocation after construction
 *   • Header-only so it can be included in hot-path translation units
 */

#pragma once

#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace themis {
namespace performance {

/**
 * @brief Bucket-mapping strategy for LockFreeHistogram.
 */
enum class HistogramMode {
    Linear,      ///< Uniform bucket width: bucket = value / bucket_width
    Exponential, ///< Power-of-2 buckets: bucket = floor(log2(value + 1))
};

/**
 * @brief Lock-free fixed-bucket histogram.
 *
 * @tparam T           Value type (must be an unsigned integral or floating-point
 *                     type that can be converted to uint64_t).
 * @tparam NumBuckets  Number of buckets (compile-time constant, must be ≥ 2).
 * @tparam Mode        Bucket-mapping strategy (default: Exponential).
 *
 * ### Linear mode
 * Every bucket covers a range of `max_value / NumBuckets` units.  Values that
 * exceed `max_value` are clamped into the last bucket (overflow bucket).
 *
 * ### Exponential mode
 * Bucket `i` covers values in the range `[2^(i-1), 2^i)`.  Bucket 0 always
 * counts the value 0.  Values that exceed `2^(NumBuckets-1)` are clamped into
 * the last bucket.  This distribution matches typical latency profiles where
 * most values cluster at the low end.
 *
 * ### Thread safety
 * All operations are thread-safe and lock-free.  `record()` is wait-free
 * (single `fetch_add`).  `percentile()` and `count()` use relaxed reads.
 * `reset()` is not globally atomic — use it only when no concurrent `record()`
 * calls are in progress (e.g., at metric export time).
 *
 * ### Example — latency histogram (µs, exponential buckets)
 * @code
 * using LatencyHist = LockFreeHistogram<uint64_t, 32>;
 * LatencyHist hist;
 * hist.record(elapsed_us);
 * double p99 = hist.percentile(99.0);
 * @endcode
 */
template<typename T      = uint64_t,
         std::size_t NumBuckets = 32,
         HistogramMode Mode     = HistogramMode::Exponential>
class LockFreeHistogram {
    static_assert(NumBuckets >= 2,
                  "LockFreeHistogram requires at least 2 buckets");

public:
    /**
     * @brief Construct with an optional maximum value (Linear mode only).
     *
     * @param max_value  In Linear mode: the upper bound mapped to the last bucket.
     *                   Ignored in Exponential mode.
     */
    explicit LockFreeHistogram(T max_value = T{1}) noexcept
        : max_value_(max_value)
    {
        for (auto& b : buckets_) {
            b.store(0, std::memory_order_relaxed);
        }
        total_.store(0, std::memory_order_relaxed);
    }

    // Non-copyable to prevent accidental copy of atomic arrays.
    LockFreeHistogram(const LockFreeHistogram&)            = delete;
    LockFreeHistogram& operator=(const LockFreeHistogram&) = delete;

    /**
     * @brief Record a single sample.
     *
     * Hot-path.  Performs exactly one `fetch_add(1, relaxed)` on the target
     * bucket plus one `fetch_add(1, relaxed)` on the global counter.
     *
     * @param value  The sample to record.
     */
    void record(T value) noexcept {
        std::size_t idx = mapToBucket(value);
        buckets_[idx].fetch_add(1, std::memory_order_relaxed);
        total_.fetch_add(1, std::memory_order_relaxed);
    }

    /**
     * @brief Compute the p-th percentile from the current bucket counts.
     *
     * Returns the *lower bound* of the bucket that contains the p-th
     * percentile sample.  In Linear mode this is `bucket_index * bucket_width`.
     * In Exponential mode this is `2^(bucket_index-1)` (or 0 for bucket 0).
     *
     * @param  p  Percentile in the range [0, 100].
     * @return    Approximate lower-bound value of the percentile bucket,
     *            as a `double`.
     * @throws std::invalid_argument if `p` is outside [0, 100].
     * @throws std::runtime_error if the histogram is empty.
     */
    double percentile(double p) const {
        if (p < 0.0 || p > 100.0) {
            throw std::invalid_argument(
                "LockFreeHistogram::percentile: p must be in [0, 100]");
        }

        uint64_t total = total_.load(std::memory_order_relaxed);
        if (total == 0) {
            throw std::runtime_error(
                "LockFreeHistogram::percentile: histogram is empty");
        }

        // Snapshot bucket counts.
        std::array<uint64_t, NumBuckets> snap;
        for (std::size_t i = 0; i < NumBuckets; ++i) {
            snap[i] = buckets_[i].load(std::memory_order_relaxed);
        }

        uint64_t target  = static_cast<uint64_t>(p / 100.0 * static_cast<double>(total));
        if (target == 0) target = 1; // at least the first sample

        uint64_t running = 0;
        for (std::size_t i = 0; i < NumBuckets; ++i) {
            running += snap[i];
            if (running >= target) {
                return bucketLowerBound(i);
            }
        }
        // Should not reach here; return lower bound of last bucket.
        return bucketLowerBound(NumBuckets - 1);
    }

    /**
     * @brief Total number of recorded samples.
     */
    uint64_t count() const noexcept {
        return total_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Count of samples in a specific bucket (0-indexed).
     *
     * @throws std::out_of_range if `idx >= NumBuckets`.
     */
    uint64_t bucket_count(std::size_t idx) const {
        if (idx >= NumBuckets) {
            throw std::out_of_range("LockFreeHistogram::bucket_count: index out of range");
        }
        return buckets_[idx].load(std::memory_order_relaxed);
    }

    /**
     * @brief Reset all bucket counts and the total counter to zero.
     *
     * Not snapshot-atomic.  Safe to call when all writers have quiesced.
     */
    void reset() noexcept {
        for (auto& b : buckets_) {
            b.store(0, std::memory_order_relaxed);
        }
        total_.store(0, std::memory_order_relaxed);
    }

    /**
     * @brief Number of buckets (compile-time constant).
     */
    static constexpr std::size_t num_buckets() noexcept { return NumBuckets; }

    /**
     * @brief Lower bound of bucket `idx` in the original value domain.
     */
    double bucketLowerBound(std::size_t idx) const noexcept {
        if constexpr (Mode == HistogramMode::Linear) {
            double width = static_cast<double>(max_value_) / NumBuckets;
            return static_cast<double>(idx) * width;
        } else {
            // Exponential: bucket 0 → 0, bucket i → 2^(i-1)
            if (idx == 0) return 0.0;
            return static_cast<double>(uint64_t{1} << (idx - 1));
        }
    }

private:
    // Align each atomic<uint64_t> to its own cache line to eliminate
    // false-sharing between concurrent record() calls on adjacent buckets.
    struct alignas(64) AlignedCounter {
        std::atomic<uint64_t> value{0};
        void store(uint64_t v, std::memory_order o) noexcept { value.store(v, o); }
        uint64_t load(std::memory_order o) const noexcept    { return value.load(o); }
        uint64_t fetch_add(uint64_t v, std::memory_order o) noexcept {
            return value.fetch_add(v, o);
        }
    };

    std::array<AlignedCounter, NumBuckets> buckets_;
    std::atomic<uint64_t>                  total_{0};
    T                                      max_value_;

    std::size_t mapToBucket(T value) const noexcept {
        if constexpr (Mode == HistogramMode::Linear) {
            if (static_cast<double>(value) >=
                    static_cast<double>(max_value_)) {
                return NumBuckets - 1;
            }
            double ratio = static_cast<double>(value) /
                           static_cast<double>(max_value_);
            std::size_t idx = static_cast<std::size_t>(ratio * NumBuckets);
            return idx < NumBuckets ? idx : NumBuckets - 1;
        } else {
            // Exponential: bucket = floor(log2(value + 1))
            uint64_t v = static_cast<uint64_t>(value);
            if (v == 0) return 0;
            // Count leading zeros to compute floor(log2(v))
            std::size_t bit = 63u - static_cast<std::size_t>(std::countl_zero(v));
            std::size_t idx = bit + 1; // bucket i covers [2^(i-1), 2^i)
            return idx < NumBuckets ? idx : NumBuckets - 1;
        }
    }
};

// ---------------------------------------------------------------------------
// Convenience aliases
// ---------------------------------------------------------------------------

/// 32-bucket exponential histogram for sub-millisecond latency tracking (µs).
using LatencyHistogram = LockFreeHistogram<uint64_t, 32, HistogramMode::Exponential>;

/// 64-bucket exponential histogram for wider value ranges.
using WideHistogram = LockFreeHistogram<uint64_t, 64, HistogramMode::Exponential>;

} // namespace performance
} // namespace themis
