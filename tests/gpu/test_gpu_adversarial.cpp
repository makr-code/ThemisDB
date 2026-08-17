#include <gtest/gtest.h>

#include "llm/gpu_safe_fail.h"
#include "tests/utils/fault_injector.h"

#include <cstdint>
#include <vector>

using namespace std::chrono_literals;

namespace themis::test {

using themis::llm::GPUSafeFailManager;

namespace {

GPUSafeFailManager::Config makeConfig() {
    GPUSafeFailManager::Config cfg;
    cfg.failure_threshold = 2;
    cfg.success_threshold = 1;
    cfg.circuit_reset_timeout = std::chrono::seconds{1};
    cfg.enable_cpu_fallback = true;
    return cfg;
}

}  // namespace

TEST(GPUAdversarialTest, DeterministicCorruptionUsesSeededFaultInjector) {
    CorruptionInjector::CorruptionConfig cfg;
    cfg.target_component = "gpu-buffer";
    cfg.corruption_rate = 0.25f;
    cfg.rng_seed = 42;

    CorruptionInjector injector_a(cfg);
    CorruptionInjector injector_b(cfg);
    ASSERT_TRUE(injector_a.inject().success);
    ASSERT_TRUE(injector_b.inject().success);

    std::vector<uint8_t> lhs(128, 0xAA);
    std::vector<uint8_t> rhs(128, 0xAA);

    const size_t lhs_corrupted = injector_a.corruptData(lhs.data(), lhs.size());
    const size_t rhs_corrupted = injector_b.corruptData(rhs.data(), rhs.size());

    EXPECT_EQ(lhs_corrupted, rhs_corrupted);
    EXPECT_EQ(lhs, rhs);
}

TEST(GPUAdversarialTest, TimeoutFaultFallsBackToCPU) {
    GPUSafeFailManager manager(makeConfig());

    TimeoutInjector::TimeoutConfig timeout_cfg;
    timeout_cfg.target_component = "cuda-kernel";
    timeout_cfg.duration = 100ms;
    timeout_cfg.trigger_immediately = true;
    timeout_cfg.auto_recover = false;
    TimeoutInjector timeout(timeout_cfg);
    ASSERT_TRUE(timeout.inject().success);

    bool gpu_attempted = false;
    bool cpu_used = false;
    const bool result = manager.executeWithFallback(
        [&]() {
            gpu_attempted = true;
            return !timeout.shouldTimeout();
        },
        [&]() {
            cpu_used = true;
            return true;
        },
        "adversarial_timeout");

    EXPECT_TRUE(result);
    EXPECT_TRUE(gpu_attempted);
    EXPECT_TRUE(cpu_used);
    EXPECT_TRUE(manager.getHealthStatus().is_cpu_fallback_active);
    EXPECT_TRUE(timeout.recover().success);
}

TEST(GPUAdversarialTest, RepeatedAdversarialFailuresOpenCircuit) {
    GPUSafeFailManager manager(makeConfig());

    for (int i = 0; i < 2; ++i) {
        manager.recordFailure(GPUSafeFailManager::FailureType::KERNEL_ERROR,
                              "deterministic adversarial failure");
    }

    EXPECT_FALSE(manager.shouldAttemptGPU());
    EXPECT_EQ(manager.getHealthStatus().state,
              GPUSafeFailManager::GPUState::CIRCUIT_OPEN);
}

TEST(GPUAdversarialTest, OpenCircuitSkipsGpuAndUsesCPUOnly) {
    GPUSafeFailManager manager(makeConfig());
    manager.recordFailure(GPUSafeFailManager::FailureType::DEVICE_ERROR, "fail-1");
    manager.recordFailure(GPUSafeFailManager::FailureType::DEVICE_ERROR, "fail-2");

    bool gpu_attempted = false;
    bool cpu_used = false;
    const bool result = manager.executeWithFallback(
        [&]() {
            gpu_attempted = true;
            return true;
        },
        [&]() {
            cpu_used = true;
            return true;
        },
        "open_circuit_cpu_fallback");

    EXPECT_TRUE(result);
    EXPECT_FALSE(gpu_attempted);
    EXPECT_TRUE(cpu_used);
}

}  // namespace themis::test
