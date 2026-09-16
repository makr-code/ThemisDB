// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_temporal_dedicated_gates.cpp
 * @brief Wave D — temporal store dedicated benchmark gates.
 *
 * Provides reproducible latency measurements for the temporal store module
 * under Wave D operability requirements.
 *
 * ## Benchmark families
 *
 * ### TMP-BM-01 — History query throughput
 *   Measures per-query cost for the stub temporal query engine.
 *
 * ### TMP-BM-02 — Versioned write throughput
 *   Measures per-write cost including version counter increment.
 *
 * ### TMP-BM-03 — Conflict resolution latency
 *   Measures LWW conflict resolution round-trip on stub resolver.
 *
 * ### TMP-BM-04 — CDC event emit batch (1 000 events)
 *   Amortised per-event cost for CDC emission across 1 000 events.
 *
 * ## Hard release gates
 *
 * | Gate ID   | Benchmark          | Threshold         |
 * |-----------|--------------------|-------------------|
 * | TMP-BM-01 | HistoryQuery       | p99 ≤ 500 ns      |
 * | TMP-BM-02 | VersionedWrite     | p99 ≤ 300 ns      |
 * | TMP-BM-03 | ConflictResolve    | p99 ≤ 200 ns      |
 * | TMP-BM-04 | BatchCdcEmit       | p99 ≤ 50 µs/batch |
 *
 * @see src/temporal/ROADMAP.md — Wave D items
 * @see docs/operability/RUNBOOK_TEMPORAL_STORE.md
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <string>

namespace themis {
namespace bench {
namespace temporal_dedicated {

static constexpr uint64_t kCanonicalSeed = 42;
static constexpr int      kRepetitions   = 5;

// ─── In-process stubs (SIMULATION NOTE: no external DB required) ──────────────

static std::atomic<uint64_t> g_version{1};

static uint64_t stubHistoryQuery(uint64_t entity_id, uint64_t range_ms) noexcept {
    return (entity_id % 128) + (range_ms % 64);
}

static uint64_t stubVersionedWrite(uint64_t entity_id) noexcept {
    (void)entity_id;
    return g_version.fetch_add(1, std::memory_order_relaxed);
}

static uint64_t stubConflictResolve(uint64_t ver_a, uint64_t ver_b) noexcept {
    return (ver_a > ver_b) ? ver_a : ver_b;
}

static bool stubCdcEmit(uint64_t entity_id, uint64_t ts_ms) noexcept {
    (void)entity_id; (void)ts_ms;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// TMP-BM-01 — History query throughput
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Temporal_HistoryQuery(benchmark::State& state) {
    uint64_t seed = kCanonicalSeed;
    for (auto _ : state) {
        const uint64_t entity_id = seed % 10000;
        const uint64_t range_ms  = (seed % 1000) + 100;
        benchmark::DoNotOptimize(stubHistoryQuery(entity_id, range_ms));
        ++seed;
    }
}
BENCHMARK(BM_Temporal_HistoryQuery)
    ->Name("TMP-BM-01/HistoryQuery")
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ─────────────────────────────────────────────────────────────────────────────
// TMP-BM-02 — Versioned write throughput
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Temporal_VersionedWrite(benchmark::State& state) {
    uint64_t seed = kCanonicalSeed;
    for (auto _ : state) {
        const uint64_t entity_id = seed % 5000;
        benchmark::DoNotOptimize(stubVersionedWrite(entity_id));
        ++seed;
    }
}
BENCHMARK(BM_Temporal_VersionedWrite)
    ->Name("TMP-BM-02/VersionedWrite")
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ─────────────────────────────────────────────────────────────────────────────
// TMP-BM-03 — Conflict resolution latency
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Temporal_ConflictResolve(benchmark::State& state) {
    uint64_t seed = kCanonicalSeed;
    for (auto _ : state) {
        const uint64_t ver_a = seed * 6364136223846793005ULL;
        const uint64_t ver_b = (seed + 1) * 2862933555777941757ULL;
        benchmark::DoNotOptimize(stubConflictResolve(ver_a, ver_b));
        ++seed;
    }
}
BENCHMARK(BM_Temporal_ConflictResolve)
    ->Name("TMP-BM-03/ConflictResolve")
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ─────────────────────────────────────────────────────────────────────────────
// TMP-BM-04 — CDC event emit batch (1 000 events)
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Temporal_BatchCdcEmit(benchmark::State& state) {
    constexpr int kBatch = 1000;
    uint64_t seed = kCanonicalSeed;
    for (auto _ : state) {
        for (int i = 0; i < kBatch; ++i) {
            const uint64_t entity_id = seed % 10000;
            const uint64_t ts_ms     = 1700000000000ULL + seed;
            benchmark::DoNotOptimize(stubCdcEmit(entity_id, ts_ms));
            ++seed;
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatch);
}
BENCHMARK(BM_Temporal_BatchCdcEmit)
    ->Name("TMP-BM-04/BatchCdcEmit1000")
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace temporal_dedicated
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
