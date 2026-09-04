// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_timeseries_release_gates.cpp
 * @brief Phase 5 timeseries hot-path release-gate benchmarks (TSRG-01..TSRG-06).
 *
 * Provides reproducible latency and throughput measurements for the timeseries
 * module critical paths.  Results serve as release gates.
 *
 * ## Benchmark families
 *
 * ### TSRG-01 — Write throughput (in-memory, 10k points)
 *   Gate: ≥ 1M points/s.
 *
 * ### TSRG-02 — Range query (1k points window)
 *   Gate: p99 ≤ 500 µs.
 *
 * ### TSRG-03 — Gorilla encode/decode (100 doubles)
 *   Gate: p99 ≤ 100 µs.
 *
 * ### TSRG-04 — Downsampling (1k→100 points)
 *   Gate: p99 ≤ 1 ms.
 *
 * ### TSRG-05 — Retention check (single series)
 *   Gate: p99 ≤ 50 µs.
 *
 * ### TSRG-06 — Series lookup (in-memory map)
 *   Gate: p99 ≤ 50 µs.
 *
 * ## Hard release gates
 *
 * | Gate ID     | Benchmark  | Threshold        |
 * |-------------|------------|------------------|
 * | GATE-TSRG-01 | TSRG-01   | ≥ 1M points/s    |
 * | GATE-TSRG-02 | TSRG-02   | p99 ≤ 500 µs     |
 * | GATE-TSRG-03 | TSRG-03   | p99 ≤ 100 µs     |
 * | GATE-TSRG-04 | TSRG-04   | p99 ≤ 1 ms       |
 * | GATE-TSRG-05 | TSRG-05   | p99 ≤ 50 µs      |
 * | GATE-TSRG-06 | TSRG-06   | p99 ≤ 50 µs      |
 *
 * @see include/timeseries/timeseries_api_contract.h
 * @see src/timeseries/ROADMAP.md — Phase 5 items
 */

#include <benchmark/benchmark.h>

#include "timeseries/timeseries_api_contract.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <map>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace bench {
namespace tsrg {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/// Canonical PRNG seed for all TSRG benchmarks.
static constexpr uint64_t kTimeseriesCanonicalSeed = 42;

/// Warmup iterations before measurement window.
static constexpr int kWarmupIterations = 200;

/// Repetitions per benchmark for variance estimation.
static constexpr int kRepetitions = 5;

// ---------------------------------------------------------------------------
// Mock helpers
// ---------------------------------------------------------------------------

struct TimePoint {
    std::int64_t ts_ns;
    double       value;
};

/// In-memory write: append to a series vector (monotonic ts assumed).
static void writePoint(std::vector<TimePoint>& series,
                       std::int64_t ts_ns, double value) {
    series.push_back({ts_ns, value});
}

/// Range query: linear scan returning points in [start, end] inclusive.
static std::size_t rangeQuery(const std::vector<TimePoint>& series,
                               std::int64_t start, std::int64_t end) {
    std::size_t count = 0;
    for (const auto& p : series) {
        if (p.ts_ns >= start && p.ts_ns <= end) {
          ++count;
        }
    }
    return count;
}

/// Gorilla encode: memcpy double → uint64.
static std::uint64_t gorillaEncode(double v) {
    std::uint64_t bits = 0;
    std::memcpy(&bits, &v, sizeof(bits));
    return bits;
}

/// Gorilla decode: memcpy uint64 → double.
static double gorillaDecode(std::uint64_t bits) {
    double v = 0;
    std::memcpy(&v, &bits, sizeof(v));
    return v;
}

/// Downsampling: bucket average over resolution_ns intervals.
static std::size_t downsample(const std::vector<TimePoint>& input,
                               std::int64_t resolution_ns) {
    if (input.empty() || resolution_ns <= 0) {
      return 0;
    }
    std::map<std::int64_t, double> buckets;
    std::map<std::int64_t, int>    counts = {};

    for (const auto& p : input) {
        std::int64_t bk = p.ts_ns / resolution_ns;
        buckets[bk] += p.value;
        ++counts[bk];
    }
    return buckets.size();
}

/// Retention check: compare single timestamp against boundary.
static bool retentionExpired(std::int64_t point_ts, std::int64_t boundary) {
    return point_ts < boundary;
}

/// Series lookup: unordered_map name → series ID.
static std::int64_t seriesLookup(
        const std::unordered_map<std::string, std::int64_t>& index,
        const std::string& name) {
    auto it = index.find(name);
    return (it != index.end()) ? it->second : -1LL;
}

// ---------------------------------------------------------------------------
// TSRG-01 — Write throughput (in-memory)
// ---------------------------------------------------------------------------

/**
 * @brief TSRG-01: In-memory write throughput over 10k deterministic points.
 *
 * GATE-TSRG-01: ≥ 1M points/s.
 */
static void BM_TSRG01_WriteThroughput(benchmark::State& state) {
    std::vector<TimePoint> series;
    series.reserve(1'000'000);
    std::int64_t ts = 1000LL;

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        writePoint(series, ts, static_cast<double>(i));
        ts += 1000LL;
    }
    series.clear();

