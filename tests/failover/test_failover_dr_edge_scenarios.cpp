// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_failover_dr_edge_scenarios.cpp
 * @brief Phase 3 / Phase 4 — Disaster recovery step isolation and edge scenario
 *        focused tests (DRE-01..DRE-08).
 *
 * Tests are fully self-contained: no network I/O; persistence cases use
 * temporary filesystem paths.
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
#include "sharding/epoch_fencing.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

using namespace themis::failover;

namespace {

/// Canonical PRNG seed (consistent with other failover test files).
[[maybe_unused]] static constexpr uint64_t kDREdgeSeed = 42;

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
    cfg.metadata_lock_timeout   = std::chrono::milliseconds{1000};
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

DisasterRecoveryPlan makeMinimalProductionPlan() {
    auto plan = makeMinimalDryRunPlan();
    plan.dry_run = false;
    plan.snapshot_id = "snapshot-drtest";
    plan.snapshot_checksum = "sha256:recovery-fixture";
    plan.required_lsn = 42;
    plan.required_term = 7;
    return plan;
}

class TemporaryMetadataLog {
public:
    TemporaryMetadataLog()
        : path(std::filesystem::temp_directory_path() /
               ("themis_dr_metadata_" +
                std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                ".log")) {}

    ~TemporaryMetadataLog() {
        std::error_code error;
        std::filesystem::remove(path, error);
        auto lock_path = path;
        lock_path += ".lock";
        std::filesystem::remove(lock_path, error);
    }

    std::filesystem::path path;
};

DisasterRecoveryConfig makeProductionConfig(const std::filesystem::path& metadata_path) {
    auto config = makeTestConfig();
    config.require_quorum = true;
    config.enforce_epoch_fencing = true;
    config.metadata_log_path = metadata_path.string();
    return config;
}

DisasterRecoveryManager::StepHook successfulAdapterHook() {
    return [](const DisasterRecoveryPlan&, std::string& detail) {
        detail = "test adapter acknowledged operation";
        return true;
    };
}

void installAdapterHooks(DisasterRecoveryManager& manager,
                         std::initializer_list<DisasterRecoveryStep> steps) {
    for (const auto step : steps) {
        manager.setStepHook(step, successfulAdapterHook());
        manager.setVerificationHook(
            step,
            [](const DisasterRecoveryPlan& plan, std::string& detail)
                -> std::optional<DisasterRecoveryVerificationProof> {
                detail = "independent adapter read-back passed";
                DisasterRecoveryVerificationProof proof;
                proof.checksum = plan.snapshot_checksum;
                proof.lsn = plan.required_lsn;
                proof.term = plan.required_term;
                proof.observed_site = plan.recovery_site;
                return proof;
            });
    }
}

#if defined(__unix__) || defined(__APPLE__)
void installCountingAdapterHooks(DisasterRecoveryManager& manager,
                                 const std::filesystem::path& operation_log) {
    installAdapterHooks(manager, {DisasterRecoveryStep::SNAPSHOT_VALIDATION,
                                  DisasterRecoveryStep::RESTORE,
                                  DisasterRecoveryStep::REPLICA_CATCHUP,
                                  DisasterRecoveryStep::TRAFFIC_SHIFT,
                                  DisasterRecoveryStep::VERIFICATION});
    for (const auto step : {DisasterRecoveryStep::SNAPSHOT_VALIDATION,
                            DisasterRecoveryStep::RESTORE,
                            DisasterRecoveryStep::REPLICA_CATCHUP,
                            DisasterRecoveryStep::TRAFFIC_SHIFT,
                            DisasterRecoveryStep::VERIFICATION}) {
        manager.setStepHook(step, [operation_log, step](
                                      const DisasterRecoveryPlan&, std::string& detail) {
            std::ofstream output(operation_log, std::ios::out | std::ios::app);
            if (!output) {
                detail = "cannot record test adapter invocation";
                return false;
            }
            output << static_cast<unsigned int>(step) << '\n';
            output.flush();
            if (!output.good()) {
                detail = "cannot flush test adapter invocation";
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{25});
            detail = "counted test adapter acknowledged operation";
            return true;
        });
    }
}
#endif

}  // namespace

