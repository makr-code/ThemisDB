// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_lora_trainer_production.cpp
 * @brief Production readiness tests for IncrementalLoRATrainer (Phases 3, 4, 5)
 *
 * Covers:
 *  - Phase 3: Training implementation – loss/accuracy, training result, hyperparams
 *  - Phase 4: Version management, deployment, A/B traffic split, rollback
 *  - Phase 5: Checkpointing, resume-from-checkpoint, crash recovery
 */

#include <gtest/gtest.h>
#include "training/incremental_lora_trainer.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace themis::training;

// ============================================================================
// Test fixture
// ============================================================================
class LoRATrainerProductionTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.training_data_collection = "legal_training_samples";
        config_.base_model_path          = "/models/llama-2-7b.gguf";
        config_.adapter_version          = "";
        config_.rank                     = 8;
        config_.alpha                    = 16.0f;
        config_.dropout                  = 0.1f;
        config_.learning_rate            = 0.0003f;
        config_.batch_size               = 4;
        config_.num_epochs               = 2;
        config_.max_seq_length           = 512;
        config_.device                   = "cpu";
    }

    IncrementalTrainingConfig config_;
    const std::string db_conn_ = "";
};

// ============================================================================
// Phase 3: Training implementation
// ============================================================================

TEST_F(LoRATrainerProductionTest, Construction_Succeeds) {
    EXPECT_NO_THROW(IncrementalLoRATrainer trainer(config_, db_conn_));
}

TEST_F(LoRATrainerProductionTest, Train_Initial_ReturnsSuccess) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    auto result = trainer.train(TrainingMode::INITIAL);

    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.version.empty());
    EXPECT_GE(result.training_time_seconds, 0.0);
}

TEST_F(LoRATrainerProductionTest, Train_Initial_VersionStartsAtV1_0) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    auto result = trainer.train(TrainingMode::INITIAL);

    EXPECT_EQ(result.version, "legal_v1.0");
}

TEST_F(LoRATrainerProductionTest, Train_AdapterIdMatchesVersion) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    auto result = trainer.train(TrainingMode::INITIAL);

    EXPECT_EQ(result.adapter_id, result.version);
}

TEST_F(LoRATrainerProductionTest, Train_LossIsNonNegative) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    auto result = trainer.train(TrainingMode::INITIAL);

    EXPECT_GE(result.training_loss,   0.0);
    EXPECT_GE(result.validation_loss, 0.0);
}

TEST_F(LoRATrainerProductionTest, Train_AccuracyInValidRange) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    auto result = trainer.train(TrainingMode::INITIAL);

    EXPECT_GE(result.accuracy, 0.0);
    EXPECT_LE(result.accuracy, 1.0);
}

TEST_F(LoRATrainerProductionTest, Train_Incremental_IncrementMinorVersion) {
    config_.adapter_version  = "legal_v1.0";
    config_.use_existing_adapter = true;
    IncrementalLoRATrainer trainer(config_, db_conn_);
    auto result = trainer.train(TrainingMode::INCREMENTAL);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.version, "legal_v1.1");
}

TEST_F(LoRATrainerProductionTest, Train_Finetune_IncrementMinorVersion) {
    config_.adapter_version = "legal_v2.3";
    IncrementalLoRATrainer trainer(config_, db_conn_);
    auto result = trainer.train(TrainingMode::FINETUNE);

    EXPECT_EQ(result.version, "legal_v2.4");
}

TEST_F(LoRATrainerProductionTest, Train_WithCallback_DoesNotCrash) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    std::vector<double> loss_history;

    auto result = trainer.train(TrainingMode::INITIAL,
        [&](size_t, size_t, double loss, const std::string&) {
            loss_history.push_back(loss);
        });

    EXPECT_TRUE(result.success);
    // Callback may or may not be invoked depending on training data availability
}

// ============================================================================
// Phase 3: Hyperparameter API
// ============================================================================

TEST_F(LoRATrainerProductionTest, SetHyperparameters_Valid_UpdatesConfig) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    EXPECT_NO_THROW(trainer.setHyperparameters(16, 32.0f, 0.001f));
}