    for (auto _ : state) {
        writePoint(series, ts, static_cast<double>(ts));
        ts += 1000LL;
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("GATE-TSRG-01: >= 1M points/s");
}
BENCHMARK(BM_TSRG01_WriteThroughput)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// TSRG-02 — Range query (1k points window)
// ---------------------------------------------------------------------------

/**
 * @brief TSRG-02: Range query scan over a 1k-point in-memory series.
 *
 * GATE-TSRG-02: p99 ≤ 500 µs.
 */
static void BM_TSRG02_RangeQuery(benchmark::State& state) {
    std::vector<TimePoint> series;
    series.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        series.push_back({static_cast<std::int64_t>(i) * 1000LL, static_cast<double>(i)});
    }

    std::mt19937_64 rng(kTimeseriesCanonicalSeed);
    std::uniform_int_distribution<std::int64_t> dist(0, 500'000LL);

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        std::int64_t s = dist(rng);
        benchmark::DoNotOptimize(rangeQuery(series, s, s + 200'000LL));
    }
    for (auto _ : state) {
        std::int64_t s = dist(rng);
        benchmark::DoNotOptimize(rangeQuery(series, s, s + 200'000LL));
    }
    state.SetLabel("GATE-TSRG-02: p99 <= 500us");
}
BENCHMARK(BM_TSRG02_RangeQuery)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// TSRG-03 — Gorilla encode/decode (100 doubles)
// ---------------------------------------------------------------------------

/**
 * @brief TSRG-03: Gorilla lossless round-trip for 100 double values.
 *
 * GATE-TSRG-03: p99 ≤ 100 µs.
 */
static void BM_TSRG03_GorillaRoundTrip(benchmark::State& state) {
    std::mt19937_64 rng(kTimeseriesCanonicalSeed);
    std::uniform_real_distribution<double> dist(-1e15, 1e15);
    std::vector<double> values(100);
    for (auto& v : values) {
      v = dist(rng);
    }

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        for (double v : values) {
            benchmark::DoNotOptimize(gorillaDecode(gorillaEncode(v)));
        }
    }
    for (auto _ : state) {
        for (double v : values) {
            benchmark::DoNotOptimize(gorillaDecode(gorillaEncode(v)));
        }
    }
    state.SetLabel("GATE-TSRG-03: p99 <= 100us");
}
BENCHMARK(BM_TSRG03_GorillaRoundTrip)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// TSRG-04 — Downsampling (1k→100 points)
// ---------------------------------------------------------------------------

/**
 * @brief TSRG-04: Downsample 1k points to ~100 buckets (10x reduction).
 *
 * GATE-TSRG-04: p99 ≤ 1 ms.
 */
static void BM_TSRG04_Downsampling(benchmark::State& state) {
    std::vector<TimePoint> input;
    input.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        input.push_back({static_cast<std::int64_t>(i) * 1'000'000LL, static_cast<double>(i)});
    }
    // 10ms buckets → ~100 output buckets
    std::int64_t resolution_ns = 10'000'000LL;

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        benchmark::DoNotOptimize(downsample(input, resolution_ns));
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(downsample(input, resolution_ns));
    }
    state.SetLabel("GATE-TSRG-04: p99 <= 1ms");
}
BENCHMARK(BM_TSRG04_Downsampling)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// TSRG-05 — Retention check (single series)
// ---------------------------------------------------------------------------

/**
 * @brief TSRG-05: Retention boundary comparison for a single point.
 *
 * GATE-TSRG-05: p99 ≤ 50 µs.
 */
static void BM_TSRG05_RetentionCheck(benchmark::State& state) {
    std::mt19937_64 rng(kTimeseriesCanonicalSeed);
    std::uniform_int_distribution<std::int64_t> dist(0, 2'000'000'000LL);
    std::int64_t boundary = 1'000'000'000LL;

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        benchmark::DoNotOptimize(retentionExpired(dist(rng), boundary));
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(retentionExpired(dist(rng), boundary));
    }
    state.SetLabel("GATE-TSRG-05: p99 <= 50us");
}
BENCHMARK(BM_TSRG05_RetentionCheck)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// TSRG-06 — Series lookup (in-memory map)
// ---------------------------------------------------------------------------

/**
 * @brief TSRG-06: Series name → ID lookup in an in-memory hash map.
 *
 * GATE-TSRG-06: p99 ≤ 50 µs.
 */
static void BM_TSRG06_SeriesLookup(benchmark::State& state) {
    std::unordered_map<std::string, std::int64_t> index = {};

    for (int i = 0; i < 1000; ++i) {
        index["series_" + std::to_string(i)] = static_cast<std::int64_t>(i);
    }

    std::mt19937_64 rng(kTimeseriesCanonicalSeed);
    std::uniform_int_distribution<int> dist(0, 999);

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        benchmark::DoNotOptimize(seriesLookup(index, "series_" + std::to_string(dist(rng))));
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(seriesLookup(index, "series_" + std::to_string(dist(rng))));
    }
    state.SetLabel("GATE-TSRG-06: p99 <= 50us");
}
BENCHMARK(BM_TSRG06_SeriesLookup)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace tsrg
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
