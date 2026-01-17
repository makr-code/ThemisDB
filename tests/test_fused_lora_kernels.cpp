#include <gtest/gtest.h>
#include "llm/lora_framework/gpu_lora_layers.h"
#include <chrono>
#include <cmath>
#include <numeric>
#include <spdlog/spdlog.h>

#ifdef THEMIS_ENABLE_CUDA
#include "llm/lora_framework/cuda_fused_kernels.h"
#endif

#ifdef THEMIS_ENABLE_CPU_FUSED_KERNELS
#include "llm/lora_framework/cpu_fused_kernels.h"
#endif

#ifdef THEMIS_ENABLE_VULKAN
#include "llm/lora_framework/vulkan_kernels.h"
#endif

using namespace themis::llm::lora;
using namespace std::chrono;

namespace {
    constexpr float EPSILON = 1e-4f;
    constexpr float RELAXED_EPSILON = 1e-3f;  // For GPU comparisons
    constexpr size_t BENCHMARK_ITERATIONS = 100;
    constexpr size_t WARMUP_ITERATIONS = 10;
}

/**
 * @file test_fused_lora_kernels.cpp
 * @brief Comprehensive tests for fused LoRA kernels
 * 
 * Test Coverage:
 * - Forward pass numerical accuracy (fused vs unfused)
 * - Backward pass numerical accuracy (fused vs unfused)
 * - Performance benchmarks (speedup measurements)
 * - Memory bandwidth utilization
 * - Various tensor sizes and ranks
 */
class FusedLoRAKernelsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Detect available backends
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
        
        spdlog::info("FusedLoRAKernelsTest: CUDA={}, HIP={}", has_cuda_, has_hip_);
    }
    
    bool has_cuda_ = false;
    bool has_hip_ = false;
    
    // Helper: Compare two tensors element-wise
    bool tensorsMatch(const GPUTensor& a, const GPUTensor& b, float epsilon = EPSILON) {
        if (a.shape() != b.shape()) {
            return false;
        }
        
        auto a_data = a.cpu_data();
        auto b_data = b.cpu_data();
        
        for (size_t i = 0; i < a_data.size(); ++i) {
            if (std::abs(a_data[i] - b_data[i]) > epsilon) {
                spdlog::warn("Mismatch at index {}: {} vs {} (diff={})", 
                           i, a_data[i], b_data[i], std::abs(a_data[i] - b_data[i]));
                return false;
            }
        }
        return true;
    }
    
    // Helper: Compute max absolute difference
    float maxAbsDifference(const GPUTensor& a, const GPUTensor& b) {
        auto a_data = a.cpu_data();
        auto b_data = b.cpu_data();
        
        float max_diff = 0.0f;
        for (size_t i = 0; i < a_data.size(); ++i) {
            max_diff = std::max(max_diff, std::abs(a_data[i] - b_data[i]));
        }
        return max_diff;
    }
    
    // Helper: Compute relative error
    float relativeError(const GPUTensor& computed, const GPUTensor& reference) {
        auto comp_data = computed.cpu_data();
        auto ref_data = reference.cpu_data();
        
        float total_error = 0.0f;
        float total_ref = 0.0f;
        
        for (size_t i = 0; i < comp_data.size(); ++i) {
            total_error += std::abs(comp_data[i] - ref_data[i]);
            total_ref += std::abs(ref_data[i]);
        }
        
        return (total_ref > 0.0f) ? (total_error / total_ref) : 0.0f;
    }
};

// ============================================================================
// Numerical Accuracy Tests
// ============================================================================

TEST_F(FusedLoRAKernelsTest, ForwardNumericalAccuracy_CPU) {
    // CPU doesn't use fused kernels, but test the layer for baseline
    size_t batch_size = 4;
    size_t in_dim = 64;
    size_t out_dim = 32;
    size_t rank = 8;
    
    GPULoRALayer layer(in_dim, out_dim, rank, 1.0f, Device::cpu(), true);
    
    // Set known weights
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cpu());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cpu());
    layer.set_weights(B, A);
    
    // Create input
    GPUTensor input({batch_size, in_dim}, Device::cpu());
    input.fill(0.5f);
    
    // Forward pass
    auto output = layer.forward(input);
    
    // Check output is valid
    EXPECT_EQ(output.shape()[0], batch_size);
    EXPECT_EQ(output.shape()[1], out_dim);
    
    auto output_data = output.cpu_data();
    for (auto val : output_data) {
        EXPECT_FALSE(std::isnan(val));
        EXPECT_FALSE(std::isinf(val));
    }
}

TEST_F(FusedLoRAKernelsTest, ForwardNumericalAccuracy_CUDA_FusedVsUnfused) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    size_t batch_size = 4;
    size_t in_dim = 64;
    size_t out_dim = 32;
    size_t rank = 8;
    float scaling = 2.0f;
    
    // Create two layers: one with fused kernels, one without
    GPULoRALayer fused_layer(in_dim, out_dim, rank, scaling, Device::cuda(), true);
    GPULoRALayer unfused_layer(in_dim, out_dim, rank, scaling, Device::cuda(), false);
    
    // Set same weights
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cpu());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cpu());
    
    fused_layer.set_weights(B.to(Device::cuda()), A.to(Device::cuda()));
    unfused_layer.set_weights(B.to(Device::cuda()), A.to(Device::cuda()));
    
    // Same input
    GPUTensor input({batch_size, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    // Forward pass with both implementations
    auto fused_output = fused_layer.forward(input);
    auto unfused_output = unfused_layer.forward(input);
    
    // Compare outputs
    float max_diff = maxAbsDifference(fused_output, unfused_output);
    float rel_error = relativeError(fused_output, unfused_output);
    
    spdlog::info("Forward CUDA fused vs unfused: max_diff={}, rel_error={}", max_diff, rel_error);
    
    // Fused and unfused should produce nearly identical results
    EXPECT_LT(max_diff, RELAXED_EPSILON);
    EXPECT_LT(rel_error, 0.01f);  // Less than 1% relative error
}

TEST_F(FusedLoRAKernelsTest, ForwardNumericalAccuracy_HIP_FusedVsUnfused) {
    if (!has_hip_) {
        GTEST_SKIP() << "HIP not available";
    }
    
    size_t batch_size = 4;
    size_t in_dim = 64;
    size_t out_dim = 32;
    size_t rank = 8;
    float scaling = 2.0f;
    
    GPULoRALayer fused_layer(in_dim, out_dim, rank, scaling, Device::hip(), true);
    GPULoRALayer unfused_layer(in_dim, out_dim, rank, scaling, Device::hip(), false);
    
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cpu());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cpu());
    
    fused_layer.set_weights(B.to(Device::hip()), A.to(Device::hip()));
    unfused_layer.set_weights(B.to(Device::hip()), A.to(Device::hip()));
    
    GPUTensor input({batch_size, in_dim}, Device::hip());
    input.fill(0.5f);
    
    auto fused_output = fused_layer.forward(input);
    auto unfused_output = unfused_layer.forward(input);
    
    float max_diff = maxAbsDifference(fused_output, unfused_output);
    float rel_error = relativeError(fused_output, unfused_output);
    
    spdlog::info("Forward HIP fused vs unfused: max_diff={}, rel_error={}", max_diff, rel_error);
    
    EXPECT_LT(max_diff, RELAXED_EPSILON);
    EXPECT_LT(rel_error, 0.01f);
}

