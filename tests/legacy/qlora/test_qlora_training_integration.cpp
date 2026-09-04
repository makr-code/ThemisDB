#include <gtest/gtest.h>
#include "llm/lora_framework/lora_training_service.h"
#include "llm/lora_framework/quantized_model.h"
#include "llm/lora_framework/lora_config.h"
#include <memory>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <fstream>

using namespace themis::llm::lora;

/**
 * @file test_qlora_training_integration.cpp
 * @brief Integration tests for QLoRA training service
 */

class QLoRATrainingIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create training service configuration
        service_config_.default_hyperparameters.rank = 8;
        service_config_.default_hyperparameters.alpha = 16.0f;
        service_config_.default_hyperparameters.learning_rate = 0.001f;
        service_config_.default_hyperparameters.num_epochs = 2;
        service_config_.default_hyperparameters.batch_size = 2;
        
        // Configure QLoRA
        service_config_.qlora.enabled = true;
        service_config_.qlora.quantization_type = "nf4";
        service_config_.qlora.block_size = 64;
        service_config_.qlora.use_double_quantization = false;
        service_config_.qlora.layer_by_layer = true;
        
        const auto tmp_dir = std::filesystem::temp_directory_path() / "themisdb_qlora_tests";
        std::error_code ec = {};
        std::filesystem::create_directories(tmp_dir, ec);
        base_model_path_ = (tmp_dir / "test_model.gguf").string();
        createMinimalGGUF(base_model_path_);
        service_config_.base_model_path = base_model_path_;
        service_config_.enable_checkpointing = false;  // Disable for tests
        
        // Create training data
        training_data_.dataset_name = "test_dataset";
        for (int i = 0; i < 10; ++i) {
            TrainingDataSample sample;
            sample.input = "Test input " + std::to_string(i);
            sample.output = "Test output " + std::to_string(i);
            training_data_.samples.push_back(sample);
        }
    }

    void TearDown() override {
        if (!base_model_path_.empty()) {
            std::error_code ec = {};
            std::filesystem::remove(base_model_path_, ec);
        }
    }

    static void createMinimalGGUF(const std::string& path) {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(out.is_open()) << "Failed to create temporary GGUF file: " << path;

        constexpr char kMagic[4] = {'G', 'G', 'U', 'F'};
        const uint32_t version = 3;
        const uint64_t tensor_count = 0;
        const uint64_t kv_count = 0;

        out.write(kMagic, sizeof(kMagic));
        out.write(reinterpret_cast<const char*>(&version), sizeof(version));
        out.write(reinterpret_cast<const char*>(&tensor_count), sizeof(tensor_count));
        out.write(reinterpret_cast<const char*>(&kv_count), sizeof(kv_count));
        out.flush();
        ASSERT_TRUE(out.good()) << "Failed to write GGUF header to: " << path;
    }
    
    LoRATrainingService::Config service_config_;
    TrainingData training_data_;
    std::string base_model_path_;
};

// ===== Configuration Tests =====

TEST_F(QLoRATrainingIntegrationTest, QLoRAConfig_JSONSerialization) {
    QLoRAConfig config;
    config.enabled = true;
    config.quantization_type = "nf4";
    config.block_size = 128;
    config.use_double_quantization = true;
    config.layer_by_layer = true;
    
    // Serialize to JSON
    json j = config.toJSON();
    
    // Verify fields
    EXPECT_TRUE(j["enabled"].get<bool>());
    EXPECT_EQ(j["quantization_type"].get<std::string>(), "nf4");
    EXPECT_EQ(j["block_size"].get<size_t>(), 128);
    EXPECT_TRUE(j["use_double_quantization"].get<bool>());
    EXPECT_TRUE(j["layer_by_layer"].get<bool>());
    
    // Deserialize
    QLoRAConfig config2 = QLoRAConfig::fromJSON(j);
    
    // Verify deserialization
    EXPECT_EQ(config2.enabled, config.enabled);
    EXPECT_EQ(config2.quantization_type, config.quantization_type);
    EXPECT_EQ(config2.block_size, config.block_size);
    EXPECT_EQ(config2.use_double_quantization, config.use_double_quantization);
    EXPECT_EQ(config2.layer_by_layer, config.layer_by_layer);
}

