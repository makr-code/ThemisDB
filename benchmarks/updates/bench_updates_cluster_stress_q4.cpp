/*
 * ThemisDB | File: bench_updates_cluster_stress_q4.cpp | Version: 1.0.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Phase 5: Cluster Stress Benchmarks (Q4 2026)
 *
 * Gate table:
 * | ID              | Metric                               | Gate          |
 * |-----------------|--------------------------------------|---------------|
 * | GATE-CSS-01     | EdgeCaseHandler detect() throughput  | ≥ 500k ops/s  |
 * | GATE-CSS-02     | StateMachine IDLE→transition rate    | ≥ 1M ops/s    |
 * | GATE-CSS-03     | Concurrent detection (4 threads)     | ≥ 100k ops/s  |
 * | GATE-CSS-04     | Rollback stats tracking              | ≥ 1M ops/s    |
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <benchmark/benchmark.h>
#include "updates/update_state_machine.h"
#include "updates/updates_edge_case_handler.h"

#include <memory>
#include <string>

using namespace themis::updates;

static constexpr uint64_t kCanonicalSeed = 42;

// ============================================================================
// BM_EdgeCaseHandler_Detect_IDLE
//
// GATE-CSS-01: detect() on an IDLE state machine ≥ 500k ops/s
//
// Measures the overhead of a single detectAndHandle() call when the machine
// is in the IDLE state (no edge case detected path — minimum-overhead branch).
// ============================================================================
static void BM_EdgeCaseHandler_Detect_IDLE(benchmark::State& state) {
    UpdatesEdgeCaseHandler handler;
    UpdateStateMachine sm("");
    // IDLE state — no edge case will be detected.
    for (auto _ : state) {
        auto result = handler.detectAndHandle(sm, "");
        benchmark::DoNotOptimize(result.handled);
    }
}
BENCHMARK(BM_EdgeCaseHandler_Detect_IDLE)
    ->Repetitions(5)
    ->ReportAggregatesOnly(true);

// ============================================================================
// BM_StateMachine_Transition_Rate
//
// GATE-CSS-02: IDLE→DOWNLOADING transition rate ≥ 1M ops/s
//
// Each iteration creates a fresh state machine in IDLE and performs one
// transition.  Measures raw transition dispatch cost.
// ============================================================================
static void BM_StateMachine_Transition_Rate(benchmark::State& state) {
    for (auto _ : state) {
        UpdateStateMachine sm("");
        bool ok = sm.transition(UpdateState::DOWNLOADING, "v1", "bench");
        benchmark::DoNotOptimize(ok);
    }
}
BENCHMARK(BM_StateMachine_Transition_Rate)
    ->Repetitions(5)
    ->ReportAggregatesOnly(true);

// ============================================================================
// BM_EdgeCaseHandler_Concurrent_4Threads
//
// GATE-CSS-03: Concurrent detection across 4 threads ≥ 100k ops/s
//
// Uses benchmark::State::threads() to run 4 parallel threads.  Each thread
// operates on a thread-local state machine in DOWNLOADING state and calls
// detectAndHandle with "concurrent" hint (STATE_ALREADY_IN_PROGRESS path).
// The shared handler's stats mutex is exercised under contention.
// ============================================================================
static void BM_EdgeCaseHandler_Concurrent_4Threads(benchmark::State& state) {
    // One shared handler, each thread has its own state machine.
    static UpdatesEdgeCaseHandler shared_handler;

    // Thread-local state machine initialised once per thread.
    static thread_local UpdateStateMachine tl_sm("");
    static thread_local bool tl_init = false;
    if (!tl_init) {
        tl_sm.transition(UpdateState::DOWNLOADING, "v1", "init");
        tl_init = true;
    }

    for (auto _ : state) {
        auto result = shared_handler.detectAndHandle(tl_sm, "concurrent");
        benchmark::DoNotOptimize(result.handled);
    }
}
BENCHMARK(BM_EdgeCaseHandler_Concurrent_4Threads)
    ->Threads(4)
    ->Repetitions(5)
    ->ReportAggregatesOnly(true);

// ============================================================================
// BM_RollbackStats_Tracking
//
// GATE-CSS-04: Rollback stats increment throughput ≥ 1M ops/s
//
// Each iteration drives detectAndHandle on a ROLLING_BACK machine with
// "cascade" hint, producing ROLLBACK_CASCADE_DETECTED which requires_rollback
// is false but IS fatal — so total_fatal and total_detected both increment.
// Measures the stats mutex acquire/release cost under single-threaded load.
// ============================================================================
static void BM_RollbackStats_Tracking(benchmark::State& state) {
    UpdatesEdgeCaseHandler handler;
    UpdateStateMachine sm("");
    sm.transition(UpdateState::DOWNLOADING, "v1", "start");
    sm.transition(UpdateState::ROLLING_BACK, "", "rb");

    for (auto _ : state) {
        auto result = handler.detectAndHandle(sm, "cascade");
        benchmark::DoNotOptimize(result.error_code);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RollbackStats_Tracking)
    ->Repetitions(5)
    ->ReportAggregatesOnly(true);
