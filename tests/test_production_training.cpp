/**
 * @file test_production_training.cpp
 * @brief Unit tests for LoRA production training features
 * 
 * Tests:
 * - Mixed precision training (FP16/BF16, loss scaling)
 * - Learning rate schedulers (constant, linear, cosine, warmup)
 * - Gradient utilities (clipping, accumulation, statistics)
 * - Distributed training (basic functionality)
 */

#ifndef THEMIS_TEST_BUILD
#define THEMIS_TEST_BUILD 1
#endif

#include <gtest/gtest.h>
#include "llm/lora_framework/mixed_precision.h"
#include "llm/lora_framework/lr_scheduler.h"
#include "llm/lora_framework/gradient_utils.h"
#include "llm/lora_framework/distributed_trainer.h"
#include "llm/lora_framework/lora_layers.h"
#include <memory>

using namespace themis::llm::lora;

// ============================================================================
// Mixed Precision Tests
// ============================================================================

TEST(MixedPrecisionTest, Initialization) {
    MixedPrecisionConfig config;
    config.mode = PrecisionMode::FP16;
    config.loss_scale = 1024.0f;
    
    MixedPrecisionTrainer trainer(config);
    
    EXPECT_TRUE(trainer.is_enabled());
    EXPECT_EQ(trainer.get_precision_mode(), PrecisionMode::FP16);
    EXPECT_FLOAT_EQ(trainer.get_loss_scale(), 1024.0f);
}

TEST(MixedPrecisionTest, LossScaling) {
    MixedPrecisionConfig config;
    config.mode = PrecisionMode::FP16;
    config.loss_scale = 1024.0f;
    
    MixedPrecisionTrainer trainer(config);
    
    float loss = 0.5f;
    float scaled_loss = trainer.scale_loss(loss);
    
    EXPECT_FLOAT_EQ(scaled_loss, 512.0f);
}

TEST(MixedPrecisionTest, GradientUnscaling) {
    MixedPrecisionConfig config;
    config.mode = PrecisionMode::FP16;
    config.loss_scale = 2.0f;
    
    MixedPrecisionTrainer trainer(config);
    
    // Create test gradients
    Tensor grad({2, 2}, 4.0f);  // All values = 4.0
    std::vector<Tensor*> gradients = {&grad};
    
    // Unscale (should divide by 2.0)
    bool success = trainer.unscale_gradients(gradients);
    
    EXPECT_TRUE(success);
    EXPECT_FLOAT_EQ(grad[0], 2.0f);
}

TEST(MixedPrecisionTest, OverflowDetection) {
    Tensor grad({4});
    grad[0] = 1.0f;
    grad[1] = 2.0f;
    grad[2] = std::nanf("");
    grad[3] = 3.0f;
    
    std::vector<Tensor*> gradients = {&grad};
    
    EXPECT_TRUE(MixedPrecisionTrainer::has_overflow(gradients));
}

TEST(MixedPrecisionTest, DynamicLossScaling) {
    MixedPrecisionConfig config;
    config.mode = PrecisionMode::FP16;
    config.loss_scale = 1024.0f;
    config.dynamic_loss_scaling = true;
    config.loss_scale_factor = 2.0f;
    
    MixedPrecisionTrainer trainer(config);
    
    // Simulate overflow
    trainer.update_loss_scale(true);
    EXPECT_FLOAT_EQ(trainer.get_loss_scale(), 512.0f);
    
    // Simulate stable period
    for (int i = 0; i < config.loss_scale_window; ++i) {
        trainer.update_loss_scale(false);
    }
    EXPECT_FLOAT_EQ(trainer.get_loss_scale(), 1024.0f);
}

// ============================================================================
// Learning Rate Scheduler Tests
// ============================================================================

TEST(LRSchedulerTest, ConstantScheduler) {
    auto scheduler = LRSchedulerFactory::createConstant(0.001f);
    
    EXPECT_FLOAT_EQ(scheduler->get_lr(0), 0.001f);
    EXPECT_FLOAT_EQ(scheduler->get_lr(100), 0.001f);
    EXPECT_FLOAT_EQ(scheduler->get_lr(1000), 0.001f);
}

TEST(LRSchedulerTest, LinearDecay) {
    auto scheduler = LRSchedulerFactory::createLinearDecay(0.001f, 0.0f, 100);
    
    EXPECT_FLOAT_EQ(scheduler->get_lr(0), 0.001f);
    EXPECT_NEAR(scheduler->get_lr(50), 0.0005f, 1e-6f);
    EXPECT_FLOAT_EQ(scheduler->get_lr(100), 0.0f);
}