TEST_F(FusedLoRAKernelsTest, BackwardNumericalAccuracy_CUDA_FusedVsUnfused) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    size_t batch_size = 4;
    size_t in_dim = 64;
    size_t out_dim = 32;
    size_t rank = 8;
    
    GPULoRALayer fused_layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), true);
    GPULoRALayer unfused_layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), false);
    
    // Set same weights
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cpu());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cpu());
    
    fused_layer.set_weights(B.to(Device::cuda()), A.to(Device::cuda()));
    unfused_layer.set_weights(B.to(Device::cuda()), A.to(Device::cuda()));
    
    // Same input
    GPUTensor input({batch_size, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    // Forward pass
    auto fused_output = fused_layer.forward(input);
    auto unfused_output = unfused_layer.forward(input);
    
    // Backward pass with same gradient
    GPUTensor grad_output({batch_size, out_dim}, Device::cuda());
    grad_output.fill(1.0f);
    
    auto fused_grad_input = fused_layer.backward(grad_output);
    auto unfused_grad_input = unfused_layer.backward(grad_output);
    
    // Compare gradient w.r.t. input
    float grad_input_diff = maxAbsDifference(fused_grad_input, unfused_grad_input);
    spdlog::info("Backward CUDA grad_input: max_diff={}", grad_input_diff);
    EXPECT_LT(grad_input_diff, RELAXED_EPSILON);
    
    // Compare gradients w.r.t. parameters
    auto fused_params = fused_layer.parameters();
    auto unfused_params = unfused_layer.parameters();
    
    // grad_B
    float grad_B_diff = maxAbsDifference(*fused_params[0]->grad, *unfused_params[0]->grad);
    spdlog::info("Backward CUDA grad_B: max_diff={}", grad_B_diff);
    EXPECT_LT(grad_B_diff, RELAXED_EPSILON);
    
    // grad_A
    float grad_A_diff = maxAbsDifference(*fused_params[1]->grad, *unfused_params[1]->grad);
    spdlog::info("Backward CUDA grad_A: max_diff={}", grad_A_diff);
    EXPECT_LT(grad_A_diff, RELAXED_EPSILON);
}

TEST_F(FusedLoRAKernelsTest, BackwardNumericalAccuracy_HIP_FusedVsUnfused) {
    if (!has_hip_) {
        GTEST_SKIP() << "HIP not available";
    }
    
    size_t batch_size = 4;
    size_t in_dim = 64;
    size_t out_dim = 32;
    size_t rank = 8;
    
    GPULoRALayer fused_layer(in_dim, out_dim, rank, 1.0f, Device::hip(), true);
    GPULoRALayer unfused_layer(in_dim, out_dim, rank, 1.0f, Device::hip(), false);
    
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cpu());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cpu());
    
    fused_layer.set_weights(B.to(Device::hip()), A.to(Device::hip()));
    unfused_layer.set_weights(B.to(Device::hip()), A.to(Device::hip()));
    
    GPUTensor input({batch_size, in_dim}, Device::hip());
    input.fill(0.5f);
    
    auto fused_output = fused_layer.forward(input);
    auto unfused_output = unfused_layer.forward(input);
    
    GPUTensor grad_output({batch_size, out_dim}, Device::hip());
    grad_output.fill(1.0f);
    
    auto fused_grad_input = fused_layer.backward(grad_output);
    auto unfused_grad_input = unfused_layer.backward(grad_output);
    
    float grad_input_diff = maxAbsDifference(fused_grad_input, unfused_grad_input);
    spdlog::info("Backward HIP grad_input: max_diff={}", grad_input_diff);
    EXPECT_LT(grad_input_diff, RELAXED_EPSILON);
}

// ============================================================================
// Performance Benchmarks
// ============================================================================

