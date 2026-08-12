#include <gtest/gtest.h>
#include "llm/lora_framework/gradient_checkpointing.h"
#include "llm/lora_framework/gpu_lora_layers.h"
#include "llm/lora_framework/gpu_training_loop.h"
#include "llm/lora_framework/gpu_data_loader.h"
#include <spdlog/spdlog.h>

using namespace themis::llm::lora;

/**
 * @brief Test suite for gradient checkpointing
 * 
 * Tests the gradient checkpointing implementation for:
 * - Checkpoint strategy selection
 * - Memory savings estimation
 * - Compute overhead estimation
 * - Integration with GPU LoRA layers
 * - Integration with training loop
 */
class GradientCheckpointingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set log level to info for testing
        spdlog::set_level(spdlog::level::info);
    }
};

// ============================================================================
// Test Checkpoint Strategy Selection
// ============================================================================

TEST_F(GradientCheckpointingTest, StrategyNone) {
    CheckpointConfig config;
    config.strategy = CheckpointStrategy::NONE;
    config.total_layers = 32;
    
    GradientCheckpointer checkpointer(config);
    
    // No layers should be checkpointed
    for (int i = 0; i < 32; ++i) {
        EXPECT_FALSE(checkpointer.shouldCheckpoint(i));
    }
}

TEST_F(GradientCheckpointingTest, StrategyUniform) {
    CheckpointConfig config;
    config.strategy = CheckpointStrategy::UNIFORM;
    config.checkpoint_frequency = 4;
    config.total_layers = 32;
    
    GradientCheckpointer checkpointer(config);
    
    // Checkpoint every 4th layer: 0, 4, 8, 12, 16, 20, 24, 28
    EXPECT_TRUE(checkpointer.shouldCheckpoint(0));
    EXPECT_FALSE(checkpointer.shouldCheckpoint(1));
    EXPECT_FALSE(checkpointer.shouldCheckpoint(2));
    EXPECT_FALSE(checkpointer.shouldCheckpoint(3));
    EXPECT_TRUE(checkpointer.shouldCheckpoint(4));
    EXPECT_TRUE(checkpointer.shouldCheckpoint(8));
    EXPECT_TRUE(checkpointer.shouldCheckpoint(12));
    EXPECT_FALSE(checkpointer.shouldCheckpoint(13));
}

TEST_F(GradientCheckpointingTest, StrategySqrtN) {
    CheckpointConfig config;
    config.strategy = CheckpointStrategy::SQRT_N;
    config.total_layers = 36;  // sqrt(36) = 6
    
    GradientCheckpointer checkpointer(config);
    
    // Checkpoint every 6th layer: 0, 6, 12, 18, 24, 30
    EXPECT_TRUE(checkpointer.shouldCheckpoint(0));
    EXPECT_FALSE(checkpointer.shouldCheckpoint(1));
    EXPECT_FALSE(checkpointer.shouldCheckpoint(5));
    EXPECT_TRUE(checkpointer.shouldCheckpoint(6));
    EXPECT_TRUE(checkpointer.shouldCheckpoint(12));
    EXPECT_TRUE(checkpointer.shouldCheckpoint(18));
    EXPECT_TRUE(checkpointer.shouldCheckpoint(24));
    EXPECT_TRUE(checkpointer.shouldCheckpoint(30));
    EXPECT_FALSE(checkpointer.shouldCheckpoint(31));
}

TEST_F(GradientCheckpointingTest, StrategyAttentionOnly) {
    CheckpointConfig config;
    config.strategy = CheckpointStrategy::ATTENTION_ONLY;
    config.total_layers = 32;
    
    GradientCheckpointer checkpointer(config);
    
    // Set layer types
    checkpointer.setLayerType(0, LayerType::ATTENTION);
    checkpointer.setLayerType(1, LayerType::FFN);
    checkpointer.setLayerType(2, LayerType::ATTENTION);
    checkpointer.setLayerType(3, LayerType::LORA);
    
    // Only attention layers should be checkpointed
    EXPECT_TRUE(checkpointer.shouldCheckpoint(0));
    EXPECT_FALSE(checkpointer.shouldCheckpoint(1));
    EXPECT_TRUE(checkpointer.shouldCheckpoint(2));
    EXPECT_FALSE(checkpointer.shouldCheckpoint(3));
}

