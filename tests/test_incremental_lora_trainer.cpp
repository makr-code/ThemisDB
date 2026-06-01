/*
 * ThemisDB | File: test_incremental_lora_trainer.cpp | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file test_incremental_lora_trainer.cpp
 * @brief Unit tests for IncrementalLoRATrainer federation bridges (IMPL-A3).
 *
 * Tests ILT-EG-01…03  — exportGradient() contract
 * Tests ILT-AG-01…02  — applyGlobalDelta() contract
 */

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "training/incremental_lora_trainer.h"
#include "distributed_knowledge/lora_federation_coordinator.h"

using namespace themis::training;
using namespace themis::distributed_knowledge;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static IncrementalTrainingConfig makeConfig() {
    IncrementalTrainingConfig cfg;
    cfg.training_data_collection = "";
    cfg.base_model_path          = "";
    cfg.adapter_version          = "";
    cfg.rank                     = 4;
    cfg.alpha                    = 8.0f;
    cfg.learning_rate            = 0.001f;
    cfg.batch_size               = 2;
    cfg.num_epochs               = 1;
    cfg.max_seq_length           = 16;
    cfg.device                   = "cpu";
    return cfg;
}

// Run one training cycle so the gradient accumulator is non-empty.
static TrainingResult runOneTrain(IncrementalLoRATrainer& trainer) {
    return trainer.train(TrainingMode::INITIAL);
}

class TestRouter final : public ILLMRouter {
public:
    bool available = true;
    bool accept_weight = true;
    std::string last_version;
    float last_weight = -1.0f;

    bool setAdapterWeight(const std::string& version, float weight) override {
        last_version = version;
        last_weight = weight;
        return accept_weight;
    }

    bool isAvailable() const override {
        return available;
    }

    std::string activeVersion() const override {
        return last_version;
    }
};

// ---------------------------------------------------------------------------
// ILT-EG-01 — exportGradient() after at least one train() call gives
//             non-empty data
// ---------------------------------------------------------------------------

TEST(ImplA3_ExportGradient, EG01_NonEmptyDataAfterTraining) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    trainer.setShardId("shard_test_01");

    auto result = runOneTrain(trainer);
    EXPECT_TRUE(result.success);

    EncryptedGradient grad = trainer.exportGradient(/*round=*/1);

    EXPECT_EQ(grad.shard_id, "shard_test_01");
    EXPECT_EQ(grad.round, 1u);
    EXPECT_GT(grad.sample_count, 0u);
    EXPECT_FALSE(grad.data.empty()) << "Gradient data map must be non-empty after training";

    // Verify the expected layer names are present
    EXPECT_TRUE(grad.data.contains("lora_A_layer_0"));
    EXPECT_TRUE(grad.data.contains("lora_B_layer_0"));
}

// ---------------------------------------------------------------------------
// ILT-EG-02 — exportGradient() throws std::runtime_error when no training
//             has occurred since the last export (or ever)
// ---------------------------------------------------------------------------

TEST(ImplA3_ExportGradient, EG02_ThrowsWithoutTraining) {
    IncrementalLoRATrainer trainer(makeConfig(), "");

    EXPECT_THROW(
        trainer.exportGradient(/*round=*/1),
        std::runtime_error);
}

// ---------------------------------------------------------------------------
// ILT-EG-03 — exportGradient() resets the accumulator; a second immediate
//             call throws std::runtime_error
// ---------------------------------------------------------------------------

TEST(ImplA3_ExportGradient, EG03_ResetsAccumulatorOnExport) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    runOneTrain(trainer);

    // First export succeeds
    EXPECT_NO_THROW(trainer.exportGradient(/*round=*/1));

    // Second export immediately after must throw because update_count == 0
    EXPECT_THROW(
        trainer.exportGradient(/*round=*/2),
        std::runtime_error);
}

// ---------------------------------------------------------------------------
// ILT-AG-01 — applyGlobalDelta() changes local weights measurably
// ---------------------------------------------------------------------------