TEST(LRSchedulerTest, CosineAnnealing) {
    auto scheduler = LRSchedulerFactory::createCosineAnnealing(0.001f, 0.0f, 100);
    
    EXPECT_FLOAT_EQ(scheduler->get_lr(0), 0.001f);
    EXPECT_GT(scheduler->get_lr(50), 0.0f);
    EXPECT_LT(scheduler->get_lr(50), 0.001f);
    EXPECT_FLOAT_EQ(scheduler->get_lr(100), 0.0f);
}

TEST(LRSchedulerTest, WarmupCosine) {
    auto scheduler = LRSchedulerFactory::createWarmupCosine(0.001f, 0.0f, 10, 100);
    
    // Warmup phase
    EXPECT_LT(scheduler->get_lr(0), 0.001f);
    EXPECT_LT(scheduler->get_lr(5), 0.001f);
    EXPECT_FLOAT_EQ(scheduler->get_lr(10), 0.001f);
    
    // Cosine decay phase
    EXPECT_LT(scheduler->get_lr(50), 0.001f);
    EXPECT_GT(scheduler->get_lr(50), 0.0f);
    EXPECT_FLOAT_EQ(scheduler->get_lr(100), 0.0f);
}

TEST(LRSchedulerTest, StepDecay) {
    StepLR scheduler(0.001f, 10, 0.5f);
    
    EXPECT_FLOAT_EQ(scheduler.get_lr(0), 0.001f);
    EXPECT_FLOAT_EQ(scheduler.get_lr(9), 0.001f);
    EXPECT_FLOAT_EQ(scheduler.get_lr(10), 0.0005f);
    EXPECT_FLOAT_EQ(scheduler.get_lr(20), 0.00025f);
}

TEST(LRSchedulerTest, ExponentialDecay) {
    ExponentialLR scheduler(0.001f, 0.9f);
    
    EXPECT_FLOAT_EQ(scheduler.get_lr(0), 0.001f);
    EXPECT_NEAR(scheduler.get_lr(1), 0.0009f, 1e-6f);
    EXPECT_LT(scheduler.get_lr(10), 0.001f);
}

// ============================================================================
// Gradient Utils Tests
// ============================================================================

TEST(GradientUtilsTest, GlobalNorm) {
    Tensor grad1({2, 2}, 1.0f);  // 4 elements, each = 1.0
    Tensor grad2({2, 2}, 1.0f);  // 4 elements, each = 1.0
    
    std::vector<Tensor*> gradients = {&grad1, &grad2};
    
    // Global norm = sqrt(8 * 1.0^2) = sqrt(8) ≈ 2.828
    float norm = GradientUtils::compute_global_norm(gradients);
    EXPECT_NEAR(norm, 2.828f, 0.001f);
}

TEST(GradientUtilsTest, ClipByNorm) {
    Tensor grad1({2, 2}, 2.0f);
    Tensor grad2({2, 2}, 2.0f);
    
    std::vector<Tensor*> gradients = {&grad1, &grad2};
    
    // Original norm = sqrt(8 * 4.0) = sqrt(32) ≈ 5.657
    float max_norm = 1.0f;
    bool clipped = GradientUtils::clip_by_norm(gradients, max_norm);
    
    EXPECT_TRUE(clipped);
    
    // New norm should be approximately max_norm
    float new_norm = GradientUtils::compute_global_norm(gradients);
    EXPECT_NEAR(new_norm, max_norm, 0.01f);
}

TEST(GradientUtilsTest, ClipByValue) {
    Tensor grad({4});
    grad[0] = 10.0f;
    grad[1] = -5.0f;
    grad[2] = 2.0f;
    grad[3] = -15.0f;
    
    std::vector<Tensor*> gradients = {&grad};
    
    float clip_value = 5.0f;
    bool clipped = GradientUtils::clip_by_value(gradients, clip_value);
    
    EXPECT_TRUE(clipped);
    EXPECT_FLOAT_EQ(grad[0], 5.0f);
    EXPECT_FLOAT_EQ(grad[1], -5.0f);
    EXPECT_FLOAT_EQ(grad[2], 2.0f);
    EXPECT_FLOAT_EQ(grad[3], -5.0f);
}

