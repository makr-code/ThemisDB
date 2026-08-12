/**
 * ThemisDB Multi-GPU Training Tests
 * 
 * Tests for multi-GPU LoRA training with NCCL/RCCL collective operations
 */

#include <gtest/gtest.h>
#include "llm/lora_framework/multi_gpu.h"
#include "llm/lora_framework/multi_gpu_trainer.h"
#include "llm/lora_framework/multi_gpu_lora_layer.h"
#include <vector>
#include <memory>
#include <cmath>

using namespace themis::llm::lora;

// ═══════════════════════════════════════════════════════════
// Test Fixtures
// ═══════════════════════════════════════════════════════════

class MultiGPUTrainingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize multi-GPU context (will auto-detect GPUs or use CPU fallback)
        ctx_ = std::make_unique<MultiGPUContext>(0);  // Use all available GPUs
        
        // Skip tests if no GPUs available
        if (ctx_->num_gpus() == 0) {
            GTEST_SKIP() << "capability:multi_gpu_runtime_available=false;reason=no_gpus_available_for_training_tests";
        }
        
        // Setup trainer config
        config_.learning_rate = 0.001f;
        config_.gradient_accumulation_steps = 1;
        config_.sync_every_step = true;
    }
    
    std::unique_ptr<MultiGPUContext> ctx_;
    MultiGPULoRATrainer::Config config_;
};

// ═══════════════════════════════════════════════════════════
// Basic Functionality Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPUTrainingTest, ContextCreation) {
    ASSERT_NE(ctx_, nullptr);
    EXPECT_GT(ctx_->num_gpus(), 0);
    
    // Check device info
    for (int i = 0; i < ctx_->num_gpus(); ++i) {
        Device device = ctx_->get_device(i);
        EXPECT_GE(device.device_id, 0);
        EXPECT_TRUE(device.type == DeviceType::CUDA || device.type == DeviceType::HIP);
    }
}

TEST_F(MultiGPUTrainingTest, TrainerCreation) {
    MultiGPULoRATrainer trainer(*ctx_, config_);
    
    // Trainer should be created successfully
    auto stats = trainer.get_stats();
    EXPECT_EQ(stats.total_steps, 0);
}

TEST_F(MultiGPUTrainingTest, LayerCreation) {
    MultiGPULoRATrainer trainer(*ctx_, config_);
    
    // Create a small LoRA layer: 64x64, rank=8
    auto layer = trainer.create_layer(64, 64, 8, 1.0f);
    
    ASSERT_NE(layer, nullptr);
    EXPECT_EQ(layer->num_gpus(), ctx_->num_gpus());
    // Note: rank and scaling are constructor params, not exposed as getters
}

// ═══════════════════════════════════════════════════════════
// Training Step Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPUTrainingTest, SingleTrainingStep) {
    MultiGPULoRATrainer trainer(*ctx_, config_);
    auto layer = trainer.create_layer(64, 64, 8, 1.0f);
    
    // Create dummy input and target data for each GPU
    std::vector<GPUTensor> inputs;
    std::vector<GPUTensor> targets;
    
    for (int i = 0; i < ctx_->num_gpus(); ++i) {
        Device device = ctx_->get_device(i);
        
        // Create tensors: batch_size=4, in_dim=64
        GPUTensor input({4, 64}, device);
        GPUTensor target({4, 64}, device);
        
        // Fill with dummy data
        std::vector<float> input_data(4 * 64, 0.5f);
        std::vector<float> target_data(4 * 64, 1.0f);
        
        input.upload(input_data);
        target.upload(target_data);
        
        inputs.push_back(std::move(input));
        targets.push_back(std::move(target));
    }
    
    // Perform training step
    float loss = trainer.train_step(*layer, inputs, targets);
    
    // Loss should be positive and finite
    EXPECT_GT(loss, 0.0f);
    EXPECT_TRUE(std::isfinite(loss));
    
    // Stats should be updated
    auto stats = trainer.get_stats();
    EXPECT_EQ(stats.total_steps, 1);
}

TEST_F(MultiGPUTrainingTest, MultipleTrainingSteps) {
    MultiGPULoRATrainer trainer(*ctx_, config_);
    auto layer = trainer.create_layer(64, 64, 8, 1.0f);
    
    const int num_steps = 5;
    std::vector<float> losses;
    
    for (int step = 0; step < num_steps; ++step) {
        // Create dummy data
        std::vector<GPUTensor> inputs;
        std::vector<GPUTensor> targets;
        
        for (int i = 0; i < ctx_->num_gpus(); ++i) {
            Device device = ctx_->get_device(i);
            
            GPUTensor input({4, 64}, device);
            GPUTensor target({4, 64}, device);
            
            std::vector<float> input_data(4 * 64, 0.5f + step * 0.1f);
            std::vector<float> target_data(4 * 64, 1.0f);
            
            input.upload(input_data);
            target.upload(target_data);
            
            inputs.push_back(std::move(input));
            targets.push_back(std::move(target));
        }
        
        float loss = trainer.train_step(*layer, inputs, targets);
        losses.push_back(loss);
        
        EXPECT_GT(loss, 0.0f);
        EXPECT_TRUE(std::isfinite(loss));
    }
    
    // Verify stats
    auto stats = trainer.get_stats();
    EXPECT_EQ(stats.total_steps, num_steps);
    
    // Loss should be recorded
    EXPECT_EQ(losses.size(), num_steps);
}

