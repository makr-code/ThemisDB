// Test: Vulkan LoRA Backend Integration Tests
// Validates Vulkan compute pipeline for LoRA training operations

#include <gtest/gtest.h>
#include "llm/lora_framework/vulkan_context.h"
#include "llm/lora_framework/vulkan_buffer.h"
#include "llm/lora_framework/vulkan_pipeline.h"
#include "llm/lora_framework/vulkan_kernels.h"
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>

using namespace themis::lora::vulkan;

class VulkanLoRATest : public ::testing::Test {
protected:
    void SetUp() override {
        // Check if Vulkan is available
        if (!is_vulkan_available()) {
            GTEST_SKIP() << "Vulkan not available on this system";
        }
        
        // Try to initialize Vulkan
        if (!initialize_vulkan_lora(0)) {
            GTEST_SKIP() << "Failed to initialize Vulkan LoRA backend";
        }
    }
    
    void TearDown() override {
        cleanup_vulkan_lora();
    }
    
    // Helper: Generate random matrix
    std::vector<float> generate_random_matrix(int rows, int cols, int seed = 42) {
        std::mt19937 gen(seed);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        
        std::vector<float> matrix(rows * cols);
        for (auto& val : matrix) {
            val = dist(gen);
        }
        return matrix;
    }
    
    // Helper: CPU matrix multiplication for reference
    std::vector<float> cpu_matmul(const std::vector<float>& A, 
                                   const std::vector<float>& B,
                                   int M, int K, int N, float alpha = 1.0f) {
        std::vector<float> C(M * N, 0.0f);
        
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                float sum = 0.0f;
                for (int k = 0; k < K; k++) {
                    sum += A[i * K + k] * B[k * N + j];
                }
                C[i * N + j] = sum * alpha;
            }
        }
        
        return C;
    }
    
    // Helper: Check if two vectors are close
    bool vectors_near(const std::vector<float>& a, const std::vector<float>& b,
                      float tolerance = 1e-3f) {
        if (a.size() != b.size()) return false;
        
        for (size_t i = 0; i < a.size(); i++) {
            if (std::abs(a[i] - b[i]) > tolerance) {
                std::cerr << "Mismatch at index " << i << ": " 
                          << a[i] << " vs " << b[i] << std::endl;
                return false;
            }
        }
        return true;
    }
};

// ===== Basic Infrastructure Tests =====

TEST_F(VulkanLoRATest, VulkanAvailability) {
    EXPECT_TRUE(is_vulkan_available());
}

TEST_F(VulkanLoRATest, ContextInitialization) {
    VulkanContext context;
    EXPECT_TRUE(context.initialize(0, false));
    EXPECT_TRUE(context.is_initialized());
    
    const auto& props = context.device_properties();
    EXPECT_GT(props.limits.maxComputeWorkGroupInvocations, 0);
    
    std::cout << "Vulkan device: " << props.deviceName << std::endl;
    
    context.cleanup();
    EXPECT_FALSE(context.is_initialized());
}

TEST_F(VulkanLoRATest, BufferAllocation) {
    VulkanContext context;
    ASSERT_TRUE(context.initialize());
    
    // Test device-local buffer
    VulkanBuffer buffer(&context, 1024, VulkanBuffer::Usage::DeviceLocal);
    EXPECT_EQ(buffer.size(), 1024);
    
    // Test staging buffer
    VulkanBuffer staging(&context, 2048, VulkanBuffer::Usage::Staging);
    EXPECT_EQ(staging.size(), 2048);
}

TEST_F(VulkanLoRATest, BufferUploadDownload) {
    VulkanContext context;
    ASSERT_TRUE(context.initialize());
    
    std::vector<float> data = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    size_t byte_size = data.size() * sizeof(float);
    
    VulkanBuffer buffer(&context, byte_size, VulkanBuffer::Usage::DeviceLocal);
    
    // Upload
    buffer.upload(data.data(), byte_size);
    
    // Download
    std::vector<float> result(data.size());
    buffer.download(result.data(), byte_size);
    
    // Verify
    EXPECT_EQ(data, result);
}

// ===== Matrix Multiplication Tests =====

TEST_F(VulkanLoRATest, MatMul_Small) {
    const int M = 4, N = 4, K = 4;
    
    auto A = generate_random_matrix(M, K);
    auto B = generate_random_matrix(K, N);
    std::vector<float> C(M * N);
    
    // Compute on Vulkan
    launch_matmul_shader(A.data(), B.data(), C.data(), M, N, K, 1.0f);
    
    // Compute on CPU
    auto C_expected = cpu_matmul(A, B, M, N, K);
    
    // Compare
    EXPECT_TRUE(vectors_near(C, C_expected, 1e-3f));
}

TEST_F(VulkanLoRATest, MatMul_Medium) {
    const int M = 64, N = 64, K = 64;
    
    auto A = generate_random_matrix(M, K);
    auto B = generate_random_matrix(K, N);
    std::vector<float> C(M * N);
    
    launch_matmul_shader(A.data(), B.data(), C.data(), M, N, K, 1.0f);
    
    auto C_expected = cpu_matmul(A, B, M, N, K);
    
    EXPECT_TRUE(vectors_near(C, C_expected, 1e-2f));
}

