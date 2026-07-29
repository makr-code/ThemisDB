// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_failover_phase2_phase3_focused.cpp
 * @brief Phase 2 / Phase 3 focused tests (P23-01..P23-08).
 *
 * Validates the hardening items delivered in Phase 2 (Core Implementation)
 * and Phase 3 (Error Handling and Edge Cases):
 *
 *   P23-01  canTransition returns false for impossible state transitions
 *   P23-02  canTransition returns true for all valid forward transitions
 *   P23-03  preventSplitBrain fails closed when no fencing manager is configured
 *   P23-04  attemptRecovery stats are correct after max attempts exhausted
 *   P23-05  triggerManualFailover drops when queue full and records the stat
 *   P23-06  executePlan concurrent call is rejected with explicit error message
 *   P23-07  emitDiagnostic fires the registered event callback for QUORUM_UNAVAILABLE
 *   P23-08  attemptRecovery stats are batch-updated: total==N and failed==N
 *
 * All tests are self-contained: no network I/O, no filesystem I/O.
 * Canonical PRNG seed: kPhase23Seed = 42.
 *
 * @see include/failover/auto_failover_manager.h
 * @see include/failover/disaster_recovery_manager.h
 * @see src/failover/ROADMAP.md — Phase 2 / Phase 3 items
 */

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "failover/auto_failover_manager.h"
#include "failover/disaster_recovery_manager.h"
#include "failover/failover_api_contract.h"

using namespace std::chrono_literals;
using namespace themis::failover;

namespace {

// ---------------------------------------------------------------------------
// Canonical seed
// ---------------------------------------------------------------------------
static constexpr uint64_t kPhase23Seed = 42;

// ---------------------------------------------------------------------------
// Config helpers
// ---------------------------------------------------------------------------

/// Minimal AutoFailoverConfig with fast timers for unit testing.
AutoFailoverConfig makeFastConfig() {
    AutoFailoverConfig cfg;
    cfg.health_check_interval           = 10ms;
    cfg.failure_detection_interval      = 10ms;
    cfg.failover_timeout                = 50ms;
    cfg.spare_activation_timeout        = 50ms;
    cfg.leader_election_timeout         = 50ms;
    cfg.recovery_retry_interval         = 0ms;   // no sleep between retries in tests
    cfg.max_recovery_attempts           = 3;
    cfg.enable_automatic_failover       = true;
    cfg.enable_automatic_recovery       = false; // avoid 5-second delay in unit tests
    cfg.enable_spare_activation         = false;
    cfg.enable_network_partition_detection = false;
    cfg.enable_split_brain_prevention   = false;
    cfg.max_concurrent_failovers        = 4;
    cfg.queue_pressure_threshold        = 0.75f;
    return cfg;
}

/// Minimal DisasterRecoveryConfig safe to use with null managers.
DisasterRecoveryConfig makeDRTestConfig() {
    DisasterRecoveryConfig cfg;
    cfg.require_quorum               = false;
    cfg.enforce_epoch_fencing        = false;
    cfg.allow_dry_run_without_managers = true;
    cfg.precheck_timeout             = 100ms;
    cfg.catchup_timeout              = 100ms;
    cfg.verification_timeout         = 100ms;
    cfg.max_verification_retries     = 1;
    return cfg;
}

/// Minimal valid dry-run plan.
DisasterRecoveryPlan makeDryRunPlan(const std::string& plan_id = "plan-p23") {
    DisasterRecoveryPlan plan;
    plan.plan_id       = plan_id;
    plan.primary_site  = "site-primary";
    plan.recovery_site = "site-recovery";
    plan.dry_run       = true;
    plan.shift_traffic = false;
    return plan;
}

}  // namespace

// ===========================================================================
// P23-01 — canTransition returns false for impossible state transitions
// ===========================================================================

TEST(FailoverPhase2Phase3, P23_01_CanTransitionFalseForImpossibleTransitions) {
    (void)kPhase23Seed;
    AutoFailoverConfig cfg = makeFastConfig();
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    // Skipping states in the forward path is invalid.
    EXPECT_FALSE(mgr.canTransition(FailoverOrchestratorState::IDLE,
                                    FailoverOrchestratorState::COMPLETING_FAILOVER));
    EXPECT_FALSE(mgr.canTransition(FailoverOrchestratorState::IDLE,
                                    FailoverOrchestratorState::UPDATING_METADATA));
    EXPECT_FALSE(mgr.canTransition(FailoverOrchestratorState::IDLE,
                                    FailoverOrchestratorState::LEADER_ELECTION_IN_PROGRESS));
    EXPECT_FALSE(mgr.canTransition(FailoverOrchestratorState::VERIFYING_FAILURE,
                                    FailoverOrchestratorState::COMPLETING_FAILOVER));
    EXPECT_FALSE(mgr.canTransition(FailoverOrchestratorState::CHECKING_QUORUM,
                                    FailoverOrchestratorState::UPDATING_METADATA));
    // COMPLETING_FAILOVER may not transition to FAILED directly — to is IDLE.
    EXPECT_FALSE(mgr.canTransition(FailoverOrchestratorState::COMPLETING_FAILOVER,
                                    FailoverOrchestratorState::CHECKING_QUORUM));
}