TEST(ImplA3_ApplyGlobalDelta, AG01_ChangesLocalWeightsMeasurably) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    trainer.setFederatedLearningRate(0.1);

    // Train first so known_layers_ is populated (required for applyGlobalDelta)
    auto res = runOneTrain(trainer);
    EXPECT_TRUE(res.success);

    // Verify initial state (before applying delta)
    EXPECT_DOUBLE_EQ(trainer.getLocalWeight("lora_A_layer_0"), 0.0);

    // Build a delta with a known value for a known layer
    GlobalAdapterDelta delta;
    delta.round        = 1;
    delta.version      = "global-v1";
    delta.participants = 2;
    delta.algorithm    = "FedAvg";
    delta.epsilon_spent = 0.05;
    delta.delta["lora_A_layer_0"] = 1.0;
    delta.delta["lora_B_layer_0"] = 2.0;

    trainer.applyGlobalDelta(delta);

    // Expected: 0.0 + 0.1 * 1.0 = 0.1
    EXPECT_NEAR(trainer.getLocalWeight("lora_A_layer_0"), 0.1, 1e-9);
    // Expected: 0.0 + 0.1 * 2.0 = 0.2
    EXPECT_NEAR(trainer.getLocalWeight("lora_B_layer_0"), 0.2, 1e-9);
}

// ---------------------------------------------------------------------------
// ILT-AG-02 — applyGlobalDelta() with unknown layer names does not throw
// ---------------------------------------------------------------------------

TEST(ImplA3_ApplyGlobalDelta, AG02_UnknownLayerNamesAreIgnored) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    // No training → known_layers_ is empty → all delta layers are "unknown"

    GlobalAdapterDelta delta;
    delta.round        = 1;
    delta.version      = "global-v1";
    delta.participants = 1;
    delta.algorithm    = "FedAvg";
    delta.epsilon_spent = 0.01;
    // Layers that do not exist in the local weight map (no training done)
    delta.delta["unknown_layer_xyz"] = 99.0;
    delta.delta["non_existent_head"] = -7.5;

    // Must not throw
    EXPECT_NO_THROW(trainer.applyGlobalDelta(delta));

    // Because no training was done, known_layers_ is empty, so the unknown
    // layers should have been ignored — local_weights_ is empty, getLocalWeight
    // returns 0.0 for any name.
    EXPECT_DOUBLE_EQ(trainer.getLocalWeight("unknown_layer_xyz"), 0.0)
        << "Unknown layers (not seen during training) must be ignored";
}

// ============================================================================
// Router mutex / data-race hardening (#5414)
// ============================================================================

// A null router must not be called; deployVersionEx with no registered version
// should fail cleanly without touching any router.
TEST(RouterMutex, DeployVersionEx_NoVersion_NullRouter_FailsClean) {
    IncrementalTrainingConfig cfg;
    cfg.adapter_version = "v1";
    IncrementalLoRATrainer trainer(cfg, "");
    // No setLLMRouter() call — router is null.
    auto r = trainer.deployVersionEx("nonexistent", 0.5f);
    EXPECT_FALSE(r.success);
    EXPECT_FALSE(r.error.empty());
}

// setLLMRouter(nullptr) detaches the router; subsequent deploy/rollback must
// not crash.
TEST(RouterMutex, SetNullRouter_DeployVersionEx_DoesNotCrash) {
    IncrementalTrainingConfig cfg;
    cfg.adapter_version = "v1";
    IncrementalLoRATrainer trainer(cfg, "");
    trainer.setLLMRouter(nullptr);
    // Should not crash with a null router.
    auto r = trainer.deployVersionEx("nonexistent", 0.5f);
    EXPECT_FALSE(r.success);
}

TEST(DeploymentDeterminism, DeployVersion_NearFullSplitTreatsAsFullDeployment) {
    IncrementalLoRATrainer trainer(makeConfig(), "");

    ASSERT_TRUE(trainer.deployVersion("v1", 0.40f));
    ASSERT_TRUE(trainer.deployVersion("v2", 0.9999996f));

    for (int i = 0; i < 50; ++i) {
        EXPECT_EQ(trainer.selectAdapterForRequest(), "v2");
    }
}

