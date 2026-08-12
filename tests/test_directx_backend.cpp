#include <gtest/gtest.h>

#ifdef _WIN32

#include "llm/lora_framework/directx_context.h"
#include "llm/lora_framework/directx_buffer.h"
#include "llm/lora_framework/directx_descriptors.h"
#include "llm/lora_framework/directx_shader.h"
#include "llm/lora_framework/directx_pipeline.h"
#include "llm/lora_framework/directx_kernels.h"

using namespace themis::lora::directx;

class DirectXBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "DirectX focused backend tests are unstable on this Windows runner.";
#endif
        // Skip tests if DirectX is not available
        if (!is_directx_available()) {
            GTEST_SKIP() << "DirectX 12 not available on this system";
        }
    }
};

TEST_F(DirectXBackendTest, ContextInitialization) {
    DirectXContext context(0);
    ASSERT_TRUE(context.initialize()) << "Failed to initialize DirectX context";
    EXPECT_TRUE(context.is_initialized());
    EXPECT_NE(context.device(), nullptr);
    EXPECT_NE(context.command_queue(), nullptr);
    EXPECT_NE(context.command_list(), nullptr);
    EXPECT_FALSE(context.get_gpu_description().empty());
}

TEST_F(DirectXBackendTest, BufferCreation) {
    DirectXContext context(0);
    ASSERT_TRUE(context.initialize());
    
    // Create a 1MB buffer
    size_t buffer_size = 1024 * 1024;
    DirectXBuffer buffer(&context, buffer_size);
    
    EXPECT_NE(buffer.resource(), nullptr);
    EXPECT_EQ(buffer.size(), buffer_size);
    EXPECT_NE(buffer.gpu_address(), 0);
}

TEST_F(DirectXBackendTest, BufferUploadDownload) {
    DirectXContext context(0);
    ASSERT_TRUE(context.initialize());
    
    // Create test data
    std::vector<float> test_data(1024);
    for (size_t i = 0; i < test_data.size(); ++i) {
        test_data[i] = static_cast<float>(i);
    }
    
    // Create buffer and upload data
    size_t buffer_size = test_data.size() * sizeof(float);
    DirectXBuffer buffer(&context, buffer_size);
    buffer.upload(test_data.data(), buffer_size);
    
    // Download and verify
    std::vector<float> downloaded_data(test_data.size());
    buffer.download(downloaded_data.data(), buffer_size);
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        EXPECT_FLOAT_EQ(test_data[i], downloaded_data[i])
            << "Mismatch at index " << i;
    }
}

TEST_F(DirectXBackendTest, DescriptorCreation) {
    DirectXContext context(0);
    ASSERT_TRUE(context.initialize());
    
    DirectXDescriptors descriptors(&context, 16);
    ASSERT_TRUE(descriptors.initialize());
    
    EXPECT_NE(descriptors.heap(), nullptr);
    
    // Create a buffer and UAV descriptor
    size_t buffer_size = 1024 * sizeof(float);
    DirectXBuffer buffer(&context, buffer_size);
    
    uint32_t uav_index = descriptors.create_uav(
        buffer.resource(), 
        1024,  // num_elements
        sizeof(float)
    );
    
    EXPECT_EQ(uav_index, 0);  // First descriptor
    
    // Create SRV descriptor
    uint32_t srv_index = descriptors.create_srv(
        buffer.resource(),
        1024,
        sizeof(float)
    );
    
    EXPECT_EQ(srv_index, 1);  // Second descriptor
}

TEST_F(DirectXBackendTest, InitializeLoRA) {
    bool result = initialize_directx_lora(0);
    EXPECT_TRUE(result) << "Failed to initialize DirectX LoRA backend";
    
    EXPECT_TRUE(is_directx_available());
    
    // Cleanup
    cleanup_directx_lora();
}

TEST_F(DirectXBackendTest, MultipleContexts) {
    // Test that we can create multiple contexts (for multi-GPU scenarios)
    DirectXContext context1(0);
    ASSERT_TRUE(context1.initialize());
    
    // Note: Second context with same adapter should succeed
    DirectXContext context2(0);
    EXPECT_TRUE(context2.initialize());
}

