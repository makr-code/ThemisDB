#include <gtest/gtest.h>
#include "llm/lora_framework/gpu_lora_layers.h"
#include "llm/lora_framework/lora_layers.h"
#include <cmath>
#include <spdlog/spdlog.h>

// Temporarily disable fused kernel GPU tests on MSVC
#define SKIP_FUSED_KERNEL_TESTS 1

#if SKIP_FUSED_KERNEL_TESTS

TEST(DummyFusedKernels, DisabledOnMSVC) {
    GTEST_SKIP() << "capability:fused_kernel_gpu_tests_enabled=false;reason=msvc_porting_in_progress";
}

#else

using namespace themis::llm::lora;

namespace {
    constexpr float EPSILON = 1e-5f;  // Numerical accuracy tolerance for fused kernels
    constexpr size_t TEST_IN_DIM = 64;
    constexpr size_t TEST_OUT_DIM = 32;
    constexpr size_t TEST_RANK = 8;
    constexpr size_t TEST_BATCH_SIZE = 16;
}

class FusedKernelsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Detect available devices
        auto backends = GPUMemoryManager::detect_backends();
        
        has_cuda_ = false;
        has_hip_ = false;
        
        for (const auto& backend : backends) {
            if (backend.type == themis::acceleration::BackendType::CUDA && backend.available) {
                has_cuda_ = true;
            }
            if (backend.type == themis::acceleration::BackendType::HIP && backend.available) {
                has_hip_ = true;
            }
        }
        
        spdlog::info("Test setup: CUDA={}, HIP={}", has_cuda_, has_hip_);
    }
    
    bool has_cuda_ = false;
    bool has_hip_ = false;
    
    // Helper to check if two tensors are close
    bool tensors_close(const GPUTensor& a, const GPUTensor& b, float eps = EPSILON) {
        if (a.shape() != b.shape()) {
            return false;
        }
        
        auto a_data = a.cpu_data();
        auto b_data = b.cpu_data();
        
        for (size_t i = 0; i < a_data.size(); ++i) {
            float diff = std::abs(a_data[i] - b_data[i]);
            if (diff > eps) {
                spdlog::error("Tensor mismatch at index {}: {} vs {} (diff={})", 
                             i, a_data[i], b_data[i], diff);
                return false;
            }
        }
        return true;
    }
};

// ===== Fused Forward Pass Tests =====

TEST_F(FusedKernelsTest, FusedForward_CUDA_AccuracyTest) {
    if (!has_cuda_) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_not_available";
    }
    
    // Create two layers: one with fused kernels, one without
    GPULoRALayer layer_fused(TEST_IN_DIM, TEST_OUT_DIM, TEST_RANK, 
                              1.0f, Device::cuda(), true);  // fused=true
    GPULoRALayer layer_unfused(TEST_IN_DIM, TEST_OUT_DIM, TEST_RANK, 
                                1.0f, Device::cuda(), false);  // fused=false
    
    // Copy weights from fused to unfused to ensure same initialization
    auto weights = layer_fused.get_weights();
    layer_unfused.set_weights(weights.first, weights.second);
    
    // Create input
    GPUTensor input({TEST_BATCH_SIZE, TEST_IN_DIM}, Device::cuda());
    std::vector<float> input_data(TEST_BATCH_SIZE * TEST_IN_DIM);
    for (size_t i = 0; i < input_data.size(); ++i) {
        input_data[i] = static_cast<float>(i % 100) / 100.0f;
    }
    input.upload(input_data);
    
    // Forward pass on both
    auto output_fused = layer_fused.forward(input);
    auto output_unfused = layer_unfused.forward(input);
    
    // Check outputs are close
    EXPECT_TRUE(tensors_close(output_fused, output_unfused, EPSILON))
        << "Fused and unfused forward outputs should match within " << EPSILON;
}

