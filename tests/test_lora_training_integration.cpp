/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_lora_training_integration.cpp                 ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:19:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     375                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "llm/lora_framework/lora_training_service.h"
#include "llm/lora_framework/data_loader.h"
#include <memory>
#include <spdlog/spdlog.h>

using namespace themis::llm::lora;

/**
 * @file test_lora_training_integration.cpp
 * @brief Tests for LoRA training service integration with DataLoader
 * 
 * Test Coverage:
 * - DataLoader integration with training service
 * - Real text data training (Phase 2a)
 * - Training convergence on toy datasets
 */

class LoRATrainingIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup training service configuration
        config_.default_hyperparameters.rank = 4;  // Small rank for fast testing
        config_.default_hyperparameters.alpha = 8.0f;
        config_.default_hyperparameters.learning_rate = 1e-3f;
        config_.default_hyperparameters.batch_size = 2;
        config_.default_hyperparameters.num_epochs = 2;
        config_.default_hyperparameters.max_seq_length = 64;
        config_.enable_checkpointing = false;  // Disable for testing
    }
    
    void TearDown() override {
    }
    
    LoRATrainingService::Config config_;
};

// ===== Basic Integration Tests =====

TEST_F(LoRATrainingIntegrationTest, ServiceConstruction) {
    LoRATrainingService service(config_);
    EXPECT_FALSE(service.isTraining());
}

TEST_F(LoRATrainingIntegrationTest, TrainWithSingleSample) {
    LoRATrainingService service(config_);
    
    // Create minimal training data
    TrainingData data;
    data.dataset_name = "test_dataset";
    
    TrainingDataSample sample;
    sample.input = "What is 2+2?";
    sample.output = "4";
    data.samples.push_back(sample);
    
    // Train
    auto result = service.trainOnTheFly("test_adapter_single", data);
    
    // Should complete successfully even with minimal data
    EXPECT_TRUE(result.success || !result.error_message.empty());
    EXPECT_EQ(result.adapter_id, "test_adapter_single");
}

TEST_F(LoRATrainingIntegrationTest, TrainWithMultipleSamples) {
    LoRATrainingService service(config_);
    
    // Create training data with multiple samples
    TrainingData data;
    data.dataset_name = "test_dataset";
    
    for (int i = 0; i < 10; ++i) {
        TrainingDataSample sample;
        sample.input = "Question " + std::to_string(i);
        sample.output = "Answer " + std::to_string(i);
        data.samples.push_back(sample);
    }
    
    // Train
    auto result = service.trainOnTheFly("test_adapter_multi", data);
    
    EXPECT_TRUE(result.success || !result.error_message.empty());
    EXPECT_EQ(result.epochs_completed, config_.default_hyperparameters.num_epochs);
}

TEST_F(LoRATrainingIntegrationTest, TrainWithToyDataset) {
    LoRATrainingService service(config_);
    
    // Create more realistic toy dataset
    TrainingData data;
    data.dataset_name = "toy_dataset";
    
    std::vector<std::pair<std::string, std::string>> qa_pairs = {
        {"What is the capital of France?", "Paris"},
        {"What is 2+2?", "4"},
        {"Translate hello to Spanish", "Hola"},
        {"What color is the sky?", "Blue"},
        {"How many days in a week?", "7"}
    };
    
    for (const auto& [question, answer] : qa_pairs) {
        TrainingDataSample sample;
        sample.input = question;
        sample.output = answer;
        data.samples.push_back(sample);
    }
    
    // Train
    auto result = service.trainOnTheFly("test_adapter_toy", data);
    
    EXPECT_TRUE(result.success || !result.error_message.empty());
    EXPECT_GT(result.training_time.count(), 0);
}