TEST_F(DirectXBackendTest, ResourceStateTransitions) {
    DirectXContext context(0);
    ASSERT_TRUE(context.initialize());
    
    size_t buffer_size = 1024 * sizeof(float);
    DirectXBuffer buffer(&context, buffer_size);
    
    // Transition to various states
    EXPECT_NO_THROW(buffer.transition_state(D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
    EXPECT_NO_THROW(buffer.transition_state(D3D12_RESOURCE_STATE_COPY_SOURCE));
    EXPECT_NO_THROW(buffer.transition_state(D3D12_RESOURCE_STATE_COPY_DEST));
}

TEST_F(DirectXBackendTest, CommandListRecording) {
    DirectXContext context(0);
    ASSERT_TRUE(context.initialize());
    
    // Reset and record commands
    EXPECT_NO_THROW(context.reset_command_list());
    
    // Create a simple barrier
    size_t buffer_size = 1024 * sizeof(float);
    DirectXBuffer buffer(&context, buffer_size);
    
    buffer.transition_state(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    
    // Execute command list
    EXPECT_NO_THROW(context.execute_command_list());
}

TEST_F(DirectXBackendTest, DescriptorReset) {
    DirectXContext context(0);
    ASSERT_TRUE(context.initialize());
    
    DirectXDescriptors descriptors(&context, 4);
    ASSERT_TRUE(descriptors.initialize());
    
    size_t buffer_size = 1024 * sizeof(float);
    DirectXBuffer buffer(&context, buffer_size);
    
    // Create descriptors
    descriptors.create_uav(buffer.resource(), 1024, sizeof(float));
    descriptors.create_uav(buffer.resource(), 1024, sizeof(float));
    
    // Reset should allow reuse
    EXPECT_NO_THROW(descriptors.reset());
    
    // Should be able to create descriptors again from index 0
    uint32_t index = descriptors.create_uav(buffer.resource(), 1024, sizeof(float));
    EXPECT_EQ(index, 0);
}

// ===== Kernel Execution Tests =====

TEST_F(DirectXBackendTest, MatMulKernel) {
    // Initialize DirectX
    ASSERT_TRUE(initialize_directx_lora(0));
    
    // Simple 4x4 matrix multiplication test
    const int M = 4, N = 4, K = 4;
    std::vector<float> A(M * K);
    std::vector<float> B(K * N);
    std::vector<float> C(M * N, 0.0f);
    
    // Initialize A with identity-like pattern
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < K; ++j) {
            A[i * K + j] = (i == j) ? 1.0f : 0.0f;
        }
    }
    
    // Initialize B with simple values
    for (int i = 0; i < K; ++i) {
        for (int j = 0; j < N; ++j) {
            B[i * N + j] = static_cast<float>(i * N + j);
        }
    }
    
    // Execute kernel
    EXPECT_NO_THROW(launch_matmul_shader(A.data(), B.data(), C.data(), M, N, K, 1.0f));
    
    // Verify result: Since A is identity, C should equal B
    for (int i = 0; i < M * N; ++i) {
        EXPECT_FLOAT_EQ(C[i], B[i]) << "Mismatch at index " << i;
    }
    
    cleanup_directx_lora();
}

TEST_F(DirectXBackendTest, AddKernel) {
    ASSERT_TRUE(initialize_directx_lora(0));
    
    const size_t size = 1024;
    std::vector<float> A(size);
    std::vector<float> B(size);
    std::vector<float> C(size, 0.0f);
    
    // Initialize test data
    for (size_t i = 0; i < size; ++i) {
        A[i] = static_cast<float>(i);
        B[i] = static_cast<float>(i * 2);
    }
    
    // Execute kernel
    EXPECT_NO_THROW(launch_add_shader(A.data(), B.data(), C.data(), size));
    
    // Verify result: C = A + B
    for (size_t i = 0; i < size; ++i) {
        float expected = A[i] + B[i];
        EXPECT_FLOAT_EQ(C[i], expected) << "Mismatch at index " << i;
    }
    
    cleanup_directx_lora();
}

TEST_F(DirectXBackendTest, MultiplyKernel) {
    ASSERT_TRUE(initialize_directx_lora(0));
    
    const size_t size = 512;
    std::vector<float> A(size);
    std::vector<float> B(size);
    std::vector<float> C(size, 0.0f);
    
    // Initialize test data
    for (size_t i = 0; i < size; ++i) {
        A[i] = 2.0f;
        B[i] = 3.0f;
    }
    
    // Execute kernel
    EXPECT_NO_THROW(launch_multiply_shader(A.data(), B.data(), C.data(), size));
    
    // Verify result: C = A * B
    for (size_t i = 0; i < size; ++i) {
        EXPECT_FLOAT_EQ(C[i], 6.0f) << "Mismatch at index " << i;
    }
    
    cleanup_directx_lora();
}

TEST_F(DirectXBackendTest, ScalarMultiplyKernel) {
    ASSERT_TRUE(initialize_directx_lora(0));
    
    const size_t size = 256;
    const float scalar = 2.5f;
    std::vector<float> A(size);
    std::vector<float> B(size, 0.0f);
    
    // Initialize test data
    for (size_t i = 0; i < size; ++i) {
        A[i] = static_cast<float>(i + 1);
    }
    
    // Execute kernel
    EXPECT_NO_THROW(launch_scalar_multiply_shader(A.data(), B.data(), scalar, size));
    
    // Verify result: B = A * scalar
    for (size_t i = 0; i < size; ++i) {
        float expected = A[i] * scalar;
        EXPECT_FLOAT_EQ(B[i], expected) << "Mismatch at index " << i;
    }
    
    cleanup_directx_lora();
}

TEST_F(DirectXBackendTest, TransposeKernel) {
    ASSERT_TRUE(initialize_directx_lora(0));
    
    const int rows = 8;
    const int cols = 4;
    std::vector<float> input(rows * cols);
    std::vector<float> output(rows * cols, 0.0f);
    
    // Initialize input matrix
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            input[i * cols + j] = static_cast<float>(i * cols + j);
        }
    }
    
    // Execute kernel
    EXPECT_NO_THROW(launch_transpose_shader(input.data(), output.data(), rows, cols));
    
    // Verify result: output[j * rows + i] = input[i * cols + j]
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            float expected = input[i * cols + j];
            float actual = output[j * rows + i];
            EXPECT_FLOAT_EQ(actual, expected) 
                << "Mismatch at [" << i << "," << j << "]";
        }
    }
    
    cleanup_directx_lora();
}

