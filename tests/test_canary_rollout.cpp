// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_canary_rollout.cpp
 * @brief Tests for the CanaryRollout module (Phase 2 – Canary rollout mode)
 *
 * All tests use only the public API of CanaryRollout and CanaryConfig so that
 * they compile without a running database or network connection.  The
 * HotReloadEngine dependency is satisfied by a lightweight mock built on top
 * of the existing ManifestDatabase / UpdateChecker stubs.
 */

#include <gtest/gtest.h>

#include "updates/canary_rollout.h"
#include "updates/updates_config.h"

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <stdexcept>

using namespace themis::updates;

// ---------------------------------------------------------------------------
// Minimal stub of HotReloadEngine
//
// We inherit from HotReloadEngine and override the two virtual-like methods
// that CanaryRollout calls: applyHotReload() and rollback().
// Because HotReloadEngine is not polymorphic we use a wrapper instead.
// ---------------------------------------------------------------------------

/**
 * Stub engine whose applyHotReload() and rollback() behaviour is injectable.
 *
 * It is wrapped in a subclass using the existing constructor signature so
 * that CanaryRollout receives a shared_ptr<HotReloadEngine>.
 */
class StubHotReloadEngine : public HotReloadEngine {
public:
    explicit StubHotReloadEngine(bool apply_succeeds = true,
                                 bool rollback_succeeds = true)
        : HotReloadEngine(nullptr, nullptr,
                          []() {
                              HotReloadEngine::Config c;
                              c.download_directory = "/tmp/stub_dl";
                              c.backup_directory = "/tmp/stub_bak";
                              c.verify_signatures = false;
                              c.create_backup = false;
                              return c;
                          }())
        , apply_succeeds_(apply_succeeds)
        , rollback_succeeds_(rollback_succeeds) {}

    ReloadResult applyHotReload(const std::string& version,
                                bool /*verify_only*/ = false) override {
        ++apply_call_count_;
        ReloadResult r;
        r.success = apply_succeeds_;
        r.rollback_id = "stub_rollback_" + version;
        if (!apply_succeeds_) {
            r.error_message = "stub: apply failed";
        }
        return r;
    }

    bool rollback(const std::string& /*rollback_id*/) override {
        ++rollback_call_count_;
        return rollback_succeeds_;
    }

    int apply_call_count_ = 0;
    int rollback_call_count_ = 0;

private:
    bool apply_succeeds_;
    bool rollback_succeeds_;
};

// ---------------------------------------------------------------------------
// Helper: build a CanaryConfig that makes the node always included (hash ≤ 1.0)
// ---------------------------------------------------------------------------
static CanaryConfig makeConfig(const std::string& version = "1.5.0",
                               const std::string& node_id = "test-node-001") {
    return CanaryConfig::withDefaultStages(version, node_id);
}

// ---------------------------------------------------------------------------
// CanaryConfig defaults
// ---------------------------------------------------------------------------

class CanaryConfigTest : public ::testing::Test {};

TEST_F(CanaryConfigTest, WithDefaultStages_HasFourStages) {
    auto cfg = CanaryConfig::withDefaultStages("1.0.0", "node-a");
    EXPECT_EQ(cfg.stages.size(), 4u);
}

TEST_F(CanaryConfigTest, WithDefaultStages_FirstStageIsOnePercent) {
    auto cfg = CanaryConfig::withDefaultStages("1.0.0", "node-a");
    EXPECT_DOUBLE_EQ(cfg.stages[0].percentage, 0.01);
}

TEST_F(CanaryConfigTest, WithDefaultStages_LastStageIsFullRollout) {
    auto cfg = CanaryConfig::withDefaultStages("1.0.0", "node-a");
    EXPECT_DOUBLE_EQ(cfg.stages.back().percentage, 1.0);
}

