/**
 * @file test_wire_perf_benchmark.cpp
 * @brief Performance benchmark tests for Themis Core Framework components
 *
 * These tests measure throughput and latency of the performance-critical
 * components and assert that they meet the targets stated in the issue:
 *
 *   • Wire Protocol latency tracking overhead     < 5 µs/call  (CI threshold; HW target < 1 µs)
 *   • PayloadBufferPool: pool-hit path            < 10 µs/acquire (CI threshold)
 *   • PayloadBufferPool: hit rate after warm-up   ≥ 85%
 *   • CompressionAdvisor::advise()               < 1000 ns/call (CI threshold; HW target < 100 ns)
 *   • WireProtocolMetrics::snapshot()            < 50 ms (CI, 10 000 samples; HW target < 5 ms)
 *
 * CI thresholds are 5–10× the hardware targets to tolerate VM variance.
 * All timings are measured with std::chrono::steady_clock.
 */

#include <gtest/gtest.h>
#include "network/wire_protocol_performance.h"

#include <chrono>
#include <cstdint>
#include <numeric>
#include <thread>
#include <vector>

#include "../test_performance_helpers.h"

using namespace themis::network;
using Clock = std::chrono::steady_clock;

namespace {

/// Returns elapsed microseconds between two time points.
double us(Clock::time_point t0, Clock::time_point t1) {
    return std::chrono::duration<double, std::micro>(t1 - t0).count();
}

/// Returns elapsed nanoseconds between two time points.
double ns(Clock::time_point t0, Clock::time_point t1) {
    return std::chrono::duration<double, std::nano>(t1 - t0).count();
}

} // anonymous namespace

// =============================================================================
// WireProtocolMetrics – record latency overhead
// =============================================================================

class WirePerfBenchmarkMetrics : public ::testing::Test {
protected:
    static constexpr int kIters = 10'000;
};

TEST_F(WirePerfBenchmarkMetrics, RecordLatencyMs_ThroughputTarget) {
    const int warmup = themis::test::BenchmarkPolicy::warmupIterations();
    const int runs = themis::test::BenchmarkPolicy::independentRuns();
    std::vector<double> per_call_samples;
    per_call_samples.reserve(static_cast<size_t>(runs));

    for (int run = 0; run < runs; ++run) {
        WireProtocolMetrics m;
        for (int i = 0; i < warmup; ++i) {
            m.recordLatencyMs(static_cast<double>(i % 100));
        }

        auto t0 = Clock::now();
        for (int i = 0; i < kIters; ++i) {
            m.recordLatencyMs(static_cast<double>(i % 100));
        }
        auto t1 = Clock::now();

        per_call_samples.push_back(us(t0, t1) / kIters);
        EXPECT_EQ(m.totalRequests(), static_cast<uint64_t>(warmup + kIters));
    }

    std::sort(per_call_samples.begin(), per_call_samples.end());
    const auto p95_index = static_cast<size_t>(
        std::ceil(0.95 * static_cast<double>(per_call_samples.size()))) - 1;
    const double p95_per_call_us = per_call_samples[std::min(p95_index, per_call_samples.size() - 1)];

    // Target: < 5 µs/call on CI hardware (generous for CI environments)
    EXPECT_LT(p95_per_call_us, 5.0)
        << "recordLatencyMs() is too slow: "
        << p95_per_call_us << " µs/call (target < 5 µs)";
}

TEST_F(WirePerfBenchmarkMetrics, RecordBytes_ThroughputTarget) {
    WireProtocolMetrics m;

    auto t0 = Clock::now();
    for (int i = 0; i < kIters; ++i)
        m.recordBytes(1024, 512);
    auto t1 = Clock::now();

    double per_call_ns = ns(t0, t1) / kIters;
    // Target: < 500 ns/call
    EXPECT_LT(per_call_ns, 500.0)
        << "recordBytes() is too slow: "
        << per_call_ns << " ns/call (target < 500 ns)";
}

TEST_F(WirePerfBenchmarkMetrics, Snapshot_LatencyWithFullWindow) {
    WireProtocolMetrics::Config cfg;
    cfg.max_samples = kIters;
    WireProtocolMetrics m(cfg);

    // Fill the entire window
    for (int i = 0; i < kIters; ++i)
        m.recordLatencyMs(static_cast<double>(i % 200));

    auto t0   = Clock::now();
    auto snap = m.snapshot();
    auto t1   = Clock::now();

    double snap_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // snapshot() must complete within 50 ms even for 10 000 samples on CI
    EXPECT_LT(snap_ms, 50.0)
        << "snapshot() too slow with " << kIters << " samples: "
        << snap_ms << " ms (target < 50 ms)";

    // Sanity: percentiles are in order
    EXPECT_LE(snap.latency.p50_ms, snap.latency.p99_ms);
}

TEST_F(WirePerfBenchmarkMetrics, SnapshotPercentileAccuracy) {
    WireProtocolMetrics m;
    // Insert [1 .. 100] uniformly
    for (int i = 1; i <= 100; ++i)
        m.recordLatencyMs(static_cast<double>(i));

    auto snap = m.snapshot();

    // p50 ≈ 50; p99 ≈ 99
    EXPECT_NEAR(snap.latency.p50_ms, 50.0, 2.0);
    EXPECT_NEAR(snap.latency.p99_ms, 99.0, 2.0);
}