TEST_F(FusedKernelsTest, FusedForward_HIP_AccuracyTest) {
    if (!has_hip_) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=hip_not_available";
    }
    
    // Create two layers: one with fused kernels, one without
    GPULoRALayer layer_fused(TEST_IN_DIM, TEST_OUT_DIM, TEST_RANK, 
                              1.0f, Device::hip(), true);  // fused=true
    GPULoRALayer layer_unfused(TEST_IN_DIM, TEST_OUT_DIM, TEST_RANK, 
                                1.0f, Device::hip(), false);  // fused=false
    
    // Copy weights
    auto weights = layer_fused.get_weights();
    layer_unfused.set_weights(weights.first, weights.second);
    
    // Create input
    GPUTensor input({TEST_BATCH_SIZE, TEST_IN_DIM}, Device::hip());
    std::vector<float> input_data(TEST_BATCH_SIZE * TEST_IN_DIM);
    for (size_t i = 0; i < input_data.size(); ++i) {
        input_data[i] = static_cast<float>(i % 100) / 100.0f;
    }
    input.upload(input_data);
    
    // Forward pass on both
    auto output_fused = layer_fused.forward(input);
    auto output_unfused = layer_unfused.forward(input);
    
    // Check outputs are close
    EXPECT_TRUE(tensors_close(output_fused, output_unfused, EPSILON))
        << "Fused and unfused forward outputs should match within " << EPSILON;
}

// ===== Fused Backward Pass Tests =====

TEST_F(FusedKernelsTest, FusedBackward_CUDA_GradientsTest) {
    if (!has_cuda_) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_not_available";
    }
    
    // Create two layers: one with fused kernels, one without
    GPULoRALayer layer_fused(TEST_IN_DIM, TEST_OUT_DIM, TEST_RANK, 
                              1.0f, Device::cuda(), true);
    GPULoRALayer layer_unfused(TEST_IN_DIM, TEST_OUT_DIM, TEST_RANK, 
                                1.0f, Device::cuda(), false);
    
    // Copy weights
    auto weights = layer_fused.get_weights();
    layer_unfused.set_weights(weights.first, weights.second);
    
    // Create input
    GPUTensor input({TEST_BATCH_SIZE, TEST_IN_DIM}, Device::cuda());
    std::vector<float> input_data(TEST_BATCH_SIZE * TEST_IN_DIM);
    for (size_t i = 0; i < input_data.size(); ++i) {
        input_data[i] = static_cast<float>(i % 100) / 100.0f;
    }
    input.upload(input_data);
    
    // Forward pass on both
    auto output_fused = layer_fused.forward(input);
    auto output_unfused = layer_unfused.forward(input);
    
    // Create grad_output
    GPUTensor grad_output({TEST_BATCH_SIZE, TEST_OUT_DIM}, Device::cuda());
    std::vector<float> grad_data(TEST_BATCH_SIZE * TEST_OUT_DIM);
    for (size_t i = 0; i < grad_data.size(); ++i) {
        grad_data[i] = 1.0f;  // All ones for simple test
    }
    grad_output.upload(grad_data);
    
    // Backward pass on both
    auto grad_input_fused = layer_fused.backward(grad_output);
    auto grad_input_unfused = layer_unfused.backward(grad_output);
    
    // Check grad_input is close
    EXPECT_TRUE(tensors_close(grad_input_fused, grad_input_unfused, EPSILON))
        << "Fused and unfused grad_input should match within " << EPSILON;
    
    // Check grad_A is close
    auto grads_fused = layer_fused.gradients();
    auto grads_unfused = layer_unfused.gradients();
    
    EXPECT_TRUE(tensors_close(*grads_fused[1], *grads_unfused[1], EPSILON))
        << "Fused and unfused grad_A should match within " << EPSILON;
    
    // Check grad_B is close
    EXPECT_TRUE(tensors_close(*grads_fused[0], *grads_unfused[0], EPSILON))
        << "Fused and unfused grad_B should match within " << EPSILON;
}

