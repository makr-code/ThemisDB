/**
 * @file test_qlora_gpu_kernels.cpp
 * @brief Tests for QLoRA GPU kernel optimizations
 */

#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_CUDA

#include "llm/lora_framework/quantization_kernels.h"
#include "llm/lora_framework/quantization.h"
#include <cuda_runtime.h>
#include <vector>
#include <cmath>
#include <random>

using namespace themis::llm::lora;
using namespace themis::llm::lora::cuda;

namespace {

#define ASSERT_CUDA_SUCCESS(call) \
    do { \
        const cudaError_t _cuda_err = (call); \
        ASSERT_EQ(_cuda_err, cudaSuccess) << #call << " failed: " << cudaGetErrorString(_cuda_err); \
    } while (false)

/**
 * @brief Generate random test data
 * @param size Number of elements
 * @param mean Mean of normal distribution
 * @param stddev Standard deviation
 * @note Uses fixed seed (42) for reproducibility across test runs
 */
std::vector<float> generateTestData(size_t size, float mean = 0.0f, float stddev = 1.0f) {
    std::vector<float> data(size);
    std::mt19937 gen(42);  // Fixed seed for reproducible tests
    std::normal_distribution<float> dist(mean, stddev);
    
    for (size_t i = 0; i < size; i++) {
        data[i] = dist(gen);
    }
    
    return data;
}

/**
 * @brief Compute Mean Squared Error between two vectors
 */
float computeMSE(const std::vector<float>& a, const std::vector<float>& b) {
    EXPECT_EQ(a.size(), b.size());
    float sum = 0.0f;
    for (size_t i = 0; i < a.size(); i++) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sum / a.size();
}

} // anonymous namespace

// ============================================================================
// NF4 GPU Quantization Tests
// ============================================================================

TEST(QLoRAGPUKernels, NF4QuantizationBasic) {
    const size_t num_elements = 1024;
    const size_t block_size = 64;
    const size_t num_blocks = (num_elements + block_size - 1) / block_size;
    
    // Generate test data
    auto input = generateTestData(num_elements);
    
    // Allocate device memory
    float* d_input;
    uint8_t* d_output;
    float* d_scales;
    float* d_zeros;
    
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_input, num_elements * sizeof(float)));
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_output, (num_elements + 1) / 2));
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_scales, num_blocks * sizeof(float)));
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_zeros, num_blocks * sizeof(float)));
    
    // Copy input to device
    ASSERT_CUDA_SUCCESS(cudaMemcpy(d_input, input.data(), num_elements * sizeof(float), cudaMemcpyHostToDevice));
    
    // Launch quantization kernel
    cudaError_t err = launch_quantize_nf4_kernel(
        d_input, d_output, d_scales, d_zeros, num_elements, block_size);
    ASSERT_EQ(err, cudaSuccess);
    
    // Wait for completion
    ASSERT_CUDA_SUCCESS(cudaDeviceSynchronize());
    
    // Copy results back
    std::vector<uint8_t> output((num_elements + 1) / 2);
    std::vector<float> scales(num_blocks);
    std::vector<float> zeros(num_blocks);
    
    ASSERT_CUDA_SUCCESS(cudaMemcpy(output.data(), d_output, (num_elements + 1) / 2, cudaMemcpyDeviceToHost));
    ASSERT_CUDA_SUCCESS(cudaMemcpy(scales.data(), d_scales, num_blocks * sizeof(float), cudaMemcpyDeviceToHost));
    ASSERT_CUDA_SUCCESS(cudaMemcpy(zeros.data(), d_zeros, num_blocks * sizeof(float), cudaMemcpyDeviceToHost));
    
    // Verify results
    EXPECT_GT(output.size(), 0);
    EXPECT_EQ(scales.size(), num_blocks);
    EXPECT_EQ(zeros.size(), num_blocks);
    
    // Check that scales are non-zero
    for (size_t i = 0; i < num_blocks; i++) {
        EXPECT_GT(std::abs(scales[i]), 0.0f);
    }
    
    // Cleanup
    cudaFree(d_input);
    cudaFree(d_output);
    cudaFree(d_scales);
    cudaFree(d_zeros);
}

TEST(QLoRAGPUKernels, NF4DequantizationRoundTrip) {
    const size_t num_elements = 1024;
    const size_t block_size = 64;
    const size_t num_blocks = (num_elements + block_size - 1) / block_size;
    
    auto input = generateTestData(num_elements);
    
    // Allocate device memory
    float* d_input;
    uint8_t* d_quantized;
    float* d_scales;
    float* d_zeros;
    float* d_output;
    
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_input, num_elements * sizeof(float)));
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_quantized, (num_elements + 1) / 2));
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_scales, num_blocks * sizeof(float)));
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_zeros, num_blocks * sizeof(float)));
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_output, num_elements * sizeof(float)));
    
    ASSERT_CUDA_SUCCESS(cudaMemcpy(d_input, input.data(), num_elements * sizeof(float), cudaMemcpyHostToDevice));
    
    // Quantize
    launch_quantize_nf4_kernel(d_input, d_quantized, d_scales, d_zeros, num_elements, block_size);
    
    // Dequantize
    launch_dequantize_nf4_kernel(d_quantized, d_scales, d_zeros, d_output, num_elements, block_size);
    
    ASSERT_CUDA_SUCCESS(cudaDeviceSynchronize());
    
    // Copy result back
    std::vector<float> output(num_elements);
    ASSERT_CUDA_SUCCESS(cudaMemcpy(output.data(), d_output, num_elements * sizeof(float), cudaMemcpyDeviceToHost));
    
    // Compute error
    float mse = computeMSE(input, output);
    EXPECT_LT(mse, 0.01f);  // Should match CPU implementation accuracy
    
    // Cleanup
    cudaFree(d_input);
    cudaFree(d_quantized);
    cudaFree(d_scales);
    cudaFree(d_zeros);
    cudaFree(d_output);
}

