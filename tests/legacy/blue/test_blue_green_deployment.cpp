// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_blue_green_deployment.cpp
 * @brief Unit tests for BlueGreenDeployment (Phase 4 – Blue/Green deployment)
 *
 * All tests use only the public API of BlueGreenDeployment.  The
 * HotReloadEngine dependency is satisfied by a lightweight stub that overrides
 * the two virtual methods called by BlueGreenDeployment: applyHotReload() and
 * rollback().
 */

#include <gtest/gtest.h>

#include "updates/blue_green_deployment.h"

#include <atomic>
#include <memory>
#include <stdexcept>
#include <string>

using namespace themis::updates;

// ---------------------------------------------------------------------------
// Minimal stub of HotReloadEngine
// ---------------------------------------------------------------------------

class StubHotReloadEngine : public HotReloadEngine {
public:
    explicit StubHotReloadEngine(bool apply_succeeds  = true,
                                 bool rollback_succeeds = true)
        : HotReloadEngine(nullptr, nullptr,
                          []() {
                              HotReloadEngine::Config c;
                              c.download_directory = "/tmp/stub_bg_dl";
                              c.backup_directory   = "/tmp/stub_bg_bak";
                              c.verify_signatures  = false;
                              c.create_backup      = false;
                              return c;
                          }())
        , apply_succeeds_(apply_succeeds)
        , rollback_succeeds_(rollback_succeeds)
    {}

    ReloadResult applyHotReload(const std::string& version,
                                bool /*verify_only*/ = false) override {
        ++apply_call_count;
        ReloadResult r;
        r.success     = apply_succeeds_;
        r.rollback_id = "stub_rollback_" + version;
        if (!apply_succeeds_) {
            r.error_message = "stub: apply failed";
        }
        return r;
    }

    bool rollback(const std::string& /*rollback_id*/) override {
        ++rollback_call_count;
        return rollback_succeeds_;
    }

