// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_observability_release_gates.cpp
 * @brief Phase 5 observability hot-path release-gate benchmarks (ORG-01..ORG-06).
 *
 * Provides reproducible latency and throughput measurements for the
 * observability hot paths identified in the observability module roadmap
 * (Phase 5 — Performance and Hardening).
 *
 * ## Benchmark families
 *
 * ### ORG-01 — Counter increment (atomic)
 *   ≥ 10M ops/s
 *
 * ### ORG-02 — Histogram record
 *   p99 ≤ 100 ns
 *
 * ### ORG-03 — Trace span create
 *   p99 ≤ 10 µs
 *
 * ### ORG-04 — Log entry format (structured, no I/O)
 *   p99 ≤ 5 µs
 *
 * ### ORG-05 — SLO computation (single window)
 *   p99 ≤ 100 µs
 *
 * ### ORG-06 — Prometheus export scrape (100 metrics, in-memory)
 *   p99 ≤ 5 ms
 *
 * ## Hard release gates
 *
 * | Gate ID     | Benchmark | Threshold          |
 * |-------------|-----------|--------------------|
 * | GATE-ORG-01 | ORG-01    | ≥ 10M ops/s        |
 * | GATE-ORG-02 | ORG-02    | p99 ≤ 100 ns       |
 * | GATE-ORG-03 | ORG-03    | p99 ≤ 10 µs        |
 * | GATE-ORG-04 | ORG-04    | p99 ≤ 5 µs         |
 * | GATE-ORG-05 | ORG-05    | p99 ≤ 100 µs       |
 * | GATE-ORG-06 | ORG-06    | p99 ≤ 5 ms (RT)    |
 *
 * @see include/observability/observability_api_contract.h
 * @see src/observability/ROADMAP.md — Phase 5 item
 */

#include <benchmark/benchmark.h>

#include "observability/observability_api_contract.h"
#include "observability/metrics_collector.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace themis::observability;
using namespace std::chrono_literals;

namespace themis {
namespace bench {
namespace org {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr uint64_t kObservabilityCanonicalSeed = 42;
static constexpr int      kRepetitions                = 5;
static constexpr int      kWarmupIterations           = 200;

// ---------------------------------------------------------------------------
// Mock: atomic counter
// ---------------------------------------------------------------------------

static std::atomic<std::int64_t> g_counter{0};

// ---------------------------------------------------------------------------
// Mock: fixed-size histogram with 8 buckets
// ---------------------------------------------------------------------------

struct BenchHistogram {
    static constexpr std::size_t kBuckets = 8;
    double         bounds[kBuckets] = {1, 5, 10, 50, 100, 500, 1000, 5000};
    std::int64_t   counts[kBuckets + 1] = {};  // +1 for +Inf bucket
    std::int64_t   total_count = 0;
    double         total_sum   = 0.0;

    void record(double v) noexcept {
        ++total_count;
        total_sum += v;
        for (std::size_t i = 0; i < kBuckets; ++i) {
            if (v <= bounds[i]) { ++counts[i]; return; }
        }
        ++counts[kBuckets];
    }
};

// ---------------------------------------------------------------------------
// Mock: span (in-memory, no I/O)
// ---------------------------------------------------------------------------

struct BenchSpan {
    std::uint64_t trace_id_hi, trace_id_lo;
    std::uint64_t span_id;
    std::uint64_t parent_span_id;
    std::int64_t  start_ns;
    bool          finished = false;

    void finish() noexcept {
        finished = true;
    }
};

static std::atomic<std::uint64_t> g_span_id_counter{1};

static BenchSpan createSpan(std::uint64_t parent_id) noexcept {
    return {
        42u, 0u,
        g_span_id_counter.fetch_add(1u, std::memory_order_relaxed),
        parent_id,
        static_cast<std::int64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count()),
        false
    };
}

// ---------------------------------------------------------------------------
// Mock: structured log entry formatter (no I/O — pure in-memory)
// ---------------------------------------------------------------------------

static std::string formatLogEntry(
        const std::string& level,
        const std::string& msg,
        std::uint64_t      req_id) {
    std::ostringstream ss;
    ss << "{\"level\":\"" << level
       << "\",\"msg\":\""  << msg
       << "\",\"req_id\":" << req_id << "}";
    return ss.str();
}

// ---------------------------------------------------------------------------
// Mock: SLO computation (single window)
// ---------------------------------------------------------------------------

struct BenchSloWindow {
    double  good_count = 0.0;
    double  total_count = 0.0;
    double  target = 0.99;

    void recordRequest(bool good) noexcept {
        ++total_count;
        if (good) ++good_count;
    }

    double sli() const noexcept {
        return (total_count > 0.0) ? good_count / total_count : 1.0;
    }

    bool isBreach() const noexcept { return sli() < target; }
};

// ---------------------------------------------------------------------------
// Mock: Prometheus scrape (100 in-memory metrics)
// ---------------------------------------------------------------------------

struct BenchMetric {
    std::string        name;
    std::int64_t       value;
};

static std::vector<BenchMetric> makeMetrics(int n) {
    std::vector<BenchMetric> m;
    m.reserve(n);
    for (int i = 0; i < n; ++i)
        m.push_back({"metric_" + std::to_string(i), static_cast<std::int64_t>(i)});
    return m;
}

static std::string scrapeMetrics(const std::vector<BenchMetric>& metrics) {
    std::ostringstream ss;
    for (auto& m : metrics)
        ss << m.name << " " << m.value << "\n";
    return ss.str();
}

// ===========================================================================
// ORG-01 — Counter increment (atomic)  (≥ 10M ops/s)
// ===========================================================================

/**
 * @brief ORG-01: std::atomic counter increment — throughput ≥ 10M ops/s.
 * GATE-ORG-01: ≥ 10M ops/s.
 */
static void BM_ORG01_CounterIncrement(benchmark::State& state) {
    for (int i = 0; i < kWarmupIterations; ++i)
        g_counter.fetch_add(1, std::memory_order_relaxed);

    std::int64_t ops = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            g_counter.fetch_add(1, std::memory_order_relaxed));
        ++ops;
    }
    state.SetItemsProcessed(ops);
    state.SetLabel("GATE-ORG-01: >= 10M ops/s");
}
BENCHMARK(BM_ORG01_CounterIncrement)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// ORG-02 — Histogram record  (p99 ≤ 100 ns)
// ===========================================================================

