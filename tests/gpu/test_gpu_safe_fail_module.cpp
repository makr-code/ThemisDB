#include <gtest/gtest.h>
#include "themis/gpu/safe_fail.h"
#include <thread>
#include <chrono>

using namespace themis::gpu;

// Helper: build a config with a short reset timeout so circuit tests don't
// take long.
static GPUSafeFail::Config FastConfig() {
    GPUSafeFail::Config cfg;
    cfg.failure_threshold     = 3;
    cfg.success_threshold     = 2;
    cfg.circuit_reset_timeout = std::chrono::seconds(1);
    cfg.enable_cpu_fallback   = true;
    return cfg;
}

// ---------------------------------------------------------------------------
// Initial state
// ---------------------------------------------------------------------------

TEST(GPUSafeFailTest, InitialState_Healthy) {
    GPUSafeFail sf;
    EXPECT_TRUE(sf.isHealthy());
    EXPECT_TRUE(sf.shouldAttemptGPU());
    const auto s = sf.getStatus();
    EXPECT_EQ(s.state, GPUSafeFail::State::HEALTHY);
    EXPECT_EQ(s.consecutive_failures, 0u);
    EXPECT_EQ(s.total_operations, 0u);
}

// ---------------------------------------------------------------------------
// RecordSuccess
// ---------------------------------------------------------------------------

TEST(GPUSafeFailTest, RecordSuccess_CountsOperation) {
    GPUSafeFail sf;
    sf.recordSuccess();
    const auto s = sf.getStatus();
    EXPECT_EQ(s.total_operations, 1u);
    EXPECT_EQ(s.consecutive_successes, 1u);
    EXPECT_EQ(s.state, GPUSafeFail::State::HEALTHY);
}

// ---------------------------------------------------------------------------
// RecordFailure — state transitions
// ---------------------------------------------------------------------------

TEST(GPUSafeFailTest, RecordFailure_TransitionsToDegraded) {
    GPUSafeFail sf(FastConfig());
    sf.recordFailure(GPUSafeFail::FailureType::MEMORY_ERROR, "OOM");
    const auto s = sf.getStatus();
    EXPECT_EQ(s.state, GPUSafeFail::State::DEGRADED);
    EXPECT_TRUE(sf.isHealthy());  // DEGRADED still counts as healthy
    EXPECT_TRUE(sf.shouldAttemptGPU());
}

TEST(GPUSafeFailTest, RecordFailure_ThresholdOpensCircuit) {
    GPUSafeFail sf(FastConfig());
    for (size_t i = 0; i < FastConfig().failure_threshold; ++i) {
        sf.recordFailure(GPUSafeFail::FailureType::DEVICE_ERROR, "err");
    }
    EXPECT_EQ(sf.getStatus().state, GPUSafeFail::State::CIRCUIT_OPEN);
    EXPECT_FALSE(sf.shouldAttemptGPU());
    EXPECT_FALSE(sf.isHealthy());
}

TEST(GPUSafeFailTest, RecordSuccess_AfterDegraded_RecoversToHealthy) {
    GPUSafeFail sf(FastConfig());
    sf.recordFailure(GPUSafeFail::FailureType::TIMEOUT, "slow");
    EXPECT_EQ(sf.getStatus().state, GPUSafeFail::State::DEGRADED);

    for (size_t i = 0; i < FastConfig().success_threshold; ++i) {
        sf.recordSuccess();
    }
    EXPECT_EQ(sf.getStatus().state, GPUSafeFail::State::HEALTHY);
}

// ---------------------------------------------------------------------------
// executeWithFallback — happy path
// ---------------------------------------------------------------------------

TEST(GPUSafeFailTest, Execute_GPUSucceeds_NoCPUFallback) {
    GPUSafeFail sf(FastConfig());
    bool gpu_ran = false, cpu_ran = false;

    bool ok = sf.executeWithFallback(
        [&] { gpu_ran = true; return true; },
        [&] { cpu_ran = true; return true; },
        "op");

    EXPECT_TRUE(ok);
    EXPECT_TRUE(gpu_ran);
    EXPECT_FALSE(cpu_ran);
    EXPECT_EQ(sf.getStatus().total_fallbacks, 0u);
}

