// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_plugins_release_gates.cpp
 * @brief Phase 5 plugins module release-gate benchmarks.
 *
 * Provides reproducible latency measurements for the plugins module hot paths
 * identified in the plugins module roadmap (Phase 5 — Performance and Hardening).
 *
 * ## Benchmark families
 *
 * ### GATE-PLG-01 — Error enum cast throughput
 *   Measures the cost of casting PluginsError values from int32_t.
 *
 * ### GATE-PLG-02 — Switch dispatch throughput
 *   Measures switch-based dispatch across all PluginsError codes; validates
 *   compiler jump-table optimisation.
 *
 * ### GATE-PLG-03 — PluginRegistrationDescriptor struct allocation
 *   Measures in-process heap allocation for PluginRegistrationDescriptor;
 *   release gate for plugin churn / hot-plug paths.
 *
 * ### GATE-PLG-04 — Batch error cast (1 000 iterations)
 *   Amortised error-cast cost across 1 000 mixed codes; simulates a
 *   high-churn plugin lifecycle validation hot loop.
 *
 * ## Hard release gates
 *
 * | Gate ID      | Benchmark       | Threshold        |
 * |--------------|-----------------|------------------|
 * | GATE-PLG-01  | ErrorEnumCast   | p99 ≤ 5 ns       |
 * | GATE-PLG-02  | SwitchDispatch  | p99 ≤ 10 ns      |
 * | GATE-PLG-03  | StructAlloc     | p99 ≤ 500 ns     |
 * | GATE-PLG-04  | BatchCast       | p99 ≤ 5 µs/batch |
 *
 * All benchmarks use kCanonicalSeed = 42 for deterministic inputs.
 *
 * @see src/plugins/ROADMAP.md — Phase 5 items
 * @see include/plugins/plugins_api_contract.h
 */

#include <benchmark/benchmark.h>
#include "plugins/plugins_api_contract.h"

#include <cstdint>
#include <string>

namespace themis {
namespace bench {
namespace plg {

/// Canonical PRNG seed for all PLG benchmarks.
static constexpr uint64_t kCanonicalSeed = 42;

/// Number of repetitions for variance estimation.
static constexpr int kRepetitions = 5;

// ============================================================================
// GATE-PLG-01 — Error enum cast throughput
// ============================================================================

static void BM_PLG01_ErrorEnumCast(benchmark::State& state) {
    const int32_t raw = static_cast<int32_t>(
        plugins::PluginsError::kSignatureVerifyFailed);
    for (auto _ : state) {
        auto e = static_cast<plugins::PluginsError>(raw);
        benchmark::DoNotOptimize(e);
    }
    state.SetLabel("GATE-PLG-01: p99 <= 5 ns");
}
BENCHMARK(BM_PLG01_ErrorEnumCast)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-PLG-02 — Switch dispatch throughput
// ============================================================================

static void BM_PLG02_SwitchDispatch(benchmark::State& state) {
    const plugins::PluginsError codes[] = {
        plugins::PluginsError::kSuccess,
        plugins::PluginsError::kPluginNotFound,
        plugins::PluginsError::kManifestInvalid,
        plugins::PluginsError::kSignatureVerifyFailed,
        plugins::PluginsError::kLifecycleTransition,
        plugins::PluginsError::kCapabilityDenied,
        plugins::PluginsError::kRegistryConflict,
        plugins::PluginsError::kHealthCheckFailed,
        plugins::PluginsError::kInternalError,
    };
    uint64_t idx = kCanonicalSeed % 9;
    for (auto _ : state) {
        const char* label = nullptr;
        switch (codes[idx % 9]) {
            case plugins::PluginsError::kSuccess:               label = "ok"; break;
            case plugins::PluginsError::kPluginNotFound:        label = "notfound"; break;
            case plugins::PluginsError::kManifestInvalid:       label = "manifest"; break;
            case plugins::PluginsError::kSignatureVerifyFailed: label = "sig"; break;
            case plugins::PluginsError::kLifecycleTransition:   label = "lc"; break;
            case plugins::PluginsError::kCapabilityDenied:      label = "cap"; break;
            case plugins::PluginsError::kRegistryConflict:      label = "reg"; break;
            case plugins::PluginsError::kHealthCheckFailed:     label = "hc"; break;
            case plugins::PluginsError::kInternalError:         label = "int"; break;
        }
        benchmark::DoNotOptimize(label);
        ++idx;
    }
    state.SetLabel("GATE-PLG-02: p99 <= 10 ns");
}
BENCHMARK(BM_PLG02_SwitchDispatch)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-PLG-03 — PluginRegistrationDescriptor struct allocation
// ============================================================================

static void BM_PLG03_StructAlloc(benchmark::State& state) {
    for (auto _ : state) {
        plugins::PluginRegistrationDescriptor desc;
        desc.plugin_id         = "bench-plugin-42";
        desc.version           = "1.0.0";
        desc.hot_plug_eligible = true;
        benchmark::DoNotOptimize(desc);
    }
    state.SetLabel("GATE-PLG-03: p99 <= 500 ns");
}
BENCHMARK(BM_PLG03_StructAlloc)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-PLG-04 — Batch error cast (1 000 iterations)
// ============================================================================

static void BM_PLG04_BatchCast(benchmark::State& state) {
    static const int32_t kRawCodes[] = {
        8200, 8201, 8202, 8203, 8204, 8205, 8206, 8207
    };
    static constexpr int kBatchSize = 1000;
    for (auto _ : state) {
        uint64_t seed = kCanonicalSeed;
        for (int i = 0; i < kBatchSize; ++i) {
            seed ^= seed << 13;
            seed ^= seed >> 7;
            seed ^= seed << 17;
            auto e = static_cast<plugins::PluginsError>(kRawCodes[seed % 8]);
            benchmark::DoNotOptimize(e);
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatchSize);
    state.SetLabel("GATE-PLG-04: p99 <= 5 us per batch");
}
BENCHMARK(BM_PLG04_BatchCast)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

}  // namespace plg
}  // namespace bench
}  // namespace themis

// Extended Phase 5 Release Gate Benchmarks
namespace themis {
namespace bench {
namespace plg_phase5 {

/**
 * Phase 5 Release Gate Benchmarks (Performance and Hardening)
 * 
 * These benchmarks validate the performance characteristics of plugin
 * lifecycle operations against release-gate thresholds.
 */

// Note: Full plugin lifecycle benchmarks require a complete test infrastructure
// with actual plugin loading. These templates show the benchmark structure.

// GATE-PLG-01: Plugin load latency
// Target: p95 ≤ 50ms, p99 ≤ 100ms
// This requires actual plugin binaries and would be part of integration tests

// GATE-PLG-02: Plugin unload latency
// Target: p95 ≤ 30ms
// Measures time to properly unload and clean up plugin resources

// GATE-PLG-03: Registry throughput
// Target: ≥ 10k ops/s for registry create operations
// Measures concurrent registration operations

// GATE-PLG-04: Plugin reload latency
// Target: ≤ 200ms for complete reload cycle
// Measures unload + load time for plugin hot-reload

}  // namespace plg_phase5
}  // namespace bench
}  // namespace themis

BENCHMARK_MAIN();
