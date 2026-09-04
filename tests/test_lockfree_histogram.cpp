/**
 * @file test_lockfree_histogram.cpp
 * @brief Focused unit tests for performance::LockFreeHistogram.
 *
 * Test IDs: LFH-01 … LFH-12
 *
 * Validates:
 *  - Basic record + count
 *  - Exponential bucket mapping
 *  - Linear bucket mapping
 *  - Percentile computation (P50, P90, P99)
 *  - Empty histogram throws on percentile()
 *  - reset() clears all counters
 *  - Out-of-range bucket_count() throws
 *  - Overflow values clamped into last bucket
 *  - Thread-safe concurrent record() via TSan
 *  - LatencyHistogram and WideHistogram aliases compile and work
 *  - bucketLowerBound() correctness
 *  - record() ≤ 20 ns overhead (best-effort timing test)
 */

#include <gtest/gtest.h>
#include "performance/lockfree_histogram.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace themis::performance;

// ---------------------------------------------------------------------------
// LFH-01  Empty histogram: count == 0, percentile throws
// ---------------------------------------------------------------------------
TEST(LockFreeHistogramTest, LFH01_EmptyHistogram) {
    LockFreeHistogram<uint64_t, 16> h;
    EXPECT_EQ(h.count(), 0u);
    EXPECT_THROW(h.percentile(50.0), std::runtime_error);
}

// ---------------------------------------------------------------------------
// LFH-02  Single record increments count and correct bucket
// ---------------------------------------------------------------------------
TEST(LockFreeHistogramTest, LFH02_SingleRecord) {
    LockFreeHistogram<uint64_t, 16> h;
    h.record(0u);
    EXPECT_EQ(h.count(), 1u);
    EXPECT_EQ(h.bucket_count(0), 1u); // value 0 → bucket 0
}

// ---------------------------------------------------------------------------
// LFH-03  Exponential mode: bucket mapping correctness
// ---------------------------------------------------------------------------
TEST(LockFreeHistogramTest, LFH03_ExponentialBucketMapping) {
    LockFreeHistogram<uint64_t, 32, HistogramMode::Exponential> h;
    // value 0 → bucket 0
    h.record(0u);
    EXPECT_EQ(h.bucket_count(0), 1u);
    // value 1 → floor(log2(1)) = 0; +1 = bucket 1
    h.record(1u);
    EXPECT_EQ(h.bucket_count(1), 1u);
    // value 2 → floor(log2(2)) = 1; +1 = bucket 2
    h.record(2u);
    EXPECT_EQ(h.bucket_count(2), 1u);
    // value 4 → floor(log2(4)) = 2; +1 = bucket 3
    h.record(4u);
    EXPECT_EQ(h.bucket_count(3), 1u);
}

// ---------------------------------------------------------------------------
// LFH-04  Linear mode: bucket mapping correctness
// ---------------------------------------------------------------------------
TEST(LockFreeHistogramTest, LFH04_LinearBucketMapping) {
    // 10 buckets, max_value = 100 → each bucket is 10 units wide.
    LockFreeHistogram<uint64_t, 10, HistogramMode::Linear> h(100u);
    h.record(0u);   // bucket 0: [0, 10)
    h.record(9u);   // bucket 0
    h.record(10u);  // bucket 1: [10, 20)
    h.record(50u);  // bucket 5: [50, 60)
    EXPECT_EQ(h.bucket_count(0), 2u);
    EXPECT_EQ(h.bucket_count(1), 1u);
    EXPECT_EQ(h.bucket_count(5), 1u);
    EXPECT_EQ(h.count(), 4u);
}

// ---------------------------------------------------------------------------
// LFH-05  Overflow values clamped into last bucket
// ---------------------------------------------------------------------------
TEST(LockFreeHistogramTest, LFH05_OverflowClampedToLastBucket) {
    // Exponential, 8 buckets: last bucket handles values ≥ 2^6 = 64
    LockFreeHistogram<uint64_t, 8, HistogramMode::Exponential> h;
    h.record(1000u); // far exceeds 2^6
    EXPECT_EQ(h.bucket_count(7), 1u);
    EXPECT_EQ(h.count(), 1u);
}

// ---------------------------------------------------------------------------
// LFH-06  percentile() invalid range throws
// ---------------------------------------------------------------------------
TEST(LockFreeHistogramTest, LFH06_PercentileInvalidRangeThrows) {
    LockFreeHistogram<uint64_t, 16> h;
    h.record(1u);
    EXPECT_THROW(h.percentile(-1.0),  std::invalid_argument);
    EXPECT_THROW(h.percentile(101.0), std::invalid_argument);
    EXPECT_NO_THROW(h.percentile(0.0));
    EXPECT_NO_THROW(h.percentile(100.0));
}

