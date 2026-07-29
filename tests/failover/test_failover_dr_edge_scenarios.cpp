// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_failover_dr_edge_scenarios.cpp
 * @brief Phase 3 / Phase 4 — Disaster recovery step isolation and edge scenario
 *        focused tests (DRE-01..DRE-08).
 *
 * Tests are fully self-contained: no network I/O, no filesystem I/O.
 * External dependencies are avoided by using dry_run mode or step hooks.
 * Canonical PRNG seed: kDREdgeSeed = 42.
 *
 * ## Test families
 *
 * ### DRE-01..DRE-05 — Plan validation contract
 *   DRE-01  validatePlan rejects plan with empty plan_id
 *   DRE-02  validatePlan rejects plan with empty primary_site
 *   DRE-03  validatePlan rejects plan with empty recovery_site
 *   DRE-04  validatePlan rejects non-dry-run plan with missing snapshot_id
 *   DRE-05  validatePlan accepts dry-run plan without snapshot_id
 *
 * ### DRE-06..DRE-08 — Execution and diagnostics contract
 *   DRE-06  executePlan with invalid plan sets FAILED state and populates error_message
 *   DRE-07  dry-run plan completes all steps and reaches COMPLETED state
 *   DRE-08  step hook failure isolates to the faulted step; statistics updated
 *
 * @see include/failover/disaster_recovery_manager.h
 * @see src/failover/ROADMAP.md — Phase 3 / Phase 4 items
 */

#include <gtest/gtest.h>

#include "failover/disaster_recovery_manager.h"

#include <cstdint>
#include <string>
#include <vector>

using namespace themis::failover;

namespace {

/// Canonical PRNG seed (consistent with other failover test files).
static constexpr uint64_t kDREdgeSeed = 42;

/// Build a minimal DisasterRecoveryConfig suitable for unit tests.
/// @return Config with quorum not required and no fencing, safe with null managers.
DisasterRecoveryConfig makeTestConfig() {
    DisasterRecoveryConfig cfg;
    cfg.require_quorum          = false;
    cfg.enforce_epoch_fencing   = false;
    cfg.allow_dry_run_without_managers = true;
    cfg.precheck_timeout        = std::chrono::milliseconds{100};
    cfg.catchup_timeout         = std::chrono::milliseconds{100};
    cfg.verification_timeout    = std::chrono::milliseconds{100};
    cfg.max_verification_retries = 1;
    return cfg;
}

/// Build a minimal valid dry-run plan.
/// @return A DisasterRecoveryPlan with all required fields set and dry_run == true.
DisasterRecoveryPlan makeMinimalDryRunPlan() {
    DisasterRecoveryPlan plan;
    plan.plan_id       = "plan-drtest-42";
    plan.primary_site  = "site-primary";
    plan.recovery_site = "site-recovery";
    plan.dry_run       = true;
    plan.shift_traffic = false;
    return plan;
}

}  // namespace

// ===========================================================================
// DRE-01 — validatePlan rejects empty plan_id
// ===========================================================================

TEST(DisasterRecoveryEdgeDRE01, EmptyPlanIdRejected) {
    DisasterRecoveryManager mgr(makeTestConfig(), nullptr, nullptr);

    DisasterRecoveryPlan plan = makeMinimalDryRunPlan();
    plan.plan_id = "";

    std::string error;
    EXPECT_FALSE(mgr.validatePlan(plan, error));
    EXPECT_FALSE(error.empty()) << "Error message must be populated on rejection";
}

// ===========================================================================
// DRE-02 — validatePlan rejects empty primary_site
// ===========================================================================

TEST(DisasterRecoveryEdgeDRE02, EmptyPrimarySiteRejected) {
    DisasterRecoveryManager mgr(makeTestConfig(), nullptr, nullptr);

    DisasterRecoveryPlan plan = makeMinimalDryRunPlan();
    plan.primary_site = "";

    std::string error;
    EXPECT_FALSE(mgr.validatePlan(plan, error));
    EXPECT_FALSE(error.empty());
}

// ===========================================================================
// DRE-03 — validatePlan rejects empty recovery_site
// ===========================================================================

TEST(DisasterRecoveryEdgeDRE03, EmptyRecoverySiteRejected) {
    DisasterRecoveryManager mgr(makeTestConfig(), nullptr, nullptr);

    DisasterRecoveryPlan plan = makeMinimalDryRunPlan();
    plan.recovery_site = "";

    std::string error;
    EXPECT_FALSE(mgr.validatePlan(plan, error));
    EXPECT_FALSE(error.empty());
}

// ===========================================================================
// DRE-04 — validatePlan rejects non-dry-run with missing snapshot_id
// ===========================================================================

