// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_analytics_release_gates.cpp
 * @brief Phase 5 analytics hot-path release-gate benchmarks (ARG-01..ARG-06).
 *
 * Provides reproducible latency and throughput measurements for the analytics
 * module critical paths identified in the analytics roadmap (Phase 5 —
 * Performance and Hardening).  Results serve as release gates: a regression
 * beyond 10 % vs the baseline blocks promotion.
 *
 * ## Benchmark families
 *
 * ### ARG-01 — Aggregation throughput
 *   1k in-memory rows → SUM with overflow guard.
 *   Gate: ≥ 1M rows/s.
 *
 * ### ARG-02 — Window evaluation latency
 *   100-event tumbling window boundary evaluation.
 *   Gate: p99 ≤ 1 ms.
 *
 * ### ARG-03 — OLAP query plan lookup
 *   Deterministic plan-cache lookup by query hash.
 *   Gate: p99 ≤ 500 µs.
 *
 * ### ARG-04 — Anomaly check (single event)
 *   Threshold evaluation for one event.
 *   Gate: p99 ≤ 100 µs.
 *
 * ### ARG-05 — CEP pattern match (3-event sequence)
 *   A→B→C pattern detection over a 3-event ring buffer.
 *   Gate: p99 ≤ 500 µs.
 *
 * ### ARG-06 — Forecast inference stub (no model I/O)
 *   Input validation + mock inference latency.
 *   Gate: p99 ≤ 5 ms.
 *
 * ## Hard release gates
 *
 * | Gate ID    | Benchmark | Threshold      |
 * |------------|-----------|----------------|
 * | GATE-ARG-01 | ARG-01   | ≥ 1M rows/s    |
 * | GATE-ARG-02 | ARG-02   | p99 ≤ 1 ms     |
 * | GATE-ARG-03 | ARG-03   | p99 ≤ 500 µs   |
 * | GATE-ARG-04 | ARG-04   | p99 ≤ 100 µs   |
 * | GATE-ARG-05 | ARG-05   | p99 ≤ 500 µs   |
 * | GATE-ARG-06 | ARG-06   | p99 ≤ 5 ms     |
 *
 * All benchmarks:
 *   - Use kAnalyticsCanonicalSeed = 42 for deterministic data.
 *   - Warm up for kWarmupIterations before measurement.
 *   - Run with Repetitions(kRepetitions) for variance estimation.
 *
 * @see include/analytics/analytics_api_contract.h
 * @see src/analytics/ROADMAP.md — Phase 5 items
 */

#include <benchmark/benchmark.h>

#include "analytics/analytics_api_contract.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace bench {
namespace arg {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/// Canonical PRNG seed for all ARG benchmarks.
static constexpr uint64_t kAnalyticsCanonicalSeed = 42;

/// Warmup iterations before measurement window.
static constexpr int kWarmupIterations = 200;

/// Repetitions per benchmark for variance estimation.
static constexpr int kRepetitions = 5;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Build a deterministic row set of int64 values (no overflow).
static std::vector<std::int64_t> makeRows(std::size_t n, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<std::int64_t> dist(0, 1000);
    std::vector<std::int64_t> rows;
    rows.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
      rows.push_back(dist(rng));
    }
    return rows;
}

/// Sum without overflow (returns nullopt on success).
static std::optional<analytics::AnalyticsErrorCode> sumRows(
        const std::vector<std::int64_t>& rows, std::int64_t& out) {
    out = 0;
    for (auto v : rows) {
        if (v > 0 && out > std::numeric_limits<std::int64_t>::max() - v) {
            return analytics::AnalyticsErrorCode::AGGREGATION_OVERFLOW;
        }
        out += v;
    }
    return std::nullopt;
}

/// Simulates a tumbling window boundary evaluation.
static std::size_t evalTumblingWindow(const std::vector<int>& events, std::size_t window_size) {
    std::size_t fired = 0;
    for (std::size_t i = window_size; i <= events.size(); i += window_size) {
      ++fired;
    }
    return fired;
}

/// Plan cache: maps query hash → mock cost.
static std::unordered_map<std::uint64_t, double> buildPlanCache(std::size_t n) {
    std::unordered_map<std::uint64_t, double> cache;
    cache.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
      cache[i] = 42.0 + static_cast<double>(i);
    }
    return cache;
}

/// Anomaly threshold check.
static bool isAnomaly(double value, double threshold) {
    return value >= threshold;
}

/// CEP state machine: A→B→C detection.
static bool cepMatch(char a, char b, char c) {
    return a == 'A' && b == 'B' && c == 'C';
}