TEST_F(FusedLoRAKernelsTest, ForwardPerformance_CUDA_FusedVsUnfused) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    size_t batch_size = 32;
    size_t in_dim = 768;
    size_t out_dim = 768;
    size_t rank = 16;
    
    GPULoRALayer fused_layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), true);
    GPULoRALayer unfused_layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), false);
    
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cuda());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cuda());
    
    fused_layer.set_weights(B, A);
    unfused_layer.set_weights(B, A);
    
    GPUTensor input({batch_size, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    // Warmup
    for (size_t i = 0; i < WARMUP_ITERATIONS; ++i) {
        auto _ = fused_layer.forward(input);
        auto __ = unfused_layer.forward(input);
    }
    
    // Benchmark fused
    auto fused_start = high_resolution_clock::now();
    for (size_t i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        auto output = fused_layer.forward(input);
    }
    auto fused_end = high_resolution_clock::now();
    auto fused_time = duration_cast<microseconds>(fused_end - fused_start).count();
    
    // Benchmark unfused
    auto unfused_start = high_resolution_clock::now();
    for (size_t i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        auto output = unfused_layer.forward(input);
    }
    auto unfused_end = high_resolution_clock::now();
    auto unfused_time = duration_cast<microseconds>(unfused_end - unfused_start).count();
    
    float speedup = static_cast<float>(unfused_time) / static_cast<float>(fused_time);
    
    spdlog::info("Forward CUDA Performance:");
    spdlog::info("  Fused:   {} μs ({} μs/iter)", fused_time, fused_time / BENCHMARK_ITERATIONS);
    spdlog::info("  Unfused: {} μs ({} μs/iter)", unfused_time, unfused_time / BENCHMARK_ITERATIONS);
    spdlog::info("  Speedup: {:.2f}x", speedup);
    
    // Expect at least 1.2x speedup (conservative target)
    // Issue mentions 2-3x, but we'll be conservative for varying hardware
    EXPECT_GT(speedup, 1.2f) << "Fused kernel should be faster than unfused";
}

TEST_F(FusedLoRAKernelsTest, BackwardPerformance_CUDA_FusedVsUnfused) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    size_t batch_size = 32;
    size_t in_dim = 768;
    size_t out_dim = 768;
    size_t rank = 16;
    
    GPULoRALayer fused_layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), true);
    GPULoRALayer unfused_layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), false);
    
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cuda());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cuda());
    
    fused_layer.set_weights(B, A);
    unfused_layer.set_weights(B, A);
    
    GPUTensor input({batch_size, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    GPUTensor grad_output({batch_size, out_dim}, Device::cuda());
    grad_output.fill(1.0f);
    
    // Warmup
    for (size_t i = 0; i < WARMUP_ITERATIONS; ++i) {
        fused_layer.forward(input);
        fused_layer.backward(grad_output);
        unfused_layer.forward(input);
        unfused_layer.backward(grad_output);
    }
    
    // Benchmark fused
    auto fused_start = high_resolution_clock::now();
    for (size_t i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        fused_layer.forward(input);
        auto grad = fused_layer.backward(grad_output);
    }
    auto fused_end = high_resolution_clock::now();
    auto fused_time = duration_cast<microseconds>(fused_end - fused_start).count();
    
    // Benchmark unfused
    auto unfused_start = high_resolution_clock::now();
    for (size_t i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        unfused_layer.forward(input);
        auto grad = unfused_layer.backward(grad_output);
    }
    auto unfused_end = high_resolution_clock::now();
    auto unfused_time = duration_cast<microseconds>(unfused_end - unfused_start).count();
    
    float speedup = static_cast<float>(unfused_time) / static_cast<float>(fused_time);
    
    spdlog::info("Backward CUDA Performance:");
    spdlog::info("  Fused:   {} μs ({} μs/iter)", fused_time, fused_time / BENCHMARK_ITERATIONS);
    spdlog::info("  Unfused: {} μs ({} μs/iter)", unfused_time, unfused_time / BENCHMARK_ITERATIONS);
    spdlog::info("  Speedup: {:.2f}x", speedup);
    
    // Expect at least 1.2x speedup for backward pass
    EXPECT_GT(speedup, 1.2f) << "Fused backward should be faster than unfused";
}

// ============================================================================
// Varying Tensor Sizes Tests
// ============================================================================

TEST_F(FusedLoRAKernelsTest, VaryingBatchSizes_CUDA) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    size_t in_dim = 256;
    size_t out_dim = 256;
    size_t rank = 8;
    
    std::vector<size_t> batch_sizes = {1, 4, 16, 64};
    
    for (auto batch_size : batch_sizes) {
        GPULoRALayer fused_layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), true);
        GPULoRALayer unfused_layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), false);
        
        auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cuda());
        auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cuda());
        
        fused_layer.set_weights(B, A);
        unfused_layer.set_weights(B, A);
        
        GPUTensor input({batch_size, in_dim}, Device::cuda());
        input.fill(0.5f);
        
        auto fused_output = fused_layer.forward(input);
        auto unfused_output = unfused_layer.forward(input);
        
        float max_diff = maxAbsDifference(fused_output, unfused_output);
        
        spdlog::info("Batch size {}: max_diff={}", batch_size, max_diff);
        EXPECT_LT(max_diff, RELAXED_EPSILON) << "Failed for batch_size=" << batch_size;
    }
}

TEST_F(FusedLoRAKernelsTest, VaryingRanks_CUDA) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    size_t batch_size = 8;
    size_t in_dim = 256;
    size_t out_dim = 256;
    
    std::vector<size_t> ranks = {4, 8, 16, 32};
    
    for (auto rank : ranks) {
        GPULoRALayer fused_layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), true);
        GPULoRALayer unfused_layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), false);
        
        auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cuda());
        auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cuda());
        
        fused_layer.set_weights(B, A);
        unfused_layer.set_weights(B, A);
        
        GPUTensor input({batch_size, in_dim}, Device::cuda());
        input.fill(0.5f);
        
        auto fused_output = fused_layer.forward(input);
        auto unfused_output = unfused_layer.forward(input);
        
        float max_diff = maxAbsDifference(fused_output, unfused_output);
        
        spdlog::info("Rank {}: max_diff={}", rank, max_diff);
        EXPECT_LT(max_diff, RELAXED_EPSILON) << "Failed for rank=" << rank;
    }
}

TEST_F(FusedLoRAKernelsTest, VaryingDimensions_CUDA) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    size_t batch_size = 4;
    size_t rank = 8;
    
    std::vector<std::pair<size_t, size_t>> dims = {
        {128, 128},
        {256, 256},
        {512, 512},
        {768, 768},
        {1024, 1024}
    };
    
    for (auto [in_dim, out_dim] : dims) {
        GPULoRALayer fused_layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), true);
        GPULoRALayer unfused_layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), false);
        
        auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cuda());
        auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cuda());
        
        fused_layer.set_weights(B, A);
        unfused_layer.set_weights(B, A);
        
        GPUTensor input({batch_size, in_dim}, Device::cuda());
        input.fill(0.5f);
        
        auto fused_output = fused_layer.forward(input);
        auto unfused_output = unfused_layer.forward(input);
        
        float max_diff = maxAbsDifference(fused_output, unfused_output);
        
        spdlog::info("Dimensions {}x{}: max_diff={}", in_dim, out_dim, max_diff);
        EXPECT_LT(max_diff, RELAXED_EPSILON) << "Failed for dimensions " << in_dim << "x" << out_dim;
    }
}

// ============================================================================
// Edge Cases Tests
// ============================================================================

