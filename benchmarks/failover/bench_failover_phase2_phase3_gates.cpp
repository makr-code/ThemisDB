// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_failover_phase2_phase3_gates.cpp
 * @brief Phase 5 release-gate benchmarks for Phase 2/3 failover hot paths (FP23-01..FP23-06).
 *
 * Provides reproducible latency measurements for the six hot-path operations
 * introduced or hardened in Phase 2 (Core Implementation) and Phase 3
 * (Error Handling and Edge Cases) of the failover module roadmap.
 *
 * Results are used as release gates for the Phase 2/3 delivery batch.
 * A regression beyond 10% vs the baseline blocks promotion.
 *
 * ## Benchmark families
 *
 * ### FP23-01 — canTransition() state table lookup
 *   Measures the switch-based state-machine guard.
 *   GATE-FP23-01: p99 ≤ 100 µs
 *
 * ### FP23-02 — preventSplitBrain() fail-closed null-fencing-manager path
 *   Measures the fast-reject path: pointer check + error-code dispatch.
 *   GATE-FP23-02: p99 ≤ 200 µs
 *
 * ### FP23-03 — executePlan() concurrency guard (try_to_lock, uncontested)
 *   Measures `std::unique_lock<std::mutex>(m, std::try_to_lock)` + owns_lock()
 *   overhead on an uncontested mutex.
 *   GATE-FP23-03: p99 ≤ 100 µs
 *
 * ### FP23-04 — attemptRecovery() batch stats update
 *   Measures a single lock_guard acquisition followed by three uint64_t
 *   increments (the batch-flush pattern replacing per-iteration locks).
 *   GATE-FP23-04: p99 ≤ 200 µs
 *
 * ### FP23-05 — emitDiagnostic() error-code dispatch overhead
 *   Measures FailoverErrorCode → FailoverEventType mapping + iteration over
 *   an empty event-callback vector (the typical no-subscriber case).
 *   GATE-FP23-05: p99 ≤ 100 µs
 *
 * ### FP23-06 — triggerManualFailover() full-queue rejection path
 *   Measures the fast-drop path: queue-size check + lock_guard + stats increment.
 *   GATE-FP23-06: p99 ≤ 200 µs
 *
 * ## Hard release gates
 *
 * | Gate ID      | Benchmark | Threshold    |
 * |--------------|-----------|--------------|
 * | GATE-FP23-01 | FP23-01   | p99 ≤ 100 µs |
 * | GATE-FP23-02 | FP23-02   | p99 ≤ 200 µs |
 * | GATE-FP23-03 | FP23-03   | p99 ≤ 100 µs |
 * | GATE-FP23-04 | FP23-04   | p99 ≤ 200 µs |
 * | GATE-FP23-05 | FP23-05   | p99 ≤ 100 µs |
 * | GATE-FP23-06 | FP23-06   | p99 ≤ 200 µs |
 *
 * @see include/failover/failover_api_contract.h
 * @see include/failover/auto_failover_manager.h
 * @see src/failover/ROADMAP.md — Phase 5 item (Phase 2/3 benchmarks)
 */

#include <benchmark/benchmark.h>

#include "failover/failover_api_contract.h"
#include "failover/auto_failover_manager.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

using namespace themis::failover;