// ---------------------------------------------------------------------------
// executeWithFallback — GPU fails, CPU saves it
// ---------------------------------------------------------------------------

TEST(GPUSafeFailTest, Execute_GPUFails_FallbackToCPU) {
    GPUSafeFail sf(FastConfig());
    bool gpu_ran = false, cpu_ran = false;

    bool ok = sf.executeWithFallback(
        [&] { gpu_ran = true; return false; },
        [&] { cpu_ran = true; return true; },
        "op");

    EXPECT_TRUE(ok);
    EXPECT_TRUE(gpu_ran);
    EXPECT_TRUE(cpu_ran);
    EXPECT_EQ(sf.getStatus().total_fallbacks, 1u);
    EXPECT_TRUE(sf.getStatus().cpu_fallback_active);
}

// ---------------------------------------------------------------------------
// executeWithFallback — circuit open, GPU skipped
// ---------------------------------------------------------------------------

TEST(GPUSafeFailTest, Execute_CircuitOpen_GPUSkipped) {
    GPUSafeFail sf(FastConfig());
    // Open the circuit.
    for (size_t i = 0; i < FastConfig().failure_threshold; ++i) {
        sf.recordFailure(GPUSafeFail::FailureType::DEVICE_ERROR, "err");
    }
    EXPECT_FALSE(sf.shouldAttemptGPU());

    bool gpu_ran = false, cpu_ran = false;
    bool ok = sf.executeWithFallback(
        [&] { gpu_ran = true; return true; },
        [&] { cpu_ran = true; return true; },
        "op");

    EXPECT_TRUE(ok);
    EXPECT_FALSE(gpu_ran);  // GPU must not be attempted when circuit is open
    EXPECT_TRUE(cpu_ran);
}

// ---------------------------------------------------------------------------
// executeWithFallback — no fallback provided, fails
// ---------------------------------------------------------------------------

TEST(GPUSafeFailTest, Execute_NoFallback_CircuitOpen_ReturnsFalse) {
    GPUSafeFail sf(FastConfig());
    for (size_t i = 0; i < FastConfig().failure_threshold; ++i) {
        sf.recordFailure(GPUSafeFail::FailureType::DEVICE_ERROR, "err");
    }
    bool ok = sf.executeWithFallback(
        [] { return true; },
        nullptr,
        "op");
    EXPECT_FALSE(ok);
}

// ---------------------------------------------------------------------------
// executeWithFallback — GPU throws, fallback saves it
// ---------------------------------------------------------------------------

TEST(GPUSafeFailTest, Execute_GPUThrows_FallbackToCPU) {
    GPUSafeFail sf(FastConfig());
    bool cpu_ran = false;

    bool ok = sf.executeWithFallback(
        [] { throw std::runtime_error("kernel crash"); return false; },
        [&] { cpu_ran = true; return true; },
        "op");

    EXPECT_TRUE(ok);
    EXPECT_TRUE(cpu_ran);
    EXPECT_GE(sf.getStatus().total_failures, 1u);
}

// ---------------------------------------------------------------------------
// Circuit reset
// ---------------------------------------------------------------------------

TEST(GPUSafeFailTest, CircuitReset_AfterTimeout) {
    GPUSafeFail sf(FastConfig());
    for (size_t i = 0; i < FastConfig().failure_threshold; ++i) {
        sf.recordFailure(GPUSafeFail::FailureType::DEVICE_ERROR, "err");
    }
    EXPECT_EQ(sf.getStatus().state, GPUSafeFail::State::CIRCUIT_OPEN);
    EXPECT_FALSE(sf.canResetCircuit());  // too soon

    std::this_thread::sleep_for(FastConfig().circuit_reset_timeout +
                                std::chrono::milliseconds(100));

    EXPECT_TRUE(sf.canResetCircuit());
    sf.tryResetCircuit();
    EXPECT_EQ(sf.getStatus().state, GPUSafeFail::State::DEGRADED);
    EXPECT_TRUE(sf.shouldAttemptGPU());
}