// =============================================================================
// PayloadBufferPool – acquire/release throughput
// =============================================================================

class WirePerfBenchmarkPool : public ::testing::Test {
protected:
    static constexpr int kWarmup = 64;
    static constexpr int kIters  = 5'000;
    static constexpr size_t kSlabSize = 64 * 1024;
};

TEST_F(WirePerfBenchmarkPool, HitRateAfterWarmup) {
    PayloadBufferPool pool(kSlabSize, 64);

    // Warm-up: fill pool
    {
        std::vector<PayloadBufferPool::Handle> handles;
        handles.reserve(kWarmup);
        for (int i = 0; i < kWarmup; ++i)
            handles.push_back(pool.acquire());
    } // all returned

    // Measure post-warmup hit rate
    for (int i = 0; i < kIters; ++i) {
        auto buf = pool.acquire();
        buf->resize(512);
    }

    double rate = pool.hitRate();
    EXPECT_GE(rate, 0.85)
        << "Hit rate too low after warm-up: " << rate
        << " (target ≥ 85%)";
}

TEST_F(WirePerfBenchmarkPool, AcquireReleaseThroughput) {
    const int warmup = themis::test::BenchmarkPolicy::warmupIterations();
    const int runs = themis::test::BenchmarkPolicy::independentRuns();
    std::vector<double> per_call_samples;
    per_call_samples.reserve(static_cast<size_t>(runs));

    for (int run = 0; run < runs; ++run) {
        PayloadBufferPool pool(kSlabSize, 128);
        for (int i = 0; i < warmup; ++i) {
            auto b = pool.acquire();
            (void)b;
        }

        auto t0 = Clock::now();
        for (int i = 0; i < kIters; ++i) {
            auto buf = pool.acquire();
            buf->resize(512);
        }
        auto t1 = Clock::now();
        per_call_samples.push_back(us(t0, t1) / kIters);
    }

    std::sort(per_call_samples.begin(), per_call_samples.end());
    const auto p95_index = static_cast<size_t>(
        std::ceil(0.95 * static_cast<double>(per_call_samples.size()))) - 1;
    const double p95_per_call_us = per_call_samples[std::min(p95_index, per_call_samples.size() - 1)];

    // Target: < 10 µs/acquire (pool hit path, generous for CI)
    EXPECT_LT(p95_per_call_us, 10.0)
        << "Pool acquire() too slow: "
        << p95_per_call_us << " µs/call (target < 10 µs)";
}

TEST_F(WirePerfBenchmarkPool, RawAllocationBaseline) {
    // Measure raw heap allocation so we can compare with pool
    auto t0 = Clock::now();
    for (int i = 0; i < kIters; ++i) {
        auto buf = std::make_unique<std::vector<uint8_t>>();
        buf->reserve(kSlabSize);
        buf->resize(512);
    }
    auto t1 = Clock::now();

    double raw_us = us(t0, t1) / kIters;
    // We only log this for informational purposes; no strict assertion
    // (CI VMs vary too much)
    SUCCEED() << "Raw heap alloc baseline: " << raw_us << " µs/iter";
}

// =============================================================================
// CompressionAdvisor – advise() throughput
// =============================================================================

class WirePerfBenchmarkCompression : public ::testing::Test {};

TEST_F(WirePerfBenchmarkCompression, AdviseThroughput) {
    CompressionAdvisor advisor;

    static const std::array<size_t, 6> sizes = {
        100, 1024, 32 * 1024, 128 * 1024, 1024 * 1024, 16 * 1024 * 1024
    };

    constexpr int kIters = 100'000;
    auto t0 = Clock::now();
    for (int i = 0; i < kIters; ++i) {
        auto d = advisor.advise(sizes[i % sizes.size()]);
        (void)d;
    }
    auto t1 = Clock::now();

    double per_call_ns = ns(t0, t1) / kIters;
    // Target: < 1000 ns/call (very generous – should be < 10 ns on real HW)
    EXPECT_LT(per_call_ns, 1000.0)
        << "advise() too slow: " << per_call_ns << " ns/call (target < 1000 ns)";
}

// =============================================================================
// Combined throughput: record + snapshot cycle
// =============================================================================

class WirePerfBenchmarkCombined : public ::testing::Test {};

TEST_F(WirePerfBenchmarkCombined, RecordAndSnapshotCycle) {
    WireProtocolMetrics m;
    constexpr int kCycles = 100;
    constexpr int kRecordsPerCycle = 100;

    auto t0 = Clock::now();
    for (int c = 0; c < kCycles; ++c) {
        for (int r = 0; r < kRecordsPerCycle; ++r)
            m.recordLatencyMs(static_cast<double>(r));
        auto snap = m.snapshot();
        (void)snap;
    }
    auto t1 = Clock::now();

    double total_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double per_cycle_ms = total_ms / kCycles;

    // Each cycle (100 records + snapshot) must complete in < 10 ms on CI
    EXPECT_LT(per_cycle_ms, 10.0)
        << "record+snapshot cycle too slow: "
        << per_cycle_ms << " ms (target < 10 ms)";
}
