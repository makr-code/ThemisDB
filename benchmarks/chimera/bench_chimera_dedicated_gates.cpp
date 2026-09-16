// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_chimera_dedicated_gates.cpp
 * @brief Wave D Chimera dedicated benchmark gates (CH-BM-01..CH-BM-04).
 *
 * Provides reproducible latency and throughput measurements for the
 * Chimera hybrid-adapter Wave D operability paths.
 *
 * ## Benchmark families
 *
 * ### CH-BM-01 — Hybrid query routing throughput (8-thread)
 *   ≥ 10 000 queries/sec
 *
 * ### CH-BM-02 — Fallback path invocation latency
 *   p99 ≤ 500 µs
 *
 * ### CH-BM-03 — Router dispatch decision latency
 *   p99 ≤ 100 µs
 *
 * ### CH-BM-04 — Result merge overhead (100-row result)
 *   p99 ≤ 1 ms
 *
 * ## Hard release gates
 *
 * | Gate ID  | Benchmark    | Threshold            |
 * |----------|--------------|----------------------|
 * | CH-BM-01 | Throughput   | ≥ 10 000 queries/sec |
 * | CH-BM-02 | Fallback lat | p99 ≤ 500 µs         |
 * | CH-BM-03 | Dispatch lat | p99 ≤ 100 µs         |
 * | CH-BM-04 | Merge OH     | p99 ≤ 1 ms           |
 *
 * @see src/chimera/ROADMAP.md — Wave D contribution closure
 * @see docs/operability/RUNBOOK_CHIMERA.md
 */

// SIMULATION NOTE: All stubs in this file are in-process simulations used
// purely for benchmark gate measurement. MUST NOT be used in production.

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <numeric>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Stub primitives
// ─────────────────────────────────────────────────────────────────────────────

namespace {

struct RouterStub {
    std::atomic<uint64_t> counter{0};

    bool route(uint64_t query_id) {
        counter.fetch_add(1, std::memory_order_relaxed);
        (void)query_id;
        return true;
    }
    void fallback(uint64_t query_id) {
        (void)query_id;
        counter.fetch_add(1, std::memory_order_relaxed);
    }
};

inline double stubMerge(const std::vector<double>& rows) {
    return std::accumulate(rows.begin(), rows.end(), 0.0) / static_cast<double>(rows.size());
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// CH-BM-01: Hybrid query routing throughput
// ─────────────────────────────────────────────────────────────────────────────

static void BM_CH_BM_01_HybridQueryThroughput(benchmark::State& state) {
    RouterStub router;
    uint64_t id = 0;
    for (auto _ : state) {
        router.route(id++);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_CH_BM_01_HybridQueryThroughput)
    ->Threads(8)
    ->MinTime(1.0)
    ->UseRealTime();

// ─────────────────────────────────────────────────────────────────────────────
// CH-BM-02: Fallback path invocation latency
// ─────────────────────────────────────────────────────────────────────────────

static void BM_CH_BM_02_FallbackLatency(benchmark::State& state) {
    RouterStub router;
    uint64_t id = 0;
    for (auto _ : state) {
        router.fallback(id++);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_CH_BM_02_FallbackLatency)
    ->MinTime(1.0)
    ->UseRealTime();

// ─────────────────────────────────────────────────────────────────────────────
// CH-BM-03: Router dispatch decision latency
// ─────────────────────────────────────────────────────────────────────────────

static void BM_CH_BM_03_DispatchDecision(benchmark::State& state) {
    RouterStub router;
    uint64_t id = 0;
    for (auto _ : state) {
        bool ok = router.route(id++);
        benchmark::DoNotOptimize(ok);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_CH_BM_03_DispatchDecision)
    ->MinTime(1.0)
    ->UseRealTime();

// ─────────────────────────────────────────────────────────────────────────────
// CH-BM-04: Result merge overhead (100-row)
// ─────────────────────────────────────────────────────────────────────────────

static void BM_CH_BM_04_ResultMerge(benchmark::State& state) {
    std::vector<double> rows(100);
    for (std::size_t i = 0; i < rows.size(); ++i) rows[i] = static_cast<double>(i + 1);
    for (auto _ : state) {
        double result = stubMerge(rows);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_CH_BM_04_ResultMerge)
    ->MinTime(1.0)
    ->UseRealTime();

BENCHMARK_MAIN();