TEST(DisasterRecoveryEdgeDRE04, MissingSnapshotIdRejectedForNonDryRun) {
    DisasterRecoveryManager mgr(makeTestConfig(), nullptr, nullptr);

    DisasterRecoveryPlan plan = makeMinimalDryRunPlan();
    plan.dry_run      = false;
    plan.snapshot_id  = "";  // missing

    std::string error;
    EXPECT_FALSE(mgr.validatePlan(plan, error));
    EXPECT_FALSE(error.empty());
}

// ===========================================================================
// DRE-05 — validatePlan accepts dry-run without snapshot_id
// ===========================================================================

TEST(DisasterRecoveryEdgeDRE05, DryRunAcceptedWithoutSnapshotId) {
    DisasterRecoveryManager mgr(makeTestConfig(), nullptr, nullptr);

    DisasterRecoveryPlan plan = makeMinimalDryRunPlan();
    plan.dry_run     = true;
    plan.snapshot_id = "";  // allowed in dry-run

    std::string error;
    EXPECT_TRUE(mgr.validatePlan(plan, error))
        << "Dry-run must be accepted without snapshot_id; error: " << error;
}

// ===========================================================================
// DRE-06 — executePlan with invalid plan → FAILED state, error_message set
// ===========================================================================

TEST(DisasterRecoveryEdgeDRE06, InvalidPlanSetsFailedStateWithError) {
    DisasterRecoveryManager mgr(makeTestConfig(), nullptr, nullptr);

    DisasterRecoveryPlan plan;  // intentionally invalid: all fields empty

    const auto result = mgr.executePlan(plan);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.final_state, DisasterRecoveryState::FAILED);
    EXPECT_FALSE(result.error_message.empty())
        << "error_message must be populated for invalid plan";
}

// ===========================================================================
// DRE-07 — dry-run plan completes all steps → COMPLETED, success == true
// ===========================================================================

TEST(DisasterRecoveryEdgeDRE07, DryRunPlanCompletesSuccessfully) {
    DisasterRecoveryManager mgr(makeTestConfig(), nullptr, nullptr);

    const auto result = mgr.executePlan(makeMinimalDryRunPlan());

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.final_state, DisasterRecoveryState::COMPLETED);
    EXPECT_TRUE(result.error_message.empty())
        << "No error expected for successful dry-run";

    // All expected steps must be present in step_results
    const std::vector<DisasterRecoveryStep> expected_steps = {
        DisasterRecoveryStep::PRECHECKS,
        DisasterRecoveryStep::SNAPSHOT_VALIDATION,
        DisasterRecoveryStep::EPOCH_FENCING,
        DisasterRecoveryStep::RESTORE,
        DisasterRecoveryStep::REPLICA_CATCHUP,
        DisasterRecoveryStep::TRAFFIC_SHIFT,
        DisasterRecoveryStep::VERIFICATION,
    };
    ASSERT_EQ(result.step_results.size(), expected_steps.size())
        << "All seven DR steps must appear in step_results";
    for (std::size_t i = 0; i < expected_steps.size(); ++i) {
        EXPECT_EQ(result.step_results[i].step, expected_steps[i])
            << "Step at index " << i << " does not match expected order";
        EXPECT_TRUE(result.step_results[i].success)
            << "Step " << i << " must succeed in dry-run";
    }
}

// ===========================================================================
// DRE-08 — step hook failure isolates to faulted step; statistics updated
// ===========================================================================

TEST(DisasterRecoveryEdgeDRE08, StepHookFailureIsolatedAndStatisticsUpdated) {
    DisasterRecoveryManager mgr(makeTestConfig(), nullptr, nullptr);

    // Inject a failure hook at PRECHECKS to simulate a dependency-degraded scenario.
    mgr.setStepHook(DisasterRecoveryStep::PRECHECKS,
        [](const DisasterRecoveryPlan&, std::string& detail) -> bool {
            detail = "dependency-degraded: fencing unavailable";
            return false;
        });

    const auto result = mgr.executePlan(makeMinimalDryRunPlan());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.final_state, DisasterRecoveryState::FAILED);
    EXPECT_FALSE(result.error_message.empty());

    // Only PRECHECKS should appear in step_results (execution stops at first failure)
    ASSERT_GE(result.step_results.size(), 1u);
    EXPECT_EQ(result.step_results[0].step, DisasterRecoveryStep::PRECHECKS);
    EXPECT_FALSE(result.step_results[0].success);
    EXPECT_FALSE(result.step_results[0].message.empty())
        << "Failed step must carry a diagnostic message";

    // Statistics: total_runs and failed_runs must each be incremented
    const auto stats = mgr.getStatistics();
    EXPECT_EQ(stats.total_runs, 1u);
    EXPECT_EQ(stats.failed_runs, 1u);
    EXPECT_EQ(stats.successful_runs, 0u);
}