// ===========================================================================
// DRE-01 — validatePlan rejects empty plan_id
// ===========================================================================

TEST(DisasterRecoveryEdgeDRE01, EmptyPlanIdRejected) {
    DisasterRecoveryManager mgr(makeTestConfig(), nullptr, nullptr);

    DisasterRecoveryPlan plan = makeMinimalDryRunPlan();
    plan.plan_id = "";

    std::string error = {};
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

    std::string error = {};
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

    std::string error = {};
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

    std::string error = {};
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

    std::string error = {};
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
    TemporaryMetadataLog metadata;
    DisasterRecoveryManager mgr(makeProductionConfig(metadata.path), nullptr,
        std::make_shared<themis::sharding::EpochFencingManager>(
            themis::sharding::EpochFencingConfig{}));
    mgr.testSetQuorumOverride(false);

    const auto result = mgr.executePlan(makeMinimalProductionPlan());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.final_state, DisasterRecoveryState::FAILED);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_EQ(result.error_code, DisasterRecoveryErrorCode::QUORUM_UNAVAILABLE);

    // Missing quorum must stop the plan before any adapter is called.
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

TEST(DisasterRecoveryEdgeDRE09, ProductionPlanRequiresSnapshotValidationAdapter) {
    TemporaryMetadataLog metadata;
    DisasterRecoveryManager mgr(makeProductionConfig(metadata.path), nullptr,
        std::make_shared<themis::sharding::EpochFencingManager>(
            themis::sharding::EpochFencingConfig{}));
    mgr.testSetQuorumOverride(true);

    const auto result = mgr.executePlan(makeMinimalProductionPlan());

    EXPECT_FALSE(result.success);
    ASSERT_EQ(result.step_results.size(), 2u);
    EXPECT_EQ(result.step_results.back().step,
              DisasterRecoveryStep::SNAPSHOT_VALIDATION);
    EXPECT_EQ(result.error_code, DisasterRecoveryErrorCode::NOT_CONFIGURED);
}

TEST(DisasterRecoveryEdgeDRE10, ProductionPlanRequiresRestoreAdapter) {
    TemporaryMetadataLog metadata;
    DisasterRecoveryManager mgr(makeProductionConfig(metadata.path), nullptr,
        std::make_shared<themis::sharding::EpochFencingManager>(
            themis::sharding::EpochFencingConfig{}));
    mgr.testSetQuorumOverride(true);
    installAdapterHooks(mgr, {DisasterRecoveryStep::SNAPSHOT_VALIDATION});

    const auto result = mgr.executePlan(makeMinimalProductionPlan());

    EXPECT_FALSE(result.success);
    ASSERT_EQ(result.step_results.size(), 4u);
    EXPECT_EQ(result.step_results.back().step, DisasterRecoveryStep::RESTORE);
    EXPECT_EQ(result.error_code, DisasterRecoveryErrorCode::NOT_CONFIGURED);
}

TEST(DisasterRecoveryEdgeDRE11, QuorumDoesNotSatisfyCatchupRequirement) {
    TemporaryMetadataLog metadata;
    DisasterRecoveryManager mgr(makeProductionConfig(metadata.path), nullptr,
        std::make_shared<themis::sharding::EpochFencingManager>(
            themis::sharding::EpochFencingConfig{}));
    mgr.testSetQuorumOverride(true);
    installAdapterHooks(mgr, {DisasterRecoveryStep::SNAPSHOT_VALIDATION,
                              DisasterRecoveryStep::RESTORE});

    const auto result = mgr.executePlan(makeMinimalProductionPlan());

    EXPECT_FALSE(result.success);
    ASSERT_EQ(result.step_results.size(), 5u);
    EXPECT_EQ(result.step_results.back().step,
              DisasterRecoveryStep::REPLICA_CATCHUP);
    EXPECT_EQ(result.error_code, DisasterRecoveryErrorCode::NOT_CONFIGURED);
}