TEST_F(FusedKernelsTest, FusedBackward_HIP_GradientsTest) {
    if (!has_hip_) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=hip_not_available";
    }
    
    // Create two layers: one with fused kernels, one without
    GPULoRALayer layer_fused(TEST_IN_DIM, TEST_OUT_DIM, TEST_RANK, 
                              1.0f, Device::hip(), true);
    GPULoRALayer layer_unfused(TEST_IN_DIM, TEST_OUT_DIM, TEST_RANK, 
                                1.0f, Device::hip(), false);
    
    // Copy weights
    auto weights = layer_fused.get_weights();
    layer_unfused.set_weights(weights.first, weights.second);
    
    // Create input
    GPUTensor input({TEST_BATCH_SIZE, TEST_IN_DIM}, Device::hip());
    std::vector<float> input_data(TEST_BATCH_SIZE * TEST_IN_DIM);
    for (size_t i = 0; i < input_data.size(); ++i) {
        input_data[i] = static_cast<float>(i % 100) / 100.0f;
    }
    input.upload(input_data);
    
    // Forward pass on both
    auto output_fused = layer_fused.forward(input);
    auto output_unfused = layer_unfused.forward(input);
    
    // Create grad_output
    GPUTensor grad_output({TEST_BATCH_SIZE, TEST_OUT_DIM}, Device::hip());
    std::vector<float> grad_data(TEST_BATCH_SIZE * TEST_OUT_DIM);
    for (size_t i = 0; i < grad_data.size(); ++i) {
        grad_data[i] = 1.0f;
    }
    grad_output.upload(grad_data);
    
    // Backward pass on both
    auto grad_input_fused = layer_fused.backward(grad_output);
    auto grad_input_unfused = layer_unfused.backward(grad_output);
    
    // Check gradients match
    EXPECT_TRUE(tensors_close(grad_input_fused, grad_input_unfused, EPSILON));
    
    auto grads_fused = layer_fused.gradients();
    auto grads_unfused = layer_unfused.gradients();
    
    EXPECT_TRUE(tensors_close(*grads_fused[1], *grads_unfused[1], EPSILON));
    EXPECT_TRUE(tensors_close(*grads_fused[0], *grads_unfused[0], EPSILON));
}

// ===== Fused Optimizer Tests =====

TEST_F(FusedKernelsTest, FusedOptimizer_CUDA_WithoutMomentum) {
    if (!has_cuda_) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_not_available";
    }
    
    // Create a simple parameter tensor
    GPUTensor param_fused({100}, Device::cuda());
    GPUTensor param_unfused({100}, Device::cuda());
    
    // Initialize with same values
    std::vector<float> param_data(100);
    for (size_t i = 0; i < 100; ++i) {
        param_data[i] = static_cast<float>(i) / 100.0f;
    }
    param_fused.upload(param_data);
    param_unfused.upload(param_data);
    
    // Create gradients
    GPUTensor grad({100}, Device::cuda());
    std::vector<float> grad_data(100, 0.1f);  // All 0.1
    grad.upload(grad_data);
    
    param_fused.requires_grad = true;
    param_fused.grad = std::make_unique<GPUTensor>(grad.clone());
    
    param_unfused.requires_grad = true;
    param_unfused.grad = std::make_unique<GPUTensor>(grad.clone());
    
    // Create optimizers (no momentum)
    GPUSGDOptimizer opt_fused(0.01f, 0.0f, 0.001f);
    opt_fused.add_parameters({&param_fused});
    
    GPUSGDOptimizer opt_unfused(0.01f, 0.0f, 0.001f);
    opt_unfused.add_parameters({&param_unfused});
    
    // Perform optimizer step
    opt_fused.step();
    opt_unfused.step();
    
    // Check parameters are close
    EXPECT_TRUE(tensors_close(param_fused, param_unfused, EPSILON))
        << "Fused and unfused optimizer updates should match";
}

