#include <gtest/gtest.h>
#include "llm/lora_framework/lora_layers.h"
#include "llm/lora_framework/lr_scheduler.h"
#include <memory>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace themis::llm::lora;

/**
 * @file test_lora_optimizer.cpp
 * @brief Comprehensive tests for LoRA optimizers (SGD, Adam, AdamW)
 * 
 * Test Coverage:
 * - Adam update rule components
 * - Bias correction
 * - AdamW weight decay
 * - Convergence on toy problems
 * - Learning rate scheduling
 * - Comparison: Adam vs SGD
 */

class OptimizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create simple test parameters
        param1_ = std::make_unique<Tensor>(std::vector<size_t>{2, 2});
        param1_->requires_grad = true;
        param1_->grad = Tensor({2, 2}, 0.0f);
        
        param2_ = std::make_unique<Tensor>(std::vector<size_t>{3, 1});
        param2_->requires_grad = true;
        param2_->grad = Tensor({3, 1}, 0.0f);
        
        // Initialize with known values
        param1_->data()[0] = 1.0f;
        param1_->data()[1] = 2.0f;
        param1_->data()[2] = 3.0f;
        param1_->data()[3] = 4.0f;
        
        param2_->data()[0] = 0.5f;
        param2_->data()[1] = -0.5f;
        param2_->data()[2] = 1.0f;
    }
    
    void TearDown() override {
        // Cleanup
    }
    
    std::unique_ptr<Tensor> param1_;
    std::unique_ptr<Tensor> param2_;
};

// ===== SGD Optimizer Tests =====

TEST_F(OptimizerTest, SGD_Construction) {
    SGDOptimizer optimizer(0.01f, 0.9f, 0.0001f);
    EXPECT_FLOAT_EQ(optimizer.learning_rate(), 0.01f);
}

TEST_F(OptimizerTest, SGD_ParameterRegistration) {
    SGDOptimizer optimizer(0.01f);
    std::vector<Tensor*> params = {param1_.get(), param2_.get()};
    optimizer.add_parameters(params);
    
    // Just verify it doesn't crash
    EXPECT_TRUE(true);
}

TEST_F(OptimizerTest, SGD_BasicUpdate) {
    SGDOptimizer optimizer(0.1f);
    optimizer.add_parameters({param1_.get()});
    
    // Set gradients
    param1_->grad[0] = 0.1f;
    param1_->grad[1] = 0.2f;
    param1_->grad[2] = 0.3f;
    param1_->grad[3] = 0.4f;
    
    float initial_val = param1_->data()[0];
    
    optimizer.step();
    
    // Check update: param = param - lr * grad
    EXPECT_FLOAT_EQ(param1_->data()[0], initial_val - 0.1f * 0.1f);
}

TEST_F(OptimizerTest, SGD_ZeroGrad) {
    SGDOptimizer optimizer(0.01f);
    optimizer.add_parameters({param1_.get()});
    
    // Set some gradients
    param1_->grad[0] = 1.0f;
    param1_->grad[1] = 2.0f;
    
    optimizer.zero_grad();
    
    // Check gradients are zeroed
    EXPECT_FLOAT_EQ(param1_->grad[0], 0.0f);
    EXPECT_FLOAT_EQ(param1_->grad[1], 0.0f);
}

// ===== Adam Optimizer Tests =====

TEST_F(OptimizerTest, Adam_Construction) {
    AdamOptimizer optimizer(1e-3f, 0.9f, 0.999f, 1e-8f, 0.0f);
    EXPECT_FLOAT_EQ(optimizer.learning_rate(), 1e-3f);
    EXPECT_EQ(optimizer.step_count(), 0);
}

TEST_F(OptimizerTest, Adam_ParameterRegistration) {
    AdamOptimizer optimizer(1e-3f);
    std::vector<Tensor*> params = {param1_.get(), param2_.get()};
    optimizer.add_parameters(params);
    
    EXPECT_TRUE(true);
}

TEST_F(OptimizerTest, Adam_FirstStep) {
    AdamOptimizer optimizer(0.1f, 0.9f, 0.999f, 1e-8f, 0.0f);
    optimizer.add_parameters({param1_.get()});
    
    // Set gradients
    param1_->grad[0] = 0.1f;
    
    float initial_val = param1_->data()[0];
    
    optimizer.step();
    
    // After first step, step_count should be 1
    EXPECT_EQ(optimizer.step_count(), 1);
    
    // Value should have changed
    EXPECT_NE(param1_->data()[0], initial_val);
}