TEST(RouterMutex, DeployVersionEx_RouterRejectsUpdate_RevertsRegistryState) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    ASSERT_TRUE(trainer.deployVersion("stable", 1.0f));

    TestRouter router;
    router.accept_weight = false;
    trainer.setLLMRouter(&router);

    auto result = trainer.deployVersionEx("candidate", 0.25f);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "router_update_failed");
    EXPECT_EQ(router.last_version, "candidate");
    EXPECT_FLOAT_EQ(router.last_weight, 0.25f);

    const auto versions = trainer.listVersions();
    ASSERT_EQ(versions.size(), 1u);
    EXPECT_EQ(versions.front(), "stable");
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(trainer.selectAdapterForRequest(), "stable");
    }
}

TEST(RouterMutex, RollbackVersionEx_RouterRejectsUpdate_RevertsRegistryState) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    ASSERT_TRUE(trainer.deployVersion("stable", 1.0f));
    ASSERT_TRUE(trainer.deployVersion("candidate", 0.0f));

    TestRouter router;
    router.accept_weight = false;
    trainer.setLLMRouter(&router);

    auto result = trainer.rollbackVersionEx("candidate");

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "router_update_failed");
    EXPECT_EQ(router.last_version, "candidate");
    EXPECT_FLOAT_EQ(router.last_weight, 1.0f);

    const auto versions = trainer.listVersions();
    ASSERT_EQ(versions.size(), 2u);
    EXPECT_EQ(versions[0], "candidate");
    EXPECT_EQ(versions[1], "stable");
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(trainer.selectAdapterForRequest(), "stable");
    }
}

TEST(RouterMutex, DeployVersionEx_RouterUnavailable_RevertsRegistryState) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    ASSERT_TRUE(trainer.deployVersion("stable", 1.0f));

    TestRouter router;
    router.available = false;
    trainer.setLLMRouter(&router);

    auto result = trainer.deployVersionEx("candidate", 0.25f);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "router_unavailable");
    EXPECT_TRUE(router.last_version.empty());
    EXPECT_LT(router.last_weight, 0.0f);

    const auto versions = trainer.listVersions();
    ASSERT_EQ(versions.size(), 1u);
    EXPECT_EQ(versions.front(), "stable");
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(trainer.selectAdapterForRequest(), "stable");
    }
}

TEST(RouterMutex, RollbackVersionEx_RouterUnavailable_RevertsRegistryState) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    ASSERT_TRUE(trainer.deployVersion("stable", 1.0f));
    ASSERT_TRUE(trainer.deployVersion("candidate", 0.0f));

    TestRouter router;
    router.available = false;
    trainer.setLLMRouter(&router);

    auto result = trainer.rollbackVersionEx("candidate");

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "router_unavailable");
    EXPECT_TRUE(router.last_version.empty());
    EXPECT_LT(router.last_weight, 0.0f);

    const auto versions = trainer.listVersions();
    ASSERT_EQ(versions.size(), 2u);
    EXPECT_EQ(versions[0], "candidate");
    EXPECT_EQ(versions[1], "stable");
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(trainer.selectAdapterForRequest(), "stable");
    }
}

// ============================================================================
// resumeFromCheckpoint() failure-path hardening (#5414)
// ============================================================================

// RFC-01: Empty checkpoint path must fail gracefully without throwing.
TEST(ResumeFromCheckpoint, RFC01_EmptyPath_ReturnsFailureWithMessage) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    auto result = trainer.resumeFromCheckpoint("");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty())
        << "error_message must be set when checkpoint_path is empty";
    EXPECT_NE(result.error_message.find("checkpoint"), std::string::npos)
        << "error_message must mention 'checkpoint'";
}

// RFC-02: Non-existent checkpoint path must fail gracefully without throwing.
TEST(ResumeFromCheckpoint, RFC02_NonExistentPath_ReturnsFailureWithMessage) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    auto result = trainer.resumeFromCheckpoint("/nonexistent/path/ckpt.bin");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty())
        << "error_message must describe the load failure";
}

// RFC-03: training_time_seconds is always populated (>= 0) even on failure.
TEST(ResumeFromCheckpoint, RFC03_FailedResume_ElapsedTimeRecorded) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    auto result = trainer.resumeFromCheckpoint("/no/such/checkpoint");
    EXPECT_GE(result.training_time_seconds, 0.0)
        << "training_time_seconds must be non-negative even after a failed resume";
}