TEST(DisasterRecoveryEdgeDRE12, ProductionPlanRequiresTrafficShiftAdapter) {
    TemporaryMetadataLog metadata;
    DisasterRecoveryManager mgr(makeProductionConfig(metadata.path), nullptr,
        std::make_shared<themis::sharding::EpochFencingManager>(
            themis::sharding::EpochFencingConfig{}));
    mgr.testSetQuorumOverride(true);
    installAdapterHooks(mgr, {DisasterRecoveryStep::SNAPSHOT_VALIDATION,
                              DisasterRecoveryStep::RESTORE,
                              DisasterRecoveryStep::REPLICA_CATCHUP});

    const auto result = mgr.executePlan(makeMinimalProductionPlan());

    EXPECT_FALSE(result.success);
    ASSERT_EQ(result.step_results.size(), 6u);
    EXPECT_EQ(result.step_results.back().step,
              DisasterRecoveryStep::TRAFFIC_SHIFT);
    EXPECT_EQ(result.error_code, DisasterRecoveryErrorCode::NOT_CONFIGURED);
}

TEST(DisasterRecoveryEdgeDRE13, ProductionPlanRequiresRecoveredStateVerification) {
    TemporaryMetadataLog metadata;
    DisasterRecoveryManager mgr(makeProductionConfig(metadata.path), nullptr,
        std::make_shared<themis::sharding::EpochFencingManager>(
            themis::sharding::EpochFencingConfig{}));
    mgr.testSetQuorumOverride(true);
    auto plan = makeMinimalProductionPlan();
    plan.shift_traffic = false;
    installAdapterHooks(mgr, {DisasterRecoveryStep::SNAPSHOT_VALIDATION,
                              DisasterRecoveryStep::RESTORE,
                              DisasterRecoveryStep::REPLICA_CATCHUP});

    const auto result = mgr.executePlan(plan);

    EXPECT_FALSE(result.success);
    ASSERT_EQ(result.step_results.size(), 7u);
    EXPECT_EQ(result.step_results.back().step,
              DisasterRecoveryStep::VERIFICATION);
    EXPECT_EQ(result.error_code, DisasterRecoveryErrorCode::NOT_CONFIGURED);
}

TEST(DisasterRecoveryEdgeDRE14, AdapterAcknowledgementWithoutVerifierIsNotConfigured) {
    TemporaryMetadataLog metadata;
    DisasterRecoveryManager mgr(makeProductionConfig(metadata.path), nullptr,
        std::make_shared<themis::sharding::EpochFencingManager>(
            themis::sharding::EpochFencingConfig{}));
    mgr.testSetQuorumOverride(true);
    mgr.setStepHook(DisasterRecoveryStep::SNAPSHOT_VALIDATION, successfulAdapterHook());

    const auto result = mgr.executePlan(makeMinimalProductionPlan());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code, DisasterRecoveryErrorCode::NOT_CONFIGURED);
    EXPECT_EQ(result.step_results.back().step,
              DisasterRecoveryStep::SNAPSHOT_VALIDATION);
}

TEST(DisasterRecoveryEdgeDRE15, ChecksumMismatchFailsIndependentVerification) {
    TemporaryMetadataLog metadata;
    DisasterRecoveryManager mgr(makeProductionConfig(metadata.path), nullptr,
        std::make_shared<themis::sharding::EpochFencingManager>(
            themis::sharding::EpochFencingConfig{}));
    mgr.testSetQuorumOverride(true);
    mgr.setStepHook(DisasterRecoveryStep::SNAPSHOT_VALIDATION, successfulAdapterHook());
    mgr.setVerificationHook(
        DisasterRecoveryStep::SNAPSHOT_VALIDATION,
        [](const DisasterRecoveryPlan&, std::string& detail)
            -> std::optional<DisasterRecoveryVerificationProof> {
            detail = "snapshot checksum differs";
            DisasterRecoveryVerificationProof proof;
            proof.checksum = "sha256:wrong";
            return proof;
        });

    const auto result = mgr.executePlan(makeMinimalProductionPlan());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code, DisasterRecoveryErrorCode::VERIFICATION_FAILED);
    EXPECT_EQ(result.step_results.back().step,
              DisasterRecoveryStep::SNAPSHOT_VALIDATION);
}