TEST_F(CanaryConfigTest, WithDefaultStages_StagesAreMonotonicallyIncreasing) {
    auto cfg = CanaryConfig::withDefaultStages("1.0.0", "node-a");
    for (size_t i = 1; i < cfg.stages.size(); ++i) {
        EXPECT_GE(cfg.stages[i].percentage, cfg.stages[i - 1].percentage);
    }
}

TEST_F(CanaryConfigTest, WithDefaultStages_SetsVersionAndNodeId) {
    auto cfg = CanaryConfig::withDefaultStages("2.0.0", "my-node");
    EXPECT_EQ(cfg.version, "2.0.0");
    EXPECT_EQ(cfg.node_id, "my-node");
}

TEST_F(CanaryConfigTest, DefaultErrorRateThresholdIsFivePercent) {
    auto cfg = CanaryConfig::withDefaultStages("1.0.0", "node-a");
    EXPECT_DOUBLE_EQ(cfg.error_rate_threshold, 0.05);
}

// ---------------------------------------------------------------------------
// CanaryRollout construction and validation
// ---------------------------------------------------------------------------

class CanaryRolloutConstructionTest : public ::testing::Test {
protected:
    std::shared_ptr<StubHotReloadEngine> engine_ =
        std::make_shared<StubHotReloadEngine>();
};

TEST_F(CanaryRolloutConstructionTest, ValidConfig_DoesNotThrow) {
    auto cfg = makeConfig();
    EXPECT_NO_THROW(CanaryRollout rollout(engine_, cfg));
}

TEST_F(CanaryRolloutConstructionTest, NullEngine_Throws) {
    auto cfg = makeConfig();
    EXPECT_THROW(CanaryRollout(nullptr, cfg), std::invalid_argument);
}

TEST_F(CanaryRolloutConstructionTest, EmptyStages_Throws) {
    auto cfg = makeConfig();
    cfg.stages.clear();
    EXPECT_THROW(CanaryRollout(engine_, cfg), std::invalid_argument);
}

TEST_F(CanaryRolloutConstructionTest, EmptyNodeId_Throws) {
    auto cfg = makeConfig();
    cfg.node_id.clear();
    EXPECT_THROW(CanaryRollout(engine_, cfg), std::invalid_argument);
}

