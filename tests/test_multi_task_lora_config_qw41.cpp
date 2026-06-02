/**
 * @file test_multi_task_lora_config_qw41.cpp
 * @brief QW-41: MultiTaskLoRATrainer configuration hardening
 *
 * Tests for LoRA model configuration security hardening.
 * Validates that invalid configurations are rejected with fail-closed guards.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "training/multi_task_lora.h"

namespace themis {
namespace {

/**
 * @class MultiTaskLoRAConfigTest
 * @brief Test fixture for LoRA configuration hardening (QW-41)
 */
class MultiTaskLoRAConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize with valid defaults
        valid_config_.shared_rank = 8;
        valid_config_.epochs = 20;
        valid_config_.batch_size = 32;
        valid_config_.learning_rate = 1e-3f;
        valid_config_.warmup_frac = 0.1f;
        valid_config_.input_dim = 0;
        valid_config_.gating_fallback_threshold = 0.2f;
    }
    
    void TearDown() override {
        // Cleanup
    }
    
    MultiTaskLoRAConfig valid_config_;
};

/**
 * @test ConfigHardening_TaskIdEmpty_Rejected
 * @brief Guard: TaskConfig.id cannot be empty (fail-closed)
 */
TEST_F(MultiTaskLoRAConfigTest, ConfigHardening_TaskIdEmpty_Rejected) {
    MultiTaskLoRATrainer trainer(valid_config_);
    
    TaskConfig task;
    task.id = "";  // Fail-closed guard: empty task id
    task.task_rank = 4;
    task.learning_rate = 1e-3f;
    task.loss_weight = 1.0f;
    
    // Expected: addTask throws std::invalid_argument
    EXPECT_THROW(trainer.addTask(task), std::invalid_argument);
}

/**
 * @test ConfigHardening_TaskRankZero_Rejected
 * @brief Guard: TaskConfig.task_rank must be >= 1 (fail-closed)
 */
TEST_F(MultiTaskLoRAConfigTest, ConfigHardening_TaskRankZero_Rejected) {
    MultiTaskLoRATrainer trainer(valid_config_);
    
    TaskConfig task;
    task.id = "task1";
    task.task_rank = 0;  // Fail-closed guard: rank must be > 0
    task.learning_rate = 1e-3f;
    task.loss_weight = 1.0f;
    
    EXPECT_THROW(trainer.addTask(task), std::invalid_argument);
}

/**
 * @test ConfigHardening_LearningRateNegative_Rejected
 * @brief Guard: TaskConfig.learning_rate must be > 0 (fail-closed)
 */
TEST_F(MultiTaskLoRAConfigTest, ConfigHardening_LearningRateNegative_Rejected) {
    MultiTaskLoRATrainer trainer(valid_config_);
    
    TaskConfig task;
    task.id = "task1";
    task.task_rank = 4;
    task.learning_rate = -1e-3f;  // Fail-closed guard: LR must be > 0
    task.loss_weight = 1.0f;
    
    EXPECT_THROW(trainer.addTask(task), std::invalid_argument);
}

/**
 * @test ConfigHardening_LearningRateZero_Rejected
 * @brief Guard: TaskConfig.learning_rate must be strictly > 0 (fail-closed)
 */
TEST_F(MultiTaskLoRAConfigTest, ConfigHardening_LearningRateZero_Rejected) {
    MultiTaskLoRATrainer trainer(valid_config_);
    
    TaskConfig task;
    task.id = "task1";
    task.task_rank = 4;
    task.learning_rate = 0.0f;  // Fail-closed guard: LR must be > 0
    task.loss_weight = 1.0f;
    
    EXPECT_THROW(trainer.addTask(task), std::invalid_argument);
}

/**
 * @test ConfigHardening_LossWeightNegative_Rejected
 * @brief Guard: TaskConfig.loss_weight must be >= 0 (fail-closed)
 */
TEST_F(MultiTaskLoRAConfigTest, ConfigHardening_LossWeightNegative_Rejected) {
    MultiTaskLoRATrainer trainer(valid_config_);
    
    TaskConfig task;
    task.id = "task1";
    task.task_rank = 4;
    task.learning_rate = 1e-3f;
    task.loss_weight = -0.1f;  // Fail-closed guard: loss weight must be >= 0
    
    EXPECT_THROW(trainer.addTask(task), std::invalid_argument);
}

/**
 * @test ConfigHardening_ValidTaskAccepted
 * @brief Verify valid TaskConfig passes all guards
 */
TEST_F(MultiTaskLoRAConfigTest, ConfigHardening_ValidTaskAccepted) {
    MultiTaskLoRATrainer trainer(valid_config_);
    
    TaskConfig task;
    task.id = "task_valid";
    task.task_rank = 8;
    task.learning_rate = 1e-3f;
    task.loss_weight = 1.0f;
    
    // Expected: No exception, task is added
    EXPECT_NO_THROW(trainer.addTask(task));
    EXPECT_EQ(trainer.taskCount(), 1);
}