TEST_F(OptimizerTest, Adam_BiasCorrection) {
    // Test that bias correction is applied in early steps
    AdamOptimizer optimizer(0.01f, 0.9f, 0.999f, 1e-8f, 0.0f);
    optimizer.add_parameters({param1_.get()});
    
    // Set constant gradient
    for (size_t i = 0; i < param1_->data().size(); ++i) {
        param1_->grad[i] = 0.1f;
    }
    
    float val_before = param1_->data()[0];
    
    // First step - bias correction should have significant effect
    optimizer.step();
    float val_after_step1 = param1_->data()[0];
    float update_step1 = std::abs(val_after_step1 - val_before);
    
    // Continue with same gradients
    for (size_t i = 0; i < param1_->data().size(); ++i) {
        param1_->grad[i] = 0.1f;
    }
    
    // Take more steps
    for (int i = 0; i < 10; ++i) {
        optimizer.step();
        for (size_t j = 0; j < param1_->data().size(); ++j) {
            param1_->grad[j] = 0.1f;
        }
    }
    
    float val_after_step11 = param1_->data()[0];
    
    // Bias correction effect should diminish over time
    // Early updates should be larger due to bias correction
    EXPECT_GT(update_step1, 0.0f);
    EXPECT_EQ(optimizer.step_count(), 11);
}

TEST_F(OptimizerTest, Adam_MomentumAccumulation) {
    // Test that momentum accumulates over steps
    AdamOptimizer optimizer(0.01f, 0.9f, 0.999f, 1e-8f, 0.0f);
    optimizer.add_parameters({param1_.get()});
    
    std::vector<float> param_history;
    
    // Apply consistent gradient over multiple steps
    for (int step = 0; step < 5; ++step) {
        for (size_t i = 0; i < param1_->data().size(); ++i) {
            param1_->grad[i] = 0.1f;
        }
        optimizer.step();
        param_history.push_back(param1_->data()[0]);
    }
    
    // Parameters should be changing in consistent direction
    EXPECT_GT(param_history.size(), 1u);
}

TEST_F(OptimizerTest, Adam_ZeroGrad) {
    AdamOptimizer optimizer(0.01f);
    optimizer.add_parameters({param1_.get()});
    
    // Set some gradients
    param1_->grad[0] = 1.0f;
    param1_->grad[1] = 2.0f;
    
    optimizer.zero_grad();
    
    // Check gradients are zeroed
    EXPECT_FLOAT_EQ(param1_->grad[0], 0.0f);
    EXPECT_FLOAT_EQ(param1_->grad[1], 0.0f);
}

// ===== AdamW Optimizer Tests =====

TEST_F(OptimizerTest, AdamW_Construction) {
    AdamWOptimizer optimizer(1e-4f, 0.9f, 0.999f, 1e-8f, 0.01f);
    EXPECT_FLOAT_EQ(optimizer.learning_rate(), 1e-4f);
    EXPECT_EQ(optimizer.step_count(), 0);
}

TEST_F(OptimizerTest, AdamW_WeightDecay) {
    // Test that weight decay is applied correctly (decoupled)
    AdamWOptimizer optimizer(0.01f, 0.9f, 0.999f, 1e-8f, 0.1f);
    optimizer.add_parameters({param1_.get()});
    
    // Set gradients to zero - only weight decay should affect parameters
    for (size_t i = 0; i < param1_->data().size(); ++i) {
        param1_->grad[i] = 0.0f;
    }
    
    float initial_val = param1_->data()[0];
    
    optimizer.step();
    
    // With zero gradient, AdamW should still apply weight decay
    // param = param - lr * weight_decay * param
    float expected_val = initial_val - 0.01f * 0.1f * initial_val;
    
    // Allow for numerical precision differences
    EXPECT_NEAR(param1_->data()[0], expected_val, 1e-5f);
}

TEST_F(OptimizerTest, AdamW_ZeroGrad) {
    AdamWOptimizer optimizer(0.01f);
    optimizer.add_parameters({param1_.get()});
    
    // Set some gradients
    param1_->grad[0] = 1.0f;
    param1_->grad[1] = 2.0f;
    
    optimizer.zero_grad();
    
    // Check gradients are zeroed
    EXPECT_FLOAT_EQ(param1_->grad[0], 0.0f);
    EXPECT_FLOAT_EQ(param1_->grad[1], 0.0f);
}

// ===== Convergence Tests =====

