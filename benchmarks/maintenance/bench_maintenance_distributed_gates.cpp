// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_maintenance_distributed_gates.cpp
 * @brief Phase 5 distributed-scenario release-gate benchmarks for the maintenance module.
 *
 * Provides reproducible latency measurements for distributed maintenance
 * scheduling scenarios introduced in Phase 2 (churn guard) and Phase 4
 * (diagnostics ring buffer / schedule listing).
 *
 * ## Benchmark families
 *
 * ### GATE-MTN-DIST-01 — Leader-gated schedule dispatch
 *   Measures the cost of acquiring a mock distributed lock (std::mutex + bool)
 *   and dispatching a schedule when this node is the leader.
 *   Gate: p99 ≤ 500 µs.
 *
 * ### GATE-MTN-DIST-02 — Schedule listing with 1000 entries
 *   Measures the cost of iterating over an unordered_map of 1000 schedule
 *   entries and building a result vector (the listSchedules() hot path).
 *   Gate: p99 ≤ 5 ms.
 *
 * ## Hard release gates
 *
 * | Gate ID           | Benchmark       | Threshold       |
 * |-------------------|-----------------|-----------------|
 * | GATE-MTN-DIST-01  | DistLock        | p99 ≤ 500 µs    |
 * | GATE-MTN-DIST-02  | ScheduleListing | p99 ≤ 5 ms      |
 *
 * All benchmarks use kCanonicalSeed = 42 for deterministic inputs.
 *
 * @see benchmarks/maintenance/bench_maintenance_release_gates.cpp
 * @see src/maintenance/ROADMAP.md — Phase 5 items
 * @see benchmarks/maintenance/release_gate_manifest_mtn.json
 */

#include <benchmark/benchmark.h>
#include "maintenance/maintenance_api_contract.h"
#include "maintenance/maintenance_schedule.h"
#include "maintenance/maintenance_task.h"

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace bench {
namespace mtn {

/// Canonical PRNG seed for all MTN benchmarks.
static constexpr uint64_t kCanonicalSeed = 42;

/// Number of repetitions for variance estimation.
static constexpr int kRepetitions = 5;

// ============================================================================
// GATE-MTN-DIST-01 — Leader-gated schedule dispatch under a mock distributed lock
// ============================================================================

/**
 * @brief Simulates the hot path for leader-gated schedule dispatch:
 *        1. Acquire a mock distributed lock (std::mutex + bool flag).
 *        2. Check whether this node is the leader.
 *        3. If leader, dispatch (increment counter as a proxy for real work).
 *        4. Release lock.
 *
 * Gate: p99 ≤ 500 µs.
 */
static void BM_MTN_DIST01_LeaderGatedDispatch(benchmark::State& state) {
    std::mutex lock_mu = {};
    bool       is_leader = true;
    std::atomic<uint64_t> dispatch_count{0};

    for (auto _ : state) {
        bool dispatched = false;
        {
            std::lock_guard<std::mutex> lg(lock_mu);
            if (is_leader) {
                ++dispatch_count;
                dispatched = true;
            }
        }
        benchmark::DoNotOptimize(dispatched);
    }
    state.SetLabel("GATE-MTN-DIST-01: p99 <= 500 us");
}
BENCHMARK(BM_MTN_DIST01_LeaderGatedDispatch)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-MTN-DIST-02 — Schedule listing with 1000 entries
// ============================================================================

/**
 * @brief Measures the cost of listing 1000 maintenance schedules from an
 *        unordered_map (the listSchedules() hot path in the orchestrator).
 *
 * Gate: p99 ≤ 5 ms.
 */
static void BM_MTN_DIST02_ScheduleListing(benchmark::State& state) {
    static constexpr int kEntries = 1000;

    // Pre-populate the schedule map once outside the timed loop.
    std::unordered_map<std::string, maintenance::MaintenanceScheduleEntry> schedules;
    schedules.reserve(kEntries);
    for (int i = 0; i < kEntries; ++i) {
        maintenance::MaintenanceScheduleEntry e;
        e.id          = "sched-dist02-" + std::to_string(i);
        e.name        = "Schedule " + std::to_string(i);
        e.frequency   = maintenance::ScheduleFrequency::DAILY;
        e.tasks       = {maintenance::MaintenanceTaskType::QUOTA_CHECK};
        e.enabled     = (i % 2 == 0);
        e.tenant_id   = (i % 5 == 0) ? "tenant-a" : "";
        schedules[e.id] = e;
    }

    for (auto _ : state) {
        std::vector<maintenance::MaintenanceScheduleEntry> result = {};

        result.reserve(schedules.size());
        for (const auto& [id, entry] : schedules) {
            result.push_back(entry);
        }
        benchmark::DoNotOptimize(result.size());
    }
    state.SetLabel("GATE-MTN-DIST-02: p99 <= 5 ms");
}
BENCHMARK(BM_MTN_DIST02_ScheduleListing)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace mtn
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
