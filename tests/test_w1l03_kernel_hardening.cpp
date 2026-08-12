#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <stdexcept>
#include <atomic>

#include "llm/lora_framework/vulkan_kernels.h"
#include "llm/lora_framework/directx_kernels.h"

namespace {

// ---------------------------------------------------------------------------
// Vulkan — guard path tests (no GPU required)
// ---------------------------------------------------------------------------

TEST(VulkanKernelHardening, LaunchMatmulThrowsWhenNotInitialized) {
    float A[4] = {1, 2, 3, 4};
    float B[4] = {1, 0, 0, 1};
    float C[4] = {};
    EXPECT_THROW(
        themis::lora::vulkan::launch_matmul_shader(A, B, C, 2, 2, 2, 1.0f),
        std::runtime_error);
}

TEST(VulkanKernelHardening, LaunchAddThrowsWhenNotInitialized) {
    float A[4] = {}, B[4] = {}, C[4] = {};
    EXPECT_THROW(
        themis::lora::vulkan::launch_add_shader(A, B, C, 4),
        std::runtime_error);
}

TEST(VulkanKernelHardening, LaunchTransposeThrowsWhenNotInitialized) {
    float in[4] = {}, out[4] = {};
    EXPECT_THROW(
        themis::lora::vulkan::launch_transpose_shader(in, out, 2, 2),
        std::runtime_error);
}

TEST(VulkanKernelHardening, EmbeddingLookupRejectsNullPointers) {
    // Null-check happens BEFORE acquiring the mutex, so no GPU needed.
    EXPECT_THROW(
        themis::lora::vulkan::launch_embedding_lookup_shader(
            nullptr, nullptr, nullptr, 1, 1, 8, 16),
        std::invalid_argument);
}

TEST(VulkanKernelHardening, EmbeddingLookupRejectsZeroDimensions) {
    float out[8] = {}, ids[1] = {}, emb[8] = {};
    EXPECT_THROW(
        themis::lora::vulkan::launch_embedding_lookup_shader(
            out, ids, emb, 0, 1, 8, 16),
        std::invalid_argument);
}

TEST(VulkanKernelHardening, SequenceMeanRejectsNullPointers) {
    EXPECT_THROW(
        themis::lora::vulkan::launch_sequence_mean_shader(
            nullptr, nullptr, 1, 4, 8),
        std::invalid_argument);
}

TEST(VulkanKernelHardening, SequenceMeanRejectsZeroDimensions) {
    float out[8] = {}, in[32] = {};
    EXPECT_THROW(
        themis::lora::vulkan::launch_sequence_mean_shader(out, in, 0, 4, 8),
        std::invalid_argument);
}

TEST(VulkanKernelHardening, FusedForwardRejectsNullPointers) {
    EXPECT_THROW(
        themis::lora::vulkan::launch_fused_lora_forward(
            nullptr, nullptr, nullptr, nullptr, 1, 8, 4, 8, 1.0f),
        std::invalid_argument);
}

TEST(VulkanKernelHardening, FusedForwardRejectsZeroDimensions) {
    float buf[8] = {};
    EXPECT_THROW(
        themis::lora::vulkan::launch_fused_lora_forward(
            buf, buf, buf, buf, 0, 8, 4, 8, 1.0f),
        std::invalid_argument);
}

// Verify concurrent uninitialized calls don't crash (race on the guard).
// Each thread should receive a std::runtime_error, not a data race or crash.
TEST(VulkanKernelHardening, ConcurrentUninitializedCallsAreRaceFree) {
    std::atomic<int> throws{0};
    std::atomic<int> unexpected{0};
    float A[4] = {}, B[4] = {}, C[4] = {};

    auto worker = [&]() {
        try {
            themis::lora::vulkan::launch_matmul_shader(A, B, C, 2, 2, 2, 1.0f);
        } catch (const std::runtime_error&) {
            throws.fetch_add(1, std::memory_order_relaxed);
        } catch (...) {
            unexpected.fetch_add(1, std::memory_order_relaxed);
        }
    };

    constexpr int kThreads = 8;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back(worker);
    }
    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(throws.load(), kThreads) << "Expected all threads to throw runtime_error";
    EXPECT_EQ(unexpected.load(), 0)    << "No unexpected exceptions should escape";
}

