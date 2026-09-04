#include <gtest/gtest.h>

#include <memory>

#include "failover/disaster_recovery_manager.h"
#include "sharding/epoch_fencing.h"

using namespace themis::failover;

namespace {
DisasterRecoveryPlan makeBasePlan(bool dry_run = true) {
    DisasterRecoveryPlan plan;
    plan.plan_id = "dr-plan-1";
    plan.primary_site = "dc-a";
    plan.recovery_site = "dc-b";
    plan.snapshot_id = dry_run ? "" : "snapshot-42";
    plan.dry_run = dry_run;
    plan.shift_traffic = true;
    plan.critical_nodes = {"n1", "n2"};
    return plan;
}
}  // namespace

TEST(DisasterRecoveryManagerTest, ValidatePlanRejectsMissingPlanId) {
    DisasterRecoveryConfig cfg;
    DisasterRecoveryManager mgr{cfg, nullptr, nullptr};

    auto plan = makeBasePlan();
    plan.plan_id.clear();

    std::string error = {};
    EXPECT_FALSE(mgr.validatePlan(plan, error));
    EXPECT_FALSE(error.empty());
}

TEST(DisasterRecoveryManagerTest, DryRunSucceedsWithoutExternalManagers) {
    DisasterRecoveryConfig cfg;
    cfg.require_quorum = true;
    cfg.enforce_epoch_fencing = true;

    DisasterRecoveryManager mgr{cfg, nullptr, nullptr};
    auto result = mgr.executePlan(makeBasePlan(true));

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.final_state, DisasterRecoveryState::COMPLETED);
    EXPECT_GE(result.step_results.size(), 7u);
}

TEST(DisasterRecoveryManagerTest, NonDryRunFailsWithoutSnapshotId) {
    DisasterRecoveryConfig cfg;
    cfg.require_quorum = false;
    cfg.enforce_epoch_fencing = false;

    DisasterRecoveryManager mgr{cfg, nullptr, nullptr};
    auto plan = makeBasePlan(false);
    plan.snapshot_id.clear();

    auto result = mgr.executePlan(plan);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.final_state, DisasterRecoveryState::FAILED);
    EXPECT_NE(result.error_message.find("snapshot_id"), std::string::npos);
}

TEST(DisasterRecoveryManagerTest, NonDryRunFailsWhenFencingRequiredButMissing) {
    DisasterRecoveryConfig cfg;
    cfg.require_quorum = false;
    cfg.enforce_epoch_fencing = true;

    DisasterRecoveryManager mgr{cfg, nullptr, nullptr};
    auto plan = makeBasePlan(false);

    auto result = mgr.executePlan(plan);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.final_state, DisasterRecoveryState::FAILED);
    EXPECT_NE(result.error_message.find("fencing"), std::string::npos);
}

TEST(DisasterRecoveryManagerTest, StepHookCanForceFailureInRestore) {
    DisasterRecoveryConfig cfg;
    cfg.require_quorum = false;
    cfg.enforce_epoch_fencing = false;

    DisasterRecoveryManager mgr{cfg, nullptr, nullptr};
    mgr.setStepHook(DisasterRecoveryStep::RESTORE,
                    [](const DisasterRecoveryPlan&, std::string& detail) {
                        detail = "restore script failed";
                        return false;
                    });

    auto result = mgr.executePlan(makeBasePlan(false));
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.final_state, DisasterRecoveryState::FAILED);
    EXPECT_NE(result.error_message.find("restore script failed"), std::string::npos);
}

TEST(DisasterRecoveryManagerTest, HookedRecoveryCanCompleteAndUpdateStats) {
    DisasterRecoveryConfig cfg;
    cfg.require_quorum = false;
    cfg.enforce_epoch_fencing = true;

    themis::sharding::EpochFencingConfig fence_cfg;
    fence_cfg.shard_id = "dr-shard";
    fence_cfg.node_id = "dr-node";
    auto fencing = std::make_shared<themis::sharding::EpochFencingManager>(fence_cfg);

    DisasterRecoveryManager mgr{cfg, nullptr, fencing};

    mgr.setStepHook(DisasterRecoveryStep::PRECHECKS,
                    [](const DisasterRecoveryPlan&, std::string& detail) {
                        detail = "synthetic prechecks pass";
                        return true;
                    });
    mgr.setStepHook(DisasterRecoveryStep::REPLICA_CATCHUP,
                    [](const DisasterRecoveryPlan&, std::string& detail) {
                        detail = "synthetic catchup pass";
                        return true;
                    });
    mgr.setStepHook(DisasterRecoveryStep::VERIFICATION,
                    [](const DisasterRecoveryPlan&, std::string& detail) {
                        detail = "synthetic verification pass";
                        return true;
                    });

    auto result = mgr.executePlan(makeBasePlan(false));
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.final_state, DisasterRecoveryState::COMPLETED);
    EXPECT_GT(result.fenced_epoch, 0u);

    const auto stats = mgr.getStatistics();
    EXPECT_EQ(stats.total_runs, 1u);
    EXPECT_EQ(stats.successful_runs, 1u);
    EXPECT_EQ(stats.failed_runs, 0u);
}

TEST(DisasterRecoveryManagerTest, StateTransitionsToCompletedOnSuccess) {
    DisasterRecoveryConfig cfg;
    DisasterRecoveryManager mgr{cfg, nullptr, nullptr};

    auto result = mgr.executePlan(makeBasePlan(true));
    EXPECT_TRUE(result.success);
    EXPECT_EQ(mgr.getState(), DisasterRecoveryState::COMPLETED);
}

TEST(DisasterRecoveryManagerTest, StateTransitionsToFailedOnError) {
    DisasterRecoveryConfig cfg;
    cfg.require_quorum = false;
    cfg.enforce_epoch_fencing = false;
    DisasterRecoveryManager mgr{cfg, nullptr, nullptr};

    auto plan = makeBasePlan(false);
    plan.snapshot_id.clear();

    auto result = mgr.executePlan(plan);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(mgr.getState(), DisasterRecoveryState::FAILED);
}