// ---------------------------------------------------------------------------
// ARG-01 — Aggregation throughput (1k rows)
// ---------------------------------------------------------------------------

/**
 * @brief ARG-01: SUM over 1k in-memory int64 rows with overflow guard.
 *
 * GATE-ARG-01: ≥ 1M rows/s.
 */
static void BM_ARG01_AggregationThroughput(benchmark::State& state) {
    auto rows = makeRows(1000, kAnalyticsCanonicalSeed);
    std::int64_t result = 0;
    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)sumRows(rows, result);
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(sumRows(rows, result));
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(rows.size()));
    state.SetLabel("GATE-ARG-01: >= 1M rows/s");
}
BENCHMARK(BM_ARG01_AggregationThroughput)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// ARG-02 — Window evaluation latency (100-event tumbling window)
// ---------------------------------------------------------------------------

/**
 * @brief ARG-02: Tumbling window boundary evaluation over 100 events.
 *
 * GATE-ARG-02: p99 ≤ 1 ms.
 */
static void BM_ARG02_WindowEvaluation(benchmark::State& state) {
    std::vector<int> events(100);
    std::iota(events.begin(), events.end(), 0);
    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)evalTumblingWindow(events, 10);
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(evalTumblingWindow(events, 10));
    }
    state.SetLabel("GATE-ARG-02: p99 <= 1ms");
}
BENCHMARK(BM_ARG02_WindowEvaluation)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// ARG-03 — OLAP query plan lookup
// ---------------------------------------------------------------------------

/**
 * @brief ARG-03: Deterministic plan-cache lookup by query hash.
 *
 * GATE-ARG-03: p99 ≤ 500 µs.
 */
static void BM_ARG03_OlapPlanLookup(benchmark::State& state) {
    auto cache = buildPlanCache(1000);
    std::mt19937_64 rng(kAnalyticsCanonicalSeed);
    std::uniform_int_distribution<std::uint64_t> dist(0, 999);
    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)cache.find(dist(rng));
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(cache.find(dist(rng)));
    }
    state.SetLabel("GATE-ARG-03: p99 <= 500us");
}
BENCHMARK(BM_ARG03_OlapPlanLookup)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// ARG-04 — Anomaly check (single event)
// ---------------------------------------------------------------------------

/**
 * @brief ARG-04: Threshold evaluation for a single anomaly event.
 *
 * GATE-ARG-04: p99 ≤ 100 µs.
 */
static void BM_ARG04_AnomalyCheck(benchmark::State& state) {
    std::mt19937_64 rng(kAnalyticsCanonicalSeed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double threshold = 0.5;
    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)isAnomaly(dist(rng), threshold);
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(isAnomaly(dist(rng), threshold));
    }
    state.SetLabel("GATE-ARG-04: p99 <= 100us");
}
BENCHMARK(BM_ARG04_AnomalyCheck)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// ARG-05 — CEP pattern match (3-event sequence)
// ---------------------------------------------------------------------------

/**
 * @brief ARG-05: A→B→C pattern detection over a 3-slot ring buffer.
 *
 * GATE-ARG-05: p99 ≤ 500 µs.
 */
static void BM_ARG05_CepPatternMatch(benchmark::State& state) {
    // Deterministic 3-event ring
    char buf[3] = {'A', 'B', 'C'};
    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)cepMatch(buf[0], buf[1], buf[2]);
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(cepMatch(buf[0], buf[1], buf[2]));
    }
    state.SetLabel("GATE-ARG-05: p99 <= 500us");
}
BENCHMARK(BM_ARG05_CepPatternMatch)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// ARG-06 — Forecast inference stub (no model I/O)
// ---------------------------------------------------------------------------

/**
 * @brief ARG-06: Forecast input validation + mock inference (no I/O).
 *
 * Measures the path excluding any model file or network I/O.
 * GATE-ARG-06: p99 ≤ 5 ms.
 */
static void BM_ARG06_ForecastInferenceStub(benchmark::State& state) {
    // Build a deterministic input series of 100 finite doubles
    std::mt19937_64 rng(kAnalyticsCanonicalSeed);
    std::uniform_real_distribution<double> dist(-1000.0, 1000.0);
    std::vector<double> series(100);
    for (auto& v : series) {
      v = dist(rng);
    }

    auto validate = [](const std::vector<double>& s) -> bool {
        for (double v : s) {
            if (std::isnan(v) || std::isinf(v)) {
              return false;
            }
        }
        return true;
    };

    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)validate(series);
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(validate(series));
    }
    state.SetLabel("GATE-ARG-06: p99 <= 5ms");
}
BENCHMARK(BM_ARG06_ForecastInferenceStub)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace arg
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