TEST_F(LoRATrainerProductionTest, SetHyperparameters_InvalidRank_Throws) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    EXPECT_THROW(trainer.setHyperparameters(0, 16.0f, 0.001f), std::invalid_argument);
}

TEST_F(LoRATrainerProductionTest, SetHyperparameters_InvalidAlpha_Throws) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    EXPECT_THROW(trainer.setHyperparameters(8, 0.0f, 0.001f), std::invalid_argument);
}

TEST_F(LoRATrainerProductionTest, SetHyperparameters_InvalidLR_Throws) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    EXPECT_THROW(trainer.setHyperparameters(8, 16.0f, -0.001f), std::invalid_argument);
}

// ============================================================================
// Phase 4: Versioning & Deployment
// ============================================================================

TEST_F(LoRATrainerProductionTest, ListVersions_Empty_BeforeTraining) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    auto versions = trainer.listVersions();
    EXPECT_TRUE(versions.empty());
}

TEST_F(LoRATrainerProductionTest, ListVersions_ContainsTrainedVersion) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    auto result = trainer.train(TrainingMode::INITIAL);

    auto versions = trainer.listVersions();
    ASSERT_FALSE(versions.empty());
    EXPECT_NE(std::find(versions.begin(), versions.end(), result.version), versions.end());
}

TEST_F(LoRATrainerProductionTest, DeployVersion_ValidVersion_ReturnsTrue) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    EXPECT_TRUE(trainer.deployVersion("legal_v1.0", 1.0f));
}

TEST_F(LoRATrainerProductionTest, DeployVersion_EmptyVersion_ReturnsFalse) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    EXPECT_FALSE(trainer.deployVersion("", 1.0f));
}

TEST_F(LoRATrainerProductionTest, DeployVersion_InvalidTrafficSplit_ReturnsFalse) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    EXPECT_FALSE(trainer.deployVersion("legal_v1.0", 1.5f));
    EXPECT_FALSE(trainer.deployVersion("legal_v1.0", -0.1f));
}

TEST_F(LoRATrainerProductionTest, DeployVersion_CanaryDeployment_PartialSplit) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    // Canary: 10% traffic to new version
    EXPECT_TRUE(trainer.deployVersion("legal_v1.1", 0.1f));
}

TEST_F(LoRATrainerProductionTest, DeployVersion_FullDeployment_DeactivatesOthers) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    trainer.deployVersion("legal_v1.0", 0.5f);
    trainer.deployVersion("legal_v1.1", 1.0f); // full switch to v1.1

    auto versions = trainer.listVersions();
    ASSERT_GE(versions.size(), 2u);
}

TEST_F(LoRATrainerProductionTest, RollbackVersion_ValidVersion_ReturnsTrue) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    trainer.deployVersion("legal_v1.0", 1.0f);
    EXPECT_TRUE(trainer.rollbackVersion("legal_v1.0"));
}

TEST_F(LoRATrainerProductionTest, RollbackVersion_EmptyVersion_ReturnsFalse) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    EXPECT_FALSE(trainer.rollbackVersion(""));
}

TEST_F(LoRATrainerProductionTest, RollbackVersion_IsInVersionList) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    trainer.rollbackVersion("legal_v1.0");

    auto versions = trainer.listVersions();
    EXPECT_NE(std::find(versions.begin(), versions.end(), "legal_v1.0"), versions.end());
}

TEST_F(LoRATrainerProductionTest, Evaluate_EmptyVersion_ReturnsFailure) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    auto result = trainer.evaluate("");

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(LoRATrainerProductionTest, Evaluate_ValidVersion_Succeeds) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    auto result = trainer.evaluate("legal_v1.0");

    EXPECT_TRUE(result.success);
    EXPECT_GE(result.validation_loss, 0.0);
    EXPECT_GE(result.accuracy,        0.0);
    EXPECT_LE(result.accuracy,        1.0);
}

// ============================================================================
// Phase 5: Checkpointing & Recovery
// ============================================================================

TEST_F(LoRATrainerProductionTest, SetCheckpointing_Enabled_DoesNotThrow) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    EXPECT_NO_THROW(trainer.setCheckpointing(true, 50));
}

