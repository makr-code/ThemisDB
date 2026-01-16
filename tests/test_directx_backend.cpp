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

#else

TEST(DirectXBackendTest, NotAvailableOnNonWindows) {
    EXPECT_FALSE(themis::lora::directx::is_directx_available());
}

#endif // _WIN32