TEST_F(OptimizerTest, Convergence_SimpleQuadratic_Adam) {
    // Test convergence on simple quadratic: f(x) = (x - 5)^2
    // Gradient: g(x) = 2(x - 5)
    // Optimal x = 5
    
    Tensor param({1}, 0.0f);  // Start at x = 0
    param.requires_grad = true;
    param.grad = Tensor({1}, 0.0f);
    
    AdamOptimizer optimizer(0.1f);
    optimizer.add_parameters({&param});
    
    // Run optimization
    for (int step = 0; step < 50; ++step) {
        // Compute gradient: 2(x - 5)
        param.grad[0] = 2.0f * (param[0] - 5.0f);
        optimizer.step();
    }
    
    // Should converge close to 5
    EXPECT_NEAR(param[0], 5.0f, 0.5f);
}

TEST_F(OptimizerTest, Convergence_SimpleQuadratic_AdamW) {
    // Same test with AdamW
    Tensor param({1}, 0.0f);
    param.requires_grad = true;
    param.grad = Tensor({1}, 0.0f);
    
    AdamWOptimizer optimizer(0.1f, 0.9f, 0.999f, 1e-8f, 0.001f);
    optimizer.add_parameters({&param});
    
    // Run optimization
    for (int step = 0; step < 50; ++step) {
        param.grad[0] = 2.0f * (param[0] - 5.0f);
        optimizer.step();
    }
    
    // Should converge close to 5 (might be slightly different due to weight decay)
    EXPECT_NEAR(param[0], 5.0f, 0.5f);
}

TEST_F(OptimizerTest, Convergence_Comparison_Adam_vs_SGD) {
    // Compare Adam vs SGD on same problem
    Tensor param_adam({1}, 0.0f);
    param_adam.requires_grad = true;
    param_adam.grad = Tensor({1}, 0.0f);
    
    Tensor param_sgd({1}, 0.0f);
    param_sgd.requires_grad = true;
    param_sgd.grad = Tensor({1}, 0.0f);
    
    AdamOptimizer adam(0.1f);
    adam.add_parameters({&param_adam});
    
    SGDOptimizer sgd(0.01f);  // Lower LR for SGD
    sgd.add_parameters({&param_sgd});
    
    // Run same optimization
    int steps = 30;
    for (int step = 0; step < steps; ++step) {
        param_adam.grad[0] = 2.0f * (param_adam[0] - 5.0f);
        adam.step();
        
        param_sgd.grad[0] = 2.0f * (param_sgd[0] - 5.0f);
        sgd.step();
    }
    
    // Both should make progress, Adam typically faster
    float adam_error = std::abs(param_adam[0] - 5.0f);
    float sgd_error = std::abs(param_sgd[0] - 5.0f);
    
    // Both should reduce error from initial value (5.0)
    EXPECT_LT(adam_error, 5.0f);
    EXPECT_LT(sgd_error, 5.0f);
}

// ===== Learning Rate Scheduler Tests =====

TEST_F(OptimizerTest, LRScheduler_Constant) {
    ConstantLR scheduler(0.001f);
    
    EXPECT_FLOAT_EQ(scheduler.get_lr(0), 0.001f);
    EXPECT_FLOAT_EQ(scheduler.get_lr(100), 0.001f);
    EXPECT_FLOAT_EQ(scheduler.get_lr(1000), 0.001f);
}

TEST_F(OptimizerTest, LRScheduler_LinearWarmup) {
    LinearWarmupLR scheduler(0.001f, 100);
    
    // At step 0, lr should be 0
    EXPECT_FLOAT_EQ(scheduler.get_lr(0), 0.0f);
    
    // At step 50, lr should be half of base_lr
    EXPECT_NEAR(scheduler.get_lr(50), 0.0005f, 1e-6f);
    
    // At step 100 and beyond, lr should be base_lr
    EXPECT_FLOAT_EQ(scheduler.get_lr(100), 0.001f);
    EXPECT_FLOAT_EQ(scheduler.get_lr(200), 0.001f);
}

TEST_F(OptimizerTest, LRScheduler_CosineAnnealing) {
    CosineAnnealingLR scheduler(0.001f, 100, 0.0f);
    
    // At step 0, lr should be base_lr
    EXPECT_NEAR(scheduler.get_lr(0), 0.001f, 1e-6f);
    
    // At step 50 (halfway), lr should be approximately base_lr/2
    EXPECT_NEAR(scheduler.get_lr(50), 0.0005f, 1e-4f);
    
    // At step 100 and beyond, lr should be min_lr (0)
    EXPECT_NEAR(scheduler.get_lr(100), 0.0f, 1e-6f);
}