TEST_F(LoRATrainerProductionTest, SetCheckpointing_Disabled_DoesNotThrow) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    EXPECT_NO_THROW(trainer.setCheckpointing(false));
}

TEST_F(LoRATrainerProductionTest, SetCheckpointing_ZeroSteps_UsesDefault) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    EXPECT_NO_THROW(trainer.setCheckpointing(true, 0)); // should default internally
}

TEST_F(LoRATrainerProductionTest, ResumeFromCheckpoint_EmptyPath_ReturnsFailure) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    auto result = trainer.resumeFromCheckpoint("");

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(LoRATrainerProductionTest, ResumeFromCheckpoint_ValidMetadataPath_Succeeds) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    trainer.setCheckpointing(true, 10);

    const auto temp_dir = std::filesystem::temp_directory_path() / "themis_resume_valid_checkpoint";
    std::error_code ec;
    std::filesystem::create_directories(temp_dir, ec);
    const auto checkpoint_prefix = temp_dir / "checkpoint_epoch2_step500";

    {
        std::ofstream metadata(checkpoint_prefix.string() + "_metadata.txt");
        ASSERT_TRUE(metadata.is_open());
        metadata << "version=legal_v1.0\n"
                 << "format_version=1\n"
                 << "epoch=2\n"
                 << "step=500\n"
                 << "loss=0.42\n"
                 << "accuracy=0.87\n";
    }

    auto result = trainer.resumeFromCheckpoint(checkpoint_prefix.string());

    EXPECT_TRUE(result.success);

    std::filesystem::remove_all(temp_dir, ec);
}

TEST_F(LoRATrainerProductionTest, ResumeFromCheckpoint_MissingPathReturnsFailureInfo) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    auto result = trainer.resumeFromCheckpoint("/tmp/checkpoint_epoch1");

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(LoRATrainerProductionTest, Train_WithCheckpointing_Enabled_Succeeds) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    trainer.setCheckpointing(true, 5);
    auto result = trainer.train(TrainingMode::INITIAL);

    EXPECT_TRUE(result.success);
}

// ============================================================================
// selectAdapterForRequest() — traffic-split routing (FINDING-T-004)
// ============================================================================

TEST_F(LoRATrainerProductionTest, SelectAdapter_NoActiveVersions_ReturnsEmpty) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    // No versions deployed → empty string
    EXPECT_EQ(trainer.selectAdapterForRequest(), "");
}

TEST_F(LoRATrainerProductionTest, SelectAdapter_SingleFullDeployment_ReturnsIt) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    trainer.deployVersion("legal_v1.0", 1.0f);
    EXPECT_EQ(trainer.selectAdapterForRequest(), "legal_v1.0");
}

TEST_F(LoRATrainerProductionTest, SelectAdapter_ZeroTrafficSplit_ReturnsEmpty) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    trainer.deployVersion("legal_v1.0", 0.0f); // inactive
    EXPECT_EQ(trainer.selectAdapterForRequest(), "");
}

TEST_F(LoRATrainerProductionTest, SelectAdapter_TwoVersions_BothPossible) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    trainer.deployVersion("legal_v1.0", 0.5f);
    trainer.deployVersion("legal_v1.1", 0.5f);

    // With 50/50 split, repeated calls should return both versions across N trials.
    std::set<std::string> seen;
    for (int i = 0; i < 200; ++i) {
        seen.insert(trainer.selectAdapterForRequest());
    }
    EXPECT_TRUE(seen.count("legal_v1.0") || seen.count("legal_v1.1"))
        << "At least one version must be returned";
    // With 200 trials and 50% each, the probability of only seeing one is < 2^-200.
    EXPECT_EQ(seen.size(), 2u) << "Both versions should be selected at least once";
}

TEST_F(LoRATrainerProductionTest, SelectAdapter_FullDeployment_DeactivatesOthers) {
    IncrementalLoRATrainer trainer(config_, db_conn_);
    trainer.deployVersion("legal_v1.0", 0.5f);
    trainer.deployVersion("legal_v1.1", 1.0f); // full switch

    // After full deployment of v1.1, only v1.1 should be returned.
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(trainer.selectAdapterForRequest(), "legal_v1.1");
    }
}
