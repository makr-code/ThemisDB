// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_rpc_grpc_circuit_breaker_focused.cpp
 * @brief Focused unit tests for the plugin-level CircuitBreaker in rpc_grpc.
 *
 * Test IDs: CB-01 through CB-12
 *
 * Tests cover all three state transitions, probe semantics, min_calls_in_window
 * guard, forceState(), reset(), stats snapshot consistency, and the
 * transition-callback hook.  All tests are deterministic and require no
 * external state.
 *
 * @see include/rpc_grpc/circuit_breaker.h
 * @see src/rpc_grpc/circuit_breaker.cpp
 * @see plugins/rpc/ROADMAP.md — Phase 3 circuit-breaker gate
 */

#include "gtest/gtest.h"
#include "rpc_grpc/circuit_breaker.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace themis::rpc;
using namespace std::chrono_literals;

namespace {

/// Build a config with a short recovery window to make OPEN→HALF_OPEN
/// transitions testable without real sleep.
CircuitBreakerConfig fastConfig(uint32_t threshold = 3,
                                std::chrono::milliseconds window = 20ms) {
    CircuitBreakerConfig cfg;
    cfg.failure_threshold         = threshold;
    cfg.min_calls_in_window       = 1;
    cfg.recovery_window           = window;
    cfg.half_open_success_threshold = 1;
    cfg.name                      = "test-cb";
    return cfg;
}

} // anonymous namespace

// ============================================================================
// CB-01 — Initial state is CLOSED
// ============================================================================

TEST(CircuitBreakerFocused, CB01_InitialStateClosed) {
    CircuitBreaker cb(fastConfig());
    EXPECT_EQ(cb.state(), CircuitState::kClosed);
}

// ============================================================================
// CB-02 — allowRequest() returns true in CLOSED state
// ============================================================================

TEST(CircuitBreakerFocused, CB02_AllowRequestWhenClosed) {
    CircuitBreaker cb(fastConfig());
    EXPECT_TRUE(cb.allowRequest());
}

// ============================================================================
// CB-03 — After failure_threshold consecutive failures, state transitions OPEN
// ============================================================================

TEST(CircuitBreakerFocused, CB03_TripsOpenAfterThreshold) {
    CircuitBreaker cb(fastConfig(/*threshold=*/3));

    for (int i = 0; i < 3; ++i) {
        ASSERT_TRUE(cb.allowRequest());
        cb.recordResult(false);
    }

    EXPECT_EQ(cb.state(), CircuitState::kOpen);
}

// ============================================================================
// CB-04 — allowRequest() returns false in OPEN state (before recovery window)
// ============================================================================

TEST(CircuitBreakerFocused, CB04_RejectWhenOpen) {
    CircuitBreaker cb(fastConfig(/*threshold=*/2, /*window=*/5s));

    for (int i = 0; i < 2; ++i) {
        ASSERT_TRUE(cb.allowRequest());
        cb.recordResult(false);
    }
    ASSERT_EQ(cb.state(), CircuitState::kOpen);

    // Immediately after tripping, all new calls are rejected.
    EXPECT_FALSE(cb.allowRequest());
    EXPECT_FALSE(cb.allowRequest());
}

// ============================================================================
// CB-05 — After recovery window elapses, state transitions to HALF_OPEN and
//          one probe is admitted.
// ============================================================================

TEST(CircuitBreakerFocused, CB05_HalfOpenAfterRecoveryWindow) {
    CircuitBreaker cb(fastConfig(/*threshold=*/2, /*window=*/20ms));

    for (int i = 0; i < 2; ++i) {
        ASSERT_TRUE(cb.allowRequest());
        cb.recordResult(false);
    }
    ASSERT_EQ(cb.state(), CircuitState::kOpen);

    // Wait for the recovery window to elapse.
    std::this_thread::sleep_for(30ms);

    // First call after window: admitted as probe, state → HALF_OPEN.
    EXPECT_TRUE(cb.allowRequest());
    EXPECT_EQ(cb.state(), CircuitState::kHalfOpen);

    // Concurrent second call: rejected because probe is in-flight.
    EXPECT_FALSE(cb.allowRequest());
}

// ============================================================================
// CB-06 — Probe success in HALF_OPEN transitions back to CLOSED.
// ============================================================================

TEST(CircuitBreakerFocused, CB06_ProbeSuccessCloses) {
    CircuitBreaker cb(fastConfig(/*threshold=*/2, /*window=*/20ms));

    for (int i = 0; i < 2; ++i) {
        ASSERT_TRUE(cb.allowRequest());
        cb.recordResult(false);
    }
    std::this_thread::sleep_for(30ms);

    ASSERT_TRUE(cb.allowRequest());  // probe
    ASSERT_EQ(cb.state(), CircuitState::kHalfOpen);

    cb.recordResult(true);  // probe success
    EXPECT_EQ(cb.state(), CircuitState::kClosed);
}

// ============================================================================
// CB-07 — Probe failure in HALF_OPEN transitions back to OPEN.
// ============================================================================

TEST(CircuitBreakerFocused, CB07_ProbeFailureReopens) {
    CircuitBreaker cb(fastConfig(/*threshold=*/2, /*window=*/20ms));

    for (int i = 0; i < 2; ++i) {
        ASSERT_TRUE(cb.allowRequest());
        cb.recordResult(false);
    }
    std::this_thread::sleep_for(30ms);

    ASSERT_TRUE(cb.allowRequest());  // probe
    ASSERT_EQ(cb.state(), CircuitState::kHalfOpen);

    cb.recordResult(false);  // probe failure
    EXPECT_EQ(cb.state(), CircuitState::kOpen);
}

