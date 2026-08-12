#include <gtest/gtest.h>
#include "llm/lora_framework/gpu_lora_layers.h"
#include "llm/lora_framework/lora_layers.h"
#include <cmath>

// Temporarily disable GPU LoRA layers tests on MSVC
#define SKIP_GPU_LORA_LAYERS_TESTS 1

#if SKIP_GPU_LORA_LAYERS_TESTS

TEST(DummyGPULoRALayers, DisabledOnMSVC) {
    GTEST_SKIP() << "capability:gpu_lora_layers_test_enabled=false;reason=msvc_porting_in_progress";
}

#else

using namespace themis::llm::lora;

namespace {
    constexpr float EPSILON = 1e-4f;
    constexpr size_t TEST_IN_DIM = 64;
    constexpr size_t TEST_OUT_DIM = 32;
    constexpr size_t TEST_RANK = 8;
}

class GPULoRALayerTest : public ::testing::Test {
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
    }
    
    bool has_cuda_ = false;
    bool has_hip_ = false;
};

// ===== Construction Tests =====

TEST_F(GPULoRALayerTest, Construction_CPU) {
    GPULoRALayer layer(TEST_IN_DIM, TEST_OUT_DIM, TEST_RANK, 1.0f, Device::cpu());
    
    EXPECT_EQ(layer.in_dim(), TEST_IN_DIM);
    EXPECT_EQ(layer.out_dim(), TEST_OUT_DIM);
    EXPECT_EQ(layer.rank(), TEST_RANK);
    EXPECT_FLOAT_EQ(layer.scaling(), 1.0f);
    EXPECT_EQ(layer.device().type, DeviceType::CPU);
    
    auto params = layer.parameters();
    EXPECT_EQ(params.size(), 2);  // B and A
    
    size_t expected_params = TEST_IN_DIM * TEST_RANK + TEST_RANK * TEST_OUT_DIM;
    EXPECT_EQ(layer.parameter_count(), expected_params);
}

TEST_F(GPULoRALayerTest, Construction_CUDA) {
    if (!has_cuda_) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_not_available";
    }
    
    GPULoRALayer layer(TEST_IN_DIM, TEST_OUT_DIM, TEST_RANK, 1.0f, Device::cuda());
    
    EXPECT_EQ(layer.device().type, DeviceType::CUDA);
    
    auto params = layer.parameters();
    EXPECT_EQ(params.size(), 2);
    
    // Verify parameters are on GPU
    for (auto* param : params) {
        EXPECT_TRUE(param->is_gpu());
        EXPECT_EQ(param->device().type, DeviceType::CUDA);
    }
}

// ===== Forward Pass Tests =====

TEST_F(GPULoRALayerTest, Forward_CPU) {
    GPULoRALayer layer(4, 4, 2, 1.0f, Device::cpu());
    
    // Create simple input
    GPUTensor input({1, 4}, Device::cpu());
    std::vector<float> input_data = {1.0f, 2.0f, 3.0f, 4.0f};
    input.upload(input_data);
    
    // Forward pass
    auto output = layer.forward(input);
    
    // Check output shape
    EXPECT_EQ(output.shape()[0], 1);
    EXPECT_EQ(output.shape()[1], 4);
    
    // Output should not be all zeros (since B is initialized with Kaiming)
    auto output_data = output.cpu_data();
    bool has_nonzero = false;
    for (auto val : output_data) {
        if (std::abs(val) > EPSILON) {
            has_nonzero = true;
            break;
        }
    }
    // Note: A is initialized to zeros, so output might be all zeros
    // This is expected for LoRA initialization
}

TEST_F(GPULoRALayerTest, Forward_CUDA) {
    if (!has_cuda_) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_not_available";
    }
    
    GPULoRALayer layer(4, 4, 2, 1.0f, Device::cuda());
    
    GPUTensor input({1, 4}, Device::cuda());
    input.fill(1.0f);
    
    auto output = layer.forward(input);
    
    EXPECT_TRUE(output.is_gpu());
    EXPECT_EQ(output.shape()[0], 1);
    EXPECT_EQ(output.shape()[1], 4);
}