TEST(DisasterRecoveryEdgeDRE20, AdapterNegativeAcknowledgementFailsStep) {
    TemporaryMetadataLog metadata;
    DisasterRecoveryManager mgr(makeProductionConfig(metadata.path), nullptr,
        std::make_shared<themis::sharding::EpochFencingManager>(
            themis::sharding::EpochFencingConfig{}));
    mgr.testSetQuorumOverride(true);
    mgr.setStepHook(
        DisasterRecoveryStep::SNAPSHOT_VALIDATION,
        [](const DisasterRecoveryPlan&, std::string& detail) {
            detail = "snapshot adapter rejected the operation";
            return false;
        });
    mgr.setVerificationHook(
        DisasterRecoveryStep::SNAPSHOT_VALIDATION,
        [](const DisasterRecoveryPlan& plan, std::string&)
            -> std::optional<DisasterRecoveryVerificationProof> {
            DisasterRecoveryVerificationProof proof;
            proof.checksum = plan.snapshot_checksum;
            return proof;
        });

    const auto result = mgr.executePlan(makeMinimalProductionPlan());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code, DisasterRecoveryErrorCode::ADAPTER_FAILURE);
    EXPECT_EQ(result.step_results.back().step,
              DisasterRecoveryStep::SNAPSHOT_VALIDATION);
}

TEST(DisasterRecoveryEdgeDRE16, StaleReplicationPositionFailsCatchup) {
    TemporaryMetadataLog metadata;
    DisasterRecoveryManager mgr(makeProductionConfig(metadata.path), nullptr,
        std::make_shared<themis::sharding::EpochFencingManager>(
            themis::sharding::EpochFencingConfig{}));
    mgr.testSetQuorumOverride(true);
    installAdapterHooks(mgr, {DisasterRecoveryStep::SNAPSHOT_VALIDATION,
                              DisasterRecoveryStep::RESTORE,
                              DisasterRecoveryStep::REPLICA_CATCHUP});
    mgr.setVerificationHook(
        DisasterRecoveryStep::REPLICA_CATCHUP,
        [](const DisasterRecoveryPlan& plan, std::string& detail)
            -> std::optional<DisasterRecoveryVerificationProof> {
            detail = "replica is behind required WAL position";
            DisasterRecoveryVerificationProof proof;
            proof.term = plan.required_term;
            proof.lsn = plan.required_lsn - 1;
            return proof;
        });

    const auto result = mgr.executePlan(makeMinimalProductionPlan());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code,
              DisasterRecoveryErrorCode::STALE_REPLICATION_POSITION);
    EXPECT_EQ(result.step_results.back().step,
              DisasterRecoveryStep::REPLICA_CATCHUP);
}

TEST(DisasterRecoveryEdgeDRE21, TrafficShiftToWrongSiteFailsVerification) {
    TemporaryMetadataLog metadata;
    DisasterRecoveryManager mgr(makeProductionConfig(metadata.path), nullptr,
        std::make_shared<themis::sharding::EpochFencingManager>(
            themis::sharding::EpochFencingConfig{}));
    mgr.testSetQuorumOverride(true);
    installAdapterHooks(mgr, {DisasterRecoveryStep::SNAPSHOT_VALIDATION,
                              DisasterRecoveryStep::RESTORE,
                              DisasterRecoveryStep::REPLICA_CATCHUP,
                              DisasterRecoveryStep::TRAFFIC_SHIFT});
    mgr.setVerificationHook(
        DisasterRecoveryStep::TRAFFIC_SHIFT,
        [](const DisasterRecoveryPlan&, std::string&)
            -> std::optional<DisasterRecoveryVerificationProof> {
            DisasterRecoveryVerificationProof proof;
            proof.observed_site = "old-primary";
            return proof;
        });

    const auto result = mgr.executePlan(makeMinimalProductionPlan());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code, DisasterRecoveryErrorCode::VERIFICATION_FAILED);
    EXPECT_EQ(result.step_results.back().step,
              DisasterRecoveryStep::TRAFFIC_SHIFT);
}

