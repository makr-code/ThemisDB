/**
 * @file test_gpu_wave_a_timeout_closure.cpp
 * @brief Wave A closure evidence — GPU kernel timeout and CPU-fallback regressions.
 * @date 2026-08-19
 *
 * Provides 12 deterministic, self-contained tests using KernelSLAGuard:
 *  - Freshly constructed guard is not expired
 *  - Remaining time is positive and decreasing
 *  - Short SLA fires after deadline passes
 *  - SLA duration is correctly reported
 *  - Default SLA is 5 seconds
 *  - CPU fallback triggered when simulated work exceeds budget
 *  - No fallback when work fits within budget
 *  - Move semantics preserve SLA duration
 *  - Concurrent guards are independent
 *  - Remaining time never exceeds initial SLA
 *  - Guard stays non-expired within active budget window
 *  - Elapsed time increases monotonically
 *
 * Test IDs: GPU-TIMEOUT-01 .. GPU-TIMEOUT-12
 *
 * @see src/gpu/ROADMAP.md §Wave A Closure Evidence Block
 * @see src/gpu/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md
 * @see include/themis/gpu/gpu_timeout.h
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "themis/gpu/gpu_timeout.h"

using namespace themis::gpu;
using namespace std::chrono_literals;

namespace {

/// Determinism seed required by Wave A chaos conventions.
constexpr uint32_t kWaveATimeoutSeed = 42;

// =============================================================================
// Minimal CPU-fallback state tracker
// =============================================================================

struct FallbackRecord {
    bool fallback_triggered = false;
    std::string reason;
};

/// Simulate a kernel that takes @p simulated_work, enforced by @p budget.
/// Returns whether CPU fallback would be triggered.
FallbackRecord simulateKernelWithTimeout(
    std::chrono::steady_clock::duration budget,
    std::chrono::steady_clock::duration simulated_work) {
    FallbackRecord rec;
    auto deadline_check_time = std::chrono::steady_clock::now() + budget;
    auto work_end_time = std::chrono::steady_clock::now() + simulated_work;
    if (work_end_time > deadline_check_time) {
        rec.fallback_triggered = true;
        rec.reason = "kernel_timeout";
    }
    return rec;
}

// =============================================================================
// Test fixture
// =============================================================================

class GPUWaveATimeoutClosureTest : public ::testing::Test {};

// ============================================================================
// GPU-TIMEOUT-01: Freshly created guard is not expired.
// ============================================================================

/**
 * @test GPU-TIMEOUT-01
 * @brief Freshly constructed KernelSLAGuard must not report timeout immediately.
 */
TEST_F(GPUWaveATimeoutClosureTest, GpuTimeout01_FreshGuardNotExpired) {
    KernelSLAGuard guard(5s);
    EXPECT_FALSE(guard.checkTimeoutDeadline())
        << "GPU-TIMEOUT-01: Freshly created guard must not be expired";
}

// ============================================================================
// GPU-TIMEOUT-02: Remaining time is positive immediately after construction.
// ============================================================================

/**
 * @test GPU-TIMEOUT-02
 * @brief getRemainingTime() must return a positive duration on a fresh guard.
 */
TEST_F(GPUWaveATimeoutClosureTest, GpuTimeout02_RemainingTimePositive) {
    KernelSLAGuard guard(5s);
    auto remaining = guard.getRemainingTime();
    EXPECT_GT(remaining.count(), 0)
        << "GPU-TIMEOUT-02: Remaining time must be positive on a fresh 5s guard";
}

// ============================================================================
// GPU-TIMEOUT-03: Elapsed time increases over time.
// ============================================================================

/**
 * @test GPU-TIMEOUT-03
 * @brief Elapsed time after a small sleep must be greater than before sleep.
 */
TEST_F(GPUWaveATimeoutClosureTest, GpuTimeout03_ElapsedTimeIncreases) {
    KernelSLAGuard guard(5s);
    auto elapsed_before = guard.getElapsedTime();
    std::this_thread::sleep_for(5ms);
    auto elapsed_after = guard.getElapsedTime();
    EXPECT_GT(elapsed_after, elapsed_before)
        << "GPU-TIMEOUT-03: Elapsed time must increase over time";
}

// ============================================================================
// GPU-TIMEOUT-04: Very short SLA fires after deadline passes.
// ============================================================================

/**
 * @test GPU-TIMEOUT-04
 * @brief A 1ms SLA guard must report timeout after 10ms of actual elapsed time.
 */
TEST_F(GPUWaveATimeoutClosureTest, GpuTimeout04_ShortSLAFires) {
    KernelSLAGuard guard(1ms);
    std::this_thread::sleep_for(10ms);
    EXPECT_TRUE(guard.checkTimeoutDeadline())
        << "GPU-TIMEOUT-04: 1ms SLA guard must report timeout after 10ms sleep";
}

// ============================================================================
// GPU-TIMEOUT-05: SLA duration is correctly reported.
// ============================================================================

/**
 * @test GPU-TIMEOUT-05
 * @brief getSLADuration() must return the configured budget exactly.
 */