    std::atomic<int> apply_call_count{0};
    std::atomic<int> rollback_call_count{0};

private:
    bool apply_succeeds_;
    bool rollback_succeeds_;
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::shared_ptr<StubHotReloadEngine> makeEngine(
    bool apply_ok   = true,
    bool rollback_ok = true)
{
    return std::make_shared<StubHotReloadEngine>(apply_ok, rollback_ok);
}

static BlueGreenConfig makeConfig(
    DeploymentSlot initial_active = DeploymentSlot::BLUE,
    double error_rate_threshold   = 0.05,
    size_t min_sample_count       = 20)
{
    BlueGreenConfig cfg;
    cfg.initial_active_slot   = initial_active;
    cfg.error_rate_threshold  = error_rate_threshold;
    cfg.min_sample_count      = min_sample_count;
    return cfg;
}

// ---------------------------------------------------------------------------
// Construction tests
// ---------------------------------------------------------------------------

class BlueGreenConstructionTest : public ::testing::Test {};

TEST_F(BlueGreenConstructionTest, DefaultConfig_ActiveSlotIsBlue) {
    auto engine = makeEngine();
    BlueGreenDeployment bg(engine);
    EXPECT_EQ(bg.activeSlot(), DeploymentSlot::BLUE);
}

TEST_F(BlueGreenConstructionTest, GreenInitialSlot_ActiveSlotIsGreen) {
    auto engine = makeEngine();
    BlueGreenDeployment bg(engine, makeConfig(DeploymentSlot::GREEN));
    EXPECT_EQ(bg.activeSlot(), DeploymentSlot::GREEN);
}

TEST_F(BlueGreenConstructionTest, NullEngine_Throws) {
    EXPECT_THROW(BlueGreenDeployment(nullptr), std::invalid_argument);
}

TEST_F(BlueGreenConstructionTest, InitialStatus_NothingDeployed) {
    auto engine = makeEngine();
    BlueGreenDeployment bg(engine);
    auto s = bg.status();
    EXPECT_EQ(s.active_slot, DeploymentSlot::BLUE);
    EXPECT_FALSE(s.standby_is_deployed);
    EXPECT_FALSE(s.is_promoted);
    EXPECT_FALSE(s.is_rolled_back);
    EXPECT_TRUE(s.blue_version.empty());
    EXPECT_TRUE(s.green_version.empty());
    EXPECT_TRUE(s.rollback_id.empty());
    EXPECT_DOUBLE_EQ(s.observed_error_rate, 0.0);
    EXPECT_EQ(s.sample_count, 0u);
}

// ---------------------------------------------------------------------------
// deployToStandby tests
// ---------------------------------------------------------------------------

class BlueGreenDeployTest : public ::testing::Test {
protected:
    std::shared_ptr<StubHotReloadEngine> engine_ = makeEngine();
    BlueGreenDeployment bg_{engine_};
};

TEST_F(BlueGreenDeployTest, Deploy_CallsEngineApply) {
    auto result = bg_.deployToStandby("1.5.0");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(engine_->apply_call_count.load(), 1);
}

TEST_F(BlueGreenDeployTest, Deploy_StoresVersionInStandbySlot) {
    bg_.deployToStandby("1.5.0");
    // Active is BLUE, standby is GREEN → version goes to green slot
    EXPECT_EQ(bg_.slotVersion(DeploymentSlot::GREEN), "1.5.0");
    EXPECT_TRUE(bg_.slotVersion(DeploymentSlot::BLUE).empty());
}

TEST_F(BlueGreenDeployTest, Deploy_SetsStandbyDeployedFlag) {
    bg_.deployToStandby("1.5.0");
    EXPECT_TRUE(bg_.status().standby_is_deployed);
}

TEST_F(BlueGreenDeployTest, Deploy_StoresRollbackId) {
    bg_.deployToStandby("1.5.0");
    auto s = bg_.status();
    EXPECT_EQ(s.rollback_id, "stub_rollback_1.5.0");
}

TEST_F(BlueGreenDeployTest, Deploy_ActiveSlotUnchanged) {
    bg_.deployToStandby("1.5.0");
    EXPECT_EQ(bg_.activeSlot(), DeploymentSlot::BLUE);
}

TEST_F(BlueGreenDeployTest, Deploy_Failure_NotMarkedAsDeployed) {
    auto engine = makeEngine(/*apply_ok=*/false);
    BlueGreenDeployment bg(engine);
    auto result = bg.deployToStandby("1.5.0");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(bg.status().standby_is_deployed);
}

TEST_F(BlueGreenDeployTest, Deploy_WhenAlreadyOccupied_Rejected) {
    bg_.deployToStandby("1.5.0");
    // Second deploy without promote/rollback should fail
    auto result = bg_.deployToStandby("1.6.0");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(engine_->apply_call_count.load(), 1); // only first call went through
}

TEST_F(BlueGreenDeployTest, Deploy_AfterRollback_Rejected) {
    bg_.deployToStandby("1.5.0");
    bg_.rollback();
    auto result = bg_.deployToStandby("1.6.0");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(engine_->apply_call_count.load(), 1);
}

// ---------------------------------------------------------------------------
// promote tests
// ---------------------------------------------------------------------------

class BlueGreenPromoteTest : public ::testing::Test {
protected:
    std::shared_ptr<StubHotReloadEngine> engine_ = makeEngine();
    BlueGreenDeployment bg_{engine_};