TEST_F(GradientCheckpointingTest, StrategyCustom) {
    CheckpointConfig config;
    config.strategy = CheckpointStrategy::CUSTOM;
    config.total_layers = 32;
    
    GradientCheckpointer checkpointer(config);
    
    // Add custom checkpoints
    checkpointer.addCustomCheckpoint(5);
    checkpointer.addCustomCheckpoint(10);
    checkpointer.addCustomCheckpoint(20);
    
    // Only custom layers should be checkpointed
    EXPECT_FALSE(checkpointer.shouldCheckpoint(0));
    EXPECT_TRUE(checkpointer.shouldCheckpoint(5));
    EXPECT_TRUE(checkpointer.shouldCheckpoint(10));
    EXPECT_FALSE(checkpointer.shouldCheckpoint(15));
    EXPECT_TRUE(checkpointer.shouldCheckpoint(20));
}

// ============================================================================
// Test Memory and Compute Estimation
// ============================================================================

TEST_F(GradientCheckpointingTest, MemorySavingsEstimation) {
    CheckpointConfig config;
    config.strategy = CheckpointStrategy::SQRT_N;
    config.total_layers = 36;  // sqrt(36) = 6, so 6 checkpoints
    
    GradientCheckpointer checkpointer(config);
    
    size_t avg_activation_size = 4 * 1024 * 1024;  // 4MB per layer
    size_t estimated_savings = checkpointer.estimateMemorySavings(avg_activation_size);
    
    // Total memory without checkpointing: 36 layers * 4MB = 144MB
    // Memory with checkpointing: 6 checkpoints * 4MB = 24MB
    // Savings: 144MB - 24MB = 120MB
    size_t expected_savings = (36 - 6) * avg_activation_size;
    
    EXPECT_EQ(estimated_savings, expected_savings);
    
    // Calculate percentage
    float memory_reduction_pct = 100.0f * static_cast<float>(estimated_savings) / 
                                 static_cast<float>(36 * avg_activation_size);
    
    // Should be approximately 83.3% (30/36)
    EXPECT_GT(memory_reduction_pct, 80.0f);
    EXPECT_LT(memory_reduction_pct, 85.0f);
}

TEST_F(GradientCheckpointingTest, ComputeOverheadEstimation) {
    CheckpointConfig config;
    config.strategy = CheckpointStrategy::SQRT_N;
    config.total_layers = 36;
    
    GradientCheckpointer checkpointer(config);
    
    float compute_overhead = checkpointer.estimateComputeOverhead();
    
    // For SQRT_N with 36 layers: 6 checkpoints / 36 total = 16.7% recompute fraction
    // Expected overhead: 16.7% * 25% (SQRT_N efficiency) ≈ 4.2%
    EXPECT_GT(compute_overhead, 0.0f);
    EXPECT_LT(compute_overhead, 10.0f);  // Should be less than 10%
}

// ============================================================================
// Test Checkpoint Save/Restore
// ============================================================================

TEST_F(GradientCheckpointingTest, SaveAndRecomputeCheckpoint) {
    CheckpointConfig config;
    config.strategy = CheckpointStrategy::SQRT_N;
    config.total_layers = 16;
    
    GradientCheckpointer checkpointer(config);
    
    // Create a simple input tensor
    Device device = Device::cpu();
    GPUTensor input({2, 4}, device);  // batch_size=2, dim=4
    std::vector<float> input_data = {1.0f, 2.0f, 3.0f, 4.0f,
                                      5.0f, 6.0f, 7.0f, 8.0f};
    input.upload(input_data);
    
    // Define a simple forward function (multiply by 2)
    auto forward_fn = [](const GPUTensor& x) -> GPUTensor {
        auto result = x.clone();
        result = result * 2.0f;
        return result;
    };
    
    // Save checkpoint
    int layer_id = 0;
    checkpointer.saveCheckpoint(layer_id, input, forward_fn);
    
    EXPECT_TRUE(checkpointer.hasCheckpoint(layer_id));
    
    // Recompute
    GPUTensor recomputed = checkpointer.recomputeActivation(layer_id);
    
    // Verify recomputed values (should be input * 2)
    auto recomputed_data = recomputed.cpu_data();
    EXPECT_EQ(recomputed_data.size(), 8);
    EXPECT_FLOAT_EQ(recomputed_data[0], 2.0f);
    EXPECT_FLOAT_EQ(recomputed_data[1], 4.0f);
    EXPECT_FLOAT_EQ(recomputed_data[2], 6.0f);
    EXPECT_FLOAT_EQ(recomputed_data[3], 8.0f);
}