// ===========================================================================
// P23-02 — canTransition returns true for all valid forward transitions
// ===========================================================================

TEST(FailoverPhase2Phase3, P23_02_CanTransitionTrueForValidForwardTransitions) {
    AutoFailoverConfig cfg = makeFastConfig();
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    // Primary forward path
    EXPECT_TRUE(mgr.canTransition(FailoverOrchestratorState::IDLE,
                                   FailoverOrchestratorState::VERIFYING_FAILURE));
    EXPECT_TRUE(mgr.canTransition(FailoverOrchestratorState::VERIFYING_FAILURE,
                                   FailoverOrchestratorState::CHECKING_QUORUM));
    EXPECT_TRUE(mgr.canTransition(FailoverOrchestratorState::CHECKING_QUORUM,
                                   FailoverOrchestratorState::STARTING_LEADER_ELECTION));
    EXPECT_TRUE(mgr.canTransition(FailoverOrchestratorState::STARTING_LEADER_ELECTION,
                                   FailoverOrchestratorState::LEADER_ELECTION_IN_PROGRESS));
    // Direct shortcut (processFailover skips LEADER_ELECTION_IN_PROGRESS)
    EXPECT_TRUE(mgr.canTransition(FailoverOrchestratorState::STARTING_LEADER_ELECTION,
                                   FailoverOrchestratorState::UPDATING_METADATA));
    EXPECT_TRUE(mgr.canTransition(FailoverOrchestratorState::LEADER_ELECTION_IN_PROGRESS,
                                   FailoverOrchestratorState::UPDATING_METADATA));
    EXPECT_TRUE(mgr.canTransition(FailoverOrchestratorState::UPDATING_METADATA,
                                   FailoverOrchestratorState::COMPLETING_FAILOVER));
    EXPECT_TRUE(mgr.canTransition(FailoverOrchestratorState::COMPLETING_FAILOVER,
                                   FailoverOrchestratorState::IDLE));

    // Any state → FAILED is always valid
    EXPECT_TRUE(mgr.canTransition(FailoverOrchestratorState::IDLE,
                                   FailoverOrchestratorState::FAILED));
    EXPECT_TRUE(mgr.canTransition(FailoverOrchestratorState::VERIFYING_FAILURE,
                                   FailoverOrchestratorState::FAILED));
    EXPECT_TRUE(mgr.canTransition(FailoverOrchestratorState::UPDATING_METADATA,
                                   FailoverOrchestratorState::FAILED));

    // Any state → IDLE is always valid (reset / stop)
    EXPECT_TRUE(mgr.canTransition(FailoverOrchestratorState::FAILED,
                                   FailoverOrchestratorState::IDLE));
    EXPECT_TRUE(mgr.canTransition(FailoverOrchestratorState::LEADER_ELECTION_IN_PROGRESS,
                                   FailoverOrchestratorState::IDLE));
}

// ===========================================================================
// P23-03 — preventSplitBrain fails closed when no fencing manager configured
// ===========================================================================

TEST(FailoverPhase2Phase3, P23_03_PreventSplitBrainFailsClosedWithNoFencingManager) {
    AutoFailoverConfig cfg = makeFastConfig();
    cfg.enable_split_brain_prevention = true;
    // Pass null fencing_manager (last argument).
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    // Must return false (fail closed): without a fencing manager we cannot
    // guarantee exclusive leadership, so the safe outcome is to reject.
    EXPECT_FALSE(mgr.testPreventSplitBrain("node-x"));
}

// ===========================================================================
// P23-04 — attemptRecovery stats are correct after max attempts exhausted
// ===========================================================================

TEST(FailoverPhase2Phase3, P23_04_AttemptRecoveryStatsAfterMaxAttemptsExhausted) {
    AutoFailoverConfig cfg = makeFastConfig();
    cfg.max_recovery_attempts  = 3;
    cfg.recovery_retry_interval = 0ms;
    // Pass null health_monitor so waitForNodeRecovery always returns false.
    AutoFailoverManager mgr(cfg, nullptr, nullptr /*health_monitor=null*/, nullptr, nullptr);

    const bool result = mgr.testAttemptRecovery("node-dead");

    EXPECT_FALSE(result);

    const auto stats = mgr.getStatistics();
    EXPECT_EQ(stats.total_retry_attempts, 3u);
    EXPECT_EQ(stats.failed_retries,       3u);
    EXPECT_EQ(stats.successful_retries,   0u);
}

