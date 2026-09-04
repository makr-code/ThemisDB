/**
 * @file test_advanced_training_features.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <gtest/gtest.h>

#include "training/incremental_lora_trainer.h"
#include "training/lora_checkpoint_manager.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

using namespace themis::training;

// ============================================================================
// QuantizationConfig tests
// ============================================================================

TEST(QuantizationConfig, DefaultIsNone) {
    QuantizationConfig cfg;
    EXPECT_EQ(cfg.type, TrainingQuantizationType::NONE);
    EXPECT_EQ(cfg.block_size, 64);
}

TEST(QuantizationConfig, ExplicitInt8) {
    QuantizationConfig cfg(TrainingQuantizationType::INT8, 128);
    EXPECT_EQ(cfg.type, TrainingQuantizationType::INT8);
    EXPECT_EQ(cfg.block_size, 128);
}

TEST(QuantizationConfig, ExplicitNF4) {
    QuantizationConfig cfg(TrainingQuantizationType::NF4, 32);
    EXPECT_EQ(cfg.type, TrainingQuantizationType::NF4);
    EXPECT_EQ(cfg.block_size, 32);
}

TEST(QuantizationConfig, ExplicitFP16) {
    QuantizationConfig cfg(TrainingQuantizationType::FP16);
    EXPECT_EQ(cfg.type, TrainingQuantizationType::FP16);
    EXPECT_EQ(cfg.block_size, 64);  // default block_size
}

// ============================================================================
// IncrementalTrainingConfig – new fields
// ============================================================================

TEST(IncrementalTrainingConfig, DefaultMultiGPUFields) {
    IncrementalTrainingConfig cfg;
    EXPECT_EQ(cfg.num_gpus, 1);
    EXPECT_TRUE(cfg.gpu_ids.empty());
    EXPECT_EQ(cfg.sync_steps, 1);
    EXPECT_EQ(cfg.quantization.type, TrainingQuantizationType::NONE);
    EXPECT_TRUE(cfg.checkpoint_dir.empty());
}

TEST(IncrementalTrainingConfig, MultiGPUFieldsRoundtrip) {
    IncrementalTrainingConfig cfg;
    cfg.num_gpus  = 4;
    cfg.gpu_ids   = {0, 1, 2, 3};
    cfg.sync_steps = 4;

    EXPECT_EQ(cfg.num_gpus, 4);
    EXPECT_EQ(cfg.gpu_ids.size(), 4u);
    EXPECT_EQ(cfg.sync_steps, 4);
}

TEST(IncrementalTrainingConfig, QuantizationFieldRoundtrip) {
    IncrementalTrainingConfig cfg;
    cfg.quantization = QuantizationConfig(TrainingQuantizationType::NF4, 64);

    EXPECT_EQ(cfg.quantization.type, TrainingQuantizationType::NF4);
    EXPECT_EQ(cfg.quantization.block_size, 64);
}

TEST(IncrementalTrainingConfig, CheckpointDirField) {
    IncrementalTrainingConfig cfg;
    cfg.checkpoint_dir = "/tmp/ckpt_test";
    EXPECT_EQ(cfg.checkpoint_dir, "/tmp/ckpt_test");
}

// ============================================================================
// TrainingResult – gpus_used field
// ============================================================================

TEST(TrainingResult, DefaultGpusUsed) {
    TrainingResult result;
    EXPECT_EQ(result.gpus_used, 1);
}

// ============================================================================
// TrainingMetrics / EpochMetrics – struct layout and reset
// ============================================================================

TEST(TrainingMetrics, DefaultValues) {
    TrainingMetrics m;
    EXPECT_TRUE(m.epoch_metrics.empty());
    EXPECT_TRUE(m.step_losses.empty());
    EXPECT_EQ(m.total_steps, 0u);
    EXPECT_EQ(m.total_epochs, 0u);
    EXPECT_EQ(m.total_elapsed_seconds, 0.0);
    // best_* are initialised to max so that any real value beats them
    EXPECT_GT(m.best_train_loss, 1e9);
    EXPECT_GT(m.best_val_loss, 1e9);
}

TEST(TrainingMetrics, Reset) {
    TrainingMetrics m;
    m.step_losses.push_back(1.2);
    m.total_steps = 5;
    m.best_train_loss = 0.5;

    m.reset();

    EXPECT_TRUE(m.step_losses.empty());
    EXPECT_EQ(m.total_steps, 0u);
    EXPECT_GT(m.best_train_loss, 1e9);
}

TEST(EpochMetrics, DefaultValues) {
    EpochMetrics em;
    EXPECT_EQ(em.epoch, 0u);
    EXPECT_EQ(em.steps, 0u);
    EXPECT_DOUBLE_EQ(em.train_loss, 0.0);
    EXPECT_DOUBLE_EQ(em.val_loss, 0.0);
    EXPECT_DOUBLE_EQ(em.accuracy, 0.0);
    EXPECT_DOUBLE_EQ(em.learning_rate, 0.0);
    EXPECT_DOUBLE_EQ(em.elapsed_seconds, 0.0);
}

// ============================================================================
// IncrementalLoRATrainer – getMetrics() after train()
// ============================================================================

TEST(IncrementalLoRATrainerMetrics, MetricsAccumulatedAfterTrain) {
    IncrementalTrainingConfig cfg;
    cfg.num_epochs    = 2;
    cfg.batch_size    = 2;
    cfg.learning_rate = 0.001f;
    cfg.rank          = 4;
    cfg.alpha         = 8.0f;
    cfg.device        = "cpu";

    IncrementalLoRATrainer trainer(cfg, "");
    auto result = trainer.train(TrainingMode::INITIAL);
    EXPECT_TRUE(result.success);

    auto metrics = trainer.getMetrics();

    // Two epochs should be recorded
    EXPECT_EQ(metrics.total_epochs, 2u);
    EXPECT_EQ(metrics.epoch_metrics.size(), 2u);

    // Step losses should be non-empty
    EXPECT_FALSE(metrics.step_losses.empty());
    EXPECT_EQ(metrics.total_steps, metrics.step_losses.size());

    // best_train_loss should be <= any individual epoch loss
    for (auto& em : metrics.epoch_metrics) {
        EXPECT_LE(metrics.best_train_loss, em.train_loss + 1e-9);
    }

    // total elapsed should be positive
    EXPECT_GT(metrics.total_elapsed_seconds, 0.0);
}

TEST(IncrementalLoRATrainerMetrics, EpochMetricsHaveCorrectEpochIndex) {
    IncrementalTrainingConfig cfg;
    cfg.num_epochs    = 3;
    cfg.batch_size    = 1;
    cfg.learning_rate = 0.001f;
    cfg.rank          = 4;
    cfg.alpha         = 8.0f;
    cfg.device        = "cpu";

    IncrementalLoRATrainer trainer(cfg, "");
    trainer.train(TrainingMode::INITIAL);

    auto metrics = trainer.getMetrics();
    ASSERT_EQ(metrics.epoch_metrics.size(), 3u);
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(metrics.epoch_metrics[i].epoch, i);
    }
}

TEST(IncrementalLoRATrainerMetrics, MetricsResetBetweenTrainCalls) {
    IncrementalTrainingConfig cfg;
    cfg.num_epochs    = 1;
    cfg.batch_size    = 1;
    cfg.learning_rate = 0.001f;
    cfg.rank          = 4;
    cfg.alpha         = 8.0f;
    cfg.device        = "cpu";

    IncrementalLoRATrainer trainer(cfg, "");

    trainer.train(TrainingMode::INITIAL);
    auto m1 = trainer.getMetrics();

    trainer.train(TrainingMode::INCREMENTAL);
    auto m2 = trainer.getMetrics();

    // Each call should reset metrics; m2 should cover exactly 1 epoch
    EXPECT_EQ(m2.total_epochs, 1u);
    EXPECT_EQ(m2.epoch_metrics.size(), 1u);
    // Step counts may differ between calls but should match total_steps
    EXPECT_EQ(m2.total_steps, m2.step_losses.size());
}

// ============================================================================
// IncrementalLoRATrainer – validation of new config fields
// ============================================================================

TEST(IncrementalLoRATrainerValidation, InvalidNumGpusThrows) {
    IncrementalTrainingConfig cfg;
    cfg.num_gpus      = 0;   // invalid
    cfg.learning_rate = 0.001f;
    cfg.rank          = 4;
    cfg.alpha         = 8.0f;
    cfg.batch_size    = 1;
    cfg.device        = "cpu";

    IncrementalLoRATrainer trainer(cfg, "");
    auto result = trainer.train(TrainingMode::INITIAL);
    // Should fail due to num_gpus == 0
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(IncrementalLoRATrainerValidation, InvalidSyncStepsThrows) {
    IncrementalTrainingConfig cfg;
    cfg.sync_steps    = 0;   // invalid
    cfg.learning_rate = 0.001f;
    cfg.rank          = 4;
    cfg.alpha         = 8.0f;
    cfg.batch_size    = 1;
    cfg.device        = "cpu";

    IncrementalLoRATrainer trainer(cfg, "");
    auto result = trainer.train(TrainingMode::INITIAL);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(IncrementalLoRATrainerValidation, InvalidBlockSizeThrows) {
    IncrementalTrainingConfig cfg;
    cfg.quantization.block_size = 0;  // invalid
    cfg.learning_rate = 0.001f;
    cfg.rank          = 4;
    cfg.alpha         = 8.0f;
    cfg.batch_size    = 1;
    cfg.device        = "cpu";

    IncrementalLoRATrainer trainer(cfg, "");
    auto result = trainer.train(TrainingMode::INITIAL);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(IncrementalLoRATrainerValidation, ValidQuantizationNoneDoesNotFail) {
    IncrementalTrainingConfig cfg;
    cfg.quantization  = QuantizationConfig(TrainingQuantizationType::NONE, 64);
    cfg.learning_rate = 0.001f;
    cfg.rank          = 4;
    cfg.alpha         = 8.0f;
    cfg.batch_size    = 1;
    cfg.num_epochs    = 1;
    cfg.device        = "cpu";

    IncrementalLoRATrainer trainer(cfg, "");
    auto result = trainer.train(TrainingMode::INITIAL);
    EXPECT_TRUE(result.success);
}

TEST(IncrementalLoRATrainerValidation, ValidQuantizationInt8DoesNotFail) {
    IncrementalTrainingConfig cfg;
    cfg.quantization  = QuantizationConfig(TrainingQuantizationType::INT8, 64);
    cfg.learning_rate = 0.001f;
    cfg.rank          = 4;
    cfg.alpha         = 8.0f;
    cfg.batch_size    = 1;
    cfg.num_epochs    = 1;
    cfg.device        = "cpu";

    IncrementalLoRATrainer trainer(cfg, "");
    auto result = trainer.train(TrainingMode::INITIAL);
    EXPECT_TRUE(result.success);
}

TEST(IncrementalLoRATrainerValidation, ValidQuantizationNF4DoesNotFail) {
    // NF4 (4-bit NormalFloat) triggers QLoRALayer path in the CPU training flow.
    IncrementalTrainingConfig cfg;
    cfg.quantization  = QuantizationConfig(TrainingQuantizationType::NF4, 64);
    cfg.learning_rate = 0.001f;
    cfg.rank          = 4;
    cfg.alpha         = 8.0f;
    cfg.batch_size    = 1;
    cfg.num_epochs    = 1;
    cfg.device        = "cpu";

    IncrementalLoRATrainer trainer(cfg, "");
    auto result = trainer.train(TrainingMode::INITIAL);
    EXPECT_TRUE(result.success);
}

TEST(IncrementalLoRATrainerValidation, ValidQuantizationFP16DoesNotFail) {
    IncrementalTrainingConfig cfg;
    cfg.quantization  = QuantizationConfig(TrainingQuantizationType::FP16, 64);
    cfg.learning_rate = 0.001f;
    cfg.rank          = 4;
    cfg.alpha         = 8.0f;
    cfg.batch_size    = 1;
    cfg.num_epochs    = 1;
    cfg.device        = "cpu";

    IncrementalLoRATrainer trainer(cfg, "");
    auto result = trainer.train(TrainingMode::INITIAL);
    EXPECT_TRUE(result.success);
}

TEST(IncrementalLoRATrainerValidation, NF4QuantizationProducesMetrics) {
    // Verify that NF4 QLoRA training accumulates metrics the same way as full-precision.
    IncrementalTrainingConfig cfg;
    cfg.quantization  = QuantizationConfig(TrainingQuantizationType::NF4, 64);
    cfg.learning_rate = 0.001f;
    cfg.rank          = 4;
    cfg.alpha         = 8.0f;
    cfg.batch_size    = 1;
    cfg.num_epochs    = 2;
    cfg.device        = "cpu";

    IncrementalLoRATrainer trainer(cfg, "");
    auto result = trainer.train(TrainingMode::INITIAL);
    EXPECT_TRUE(result.success);

    auto metrics = trainer.getMetrics();
    EXPECT_EQ(metrics.total_epochs, 2u);
    EXPECT_FALSE(metrics.step_losses.empty());
    EXPECT_EQ(metrics.total_steps, metrics.step_losses.size());
    EXPECT_GT(metrics.total_elapsed_seconds, 0.0);
}

// ============================================================================
// getMetrics – single-GPU path reports gpus_used == 1
// ============================================================================

TEST(IncrementalLoRATrainerMetrics, GpusUsedIsSingleGPU) {
    IncrementalTrainingConfig cfg;
    cfg.num_epochs    = 1;
    cfg.batch_size    = 1;
    cfg.learning_rate = 0.001f;
    cfg.rank          = 4;
    cfg.alpha         = 8.0f;
    cfg.device        = "cpu";

    IncrementalLoRATrainer trainer(cfg, "");
    auto result = trainer.train(TrainingMode::INITIAL);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.gpus_used, 1);
}

// ============================================================================
// Checkpoint/resume – train → resumeFromCheckpoint returns success
// ============================================================================

TEST(IncrementalLoRATrainerCheckpoint, ResumeFromEmptyPathFails) {
    IncrementalTrainingConfig cfg;
    cfg.num_epochs    = 1;
    cfg.batch_size    = 1;
    cfg.learning_rate = 0.001f;
    cfg.rank          = 4;
    cfg.alpha         = 8.0f;
    cfg.device        = "cpu";

    IncrementalLoRATrainer trainer(cfg, "");
    auto result = trainer.resumeFromCheckpoint("");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(IncrementalLoRATrainerCheckpoint, ResumeFromNonexistentPathFails) {
    IncrementalTrainingConfig cfg;
    cfg.num_epochs    = 1;
    cfg.batch_size    = 1;
    cfg.learning_rate = 0.001f;
    cfg.rank          = 4;
    cfg.alpha         = 8.0f;
    cfg.device        = "cpu";

    IncrementalLoRATrainer trainer(cfg, "");
    auto result = trainer.resumeFromCheckpoint("/nonexistent/path/checkpoint");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(IncrementalLoRATrainerCheckpoint, ResumeWithManagedCheckpointDirRequiresManifestIntegrity) {
    const std::string ckpt_dir = "/tmp/themis_trainer_integrity_guard";
    std::error_code ec = {};
    std::filesystem::create_directories(ckpt_dir, ec);

    IncrementalTrainingConfig cfg;
    cfg.num_epochs      = 1;
    cfg.batch_size      = 1;
    cfg.learning_rate   = 0.001f;
    cfg.rank            = 4;
    cfg.alpha           = 8.0f;
    cfg.device          = "cpu";
    cfg.checkpoint_dir  = ckpt_dir;

    IncrementalLoRATrainer trainer(cfg, "");
    auto result = trainer.resumeFromCheckpoint(ckpt_dir + "/unmanaged_checkpoint");

    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("integrity"), std::string::npos);

    std::filesystem::remove_all(ckpt_dir, ec);
}

// ============================================================================
// setCheckpointing + getMetrics: checkpoint steps collected
// ============================================================================

TEST(IncrementalLoRATrainerCheckpoint, CheckpointingEnabledDoesNotBreakMetrics) {
    IncrementalTrainingConfig cfg;
    cfg.num_epochs    = 2;
    cfg.batch_size    = 1;
    cfg.learning_rate = 0.001f;
    cfg.rank          = 4;
    cfg.alpha         = 8.0f;
    cfg.device        = "cpu";

    IncrementalLoRATrainer trainer(cfg, "");
    // Enable checkpointing with a very large step count so no actual file is
    // written during the (tiny) synthetic training run.
    trainer.setCheckpointing(true, 100000);
    auto result = trainer.train(TrainingMode::INITIAL);
    EXPECT_TRUE(result.success);

    auto metrics = trainer.getMetrics();
    EXPECT_EQ(metrics.total_epochs, 2u);
}