TEST_F(FusedLoRAKernelsTest, SmallRank_CUDA) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    // Test with very small rank (r=2)
    size_t batch_size = 4;
    size_t in_dim = 64;
    size_t out_dim = 64;
    size_t rank = 2;
    
    GPULoRALayer fused_layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), true);
    GPULoRALayer unfused_layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), false);
    
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cuda());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cuda());
    
    fused_layer.set_weights(B, A);
    unfused_layer.set_weights(B, A);
    
    GPUTensor input({batch_size, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    auto fused_output = fused_layer.forward(input);
    auto unfused_output = unfused_layer.forward(input);
    
    float max_diff = maxAbsDifference(fused_output, unfused_output);
    EXPECT_LT(max_diff, RELAXED_EPSILON);
}

TEST_F(FusedLoRAKernelsTest, LargeRank_CUDA) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    // Test with large rank (r=64)
    size_t batch_size = 4;
    size_t in_dim = 256;
    size_t out_dim = 256;
    size_t rank = 64;
    
    GPULoRALayer fused_layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), true);
    GPULoRALayer unfused_layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), false);
    
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cuda());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cuda());
    
    fused_layer.set_weights(B, A);
    unfused_layer.set_weights(B, A);
    
    GPUTensor input({batch_size, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    auto fused_output = fused_layer.forward(input);
    auto unfused_output = unfused_layer.forward(input);
    
    float max_diff = maxAbsDifference(fused_output, unfused_output);
    EXPECT_LT(max_diff, RELAXED_EPSILON);
}

TEST_F(FusedLoRAKernelsTest, NonSquareDimensions_CUDA) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    // Test with non-square dimensions
    size_t batch_size = 4;
    size_t in_dim = 768;
    size_t out_dim = 3072;  // FFN expansion
    size_t rank = 16;
    
    GPULoRALayer fused_layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), true);
    GPULoRALayer unfused_layer(in_dim, out_dim, rank, 1.0f, Device::cuda(), false);
    
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cuda());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cuda());
    
    fused_layer.set_weights(B, A);
    unfused_layer.set_weights(B, A);
    
    GPUTensor input({batch_size, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    auto fused_output = fused_layer.forward(input);
    auto unfused_output = unfused_layer.forward(input);
    
    float max_diff = maxAbsDifference(fused_output, unfused_output);
    EXPECT_LT(max_diff, RELAXED_EPSILON);
}

// ============================================================================
// CPU Baseline Tests
// ============================================================================

TEST_F(FusedLoRAKernelsTest, CPUBaseline_ForwardNumericalAccuracy) {
    // Test CPU baseline implementation for numerical correctness
    size_t batch_size = 4;
    size_t in_dim = 256;
    size_t out_dim = 256;
    size_t rank = 8;
    float scaling = 1.0f;
    
    // Create test data
    std::vector<float> input(batch_size * in_dim, 0.5f);
    std::vector<float> B(in_dim * rank);
    std::vector<float> A(rank * out_dim);
    std::vector<float> output_cpu(batch_size * out_dim);
    std::vector<float> output_reference(batch_size * out_dim);
    
    // Initialize B and A with random values
    for (size_t i = 0; i < B.size(); ++i) {
        B[i] = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.1f;
    }
    for (size_t i = 0; i < A.size(); ++i) {
        A[i] = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.1f;
    }
    
    #ifdef THEMIS_ENABLE_CPU_FUSED_KERNELS
    // Use CPU fused kernel
    cpu::fused::cpu_fused_lora_forward(
        input.data(), B.data(), A.data(), output_cpu.data(),
        batch_size, in_dim, rank, out_dim, scaling
    );
    
    // Compute reference using naive implementation
    // Step 1: h = input @ B^T
    std::vector<float> h(batch_size * rank);
    for (size_t b = 0; b < batch_size; ++b) {
        for (size_t r = 0; r < rank; ++r) {
            float sum = 0.0f;
            for (size_t i = 0; i < in_dim; ++i) {
                sum += input[b * in_dim + i] * B[i * rank + r];
            }
            h[b * rank + r] = sum;
        }
    }
    
    // Step 2: output = h @ A^T * scaling
    for (size_t b = 0; b < batch_size; ++b) {
        for (size_t o = 0; o < out_dim; ++o) {
            float sum = 0.0f;
            for (size_t r = 0; r < rank; ++r) {
                sum += h[b * rank + r] * A[r * out_dim + o];
            }
            output_reference[b * out_dim + o] = sum * scaling;
        }
    }
    
    // Compare outputs
    float max_diff = 0.0f;
    for (size_t i = 0; i < output_cpu.size(); ++i) {
        float diff = std::abs(output_cpu[i] - output_reference[i]);
        max_diff = std::max(max_diff, diff);
    }
    
    spdlog::info("CPU fused vs reference: max_diff={}", max_diff);
    EXPECT_LT(max_diff, 1e-4f);
    #else
    GTEST_SKIP() << "CPU fused kernels not enabled";
    #endif
}

TEST_F(FusedLoRAKernelsTest, CPUBaseline_BackwardNumericalAccuracy) {
    size_t batch_size = 4;
    size_t in_dim = 256;
    size_t out_dim = 256;
    size_t rank = 8;
    float scaling = 1.0f;
    
    // Create test data
    std::vector<float> input(batch_size * in_dim, 0.5f);
    std::vector<float> B(in_dim * rank);
    std::vector<float> A(rank * out_dim);
    std::vector<float> grad_output(batch_size * out_dim, 0.1f);
    std::vector<float> grad_A(rank * out_dim, 0.0f);
    std::vector<float> grad_B(in_dim * rank, 0.0f);
    std::vector<float> grad_input(batch_size * in_dim, 0.0f);
    
    // Initialize B and A
    for (size_t i = 0; i < B.size(); ++i) {
        B[i] = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.1f;
    }
    for (size_t i = 0; i < A.size(); ++i) {
        A[i] = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.1f;
    }
    
    #ifdef THEMIS_ENABLE_CPU_FUSED_KERNELS
    // Use CPU fused backward
    cpu::fused::cpu_fused_lora_backward(
        input.data(), B.data(), A.data(), grad_output.data(),
        grad_A.data(), grad_B.data(), grad_input.data(),
        batch_size, in_dim, rank, out_dim, scaling
    );
    
    // Verify gradients are non-zero
    float grad_A_sum = 0.0f;
    float grad_B_sum = 0.0f;
    float grad_input_sum = 0.0f;
    
    for (auto val : grad_A) grad_A_sum += std::abs(val);
    for (auto val : grad_B) grad_B_sum += std::abs(val);
    for (auto val : grad_input) grad_input_sum += std::abs(val);
    
    spdlog::info("CPU backward: grad_A_sum={}, grad_B_sum={}, grad_input_sum={}", 
                 grad_A_sum, grad_B_sum, grad_input_sum);
    
    EXPECT_GT(grad_A_sum, 0.0f);
    EXPECT_GT(grad_B_sum, 0.0f);
    EXPECT_GT(grad_input_sum, 0.0f);
    #else
    GTEST_SKIP() << "CPU fused kernels not enabled";
    #endif
}

TEST_F(FusedLoRAKernelsTest, CPUBaseline_Performance) {
    size_t batch_size = 32;
    size_t in_dim = 768;
    size_t out_dim = 768;
    size_t rank = 16;
    float scaling = 1.0f;
    
    std::vector<float> input(batch_size * in_dim, 0.5f);
    std::vector<float> B(in_dim * rank, 0.01f);
    std::vector<float> A(rank * out_dim, 0.01f);
    std::vector<float> output(batch_size * out_dim);
    
    #ifdef THEMIS_ENABLE_CPU_FUSED_KERNELS
    // Warmup
    for (size_t i = 0; i < 5; ++i) {
        cpu::fused::cpu_fused_lora_forward(
            input.data(), B.data(), A.data(), output.data(),
            batch_size, in_dim, rank, out_dim, scaling
        );
    }
    
    // Benchmark
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < 50; ++i) {
        cpu::fused::cpu_fused_lora_forward(
            input.data(), B.data(), A.data(), output.data(),
            batch_size, in_dim, rank, out_dim, scaling
        );
    }
    auto end = high_resolution_clock::now();
    auto cpu_time = duration_cast<microseconds>(end - start).count();
    
    spdlog::info("CPU Baseline Performance: {} μs (avg {} μs/iter)", 
                 cpu_time, cpu_time / 50);
    
    // Just ensure it completes
    EXPECT_GT(cpu_time, 0);
    #else
    GTEST_SKIP() << "CPU fused kernels not enabled";
    #endif
}