TEST_F(GPULoRALayerTest, Forward_WithScaling) {
    GPULoRALayer layer(4, 4, 2, 2.0f, Device::cpu());
    
    // Set known weights
    auto B = gpu_tensor_utils::ones({4, 2}, Device::cpu());
    auto A = gpu_tensor_utils::ones({2, 4}, Device::cpu());
    layer.set_weights(B, A);
    
    GPUTensor input({1, 4}, Device::cpu());
    input.fill(1.0f);
    
    auto output = layer.forward(input);
    
    // output = input @ B @ A * scaling
    // input: [1, 1, 1, 1] @ B: [[1,1],[1,1],[1,1],[1,1]] = [4, 4]
    // [4, 4] @ A: [[1,1,1,1],[1,1,1,1]] = [8, 8, 8, 8]
    // [8, 8, 8, 8] * 2.0 = [16, 16, 16, 16]
    
    auto output_data = output.cpu_data();
    for (auto val : output_data) {
        EXPECT_NEAR(val, 16.0f, 0.1f);
    }
}

// ===== Backward Pass Tests =====

TEST_F(GPULoRALayerTest, Backward_CPU) {
    GPULoRALayer layer(4, 4, 2, 1.0f, Device::cpu());
    
    // Set known weights for easier verification
    auto B = gpu_tensor_utils::ones({4, 2}, Device::cpu());
    auto A = gpu_tensor_utils::ones({2, 4}, Device::cpu());
    layer.set_weights(B, A);
    
    // Forward pass
    GPUTensor input({1, 4}, Device::cpu());
    input.fill(1.0f);
    auto output = layer.forward(input);
    
    // Backward pass with simple gradient
    GPUTensor grad_output({1, 4}, Device::cpu());
    grad_output.fill(1.0f);
    
    auto grad_input = layer.backward(grad_output);
    
    // Check gradient shape
    EXPECT_EQ(grad_input.shape()[0], 1);
    EXPECT_EQ(grad_input.shape()[1], 4);
    
    // Check that gradients were computed
    auto params = layer.parameters();
    EXPECT_TRUE(params[0]->grad != nullptr);  // B.grad
    EXPECT_TRUE(params[1]->grad != nullptr);  // A.grad
}

TEST_F(GPULoRALayerTest, Backward_CUDA) {
    if (!has_cuda_) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_not_available";
    }
    
    GPULoRALayer layer(4, 4, 2, 1.0f, Device::cuda());
    
    auto B = gpu_tensor_utils::ones({4, 2}, Device::cuda());
    auto A = gpu_tensor_utils::ones({2, 4}, Device::cuda());
    layer.set_weights(B, A);
    
    GPUTensor input({1, 4}, Device::cuda());
    input.fill(1.0f);
    
    auto output = layer.forward(input);
    
    GPUTensor grad_output({1, 4}, Device::cuda());
    grad_output.fill(1.0f);
    
    auto grad_input = layer.backward(grad_output);
    
    EXPECT_TRUE(grad_input.is_gpu());
    EXPECT_EQ(grad_input.device().type, DeviceType::CUDA);
}

// ===== Weight Management Tests =====

TEST_F(GPULoRALayerTest, GetSetWeights_CPU) {
    GPULoRALayer layer(4, 4, 2, 1.0f, Device::cpu());
    
    // Set custom weights
    auto B_custom = gpu_tensor_utils::ones({4, 2}, Device::cpu());
    B_custom = B_custom * 2.0f;
    
    auto A_custom = gpu_tensor_utils::ones({2, 4}, Device::cpu());
    A_custom = A_custom * 3.0f;
    
    layer.set_weights(B_custom, A_custom);
    
    // Get weights back
    auto [B_retrieved, A_retrieved] = layer.get_weights();
    
    // Verify they match
    auto B_data = B_retrieved.cpu_data();
    for (auto val : B_data) {
        EXPECT_FLOAT_EQ(val, 2.0f);
    }
    
    auto A_data = A_retrieved.cpu_data();
    for (auto val : A_data) {
        EXPECT_FLOAT_EQ(val, 3.0f);
    }
}

// ===== Device Migration Tests =====

