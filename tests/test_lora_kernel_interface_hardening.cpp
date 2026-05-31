#include <gtest/gtest.h>

#include "llm/lora_framework/vulkan_kernels.h"
#include "llm/lora_framework/directx_kernels.h"
#include "llm/lora_framework/gpu_lora_layers.h"

#include <array>
#include <atomic>
#include <chrono>
#include <future>
#include <limits>
#include <string>

namespace {

using namespace themis::lora;
using namespace themis::llm::lora;

TEST(LoRAKernelInterfaceHardeningTest, GPULoRALayerConstructorRejectsInvalidDimensions) {
    EXPECT_THROW((void)GPULoRALayer(0, 8, 4, 1.0f, Device::cpu(), false), std::invalid_argument);
    EXPECT_THROW((void)GPULoRALayer(8, 0, 4, 1.0f, Device::cpu(), false), std::invalid_argument);
    EXPECT_THROW((void)GPULoRALayer(8, 8, 0, 1.0f, Device::cpu(), false), std::invalid_argument);
    EXPECT_THROW((void)GPULoRALayer(8, 8, 4, std::numeric_limits<float>::infinity(), Device::cpu(), false), std::invalid_argument);
}

TEST(LoRAKernelInterfaceHardeningTest, GPULoRALayerRejectsForwardBackwardShapeMismatch) {
    GPULoRALayer layer(/*in_dim=*/4, /*out_dim=*/3, /*rank=*/2, 1.0f, Device::cpu(), false);

    GPUTensor wrong_input({2, 5}, Device::cpu());
    EXPECT_THROW((void)layer.forward(wrong_input), std::invalid_argument);

    GPUTensor input({2, 4}, Device::cpu());
    auto output = layer.forward(input);
    EXPECT_EQ(output.shape(), std::vector<size_t>({2, 3}));

    GPUTensor wrong_grad({2, 2}, Device::cpu());
    EXPECT_THROW((void)layer.backward(wrong_grad), std::invalid_argument);
}

TEST(LoRAKernelInterfaceHardeningTest, GPULoRALayerRejectsBackwardBeforeForward) {
    GPULoRALayer layer(/*in_dim=*/4, /*out_dim=*/3, /*rank=*/2, 1.0f, Device::cpu(), false);
    GPUTensor grad_output({2, 3}, Device::cpu());

    EXPECT_THROW((void)layer.backward(grad_output), std::runtime_error);
}

TEST(LoRAKernelInterfaceHardeningTest, VulkanUninitializedCallsFailFast) {
    std::array<float, 4> a{1.0f, 2.0f, 3.0f, 4.0f};
    std::array<float, 4> b{5.0f, 6.0f, 7.0f, 8.0f};
    std::array<float, 4> c{0.0f, 0.0f, 0.0f, 0.0f};

    EXPECT_THROW(vulkan::launch_matmul_shader(a.data(), b.data(), c.data(), 2, 2, 2, 1.0f), std::runtime_error);
    EXPECT_THROW(vulkan::launch_add_shader(a.data(), b.data(), c.data(), 4), std::runtime_error);
    EXPECT_THROW(vulkan::launch_sequence_mean_shader(c.data(), a.data(), 1, 2, 2), std::runtime_error);
}

TEST(LoRAKernelInterfaceHardeningTest, VulkanInitializedInvalidDimensionsFailClosed) {
    if (!vulkan::is_vulkan_available()) {
        GTEST_SKIP() << "Vulkan unavailable on this host";
    }
    ASSERT_TRUE(vulkan::initialize_vulkan_lora(0));

    std::array<float, 4> a{1.0f, 2.0f, 3.0f, 4.0f};
    std::array<float, 4> b{5.0f, 6.0f, 7.0f, 8.0f};
    std::array<float, 4> c{0.0f, 0.0f, 0.0f, 0.0f};

    EXPECT_THROW(vulkan::launch_fused_lora_forward(
        a.data(), b.data(), b.data(), c.data(),
        /*batch_size=*/0, /*in_dim=*/2, /*rank=*/2, /*out_dim=*/2, /*scaling=*/1.0f),
        std::invalid_argument);

    vulkan::cleanup_vulkan_lora();
}

TEST(LoRAKernelInterfaceHardeningTest, VulkanConcurrentLifecycleNoLockTimeout) {
    std::atomic<bool> saw_lock_timeout{false};

    auto worker = [&saw_lock_timeout]() {
        for (int i = 0; i < 20; ++i) {
            try {
                (void)vulkan::initialize_vulkan_lora(0);
                vulkan::cleanup_vulkan_lora();
            } catch (const std::exception& ex) {
                if (std::string(ex.what()).find("Timeout while waiting for Vulkan kernel state lock") != std::string::npos) {
                    saw_lock_timeout.store(true, std::memory_order_relaxed);
                }
            }
        }
    };

    auto fut_a = std::async(std::launch::async, worker);
    auto fut_b = std::async(std::launch::async, worker);

    ASSERT_EQ(fut_a.wait_for(std::chrono::seconds(20)), std::future_status::ready);
    ASSERT_EQ(fut_b.wait_for(std::chrono::seconds(20)), std::future_status::ready);
    fut_a.get();
    fut_b.get();

    EXPECT_FALSE(saw_lock_timeout.load(std::memory_order_relaxed));
}

#if defined(_WIN32) && defined(THEMIS_ENABLE_DIRECTX)

TEST(LoRAKernelInterfaceHardeningTest, DirectXInvalidInputRejectedBeforeDispatch) {
    if (!directx::is_directx_available()) {
        GTEST_SKIP() << "DirectX unavailable on this host";
    }
    ASSERT_TRUE(directx::initialize_directx_lora(0));
    std::array<float, 4> a{1.0f, 2.0f, 3.0f, 4.0f};
    std::array<float, 4> b{0.0f, 0.0f, 0.0f, 0.0f};

    EXPECT_THROW(directx::launch_add_shader(nullptr, a.data(), b.data(), 4), std::invalid_argument);
    EXPECT_THROW(directx::launch_transpose_shader(a.data(), b.data(), 0, 2), std::invalid_argument);

    directx::cleanup_directx_lora();
}

TEST(LoRAKernelInterfaceHardeningTest, DirectXConcurrentLifecycleNoLockTimeout) {
    std::atomic<bool> saw_lock_timeout{false};

    auto worker = [&saw_lock_timeout]() {
        for (int i = 0; i < 20; ++i) {
            try {
                (void)directx::initialize_directx_lora(0);
                directx::cleanup_directx_lora();
            } catch (const std::exception& ex) {
                if (std::string(ex.what()).find("Timeout while waiting for DirectX kernel state lock") != std::string::npos) {
                    saw_lock_timeout.store(true, std::memory_order_relaxed);
                }
            }
        }
    };

    auto fut_a = std::async(std::launch::async, worker);
    auto fut_b = std::async(std::launch::async, worker);

    ASSERT_EQ(fut_a.wait_for(std::chrono::seconds(20)), std::future_status::ready);
    ASSERT_EQ(fut_b.wait_for(std::chrono::seconds(20)), std::future_status::ready);
    fut_a.get();
    fut_b.get();

    EXPECT_FALSE(saw_lock_timeout.load(std::memory_order_relaxed));
}

#else

TEST(LoRAKernelInterfaceHardeningTest, DirectXTestsDisabledWhenFeatureOff) {
    GTEST_SKIP() << "DirectX LoRA kernels are disabled in this build";
}

#endif

} // namespace
