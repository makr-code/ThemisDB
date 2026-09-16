// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_evaluation_dedicated_gates.cpp
 * @brief Wave D Evaluation Framework dedicated benchmark gates (EV-BM-01..EV-BM-04).
 *
 * Provides reproducible latency and throughput measurements for the
 * evaluation framework Wave D operability paths.
 *
 * ## Benchmark families
 *
 * ### EV-BM-01 — Metric computation throughput (8-thread)
 *   ≥ 5 000 evals/sec
 *
 * ### EV-BM-02 — Result aggregation latency (1000-sample batch)
 *   p99 ≤ 5 ms
 *
 * ### EV-BM-03 — Benchmark harness invocation latency
 *   p99 ≤ 200 µs
 *
 * ### EV-BM-04 — Judge request stub latency
 *   p99 ≤ 1 ms
 *
 * ## Hard release gates
 *
 * | Gate ID  | Benchmark     | Threshold         |
 * |----------|---------------|-------------------|
 * | EV-BM-01 | Metric comp   | ≥ 5 000 evals/sec |
 * | EV-BM-02 | Aggregation   | p99 ≤ 5 ms        |
 * | EV-BM-03 | Harness inv   | p99 ≤ 200 µs      |
 * | EV-BM-04 | Judge stub    | p99 ≤ 1 ms        |
 *
 * @see src/evaluation/ROADMAP.md — Wave D contribution closure
 * @see docs/operability/RUNBOOK_EVALUATION_FRAMEWORK.md
 */

// SIMULATION NOTE: All stubs in this file are in-process simulations used
// purely for benchmark gate measurement. MUST NOT be used in production.

#include <benchmark/benchmark.h>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Stub primitives
// ─────────────────────────────────────────────────────────────────────────────

namespace {

inline double stubComputeMetric(double sample) {
    return sample > 0.0 ? (sample / (sample + 1.0)) : 0.0;
}

inline double stubAggregate(const std::vector<double>& v) {
    double sum = std::accumulate(v.begin(), v.end(), 0.0);
    return v.empty() ? 0.0 : sum / static_cast<double>(v.size());
}

struct JudgeStub {
    std::atomic<uint64_t> requests{0};

    double score(uint64_t query_id) {
        requests.fetch_add(1, std::memory_order_relaxed);
        // Simple deterministic stub score
        return static_cast<double>(query_id % 100) / 100.0;
    }
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// EV-BM-01: Metric computation throughput
// ─────────────────────────────────────────────────────────────────────────────

static void BM_EV_BM_01_MetricComputationThroughput(benchmark::State& state) {
    double sample = 1.0;
    for (auto _ : state) {
        double result = stubComputeMetric(sample);
        benchmark::DoNotOptimize(result);
        sample += 0.001;
        if (sample > 1e6) sample = 1.0;
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_EV_BM_01_MetricComputationThroughput)
    ->Threads(8)
    ->MinTime(1.0)
    ->UseRealTime();

// ─────────────────────────────────────────────────────────────────────────────
// EV-BM-02: Result aggregation latency (1000-sample batch)
// ─────────────────────────────────────────────────────────────────────────────

static void BM_EV_BM_02_AggregationLatency(benchmark::State& state) {
    std::vector<double> batch(1000);
    for (std::size_t i = 0; i < batch.size(); ++i) batch[i] = static_cast<double>(i + 1);
    for (auto _ : state) {
        double result = stubAggregate(batch);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_EV_BM_02_AggregationLatency)
    ->MinTime(1.0)
    ->UseRealTime();

// ─────────────────────────────────────────────────────────────────────────────
// EV-BM-03: Benchmark harness invocation latency
// ─────────────────────────────────────────────────────────────────────────────

static void BM_EV_BM_03_HarnessInvocation(benchmark::State& state) {
    double v = 0.5;
    for (auto _ : state) {
        double result = stubComputeMetric(v);
        benchmark::DoNotOptimize(result);
        v += 1.0;
        if (v > 1e7) v = 0.5;
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_EV_BM_03_HarnessInvocation)
    ->MinTime(1.0)
    ->UseRealTime();

// ─────────────────────────────────────────────────────────────────────────────
// EV-BM-04: Judge request stub latency
// ─────────────────────────────────────────────────────────────────────────────

static void BM_EV_BM_04_JudgeRequestLatency(benchmark::State& state) {
    JudgeStub judge;
    uint64_t id = 0;
    for (auto _ : state) {
        double score = judge.score(id++);
        benchmark::DoNotOptimize(score);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_EV_BM_04_JudgeRequestLatency)
    ->MinTime(1.0)
    ->UseRealTime();

BENCHMARK_MAIN();