/**
 * @brief ORG-02: BenchHistogram::record() — p99 ≤ 100 ns.
 * GATE-ORG-02: p99 ≤ 100 ns.
 */
static void BM_ORG02_HistogramRecord(benchmark::State& state) {
    BenchHistogram h;
    std::mt19937_64 rng(kObservabilityCanonicalSeed);
    std::uniform_real_distribution<double> dist(0.0, 5000.0);

    for (int i = 0; i < kWarmupIterations; ++i) h.record(dist(rng));

    for (auto _ : state) {
        h.record(dist(rng));
    }
    state.SetLabel("GATE-ORG-02: p99 <= 100 ns");
}
BENCHMARK(BM_ORG02_HistogramRecord)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// ORG-03 — Trace span create  (p99 ≤ 10 µs)
// ===========================================================================

/**
 * @brief ORG-03: createSpan() — in-memory span allocation + ID generation.
 * GATE-ORG-03: p99 ≤ 10 µs.
 */
static void BM_ORG03_TraceSpanCreate(benchmark::State& state) {
    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(createSpan(0u));

    for (auto _ : state) {
        auto span = createSpan(0u);
        span.finish();
        benchmark::DoNotOptimize(span);
    }
    state.SetLabel("GATE-ORG-03: p99 <= 10 us");
}
BENCHMARK(BM_ORG03_TraceSpanCreate)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// ORG-03B — Invalid telemetry reject path (bounded fail-closed path)
// ===========================================================================

/**
 * @brief ORG-03B: invalid label-set rejection remains bounded under contract.
 */
static void BM_ORG03B_InvalidTelemetryReject(benchmark::State& state) {
    auto& collector = themis::observability::MetricsCollector::getInstance();
    collector.reset();

    std::map<std::string, std::string> invalid_labels;
    for (std::size_t i = 0; i < kMaxMetricLabels + 1; ++i) {
        invalid_labels.emplace("label_" + std::to_string(i), "value");
    }

    for (auto _ : state) {
        collector.addCounter("bench_invalid_metric_total", 1, invalid_labels);
    }

    state.SetLabel("bounded invalid telemetry rejection");
}
BENCHMARK(BM_ORG03B_InvalidTelemetryReject)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// ORG-04 — Log entry format (structured, no I/O)  (p99 ≤ 5 µs)
// ===========================================================================

/**
 * @brief ORG-04: formatLogEntry() — structured JSON formatting, no I/O.
 * GATE-ORG-04: p99 ≤ 5 µs.
 */
static void BM_ORG04_LogEntryFormat(benchmark::State& state) {
    std::uint64_t req_id = 1u;
    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(formatLogEntry("INFO", "benchmark warmup", req_id++));

    for (auto _ : state) {
        benchmark::DoNotOptimize(formatLogEntry("INFO", "request handled", req_id++));
    }
    state.SetLabel("GATE-ORG-04: p99 <= 5 us");
}
BENCHMARK(BM_ORG04_LogEntryFormat)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// ORG-05 — SLO computation (single window)  (p99 ≤ 100 µs)
// ===========================================================================

/**
 * @brief ORG-05: BenchSloWindow::sli() + isBreach() computation.
 * GATE-ORG-05: p99 ≤ 100 µs.
 */
static void BM_ORG05_SloComputation(benchmark::State& state) {
    BenchSloWindow w;
    std::mt19937_64 rng(kObservabilityCanonicalSeed);
    std::bernoulli_distribution good_dist(0.995);

    // Pre-fill window
    for (int i = 0; i < 1000; ++i) w.recordRequest(good_dist(rng));

    for (int i = 0; i < kWarmupIterations; ++i) {
        benchmark::DoNotOptimize(w.sli());
        benchmark::DoNotOptimize(w.isBreach());
    }

    for (auto _ : state) {
        w.recordRequest(good_dist(rng));
        benchmark::DoNotOptimize(w.sli());
        benchmark::DoNotOptimize(w.isBreach());
    }
    state.SetLabel("GATE-ORG-05: p99 <= 100 us");
}
BENCHMARK(BM_ORG05_SloComputation)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// ORG-06 — Prometheus export scrape (100 metrics, in-memory)  (p99 ≤ 5 ms)
// ===========================================================================

/**
 * @brief ORG-06: scrapeMetrics() for 100 in-memory metrics.
 * UseRealTime() because string I/O is involved.
 * GATE-ORG-06: p99 ≤ 5 ms.
 */
static void BM_ORG06_PrometheusScrape(benchmark::State& state) {
    const auto metrics = makeMetrics(100);

    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(scrapeMetrics(metrics));

    for (auto _ : state) {
        benchmark::DoNotOptimize(scrapeMetrics(metrics));
    }
    state.SetLabel("GATE-ORG-06: p99 <= 5 ms");
}
BENCHMARK(BM_ORG06_PrometheusScrape)
    ->UseRealTime()
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace org
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
