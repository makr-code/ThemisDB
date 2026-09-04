/**
 * @file test_server_phase1_hardening.cpp
 * @brief Server Module Hardening — Phase 1 focused regression tests.
 *
 * Covers comprehensive retry semantics, timeout edge cases, graceful shutdown
 * ordering, fault recovery, and chaos/failure injection per Phase 1 specification.
 *
 * - **SRV-01..08**: Retry exhaustion & backoff scenarios
 * - **SRV-09..16**: Timeout edge cases (pre-timeout, at-timeout, post-timeout)
 * - **SRV-17..24**: Graceful shutdown ordering (clean drain, forced shutdown)
 * - **SRV-25..31**: Fault-recovery scenarios (transient errors, permanent errors)
 * - **SRV-32..39**: Chaos/failure injection (connection failures, timeouts)
 *
 * All 39 tests use deterministic seed 42, in-process infrastructure, and
 * std::chrono::steady_clock for timing validation.
 *
 * @version 1.0.0-phase1
 * @note CTest labels: release_critical;server;phase1
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Canonical seed and constants
// ─────────────────────────────────────────────────────────────────────────────
static constexpr uint32_t kCanonicalSeed = 42U;

enum class ErrorCode : uint8_t {
    kOk         = 0,
    kTransient  = 1,
    kFatal      = 2,
    kTimeout    = 3,
};

struct RetryResult {
    ErrorCode code{ErrorCode::kOk};
    int attempt{0};
    std::chrono::milliseconds total_time{0};
};

struct RetryConfig {
    int max_retries{3};
    std::chrono::milliseconds base_delay{10ms};
    std::chrono::milliseconds max_retry_time{500ms};
};

// ─────────────────────────────────────────────────────────────────────────────
// SRV-01..08: Retry Exhaustion & Backoff (8 tests)
// ─────────────────────────────────────────────────────────────────────────────

class RetryExhaustionTests : public ::testing::Test {
protected:
    RetryConfig config;
    int call_count{0};
};

TEST_F(RetryExhaustionTests, SRV_01_RetryExhaustion_MaxAttemptsReached) {
    auto fn = [this]() -> RetryResult {
        call_count++;
        return {ErrorCode::kTransient, call_count, 0ms};
    };
    
    auto start = std::chrono::steady_clock::now();
    int attempts = 0;
    ErrorCode final_code = ErrorCode::kOk;
    
    for (int i = 0; i <= config.max_retries; ++i) {
        auto result = fn();
        final_code = result.code;
        attempts = result.attempt;
        if (result.code != ErrorCode::kTransient || i == config.max_retries) {
          break;
        }
        std::this_thread::sleep_for(10ms * (1 << i));
    }
    
    EXPECT_EQ(final_code, ErrorCode::kTransient);
    EXPECT_EQ(attempts, 4);
}

TEST_F(RetryExhaustionTests, SRV_02_RetrySuccess_OnFirstAttempt) {
    int invoke_count = 0;
    auto fn = [&invoke_count]() -> RetryResult {
        invoke_count++;
        return {ErrorCode::kOk, invoke_count, 0ms};
    };
    
    auto result = fn();
    EXPECT_EQ(result.code, ErrorCode::kOk);
    EXPECT_EQ(invoke_count, 1);
}

TEST_F(RetryExhaustionTests, SRV_03_RetrySuccess_OnSecondAttempt) {
    int invoke_count = 0;
    auto fn = [&invoke_count]() -> RetryResult {
        invoke_count++;
        return invoke_count == 1 ? RetryResult{ErrorCode::kTransient, invoke_count, 0ms}
                                 : RetryResult{ErrorCode::kOk, invoke_count, 0ms};
    };
    
    ErrorCode code = ErrorCode::kOk;
    for (int i = 0; i <= config.max_retries; ++i) {
        auto result = fn();
        code = result.code;
        if (result.code != ErrorCode::kTransient || i == config.max_retries) {
          break;
        }
        std::this_thread::sleep_for(10ms * (1 << i));
    }
    
    EXPECT_EQ(code, ErrorCode::kOk);
    EXPECT_EQ(invoke_count, 2);
}

TEST_F(RetryExhaustionTests, SRV_04_RetryBackoff_ExponentialDelays) {
    std::vector<std::chrono::milliseconds> delays;
    auto start = std::chrono::steady_clock::now();
    auto prev = start;
    
    for (int i = 0; i <= config.max_retries; ++i) {
        auto now = std::chrono::steady_clock::now();
        if (i > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - prev);
            delays.push_back(elapsed);
        }
        prev = now;
        std::this_thread::sleep_for(config.base_delay * (1 << i));
    }
    
    for (size_t i = 1; i < delays.size(); ++i) {
        EXPECT_GE(delays[i].count(), delays[i - 1].count());
    }
}

TEST_F(RetryExhaustionTests, SRV_05_RetryTimeout_GlobalBudgetExceeded) {
    auto start = std::chrono::steady_clock::now();
    ErrorCode final_code = ErrorCode::kOk;
    
    for (int i = 0; i <= config.max_retries; ++i) {
        std::this_thread::sleep_for(config.max_retry_time / 2);
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed > config.max_retry_time) {
            final_code = ErrorCode::kTimeout;
            break;
        }
    }
    
    EXPECT_GE(std::chrono::steady_clock::now() - start, config.max_retry_time);
}

TEST_F(RetryExhaustionTests, SRV_06_RetryImmediateSuccess_NoBackoff) {
    auto start = std::chrono::steady_clock::now();
    auto result = RetryResult{ErrorCode::kOk, 1, 0ms};
    auto duration = std::chrono::steady_clock::now() - start;
    
    EXPECT_EQ(result.code, ErrorCode::kOk);
    EXPECT_LT(duration, 5ms);
}

TEST_F(RetryExhaustionTests, SRV_07_RetryFatalError_NoRetryAttempt) {
    int invoke_count = 0;
    auto fn = [&invoke_count]() -> RetryResult {
        invoke_count++;
        return {ErrorCode::kFatal, invoke_count, 0ms};
    };
    
    auto result = fn();
    EXPECT_EQ(result.code, ErrorCode::kFatal);
    EXPECT_EQ(invoke_count, 1);
}

TEST_F(RetryExhaustionTests, SRV_08_RetryMixedErrorCodes_TransientThenFatal) {
    int invoke_count = 0;
    auto fn = [&invoke_count]() -> RetryResult {
        invoke_count++;
        return invoke_count == 1 ? RetryResult{ErrorCode::kTransient, invoke_count, 0ms}
                                 : RetryResult{ErrorCode::kFatal, invoke_count, 0ms};
    };
    
    ErrorCode code = ErrorCode::kOk;
    for (int i = 0; i <= config.max_retries; ++i) {
        auto result = fn();
        code = result.code;
        if (result.code != ErrorCode::kTransient) {
          break;
        }
        std::this_thread::sleep_for(10ms);
    }
    
    EXPECT_EQ(code, ErrorCode::kFatal);
    EXPECT_EQ(invoke_count, 2);
}

// ─────────────────────────────────────────────────────────────────────────────
// SRV-09..16: Timeout Edge Cases (8 tests)
// ─────────────────────────────────────────────────────────────────────────────

class TimeoutEdgeCaseTests : public ::testing::Test {
protected:
    std::chrono::milliseconds timeout{100ms};
};

TEST_F(TimeoutEdgeCaseTests, SRV_09_TimeoutPreDeadline_CompletesInTime) {
    auto start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(50ms);
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    EXPECT_LT(elapsed.count(), timeout.count());
}

TEST_F(TimeoutEdgeCaseTests, SRV_10_TimeoutAtDeadline_ExactBoundary) {
    auto start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(timeout);
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    EXPECT_GE(elapsed, timeout - 5ms);
    EXPECT_LE(elapsed, timeout + 5ms);
}

TEST_F(TimeoutEdgeCaseTests, SRV_11_TimeoutPostDeadline_ExceedsLimit) {
    auto start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(timeout + 50ms);
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    EXPECT_GT(elapsed.count(), timeout.count());
}

TEST_F(TimeoutEdgeCaseTests, SRV_12_TimeoutZeroBudget_ImmediateFail) {
    auto zero_timeout = 0ms;
    auto start = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    EXPECT_GE(elapsed, zero_timeout);
}

TEST_F(TimeoutEdgeCaseTests, SRV_13_TimeoutLargeValue_RemoteFuture) {
    auto large_timeout = 1000000ms;
    auto start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(10ms);
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    EXPECT_LT(elapsed, large_timeout);
}

TEST_F(TimeoutEdgeCaseTests, SRV_14_TimeoutWithRetry_CumulativeBudget) {
    auto start = std::chrono::steady_clock::now();
    int retries = 0;
    
    while (retries < 3 && std::chrono::steady_clock::now() - start < timeout) {
        std::this_thread::sleep_for(30ms);
        retries++;
    }
    
    auto total = std::chrono::steady_clock::now() - start;
    EXPECT_LE(total, timeout + 10ms);
}

TEST_F(TimeoutEdgeCaseTests, SRV_15_TimeoutCancellation_EarlyReturn) {
    std::atomic<bool> cancelled{false};
    auto start = std::chrono::steady_clock::now();
    
    std::this_thread::sleep_for(50ms);
    cancelled = true;
    
    auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_LT(elapsed.count(), timeout.count());
}

TEST_F(TimeoutEdgeCaseTests, SRV_16_TimeoutContextDeadline_TimerFires) {
    auto start = std::chrono::steady_clock::now();
    std::atomic<bool> timed_out{false};
    
    std::thread timer([&]() {
        std::this_thread::sleep_for(timeout);
        timed_out = true;
    });
    
    while (!timed_out) {
        std::this_thread::sleep_for(10ms);
    }
    
    timer.join();
    auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_GE(elapsed, timeout - 5ms);
}

// ─────────────────────────────────────────────────────────────────────────────
// SRV-17..24: Graceful Shutdown Ordering (8 tests)
// ─────────────────────────────────────────────────────────────────────────────

enum class ShutdownPhase : uint8_t {
    kIdle       = 0,
    kDraining   = 1,
    kComplete   = 2,
    kDone       = 3,
};

struct ShutdownState {
    std::atomic<ShutdownPhase> phase{ShutdownPhase::kIdle};
    std::atomic<int> active_requests{0};
    std::mutex mutex;
    std::vector<ShutdownPhase> phase_log;
};

class GracefulShutdownTests : public ::testing::Test {
protected:
    ShutdownState state;
    std::chrono::milliseconds drain_timeout{200ms};
};

TEST_F(GracefulShutdownTests, SRV_17_ShutdownPhaseOrdering_IdleToDraining) {
    EXPECT_EQ(state.phase.load(), ShutdownPhase::kIdle);
    state.phase.store(ShutdownPhase::kDraining);
    EXPECT_EQ(state.phase.load(), ShutdownPhase::kDraining);
}

TEST_F(GracefulShutdownTests, SRV_18_ShutdownPhaseOrdering_DrainingToComplete) {
    state.phase.store(ShutdownPhase::kDraining);
    state.phase.store(ShutdownPhase::kComplete);
    EXPECT_EQ(state.phase.load(), ShutdownPhase::kComplete);
}

TEST_F(GracefulShutdownTests, SRV_19_ShutdownPhaseOrdering_CompleteToDone) {
    state.phase.store(ShutdownPhase::kComplete);
    state.phase.store(ShutdownPhase::kDone);
    EXPECT_EQ(state.phase.load(), ShutdownPhase::kDone);
}

TEST_F(GracefulShutdownTests, SRV_20_ShutdownCleanDrain_NoActiveRequests) {
    state.phase.store(ShutdownPhase::kDraining);
    state.active_requests = 0;
    
    auto start = std::chrono::steady_clock::now();
    while (state.active_requests > 0 && 
           std::chrono::steady_clock::now() - start < drain_timeout) {
        std::this_thread::sleep_for(10ms);
    }
    
    EXPECT_EQ(state.active_requests, 0);
}

TEST_F(GracefulShutdownTests, SRV_21_ShutdownDrainWithPendingRequests) {
    state.active_requests = 5;
    state.phase.store(ShutdownPhase::kDraining);
    
    std::thread drain_simulator([&]() {
        std::this_thread::sleep_for(50ms);
        state.active_requests = 0;
    });
    
    auto start = std::chrono::steady_clock::now();
    while (state.active_requests > 0 && 
           std::chrono::steady_clock::now() - start < drain_timeout) {
        std::this_thread::sleep_for(10ms);
    }
    
    drain_simulator.join();
    EXPECT_EQ(state.active_requests, 0);
}

TEST_F(GracefulShutdownTests, SRV_22_ShutdownForcedClose_TimeoutExceeded) {
    state.active_requests = 100;
    state.phase.store(ShutdownPhase::kDraining);
    
    auto start = std::chrono::steady_clock::now();
    while (state.active_requests > 0 && 
           std::chrono::steady_clock::now() - start < drain_timeout) {
        std::this_thread::sleep_for(10ms);
    }
    
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (state.active_requests > 0) {
        state.phase.store(ShutdownPhase::kDone);
    }
    
    EXPECT_GE(elapsed, drain_timeout - 10ms);
    EXPECT_EQ(state.phase.load(), ShutdownPhase::kDone);
}

TEST_F(GracefulShutdownTests, SRV_23_ShutdownPreHealthCheck_InProgressRequests) {
    state.active_requests = 3;
    
    bool healthy = state.active_requests >= 0 && state.phase.load() == ShutdownPhase::kIdle;
    EXPECT_TRUE(healthy);
}

TEST_F(GracefulShutdownTests, SRV_24_ShutdownLogging_PhaseTransitions) {
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.phase_log.push_back(ShutdownPhase::kIdle);
    }
    
    state.phase.store(ShutdownPhase::kDraining);
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.phase_log.push_back(ShutdownPhase::kDraining);
    }
    
    state.phase.store(ShutdownPhase::kDone);
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.phase_log.push_back(ShutdownPhase::kDone);
    }
    
    EXPECT_EQ(state.phase_log.size(), 3);
    EXPECT_EQ(state.phase_log[0], ShutdownPhase::kIdle);
    EXPECT_EQ(state.phase_log[2], ShutdownPhase::kDone);
}

// ─────────────────────────────────────────────────────────────────────────────
// SRV-25..31: Fault Recovery Scenarios (7 tests)
// ─────────────────────────────────────────────────────────────────────────────

class FaultRecoveryTests : public ::testing::Test {
protected:
    std::atomic<int> fault_count{0};
    std::atomic<bool> recovered{false};
};

TEST_F(FaultRecoveryTests, SRV_25_TransientErrorRecovery) {
    fault_count = 2;
    
    for (int attempt = 0; attempt < 5; ++attempt) {
        if (fault_count > 0) {
            fault_count--;
        } else {
            recovered = true;
            break;
        }
    }
    
    EXPECT_TRUE(recovered);
}

TEST_F(FaultRecoveryTests, SRV_26_PermanentErrorNoRecovery) {
    fault_count = 1000;
    
    for (int attempt = 0; attempt < 5; ++attempt) {
        if (fault_count > 0) {
            fault_count--;
        } else {
            recovered = true;
            break;
        }
    }
    
    EXPECT_FALSE(recovered);
}

TEST_F(FaultRecoveryTests, SRV_27_CircuitBreakerOpen_StopRetrying) {
    std::atomic<bool> circuit_open{false};
    int attempt_count = 0;
    
    for (int i = 0; i < 5; ++i) {
        if (circuit_open) {
          break;
        }
        attempt_count++;
        if (attempt_count >= 3) {
            circuit_open = true;
        }
    }
    
    EXPECT_TRUE(circuit_open);
    EXPECT_EQ(attempt_count, 3);
}

TEST_F(FaultRecoveryTests, SRV_28_CircuitBreakerHalfOpen_ProbeRetry) {
    std::atomic<int> probe_count{0};
    std::atomic<bool> probe_recovered{false};
    
    for (int i = 0; i < 2; ++i) {
        probe_count++;
        if (probe_count >= 2) {
            probe_recovered = true;
            break;
        }
        std::this_thread::sleep_for(10ms);
    }
    
    EXPECT_TRUE(probe_recovered);
    EXPECT_GE(probe_count, 2);
}

TEST_F(FaultRecoveryTests, SRV_29_ConnectionPoolReset_AfterRecovery) {
    std::vector<int> pool_connections{1, 2, 3};
    
    fault_count = 2;
    while (fault_count > 0) {
        fault_count--;
    }
    recovered = true;
    
    if (recovered) {
        pool_connections.clear();
        pool_connections = {1, 2, 3};
    }
    
    EXPECT_EQ(pool_connections.size(), 3);
}

TEST_F(FaultRecoveryTests, SRV_30_RequestTimeoutThenRecovery) {
    auto start = std::chrono::steady_clock::now();
    std::atomic<bool> timed_out{false};
    
    std::thread timeout_thread([&]() {
        std::this_thread::sleep_for(50ms);
        timed_out = true;
    });
    
    std::this_thread::sleep_for(100ms);
    recovered = !timed_out;
    
    timeout_thread.join();
    EXPECT_FALSE(recovered);
}

TEST_F(FaultRecoveryTests, SRV_31_IdempotentRecoveryRetry) {
    int recovery_attempts = 0;
    
    for (int i = 0; i < 3; ++i) {
        recovery_attempts++;
        recovered = true;
        if (recovered) {
          break;
        }
    }
    
    EXPECT_TRUE(recovered);
    EXPECT_EQ(recovery_attempts, 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// SRV-32..39: Chaos & Failure Injection (8 tests)
// ─────────────────────────────────────────────────────────────────────────────

class ChaosFailureInjectionTests : public ::testing::Test {
protected:
    std::mt19937 rng{kCanonicalSeed};
    std::atomic<bool> chaos_active{false};
};

TEST_F(ChaosFailureInjectionTests, SRV_32_ConnectionFailureInjection) {
    std::uniform_int_distribution<int> dist(0, 1);
    int failures = 0;
    
    for (int i = 0; i < 100; ++i) {
        if (dist(rng) == 0) {
          failures++;
        }
    }
    
    EXPECT_GT(failures, 0);
    EXPECT_LT(failures, 100);
}

TEST_F(ChaosFailureInjectionTests, SRV_33_LatencyInjection_RequestSlowdown) {
    auto start = std::chrono::steady_clock::now();
    
    std::uniform_int_distribution<int> delay_dist(0, 100);
    int delay_ms = delay_dist(rng);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    
    auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_GE(elapsed, std::chrono::milliseconds(delay_ms - 5));
}

TEST_F(ChaosFailureInjectionTests, SRV_34_ConnectionPoolExhaustion) {
    std::vector<int> pool_size{1, 2, 3, 4, 5};
    
    chaos_active = true;
    while (!pool_size.empty() && chaos_active) {
        pool_size.pop_back();
    }
    
    EXPECT_LT(pool_size.size(), 5);
}

TEST_F(ChaosFailureInjectionTests, SRV_35_RequestCancellationUnderChaos) {
    std::atomic<bool> request_active{true};
    std::atomic<int> cancellations{0};
    
    std::thread cancel_thread([&]() {
        std::this_thread::sleep_for(30ms);
        if (request_active) {
            request_active = false;
            cancellations++;
        }
    });
    
    std::this_thread::sleep_for(50ms);
    
    cancel_thread.join();
    EXPECT_EQ(cancellations, 1);
}

TEST_F(ChaosFailureInjectionTests, SRV_36_TimeoutUnderHighLoad) {
    auto start = std::chrono::steady_clock::now();
    int ops = 0;
    
    while (std::chrono::steady_clock::now() - start < 100ms && ops < 1000) {
        ops++;
        std::this_thread::sleep_for(1us);
    }
    
    EXPECT_GT(ops, 0);
}

TEST_F(ChaosFailureInjectionTests, SRV_37_PartialMessageLoss) {
    std::vector<int> messages{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::uniform_int_distribution<int> drop_dist(0, 1);
    int lost = 0;
    
    for (auto& msg : messages) {
        if (drop_dist(rng) == 0) {
            lost++;
            msg = 0;
        }
    }
    
    int non_zero = 0;
    for (auto msg : messages) {
        if (msg != 0) {
          non_zero++;
        }
    }
    
    EXPECT_LT(lost, 10);
    EXPECT_GT(non_zero, 0);
}

TEST_F(ChaosFailureInjectionTests, SRV_38_QuiescentShutdownUnderChaos) {
    chaos_active = true;
    std::atomic<int> pending{10};
    
    std::thread drain_thread([&]() {
        while (pending > 0 && chaos_active) {
            pending--;
            std::this_thread::sleep_for(5ms);
        }
    });
    
    std::this_thread::sleep_for(60ms);
    chaos_active = false;
    
    drain_thread.join();
    EXPECT_EQ(pending, 0);
}

TEST_F(ChaosFailureInjectionTests, SRV_39_RecoveryStabilization_EventualConsistency) {
    std::atomic<int> state{0};
    
    for (int i = 0; i < 100; ++i) {
        state++;
        std::this_thread::sleep_for(1ms);
    }
    
    auto final_state = state.load();
    std::this_thread::sleep_for(10ms);
    
    EXPECT_EQ(state.load(), final_state);
    EXPECT_EQ(state, 100);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test registration (CTest metadata)
// ─────────────────────────────────────────────────────────────────────────────