// ---------------------------------------------------------------------------
// DirectX — non-Windows stub path always throws std::runtime_error
// ---------------------------------------------------------------------------

#if defined(THEMIS_ENABLE_DIRECTX)

TEST(DirectXKernelHardening, IsDirectXAvailableReturnsFalseOnNonWindows) {
#ifndef _WIN32
    EXPECT_FALSE(themis::lora::directx::is_directx_available());
#else
    GTEST_SKIP() << "Test only meaningful on non-Windows";
#endif
}

TEST(DirectXKernelHardening, LaunchMatmulThrowsOnNonWindows) {
#ifndef _WIN32
    float A[4] = {}, B[4] = {}, C[4] = {};
    EXPECT_THROW(
        themis::lora::directx::launch_matmul_shader(A, B, C, 2, 2, 2, 1.0f),
        std::runtime_error);
#else
    GTEST_SKIP() << "Test only meaningful on non-Windows";
#endif
}

TEST(DirectXKernelHardening, EmbeddingLookupNullCheckBeforeMutex) {
    // On non-Windows: the stub throws runtime_error, but the null check in
    // the Windows path should be hit before the mutex on Windows.
    // On non-Windows we just verify the stub throws cleanly.
#ifdef _WIN32
    EXPECT_THROW(
        themis::lora::directx::launch_embedding_lookup_shader(
            nullptr, nullptr, nullptr, 1, 1, 8, 16),
        std::invalid_argument);
#else
    EXPECT_THROW(
        themis::lora::directx::launch_embedding_lookup_shader(
            nullptr, nullptr, nullptr, 1, 1, 8, 16),
        std::runtime_error
    );
#endif
}

TEST(DirectXKernelHardening, SequenceMeanNullCheckBeforeMutex) {
#ifdef _WIN32
    EXPECT_THROW(
        themis::lora::directx::launch_sequence_mean_shader(
            nullptr, nullptr, 1, 4, 8),
        std::invalid_argument);
#else
    EXPECT_THROW(
        themis::lora::directx::launch_sequence_mean_shader(
            nullptr, nullptr, 1, 4, 8),
        std::runtime_error
    );
#endif
}

#ifdef _WIN32
// On Windows only: guard-path tests without actual GPU
TEST(DirectXKernelHardening, LaunchMatmulThrowsWhenNotInitialized) {
    if (!themis::lora::directx::is_directx_available()) {
        GTEST_SKIP() << "DirectX 12 not available";
    }
    float A[4] = {}, B[4] = {}, C[4] = {};
    // initialize_directx_lora was NOT called → must throw
    EXPECT_THROW(
        themis::lora::directx::launch_matmul_shader(A, B, C, 2, 2, 2, 1.0f),
        std::runtime_error);
}

TEST(DirectXKernelHardening, EmbeddingLookupRejectsNullOnWindows) {
    EXPECT_THROW(
        themis::lora::directx::launch_embedding_lookup_shader(
            nullptr, nullptr, nullptr, 1, 1, 8, 16),
        std::invalid_argument);
}

TEST(DirectXKernelHardening, EmbeddingLookupRejectsZeroDimensionsOnWindows) {
    float out[8] = {}, ids[1] = {}, emb[8] = {};
    EXPECT_THROW(
        themis::lora::directx::launch_embedding_lookup_shader(
            out, ids, emb, 0, 1, 8, 16),
        std::invalid_argument);
}

TEST(DirectXKernelHardening, ConcurrentUninitializedCallsAreRaceFree) {
    std::atomic<int> throws{0};
    std::atomic<int> unexpected{0};
    float A[4] = {}, B[4] = {}, C[4] = {};

    auto worker = [&]() {
        try {
            themis::lora::directx::launch_matmul_shader(A, B, C, 2, 2, 2, 1.0f);
        } catch (const std::runtime_error&) {
            throws.fetch_add(1, std::memory_order_relaxed);
        } catch (...) {
            unexpected.fetch_add(1, std::memory_order_relaxed);
        }
    };

    constexpr int kThreads = 8;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back(worker);
    }
    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(throws.load(), kThreads);
    EXPECT_EQ(unexpected.load(), 0);
}
#endif // _WIN32

#else

TEST(DirectXKernelHardening, DisabledWhenDirectXFeatureOff) {
    GTEST_SKIP() << "DirectX LoRA kernels are disabled in this build";
}

#endif

} // namespace