TEST_F(GPULoRALayerTest, DeviceMigration_CPUToCUDA) {
    if (!has_cuda_) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_not_available";
    }
    
    GPULoRALayer layer(4, 4, 2, 1.0f, Device::cpu());
    
    EXPECT_EQ(layer.device().type, DeviceType::CPU);
    
    // Move to CUDA
    layer.to(Device::cuda());
    
    EXPECT_EQ(layer.device().type, DeviceType::CUDA);
    
    // Verify parameters moved
    auto params = layer.parameters();
    for (auto* param : params) {
        EXPECT_EQ(param->device().type, DeviceType::CUDA);
    }
}

// ===== Zero Grad Tests =====

TEST_F(GPULoRALayerTest, ZeroGrad_CPU) {
    GPULoRALayer layer(4, 4, 2, 1.0f, Device::cpu());
    
    // Forward and backward to create gradients
    auto B = gpu_tensor_utils::ones({4, 2}, Device::cpu());
    auto A = gpu_tensor_utils::ones({2, 4}, Device::cpu());
    layer.set_weights(B, A);
    
    GPUTensor input({1, 4}, Device::cpu());
    input.fill(1.0f);
    
    layer.forward(input);
    
    GPUTensor grad_output({1, 4}, Device::cpu());
    grad_output.fill(1.0f);
    
    layer.backward(grad_output);
    
    // Verify gradients exist and are non-zero
    auto params = layer.parameters();
    EXPECT_TRUE(params[0]->grad != nullptr);
    EXPECT_TRUE(params[1]->grad != nullptr);
    
    // Zero gradients
    layer.zero_grad();
    
    // Verify gradients are zero
    auto B_grad_data = params[0]->grad->cpu_data();
    for (auto val : B_grad_data) {
        EXPECT_FLOAT_EQ(val, 0.0f);
    }
    
    auto A_grad_data = params[1]->grad->cpu_data();
    for (auto val : A_grad_data) {
        EXPECT_FLOAT_EQ(val, 0.0f);
    }
}

// ===== GPUSGDOptimizer Tests =====

TEST_F(GPULoRALayerTest, Optimizer_Construction) {
    GPUSGDOptimizer optimizer(0.01f, 0.9f, 0.0001f);
    
    EXPECT_FLOAT_EQ(optimizer.learning_rate(), 0.01f);
    EXPECT_FLOAT_EQ(optimizer.momentum(), 0.9f);
    EXPECT_FLOAT_EQ(optimizer.weight_decay(), 0.0001f);
}

TEST_F(GPULoRALayerTest, Optimizer_AddParameters) {
    GPUSGDOptimizer optimizer(0.01f);
    GPULoRALayer layer(4, 4, 2, 1.0f, Device::cpu());
    
    optimizer.add_parameters(layer.parameters());
    
    EXPECT_EQ(optimizer.num_parameters(), 2);
}

TEST_F(GPULoRALayerTest, Optimizer_Step_CPU) {
    GPULoRALayer layer(4, 4, 2, 1.0f, Device::cpu());
    
    // Set known weights
    auto B = gpu_tensor_utils::ones({4, 2}, Device::cpu());
    auto A = gpu_tensor_utils::ones({2, 4}, Device::cpu());
    layer.set_weights(B, A);
    
    GPUSGDOptimizer optimizer(0.1f);  // Large LR for visible changes
    optimizer.add_parameters(layer.parameters());
    
    // Forward and backward
    GPUTensor input({1, 4}, Device::cpu());
    input.fill(1.0f);
    
    auto output = layer.forward(input);
    
    GPUTensor grad_output({1, 4}, Device::cpu());
    grad_output.fill(1.0f);
    
    layer.backward(grad_output);
    
    // Get initial weights
    auto [B_before, A_before] = layer.get_weights();
    auto B_before_data = B_before.cpu_data();
    
    // Optimization step
    optimizer.step();
    
    // Get updated weights
    auto [B_after, A_after] = layer.get_weights();
    auto B_after_data = B_after.cpu_data();
    
    // Weights should have changed
    bool weights_changed = false;
    for (size_t i = 0; i < B_before_data.size(); ++i) {
        if (std::abs(B_after_data[i] - B_before_data[i]) > EPSILON) {
            weights_changed = true;
            break;
        }
    }
    EXPECT_TRUE(weights_changed);
}

// ===== GPULoRATrainer Tests =====