TEST_F(VulkanLoRATest, MatMul_WithAlpha) {
    const int M = 16, N = 16, K = 16;
    const float alpha = 2.5f;
    
    auto A = generate_random_matrix(M, K);
    auto B = generate_random_matrix(K, N);
    std::vector<float> C(M * N);
    
    launch_matmul_shader(A.data(), B.data(), C.data(), M, N, K, alpha);
    
    auto C_expected = cpu_matmul(A, B, M, N, K, alpha);
    
    EXPECT_TRUE(vectors_near(C, C_expected, 1e-2f));
}

// ===== Element-wise Operation Tests =====

TEST_F(VulkanLoRATest, ElementwiseAdd) {
    const size_t size = 1024;
    
    auto A = generate_random_matrix(1, size);
    auto B = generate_random_matrix(1, size);
    std::vector<float> C(size);
    
    launch_add_shader(A.data(), B.data(), C.data(), size);
    
    // Verify
    for (size_t i = 0; i < size; i++) {
        EXPECT_NEAR(C[i], A[i] + B[i], 1e-5f);
    }
}

TEST_F(VulkanLoRATest, ElementwiseMultiply) {
    const size_t size = 512;
    
    auto A = generate_random_matrix(1, size);
    auto B = generate_random_matrix(1, size);
    std::vector<float> C(size);
    
    launch_multiply_shader(A.data(), B.data(), C.data(), size);
    
    // Verify
    for (size_t i = 0; i < size; i++) {
        EXPECT_NEAR(C[i], A[i] * B[i], 1e-5f);
    }
}

TEST_F(VulkanLoRATest, ScalarMultiply) {
    const size_t size = 256;
    const float scalar = 3.5f;
    
    auto A = generate_random_matrix(1, size);
    std::vector<float> B(size);
    
    launch_scalar_multiply_shader(A.data(), B.data(), scalar, size);
    
    // Verify
    for (size_t i = 0; i < size; i++) {
        EXPECT_NEAR(B[i], A[i] * scalar, 1e-5f);
    }
}

// ===== Transpose Tests =====

TEST_F(VulkanLoRATest, Transpose) {
    const int rows = 8, cols = 16;
    
    auto input = generate_random_matrix(rows, cols);
    std::vector<float> output(rows * cols);
    
    launch_transpose_shader(input.data(), output.data(), rows, cols);
    
    // Verify
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            EXPECT_NEAR(output[j * rows + i], input[i * cols + j], 1e-5f);
        }
    }
}

// ===== LoRA Gradient Tests =====

TEST_F(VulkanLoRATest, LoRAGradientA) {
    const int M = 32, K = 8, N = 64;  // batch, rank, out_dim
    const float scaling = 1.0f;
    
    auto h = generate_random_matrix(M, K);
    auto grad_output = generate_random_matrix(M, N);
    std::vector<float> grad_A(K * N);
    
    launch_lora_grad_A_shader(h.data(), grad_output.data(), grad_A.data(),
                               M, K, N, scaling);
    
    // Verify dimensions
    EXPECT_EQ(grad_A.size(), K * N);
    
    // Gradient should not be all zeros
    bool has_nonzero = false;
    for (auto val : grad_A) {
        if (std::abs(val) > 1e-6f) {
            has_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(has_nonzero);
}

TEST_F(VulkanLoRATest, LoRAGradientB) {
    const int M = 32, D = 128, K = 8;  // batch, in_dim, rank
    
    auto input = generate_random_matrix(M, D);
    auto grad_h = generate_random_matrix(M, K);
    std::vector<float> grad_B(D * K);
    
    launch_lora_grad_B_shader(input.data(), grad_h.data(), grad_B.data(),
                               M, D, K);
    
    // Verify dimensions
    EXPECT_EQ(grad_B.size(), D * K);
    
    // Gradient should not be all zeros
    bool has_nonzero = false;
    for (auto val : grad_B) {
        if (std::abs(val) > 1e-6f) {
            has_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(has_nonzero);
}

// ===== Performance Tests =====

TEST_F(VulkanLoRATest, MatMul_Performance) {
    const int M = 768, N = 768, K = 768;
    
    auto A = generate_random_matrix(M, K);
    auto B = generate_random_matrix(K, N);
    std::vector<float> C(M * N);
    
    // Warmup
    launch_matmul_shader(A.data(), B.data(), C.data(), M, N, K, 1.0f);
    
    // Timed run
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10; i++) {
        launch_matmul_shader(A.data(), B.data(), C.data(), M, N, K, 1.0f);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double avg_time_ms = duration.count() / 10000.0;
    std::cout << "Average MatMul time (768x768): " << avg_time_ms << " ms" << std::endl;
    
    // Should be much faster than CPU (target: < 10ms)
    EXPECT_LT(avg_time_ms, 50.0);
}

// ===== Stress Tests =====

TEST_F(VulkanLoRATest, MultipleOperations) {
    const int M = 32, N = 32, K = 32;
    
    // Perform multiple operations in sequence
    for (int iteration = 0; iteration < 5; iteration++) {
        auto A = generate_random_matrix(M, K, iteration);
        auto B = generate_random_matrix(K, N, iteration + 100);
        std::vector<float> C(M * N);
        
        launch_matmul_shader(A.data(), B.data(), C.data(), M, N, K, 1.0f);
        
        auto C_expected = cpu_matmul(A, B, M, N, K);
        EXPECT_TRUE(vectors_near(C, C_expected, 1e-2f));
    }
}
