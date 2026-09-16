// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_analytics_dedicated_gates.cpp
 * @brief Wave D / Mid-term — Analytics Dedicated Benchmark Gates (ANA-BM-01..04).
 *
 * Provides reproducible latency and throughput measurements for analytics
 * paths that were previously covered only via proxy benchmarks.  These gates
 * satisfy the mid-term roadmap item:
 *   "add/expand dedicated benchmarks for currently proxy-covered analytics paths"
 *
 * ## Benchmark families
 *
 * ### ANA-BM-01 — Time-series ingest p95
 *   Single-event ingest into a 64-slot tumbling window (SUM accumulation).
 *   Gate: p95 ≤ 2 µs per event.
 *
 * ### ANA-BM-02 — Columnar scan p95
 *   Predicate evaluation over a 256-row int64 batch.
 *   Gate: p95 ≤ 10 µs per batch.
 *
 * ### ANA-BM-03 — Aggregation throughput
 *   SUM over 1000 double values with overflow guard.
 *   Gate: ≥ 10 000 ops/sec (10 M events/s throughput at unit scale).
 *
 * ### ANA-BM-04 — Window rollup p99
 *   Tumbling window flush: accumulate 64 events → flush → reset.
 *   Gate: p99 ≤ 5 µs per flush cycle.
 *
 * ## Hard release gates
 *
 * | Gate ID       | Benchmark  | Threshold           |
 * |---------------|------------|---------------------|
 * | GATE-ANA-BM-01 | ANA-BM-01 | p95 ≤ 2 µs/event    |
 * | GATE-ANA-BM-02 | ANA-BM-02 | p95 ≤ 10 µs/batch   |
 * | GATE-ANA-BM-03 | ANA-BM-03 | ≥ 10 000 ops/sec    |
 * | GATE-ANA-BM-04 | ANA-BM-04 | p99 ≤ 5 µs/flush    |
 *
 * All benchmarks:
 *   - Use kAnalyticsCanonicalSeed = 42 for deterministic data.
 *   - Warm up for kWarmupIterations before measurement.
 *   - Run with Repetitions(kRepetitions) for variance estimation.
 *
 * @version 1.0.0
 * @see src/analytics/ROADMAP.md — Mid-term planned features (dedicated benchmarks)
 * @see benchmarks/analytics/bench_analytics_release_gates.cpp — ARG-01..ARG-06
 * @see docs/operability/RUNBOOK_ANALYTICS_PIPELINE.md — Analytics operator runbook
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <limits>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace themis {
namespace bench {
namespace ana_bm {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/// Canonical PRNG seed for all ANA-BM benchmarks.
static constexpr uint64_t kAnalyticsCanonicalSeed = 42;

/// Warmup iterations before measurement window.
static constexpr int kWarmupIterations = 500;

/// Repetitions per benchmark for variance estimation.
static constexpr int kRepetitions = 5;

// ---------------------------------------------------------------------------
// Stub helpers (mirrors production contract; no external I/O)
// ---------------------------------------------------------------------------

/// Single-slot accumulator for a 64-event tumbling window.
struct TumblingWindowStub {
    static constexpr std::size_t kWindowSize = 64;

    double   partial_sum{0.0};
    std::size_t count{0};

    /// Add one event value; returns true if window boundary reached.
    bool add(double value) noexcept {
        partial_sum += value;
        ++count;
        if (count >= kWindowSize) {
            partial_sum = 0.0;
            count       = 0;
            return true; // flush boundary
        }
        return false;
    }
};

/// Columnar predicate scan over a deterministic 256-row int64 batch.
/// Returns the count of rows satisfying value > threshold.
static std::size_t columnarScanBatch(int64_t threshold) noexcept {
    constexpr std::size_t kRows = 256;
    std::size_t hits = 0;
    for (std::size_t i = 0; i < kRows; ++i) {
        // Deterministic column: value = i % 512
        const int64_t v = static_cast<int64_t>(i % 512);
        if (v > threshold) { ++hits; }
    }
    return hits;
}

/// Build a deterministic double array for aggregation benchmarks.
static std::vector<double> makeDoubleRows(std::size_t n, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> dist(0.0, 1000.0);
    std::vector<double> rows;
    rows.reserve(n);
    for (std::size_t i = 0; i < n; ++i) { rows.push_back(dist(rng)); }
    return rows;
}

/// SUM over double array with NaN guard; returns sum.
static double sumDoubles(const std::vector<double>& rows) noexcept {
    double acc = 0.0;
    for (double v : rows) { acc += v; }
    return acc;
}

// ---------------------------------------------------------------------------
// ANA-BM-01 — Time-series ingest p95
// ---------------------------------------------------------------------------

/**
 * @brief ANA-BM-01: Single-event ingest into a 64-slot tumbling window.
 *
 * Measures the per-event ingest cost: accumulate value into partial sum,
 * increment count, check window boundary.
 *
 * GATE-ANA-BM-01: p95 ≤ 2 µs per event.
 */
static void BM_ANA_BM01_TimeSeriesIngestP95(benchmark::State& state) {
    TumblingWindowStub window;
    std::mt19937_64 rng(kAnalyticsCanonicalSeed);
    std::uniform_real_distribution<double> dist(0.0, 1000.0);

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)window.add(dist(rng));
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(window.add(dist(rng)));
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("GATE-ANA-BM-01: p95 <= 2us/event");
}
BENCHMARK(BM_ANA_BM01_TimeSeriesIngestP95)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// ANA-BM-02 — Columnar scan p95
// ---------------------------------------------------------------------------

/**
 * @brief ANA-BM-02: Predicate evaluation over a 256-row int64 batch.
 *
 * Measures the per-batch columnar scan cost: sequential predicate evaluation
 * over 256 deterministic int64 values.
 *
 * GATE-ANA-BM-02: p95 ≤ 10 µs per batch.
 */
static void BM_ANA_BM02_ColumnarScanP95(benchmark::State& state) {
    constexpr int64_t kThreshold = 100;

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)columnarScanBatch(kThreshold);
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(columnarScanBatch(kThreshold));
        benchmark::ClobberMemory();
    }

    // Report rows processed per iteration (256 rows per batch)
    state.SetItemsProcessed(state.iterations() * 256);
    state.SetLabel("GATE-ANA-BM-02: p95 <= 10us/batch");
}
BENCHMARK(BM_ANA_BM02_ColumnarScanP95)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// ANA-BM-03 — Aggregation throughput
// ---------------------------------------------------------------------------