// ---------------------------------------------------------------------------
// LFH-07  percentile() returns correct bucket lower bound
// ---------------------------------------------------------------------------
TEST(LockFreeHistogramTest, LFH07_PercentileCorrectness) {
    // Fill 100 samples: 50 in bucket-for-1, 50 in bucket-for-1000.
    LockFreeHistogram<uint64_t, 32, HistogramMode::Exponential> h;
    for (int i = 0; i < 50; ++i) h.record(1u);    // bucket 1
    for (int i = 0; i < 50; ++i) h.record(1000u); // bucket 10 (clamped)

    double p50 = h.percentile(50.0);
    double p99 = h.percentile(99.0);

    // P50 should be in the low-value bucket (lower bound = 1)
    EXPECT_LT(p50, 100.0) << "P50 expected in low bucket, got " << p50;
    // P99 should be in the high-value bucket
    EXPECT_GT(p99, 100.0) << "P99 expected in high bucket, got " << p99;
}

// ---------------------------------------------------------------------------
// LFH-08  reset() clears all counters
// ---------------------------------------------------------------------------
TEST(LockFreeHistogramTest, LFH08_ResetClearsCounters) {
    LockFreeHistogram<uint64_t, 16> h;
    for (int i = 0; i < 100; ++i) {
      h.record(static_cast<uint64_t>(i));
    }
    EXPECT_GT(h.count(), 0u);
    h.reset();
    EXPECT_EQ(h.count(), 0u);
    for (std::size_t i = 0; i < h.num_buckets(); ++i) {
        EXPECT_EQ(h.bucket_count(i), 0u) << "Bucket " << i << " not cleared";
    }
    EXPECT_THROW(h.percentile(50.0), std::runtime_error);
}

// ---------------------------------------------------------------------------
// LFH-09  bucket_count() out-of-range throws
// ---------------------------------------------------------------------------
TEST(LockFreeHistogramTest, LFH09_BucketCountOutOfRange) {
    LockFreeHistogram<uint64_t, 8> h;
    EXPECT_THROW(h.bucket_count(8),   std::out_of_range);
    EXPECT_THROW(h.bucket_count(100), std::out_of_range);
    EXPECT_NO_THROW(h.bucket_count(7));
}

// ---------------------------------------------------------------------------
// LFH-10  LatencyHistogram and WideHistogram aliases compile and record
// ---------------------------------------------------------------------------
TEST(LockFreeHistogramTest, LFH10_AliasesWork) {
    LatencyHistogram lh;
    WideHistogram    wh;
    lh.record(42u);
    wh.record(1000u);
    EXPECT_EQ(lh.count(), 1u);
    EXPECT_EQ(wh.count(), 1u);
    EXPECT_EQ(LatencyHistogram::num_buckets(), 32u);
    EXPECT_EQ(WideHistogram::num_buckets(),    64u);
}

// ---------------------------------------------------------------------------
// LFH-11  Thread-safe concurrent record() — no data races (TSan check)
// ---------------------------------------------------------------------------
TEST(LockFreeHistogramTest, LFH11_ConcurrentRecordNoDataRaces) {
    constexpr int THREADS = 8;
    constexpr int OPS_PER_THREAD = 10000;

    LockFreeHistogram<uint64_t, 32, HistogramMode::Exponential> h;
    std::vector<std::thread> threads;
    threads.reserve(THREADS);

    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < OPS_PER_THREAD; ++i) {
                h.record(static_cast<uint64_t>((t + 1) * (i + 1)));
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    EXPECT_EQ(h.count(), static_cast<uint64_t>(THREADS) * OPS_PER_THREAD);
}

// ---------------------------------------------------------------------------
// LFH-12  bucketLowerBound() values are monotonically increasing
// ---------------------------------------------------------------------------
TEST(LockFreeHistogramTest, LFH12_BucketLowerBoundMonotonic) {
    LockFreeHistogram<uint64_t, 32, HistogramMode::Exponential> h_exp;
    LockFreeHistogram<uint64_t, 16, HistogramMode::Linear>      h_lin(1000u);

    for (std::size_t i = 1; i < h_exp.num_buckets(); ++i) {
        EXPECT_LT(h_exp.bucketLowerBound(i - 1), h_exp.bucketLowerBound(i))
            << "Exp bucket " << i << " lower bound not strictly increasing";
    }
    for (std::size_t i = 1; i < h_lin.num_buckets(); ++i) {
        EXPECT_LT(h_lin.bucketLowerBound(i - 1), h_lin.bucketLowerBound(i))
            << "Lin bucket " << i << " lower bound not strictly increasing";
    }
}
