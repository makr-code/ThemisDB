// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_scheduler_release_gates.cpp
 * @brief Phase 5 scheduler module release-gate benchmarks.
 *
 * Provides reproducible latency measurements for the scheduler module hot
 * paths identified in the scheduler module roadmap (Phase 5 — Performance
 * and Hardening).
 *
 * ## Benchmark families
 *
 * ### GATE-SCH-01 — Error enum cast throughput
 *   Measures the cost of casting SchedulerError values from int32_t.
 *
 * ### GATE-SCH-02 — Switch dispatch throughput
 *   Measures switch-based dispatch across all SchedulerError codes.
 *
 * ### GATE-SCH-03 — TaskRegistrationDescriptor struct allocation
 *   Measures in-process heap allocation for TaskRegistrationDescriptor;
 *   release gate for task-registration and burst-submission paths.
 *
 * ### GATE-SCH-04 — Batch error cast (1 000 iterations)
 *   Amortised error-cast cost across 1 000 mixed codes; simulates a
 *   high-throughput task-result classification hot loop.
 *
 * ## Hard release gates
 *
 * | Gate ID      | Benchmark       | Threshold        |
 * |--------------|-----------------|------------------|
 * | GATE-SCH-01  | ErrorEnumCast   | p99 ≤ 5 ns       |
 * | GATE-SCH-02  | SwitchDispatch  | p99 ≤ 10 ns      |
 * | GATE-SCH-03  | StructAlloc     | p99 ≤ 500 ns     |
 * | GATE-SCH-04  | BatchCast       | p99 ≤ 5 µs/batch |
 *
 * All benchmarks use kCanonicalSeed = 42 for deterministic inputs.
 *
 * @see src/scheduler/ROADMAP.md — Phase 5 items
 * @see include/scheduler/scheduler_api_contract.h
 */

#include <benchmark/benchmark.h>
#include "scheduler/scheduler_api_contract.h"

#include <cstdint>
#include <string>

namespace themis {
namespace bench {
namespace sch {

/// Canonical PRNG seed for all SCH benchmarks.
static constexpr uint64_t kCanonicalSeed = 42;

/// Number of repetitions for variance estimation.
static constexpr int kRepetitions = 5;

// ============================================================================
// GATE-SCH-01 — Error enum cast throughput
// ============================================================================

static void BM_SCH01_ErrorEnumCast(benchmark::State& state) {
    const int32_t raw = static_cast<int32_t>(
        scheduler::SchedulerError::kCoordinationError);
    for (auto _ : state) {
        auto e = static_cast<scheduler::SchedulerError>(raw);
        benchmark::DoNotOptimize(e);
    }
    state.SetLabel("GATE-SCH-01: p99 <= 5 ns");
}
BENCHMARK(BM_SCH01_ErrorEnumCast)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-SCH-02 — Switch dispatch throughput
// ============================================================================

static void BM_SCH02_SwitchDispatch(benchmark::State& state) {
    const scheduler::SchedulerError codes[] = {
        scheduler::SchedulerError::kSuccess,
        scheduler::SchedulerError::kTaskNotFound,
        scheduler::SchedulerError::kTaskAlreadyExists,
        scheduler::SchedulerError::kExecutionFailed,
        scheduler::SchedulerError::kCoordinationError,
        scheduler::SchedulerError::kRetentionLimitExceeded,
        scheduler::SchedulerError::kTriggerInvalid,
        scheduler::SchedulerError::kAnomalyDetected,
        scheduler::SchedulerError::kInternalError,
    };
    uint64_t idx = kCanonicalSeed % 9;
    for (auto _ : state) {
        const char* label = nullptr;
        switch (codes[idx % 9]) {
            case scheduler::SchedulerError::kSuccess:                label = "ok"; break;
            case scheduler::SchedulerError::kTaskNotFound:           label = "notfound"; break;
            case scheduler::SchedulerError::kTaskAlreadyExists:      label = "exists"; break;
            case scheduler::SchedulerError::kExecutionFailed:        label = "exec"; break;
            case scheduler::SchedulerError::kCoordinationError:      label = "coord"; break;
            case scheduler::SchedulerError::kRetentionLimitExceeded: label = "ret"; break;
            case scheduler::SchedulerError::kTriggerInvalid:         label = "trig"; break;
            case scheduler::SchedulerError::kAnomalyDetected:        label = "anom"; break;
            case scheduler::SchedulerError::kInternalError:          label = "int"; break;
        }
        benchmark::DoNotOptimize(label);
        ++idx;
    }
    state.SetLabel("GATE-SCH-02: p99 <= 10 ns");
}
BENCHMARK(BM_SCH02_SwitchDispatch)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-SCH-03 — TaskRegistrationDescriptor struct allocation
// ============================================================================

static void BM_SCH03_StructAlloc(benchmark::State& state) {
    for (auto _ : state) {
        scheduler::TaskRegistrationDescriptor desc;
        desc.task_id   = "bench-task-42";
        desc.task_name = "BenchmarkCompaction";
        desc.allow_concurrent = false;
        benchmark::DoNotOptimize(desc);
    }
    state.SetLabel("GATE-SCH-03: p99 <= 500 ns");
}
BENCHMARK(BM_SCH03_StructAlloc)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-SCH-04 — Batch error cast (1 000 iterations)
// ============================================================================

static void BM_SCH04_BatchCast(benchmark::State& state) {
    static const int32_t kRawCodes[] = {
        8400, 8401, 8402, 8403, 8404, 8405, 8406, 8407
    };
    static constexpr int kBatchSize = 1000;
    for (auto _ : state) {
        uint64_t seed = kCanonicalSeed;
        for (int i = 0; i < kBatchSize; ++i) {
            seed ^= seed << 13;
            seed ^= seed >> 7;
            seed ^= seed << 17;
            auto e = static_cast<scheduler::SchedulerError>(kRawCodes[seed % 8]);
            benchmark::DoNotOptimize(e);
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatchSize);
    state.SetLabel("GATE-SCH-04: p99 <= 5 us per batch");
}
BENCHMARK(BM_SCH04_BatchCast)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace sch
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