TEST_F(GradientCheckpointingTest, ClearCheckpoint) {
    CheckpointConfig config;
    config.strategy = CheckpointStrategy::SQRT_N;
    config.total_layers = 16;
    
    GradientCheckpointer checkpointer(config);
    
    Device device = Device::cpu();
    GPUTensor input({2, 4}, device);
    auto forward_fn = [](const GPUTensor& x) -> GPUTensor { return x.clone(); };
    
    // Save checkpoint
    checkpointer.saveCheckpoint(0, input, forward_fn);
    EXPECT_TRUE(checkpointer.hasCheckpoint(0));
    
    // Clear checkpoint
    checkpointer.clearCheckpoint(0);
    EXPECT_FALSE(checkpointer.hasCheckpoint(0));
}

TEST_F(GradientCheckpointingTest, ClearAllCheckpoints) {
    CheckpointConfig config;
    config.strategy = CheckpointStrategy::SQRT_N;
    config.total_layers = 16;
    
    GradientCheckpointer checkpointer(config);
    
    Device device = Device::cpu();
    GPUTensor input({2, 4}, device);
    auto forward_fn = [](const GPUTensor& x) -> GPUTensor { return x.clone(); };
    
    // Save multiple checkpoints
    checkpointer.saveCheckpoint(0, input, forward_fn);
    checkpointer.saveCheckpoint(4, input, forward_fn);
    checkpointer.saveCheckpoint(8, input, forward_fn);
    
    EXPECT_TRUE(checkpointer.hasCheckpoint(0));
    EXPECT_TRUE(checkpointer.hasCheckpoint(4));
    EXPECT_TRUE(checkpointer.hasCheckpoint(8));
    
    // Clear all
    checkpointer.clearAll();
    
    EXPECT_FALSE(checkpointer.hasCheckpoint(0));
    EXPECT_FALSE(checkpointer.hasCheckpoint(4));
    EXPECT_FALSE(checkpointer.hasCheckpoint(8));
}

// ============================================================================
// Test Statistics Tracking
// ============================================================================

TEST_F(GradientCheckpointingTest, StatisticsTracking) {
    CheckpointConfig config;
    config.strategy = CheckpointStrategy::SQRT_N;
    config.total_layers = 36;
    
    GradientCheckpointer checkpointer(config);
    
    auto stats = checkpointer.getStats();
    
    EXPECT_EQ(stats.total_layers, 36);
    EXPECT_EQ(stats.num_checkpoints, 0);  // No checkpoints saved yet
    EXPECT_GE(stats.memory_reduction_pct, 0.0f);
    EXPECT_GE(stats.compute_overhead_pct, 0.0f);
}

// ============================================================================
// Test GPU LoRA Layer Integration
// ============================================================================

TEST_F(GradientCheckpointingTest, GPULoRALayerCheckpointing) {
    Device device = Device::cpu();
    
    size_t in_dim = 8;
    size_t out_dim = 8;
    size_t rank = 4;
    
    GPULoRALayer layer(in_dim, out_dim, rank, 1.0f, device, false, false);
    
    // Enable checkpointing
    layer.set_checkpointing(true);
    layer.set_layer_id(0);
    
    EXPECT_TRUE(layer.use_checkpointing());
    EXPECT_EQ(layer.layer_id(), 0);
    
    // Test forward pass with checkpointing
    GPUTensor input({2, in_dim}, device);
    std::vector<float> input_data(2 * in_dim, 1.0f);
    input.upload(input_data);
    
    GPUTensor output = layer.forward(input);
    
    // Verify output shape
    auto output_shape = output.shape();
    EXPECT_EQ(output_shape.size(), 2);
    EXPECT_EQ(output_shape[0], 2);  // batch_size
    EXPECT_EQ(output_shape[1], out_dim);
    
    // Test backward pass (should recompute activations)
    GPUTensor grad_output({2, out_dim}, device);
    std::vector<float> grad_data(2 * out_dim, 0.1f);
    grad_output.upload(grad_data);
    
    GPUTensor grad_input = layer.backward(grad_output);
    
    // Verify gradient shape
    auto grad_shape = grad_input.shape();
    EXPECT_EQ(grad_shape.size(), 2);
    EXPECT_EQ(grad_shape[0], 2);  // batch_size
    EXPECT_EQ(grad_shape[1], in_dim);
}