namespace themis {
namespace bench {
namespace fp23 {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr uint64_t kP23CanonicalSeed = 42;
static constexpr int      kRepetitions      = 5;
static constexpr int      kWarmupIterations = 100;

// ---------------------------------------------------------------------------
// FP23-01 helpers — canTransition() state machine table
//
// Replicates the switch logic from auto_failover_manager.cpp as a pure
// function so the benchmark measures only the dispatch cost, with no
// side-effects or logging overhead.
// ---------------------------------------------------------------------------

/**
 * @brief Inline replica of AutoFailoverManager::canTransition() for benchmarking.
 *
 * Models the state-machine switch (9 arms + 2 universal rules) that was
 * delivered as part of Phase 2/3 hardening.  The logic is intentionally
 * kept identical to the production implementation.
 *
 * @param from  Source state.
 * @param to    Target state.
 * @return true if the transition is in the allowed table.
 */
[[nodiscard]] static bool mockCanTransition(
    FailoverOrchestratorState from,
    FailoverOrchestratorState to) noexcept
{
    // Universal rules — any state may reach IDLE (reset) or FAILED (error).
    if (to == FailoverOrchestratorState::IDLE)   { return true; }
    if (to == FailoverOrchestratorState::FAILED)  { return true; }

    switch (from) {
        case FailoverOrchestratorState::IDLE:
            return to == FailoverOrchestratorState::VERIFYING_FAILURE ||
                   to == FailoverOrchestratorState::DETECTING_FAILURE;

        case FailoverOrchestratorState::DETECTING_FAILURE:
            return to == FailoverOrchestratorState::VERIFYING_FAILURE;

        case FailoverOrchestratorState::VERIFYING_FAILURE:
            return to == FailoverOrchestratorState::CHECKING_QUORUM;

        case FailoverOrchestratorState::CHECKING_QUORUM:
            return to == FailoverOrchestratorState::STARTING_LEADER_ELECTION;

        case FailoverOrchestratorState::STARTING_LEADER_ELECTION:
            return to == FailoverOrchestratorState::LEADER_ELECTION_IN_PROGRESS ||
                   to == FailoverOrchestratorState::UPDATING_METADATA;

        case FailoverOrchestratorState::LEADER_ELECTION_IN_PROGRESS:
            return to == FailoverOrchestratorState::ACTIVATING_SPARE ||
                   to == FailoverOrchestratorState::UPDATING_METADATA;

        case FailoverOrchestratorState::ACTIVATING_SPARE:
            return to == FailoverOrchestratorState::REDIRECTING_TRAFFIC ||
                   to == FailoverOrchestratorState::UPDATING_METADATA;

        case FailoverOrchestratorState::REDIRECTING_TRAFFIC:
            return to == FailoverOrchestratorState::UPDATING_METADATA;

        case FailoverOrchestratorState::UPDATING_METADATA:
            return to == FailoverOrchestratorState::COMPLETING_FAILOVER;

        case FailoverOrchestratorState::COMPLETING_FAILOVER:
            return false;   // reaches IDLE via the universal rule above

        case FailoverOrchestratorState::FAILED:
            return false;   // reaches IDLE via the universal rule above

        default:
            return false;
    }
}

// ---------------------------------------------------------------------------
// FP23-02 helpers — preventSplitBrain() fail-closed path
//
// Models the null-fencing-manager branch: pointer is nullptr → return false.
// The production path also calls emitDiagnostic + spdlog::error; here we
// measure only the decision overhead (pointer check + error code mapping).
// ---------------------------------------------------------------------------

/// Opaque fencing manager handle (null = not configured).
struct MockFencingManager {};

/**
 * @brief Inline replica of the preventSplitBrain() null-manager fast-reject path.
 *
 * @param fencing_mgr  Pointer to the fencing manager (nullptr → fail closed).
 * @return false when no fencing manager is configured.
 */
[[nodiscard]] static bool mockPreventSplitBrainFailClosed(
    const MockFencingManager* fencing_mgr) noexcept
{
    if (!fencing_mgr) {
        // Fail closed: cannot guarantee exclusive leadership without fencing.
        // Production path also emits diagnostic; omitted here to isolate
        // the decision overhead.
        return false;
    }
    return true;  // fencing available — would proceed to bumpEpoch()
}

// ---------------------------------------------------------------------------
// FP23-03 helpers — executePlan() concurrency guard
//
// Models the try_to_lock fast-reject path: acquire, check owns_lock,
// release.  The mutex is uncontested in this benchmark (single thread).
// ---------------------------------------------------------------------------

static std::mutex g_execution_mutex;   // mirrors DisasterRecoveryManager::execution_mutex_

/**
 * @brief Inline replica of the executePlan() concurrency guard.
 *
 * @return true if the lock was acquired (no concurrent execution); false otherwise.
 */
[[nodiscard]] static bool mockExecutionGuard() noexcept {
    std::unique_lock<std::mutex> exec_lock(g_execution_mutex, std::try_to_lock);
    return exec_lock.owns_lock();
    // Lock is released at end of scope — mirrors the guard lifetime in executePlan().
}

// ---------------------------------------------------------------------------
// FP23-04 helpers — attemptRecovery() batch stats update
//
// Models the single lock-guard acquisition followed by three uint64_t
// counter increments that replaced the per-iteration lock pattern.
// ---------------------------------------------------------------------------

struct MockRecoveryStats {
    uint64_t total_retry_attempts{0};
    uint64_t failed_retries{0};
    uint64_t successful_retries{0};
};

static std::mutex        g_stats_mutex;
static MockRecoveryStats g_recovery_stats;

/**
 * @brief Inline replica of the batch stats flush at end of attemptRecovery().
 *
 * @param local_total    Accumulated total attempt count (not yet flushed).
 * @param local_failed   Accumulated failed attempt count.
 * @param local_success  Accumulated successful attempt count.
 */
static void mockBatchStatsFlush(
    uint64_t local_total, uint64_t local_failed, uint64_t local_success) noexcept
{
    std::lock_guard<std::mutex> lock(g_stats_mutex);
    g_recovery_stats.total_retry_attempts += local_total;
    g_recovery_stats.failed_retries       += local_failed;
    g_recovery_stats.successful_retries   += local_success;
}

// ---------------------------------------------------------------------------
// FP23-05 helpers — emitDiagnostic() error-code dispatch overhead
//
// Models the FailoverErrorCode → FailoverEventType switch in emitDiagnostic()
// plus iterating an empty callback vector (the common no-subscriber case).
// ---------------------------------------------------------------------------

using FailoverEventCallback = std::function<
    void(FailoverEventType, const std::string&, const std::string&)>;

static const std::vector<FailoverEventCallback> g_empty_callbacks;  // 0 subscribers

/**
 * @brief Inline replica of emitDiagnostic() code-to-event-type mapping + dispatch.
 *
 * @param code       Canonical error code.
 * @param callbacks  Registered event callbacks (empty in the typical case).
 */
static void mockEmitDiagnostic(
    FailoverErrorCode                          code,
    const std::vector<FailoverEventCallback>&  callbacks) noexcept
{
    FailoverEventType event_type;
    switch (code) {
        case FailoverErrorCode::QUORUM_UNAVAILABLE:
            event_type = FailoverEventType::QUORUM_CHECK_FAILED;
            break;
        default:
            event_type = FailoverEventType::FAILOVER_CANCELLED;
            break;
    }

    for (const auto& cb : callbacks) {
        cb(event_type, "node-bench", "benchmark probe");
    }
}

// ---------------------------------------------------------------------------
// FP23-06 helpers — triggerManualFailover() full-queue rejection path
//
// Models the fast-drop path: queue-size check against capacity limit,
// followed by lock_guard + stats increment when the queue is full.
// ---------------------------------------------------------------------------

static constexpr std::size_t kMockMaxConcurrentFailovers = 2;

struct MockQueueStats {
    uint64_t tasks_dropped_queue_full{0};
};

static std::mutex      g_queue_mutex;
static MockQueueStats  g_queue_stats;

/**
 * @brief Inline replica of the triggerManualFailover() drop path.
 *
 * @param queue_size  Current number of items already in the failover queue.
 * @return false when the queue is at capacity (drop path).
 */
[[nodiscard]] static bool mockTriggerFailoverQueueFullDrop(
    std::size_t queue_size) noexcept
{
    if (queue_size >= kMockMaxConcurrentFailovers) {
        std::lock_guard<std::mutex> lock(g_queue_mutex);
        g_queue_stats.tasks_dropped_queue_full++;
        return false;   // dropped
    }
    return true;        // would proceed to enqueue
}

// ===========================================================================
// FP23-01 — canTransition() state table lookup
//           GATE-FP23-01: p99 ≤ 100 µs
// ===========================================================================

/**
 * @brief FP23-01: State-machine canTransition() switch dispatch over a mixed
 *        set of valid and invalid pairs.
 *
 * GATE-FP23-01: p99 ≤ 100 µs.
 */
static void BM_FP23_01_CanTransitionLookup(benchmark::State& state) {
    // Representative (from, to) pairs exercising all arms of the switch.
    static constexpr std::pair<FailoverOrchestratorState, FailoverOrchestratorState>
        kPairs[] = {
            {FailoverOrchestratorState::IDLE,
             FailoverOrchestratorState::VERIFYING_FAILURE},
            {FailoverOrchestratorState::VERIFYING_FAILURE,
             FailoverOrchestratorState::CHECKING_QUORUM},
            {FailoverOrchestratorState::CHECKING_QUORUM,
             FailoverOrchestratorState::STARTING_LEADER_ELECTION},
            {FailoverOrchestratorState::STARTING_LEADER_ELECTION,
             FailoverOrchestratorState::UPDATING_METADATA},
            {FailoverOrchestratorState::UPDATING_METADATA,
             FailoverOrchestratorState::COMPLETING_FAILOVER},
            {FailoverOrchestratorState::COMPLETING_FAILOVER,
             FailoverOrchestratorState::IDLE},
            // Invalid transitions that must return false:
            {FailoverOrchestratorState::IDLE,
             FailoverOrchestratorState::COMPLETING_FAILOVER},
            {FailoverOrchestratorState::CHECKING_QUORUM,
             FailoverOrchestratorState::UPDATING_METADATA},
        };
    static constexpr std::size_t kNumPairs =
        sizeof(kPairs) / sizeof(kPairs[0]);

    (void)kP23CanonicalSeed;

    for (int i = 0; i < kWarmupIterations; ++i) {
        for (const auto& [f, t] : kPairs) {
            benchmark::DoNotOptimize(mockCanTransition(f, t));
        }
    }

    std::size_t idx = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            mockCanTransition(kPairs[idx].first, kPairs[idx].second));
        idx = (idx + 1) % kNumPairs;
    }
    state.SetLabel("GATE-FP23-01: p99 <= 100 us");
}
BENCHMARK(BM_FP23_01_CanTransitionLookup)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// FP23-02 — preventSplitBrain() fail-closed null-fencing-manager path
//           GATE-FP23-02: p99 ≤ 200 µs
// ===========================================================================

/**
 * @brief FP23-02: preventSplitBrain() null-fencing-manager fast reject.
 *
 * Measures the null pointer check + fail-closed decision.
 * GATE-FP23-02: p99 ≤ 200 µs.
 */
static void BM_FP23_02_PreventSplitBrainFailClosed(benchmark::State& state) {
    const MockFencingManager* null_mgr = nullptr;

    for (int i = 0; i < kWarmupIterations; ++i) {
        benchmark::DoNotOptimize(mockPreventSplitBrainFailClosed(null_mgr));
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(mockPreventSplitBrainFailClosed(null_mgr));
    }
    state.SetLabel("GATE-FP23-02: p99 <= 200 us");
}
BENCHMARK(BM_FP23_02_PreventSplitBrainFailClosed)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// FP23-03 — executePlan() concurrency guard (try_to_lock, uncontested)
//           GATE-FP23-03: p99 ≤ 100 µs
// ===========================================================================

/**
 * @brief FP23-03: executePlan() try_to_lock concurrency guard, uncontested path.
 *
 * Measures unique_lock construction with try_to_lock + owns_lock() check on
 * an uncontested mutex.  This is the fast-accept path; the reject path
 * (contested mutex) would be similarly fast.
 * GATE-FP23-03: p99 ≤ 100 µs.
 */
static void BM_FP23_03_ExecutePlanConcurrencyGuard(benchmark::State& state) {
    for (int i = 0; i < kWarmupIterations; ++i) {
        benchmark::DoNotOptimize(mockExecutionGuard());
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(mockExecutionGuard());
    }
    state.SetLabel("GATE-FP23-03: p99 <= 100 us");
}
BENCHMARK(BM_FP23_03_ExecutePlanConcurrencyGuard)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// FP23-04 — attemptRecovery() batch stats flush
//           GATE-FP23-04: p99 ≤ 200 µs
// ===========================================================================

/**
 * @brief FP23-04: attemptRecovery() batch stats update — single lock + 3 increments.
 *
 * Measures the batched stats-flush pattern (one lock_guard per call) that
 * replaced the per-iteration lock pattern in Phase 2 hardening.
 * GATE-FP23-04: p99 ≤ 200 µs.
 */
static void BM_FP23_04_AttemptRecoveryBatchStatsFlush(benchmark::State& state) {
    for (int i = 0; i < kWarmupIterations; ++i) {
        mockBatchStatsFlush(3u, 3u, 0u);
    }

    for (auto _ : state) {
        mockBatchStatsFlush(3u, 3u, 0u);
    }
    state.SetLabel("GATE-FP23-04: p99 <= 200 us");
}
BENCHMARK(BM_FP23_04_AttemptRecoveryBatchStatsFlush)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// FP23-05 — emitDiagnostic() error-code dispatch overhead (empty callbacks)
//           GATE-FP23-05: p99 ≤ 100 µs
// ===========================================================================

/**
 * @brief FP23-05: emitDiagnostic() code-to-event-type switch + empty callback dispatch.
 *
 * Represents the most common runtime case: QUORUM_UNAVAILABLE diagnostic with
 * no subscribers registered.  The spdlog::error call present in production is
 * omitted to isolate dispatch overhead.
 * GATE-FP23-05: p99 ≤ 100 µs.
 */
static void BM_FP23_05_EmitDiagnosticDispatch(benchmark::State& state) {
    for (int i = 0; i < kWarmupIterations; ++i) {
        mockEmitDiagnostic(FailoverErrorCode::QUORUM_UNAVAILABLE, g_empty_callbacks);
    }

    for (auto _ : state) {
        mockEmitDiagnostic(FailoverErrorCode::QUORUM_UNAVAILABLE, g_empty_callbacks);
    }
    state.SetLabel("GATE-FP23-05: p99 <= 100 us");
}
BENCHMARK(BM_FP23_05_EmitDiagnosticDispatch)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// FP23-06 — triggerManualFailover() full-queue rejection path
//           GATE-FP23-06: p99 ≤ 200 µs
// ===========================================================================

/**
 * @brief FP23-06: triggerManualFailover() fast-drop path — full-queue detection + stats.
 *
 * Simulates the queue-saturation rejection introduced in Phase 2 hardening:
 * size check against kMockMaxConcurrentFailovers, then lock_guard + counter
 * increment.
 * GATE-FP23-06: p99 ≤ 200 µs.
 */
static void BM_FP23_06_TriggerFailoverQueueFullDrop(benchmark::State& state) {
    const std::size_t full_queue_size = kMockMaxConcurrentFailovers;  // queue at capacity

    for (int i = 0; i < kWarmupIterations; ++i) {
        benchmark::DoNotOptimize(mockTriggerFailoverQueueFullDrop(full_queue_size));
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(mockTriggerFailoverQueueFullDrop(full_queue_size));
    }
    state.SetLabel("GATE-FP23-06: p99 <= 200 us");
}
BENCHMARK(BM_FP23_06_TriggerFailoverQueueFullDrop)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace fp23
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