// ===========================================================================
// P23-05 — triggerManualFailover drops when queue full, records stat
// ===========================================================================

TEST(FailoverPhase2Phase3, P23_05_TriggerManualFailoverDropsWhenQueueFull) {
    AutoFailoverConfig cfg = makeFastConfig();
    cfg.max_concurrent_failovers = 1;

    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);
    ASSERT_TRUE(mgr.start());

    const bool first  = mgr.triggerManualFailover("node-a");
    const bool second = mgr.triggerManualFailover("node-b"); // queue full → dropped

    EXPECT_TRUE(first);
    EXPECT_FALSE(second);

    // Allow the worker time to record the drop statistic.
    std::this_thread::sleep_for(30ms);
    const auto stats = mgr.getStatistics();
    EXPECT_GE(stats.tasks_dropped_queue_full, 1u);

    ASSERT_TRUE(mgr.stop());
}

// ===========================================================================
// P23-06 — executePlan concurrent call is rejected with explicit error
// ===========================================================================

TEST(FailoverPhase2Phase3, P23_06_ExecutePlanConcurrentCallRejected) {
    DisasterRecoveryManager mgr(makeDRTestConfig(), nullptr, nullptr);

    // Gate objects: first execution signals entry then blocks until released.
    std::promise<void> hook_entered;
    std::promise<void> hook_release;
    auto hook_entered_future = hook_entered.get_future();
    auto hook_release_future = hook_release.get_future();

    mgr.setStepHook(DisasterRecoveryStep::PRECHECKS,
        [&](const DisasterRecoveryPlan&, std::string& detail) -> bool {
            hook_entered.set_value();          // signal: first execution is inside
            hook_release_future.wait();        // block until test releases us
            detail = "prechecks done";
            return true;
        });

    const auto plan  = makeDryRunPlan("plan-p23-06-a");
    const auto plan2 = makeDryRunPlan("plan-p23-06-b");

    DisasterRecoveryResult first_result;
    std::thread t([&]() {
        first_result = mgr.executePlan(plan);
    });

    // Wait until the first execution has entered the blocking hook (holds mutex).
    hook_entered_future.wait();

    // This concurrent call must be rejected immediately (try_to_lock fails).
    const auto rejected = mgr.executePlan(plan2);

    EXPECT_FALSE(rejected.success);
    EXPECT_EQ(rejected.error_message, "concurrent execution rejected");

    // Release the first execution so the thread can complete cleanly.
    hook_release.set_value();
    t.join();

    EXPECT_TRUE(first_result.success);
}

// ===========================================================================
// P23-07 — emitDiagnostic fires registered event callback for QUORUM_UNAVAILABLE
// ===========================================================================

TEST(FailoverPhase2Phase3, P23_07_EmitDiagnosticFiresCallbackForQuorumUnavailable) {
    AutoFailoverConfig cfg = makeFastConfig();
    AutoFailoverManager mgr(cfg, nullptr, nullptr, nullptr, nullptr);

    std::atomic<int> quorum_failed_events{0};
    mgr.registerEventCallback(
        [&](FailoverEventType type, const std::string&, const std::string&) {
            if (type == FailoverEventType::QUORUM_CHECK_FAILED) {
                quorum_failed_events.fetch_add(1, std::memory_order_relaxed);
            }
        });

    // Fire the diagnostic directly via the test accessor.
    mgr.testEmitDiagnostic(FailoverErrorCode::QUORUM_UNAVAILABLE,
                            "node-qtest",
                            "unit test — QUORUM_UNAVAILABLE diagnostic");

    EXPECT_EQ(quorum_failed_events.load(), 1);
}

// ===========================================================================
// P23-08 — attemptRecovery stats are batch-updated: total==N, failed==N
// ===========================================================================

TEST(FailoverPhase2Phase3, P23_08_AttemptRecoveryStatsBatchUpdated) {
    AutoFailoverConfig cfg = makeFastConfig();
    cfg.max_recovery_attempts   = 5;
    cfg.recovery_retry_interval = 0ms;
    // Null health_monitor forces all waitForNodeRecovery() calls to return false.
    AutoFailoverManager mgr(cfg, nullptr, nullptr /*health_monitor=null*/, nullptr, nullptr);

    mgr.testAttemptRecovery("node-batch");

    const auto stats = mgr.getStatistics();

    // Batch update contract: after N failed attempts the counters reflect
    // exactly N total and N failed (no double-counting, no partial flush).
    EXPECT_EQ(stats.total_retry_attempts, 5u);
    EXPECT_EQ(stats.failed_retries,       5u);
    EXPECT_EQ(stats.successful_retries,   0u);
}