    void SetUp() override {
        bg_.deployToStandby("1.5.0");
    }
};

TEST_F(BlueGreenPromoteTest, Promote_SwitchesActiveSlot) {
    EXPECT_EQ(bg_.activeSlot(), DeploymentSlot::BLUE);
    bool ok = bg_.promote();
    EXPECT_TRUE(ok);
    EXPECT_EQ(bg_.activeSlot(), DeploymentSlot::GREEN);
}

TEST_F(BlueGreenPromoteTest, Promote_SetsPromotedFlag) {
    bg_.promote();
    EXPECT_TRUE(bg_.status().is_promoted);
}

TEST_F(BlueGreenPromoteTest, Promote_DoesNotCallEngine) {
    bg_.promote();
    // promote() only changes slot state; it does not invoke applyHotReload
    EXPECT_EQ(engine_->apply_call_count.load(), 1); // only from deployToStandby
}

TEST_F(BlueGreenPromoteTest, Promote_InvokesCallback) {
    DeploymentSlot cb_slot  = DeploymentSlot::BLUE;
    std::string    cb_ver;
    bg_.setPromotionCallback([&](DeploymentSlot slot, const std::string& ver) {
        cb_slot = slot;
        cb_ver  = ver;
    });
    bg_.promote();
    EXPECT_EQ(cb_slot, DeploymentSlot::GREEN);
    EXPECT_EQ(cb_ver, "1.5.0");
}

TEST_F(BlueGreenPromoteTest, Promote_ResetsHealthCounters) {
    bg_.reportSuccess();
    bg_.reportError();
    bg_.promote();
    EXPECT_DOUBLE_EQ(bg_.errorRate(), 0.0);
    EXPECT_EQ(bg_.status().sample_count, 0u);
}

TEST_F(BlueGreenPromoteTest, Promote_CalledTwice_ReturnsFalse) {
    bg_.promote();
    EXPECT_FALSE(bg_.promote());
}

TEST_F(BlueGreenPromoteTest, Promote_WithoutDeploy_ReturnsFalse) {
    auto engine = makeEngine();
    BlueGreenDeployment bg(engine);
    EXPECT_FALSE(bg.promote());
}

TEST_F(BlueGreenPromoteTest, Promote_AfterRollback_ReturnsFalse) {
    bg_.rollback();
    EXPECT_FALSE(bg_.promote());
}

// ---------------------------------------------------------------------------
// rollback tests
// ---------------------------------------------------------------------------

class BlueGreenRollbackTest : public ::testing::Test {
protected:
    std::shared_ptr<StubHotReloadEngine> engine_ = makeEngine();
    BlueGreenDeployment bg_{engine_};

    void SetUp() override {
        bg_.deployToStandby("1.5.0");
        bg_.promote();
    }
};

TEST_F(BlueGreenRollbackTest, Rollback_InvokesEngineRollback) {
    bg_.rollback();
    EXPECT_EQ(engine_->rollback_call_count.load(), 1);
}

TEST_F(BlueGreenRollbackTest, Rollback_SetsRolledBackFlag) {
    bg_.rollback();
    EXPECT_TRUE(bg_.status().is_rolled_back);
}

TEST_F(BlueGreenRollbackTest, Rollback_RestoresOriginalActiveSlot) {
    // Active was promoted to GREEN; rollback should restore BLUE
    EXPECT_EQ(bg_.activeSlot(), DeploymentSlot::GREEN);
    bg_.rollback();
    EXPECT_EQ(bg_.activeSlot(), DeploymentSlot::BLUE);
}

TEST_F(BlueGreenRollbackTest, Rollback_InvokesCallback) {
    std::string reason_received;
    bg_.setRollbackCallback([&](const std::string& r) {
        reason_received = r;
    });
    bg_.rollback("too many errors");
    EXPECT_EQ(reason_received, "too many errors");
}

TEST_F(BlueGreenRollbackTest, Rollback_DefaultReason) {
    bg_.rollback();
    EXPECT_EQ(bg_.status().rollback_reason, "manual rollback");
}

TEST_F(BlueGreenRollbackTest, Rollback_CalledTwice_ReturnsFalse) {
    bg_.rollback();
    EXPECT_FALSE(bg_.rollback());
}

TEST_F(BlueGreenRollbackTest, Rollback_EngineFailure_ReturnsFalse) {
    auto engine = makeEngine(/*apply_ok=*/true, /*rollback_ok=*/false);
    BlueGreenDeployment bg(engine);
    bg.deployToStandby("1.5.0");
    bg.promote();
    EXPECT_FALSE(bg.rollback());
    EXPECT_TRUE(bg.status().is_rolled_back);
}

TEST_F(BlueGreenRollbackTest, Rollback_WithoutRollbackId_ReturnsTrue) {
    // Deploy failed so no rollback_id was stored; rollback should still
    // succeed (nothing to undo at the engine level).
    auto engine = makeEngine();
    BlueGreenDeployment bg(engine);
    // Never deployed → rollback_id is empty
    bg.rollback("precautionary");
    EXPECT_TRUE(bg.status().is_rolled_back);
    EXPECT_EQ(engine->rollback_call_count.load(), 0);
}

// ---------------------------------------------------------------------------
// Health tracking tests
// ---------------------------------------------------------------------------

class BlueGreenHealthTest : public ::testing::Test {
protected:
    std::shared_ptr<StubHotReloadEngine> engine_ = makeEngine();
    // Use a low threshold and sample count for fast testing
    BlueGreenDeployment bg_{engine_,
                             makeConfig(DeploymentSlot::BLUE, 0.3, 5)};