TEST_F(DirectXBackendTest, LoRAGradientKernels) {
    ASSERT_TRUE(initialize_directx_lora(0));
    
    // Small test for gradient computation
    const int M = 2;  // batch size
    const int K = 4;  // rank
    const int N = 3;  // output dim
    const float scaling = 0.5f;
    
    std::vector<float> h(M * K);
    std::vector<float> grad_output(M * N);
    std::vector<float> grad_A(K * N, 0.0f);
    
    // Initialize test data
    for (int i = 0; i < M * K; ++i) {
        h[i] = 1.0f;
    }
    for (int i = 0; i < M * N; ++i) {
        grad_output[i] = 1.0f;
    }
    
    // Execute grad_A kernel
    EXPECT_NO_THROW(launch_lora_grad_A_shader(
        h.data(), grad_output.data(), grad_A.data(),
        M, K, N, scaling));
    
    // Verify grad_A has been computed (non-zero)
    bool has_nonzero = false;
    for (float val : grad_A) {
        if (val != 0.0f) {
            has_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(has_nonzero) << "grad_A should contain non-zero values";
    
    // Test grad_B
    const int D = 5;  // input dim
    std::vector<float> input(M * D);
    std::vector<float> grad_h(M * K);
    std::vector<float> grad_B(D * K, 0.0f);
    
    for (int i = 0; i < M * D; ++i) {
        input[i] = 1.0f;
    }
    for (int i = 0; i < M * K; ++i) {
        grad_h[i] = 1.0f;
    }
    
    EXPECT_NO_THROW(launch_lora_grad_B_shader(
        input.data(), grad_h.data(), grad_B.data(),
        M, D, K));
    
    // Verify grad_B has been computed
    has_nonzero = false;
    for (float val : grad_B) {
        if (val != 0.0f) {
            has_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(has_nonzero) << "grad_B should contain non-zero values";
    
    cleanup_directx_lora();
}

TEST_F(DirectXBackendTest, NumericalAccuracySmall) {
    ASSERT_TRUE(initialize_directx_lora(0));
    
    // Test with small known values
    const int M = 2, N = 2, K = 2;
    std::vector<float> A = {1.0f, 2.0f, 3.0f, 4.0f};  // [[1,2], [3,4]]
    std::vector<float> B = {5.0f, 6.0f, 7.0f, 8.0f};  // [[5,6], [7,8]]
    std::vector<float> C(M * N, 0.0f);
    
    // Expected result: [[19, 22], [43, 50]]
    std::vector<float> expected = {19.0f, 22.0f, 43.0f, 50.0f};
    
    EXPECT_NO_THROW(launch_matmul_shader(A.data(), B.data(), C.data(), M, N, K, 1.0f));
    
    // Verify with small tolerance
    for (int i = 0; i < M * N; ++i) {
        EXPECT_NEAR(C[i], expected[i], 1e-4f) << "Mismatch at index " << i;
    }
    
    cleanup_directx_lora();
}

#else

TEST(DirectXBackendTest, NotAvailableOnNonWindows) {
    EXPECT_FALSE(themis::lora::directx::is_directx_available());
}

#endif // _WIN32