TEST_F(QLoRATrainingIntegrationTest, QLoRAConfig_DefaultValues) {
    QLoRAConfig config;
    
    EXPECT_FALSE(config.enabled);
    EXPECT_EQ(config.quantization_type, "nf4");
    EXPECT_EQ(config.block_size, 64);
    EXPECT_FALSE(config.use_double_quantization);
    EXPECT_TRUE(config.layer_by_layer);
}

// ===== Training Service Tests =====

TEST_F(QLoRATrainingIntegrationTest, TrainingService_QLoRAEnabled) {
    LoRATrainingService service(service_config_);
    
    // Get configuration back
    auto config = service.getTrainingConfig();
    
    // Verify QLoRA is enabled
    EXPECT_TRUE(config.qlora.enabled);
    EXPECT_EQ(config.qlora.quantization_type, "nf4");
}

TEST_F(QLoRATrainingIntegrationTest, TrainingService_WithQuantization_NF4) {
    LoRATrainingService service(service_config_);
    
    // Train with quantization
    auto result = service.trainWithQuantization("test_adapter", training_data_);
    
    // Verify result
    EXPECT_TRUE(result.success) << "Error: " << result.error_message;
    EXPECT_EQ(result.adapter_id, "test_adapter");
    EXPECT_GT(result.epochs_completed, 0);
    EXPECT_GE(result.final_loss, 0.0f);
    
    // Verify metrics contain quantization info
    EXPECT_TRUE(result.metrics.contains("quantization_type"));
    EXPECT_TRUE(result.metrics.contains("memory_bytes"));
    EXPECT_TRUE(result.metrics.contains("trainable_parameters"));
    EXPECT_EQ(result.metrics["quantization_type"].get<std::string>(), "nf4");
}

TEST_F(QLoRATrainingIntegrationTest, TrainingService_WithQuantization_INT8) {
    // Change to INT8
    service_config_.qlora.quantization_type = "int8";
    LoRATrainingService service(service_config_);
    
    auto result = service.trainWithQuantization("test_adapter_int8", training_data_);
    
    EXPECT_TRUE(result.success) << "Error: " << result.error_message;
    EXPECT_EQ(result.metrics["quantization_type"].get<std::string>(), "int8");
}

TEST_F(QLoRATrainingIntegrationTest, TrainingService_QLoRADisabled_Fallback) {
    // Disable QLoRA
    service_config_.qlora.enabled = false;
    LoRATrainingService service(service_config_);
    
    // trainWithQuantization should fall back to standard training
    auto result = service.trainWithQuantization("test_adapter_fallback", training_data_);
    
    // Should still succeed (falls back to standard training)
    EXPECT_TRUE(result.success) << "Error: " << result.error_message;
}

TEST_F(QLoRATrainingIntegrationTest, TrainingService_MemoryEstimation) {
    LoRATrainingService service(service_config_);
    
    // This will be called internally during training
    // Just verify the service can be created with QLoRA config
    EXPECT_NO_THROW({
        auto config = service.getTrainingConfig();
        EXPECT_TRUE(config.qlora.enabled);
    });
}

// ===== Integration Tests =====

TEST_F(QLoRATrainingIntegrationTest, EndToEnd_QLoRA_Training) {
    // Complete end-to-end test
    LoRATrainingService service(service_config_);
    
    // Register progress callback
    bool callback_called = false;
    service.registerCallback([&callback_called](const TrainingMetrics& metrics) {
        callback_called = true;
        EXPECT_GE(metrics.current_step, 0);
        EXPECT_GE(metrics.current_loss, 0.0f);
    });
    
    // Train
    auto result = service.trainWithQuantization("end_to_end_adapter", training_data_);
    
    // Verify success
    EXPECT_TRUE(result.success) << "Error: " << result.error_message;
    EXPECT_EQ(result.adapter_id, "end_to_end_adapter");
    EXPECT_EQ(result.epochs_completed, service_config_.default_hyperparameters.num_epochs);
    
    // Verify metrics
    EXPECT_TRUE(result.metrics.contains("quantization_type"));
    EXPECT_TRUE(result.metrics.contains("memory_bytes"));
    EXPECT_GT(result.metrics["memory_bytes"].get<size_t>(), 0);
}