TEST(DisasterRecoveryEdgeDRE17, MissingFencingManagerBlocksBeforeRestore) {
    TemporaryMetadataLog metadata;
    DisasterRecoveryManager mgr(makeProductionConfig(metadata.path), nullptr, nullptr);
    mgr.testSetQuorumOverride(true);
    installAdapterHooks(mgr, {DisasterRecoveryStep::SNAPSHOT_VALIDATION});

    const auto result = mgr.executePlan(makeMinimalProductionPlan());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code, DisasterRecoveryErrorCode::NOT_CONFIGURED);
    EXPECT_EQ(result.step_results.back().step, DisasterRecoveryStep::EPOCH_FENCING);
}

TEST(DisasterRecoveryEdgeDRE19, MetadataWriteFailureBlocksBeforeAdapters) {
    const auto metadata_directory = std::filesystem::temp_directory_path() /
        ("themis_dr_metadata_directory_" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ASSERT_TRUE(std::filesystem::create_directory(metadata_directory));
    auto config = makeProductionConfig(metadata_directory);
    DisasterRecoveryManager mgr(config, nullptr,
        std::make_shared<themis::sharding::EpochFencingManager>(
            themis::sharding::EpochFencingConfig{}));
    mgr.testSetQuorumOverride(true);

    const auto result = mgr.executePlan(makeMinimalProductionPlan());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code,
              DisasterRecoveryErrorCode::METADATA_PERSISTENCE_FAILED);
    ASSERT_EQ(result.step_results.size(), 1u);
    EXPECT_EQ(result.step_results.front().step, DisasterRecoveryStep::PRECHECKS);
    std::error_code error;
    std::filesystem::remove(metadata_directory, error);
    auto metadata_lock = metadata_directory;
    metadata_lock += ".lock";
    std::filesystem::remove(metadata_lock, error);
}

TEST(DisasterRecoveryEdgeDRE23, CorruptMetadataBlocksBeforeAdapters) {
    TemporaryMetadataLog metadata;
    {
        DisasterRecoveryManager writer(makeProductionConfig(metadata.path), nullptr,
            std::make_shared<themis::sharding::EpochFencingManager>(
                themis::sharding::EpochFencingConfig{}));
        writer.testSetQuorumOverride(true);
        installAdapterHooks(writer, {DisasterRecoveryStep::SNAPSHOT_VALIDATION,
                                     DisasterRecoveryStep::RESTORE,
                                     DisasterRecoveryStep::REPLICA_CATCHUP,
                                     DisasterRecoveryStep::TRAFFIC_SHIFT,
                                     DisasterRecoveryStep::VERIFICATION});
        ASSERT_TRUE(writer.executePlan(makeMinimalProductionPlan()).success);
    }
    {
        std::ofstream output(metadata.path, std::ios::out | std::ios::app);
        ASSERT_TRUE(output.is_open());
        output << "corrupt-completion-record\n";
        ASSERT_TRUE(output.good());
    }

    DisasterRecoveryManager manager(makeProductionConfig(metadata.path), nullptr,
        std::make_shared<themis::sharding::EpochFencingManager>(
            themis::sharding::EpochFencingConfig{}));
    manager.testSetQuorumOverride(true);

    const auto result = manager.executePlan(makeMinimalProductionPlan());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code,
              DisasterRecoveryErrorCode::METADATA_PERSISTENCE_FAILED);
    ASSERT_EQ(result.step_results.size(), 1u);
    EXPECT_EQ(result.step_results.front().step, DisasterRecoveryStep::PRECHECKS);
    EXPECT_NE(result.error_message.find("malformed"), std::string::npos);

    const auto repeated = manager.executePlan(makeMinimalProductionPlan());
    EXPECT_FALSE(repeated.success);
    EXPECT_EQ(repeated.error_code,
              DisasterRecoveryErrorCode::METADATA_PERSISTENCE_FAILED);
    EXPECT_EQ(manager.getStatistics().total_runs, 1u)
        << "the replay failure should be cached instead of re-running the plan";
}