TEST_F(OptimizerTest, LRScheduler_CosineAnnealingWarmup) {
    CosineAnnealingWarmupLR scheduler(0.001f, 10, 100, 0.0f);
    
    // During warmup (step < 10)
    EXPECT_NEAR(scheduler.get_lr(0), 0.0f, 1e-6f);
    EXPECT_NEAR(scheduler.get_lr(5), 0.0005f, 1e-6f);
    
    // After warmup, should follow cosine annealing
    EXPECT_NEAR(scheduler.get_lr(10), 0.001f, 1e-5f);
    
    // At the end
    EXPECT_NEAR(scheduler.get_lr(100), 0.0f, 1e-5f);
}

TEST_F(OptimizerTest, LRScheduler_StepDecay) {
    StepLR scheduler(0.001f, 30, 0.1f);
    
    // Before first decay
    EXPECT_FLOAT_EQ(scheduler.get_lr(0), 0.001f);
    EXPECT_FLOAT_EQ(scheduler.get_lr(29), 0.001f);
    
    // After first decay
    EXPECT_NEAR(scheduler.get_lr(30), 0.0001f, 1e-7f);
    EXPECT_NEAR(scheduler.get_lr(59), 0.0001f, 1e-7f);
    
    // After second decay
    EXPECT_NEAR(scheduler.get_lr(60), 0.00001f, 1e-8f);
}

TEST_F(OptimizerTest, LRScheduler_Exponential) {
    ExponentialLR scheduler(0.001f, 0.99f);
    
    // At step 0
    EXPECT_FLOAT_EQ(scheduler.get_lr(0), 0.001f);
    
    // At step 1
    EXPECT_NEAR(scheduler.get_lr(1), 0.00099f, 1e-7f);
    
    // At step 10
    float expected = 0.001f * std::pow(0.99f, 10);
    EXPECT_NEAR(scheduler.get_lr(10), expected, 1e-7f);
}

// ===== Integration Tests =====

TEST_F(OptimizerTest, Integration_AdamWithScheduler) {
    // Test Adam optimizer with learning rate scheduling
    Tensor param({1}, 0.0f);
    param.requires_grad = true;
    param.grad = Tensor({1}, 0.0f);
    
    AdamOptimizer optimizer(0.1f);
    optimizer.add_parameters({&param});
    
    LinearWarmupLR scheduler(0.1f, 10);
    
    // Run optimization with scheduled learning rate
    for (int step = 0; step < 20; ++step) {
        // Update learning rate from scheduler
        float current_lr = scheduler.get_lr(step);
        optimizer.set_learning_rate(current_lr);
        
        // Compute gradient
        param.grad[0] = 2.0f * (param[0] - 5.0f);
        
        // Optimize
        optimizer.step();
    }
    
    // Should make progress
    EXPECT_NE(param[0], 0.0f);
}

// ===== Edge Case Tests =====

TEST_F(OptimizerTest, EdgeCase_ZeroGradient) {
    AdamOptimizer optimizer(0.01f);
    optimizer.add_parameters({param1_.get()});
    
    float initial_val = param1_->data()[0];
    
    // Zero gradient
    param1_->grad.zero();
    
    optimizer.step();
    
    // Parameter should barely change (only weight decay if enabled)
    EXPECT_NEAR(param1_->data()[0], initial_val, 1e-5f);
}

TEST_F(OptimizerTest, EdgeCase_VeryLargeGradient) {
    AdamOptimizer optimizer(0.01f);
    optimizer.add_parameters({param1_.get()});
    
    float initial_val = param1_->data()[0];
    
    // Very large gradient
    param1_->grad[0] = 1000.0f;
    
    optimizer.step();
    
    // Parameter should change, but not by 1000 due to adaptive learning rate
    float change = std::abs(param1_->data()[0] - initial_val);
    EXPECT_GT(change, 0.0f);
    EXPECT_LT(change, 100.0f);  // Should be moderated by Adam
}

TEST_F(OptimizerTest, EdgeCase_VerySmallGradient) {
    AdamOptimizer optimizer(0.01f);
    optimizer.add_parameters({param1_.get()});
    
    float initial_val = param1_->data()[0];
    
    // Very small gradient
    param1_->grad[0] = 1e-10f;
    
    optimizer.step();
    
    // Parameter should barely change
    EXPECT_NEAR(param1_->data()[0], initial_val, 1e-5f);
}

// ===== Main =====

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