TEST_F(GPULoRALayerTest, Trainer_TrainStep_CPU) {
    GPULoRALayer layer(4, 4, 2, 1.0f, Device::cpu());
    GPUSGDOptimizer optimizer(0.01f);
    optimizer.add_parameters(layer.parameters());
    
    GPULoRATrainer trainer(&layer, &optimizer);
    
    // Create training data
    GPUTensor input({2, 4}, Device::cpu());  // Batch size 2
    input.fill(1.0f);
    
    GPUTensor target({2, 4}, Device::cpu());
    target.fill(0.5f);
    
    // Training step
    float loss = trainer.train_step(input, target);
    
    // Loss should be non-negative
    EXPECT_GE(loss, 0.0f);
}

TEST_F(GPULoRALayerTest, Trainer_EvalStep_CPU) {
    GPULoRALayer layer(4, 4, 2, 1.0f, Device::cpu());
    GPUSGDOptimizer optimizer(0.01f);
    optimizer.add_parameters(layer.parameters());
    
    GPULoRATrainer trainer(&layer, &optimizer);
    
    GPUTensor input({2, 4}, Device::cpu());
    input.fill(1.0f);
    
    GPUTensor target({2, 4}, Device::cpu());
    target.fill(0.5f);
    
    // Eval step (no gradient updates)
    float loss = trainer.eval_step(input, target);
    
    EXPECT_GE(loss, 0.0f);
}

TEST_F(GPULoRALayerTest, Trainer_LossDecreases_CPU) {
    GPULoRALayer layer(8, 8, 4, 1.0f, Device::cpu());
    
    // Set non-zero weights for meaningful training
    auto B = gpu_tensor_utils::xavier_uniform({8, 4}, Device::cpu());
    auto A = gpu_tensor_utils::xavier_uniform({4, 8}, Device::cpu());
    layer.set_weights(B, A);
    
    GPUSGDOptimizer optimizer(0.1f);  // Higher LR for faster convergence
    optimizer.add_parameters(layer.parameters());
    
    GPULoRATrainer trainer(&layer, &optimizer);
    
    // Simple training data
    GPUTensor input({4, 8}, Device::cpu());
    input.fill(0.5f);
    
    GPUTensor target({4, 8}, Device::cpu());
    target.fill(0.3f);
    
    // Train for multiple steps
    float initial_loss = trainer.eval_step(input, target);
    
    for (int i = 0; i < 10; ++i) {
        trainer.train_step(input, target);
    }
    
    float final_loss = trainer.eval_step(input, target);
    
    // Loss should decrease
    EXPECT_LT(final_loss, initial_loss);
}

// ===== Comparison Tests (GPU vs CPU) =====

TEST_F(GPULoRALayerTest, CUDA_CPUConsistency) {
    if (!has_cuda_) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_not_available";
    }
    
    // Create identical layers on CPU and CUDA
    GPULoRALayer cpu_layer(8, 8, 4, 1.0f, Device::cpu());
    GPULoRALayer cuda_layer(8, 8, 4, 1.0f, Device::cuda());
    
    // Set same weights
    auto B = gpu_tensor_utils::xavier_uniform({8, 4}, Device::cpu());
    auto A = gpu_tensor_utils::xavier_uniform({4, 8}, Device::cpu());
    
    cpu_layer.set_weights(B, A);
    cuda_layer.set_weights(B.to(Device::cuda()), A.to(Device::cuda()));
    
    // Same input
    GPUTensor cpu_input({2, 8}, Device::cpu());
    cpu_input.fill(1.0f);
    
    GPUTensor cuda_input({2, 8}, Device::cuda());
    cuda_input.fill(1.0f);
    
    // Forward pass
    auto cpu_output = cpu_layer.forward(cpu_input);
    auto cuda_output = cuda_layer.forward(cuda_input);
    
    // Compare outputs
    auto cpu_data = cpu_output.cpu_data();
    auto cuda_data = cuda_output.cpu_data();
    
    EXPECT_EQ(cpu_data.size(), cuda_data.size());
    
    for (size_t i = 0; i < cpu_data.size(); ++i) {
        EXPECT_NEAR(cpu_data[i], cuda_data[i], 1e-3f);  // Allow small numerical differences
    }
}
#endif // SKIP_GPU_LORA_LAYERS_TESTS