TEST(DisasterRecoveryEdgeDRE18, CompletedMetadataSurvivesManagerRestart) {
    TemporaryMetadataLog metadata;
    const auto plan = makeMinimalProductionPlan();
    {
        DisasterRecoveryManager mgr(makeProductionConfig(metadata.path), nullptr,
            std::make_shared<themis::sharding::EpochFencingManager>(
                themis::sharding::EpochFencingConfig{}));
        mgr.testSetQuorumOverride(true);
        installAdapterHooks(mgr, {DisasterRecoveryStep::SNAPSHOT_VALIDATION,
                                  DisasterRecoveryStep::RESTORE,
                                  DisasterRecoveryStep::REPLICA_CATCHUP,
                                  DisasterRecoveryStep::TRAFFIC_SHIFT,
                                  DisasterRecoveryStep::VERIFICATION});

        const auto result = mgr.executePlan(plan);
        ASSERT_TRUE(result.success) << result.error_message;
        EXPECT_TRUE(std::filesystem::exists(metadata.path));
    }

    DisasterRecoveryManager restarted(makeProductionConfig(metadata.path), nullptr, nullptr);
    const auto recovered = restarted.executePlan(plan);
    EXPECT_TRUE(recovered.success);
    EXPECT_EQ(recovered.final_state, DisasterRecoveryState::COMPLETED);
    EXPECT_EQ(recovered.step_results.size(), 7u);
    EXPECT_FALSE(recovered.step_results.back().message.empty());

    auto conflicting_plan = plan;
    conflicting_plan.recovery_site = "different-site";
    const auto conflict = restarted.executePlan(conflicting_plan);
    EXPECT_FALSE(conflict.success);
    EXPECT_EQ(conflict.error_code, DisasterRecoveryErrorCode::INVALID_PLAN);
}

TEST(DisasterRecoveryEdgeDRE26, MissingPreviouslyLoadedMetadataBlocksCachedSuccess) {
    TemporaryMetadataLog metadata;
    DisasterRecoveryManager manager(makeProductionConfig(metadata.path), nullptr,
        std::make_shared<themis::sharding::EpochFencingManager>(
            themis::sharding::EpochFencingConfig{}));
    manager.testSetQuorumOverride(true);
    installAdapterHooks(manager, {DisasterRecoveryStep::SNAPSHOT_VALIDATION,
                                  DisasterRecoveryStep::RESTORE,
                                  DisasterRecoveryStep::REPLICA_CATCHUP,
                                  DisasterRecoveryStep::TRAFFIC_SHIFT,
                                  DisasterRecoveryStep::VERIFICATION});

    const auto plan = makeMinimalProductionPlan();
    ASSERT_TRUE(manager.executePlan(plan).success);
    std::error_code error;
    ASSERT_TRUE(std::filesystem::remove(metadata.path, error));
    ASSERT_FALSE(error);

    const auto result = manager.executePlan(plan);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code, DisasterRecoveryErrorCode::METADATA_PERSISTENCE_FAILED);
    ASSERT_EQ(result.step_results.size(), 1u);
    EXPECT_EQ(result.step_results.front().step, DisasterRecoveryStep::PRECHECKS);
}

#if defined(__unix__) || defined(__APPLE__)
TEST(DisasterRecoveryEdgeDRE22, CompletedMetadataReplaysAcrossProcesses) {
    TemporaryMetadataLog metadata;
    const auto plan = makeMinimalProductionPlan();

    const pid_t child = ::fork();
    ASSERT_NE(child, -1) << "fork failed";
    if (child == 0) {
        DisasterRecoveryManager manager(makeProductionConfig(metadata.path), nullptr,
            std::make_shared<themis::sharding::EpochFencingManager>(
                themis::sharding::EpochFencingConfig{}));
        manager.testSetQuorumOverride(true);
        installAdapterHooks(manager, {DisasterRecoveryStep::SNAPSHOT_VALIDATION,
                                      DisasterRecoveryStep::RESTORE,
                                      DisasterRecoveryStep::REPLICA_CATCHUP,
                                      DisasterRecoveryStep::TRAFFIC_SHIFT,
                                      DisasterRecoveryStep::VERIFICATION});
        const auto result = manager.executePlan(plan);
        ::_exit(result.success ? 0 : 1);
    }

    int status = 0;
    ASSERT_EQ(::waitpid(child, &status, 0), child);
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 0)
        << "child process must persist a completed DR plan before exiting";

    DisasterRecoveryManager restarted(makeProductionConfig(metadata.path), nullptr, nullptr);
    const auto recovered = restarted.executePlan(plan);
    EXPECT_TRUE(recovered.success);
    EXPECT_EQ(recovered.final_state, DisasterRecoveryState::COMPLETED);
    EXPECT_EQ(recovered.step_results.size(), 7u);
}