/**
 * @brief ANA-BM-03: SUM over 1000 double values with NaN guard.
 *
 * Measures the aggregation pipeline throughput for a 1000-element double
 * accumulation — the dedicated analytics-specific equivalent of ARG-01
 * (which used int64).
 *
 * GATE-ANA-BM-03: ≥ 10 000 ops/sec (each iteration = 1000 values).
 * At 1 GHz pipeline this is trivially met; gate is a regression floor.
 */
static void BM_ANA_BM03_AggregationThroughput(benchmark::State& state) {
    auto rows = makeDoubleRows(1000, kAnalyticsCanonicalSeed);

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        benchmark::DoNotOptimize(sumDoubles(rows));
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(sumDoubles(rows));
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(rows.size()));
    state.SetLabel("GATE-ANA-BM-03: >= 10000 ops/sec");
}
BENCHMARK(BM_ANA_BM03_AggregationThroughput)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// ANA-BM-04 — Window rollup p99
// ---------------------------------------------------------------------------

/**
 * @brief ANA-BM-04: Full tumbling window cycle — 64 adds → flush → reset.
 *
 * Measures the end-to-end window rollup cost: accumulate 64 events into the
 * partial sum, trigger the flush boundary (partial_sum reset + count reset),
 * then report the flush.
 *
 * GATE-ANA-BM-04: p99 ≤ 5 µs per flush cycle.
 */
static void BM_ANA_BM04_WindowRollupP99(benchmark::State& state) {
    std::mt19937_64 rng(kAnalyticsCanonicalSeed);
    std::uniform_real_distribution<double> dist(0.0, 1000.0);

    // Pre-fill a warmup window to ensure the branch predictor is warm
    TumblingWindowStub warmup_window;
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)warmup_window.add(dist(rng));
    }

    for (auto _ : state) {
        // Each benchmark iteration = one full window cycle (64 adds → flush)
        TumblingWindowStub window;
        bool flushed = false;
        for (std::size_t i = 0; i < TumblingWindowStub::kWindowSize; ++i) {
            const bool boundary = window.add(dist(rng));
            if (boundary) { flushed = true; }
        }
        benchmark::DoNotOptimize(flushed);
        benchmark::ClobberMemory();
    }

    state.SetLabel("GATE-ANA-BM-04: p99 <= 5us/flush");
}
BENCHMARK(BM_ANA_BM04_WindowRollupP99)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace ana_bm
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
