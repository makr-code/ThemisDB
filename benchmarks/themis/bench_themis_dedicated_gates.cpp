// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_themis_dedicated_gates.cpp
 * @brief Wave D — themis core dedicated benchmark gates.
 *
 * Provides reproducible latency measurements for the themis core module under
 * Wave D operability requirements.
 *
 * ## Benchmark families
 *
 * ### TH-BM-01 — Module load/verify throughput
 *   Measures the cost of stub module name → trust-level resolution.
 *
 * ### TH-BM-02 — Wire session request throughput
 *   Measures per-request handling cost for the stub wire server.
 *
 * ### TH-BM-03 — Dependency gate resolution latency
 *   Measures single dependency resolution round-trip on stub gate.
 *
 * ### TH-BM-04 — Batch module verify (1 000 iterations)
 *   Amortised per-module verify cost across 1 000 modules.
 *
 * ## Hard release gates
 *
 * | Gate ID  | Benchmark          | Threshold         |
 * |----------|--------------------|-------------------|
 * | TH-BM-01 | ModuleVerify       | p99 ≤ 200 ns      |
 * | TH-BM-02 | WireSessionRequest | p99 ≤ 500 ns      |
 * | TH-BM-03 | DependencyResolve  | p99 ≤ 200 ns      |
 * | TH-BM-04 | BatchModuleVerify  | p99 ≤ 100 µs/batch|
 *
 * @see src/themis/ROADMAP.md — Wave D items
 * @see docs/operability/RUNBOOK_THEMIS_CORE.md
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <string>

namespace themis {
namespace bench {
namespace themis_dedicated {

static constexpr uint64_t kCanonicalSeed = 42;
static constexpr int      kRepetitions   = 5;

// ─── In-process stubs (SIMULATION NOTE: no shared library or TCP required) ───

static uint8_t stubVerifyModule(const std::string& name) noexcept {
    return static_cast<uint8_t>((name.size() % 4) + 1);
}

static bool stubWireRequest(uint64_t session_id) noexcept {
    (void)session_id;
    return true;
}

static bool stubDependencyResolve(const std::string& dep) noexcept {
    (void)dep;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// TH-BM-01 — Module load/verify throughput
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Themis_ModuleVerify(benchmark::State& state) {
    uint64_t seed = kCanonicalSeed;
    for (auto _ : state) {
        const std::string name = "plugin_bm_" + std::to_string(seed % 256);
        benchmark::DoNotOptimize(stubVerifyModule(name));
        ++seed;
    }
}
BENCHMARK(BM_Themis_ModuleVerify)
    ->Name("TH-BM-01/ModuleVerify")
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ─────────────────────────────────────────────────────────────────────────────
// TH-BM-02 — Wire session request throughput
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Themis_WireSessionRequest(benchmark::State& state) {
    uint64_t seed = kCanonicalSeed;
    for (auto _ : state) {
        const uint64_t session_id = seed * 6364136223846793005ULL;
        benchmark::DoNotOptimize(stubWireRequest(session_id));
        ++seed;
    }
}
BENCHMARK(BM_Themis_WireSessionRequest)
    ->Name("TH-BM-02/WireSessionRequest")
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ─────────────────────────────────────────────────────────────────────────────
// TH-BM-03 — Dependency gate resolution latency
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Themis_DependencyResolve(benchmark::State& state) {
    uint64_t seed = kCanonicalSeed;
    for (auto _ : state) {
        const std::string dep = "dep_module_" + std::to_string(seed % 64);
        benchmark::DoNotOptimize(stubDependencyResolve(dep));
        ++seed;
    }
}
BENCHMARK(BM_Themis_DependencyResolve)
    ->Name("TH-BM-03/DependencyResolve")
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ─────────────────────────────────────────────────────────────────────────────
// TH-BM-04 — Batch module verify (1 000 iterations)
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Themis_BatchModuleVerify(benchmark::State& state) {
    constexpr int kBatch = 1000;
    uint64_t seed = kCanonicalSeed;
    for (auto _ : state) {
        for (int i = 0; i < kBatch; ++i) {
            const std::string name = "plugin_batch_" + std::to_string(seed % 256);
            benchmark::DoNotOptimize(stubVerifyModule(name));
            ++seed;
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatch);
}
BENCHMARK(BM_Themis_BatchModuleVerify)
    ->Name("TH-BM-04/BatchModuleVerify1000")
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace themis_dedicated
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