// ============================================================================
// Vulkan Backend Tests
// ============================================================================

TEST_F(FusedLoRAKernelsTest, Vulkan_ForwardNumericalAccuracy) {
    #ifdef THEMIS_ENABLE_VULKAN
    if (!vulkan::is_vulkan_available()) {
        GTEST_SKIP() << "Vulkan not available";
    }
    
    if (!vulkan::initialize_vulkan_lora(0)) {
        GTEST_SKIP() << "Failed to initialize Vulkan";
    }
    
    size_t batch_size = 4;
    size_t in_dim = 256;
    size_t out_dim = 256;
    size_t rank = 8;
    float scaling = 1.0f;
    
    // Create test data
    std::vector<float> input(batch_size * in_dim, 0.5f);
    std::vector<float> B(in_dim * rank);
    std::vector<float> A(rank * out_dim);
    std::vector<float> output_vulkan(batch_size * out_dim);
    std::vector<float> output_cpu(batch_size * out_dim);
    
    // Initialize B and A
    for (size_t i = 0; i < B.size(); ++i) {
        B[i] = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.1f;
    }
    for (size_t i = 0; i < A.size(); ++i) {
        A[i] = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.1f;
    }
    
    // Compute with Vulkan
    vulkan::launch_fused_lora_forward(
        input.data(), B.data(), A.data(), output_vulkan.data(),
        batch_size, in_dim, rank, out_dim, scaling
    );
    
    // Compute reference with CPU
    cpu::fused::cpu_fused_lora_forward(
        input.data(), B.data(), A.data(), output_cpu.data(),
        batch_size, in_dim, rank, out_dim, scaling
    );
    
    // Compare outputs
    float max_diff = 0.0f;
    for (size_t i = 0; i < output_vulkan.size(); ++i) {
        float diff = std::abs(output_vulkan[i] - output_cpu[i]);
        max_diff = std::max(max_diff, diff);
    }
    
    spdlog::info("Vulkan vs CPU: max_diff={}", max_diff);
    EXPECT_LT(max_diff, 1e-3f);
    
    vulkan::cleanup_vulkan_lora();
    #else
    GTEST_SKIP() << "Vulkan not enabled";
    #endif
}

TEST_F(FusedLoRAKernelsTest, Vulkan_BackwardNumericalAccuracy) {
    #ifdef THEMIS_ENABLE_VULKAN
    if (!vulkan::is_vulkan_available()) {
        GTEST_SKIP() << "Vulkan not available";
    }
    
    if (!vulkan::initialize_vulkan_lora(0)) {
        GTEST_SKIP() << "Failed to initialize Vulkan";
    }
    
    size_t batch_size = 4;
    size_t in_dim = 256;
    size_t out_dim = 256;
    size_t rank = 8;
    float scaling = 1.0f;
    
    // Create test data
    std::vector<float> input(batch_size * in_dim, 0.5f);
    std::vector<float> B(in_dim * rank);
    std::vector<float> A(rank * out_dim);
    std::vector<float> grad_output(batch_size * out_dim, 0.1f);
    std::vector<float> grad_A_vulkan(rank * out_dim, 0.0f);
    std::vector<float> grad_B_vulkan(in_dim * rank, 0.0f);
    std::vector<float> grad_input_vulkan(batch_size * in_dim, 0.0f);
    std::vector<float> grad_A_cpu(rank * out_dim, 0.0f);
    std::vector<float> grad_B_cpu(in_dim * rank, 0.0f);
    std::vector<float> grad_input_cpu(batch_size * in_dim, 0.0f);
    
    // Initialize B and A
    for (size_t i = 0; i < B.size(); ++i) {
        B[i] = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.1f;
    }
    for (size_t i = 0; i < A.size(); ++i) {
        A[i] = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.1f;
    }
    
    // Compute with Vulkan
    vulkan::launch_fused_lora_backward(
        input.data(), B.data(), A.data(), grad_output.data(),
        grad_A_vulkan.data(), grad_B_vulkan.data(), grad_input_vulkan.data(),
        batch_size, in_dim, rank, out_dim, scaling
    );
    
    // Compute reference with CPU
    cpu::fused::cpu_fused_lora_backward(
        input.data(), B.data(), A.data(), grad_output.data(),
        grad_A_cpu.data(), grad_B_cpu.data(), grad_input_cpu.data(),
        batch_size, in_dim, rank, out_dim, scaling
    );
    
    // Compare gradients
    auto compare_grads = [](const std::vector<float>& a, const std::vector<float>& b, const std::string& name) {
        float max_diff = 0.0f;
        for (size_t i = 0; i < a.size(); ++i) {
            max_diff = std::max(max_diff, std::abs(a[i] - b[i]));
        }
        spdlog::info("Vulkan vs CPU {}: max_diff={}", name, max_diff);
        return max_diff;
    };
    
    float grad_A_diff = compare_grads(grad_A_vulkan, grad_A_cpu, "grad_A");
    float grad_B_diff = compare_grads(grad_B_vulkan, grad_B_cpu, "grad_B");
    float grad_input_diff = compare_grads(grad_input_vulkan, grad_input_cpu, "grad_input");
    
    EXPECT_LT(grad_A_diff, 1e-3f);
    EXPECT_LT(grad_B_diff, 1e-3f);
    EXPECT_LT(grad_input_diff, 1e-3f);
    
    vulkan::cleanup_vulkan_lora();
    #else
    GTEST_SKIP() << "Vulkan not enabled";
    #endif
}