TEST(DisasterRecoveryEdgeDRE24, CompetingProcessesExecutePlanOnce) {
    TemporaryMetadataLog metadata;
    auto operation_log = metadata.path;
    operation_log += ".operations";
    const auto plan = makeMinimalProductionPlan();
    int start_pipe[2] = {-1, -1};
    ASSERT_EQ(::pipe(start_pipe), 0);

    std::vector<pid_t> children;
    for (int child_index = 0; child_index < 2; ++child_index) {
        const pid_t child = ::fork();
        ASSERT_NE(child, -1);
        if (child == 0) {
            ::close(start_pipe[1]);
            char start_signal = 0;
            const auto bytes_read = ::read(start_pipe[0], &start_signal, 1);
            ::close(start_pipe[0]);
            if (bytes_read != 1) {
                ::_exit(2);
            }

            DisasterRecoveryManager manager(makeProductionConfig(metadata.path), nullptr,
                std::make_shared<themis::sharding::EpochFencingManager>(
                    themis::sharding::EpochFencingConfig{}));
            manager.testSetQuorumOverride(true);
            installCountingAdapterHooks(manager, operation_log);
            const auto result = manager.executePlan(plan);
            ::_exit(result.success ? 0 : 1);
        }
        children.push_back(child);
    }

    ::close(start_pipe[0]);
    const char start_signals[2] = {'x', 'x'};
    ASSERT_EQ(::write(start_pipe[1], start_signals, sizeof(start_signals)),
              static_cast<ssize_t>(sizeof(start_signals)));
    ::close(start_pipe[1]);

    for (const auto child : children) {
        int status = 0;
        ASSERT_EQ(::waitpid(child, &status, 0), child);
        ASSERT_TRUE(WIFEXITED(status));
        EXPECT_EQ(WEXITSTATUS(status), 0)
            << "both processes should observe a successful idempotent result";
    }

    std::ifstream operations(operation_log);
    ASSERT_TRUE(operations.is_open());
    std::size_t operation_count = 0;
    std::string operation;
    while (std::getline(operations, operation)) {
        ++operation_count;
    }
    EXPECT_EQ(operation_count, 5u)
        << "only the process holding the metadata lock may perform DR operations";

    std::ifstream completion_records(metadata.path);
    ASSERT_TRUE(completion_records.is_open());
    std::size_t completion_count = 0;
    std::string completion;
    while (std::getline(completion_records, completion)) {
        ++completion_count;
    }
    EXPECT_EQ(completion_count, 1u);
    std::error_code error;
    std::filesystem::remove(operation_log, error);
}

TEST(DisasterRecoveryEdgeDRE25, MetadataLockTimeoutFailsClosed) {
    TemporaryMetadataLog metadata;
    auto config = makeProductionConfig(metadata.path);
    config.metadata_lock_timeout = std::chrono::milliseconds{25};

    auto lock_path = metadata.path;
    lock_path += ".lock";
    const int lock_fd = ::open(lock_path.c_str(), O_CREAT | O_RDWR, 0600);
    ASSERT_GE(lock_fd, 0);
    ASSERT_EQ(::flock(lock_fd, LOCK_EX | LOCK_NB), 0);

    DisasterRecoveryManager manager(config, nullptr, nullptr);
    const auto result = manager.executePlan(makeMinimalProductionPlan());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code, DisasterRecoveryErrorCode::METADATA_PERSISTENCE_FAILED);
    EXPECT_NE(result.error_message.find("timed out"), std::string::npos);
    EXPECT_TRUE(result.step_results.empty())
        << "the manager must not begin recovery steps without the metadata lock";
    ::flock(lock_fd, LOCK_UN);
    ::close(lock_fd);
}
#endif