TEST(GradientUtilsTest, InvalidGradients) {
    Tensor grad1({2, 2}, 1.0f);
    Tensor grad2({2, 2});
    grad2[0] = std::nanf("");
    grad2[1] = 1.0f;
    grad2[2] = std::numeric_limits<float>::infinity();
    grad2[3] = 1.0f;
    
    std::vector<Tensor*> gradients1 = {&grad1};
    std::vector<Tensor*> gradients2 = {&grad2};
    
    EXPECT_FALSE(GradientUtils::has_invalid_gradients(gradients1));
    EXPECT_TRUE(GradientUtils::has_invalid_gradients(gradients2));
}

TEST(GradientUtilsTest, GradientAccumulation) {
    GradientAccumulationConfig config;
    config.accumulation_steps = 4;
    config.normalize = true;
    
    GradientAccumulator accumulator(config);
    
    // Accumulate gradients over 4 steps
    for (int i = 0; i < 4; ++i) {
        Tensor grad({2, 2}, 1.0f);
        std::vector<Tensor*> gradients = {&grad};
        accumulator.accumulate(gradients);
    }
    
    EXPECT_TRUE(accumulator.should_step());
    
    auto accumulated = accumulator.get_accumulated_gradients();
    EXPECT_EQ(accumulated.size(), 1);
    
    // After normalization, each value should be 1.0 (4 * 1.0 / 4)
    EXPECT_FLOAT_EQ(accumulated[0]->data()[0], 1.0f);
}

// ============================================================================
// Distributed Training Tests
// ============================================================================

TEST(DistributedTrainerTest, Initialization) {
    DistributedConfig config;
    config.world_size = 1;
    config.rank = 0;
    
    DistributedTrainer trainer(config);
    
    EXPECT_FALSE(trainer.is_distributed());
    EXPECT_TRUE(trainer.is_master());
    EXPECT_TRUE(trainer.initialize());
}

TEST(DistributedTrainerTest, MultiProcess) {
    DistributedConfig config;
    config.world_size = 4;
    config.rank = 0;
    
    DistributedTrainer trainer(config);
    
    EXPECT_TRUE(trainer.is_distributed());
    EXPECT_TRUE(trainer.is_master());
    EXPECT_EQ(trainer.world_size(), 4);
    EXPECT_EQ(trainer.rank(), 0);
}

TEST(DistributedTrainerTest, LearningRateScaling) {
    float base_lr = 0.001f;
    int world_size = 4;
    
    // Linear scaling
    float linear_lr = DistributedTrainer::scale_learning_rate(base_lr, world_size, "linear");
    EXPECT_FLOAT_EQ(linear_lr, 0.004f);
    
    // Square root scaling
    float sqrt_lr = DistributedTrainer::scale_learning_rate(base_lr, world_size, "sqrt");
    EXPECT_FLOAT_EQ(sqrt_lr, 0.002f);
}

TEST(DistributedTrainerTest, BackendDetection) {
    auto backends = detect_available_backends();
    
    EXPECT_GT(backends.size(), 0);
    EXPECT_EQ(backends[0], DistributedBackend::NONE);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(ProductionTrainingTest, FullPipeline) {
    // Create a simple LoRA layer
    LoRALayer layer(128, 128, 8, 1.0f);
    
    // Setup production features
    MixedPrecisionConfig mp_config;
    mp_config.mode = PrecisionMode::FP16;
    MixedPrecisionTrainer mp_trainer(mp_config);
    
    auto scheduler = LRSchedulerFactory::createWarmupCosine(0.001f, 0.0f, 10, 100);
    
    GradientClippingConfig clip_config;
    clip_config.method = ClippingMethod::BY_NORM;
    clip_config.max_norm = 1.0f;
    
    GradientAccumulationConfig accum_config;
    accum_config.accumulation_steps = 2;
    GradientAccumulator accumulator(accum_config);
    
    // Simulate training steps
    for (int step = 0; step < 5; ++step) {
        // Get learning rate
        float lr = scheduler->get_lr(step);
        EXPECT_GT(lr, 0.0f);
        
        // Create dummy input/output
        Tensor input({4, 128}, 1.0f);
        Tensor output = layer.forward(input);
        
        // Compute dummy gradients
        Tensor grad_output({4, 128}, 0.1f);
        layer.backward(grad_output);
        
        // Get parameters
        auto params = layer.parameters();
        
        // Apply gradient clipping
        GradientStats stats = GradientUtils::apply_clipping(params, clip_config);
        EXPECT_GE(stats.global_norm, 0.0f);
        
        // Accumulate
        accumulator.accumulate(params);
    }
    
    EXPECT_TRUE(true);  // Pipeline completed successfully
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