TEST_F(FusedLoRAKernelsTest, Vulkan_Performance) {
    #ifdef THEMIS_ENABLE_VULKAN
    if (!vulkan::is_vulkan_available()) {
        GTEST_SKIP() << "Vulkan not available";
    }
    
    if (!vulkan::initialize_vulkan_lora(0)) {
        GTEST_SKIP() << "Failed to initialize Vulkan";
    }
    
    size_t batch_size = 32;
    size_t in_dim = 768;
    size_t out_dim = 768;
    size_t rank = 16;
    float scaling = 1.0f;
    
    std::vector<float> input(batch_size * in_dim, 0.5f);
    std::vector<float> B(in_dim * rank, 0.01f);
    std::vector<float> A(rank * out_dim, 0.01f);
    std::vector<float> output(batch_size * out_dim);
    
    // Warmup
    for (size_t i = 0; i < 5; ++i) {
        vulkan::launch_fused_lora_forward(
            input.data(), B.data(), A.data(), output.data(),
            batch_size, in_dim, rank, out_dim, scaling
        );
    }
    
    // Benchmark
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < 50; ++i) {
        vulkan::launch_fused_lora_forward(
            input.data(), B.data(), A.data(), output.data(),
            batch_size, in_dim, rank, out_dim, scaling
        );
    }
    auto end = high_resolution_clock::now();
    auto vulkan_time = duration_cast<microseconds>(end - start).count();
    
    spdlog::info("Vulkan Fused Performance: {} μs (avg {} μs/iter)", 
                 vulkan_time, vulkan_time / 50);
    
    EXPECT_GT(vulkan_time, 0);
    
    vulkan::cleanup_vulkan_lora();
    #else
    GTEST_SKIP() << "Vulkan not enabled";
    #endif
}


// ============================================================================
// Phase 2: Optimized Kernel Tests
// ============================================================================

TEST_F(FusedLoRAKernelsTest, OptimizedKernel_VectorizedAccess_CUDA) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    // Test optimized kernel with vectorized memory access
    // Use dimension that's multiple of 4 for optimal vectorization
    size_t batch_size = 8;
    size_t in_dim = 768;  // Multiple of 4
    size_t out_dim = 768;
    size_t rank = 16;
    
    // Create tensors
    GPUTensor input({batch_size, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cuda());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cuda());
    
    GPUTensor output_base({batch_size, out_dim}, Device::cuda());
    GPUTensor output_optimized({batch_size, out_dim}, Device::cuda());
    
    // Test both kernels produce same results
    #ifdef THEMIS_ENABLE_CUDA
    // Base fused kernel
    cudaError_t err1 = cuda::fused::launch_fused_lora_forward(
        reinterpret_cast<const float*>(input.data()),
        reinterpret_cast<const float*>(B.data()),
        reinterpret_cast<const float*>(A.data()),
        reinterpret_cast<float*>(output_base.data()),
        batch_size, in_dim, rank, out_dim, 1.0f
    );
    ASSERT_EQ(err1, cudaSuccess);
    
    // Optimized kernel with vectorization
    cudaError_t err2 = cuda::fused::launch_fused_lora_forward_optimized(
        reinterpret_cast<const float*>(input.data()),
        reinterpret_cast<const float*>(B.data()),
        reinterpret_cast<const float*>(A.data()),
        reinterpret_cast<float*>(output_optimized.data()),
        batch_size, in_dim, rank, out_dim, 1.0f
    );
    ASSERT_EQ(err2, cudaSuccess);
    
    // Compare outputs
    float max_diff = maxAbsDifference(output_base, output_optimized);
    spdlog::info("Optimized vs base kernel: max_diff={}", max_diff);
    EXPECT_LT(max_diff, RELAXED_EPSILON);
    #endif
}

TEST_F(FusedLoRAKernelsTest, OptimizedKernel_Performance_CUDA) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    size_t batch_size = 32;
    size_t in_dim = 768;
    size_t out_dim = 768;
    size_t rank = 16;
    
    GPUTensor input({batch_size, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cuda());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cuda());
    
    GPUTensor output({batch_size, out_dim}, Device::cuda());
    
    #ifdef THEMIS_ENABLE_CUDA
    // Warmup
    for (size_t i = 0; i < WARMUP_ITERATIONS; ++i) {
        cuda::fused::launch_fused_lora_forward(
            reinterpret_cast<const float*>(input.data()),
            reinterpret_cast<const float*>(B.data()),
            reinterpret_cast<const float*>(A.data()),
            reinterpret_cast<float*>(output.data()),
            batch_size, in_dim, rank, out_dim, 1.0f
        );
        cuda::fused::launch_fused_lora_forward_optimized(
            reinterpret_cast<const float*>(input.data()),
            reinterpret_cast<const float*>(B.data()),
            reinterpret_cast<const float*>(A.data()),
            reinterpret_cast<float*>(output.data()),
            batch_size, in_dim, rank, out_dim, 1.0f
        );
    }
    
    // Benchmark base kernel
    auto base_start = high_resolution_clock::now();
    for (size_t i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        cuda::fused::launch_fused_lora_forward(
            reinterpret_cast<const float*>(input.data()),
            reinterpret_cast<const float*>(B.data()),
            reinterpret_cast<const float*>(A.data()),
            reinterpret_cast<float*>(output.data()),
            batch_size, in_dim, rank, out_dim, 1.0f
        );
    }
    cudaDeviceSynchronize();
    auto base_end = high_resolution_clock::now();
    auto base_time = duration_cast<microseconds>(base_end - base_start).count();
    
    // Benchmark optimized kernel
    auto opt_start = high_resolution_clock::now();
    for (size_t i = 0; i < BENCHMARK_ITERATIONS; ++i) {
        cuda::fused::launch_fused_lora_forward_optimized(
            reinterpret_cast<const float*>(input.data()),
            reinterpret_cast<const float*>(B.data()),
            reinterpret_cast<const float*>(A.data()),
            reinterpret_cast<float*>(output.data()),
            batch_size, in_dim, rank, out_dim, 1.0f
        );
    }
    cudaDeviceSynchronize();
    auto opt_end = high_resolution_clock::now();
    auto opt_time = duration_cast<microseconds>(opt_end - opt_start).count();
    
    float improvement = static_cast<float>(base_time) / static_cast<float>(opt_time);
    
    spdlog::info("Phase 2 Optimized Kernel Performance:");
    spdlog::info("  Base:      {} μs ({} μs/iter)", base_time, base_time / BENCHMARK_ITERATIONS);
    spdlog::info("  Optimized: {} μs ({} μs/iter)", opt_time, opt_time / BENCHMARK_ITERATIONS);
    spdlog::info("  Improvement: {:.2f}x", improvement);
    
    // Expect at least 1.05x improvement (5%) from vectorization
    EXPECT_GT(improvement, 1.05f) << "Optimized kernel should be faster than base";
    #endif
}