// ═══════════════════════════════════════════════════════════
// Gradient Accumulation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPUTrainingTest, GradientAccumulation) {
    config_.gradient_accumulation_steps = 4;
    config_.sync_every_step = false;  // Only sync after accumulation
    
    MultiGPULoRATrainer trainer(*ctx_, config_);
    auto layer = trainer.create_layer(64, 64, 8, 1.0f);
    
    for (int step = 0; step < 8; ++step) {
        std::vector<GPUTensor> inputs;
        std::vector<GPUTensor> targets;
        
        for (int i = 0; i < ctx_->num_gpus(); ++i) {
            Device device = ctx_->get_device(i);
            GPUTensor input({4, 64}, device);
            GPUTensor target({4, 64}, device);
            
            std::vector<float> input_data(4 * 64, 0.5f);
            std::vector<float> target_data(4 * 64, 1.0f);
            input.upload(input_data);
            target.upload(target_data);
            
            inputs.push_back(std::move(input));
            targets.push_back(std::move(target));
        }
        
        float loss = trainer.train_step(*layer, inputs, targets);
        EXPECT_GT(loss, 0.0f);
    }
    
    auto stats = trainer.get_stats();
    EXPECT_EQ(stats.total_steps, 8);
}

// ═══════════════════════════════════════════════════════════
// Performance and Stats Tests
// ═══════════════════════════════════════════════════════════

TEST_F(MultiGPUTrainingTest, StatisticsTracking) {
    MultiGPULoRATrainer trainer(*ctx_, config_);
    auto layer = trainer.create_layer(64, 64, 8, 1.0f);
    
    // Perform several steps
    for (int step = 0; step < 10; ++step) {
        std::vector<GPUTensor> inputs;
        std::vector<GPUTensor> targets;
        
        for (int i = 0; i < ctx_->num_gpus(); ++i) {
            Device device = ctx_->get_device(i);
            GPUTensor input({4, 64}, device);
            GPUTensor target({4, 64}, device);
            
            std::vector<float> input_data(4 * 64, 0.5f);
            std::vector<float> target_data(4 * 64, 1.0f);
            input.upload(input_data);
            target.upload(target_data);
            
            inputs.push_back(std::move(input));
            targets.push_back(std::move(target));
        }
        
        trainer.train_step(*layer, inputs, targets);
    }
    
    auto stats = trainer.get_stats();
    
    // Verify statistics
    EXPECT_EQ(stats.total_steps, 10);
    EXPECT_GT(stats.avg_step_time_ms, 0.0f);
    EXPECT_GE(stats.avg_communication_time_ms, 0.0f);
    EXPECT_GT(stats.avg_loss, 0.0f);
    
    // Communication overhead should be reasonable (< 50%)
    if (ctx_->num_gpus() > 1) {
        float comm_overhead = stats.communication_overhead();
        EXPECT_GE(comm_overhead, 0.0f);
        EXPECT_LT(comm_overhead, 0.5f);  // Less than 50% overhead
    }
}

// ═══════════════════════════════════════════════════════════
// CPU Fallback Tests
// ═══════════════════════════════════════════════════════════

TEST(MultiGPUTrainingCPUTest, CPUFallback) {
    // Attempt to create a context that would fall back to CPU when no GPUs are available.
    // Note: MultiGPUContext(0, {}) may still auto-detect GPUs if present.
    std::vector<int> no_gpus;
    MultiGPUContext ctx(0, no_gpus);
    
    if (ctx.num_gpus() != 0) {
        // On systems with GPUs, this API does not enforce CPU-only mode.
        // Skip to avoid asserting an incorrect assumption about num_gpus().
        GTEST_SKIP() << "capability:cpu_only_mode_enforceable=false;reason=gpu_detected_and_api_cannot_force_cpu_only";
    }
    
    // In a true CPU-only environment, verify that no GPUs are reported and skip training.
    EXPECT_EQ(ctx.num_gpus(), 0);
    
    // This test demonstrates graceful handling when no GPUs are available.
    GTEST_SKIP() << "capability:multi_gpu_runtime_available=false;reason=cpu_only_mode_no_gpus";
}
