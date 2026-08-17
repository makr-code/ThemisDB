#include <gtest/gtest.h>

#include "gpu/gpu_safe_raii.h"
#include "gpu/kernel_timeout_enforcer.h"

#include <chrono>
#include <stdexcept>
#include <thread>

using namespace themis::gpu;

TEST(GPUKernelTimeoutEnforcerTest, ExecuteWithTimeoutCompletesWithinBudget) {
    KernelTimeoutEnforcer enforcer;
    KernelTimeoutEnforcer::KernelConfig config;
    config.timeout_ms = 50;

    const bool completed = enforcer.executeWithTimeout(
        []() { std::this_thread::sleep_for(std::chrono::milliseconds(5)); },
        config);

    EXPECT_TRUE(completed);
}

TEST(GPUKernelTimeoutEnforcerTest, ExecuteWithTimeoutReturnsFalseOnTimeout) {
    KernelTimeoutEnforcer enforcer;
    KernelTimeoutEnforcer::KernelConfig config;
    config.timeout_ms = 10;

    const bool completed = enforcer.executeWithTimeout(
        []() { std::this_thread::sleep_for(std::chrono::milliseconds(30)); },
        config);

    EXPECT_FALSE(completed);
}

TEST(GPUKernelTimeoutEnforcerTest, ExecuteWithFallbackRunsCpuPathOnTimeout) {
    KernelTimeoutEnforcer enforcer;
    KernelTimeoutEnforcer::KernelConfig config;
    config.timeout_ms = 10;

    bool cpu_fallback_ran = false;
    const bool used_gpu = enforcer.executeWithFallback(
        []() { std::this_thread::sleep_for(std::chrono::milliseconds(30)); },
        [&cpu_fallback_ran]() { cpu_fallback_ran = true; },
        config);

    EXPECT_FALSE(used_gpu);
    EXPECT_TRUE(cpu_fallback_ran);
}

TEST(GPUKernelTimeoutEnforcerTest, ExecuteWithFallbackRunsCpuPathOnException) {
    KernelTimeoutEnforcer enforcer;
    KernelTimeoutEnforcer::KernelConfig config;
    config.timeout_ms = 50;

    bool cpu_fallback_ran = false;
    const bool used_gpu = enforcer.executeWithFallback(
        []() { throw std::runtime_error("gpu failure"); },
        [&cpu_fallback_ran]() { cpu_fallback_ran = true; },
        config);

    EXPECT_FALSE(used_gpu);
    EXPECT_TRUE(cpu_fallback_ran);
}

TEST(GPUKernelTimeoutEnforcerTest, ExecuteWithFallbackRethrowsWithoutCpuPath) {
    KernelTimeoutEnforcer enforcer;
    KernelTimeoutEnforcer::KernelConfig config;

    EXPECT_THROW(
        [&]() {
            static_cast<void>(enforcer.executeWithFallback(
                []() { throw std::runtime_error("gpu failure"); },
                nullptr,
                config));
        }(),
        std::runtime_error);
}

TEST(GPUSafeRaiiTest, KernelTimeoutGuardReportsTimeoutWithoutDestroyingStream) {
    KernelTimeoutGuard guard(nullptr, 20);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(guard.didTimeout());
    guard.markCompleted();
}

TEST(GPUSafeRaiiTest, KernelTimeoutGuardStaysClearAfterEarlyCompletion) {
    KernelTimeoutGuard guard(nullptr, 100);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    guard.markCompleted();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_FALSE(guard.didTimeout());
}

TEST(GPUSafeRaiiTest, DeviceMemoryGuardDefaultConstructsEmpty) {
    DeviceMemoryGuard<int> guard;
    EXPECT_FALSE(guard.isValid());
    EXPECT_EQ(guard.size(), 0u);
    EXPECT_EQ(guard.get(), nullptr);
}

#if !THEMIS_GPU_SAFE_RAII_HAS_CUDA
TEST(GPUSafeRaiiTest, DeviceMemoryGuardThrowsWhenCudaRuntimeUnavailable) {
    EXPECT_THROW(
        []() {
            DeviceMemoryGuard<int> guard(4);
            static_cast<void>(guard);
        }(),
        std::runtime_error);
}
#endif