TEST_F(CanaryRolloutConstructionTest, EmptyVersion_Throws) {
    auto cfg = makeConfig();
    cfg.version.clear();
    EXPECT_THROW(CanaryRollout(engine_, cfg), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// Node membership – deterministic hash
// ---------------------------------------------------------------------------

class CanaryNodeMembershipTest : public ::testing::Test {
protected:
    std::shared_ptr<StubHotReloadEngine> engine_ =
        std::make_shared<StubHotReloadEngine>();
};

TEST_F(CanaryNodeMembershipTest, LastStageFull_AlwaysIncludesAllNodes) {
    // The last stage has percentage == 1.0; every node must be included.
    auto cfg = makeConfig("1.0.0", "any-node");
    CanaryRollout rollout(engine_, cfg);
    EXPECT_TRUE(rollout.isNodeInStage(cfg.stages.size() - 1));
}

TEST_F(CanaryNodeMembershipTest, HashIsDeterministic_SameResultOnTwoCalls) {
    auto cfg = makeConfig();
    CanaryRollout r1(engine_, cfg);
    CanaryRollout r2(engine_, cfg);
    EXPECT_EQ(r1.isNodeInStage(0), r2.isNodeInStage(0));
    EXPECT_EQ(r1.isNodeInStage(1), r2.isNodeInStage(1));
}

TEST_F(CanaryNodeMembershipTest, DifferentNodeIds_MayDifferAtLowPercentage) {
    // With 1% rollout, at least some nodes should be excluded and some included
    // across a large enough set.  We simply verify that different IDs can give
    // different answers (not all identical).
    std::vector<std::string> ids;
    for (int i = 0; i < 200; ++i) {
        ids.push_back("node-" + std::to_string(i));
    }
    int included = 0;
    for (const auto& id : ids) {
        auto cfg = CanaryConfig::withDefaultStages("1.0.0", id);
        CanaryRollout rollout(engine_, cfg);
        if (rollout.isNodeInStage(0)) {  // stage 0 = 1%
            ++included;
        }
    }
    // With 200 nodes and 1% threshold we expect very few inclusions (0-5 is
    // acceptable given hash distribution), but definitely not all 200.
    EXPECT_LT(included, 20) << "Too many nodes included in 1% stage";
}

TEST_F(CanaryNodeMembershipTest, StageOutOfRange_ReturnsFalse) {
    auto cfg = makeConfig();
    CanaryRollout rollout(engine_, cfg);
    EXPECT_FALSE(rollout.isNodeInStage(999));
}

// ---------------------------------------------------------------------------
// Stage advancement
// ---------------------------------------------------------------------------

class CanaryStageAdvanceTest : public ::testing::Test {
protected:
    std::shared_ptr<StubHotReloadEngine> engine_ =
        std::make_shared<StubHotReloadEngine>();
};

TEST_F(CanaryStageAdvanceTest, InitialStage_IsZero) {
    auto cfg = makeConfig();
    CanaryRollout rollout(engine_, cfg);
    EXPECT_EQ(rollout.currentStage(), 0u);
}

TEST_F(CanaryStageAdvanceTest, AdvanceStage_IncrementsIndex) {
    auto cfg = makeConfig();
    CanaryRollout rollout(engine_, cfg);
    EXPECT_TRUE(rollout.advanceStage());
    EXPECT_EQ(rollout.currentStage(), 1u);
}

TEST_F(CanaryStageAdvanceTest, AdvanceAllStages_ReturnsCompleteStatus) {
    auto cfg = makeConfig();
    CanaryRollout rollout(engine_, cfg);
    for (size_t i = 0; i < cfg.stages.size() - 1; ++i) {
        EXPECT_TRUE(rollout.advanceStage());
    }
    // Now at last stage; one more advance should return false and mark complete
    EXPECT_FALSE(rollout.advanceStage());
    EXPECT_TRUE(rollout.status().is_complete);
}

TEST_F(CanaryStageAdvanceTest, AdvanceAfterComplete_ReturnsFalse) {
    auto cfg = makeConfig();
    CanaryRollout rollout(engine_, cfg);
    for (size_t i = 0; i < cfg.stages.size(); ++i) {
        rollout.advanceStage();
    }
    EXPECT_FALSE(rollout.advanceStage());
}

TEST_F(CanaryStageAdvanceTest, StageCompleteCallback_IsCalled) {
    auto cfg = makeConfig();
    CanaryRollout rollout(engine_, cfg);
    int call_count = 0;
    rollout.setStageCompleteCallback(
        [&](size_t /*stage*/, double /*pct*/) { ++call_count; });
    rollout.advanceStage();
    EXPECT_EQ(call_count, 1);
}

TEST_F(CanaryStageAdvanceTest, StageCompleteCallback_ReceivesCorrectStage) {
    auto cfg = makeConfig();
    CanaryRollout rollout(engine_, cfg);
    size_t reported_stage = 999;
    rollout.setStageCompleteCallback(
        [&](size_t stage, double /*pct*/) { reported_stage = stage; });
    rollout.advanceStage();
    EXPECT_EQ(reported_stage, 0u);
}

TEST_F(CanaryStageAdvanceTest, AdvanceResetsErrorCounters) {
    auto cfg = makeConfig();
    CanaryRollout rollout(engine_, cfg);

    // Report some events in stage 0
    for (int i = 0; i < 5; ++i) rollout.reportSuccess();
    for (int i = 0; i < 5; ++i) rollout.reportError();
    EXPECT_GT(rollout.status().sample_count, 0u);

    rollout.advanceStage();
    EXPECT_EQ(rollout.status().sample_count, 0u);
    EXPECT_DOUBLE_EQ(rollout.errorRate(), 0.0);
}

// ---------------------------------------------------------------------------
// applyIfIncluded
// ---------------------------------------------------------------------------

class CanaryApplyTest : public ::testing::Test {};

TEST_F(CanaryApplyTest, NodeInLastStage_AppliesSuccessfully) {
    // Build config where the only stage covers 100% of nodes
    CanaryConfig cfg;
    cfg.version = "1.5.0";
    cfg.node_id = "node-full";
    cfg.stages = {{1.0, std::chrono::seconds{0}}};

    auto engine = std::make_shared<StubHotReloadEngine>(/*apply_succeeds=*/true);
    CanaryRollout rollout(engine, cfg);

    auto result = rollout.applyIfIncluded();
    EXPECT_TRUE(result.success);
    EXPECT_EQ(engine->apply_call_count_, 1);
}

TEST_F(CanaryApplyTest, NodeNotInStage_SkipsWithoutCallingEngine) {
    // Build a config with 0% stage so no node is ever included
    CanaryConfig cfg;
    cfg.version = "1.5.0";
    cfg.node_id = "node-excluded";
    cfg.stages = {{0.0, std::chrono::seconds{0}}};  // 0% → no node included

    auto engine = std::make_shared<StubHotReloadEngine>();
    CanaryRollout rollout(engine, cfg);

    auto result = rollout.applyIfIncluded();
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_EQ(engine->apply_call_count_, 0);
}

TEST_F(CanaryApplyTest, EngineFailure_PropagatesResult) {
    CanaryConfig cfg;
    cfg.version = "1.5.0";
    cfg.node_id = "node-full";
    cfg.stages = {{1.0, std::chrono::seconds{0}}};

    auto engine = std::make_shared<StubHotReloadEngine>(/*apply_succeeds=*/false);
    CanaryRollout rollout(engine, cfg);

    auto result = rollout.applyIfIncluded();
    EXPECT_FALSE(result.success);
}

TEST_F(CanaryApplyTest, RolledBack_ApplyReturnsFalse) {
    CanaryConfig cfg;
    cfg.version = "1.5.0";
    cfg.node_id = "node-full";
    cfg.stages = {{1.0, std::chrono::seconds{0}}};

    auto engine = std::make_shared<StubHotReloadEngine>();
    CanaryRollout rollout(engine, cfg);
    rollout.rollback("test");

    auto result = rollout.applyIfIncluded();
    EXPECT_FALSE(result.success);
    EXPECT_EQ(engine->apply_call_count_, 0);
}

// ---------------------------------------------------------------------------
// Rollback
// ---------------------------------------------------------------------------

class CanaryRollbackTest : public ::testing::Test {};

TEST_F(CanaryRollbackTest, Rollback_CallsEngineRollback) {
    CanaryConfig cfg;
    cfg.version = "1.5.0";
    cfg.node_id = "node-full";
    cfg.stages = {{1.0, std::chrono::seconds{0}}};

    auto engine = std::make_shared<StubHotReloadEngine>(true, /*rollback_succeeds=*/true);
    CanaryRollout rollout(engine, cfg);
    rollout.applyIfIncluded();  // sets rollback_id

    EXPECT_TRUE(rollout.rollback("test reason"));
    EXPECT_EQ(engine->rollback_call_count_, 1);
}

TEST_F(CanaryRollbackTest, Rollback_SetsRolledBackFlag) {
    CanaryConfig cfg;
    cfg.version = "1.5.0";
    cfg.node_id = "node-full";
    cfg.stages = {{1.0, std::chrono::seconds{0}}};

    auto engine = std::make_shared<StubHotReloadEngine>();
    CanaryRollout rollout(engine, cfg);
    rollout.rollback("intentional");

    EXPECT_TRUE(rollout.status().is_rolled_back);
}

TEST_F(CanaryRollbackTest, Rollback_SetsReason) {
    CanaryConfig cfg;
    cfg.version = "1.5.0";
    cfg.node_id = "node-full";
    cfg.stages = {{1.0, std::chrono::seconds{0}}};

    auto engine = std::make_shared<StubHotReloadEngine>();
    CanaryRollout rollout(engine, cfg);
    rollout.rollback("disk full");

    EXPECT_EQ(rollout.status().rollback_reason, "disk full");
}

TEST_F(CanaryRollbackTest, DoubleRollback_ReturnsFalse) {
    CanaryConfig cfg;
    cfg.version = "1.5.0";
    cfg.node_id = "node-full";
    cfg.stages = {{1.0, std::chrono::seconds{0}}};

    auto engine = std::make_shared<StubHotReloadEngine>();
    CanaryRollout rollout(engine, cfg);
    rollout.rollback("first");
    EXPECT_FALSE(rollout.rollback("second"));
}

TEST_F(CanaryRollbackTest, RollbackCallback_IsCalled) {
    CanaryConfig cfg;
    cfg.version = "1.5.0";
    cfg.node_id = "node-full";
    cfg.stages = {{1.0, std::chrono::seconds{0}}};

    auto engine = std::make_shared<StubHotReloadEngine>();
    CanaryRollout rollout(engine, cfg);
    std::string received_reason;
    rollout.setRollbackCallback(
        [&](const std::string& r) { received_reason = r; });

    rollout.rollback("network error");
    EXPECT_EQ(received_reason, "network error");
}

// ---------------------------------------------------------------------------
// Health tracking – error rate and auto-rollback
// ---------------------------------------------------------------------------

class CanaryHealthTest : public ::testing::Test {
protected:
    std::shared_ptr<StubHotReloadEngine> engine_ =
        std::make_shared<StubHotReloadEngine>();
};

TEST_F(CanaryHealthTest, InitialErrorRate_IsZero) {
    auto cfg = makeConfig();
    CanaryRollout rollout(engine_, cfg);
    EXPECT_DOUBLE_EQ(rollout.errorRate(), 0.0);
}

TEST_F(CanaryHealthTest, AllSuccess_ErrorRateIsZero) {
    auto cfg = makeConfig();
    CanaryRollout rollout(engine_, cfg);
    for (int i = 0; i < 10; ++i) rollout.reportSuccess();
    EXPECT_DOUBLE_EQ(rollout.errorRate(), 0.0);
}

TEST_F(CanaryHealthTest, AllErrors_ErrorRateIsOne) {
    auto cfg = makeConfig();
    CanaryRollout rollout(engine_, cfg);
    for (int i = 0; i < 10; ++i) rollout.reportError();
    // Auto-rollback triggers after min_sample_count, but errorRate is still 1
    EXPECT_NEAR(rollout.errorRate(), 1.0, 0.001);
}

TEST_F(CanaryHealthTest, MixedEvents_ErrorRateIsCorrect) {
    auto cfg = makeConfig();
    cfg.min_sample_count = 100;  // Prevent auto-rollback during this test
    CanaryRollout rollout(engine_, cfg);
    for (int i = 0; i < 90; ++i) rollout.reportSuccess();
    for (int i = 0; i < 10; ++i) rollout.reportError();
    EXPECT_NEAR(rollout.errorRate(), 0.1, 0.001);
}

TEST_F(CanaryHealthTest, BelowMinSampleCount_ShouldRollbackIsFalse) {
    auto cfg = makeConfig();
    cfg.min_sample_count = 100;
    cfg.error_rate_threshold = 0.05;
    CanaryRollout rollout(engine_, cfg);
    // 10 errors out of 10 = 100% error rate but sample < min
    for (int i = 0; i < 10; ++i) rollout.reportError();
    EXPECT_FALSE(rollout.shouldRollback());
}

TEST_F(CanaryHealthTest, AboveThreshold_ShouldRollbackIsTrue) {
    auto cfg = makeConfig();
    cfg.min_sample_count = 10;
    cfg.error_rate_threshold = 0.05;
    CanaryRollout rollout(engine_, cfg);
    // 10 successes + 1 error = ~9.09% error rate
    for (int i = 0; i < 10; ++i) rollout.reportSuccess();
    rollout.reportError();
    EXPECT_TRUE(rollout.shouldRollback());
}

TEST_F(CanaryHealthTest, AutoRollback_TriggeredWhenThresholdExceeded) {
    CanaryConfig cfg;
    cfg.version = "1.5.0";
    cfg.node_id = "node-full";
    cfg.stages = {{1.0, std::chrono::seconds{0}}};
    cfg.error_rate_threshold = 0.05;
    cfg.min_sample_count = 10;

    bool rollback_fired = false;
    CanaryRollout rollout(engine_, cfg);
    rollout.setRollbackCallback(
        [&](const std::string&) { rollback_fired = true; });

    for (int i = 0; i < 10; ++i) rollout.reportSuccess();
    for (int i = 0; i < 2; ++i) rollout.reportError();  // 16% > 5%

    EXPECT_TRUE(rollback_fired);
    EXPECT_TRUE(rollout.status().is_rolled_back);
}

TEST_F(CanaryHealthTest, AutoRollback_NotTriggeredBelowThreshold) {
    CanaryConfig cfg;
    cfg.version = "1.5.0";
    cfg.node_id = "node-full";
    cfg.stages = {{1.0, std::chrono::seconds{0}}};
    cfg.error_rate_threshold = 0.10;
    cfg.min_sample_count = 10;

    bool rollback_fired = false;
    CanaryRollout rollout(engine_, cfg);
    rollout.setRollbackCallback(
        [&](const std::string&) { rollback_fired = true; });

    // 95 successes + 5 errors = 5% error rate (< 10% threshold)
    for (int i = 0; i < 95; ++i) rollout.reportSuccess();
    for (int i = 0; i < 5; ++i) rollout.reportError();

    EXPECT_FALSE(rollback_fired);
    EXPECT_FALSE(rollout.status().is_rolled_back);
}

// ---------------------------------------------------------------------------
// Status snapshot
// ---------------------------------------------------------------------------

class CanaryStatusTest : public ::testing::Test {
protected:
    std::shared_ptr<StubHotReloadEngine> engine_ =
        std::make_shared<StubHotReloadEngine>();
};

TEST_F(CanaryStatusTest, InitialStatus_CorrectDefaults) {
    auto cfg = makeConfig("1.5.0", "node-x");
    CanaryRollout rollout(engine_, cfg);
    auto s = rollout.status();

    EXPECT_EQ(s.current_stage, 0u);
    EXPECT_EQ(s.total_stages, cfg.stages.size());
    EXPECT_FALSE(s.is_complete);
    EXPECT_FALSE(s.is_rolled_back);
    EXPECT_EQ(s.version, "1.5.0");
    EXPECT_TRUE(s.rollback_reason.empty());
    EXPECT_DOUBLE_EQ(s.observed_error_rate, 0.0);
    EXPECT_EQ(s.sample_count, 0u);
}

TEST_F(CanaryStatusTest, RollbackId_SetAfterSuccessfulApply) {
    CanaryConfig cfg;
    cfg.version = "1.5.0";
    cfg.node_id = "node-full";
    cfg.stages = {{1.0, std::chrono::seconds{0}}};

    CanaryRollout rollout(engine_, cfg);
    rollout.applyIfIncluded();

    EXPECT_FALSE(rollout.status().rollback_id.empty());
}

// ---------------------------------------------------------------------------
// UpdatesConfig canary section round-trip
// ---------------------------------------------------------------------------

class UpdatesConfigCanaryTest : public ::testing::Test {};

TEST_F(UpdatesConfigCanaryTest, DefaultCanaryConfig_IsDisabled) {
    UpdatesConfig cfg;
    EXPECT_FALSE(cfg.canary.enabled);
}

TEST_F(UpdatesConfigCanaryTest, DefaultCanaryConfig_HasFourStages) {
    UpdatesConfig cfg;
    EXPECT_EQ(cfg.canary.stages.size(), 4u);
}

TEST_F(UpdatesConfigCanaryTest, ToJson_HasCanarySection) {
    UpdatesConfig cfg;
    cfg.canary.enabled = true;
    cfg.canary.node_id = "my-node";
    auto j = cfg.toJson();
    EXPECT_TRUE(j.contains("canary"));
    EXPECT_TRUE(j["canary"]["enabled"].get<bool>());
    EXPECT_EQ(j["canary"]["node_id"].get<std::string>(), "my-node");
}

TEST_F(UpdatesConfigCanaryTest, FromJson_RoundTrip) {
    UpdatesConfig original;
    original.canary.enabled = true;
    original.canary.node_id = "round-trip-node";
    original.canary.error_rate_threshold = 0.03;
    original.canary.min_sample_count = 50;
    original.canary.stages = {{0.02, 1800}, {0.10, 3600}, {1.0, 0}};

    auto j = original.toJson();
    auto parsed = UpdatesConfig::fromJson(j);

    EXPECT_TRUE(parsed.canary.enabled);
    EXPECT_EQ(parsed.canary.node_id, "round-trip-node");
    EXPECT_DOUBLE_EQ(parsed.canary.error_rate_threshold, 0.03);
    EXPECT_EQ(parsed.canary.min_sample_count, 50u);
    ASSERT_EQ(parsed.canary.stages.size(), 3u);
    EXPECT_DOUBLE_EQ(parsed.canary.stages[0].percentage, 0.02);
    EXPECT_EQ(parsed.canary.stages[0].observation_seconds, 1800);
    EXPECT_DOUBLE_EQ(parsed.canary.stages[2].percentage, 1.0);
}

// ---------------------------------------------------------------------------
// Regression: throwing callbacks do not propagate exceptions
// ---------------------------------------------------------------------------

TEST(CanaryRegressionTest, ThrowingStageCallback_DoesNotPropagateException) {
    auto engine = std::make_shared<StubHotReloadEngine>();
    auto cfg = makeConfig();
    CanaryRollout rollout(engine, cfg);
    rollout.setStageCompleteCallback([](size_t, double) {
        throw std::runtime_error("callback boom");
    });
    EXPECT_NO_THROW(rollout.advanceStage());
}

TEST(CanaryRegressionTest, ThrowingRollbackCallback_DoesNotPropagateException) {
    auto engine = std::make_shared<StubHotReloadEngine>();
    CanaryConfig cfg;
    cfg.version = "1.0.0";
    cfg.node_id = "test";
    cfg.stages = {{1.0, std::chrono::seconds{0}}};
    CanaryRollout rollout(engine, cfg);
    rollout.setRollbackCallback([](const std::string&) {
        throw std::runtime_error("rollback callback boom");
    });
    EXPECT_NO_THROW(rollout.rollback("test"));
}

// ---------------------------------------------------------------------------
// Fix 1: Double-apply guard
// ---------------------------------------------------------------------------

class CanaryDoubleApplyTest : public ::testing::Test {};

TEST_F(CanaryDoubleApplyTest, SecondApply_DoesNotCallEngineAgain) {
    auto engine = std::make_shared<StubHotReloadEngine>(/*apply_succeeds=*/true);
    CanaryConfig cfg;
    cfg.version = "1.5.0";
    cfg.node_id = "node-full";
    cfg.stages = {{1.0, std::chrono::seconds{0}}};
    CanaryRollout rollout(engine, cfg);

    rollout.applyIfIncluded();  // first apply
    rollout.applyIfIncluded();  // second apply – must be a no-op

    EXPECT_EQ(engine->apply_call_count_, 1) << "Engine must be called exactly once";
}

TEST_F(CanaryDoubleApplyTest, SecondApply_ReturnsSuccessWithRollbackId) {
    auto engine = std::make_shared<StubHotReloadEngine>(/*apply_succeeds=*/true);
    CanaryConfig cfg;
    cfg.version = "1.5.0";
    cfg.node_id = "node-full";
    cfg.stages = {{1.0, std::chrono::seconds{0}}};
    CanaryRollout rollout(engine, cfg);

    rollout.applyIfIncluded();
    auto result = rollout.applyIfIncluded();

    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.rollback_id.empty());
}

TEST_F(CanaryDoubleApplyTest, FailedFirstApply_AllowsRetry) {
    auto engine = std::make_shared<StubHotReloadEngine>(/*apply_succeeds=*/false);
    CanaryConfig cfg;
    cfg.version = "1.5.0";
    cfg.node_id = "node-full";
    cfg.stages = {{1.0, std::chrono::seconds{0}}};
    CanaryRollout rollout(engine, cfg);

    auto r1 = rollout.applyIfIncluded();  // fails
    EXPECT_FALSE(r1.success);

    // Engine can try again since is_applied_ is only set on success
    engine->apply_call_count_ = 0;  // reset counter for clarity
    rollout.applyIfIncluded();
    EXPECT_EQ(engine->apply_call_count_, 1);
}

// ---------------------------------------------------------------------------
// Fix 2: UpdatesConfig::CanaryConfig::toCanaryConfig bridge
// ---------------------------------------------------------------------------

class UpdatesConfigToCanaryConfigTest : public ::testing::Test {};

TEST_F(UpdatesConfigToCanaryConfigTest, DefaultConfig_ConvertsToRuntimeConfig) {
    UpdatesConfig cfg;
    cfg.canary.node_id = "my-node";

    auto runtime = cfg.canary.toCanaryConfig("1.5.0");

    EXPECT_EQ(runtime.version, "1.5.0");
    EXPECT_EQ(runtime.node_id, "my-node");
    EXPECT_EQ(runtime.stages.size(), cfg.canary.stages.size());
    EXPECT_DOUBLE_EQ(runtime.error_rate_threshold, cfg.canary.error_rate_threshold);
    EXPECT_EQ(runtime.min_sample_count, cfg.canary.min_sample_count);
}

TEST_F(UpdatesConfigToCanaryConfigTest, CustomStages_AreConvertedCorrectly) {
    UpdatesConfig cfg;
    cfg.canary.node_id = "node-x";
    cfg.canary.stages = {{0.02, 1800}, {0.20, 3600}, {1.0, 0}};

    auto runtime = cfg.canary.toCanaryConfig("2.0.0");

    ASSERT_EQ(runtime.stages.size(), 3u);
    EXPECT_DOUBLE_EQ(runtime.stages[0].percentage, 0.02);
    EXPECT_EQ(runtime.stages[0].observation_duration, std::chrono::seconds{1800});
    EXPECT_DOUBLE_EQ(runtime.stages[1].percentage, 0.20);
    EXPECT_EQ(runtime.stages[1].observation_duration, std::chrono::seconds{3600});
    EXPECT_DOUBLE_EQ(runtime.stages[2].percentage, 1.0);
    EXPECT_EQ(runtime.stages[2].observation_duration, std::chrono::seconds{0});
}

TEST_F(UpdatesConfigToCanaryConfigTest, ConvertedConfig_CanConstructCanaryRollout) {
    auto engine = std::make_shared<StubHotReloadEngine>();
    UpdatesConfig cfg;
    cfg.canary.enabled = true;
    cfg.canary.node_id = "integration-node";
    cfg.canary.error_rate_threshold = 0.03;
    cfg.canary.min_sample_count = 30;

    auto runtime = cfg.canary.toCanaryConfig("1.6.0");
    EXPECT_NO_THROW(CanaryRollout rollout(engine, runtime));
}

TEST_F(UpdatesConfigToCanaryConfigTest, DefaultStagesYieldFourStages) {
    UpdatesConfig cfg;
    cfg.canary.node_id = "node-a";
    auto runtime = cfg.canary.toCanaryConfig("1.0.0");
    EXPECT_EQ(runtime.stages.size(), 4u);
    EXPECT_DOUBLE_EQ(runtime.stages.back().percentage, 1.0);
}