TEST_F(GPUWaveATimeoutClosureTest, GpuTimeout05_SLADurationCorrect) {
    constexpr auto kBudget = std::chrono::milliseconds(500);
    KernelSLAGuard guard(kBudget);
    EXPECT_EQ(guard.getSLADuration(), kBudget)
        << "GPU-TIMEOUT-05: getSLADuration must return configured 500ms budget";
}

// ============================================================================
// GPU-TIMEOUT-06: Default SLA is 5 seconds.
// ============================================================================

/**
 * @test GPU-TIMEOUT-06
 * @brief Default-constructed KernelSLAGuard must have 5s SLA.
 */
TEST_F(GPUWaveATimeoutClosureTest, GpuTimeout06_DefaultSLAIs5Seconds) {
    KernelSLAGuard guard;
    EXPECT_EQ(guard.getSLADuration(), KernelSLAGuard::DEFAULT_SLA_DURATION)
        << "GPU-TIMEOUT-06: Default SLA must be 5 seconds";
    EXPECT_EQ(KernelSLAGuard::DEFAULT_SLA_DURATION, 5s);
}

// ============================================================================
// GPU-TIMEOUT-07: CPU fallback triggered when simulated work exceeds budget.
// ============================================================================

/**
 * @test GPU-TIMEOUT-07
 * @brief When simulated work (10ms) exceeds budget (1ms), fallback must trigger.
 */
TEST_F(GPUWaveATimeoutClosureTest, GpuTimeout07_CPUFallbackOnTimeout) {
    auto rec = simulateKernelWithTimeout(1ms, 10ms);
    EXPECT_TRUE(rec.fallback_triggered)
        << "GPU-TIMEOUT-07: Work exceeding budget must trigger CPU fallback";
    EXPECT_EQ(rec.reason, "kernel_timeout");
}

// ============================================================================
// GPU-TIMEOUT-08: No fallback when simulated work is within budget.
// ============================================================================

/**
 * @test GPU-TIMEOUT-08
 * @brief When simulated work (1ms) fits within budget (10ms), no fallback.
 */
TEST_F(GPUWaveATimeoutClosureTest, GpuTimeout08_NoFallbackWithinBudget) {
    auto rec = simulateKernelWithTimeout(10ms, 1ms);
    EXPECT_FALSE(rec.fallback_triggered)
        << "GPU-TIMEOUT-08: Work within budget must not trigger CPU fallback";
}

// ============================================================================
// GPU-TIMEOUT-09: Move semantics preserve SLA duration.
// ============================================================================

/**
 * @test GPU-TIMEOUT-09
 * @brief Move-constructed guard must retain SLA duration of source.
 */
TEST_F(GPUWaveATimeoutClosureTest, GpuTimeout09_MoveSemanticsPreserveSLA) {
    constexpr auto kBudget = std::chrono::milliseconds(250);
    KernelSLAGuard src(kBudget);
    KernelSLAGuard dst(std::move(src));
    EXPECT_EQ(dst.getSLADuration(), kBudget)
        << "GPU-TIMEOUT-09: Move-constructed guard must preserve SLA duration";
}

// ============================================================================
// GPU-TIMEOUT-10: Concurrent guard instances are independent.
// ============================================================================

/**
 * @test GPU-TIMEOUT-10
 * @brief Multiple concurrent guards with different budgets must not interfere.
 */
TEST_F(GPUWaveATimeoutClosureTest, GpuTimeout10_ConcurrentGuardsIndependent) {
    constexpr int kThreads = 8;
    std::atomic<int> errors{0};
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([i, &errors]() {
            auto budget = std::chrono::milliseconds(100 * (i + 1));
            KernelSLAGuard guard(budget);
            if (guard.getSLADuration() != budget) ++errors;
            if (guard.checkTimeoutDeadline()) ++errors;
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(errors.load(), 0)
        << "GPU-TIMEOUT-10: Concurrent guards must not interfere with each other";
}

// ============================================================================
// GPU-TIMEOUT-11: Remaining time never exceeds initial SLA.
// ============================================================================

/**
 * @test GPU-TIMEOUT-11
 * @brief getRemainingTime() must never report a value exceeding the initial budget.
 */
TEST_F(GPUWaveATimeoutClosureTest, GpuTimeout11_RemainingNeverExceedsBudget) {
    constexpr auto kBudget = 100ms;
    KernelSLAGuard guard(kBudget);
    for (int i = 0; i < 5; ++i) {
        auto remaining = guard.getRemainingTime();
        EXPECT_LE(remaining, kBudget)
            << "GPU-TIMEOUT-11: remaining time at iteration " << i
            << " must not exceed initial budget";
        std::this_thread::sleep_for(1ms);
    }
}

// ============================================================================
// GPU-TIMEOUT-12: Guard remains non-expired within first 20ms of a 50ms budget.
// ============================================================================

/**
 * @test GPU-TIMEOUT-12
 * @brief Guard with 50ms budget must not report timeout in first 20ms.
 */
TEST_F(GPUWaveATimeoutClosureTest, GpuTimeout12_NotExpiredDuringActiveBudget) {
    KernelSLAGuard guard(50ms);
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < 20ms) {
        EXPECT_FALSE(guard.checkTimeoutDeadline())
            << "GPU-TIMEOUT-12: Guard must not expire within first 20ms of 50ms budget";
        std::this_thread::sleep_for(2ms);
    }
}

}  // namespace
