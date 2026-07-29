// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_maintenance_release_gates.cpp
 * @brief Phase 5 maintenance module release-gate benchmarks.
 *
 * Provides reproducible latency measurements for the maintenance module hot
 * paths identified in the maintenance module roadmap (Phase 5 — Performance
 * and Hardening).
 *
 * ## Benchmark families
 *
 * ### GATE-MTN-01 — Error enum cast throughput
 *   Measures the cost of casting MaintenanceError values from int32_t; serves
 *   as a baseline for error-path hot loops.
 *
 * ### GATE-MTN-02 — Switch dispatch throughput
 *   Measures switch-based dispatch across all MaintenanceError codes; validates
 *   that the compiler optimises the switch into an O(1) jump table.
 *
 * ### GATE-MTN-03 — ScheduleDescriptor struct allocation
 *   Measures in-process heap allocation and initialisation cost for
 *   MaintenanceScheduleDescriptor; release gate for schedule-churn paths.
 *
 * ### GATE-MTN-04 — Batch error cast (1 000 iterations)
 *   Measures amortised error-cast cost across a batch of 1 000 mixed codes;
 *   simulates a high-churn schedule-validation hot loop.
 *
 * ## Hard release gates
 *
 * | Gate ID      | Benchmark       | Threshold        |
 * |--------------|-----------------|------------------|
 * | GATE-MTN-01  | ErrorEnumCast   | p99 ≤ 5 ns       |
 * | GATE-MTN-02  | SwitchDispatch  | p99 ≤ 10 ns      |
 * | GATE-MTN-03  | StructAlloc     | p99 ≤ 500 ns     |
 * | GATE-MTN-04  | BatchCast       | p99 ≤ 5 µs/batch |
 *
 * All benchmarks use kCanonicalSeed = 42 for deterministic inputs.
 *
 * @see src/maintenance/ROADMAP.md — Phase 5 items
 * @see include/maintenance/maintenance_api_contract.h
 */

#include <benchmark/benchmark.h>
#include "maintenance/maintenance_api_contract.h"

#include <cstdint>
#include <string>
#include <vector>

namespace themis {
namespace bench {
namespace mtn {

/// Canonical PRNG seed for all MTN benchmarks.
static constexpr uint64_t kCanonicalSeed = 42;

/// Number of repetitions for variance estimation.
static constexpr int kRepetitions = 5;

// ============================================================================
// GATE-MTN-01 — Error enum cast throughput
// ============================================================================

static void BM_MTN01_ErrorEnumCast(benchmark::State& state) {
    const int32_t raw = static_cast<int32_t>(
        maintenance::MaintenanceError::kOrchestratorDegraded);
    for (auto _ : state) {
        auto e = static_cast<maintenance::MaintenanceError>(raw);
        benchmark::DoNotOptimize(e);
    }
    state.SetLabel("GATE-MTN-01: p99 <= 5 ns");
}
BENCHMARK(BM_MTN01_ErrorEnumCast)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-MTN-02 — Switch dispatch throughput
// ============================================================================

static void BM_MTN02_SwitchDispatch(benchmark::State& state) {
    const maintenance::MaintenanceError codes[] = {
        maintenance::MaintenanceError::kSuccess,
        maintenance::MaintenanceError::kScheduleNotFound,
        maintenance::MaintenanceError::kHandlerNotRegistered,
        maintenance::MaintenanceError::kPersistenceFailed,
        maintenance::MaintenanceError::kExecutionTimeout,
        maintenance::MaintenanceError::kConcurrentModification,
        maintenance::MaintenanceError::kInvalidSchedule,
        maintenance::MaintenanceError::kOrchestratorDegraded,
        maintenance::MaintenanceError::kInternalError,
    };
    uint64_t idx = kCanonicalSeed % 9;
    for (auto _ : state) {
        const char* label = nullptr;
        switch (codes[idx % 9]) {
            case maintenance::MaintenanceError::kSuccess:               label = "ok"; break;
            case maintenance::MaintenanceError::kScheduleNotFound:      label = "sched"; break;
            case maintenance::MaintenanceError::kHandlerNotRegistered:  label = "hdlr"; break;
            case maintenance::MaintenanceError::kPersistenceFailed:     label = "pers"; break;
            case maintenance::MaintenanceError::kExecutionTimeout:      label = "tout"; break;
            case maintenance::MaintenanceError::kConcurrentModification: label = "conc"; break;
            case maintenance::MaintenanceError::kInvalidSchedule:       label = "inv"; break;
            case maintenance::MaintenanceError::kOrchestratorDegraded:  label = "deg"; break;
            case maintenance::MaintenanceError::kInternalError:         label = "int"; break;
        }
        benchmark::DoNotOptimize(label);
        ++idx;
    }
    state.SetLabel("GATE-MTN-02: p99 <= 10 ns");
}
BENCHMARK(BM_MTN02_SwitchDispatch)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-MTN-03 — ScheduleDescriptor struct allocation
// ============================================================================

static void BM_MTN03_StructAlloc(benchmark::State& state) {
    for (auto _ : state) {
        maintenance::MaintenanceScheduleDescriptor desc;
        desc.schedule_id = "bench-sched-42";
        desc.task_type   = "compaction";
        desc.interval    = std::chrono::seconds{3600};
        desc.enabled     = true;
        benchmark::DoNotOptimize(desc);
    }
    state.SetLabel("GATE-MTN-03: p99 <= 500 ns");
}
BENCHMARK(BM_MTN03_StructAlloc)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-MTN-04 — Batch error cast (1 000 iterations)
// ============================================================================

static void BM_MTN04_BatchCast(benchmark::State& state) {
    static const int32_t kRawCodes[] = {
        8100, 8101, 8102, 8103, 8104, 8105, 8106, 8107
    };
    static constexpr int kBatchSize = 1000;
    for (auto _ : state) {
        uint64_t seed = kCanonicalSeed;
        for (int i = 0; i < kBatchSize; ++i) {
            seed ^= seed << 13;
            seed ^= seed >> 7;
            seed ^= seed << 17;
            auto e = static_cast<maintenance::MaintenanceError>(
                kRawCodes[seed % 8]);
            benchmark::DoNotOptimize(e);
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatchSize);
    state.SetLabel("GATE-MTN-04: p99 <= 5 us per batch");
}
BENCHMARK(BM_MTN04_BatchCast)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace mtn
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