// ============================================================================
// CB-08 — reset() returns the circuit to CLOSED and clears counters.
// ============================================================================

TEST(CircuitBreakerFocused, CB08_ResetCloses) {
    CircuitBreaker cb(fastConfig(/*threshold=*/2));

    for (int i = 0; i < 2; ++i) {
        ASSERT_TRUE(cb.allowRequest());
        cb.recordResult(false);
    }
    ASSERT_EQ(cb.state(), CircuitState::kOpen);

    cb.reset();
    EXPECT_EQ(cb.state(), CircuitState::kClosed);
    EXPECT_TRUE(cb.allowRequest());
}

// ============================================================================
// CB-09 — stats() returns a consistent snapshot (counters add up).
// ============================================================================

TEST(CircuitBreakerFocused, CB09_StatsConsistency) {
    CircuitBreaker cb(fastConfig(/*threshold=*/10));

    for (int i = 0; i < 3; ++i) {
        ASSERT_TRUE(cb.allowRequest());
        cb.recordResult(true);
    }
    for (int i = 0; i < 2; ++i) {
        ASSERT_TRUE(cb.allowRequest());
        cb.recordResult(false);
    }

    const auto s = cb.stats();
    EXPECT_EQ(s.state, CircuitState::kClosed);
    // total_calls counts both allowRequest+recordResult pairs and rejected calls;
    // here no calls were rejected, so total = 5 (3 successes + 2 failures) + extra
    // allowRequest calls = 5.
    EXPECT_EQ(s.successful_calls, 3u);
    EXPECT_EQ(s.failed_calls, 2u);
    EXPECT_EQ(s.rejected_calls, 0u);
    EXPECT_EQ(s.name, "test-cb");
}

// ============================================================================
// CB-10 — setTransitionCallback() fires on each state change.
// ============================================================================

TEST(CircuitBreakerFocused, CB10_TransitionCallbackFires) {
    CircuitBreaker cb(fastConfig(/*threshold=*/2, /*window=*/20ms));

    std::vector<std::pair<CircuitState, CircuitState>> transitions;
    cb.setTransitionCallback([&](CircuitState from, CircuitState to, const std::string&) {
        transitions.emplace_back(from, to);
    });

    // Trip to OPEN.
    for (int i = 0; i < 2; ++i) {
        ASSERT_TRUE(cb.allowRequest());
        cb.recordResult(false);
    }
    ASSERT_EQ(cb.state(), CircuitState::kOpen);

    // Wait and probe.
    std::this_thread::sleep_for(30ms);
    ASSERT_TRUE(cb.allowRequest());

    // Close via successful probe.
    cb.recordResult(true);

    ASSERT_EQ(transitions.size(), 3u);
    EXPECT_EQ(transitions[0], std::make_pair(CircuitState::kClosed, CircuitState::kOpen));
    EXPECT_EQ(transitions[1], std::make_pair(CircuitState::kOpen,   CircuitState::kHalfOpen));
    EXPECT_EQ(transitions[2], std::make_pair(CircuitState::kHalfOpen, CircuitState::kClosed));
}

// ============================================================================
// CB-11 — forceState() sets the state and allowRequest() respects it.
// ============================================================================

TEST(CircuitBreakerFocused, CB11_ForceState) {
    CircuitBreaker cb(fastConfig());

    cb.forceState(CircuitState::kOpen);
    EXPECT_EQ(cb.state(), CircuitState::kOpen);
    // forceState(OPEN) with a very long recovery window should reject calls.
    CircuitBreakerConfig cfg;
    cfg.failure_threshold = 3;
    cfg.min_calls_in_window = 1;
    cfg.recovery_window = 60s;  // Will not elapse during the test.
    cfg.half_open_success_threshold = 1;
    CircuitBreaker cb2(cfg);
    cb2.forceState(CircuitState::kOpen);
    EXPECT_FALSE(cb2.allowRequest());

    cb2.forceState(CircuitState::kClosed);
    EXPECT_EQ(cb2.state(), CircuitState::kClosed);
    EXPECT_TRUE(cb2.allowRequest());
}

// ============================================================================
// CB-12 — min_calls_in_window prevents tripping on fewer calls than the window.
// ============================================================================

TEST(CircuitBreakerFocused, CB12_MinCallsWindowPreventsEarlyTrip) {
    CircuitBreakerConfig cfg;
    cfg.failure_threshold       = 2;
    cfg.min_calls_in_window     = 5;   // Require at least 5 calls before tripping.
    cfg.recovery_window         = 5s;
    cfg.half_open_success_threshold = 1;

    CircuitBreaker cb(cfg);

    // 2 failures are below min_calls_in_window — circuit must stay CLOSED.
    for (int i = 0; i < 2; ++i) {
        ASSERT_TRUE(cb.allowRequest());
        cb.recordResult(false);
    }
    EXPECT_EQ(cb.state(), CircuitState::kClosed);

    // 3 more failures now meet min_calls_in_window while exceeding threshold.
    for (int i = 0; i < 3; ++i) {
        ASSERT_TRUE(cb.allowRequest());
        cb.recordResult(false);
    }
    EXPECT_EQ(cb.state(), CircuitState::kOpen);
}