// ---------------------------------------------------------------------------
// Force states
// ---------------------------------------------------------------------------

TEST(GPUSafeFailTest, ForceHealthy_ClearsCircuit) {
    GPUSafeFail sf(FastConfig());
    for (size_t i = 0; i < FastConfig().failure_threshold; ++i) {
        sf.recordFailure(GPUSafeFail::FailureType::DEVICE_ERROR, "err");
    }
    EXPECT_FALSE(sf.isHealthy());
    sf.forceHealthy();
    EXPECT_TRUE(sf.isHealthy());
    EXPECT_TRUE(sf.shouldAttemptGPU());
    EXPECT_EQ(sf.getStatus().state, GPUSafeFail::State::HEALTHY);
}

TEST(GPUSafeFailTest, ForceFailed_BlocksGPU) {
    GPUSafeFail sf;
    sf.forceFailed("maintenance");
    EXPECT_FALSE(sf.isHealthy());
    EXPECT_FALSE(sf.shouldAttemptGPU());
    EXPECT_EQ(sf.getStatus().state, GPUSafeFail::State::FAILED);
    EXPECT_EQ(sf.getStatus().last_error, "maintenance");
}

// ---------------------------------------------------------------------------
// Error rate
// ---------------------------------------------------------------------------

TEST(GPUSafeFailTest, ErrorRate_CalculatedCorrectly) {
    GPUSafeFail sf;
    // 3 successes — no fallback provided, GPU succeeds.
    for (int i = 0; i < 3; ++i) {
        bool ok = sf.executeWithFallback([] { return true; }, nullptr, "ok");
        EXPECT_TRUE(ok);
    }
    // 2 failures — no fallback provided, operation returns false.
    for (int i = 0; i < 2; ++i) {
        bool ok = sf.executeWithFallback([] { return false; }, nullptr, "fail");
        EXPECT_FALSE(ok);
    }
    // 5 total ops, 2 failures → rate = 0.4
    EXPECT_NEAR(sf.getErrorRate(), 0.4f, 0.01f);
}

// ---------------------------------------------------------------------------
// checkMemoryAvailable
// ---------------------------------------------------------------------------

TEST(GPUSafeFailTest, CheckMemory_Sufficient) {
    GPUSafeFail sf;
    EXPECT_TRUE(sf.checkMemoryAvailable(100, 500, 1000));
}

TEST(GPUSafeFailTest, CheckMemory_InsufficientBytes) {
    GPUSafeFail sf;
    EXPECT_FALSE(sf.checkMemoryAvailable(600, 500, 1000));
}

TEST(GPUSafeFailTest, CheckMemory_AboveOOMThreshold) {
    GPUSafeFail::Config cfg;
    cfg.oom_threshold = 0.90f;
    GPUSafeFail sf(cfg);
    // 950/1000 used → 95% > threshold of 90%, only 50 left, requesting 30.
    EXPECT_FALSE(sf.checkMemoryAvailable(30, 50, 1000));
}

TEST(GPUSafeFailTest, CheckMemory_BelowOOMThreshold) {
    GPUSafeFail::Config cfg;
    cfg.oom_threshold = 0.90f;
    GPUSafeFail sf(cfg);
    // 800/1000 used → 80% < threshold of 90%, 200 left, requesting 100.
    EXPECT_TRUE(sf.checkMemoryAvailable(100, 200, 1000));
}

// ---------------------------------------------------------------------------
// Total fallback counter across multiple operations
// ---------------------------------------------------------------------------

TEST(GPUSafeFailTest, TotalFallbacks_CountsAllFallbackOps) {
    GPUSafeFail sf(FastConfig());
    for (int i = 0; i < 4; ++i) {
        sf.executeWithFallback(
            [] { return false; },
            [] { return true; },
            "op");
    }
    EXPECT_EQ(sf.getStatus().total_fallbacks, 4u);
}