// ============================================================================
// INT8 GPU Quantization Tests
// ============================================================================

TEST(QLoRAGPUKernels, INT8QuantizationBasic) {
    const size_t num_elements = 1024;
    const size_t block_size = 64;
    const size_t num_blocks = (num_elements + block_size - 1) / block_size;
    
    auto input = generateTestData(num_elements);
    
    // Allocate device memory
    float* d_input;
    int8_t* d_output;
    float* d_scales;
    
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_input, num_elements * sizeof(float)));
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_output, num_elements * sizeof(int8_t)));
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_scales, num_blocks * sizeof(float)));
    
    ASSERT_CUDA_SUCCESS(cudaMemcpy(d_input, input.data(), num_elements * sizeof(float), cudaMemcpyHostToDevice));
    
    // Launch kernel
    cudaError_t err = launch_quantize_int8_kernel(d_input, d_output, d_scales, num_elements, block_size);
    ASSERT_EQ(err, cudaSuccess);
    
    ASSERT_CUDA_SUCCESS(cudaDeviceSynchronize());
    
    // Verify
    std::vector<int8_t> output(num_elements);
    std::vector<float> scales(num_blocks);
    
    ASSERT_CUDA_SUCCESS(cudaMemcpy(output.data(), d_output, num_elements * sizeof(int8_t), cudaMemcpyDeviceToHost));
    ASSERT_CUDA_SUCCESS(cudaMemcpy(scales.data(), d_scales, num_blocks * sizeof(float), cudaMemcpyDeviceToHost));
    
    EXPECT_EQ(output.size(), num_elements);
    EXPECT_EQ(scales.size(), num_blocks);
    
    // Cleanup
    cudaFree(d_input);
    cudaFree(d_output);
    cudaFree(d_scales);
}

TEST(QLoRAGPUKernels, INT8DequantizationRoundTrip) {
    const size_t num_elements = 1024;
    const size_t block_size = 64;
    const size_t num_blocks = (num_elements + block_size - 1) / block_size;
    
    auto input = generateTestData(num_elements);
    
    // Allocate device memory
    float* d_input;
    int8_t* d_quantized;
    float* d_scales;
    float* d_output;
    
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_input, num_elements * sizeof(float)));
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_quantized, num_elements * sizeof(int8_t)));
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_scales, num_blocks * sizeof(float)));
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_output, num_elements * sizeof(float)));
    
    ASSERT_CUDA_SUCCESS(cudaMemcpy(d_input, input.data(), num_elements * sizeof(float), cudaMemcpyHostToDevice));
    
    // Quantize and dequantize
    launch_quantize_int8_kernel(d_input, d_quantized, d_scales, num_elements, block_size);
    launch_dequantize_int8_kernel(d_quantized, d_scales, d_output, num_elements, block_size);
    
    ASSERT_CUDA_SUCCESS(cudaDeviceSynchronize());
    
    // Copy result
    std::vector<float> output(num_elements);
    ASSERT_CUDA_SUCCESS(cudaMemcpy(output.data(), d_output, num_elements * sizeof(float), cudaMemcpyDeviceToHost));
    
    // Verify accuracy
    float mse = computeMSE(input, output);
    EXPECT_LT(mse, 0.0001f);  // INT8 should be more accurate than NF4
    
    // Cleanup
    cudaFree(d_input);
    cudaFree(d_quantized);
    cudaFree(d_scales);
    cudaFree(d_output);
}

// ============================================================================
// Fused Kernel Tests
// ============================================================================