/**
 * @test ConfigHardening_MultipleValidTasks
 * @brief Verify multiple valid tasks can be added
 */
TEST_F(MultiTaskLoRAConfigTest, ConfigHardening_MultipleValidTasks) {
    MultiTaskLoRATrainer trainer(valid_config_);
    
    for (int i = 0; i < 5; ++i) {
        TaskConfig task;
        task.id = "task_" + std::to_string(i);
        task.task_rank = 4 + i;
        task.learning_rate = 1e-3f;
        task.loss_weight = 1.0f + i * 0.1f;
        
        EXPECT_NO_THROW(trainer.addTask(task));
    }
    
    EXPECT_EQ(trainer.taskCount(), 5);
}

/**
 * @test ConfigHardening_DuplicateTaskIdIgnored
 * @brief Verify duplicate task IDs are silently ignored (idempotent)
 */
TEST_F(MultiTaskLoRAConfigTest, ConfigHardening_DuplicateTaskIdIgnored) {
    MultiTaskLoRATrainer trainer(valid_config_);
    
    TaskConfig task;
    task.id = "task1";
    task.task_rank = 4;
    task.learning_rate = 1e-3f;
    task.loss_weight = 1.0f;
    
    EXPECT_NO_THROW(trainer.addTask(task));
    EXPECT_EQ(trainer.taskCount(), 1);
    
    // Add same task again
    EXPECT_NO_THROW(trainer.addTask(task));
    EXPECT_EQ(trainer.taskCount(), 1);  // Still 1, duplicate ignored
}

/**
 * @test ConfigHardening_SharedRankRange
 * @brief Verify MultiTaskLoRAConfig shared_rank is reasonable
 */
TEST_F(MultiTaskLoRAConfigTest, ConfigHardening_SharedRankRange) {
    // Valid shared_rank (should be > 0 and typically small)
    MultiTaskLoRAConfig cfg;
    cfg.shared_rank = 8;
    cfg.epochs = 10;
    cfg.batch_size = 16;
    cfg.learning_rate = 1e-3f;
    
    // Expected: Valid configuration
    EXPECT_NO_THROW(MultiTaskLoRATrainer trainer(cfg));
}

/**
 * @test ConfigHardening_BoundaryCase_MinimalValidConfig
 * @brief Verify minimal valid configuration (rank=1, batch_size=1, lr=smallest positive)
 */
TEST_F(MultiTaskLoRAConfigTest, ConfigHardening_BoundaryCase_MinimalValidConfig) {
    MultiTaskLoRATrainer trainer(valid_config_);
    
    TaskConfig task;
    task.id = "minimal";
    task.task_rank = 1;  // Minimum valid rank
    task.learning_rate = 1e-6f;  // Very small but positive
    task.loss_weight = 0.0f;  // Minimum valid loss weight
    
    EXPECT_NO_THROW(trainer.addTask(task));
}

/**
 * @test ConfigHardening_FailClosedOnInvalid
 * @brief Verify fail-closed behavior: invalid config is rejected, trainer remains usable
 */
TEST_F(MultiTaskLoRAConfigTest, ConfigHardening_FailClosedOnInvalid) {
    MultiTaskLoRATrainer trainer(valid_config_);
    
    // Add valid task
    TaskConfig valid_task;
    valid_task.id = "valid";
    valid_task.task_rank = 4;
    valid_task.learning_rate = 1e-3f;
    EXPECT_NO_THROW(trainer.addTask(valid_task));
    
    // Attempt invalid task
    TaskConfig invalid_task;
    invalid_task.id = "";  // Invalid: empty id
    invalid_task.task_rank = 4;
    invalid_task.learning_rate = 1e-3f;
    EXPECT_THROW(trainer.addTask(invalid_task), std::invalid_argument);
    
    // Verify valid task is still there (fail-closed: trainer not corrupted)
    EXPECT_EQ(trainer.taskCount(), 1);
    
    // Can still add other valid tasks
    TaskConfig another_task;
    another_task.id = "another";
    another_task.task_rank = 4;
    another_task.learning_rate = 1e-3f;
    EXPECT_NO_THROW(trainer.addTask(another_task));
    EXPECT_EQ(trainer.taskCount(), 2);
}

/**
 * @test ConfigHardening_ErrorMessageClarity
 * @brief Verify error messages clearly indicate which guard failed
 */
TEST_F(MultiTaskLoRAConfigTest, ConfigHardening_ErrorMessageClarity) {
    MultiTaskLoRATrainer trainer(valid_config_);
    
    TaskConfig task;
    task.id = "test";
    task.task_rank = 0;  // Invalid rank
    task.learning_rate = 1e-3f;
    
    try {
        trainer.addTask(task);
        FAIL() << "Expected std::invalid_argument";
    } catch (const std::invalid_argument& e) {
        // Verify error message mentions task_rank
        std::string err_msg(e.what());
        EXPECT_TRUE(err_msg.find("task_rank") != std::string::npos ||
                    err_msg.find("rank") != std::string::npos);
    }
}

}  // namespace
}  // namespace themis

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