TEST_F(GradientCheckpointingTest, GPULoRALayerWithoutCheckpointing) {
    Device device = Device::cpu();
    
    size_t in_dim = 8;
    size_t out_dim = 8;
    size_t rank = 4;
    
    GPULoRALayer layer(in_dim, out_dim, rank, 1.0f, device, false, false);
    
    // Checkpointing disabled by default
    EXPECT_FALSE(layer.use_checkpointing());
    
    // Normal forward/backward should work
    GPUTensor input({2, in_dim}, device);
    std::vector<float> input_data(2 * in_dim, 1.0f);
    input.upload(input_data);
    
    GPUTensor output = layer.forward(input);
    
    GPUTensor grad_output({2, out_dim}, device);
    std::vector<float> grad_data(2 * out_dim, 0.1f);
    grad_output.upload(grad_data);
    
    GPUTensor grad_input = layer.backward(grad_output);
    
    // Should complete without errors
    EXPECT_GT(grad_input.size(), 0);
}

// ============================================================================
// Test Training Loop Integration
// ============================================================================

TEST_F(GradientCheckpointingTest, TrainingLoopCheckpointingConfig) {
    GPUTrainingConfig config;
    config.enable_gradient_checkpointing = true;
    config.checkpoint_strategy = CheckpointStrategy::SQRT_N;
    config.checkpoint_frequency = 4;
    config.num_epochs = 1;
    config.device = Device::cpu();
    
    GPUTrainingLoop loop(config);
    
    // Configuration should be stored
    // Note: We can't directly test internal state, but we can verify
    // that the training loop was created successfully
    EXPECT_FALSE(loop.isTraining());
}

TEST_F(GradientCheckpointingTest, DifferentStrategies) {
    // Test that different strategies can be configured
    std::vector<CheckpointStrategy> strategies = {
        CheckpointStrategy::NONE,
        CheckpointStrategy::UNIFORM,
        CheckpointStrategy::SQRT_N,
        CheckpointStrategy::ATTENTION_ONLY,
        CheckpointStrategy::CUSTOM
    };
    
    for (auto strategy : strategies) {
        CheckpointConfig config;
        config.strategy = strategy;
        config.total_layers = 32;
        config.checkpoint_frequency = 4;
        
        // Should not throw
        EXPECT_NO_THROW({
            GradientCheckpointer checkpointer(config);
        });
    }
}

// ============================================================================
// Test Edge Cases
// ============================================================================

TEST_F(GradientCheckpointingTest, ZeroLayers) {
    CheckpointConfig config;
    config.strategy = CheckpointStrategy::SQRT_N;
    config.total_layers = 0;
    
    // Should fall back to NONE strategy
    GradientCheckpointer checkpointer(config);
    
    EXPECT_FALSE(checkpointer.shouldCheckpoint(0));
}

TEST_F(GradientCheckpointingTest, NegativeLayerId) {
    CheckpointConfig config;
    config.strategy = CheckpointStrategy::SQRT_N;
    config.total_layers = 32;
    
    GradientCheckpointer checkpointer(config);
    
    // Negative layer IDs should return false
    EXPECT_FALSE(checkpointer.shouldCheckpoint(-1));
    EXPECT_FALSE(checkpointer.shouldCheckpoint(-10));
}

TEST_F(GradientCheckpointingTest, RecomputeWithoutSaving) {
    CheckpointConfig config;
    config.strategy = CheckpointStrategy::SQRT_N;
    config.total_layers = 16;
    
    GradientCheckpointer checkpointer(config);
    
    // Try to recompute without saving
    EXPECT_THROW({
        checkpointer.recomputeActivation(0);
    }, std::runtime_error);
}