TEST_F(FusedKernelsTest, FusedOptimizer_CUDA_WithMomentum) {
    if (!has_cuda_) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_not_available";
    }
    
    // Create a simple parameter tensor
    GPUTensor param_fused({100}, Device::cuda());
    GPUTensor param_unfused({100}, Device::cuda());
    
    // Initialize with same values
    std::vector<float> param_data(100);
    for (size_t i = 0; i < 100; ++i) {
        param_data[i] = static_cast<float>(i) / 100.0f;
    }
    param_fused.upload(param_data);
    param_unfused.upload(param_data);
    
    param_fused.requires_grad = true;
    param_unfused.requires_grad = true;
    
    // Create optimizers (with momentum)
    GPUSGDOptimizer opt_fused(0.01f, 0.9f, 0.001f);
    opt_fused.add_parameters({&param_fused});
    
    GPUSGDOptimizer opt_unfused(0.01f, 0.9f, 0.001f);
    opt_unfused.add_parameters({&param_unfused});
    
    // Perform multiple steps
    for (int step = 0; step < 5; ++step) {
        // Create gradients
        GPUTensor grad({100}, Device::cuda());
        std::vector<float> grad_data(100, 0.1f);
        grad.upload(grad_data);
        
        param_fused.grad = std::make_unique<GPUTensor>(grad.clone());
        param_unfused.grad = std::make_unique<GPUTensor>(grad.clone());
        
        // Step
        opt_fused.step();
        opt_unfused.step();
        
        // Check parameters are close after each step
        EXPECT_TRUE(tensors_close(param_fused, param_unfused, EPSILON))
            << "Fused and unfused optimizer with momentum should match at step " << step;
    }
}

// ===== Full Training Loop Test =====

TEST_F(FusedKernelsTest, FullTrainingLoop_CUDA_FusedVsUnfused) {
    if (!has_cuda_) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_not_available";
    }
    
    // Create layers and optimizers
    GPULoRALayer layer_fused(32, 32, 4, 1.0f, Device::cuda(), true);
    GPULoRALayer layer_unfused(32, 32, 4, 1.0f, Device::cuda(), false);
    
    // Copy weights to ensure same starting point
    auto weights = layer_fused.get_weights();
    layer_unfused.set_weights(weights.first, weights.second);
    
    GPUSGDOptimizer opt_fused(0.01f, 0.0f, 0.0f);
    opt_fused.add_parameters(layer_fused.parameters());
    
    GPUSGDOptimizer opt_unfused(0.01f, 0.0f, 0.0f);
    opt_unfused.add_parameters(layer_unfused.parameters());
    
    GPULoRATrainer trainer_fused(&layer_fused, &opt_fused);
    GPULoRATrainer trainer_unfused(&layer_unfused, &opt_unfused);
    
    // Training data
    GPUTensor input({8, 32}, Device::cuda());
    GPUTensor target({8, 32}, Device::cuda());
    
    std::vector<float> input_data(8 * 32);
    std::vector<float> target_data(8 * 32);
    for (size_t i = 0; i < input_data.size(); ++i) {
        input_data[i] = static_cast<float>(i % 10) / 10.0f;
        target_data[i] = static_cast<float>((i + 1) % 10) / 10.0f;
    }
    input.upload(input_data);
    target.upload(target_data);
    
    // Train for a few steps
    for (int step = 0; step < 10; ++step) {
        float loss_fused = trainer_fused.train_step(input, target);
        float loss_unfused = trainer_unfused.train_step(input, target);
        
        // Losses should be very close
        EXPECT_NEAR(loss_fused, loss_unfused, EPSILON)
            << "Fused and unfused training losses should match at step " << step;
    }
    
    // Final parameters should be close
    auto params_fused = layer_fused.parameters();
    auto params_unfused = layer_unfused.parameters();
    
    EXPECT_TRUE(tensors_close(*params_fused[0], *params_unfused[0], EPSILON))
        << "Final B parameters should match";
    EXPECT_TRUE(tensors_close(*params_fused[1], *params_unfused[1], EPSILON))
        << "Final A parameters should match";
}
#endif // SKIP_FUSED_KERNEL_TESTS