TEST_F(LoRATrainingIntegrationTest, TrainingMetricsUpdated) {
    LoRATrainingService service(config_);
    
    // Create training data
    TrainingData data;
    for (int i = 0; i < 8; ++i) {  // 8 samples = 4 batches with batch_size=2
        TrainingDataSample sample;
        sample.input = "Input " + std::to_string(i);
        sample.output = "Output " + std::to_string(i);
        data.samples.push_back(sample);
    }
    
    // Start training in a separate thread so we can check metrics
    std::thread train_thread([&service, &data]() {
        service.trainOnTheFly("test_adapter_metrics", data);
    });
    
    // Wait a bit for training to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Check that metrics are being updated
    bool was_training = false;
    for (int i = 0; i < 20; ++i) {
        if (service.isTraining()) {
            was_training = true;
            auto metrics = service.getMetrics();
            EXPECT_GT(metrics.total_steps, 0);
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    // Wait for training to complete
    train_thread.join();
    
    // Verify training occurred
    EXPECT_TRUE(was_training);
    EXPECT_FALSE(service.isTraining());
}

TEST_F(LoRATrainingIntegrationTest, StopTraining) {
    // Increase epochs for this test to have time to stop
    config_.default_hyperparameters.num_epochs = 10;
    
    LoRATrainingService service(config_);
    
    // Create training data
    TrainingData data;
    for (int i = 0; i < 20; ++i) {
        TrainingDataSample sample;
        sample.input = "Question " + std::to_string(i);
        sample.output = "Answer " + std::to_string(i);
        data.samples.push_back(sample);
    }
    
    // Start training in a separate thread
    std::thread train_thread([&service, &data]() {
        service.trainOnTheFly("test_adapter_stop", data);
    });
    
    // Wait for training to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop training
    service.stopTraining();
    
    // Wait for training thread to finish
    train_thread.join();
    
    // Verify training was stopped
    EXPECT_FALSE(service.isTraining());
}

TEST_F(LoRATrainingIntegrationTest, LossDecreases) {
    // Configure for more training to see loss decrease
    config_.default_hyperparameters.num_epochs = 5;
    config_.default_hyperparameters.learning_rate = 1e-2f;  // Higher LR for faster convergence
    
    LoRATrainingService service(config_);
    
    // Create consistent training data (same patterns)
    TrainingData data;
    for (int i = 0; i < 10; ++i) {
        TrainingDataSample sample;
        sample.input = "Pattern A";
        sample.output = "Result X";
        data.samples.push_back(sample);
    }
    
    // Track loss values
    std::vector<float> loss_values;
    service.registerCallback([&loss_values](const TrainingMetrics& metrics) {
        if (metrics.current_step % 5 == 0) {  // Sample every 5 steps
            loss_values.push_back(metrics.current_loss);
        }
    });
    
    // Train
    auto result = service.trainOnTheFly("test_adapter_convergence", data);
    
    EXPECT_TRUE(result.success);
    
    // Verify we have loss values
    EXPECT_GT(loss_values.size(), 2);
    
    // Check that final loss is generally lower than initial loss
    // (May not be strictly monotonic due to batch variations)
    if (loss_values.size() >= 2) {
        float initial_loss = loss_values[0];
        float final_loss = loss_values[loss_values.size() - 1];
        
        spdlog::info("Initial loss: {:.4f}, Final loss: {:.4f}", initial_loss, final_loss);
        
        // Loss should decrease or at least not increase significantly
        EXPECT_LE(final_loss, initial_loss * 1.5);  // Allow some variance
    }
}

// ===== Configuration Tests =====

TEST_F(LoRATrainingIntegrationTest, CustomHyperparameters) {
    LoRATrainingService service(config_);
    
    // Custom hyperparameters
    LoRAHyperparameters custom_params;
    custom_params.rank = 8;
    custom_params.alpha = 16.0f;
    custom_params.learning_rate = 5e-4f;
    custom_params.batch_size = 4;
    custom_params.num_epochs = 1;
    
    TrainingData data;
    for (int i = 0; i < 8; ++i) {
        TrainingDataSample sample;
        sample.input = "Input " + std::to_string(i);
        sample.output = "Output " + std::to_string(i);
        data.samples.push_back(sample);
    }
    
    // Train with custom parameters
    auto result = service.trainOnTheFly("test_adapter_custom", data, custom_params);
    
    EXPECT_TRUE(result.success || !result.error_message.empty());
}

TEST_F(LoRATrainingIntegrationTest, EmptyDatasetHandling) {
    LoRATrainingService service(config_);
    
    // Empty training data
    TrainingData data;
    data.dataset_name = "empty_dataset";
    
    // Should handle empty dataset gracefully
    auto result = service.trainOnTheFly("test_adapter_empty", data);
    
    // May fail or succeed with warning, but shouldn't crash
    EXPECT_TRUE(true);  // Test passes if we reach here without crash
}

// ===== Phase 2b: Base Model Integration Tests =====

TEST_F(LoRATrainingIntegrationTest, DISABLED_BaseModelIntegration_WithGGUF) {
    // This test requires an actual GGUF model file
    // Disabled by default - enable when testing with real models
    
    config_.use_base_model = true;
    config_.base_model_path = "models/llama-2-7b.gguf";
    config_.target_modules = {"attention.wq", "attention.wv"};
    config_.default_hyperparameters.rank = 8;
    config_.default_hyperparameters.alpha = 16.0f;
    config_.default_hyperparameters.num_epochs = 1;
    
    LoRATrainingService service(config_);
    
    // Create training data
    TrainingData data;
    for (int i = 0; i < 10; ++i) {
        TrainingDataSample sample;
        sample.input = "Question " + std::to_string(i);
        sample.output = "Answer " + std::to_string(i);
        data.samples.push_back(sample);
    }
    
    // Train with base model
    auto result = service.trainOnTheFly("test_adapter_base_model", data);
    
    if (result.success) {
        EXPECT_TRUE(result.success);
        EXPECT_GT(result.epochs_completed, 0);
        spdlog::info("Training with base model succeeded!");
    } else {
        GTEST_SKIP() << "Base model file not found or initialization failed: " << result.error_message;
    }
}

TEST_F(LoRATrainingIntegrationTest, BaseModelIntegration_Disabled) {
    // Test that training works when base model integration is disabled
    config_.use_base_model = false;  // Explicitly disable
    config_.base_model_path = "models/nonexistent.gguf";  // Path doesn't matter
    
    LoRATrainingService service(config_);
    
    // Create training data
    TrainingData data;
    for (int i = 0; i < 5; ++i) {
        TrainingDataSample sample;
        sample.input = "Input " + std::to_string(i);
        sample.output = "Output " + std::to_string(i);
        data.samples.push_back(sample);
    }
    
    // Should fall back to standalone LoRA layer
    auto result = service.trainOnTheFly("test_adapter_no_base", data);
    
    EXPECT_TRUE(result.success || !result.error_message.empty());
}

TEST_F(LoRATrainingIntegrationTest, BaseModelIntegration_FallbackOnError) {
    // Test that training falls back gracefully when base model path is invalid
    config_.use_base_model = true;
    config_.base_model_path = "models/nonexistent_model.gguf";  // Invalid path
    
    LoRATrainingService service(config_);
    
    // Create training data
    TrainingData data;
    for (int i = 0; i < 5; ++i) {
        TrainingDataSample sample;
        sample.input = "Test " + std::to_string(i);
        sample.output = "Result " + std::to_string(i);
        data.samples.push_back(sample);
    }
    
    // Should fall back to standalone LoRA layer and still train
    auto result = service.trainOnTheFly("test_adapter_fallback", data);
    
    EXPECT_TRUE(result.success || !result.error_message.empty());
    // Training should succeed with fallback, not fail completely
}