TEST(QLoRAGPUKernels, FusedDequantMatMulNF4) {
    const size_t M = 32;
    const size_t K = 64;
    const size_t N = 48;
    const size_t block_size = 64;
    const size_t num_blocks = (K * N + block_size - 1) / block_size;
    
    // Generate test data
    auto input = generateTestData(M * K);
    auto weights = generateTestData(K * N);
    
    // Allocate and quantize weights
    float* d_weights_fp32;
    uint8_t* d_weights_quant;
    float* d_scales;
    float* d_zeros;
    
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_weights_fp32, K * N * sizeof(float)));
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_weights_quant, (K * N + 1) / 2));
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_scales, num_blocks * sizeof(float)));
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_zeros, num_blocks * sizeof(float)));
    
    ASSERT_CUDA_SUCCESS(cudaMemcpy(d_weights_fp32, weights.data(), K * N * sizeof(float), cudaMemcpyHostToDevice));
    launch_quantize_nf4_kernel(d_weights_fp32, d_weights_quant, d_scales, d_zeros, K * N, block_size);
    
    // Allocate input and output
    float* d_input;
    float* d_output;
    
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_input, M * K * sizeof(float)));
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_output, M * N * sizeof(float)));
    
    ASSERT_CUDA_SUCCESS(cudaMemcpy(d_input, input.data(), M * K * sizeof(float), cudaMemcpyHostToDevice));
    
    // Launch fused kernel
    cudaError_t err = launch_fused_dequant_matmul_kernel(
        d_weights_quant, d_scales, d_zeros, d_input, d_output, M, K, N, block_size, true);
    ASSERT_EQ(err, cudaSuccess);
    
    ASSERT_CUDA_SUCCESS(cudaDeviceSynchronize());
    
    // Verify output
    std::vector<float> output(M * N);
    ASSERT_CUDA_SUCCESS(cudaMemcpy(output.data(), d_output, M * N * sizeof(float), cudaMemcpyDeviceToHost));
    
    EXPECT_EQ(output.size(), M * N);
    
    // Cleanup
    cudaFree(d_weights_fp32);
    cudaFree(d_weights_quant);
    cudaFree(d_scales);
    cudaFree(d_zeros);
    cudaFree(d_input);
    cudaFree(d_output);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST(QLoRAGPUKernels, PerformanceNF4Quantization) {
    const size_t num_elements = 1024 * 1024;  // 1M parameters
    const size_t block_size = 64;
    const size_t num_blocks = (num_elements + block_size - 1) / block_size;
    
    auto input = generateTestData(num_elements);
    
    // Allocate device memory
    float* d_input;
    uint8_t* d_output;
    float* d_scales;
    float* d_zeros;
    
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_input, num_elements * sizeof(float)));
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_output, (num_elements + 1) / 2));
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_scales, num_blocks * sizeof(float)));
    ASSERT_CUDA_SUCCESS(cudaMalloc(&d_zeros, num_blocks * sizeof(float)));
    
    ASSERT_CUDA_SUCCESS(cudaMemcpy(d_input, input.data(), num_elements * sizeof(float), cudaMemcpyHostToDevice));
    
    // Warmup
    launch_quantize_nf4_kernel(d_input, d_output, d_scales, d_zeros, num_elements, block_size);
    ASSERT_CUDA_SUCCESS(cudaDeviceSynchronize());
    
    // Measure performance
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    
    cudaEventRecord(start);
    launch_quantize_nf4_kernel(d_input, d_output, d_scales, d_zeros, num_elements, block_size);
    cudaEventRecord(stop);
    
    cudaEventSynchronize(stop);
    
    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, start, stop);
    
    std::cout << "NF4 Quantization (1M params): " << milliseconds << " ms" << std::endl;
    
    // Target: < 5ms (vs 10-20ms on CPU)
    EXPECT_LT(milliseconds, 10.0f);
    
    // Cleanup
    cudaEventDestroy(start);
    cudaEventDestroy(stop);
    cudaFree(d_input);
    cudaFree(d_output);
    cudaFree(d_scales);
    cudaFree(d_zeros);
}

// ============================================================================
// GPU Memory Manager Tests
// ============================================================================

TEST(QLoRAGPUKernels, MemoryManagerBasic) {
    GPUMemoryManager manager;
    
    // Allocate quantized buffer
    void* buffer = manager.allocateQuantizedBuffer(1024, true);
    ASSERT_NE(buffer, nullptr);
    
    // Allocate pinned host memory
    void* pinned = manager.allocatePinnedHost(1024 * sizeof(float));
    ASSERT_NE(pinned, nullptr);
    
    // Verify allocation tracking
    EXPECT_GT(manager.getTotalAllocated(), 0);
    
    // Cleanup
    manager.freeDevice(buffer);
    manager.freePinned(pinned);
}

TEST(QLoRAGPUKernels, AsyncTransfer) {
    GPUMemoryManager manager;
    
    const size_t size = 1024 * sizeof(float);
    std::vector<float> host_data(1024, 1.0f);
    
    // Allocate device memory
    void* device_buffer = manager.allocateQuantizedBuffer(1024 * sizeof(float), false);
    ASSERT_NE(device_buffer, nullptr);
    
    // Create stream
    cudaStream_t stream;
    ASSERT_CUDA_SUCCESS(cudaStreamCreate(&stream));
    
    // Async transfer
    cudaError_t err = manager.transferToGPUAsync(device_buffer, host_data.data(), size, stream);
    EXPECT_EQ(err, cudaSuccess);
    
    // Wait
    ASSERT_CUDA_SUCCESS(cudaStreamSynchronize(stream));
    
    // Cleanup
    ASSERT_CUDA_SUCCESS(cudaStreamDestroy(stream));
    manager.freeDevice(device_buffer);
}

#endif // THEMIS_ENABLE_CUDA