    void SetUp() override {
        bg_.deployToStandby("1.5.0");
        bg_.promote();
    }
};

TEST_F(BlueGreenHealthTest, ErrorRate_ZeroWhenNoEvents) {
    EXPECT_DOUBLE_EQ(bg_.errorRate(), 0.0);
}

TEST_F(BlueGreenHealthTest, ErrorRate_CalculatedCorrectly) {
    bg_.reportSuccess();
    bg_.reportSuccess();
    bg_.reportError();
    // 1 error / 3 total = 0.333...
    EXPECT_NEAR(bg_.errorRate(), 1.0 / 3.0, 1e-9);
}

TEST_F(BlueGreenHealthTest, ShouldRollback_FalseBeforeMinSampleCount) {
    // Only 3 events, threshold requires 5
    bg_.reportError();
    bg_.reportError();
    bg_.reportError();
    EXPECT_FALSE(bg_.shouldRollback());
}

TEST_F(BlueGreenHealthTest, ShouldRollback_TrueAfterThreshold) {
    // 5 events, 4 errors → 80% error rate > 30% threshold
    bg_.reportError();
    bg_.reportError();
    bg_.reportError();
    bg_.reportError();
    bg_.reportSuccess();
    EXPECT_TRUE(bg_.shouldRollback());
}

TEST_F(BlueGreenHealthTest, AutoRollback_TriggeredByReportError) {
    // Fill 4 successes then trigger the 5th event as error to pass threshold
    bg_.reportSuccess();
    bg_.reportSuccess();
    bg_.reportSuccess();
    bg_.reportSuccess();
    // 5th event: error → 1/5 = 20% < 30% threshold → no rollback yet
    bg_.reportError();
    EXPECT_FALSE(bg_.status().is_rolled_back);

    // Additional errors to exceed threshold
    bg_.reportError();  // 2/6 = 33% > 30% → rollback
    EXPECT_TRUE(bg_.status().is_rolled_back);
    EXPECT_EQ(engine_->rollback_call_count.load(), 1);
}

TEST_F(BlueGreenHealthTest, ShouldRollback_FalseWhenNotYetPromoted) {
    auto engine = makeEngine();
    BlueGreenDeployment bg(engine, makeConfig(DeploymentSlot::BLUE, 0.05, 2));
    bg.deployToStandby("1.5.0");
    // Not promoted: auto-rollback must not trigger
    bg.reportError();
    bg.reportError();
    EXPECT_FALSE(bg.shouldRollback());
    EXPECT_FALSE(bg.status().is_rolled_back);
}

// ---------------------------------------------------------------------------
// slotVersion tests
// ---------------------------------------------------------------------------

class BlueGreenSlotVersionTest : public ::testing::Test {};

TEST_F(BlueGreenSlotVersionTest, SlotVersion_EmptyBeforeDeploy) {
    auto engine = makeEngine();
    BlueGreenDeployment bg(engine);
    EXPECT_TRUE(bg.slotVersion(DeploymentSlot::BLUE).empty());
    EXPECT_TRUE(bg.slotVersion(DeploymentSlot::GREEN).empty());
}

TEST_F(BlueGreenSlotVersionTest, SlotVersion_SetAfterDeploy) {
    auto engine = makeEngine();
    BlueGreenDeployment bg(engine);
    bg.deployToStandby("2.0.0");
    // Active=BLUE → standby=GREEN → green gets the version
    EXPECT_EQ(bg.slotVersion(DeploymentSlot::GREEN), "2.0.0");
    EXPECT_TRUE(bg.slotVersion(DeploymentSlot::BLUE).empty());
}

TEST_F(BlueGreenSlotVersionTest, SlotVersion_GreenInitialActive_DeploysToBlue) {
    auto engine = makeEngine();
    BlueGreenDeployment bg(engine, makeConfig(DeploymentSlot::GREEN));
    bg.deployToStandby("2.0.0");
    // Active=GREEN → standby=BLUE → blue gets the version
    EXPECT_EQ(bg.slotVersion(DeploymentSlot::BLUE), "2.0.0");
    EXPECT_TRUE(bg.slotVersion(DeploymentSlot::GREEN).empty());
}

// ---------------------------------------------------------------------------
// Full lifecycle test
// ---------------------------------------------------------------------------

class BlueGreenLifecycleTest : public ::testing::Test {};

TEST_F(BlueGreenLifecycleTest, FullCycle_DeployPromoteRollback) {
    auto engine = makeEngine();
    BlueGreenDeployment bg(engine);

    // Phase 1: deploy to standby
    auto r = bg.deployToStandby("1.5.0");
    EXPECT_TRUE(r.success);
    EXPECT_EQ(bg.activeSlot(), DeploymentSlot::BLUE);

    // Phase 2: promote
    EXPECT_TRUE(bg.promote());
    EXPECT_EQ(bg.activeSlot(), DeploymentSlot::GREEN);
    EXPECT_EQ(bg.slotVersion(DeploymentSlot::GREEN), "1.5.0");

    // Phase 3: rollback
    EXPECT_TRUE(bg.rollback("test rollback"));
    EXPECT_EQ(bg.activeSlot(), DeploymentSlot::BLUE);
    EXPECT_TRUE(bg.status().is_rolled_back);
    EXPECT_EQ(engine->rollback_call_count.load(), 1);
}

TEST_F(BlueGreenLifecycleTest, FullCycle_DeployPromoteNewDeploy) {
    auto engine = makeEngine();
    BlueGreenDeployment bg(engine, makeConfig(DeploymentSlot::BLUE, 0.05, 100));

    // First deployment to GREEN
    bg.deployToStandby("1.5.0");
    bg.promote();
    EXPECT_EQ(bg.activeSlot(), DeploymentSlot::GREEN);

    // Second deployment should now target BLUE (the new standby)
    auto r = bg.deployToStandby("1.6.0");
    EXPECT_TRUE(r.success);
    EXPECT_EQ(bg.slotVersion(DeploymentSlot::BLUE), "1.6.0");
    EXPECT_EQ(bg.slotVersion(DeploymentSlot::GREEN), "1.5.0");

    // Promote again: BLUE becomes active
    EXPECT_TRUE(bg.promote());
    EXPECT_EQ(bg.activeSlot(), DeploymentSlot::BLUE);
    EXPECT_EQ(engine->apply_call_count.load(), 2);
}

TEST_F(BlueGreenLifecycleTest, Status_ReflectsAllStateFields) {
    auto engine = makeEngine();
    BlueGreenDeployment bg(engine);

    bg.deployToStandby("1.5.0");
    bg.promote();
    bg.reportSuccess();
    bg.reportError();

    auto s = bg.status();
    EXPECT_EQ(s.active_slot, DeploymentSlot::GREEN);
    EXPECT_EQ(s.green_version, "1.5.0");
    EXPECT_TRUE(s.standby_is_deployed);
    EXPECT_TRUE(s.is_promoted);
    EXPECT_FALSE(s.is_rolled_back);
    EXPECT_EQ(s.rollback_id, "stub_rollback_1.5.0");
    EXPECT_EQ(s.sample_count, 2u);
    EXPECT_NEAR(s.observed_error_rate, 0.5, 1e-9);
}