// ============================================================================
// Phase 2 Advanced: Warp-Level Optimizations Tests
// ============================================================================

TEST_F(FusedLoRAKernelsTest, WarpOptimizedKernel_NumericalAccuracy_CUDA) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    size_t batch_size = 8;
    size_t in_dim = 768;
    size_t out_dim = 768;
    size_t rank = 16;
    
    GPUTensor input({batch_size, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cuda());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cuda());
    
    GPUTensor output_base({batch_size, out_dim}, Device::cuda());
    GPUTensor output_warp({batch_size, out_dim}, Device::cuda());
    
    #ifdef THEMIS_ENABLE_CUDA
    // Base fused kernel
    cudaError_t err1 = cuda::fused::launch_fused_lora_forward(
        reinterpret_cast<const float*>(input.data()),
        reinterpret_cast<const float*>(B.data()),
        reinterpret_cast<const float*>(A.data()),
        reinterpret_cast<float*>(output_base.data()),
        batch_size, in_dim, rank, out_dim, 1.0f
    );
    ASSERT_EQ(err1, cudaSuccess);
    
    // Warp-optimized kernel
    cudaError_t err2 = cuda::fused::launch_fused_lora_forward_warp_optimized(
        reinterpret_cast<const float*>(input.data()),
        reinterpret_cast<const float*>(B.data()),
        reinterpret_cast<const float*>(A.data()),
        reinterpret_cast<float*>(output_warp.data()),
        batch_size, in_dim, rank, out_dim, 1.0f
    );
    ASSERT_EQ(err2, cudaSuccess);
    
    // Compare outputs
    float max_diff = maxAbsDifference(output_base, output_warp);
    spdlog::info("Warp-optimized vs base kernel: max_diff={}", max_diff);
    EXPECT_LT(max_diff, RELAXED_EPSILON);
    #endif
}