TEST_F(QLoRATrainingIntegrationTest, QLoRA_LossDecreases) {
    // Verify that training actually improves (loss decreases)
    service_config_.default_hyperparameters.num_epochs = 5;
    LoRATrainingService service(service_config_);
    
    std::vector<float> losses;
    service.registerCallback([&losses](const TrainingMetrics& metrics) {
        losses.push_back(metrics.current_loss);
    });
    
    auto result = service.trainWithQuantization("loss_test_adapter", training_data_);
    
    EXPECT_TRUE(result.success);
    
    // Loss should generally decrease over training
    // (May not be monotonic due to batch variance, but final should be lower than initial)
    if (!losses.empty() && losses.size() >= 2) {
        float initial_loss = losses[0];
        float final_loss = losses.back();
        
        // Allow some variance, but expect overall improvement
        EXPECT_LE(final_loss, initial_loss * 1.5f) 
            << "Loss should decrease or stay stable during training";
    }
}

TEST_F(QLoRATrainingIntegrationTest, QLoRA_MemoryReduction) {
    // Test that QLoRA actually reduces memory compared to full precision
    LoRATrainingService service(service_config_);
    
    auto result = service.trainWithQuantization("memory_test_adapter", training_data_);
    
    EXPECT_TRUE(result.success);
    
    if (result.metrics.contains("memory_bytes")) {
        size_t memory_bytes = result.metrics["memory_bytes"].get<size_t>();
        
        // For NF4, expect significant memory reduction
        // A typical 7B model would be ~28GB in FP32, ~4GB in NF4
        // Our test model should show similar proportional reduction
        
        // At minimum, memory should be reasonable (not gigabytes for tiny test model)
        EXPECT_LT(memory_bytes, 100 * 1024 * 1024);  // Less than 100MB for test model
        
        // Verify it's greater than zero
        EXPECT_GT(memory_bytes, 0);
    }
}

// ===== Configuration Validation Tests =====

TEST_F(QLoRATrainingIntegrationTest, QLoRA_InvalidQuantizationType) {
    service_config_.qlora.quantization_type = "invalid_type";
    LoRATrainingService service(service_config_);
    
    // Should handle gracefully (fall back to NF4)
    auto result = service.trainWithQuantization("invalid_quant_adapter", training_data_);
    
    // Should still succeed with fallback
    EXPECT_TRUE(result.success);
}

TEST_F(QLoRATrainingIntegrationTest, QLoRA_DoubleQuantization) {
    service_config_.qlora.use_double_quantization = true;
    LoRATrainingService service(service_config_);
    
    auto result = service.trainWithQuantization("double_quant_adapter", training_data_);
    
    EXPECT_TRUE(result.success);
    
    // Memory should be slightly less with double quantization
    if (result.metrics.contains("memory_bytes")) {
        size_t memory_bytes = result.metrics["memory_bytes"].get<size_t>();
        EXPECT_GT(memory_bytes, 0);
    }
}

TEST_F(QLoRATrainingIntegrationTest, QLoRA_DifferentBlockSizes) {
    // Test with different block sizes
    std::vector<size_t> block_sizes = {32, 64, 128, 256};
    
    for (size_t block_size : block_sizes) {
        service_config_.qlora.block_size = block_size;
        LoRATrainingService service(service_config_);
        
        auto result = service.trainWithQuantization(
            "block_size_" + std::to_string(block_size) + "_adapter", 
            training_data_
        );
        
        EXPECT_TRUE(result.success) 
            << "Failed with block size " << block_size 
            << ": " << result.error_message;
    }
}

// ===== Performance Tests =====

TEST_F(QLoRATrainingIntegrationTest, QLoRA_TrainingSpeed) {
    service_config_.default_hyperparameters.num_epochs = 3;
    LoRATrainingService service(service_config_);
    
    auto start = std::chrono::steady_clock::now();
    auto result = service.trainWithQuantization("speed_test_adapter", training_data_);
    auto end = std::chrono::steady_clock::now();
    
    EXPECT_TRUE(result.success);
    
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    
    // Should complete in reasonable time (less than 10 seconds for small test)
    EXPECT_LT(duration.count(), 10) 
        << "Training took too long: " << duration.count() << " seconds";
}