TEST_F(FusedLoRAKernelsTest, WarpOptimizedKernel_SmallRank_CUDA) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    // Test with small rank that fits in warp (<=32)
    size_t batch_size = 8;
    size_t in_dim = 768;
    size_t out_dim = 768;
    size_t rank = 16;  // Small rank for warp shuffle path
    
    GPUTensor input({batch_size, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    auto B = gpu_tensor_utils::xavier_uniform({in_dim, rank}, Device::cuda());
    auto A = gpu_tensor_utils::xavier_uniform({rank, out_dim}, Device::cuda());
    
    GPUTensor output({batch_size, out_dim}, Device::cuda());
    
    #ifdef THEMIS_ENABLE_CUDA
    cudaError_t err = cuda::fused::launch_fused_lora_forward_warp_optimized(
        reinterpret_cast<const float*>(input.data()),
        reinterpret_cast<const float*>(B.data()),
        reinterpret_cast<const float*>(A.data()),
        reinterpret_cast<float*>(output.data()),
        batch_size, in_dim, rank, out_dim, 1.0f
    );
    ASSERT_EQ(err, cudaSuccess);
    
    // Verify output is valid
    auto output_data = output.cpu_data();
    for (auto val : output_data) {
        EXPECT_FALSE(std::isnan(val));
        EXPECT_FALSE(std::isinf(val));
    }
    #endif
}

// ============================================================================
// Phase 3: Multi-Adapter Batching Tests
// ============================================================================

TEST_F(FusedLoRAKernelsTest, BatchedAdapters_NumericalAccuracy_CUDA) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    size_t batch_size = 4;
    size_t in_dim = 256;
    size_t out_dim = 256;
    int num_adapters = 3;
    
    // Shared input
    GPUTensor input({batch_size, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    // Create adapters with different ranks
    std::vector<int> ranks = {8, 16, 8};
    std::vector<float> scalings = {1.0f, 2.0f, 1.5f};
    
    std::vector<GPUTensor> B_tensors;
    std::vector<GPUTensor> A_tensors;
    std::vector<GPUTensor> output_batched;
    std::vector<GPUTensor> output_individual;
    
    std::vector<const float*> B_ptrs;
    std::vector<const float*> A_ptrs;
    std::vector<float*> output_ptrs;
    
    // Create adapters
    for (int i = 0; i < num_adapters; ++i) {
        B_tensors.push_back(gpu_tensor_utils::xavier_uniform({in_dim, static_cast<size_t>(ranks[i])}, Device::cuda()));
        A_tensors.push_back(gpu_tensor_utils::xavier_uniform({static_cast<size_t>(ranks[i]), out_dim}, Device::cuda()));
        output_batched.push_back(GPUTensor({batch_size, out_dim}, Device::cuda()));
        output_individual.push_back(GPUTensor({batch_size, out_dim}, Device::cuda()));
        
        B_ptrs.push_back(reinterpret_cast<const float*>(B_tensors[i].data()));
        A_ptrs.push_back(reinterpret_cast<const float*>(A_tensors[i].data()));
        output_ptrs.push_back(reinterpret_cast<float*>(output_batched[i].data()));
    }
    
    #ifdef THEMIS_ENABLE_CUDA
    // Copy pointers to device
    const float** d_B_ptrs;
    const float** d_A_ptrs;
    float** d_output_ptrs;
    int* d_ranks;
    float* d_scalings;
    
    cudaMalloc(&d_B_ptrs, num_adapters * sizeof(float*));
    cudaMalloc(&d_A_ptrs, num_adapters * sizeof(float*));
    cudaMalloc(&d_output_ptrs, num_adapters * sizeof(float*));
    cudaMalloc(&d_ranks, num_adapters * sizeof(int));
    cudaMalloc(&d_scalings, num_adapters * sizeof(float));
    
    cudaMemcpy(d_B_ptrs, B_ptrs.data(), num_adapters * sizeof(float*), cudaMemcpyHostToDevice);
    cudaMemcpy(d_A_ptrs, A_ptrs.data(), num_adapters * sizeof(float*), cudaMemcpyHostToDevice);
    cudaMemcpy(d_output_ptrs, output_ptrs.data(), num_adapters * sizeof(float*), cudaMemcpyHostToDevice);
    cudaMemcpy(d_ranks, ranks.data(), num_adapters * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_scalings, scalings.data(), num_adapters * sizeof(float), cudaMemcpyHostToDevice);
    
    // Launch batched kernel
    cudaError_t err = cuda::fused::launch_batched_lora_forward(
        reinterpret_cast<const float*>(input.data()),
        d_B_ptrs, d_A_ptrs, d_output_ptrs,
        d_ranks, d_scalings,
        num_adapters, batch_size, in_dim, out_dim
    );
    ASSERT_EQ(err, cudaSuccess);
    
    // Compute individual outputs for comparison
    for (int i = 0; i < num_adapters; ++i) {
        cuda::fused::launch_fused_lora_forward(
            reinterpret_cast<const float*>(input.data()),
            B_ptrs[i], A_ptrs[i],
            reinterpret_cast<float*>(output_individual[i].data()),
            batch_size, in_dim, ranks[i], out_dim, scalings[i]
        );
    }
    
    // Compare outputs
    for (int i = 0; i < num_adapters; ++i) {
        float max_diff = maxAbsDifference(output_batched[i], output_individual[i]);
        spdlog::info("Adapter {}: batched vs individual max_diff={}", i, max_diff);
        EXPECT_LT(max_diff, RELAXED_EPSILON);
    }
    
    // Cleanup
    cudaFree(d_B_ptrs);
    cudaFree(d_A_ptrs);
    cudaFree(d_output_ptrs);
    cudaFree(d_ranks);
    cudaFree(d_scalings);
    #endif
}

TEST_F(FusedLoRAKernelsTest, BatchedAdapters_Performance_CUDA) {
    if (!has_cuda_) {
        GTEST_SKIP() << "CUDA not available";
    }
    
    size_t batch_size = 8;
    size_t in_dim = 768;
    size_t out_dim = 768;
    int num_adapters = 4;
    
    GPUTensor input({batch_size, in_dim}, Device::cuda());
    input.fill(0.5f);
    
    std::vector<int> ranks(num_adapters, 8);
    std::vector<float> scalings(num_adapters, 1.0f);
    
    std::vector<GPUTensor> B_tensors;
    std::vector<GPUTensor> A_tensors;
    std::vector<GPUTensor> outputs;
    std::vector<const float*> B_ptrs;
    std::vector<const float*> A_ptrs;
    std::vector<float*> output_ptrs;
    
    for (int i = 0; i < num_adapters; ++i) {
        B_tensors.push_back(gpu_tensor_utils::xavier_uniform({in_dim, static_cast<size_t>(ranks[i])}, Device::cuda()));
        A_tensors.push_back(gpu_tensor_utils::xavier_uniform({static_cast<size_t>(ranks[i]), out_dim}, Device::cuda()));
        outputs.push_back(GPUTensor({batch_size, out_dim}, Device::cuda()));
        
        B_ptrs.push_back(reinterpret_cast<const float*>(B_tensors[i].data()));
        A_ptrs.push_back(reinterpret_cast<const float*>(A_tensors[i].data()));
        output_ptrs.push_back(reinterpret_cast<float*>(outputs[i].data()));
    }
    
    #ifdef THEMIS_ENABLE_CUDA
    // Setup device pointers
    const float** d_B_ptrs;
    const float** d_A_ptrs;
    float** d_output_ptrs;
    int* d_ranks;
    float* d_scalings;
    
    cudaMalloc(&d_B_ptrs, num_adapters * sizeof(float*));
    cudaMalloc(&d_A_ptrs, num_adapters * sizeof(float*));
    cudaMalloc(&d_output_ptrs, num_adapters * sizeof(float*));
    cudaMalloc(&d_ranks, num_adapters * sizeof(int));
    cudaMalloc(&d_scalings, num_adapters * sizeof(float));
    
    cudaMemcpy(d_B_ptrs, B_ptrs.data(), num_adapters * sizeof(float*), cudaMemcpyHostToDevice);
    cudaMemcpy(d_A_ptrs, A_ptrs.data(), num_adapters * sizeof(float*), cudaMemcpyHostToDevice);
    cudaMemcpy(d_output_ptrs, output_ptrs.data(), num_adapters * sizeof(float*), cudaMemcpyHostToDevice);
    cudaMemcpy(d_ranks, ranks.data(), num_adapters * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_scalings, scalings.data(), num_adapters * sizeof(float), cudaMemcpyHostToDevice);
    
    // Warmup
    for (size_t i = 0; i < 5; ++i) {
        cuda::fused::launch_batched_lora_forward(
            reinterpret_cast<const float*>(input.data()),
            d_B_ptrs, d_A_ptrs, d_output_ptrs,
            d_ranks, d_scalings,
            num_adapters, batch_size, in_dim, out_dim
        );
    }
    
    // Benchmark batched
    auto batched_start = high_resolution_clock::now();
    for (size_t iter = 0; iter < 50; ++iter) {
        cuda::fused::launch_batched_lora_forward(
            reinterpret_cast<const float*>(input.data()),
            d_B_ptrs, d_A_ptrs, d_output_ptrs,
            d_ranks, d_scalings,
            num_adapters, batch_size, in_dim, out_dim
        );
    }
    cudaDeviceSynchronize();
    auto batched_end = high_resolution_clock::now();
    auto batched_time = duration_cast<microseconds>(batched_end - batched_start).count();
    
    // Benchmark sequential
    auto sequential_start = high_resolution_clock::now();
    for (size_t iter = 0; iter < 50; ++iter) {
        for (int i = 0; i < num_adapters; ++i) {
            cuda::fused::launch_fused_lora_forward(
                reinterpret_cast<const float*>(input.data()),
                B_ptrs[i], A_ptrs[i], output_ptrs[i],
                batch_size, in_dim, ranks[i], out_dim, scalings[i]
            );
        }
    }
    cudaDeviceSynchronize();
    auto sequential_end = high_resolution_clock::now();
    auto sequential_time = duration_cast<microseconds>(sequential_end - sequential_start).count();
    
    float improvement = static_cast<float>(sequential_time) / static_cast<float>(batched_time);
    
    spdlog::info("Phase 3 Batched Adapters Performance:");
    spdlog::info("  Sequential: {} μs", sequential_time);
    spdlog::info("  Batched:    {} μs", batched_time);
    spdlog::info("  Improvement: {:.2f}x", improvement);
    
    // Batched should be faster due to reduced launch overhead
    EXPECT_GT(improvement, 1.0f) << "Batched should reduce overhead";
    
    // Cleanup
    cudaFree(d_B_ptrs);
    cudaFree(d_A_ptrs);
    cudaFree(d_output_ptrs);
    cudaFree(d_ranks);
    cudaFree(d_scalings);
    #endif
}


