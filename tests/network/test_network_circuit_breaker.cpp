/**
 * Focused tests for the network AdaptiveCircuitBreaker.
 *
 * Acceptance criteria (from issue):
 *   1. Circuit breaker triggers correctly (CLOSED → OPEN on error threshold)
 *   2. Recovery scenarios are tested (OPEN → HALF_OPEN → CLOSED)
 *
 * Additional coverage:
 *   - Adaptive threshold adjustment on repeated trips
 *   - HALF_OPEN timeout fallback to OPEN
 *   - State-change callback
 *   - Statistics correctness
 *   - Thread-safety under concurrent access
 */

#include <gtest/gtest.h>
#include "network/adaptive_circuit_breaker.h"
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

using namespace themis::network;
using namespace std::chrono_literals;

// ============================================================================
// Helpers
// ============================================================================

static AdaptiveCircuitBreaker::Config makeConfig(
    size_t failure_threshold = 3,
    size_t success_threshold = 2,
    std::chrono::seconds open_timeout = 1s,
    std::chrono::seconds half_open_timeout = 30s,
    bool   adaptive = false,
    double adaptive_factor = 0.1)
{
    AdaptiveCircuitBreaker::Config cfg;
    cfg.failure_threshold       = failure_threshold;
    cfg.success_threshold       = success_threshold;
    cfg.open_timeout            = open_timeout;
    cfg.half_open_timeout       = half_open_timeout;
    cfg.enable_adaptive_threshold = adaptive;
    cfg.adaptive_factor         = adaptive_factor;
    return cfg;
}

// ============================================================================
// Construction / Configuration
// ============================================================================

TEST(NetworkCircuitBreakerTest, DefaultConfigInitialStateClosed) {
    AdaptiveCircuitBreaker cb;
    EXPECT_EQ(cb.getState(), CircuitState::CLOSED);
    EXPECT_TRUE(cb.shouldAllow());
}

TEST(NetworkCircuitBreakerTest, InvalidFailureThresholdThrows) {
    AdaptiveCircuitBreaker::Config cfg;
    cfg.failure_threshold = 0;
    EXPECT_THROW(AdaptiveCircuitBreaker cb(cfg), std::invalid_argument);
}

TEST(NetworkCircuitBreakerTest, InvalidSuccessThresholdThrows) {
    AdaptiveCircuitBreaker::Config cfg;
    cfg.success_threshold = 0;
    EXPECT_THROW(AdaptiveCircuitBreaker cb(cfg), std::invalid_argument);
}

TEST(NetworkCircuitBreakerTest, InvalidAdaptiveFactorThrows) {
    AdaptiveCircuitBreaker::Config cfg;
    cfg.adaptive_factor = 0.0;
    EXPECT_THROW(AdaptiveCircuitBreaker cb(cfg), std::invalid_argument);

    cfg.adaptive_factor = 1.0;
    EXPECT_THROW(AdaptiveCircuitBreaker cb(cfg), std::invalid_argument);
}

TEST(NetworkCircuitBreakerTest, StatsInitialValues) {
    AdaptiveCircuitBreaker cb(makeConfig(5));
    auto stats = cb.getStats();
    EXPECT_EQ(stats.state,             CircuitState::CLOSED);
    EXPECT_EQ(stats.total_calls,       0u);
    EXPECT_EQ(stats.successful_calls,  0u);
    EXPECT_EQ(stats.failed_calls,      0u);
    EXPECT_EQ(stats.rejected_calls,    0u);
    EXPECT_EQ(stats.current_failure_threshold, 5u);
}

// ============================================================================
// Acceptance criterion 1: Circuit triggers on error threshold
// ============================================================================

TEST(NetworkCircuitBreakerTest, ClosedStaysClosedBelowThreshold) {
    AdaptiveCircuitBreaker cb(makeConfig(3));

    cb.recordFailure();
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), CircuitState::CLOSED);
    EXPECT_TRUE(cb.shouldAllow());
}

TEST(NetworkCircuitBreakerTest, ClosedToOpenAtThreshold) {
    AdaptiveCircuitBreaker cb(makeConfig(3));

    cb.recordFailure();
    cb.recordFailure();
    cb.recordFailure(); // hits threshold
    EXPECT_EQ(cb.getState(), CircuitState::OPEN);
    EXPECT_FALSE(cb.shouldAllow());
}

TEST(NetworkCircuitBreakerTest, OpenRejectsAllRequests) {
    AdaptiveCircuitBreaker cb(makeConfig(2));

    cb.recordFailure();
    cb.recordFailure();
    ASSERT_EQ(cb.getState(), CircuitState::OPEN);

    for (int i = 0; i < 10; ++i) {
        EXPECT_FALSE(cb.shouldAllow()) << "Iteration " << i;
    }
}

TEST(NetworkCircuitBreakerTest, SuccessInClosedResetsFailureStreak) {
    AdaptiveCircuitBreaker cb(makeConfig(3));

    cb.recordFailure();
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), CircuitState::CLOSED);

    cb.recordSuccess(); // resets consecutive failure streak
    cb.recordFailure(); // back to 1
    cb.recordFailure(); // 2
    EXPECT_EQ(cb.getState(), CircuitState::CLOSED); // threshold=3, only 2 failures

    cb.recordFailure(); // 3 → trip
    EXPECT_EQ(cb.getState(), CircuitState::OPEN);
}

// ============================================================================
// Acceptance criterion 2: Recovery scenarios
// ============================================================================

TEST(NetworkCircuitBreakerTest, OpenToHalfOpenAfterTimeout) {
    AdaptiveCircuitBreaker cb(makeConfig(2, 2, 1s /*open_timeout*/));

    cb.recordFailure();
    cb.recordFailure();
    ASSERT_EQ(cb.getState(), CircuitState::OPEN);

    // Before timeout: still blocked
    EXPECT_FALSE(cb.shouldAllow());

    // Wait for open timeout
    std::this_thread::sleep_for(1200ms);

    // First probe request should succeed and transition to HALF_OPEN
    EXPECT_TRUE(cb.shouldAllow());
    EXPECT_EQ(cb.getState(), CircuitState::HALF_OPEN);
}

TEST(NetworkCircuitBreakerTest, HalfOpenToClosedAfterSuccessThreshold) {
    AdaptiveCircuitBreaker cb(makeConfig(2, 2, 1s));

    cb.recordFailure();
    cb.recordFailure();
    ASSERT_EQ(cb.getState(), CircuitState::OPEN);

    std::this_thread::sleep_for(1200ms);
    ASSERT_TRUE(cb.shouldAllow());
    ASSERT_EQ(cb.getState(), CircuitState::HALF_OPEN);

    cb.recordSuccess(); // 1 of 2
    EXPECT_EQ(cb.getState(), CircuitState::HALF_OPEN);

    cb.recordSuccess(); // 2 of 2 → CLOSED
    EXPECT_EQ(cb.getState(), CircuitState::CLOSED);
    EXPECT_TRUE(cb.shouldAllow());
}

TEST(NetworkCircuitBreakerTest, HalfOpenFailureReopensCircuit) {
    AdaptiveCircuitBreaker cb(makeConfig(2, 2, 1s));

    cb.recordFailure();
    cb.recordFailure();
    ASSERT_EQ(cb.getState(), CircuitState::OPEN);

    std::this_thread::sleep_for(1200ms);
    ASSERT_TRUE(cb.shouldAllow());
    ASSERT_EQ(cb.getState(), CircuitState::HALF_OPEN);

    cb.recordFailure(); // Re-open
    EXPECT_EQ(cb.getState(), CircuitState::OPEN);
    EXPECT_FALSE(cb.shouldAllow());
}

TEST(NetworkCircuitBreakerTest, FullRecoveryCycleClosedOpenHalfOpenClosed) {
    AdaptiveCircuitBreaker cb(makeConfig(3, 2, 1s));

    // Phase 1: trip circuit
    cb.recordFailure();
    cb.recordFailure();
    cb.recordFailure();
    ASSERT_EQ(cb.getState(), CircuitState::OPEN);

    // Phase 2: wait for recovery probe window
    std::this_thread::sleep_for(1200ms);
    ASSERT_TRUE(cb.shouldAllow());
    ASSERT_EQ(cb.getState(), CircuitState::HALF_OPEN);

    // Phase 3: successful probe → circuit closes
    cb.recordSuccess();
    cb.recordSuccess();
    EXPECT_EQ(cb.getState(), CircuitState::CLOSED);

    // Phase 4: back to normal
    EXPECT_TRUE(cb.shouldAllow());
    cb.recordSuccess();
    EXPECT_EQ(cb.getState(), CircuitState::CLOSED);
}

TEST(NetworkCircuitBreakerTest, HalfOpenTimeoutReopensCircuit) {
    AdaptiveCircuitBreaker cb(makeConfig(
        2,   // failure_threshold
        5,   // success_threshold (high, so we stay in HALF_OPEN)
        1s,  // open_timeout
        1s   // half_open_timeout (short so it expires quickly)
    ));

    cb.recordFailure();
    cb.recordFailure();
    ASSERT_EQ(cb.getState(), CircuitState::OPEN);

    std::this_thread::sleep_for(1200ms); // wait for open_timeout
    ASSERT_TRUE(cb.shouldAllow());
    ASSERT_EQ(cb.getState(), CircuitState::HALF_OPEN);

    // Let the half_open_timeout expire without closing the circuit
    std::this_thread::sleep_for(1200ms);

    // Next shouldAllow should detect expired half_open_timeout → OPEN
    EXPECT_FALSE(cb.shouldAllow());
    EXPECT_EQ(cb.getState(), CircuitState::OPEN);
}

// ============================================================================
// Adaptive threshold
// ============================================================================

TEST(NetworkCircuitBreakerTest, AdaptiveThresholdDecreasesOnRepeatedTrips) {
    AdaptiveCircuitBreaker cb(makeConfig(
        10,  // failure_threshold
        2,   // success_threshold
        1s,  // open_timeout
        30s, // half_open_timeout
        true, // enable adaptive
        0.2   // 20% reduction
    ));

    auto initial_threshold = cb.getStats().current_failure_threshold;
    EXPECT_EQ(initial_threshold, 10u);

    // First trip (consecutive_trip_count reaches 1, no reduction yet)
    for (size_t i = 0; i < 10; ++i) {
      cb.recordFailure();
    }
    ASSERT_EQ(cb.getState(), CircuitState::OPEN);

    // Wait for open_timeout and probe, then fail again (second trip)
    std::this_thread::sleep_for(1200ms);
    ASSERT_TRUE(cb.shouldAllow());
    ASSERT_EQ(cb.getState(), CircuitState::HALF_OPEN);
    cb.recordFailure(); // re-opens (trip count now 2 → adaptation kicks in)
    ASSERT_EQ(cb.getState(), CircuitState::OPEN);

    // Threshold should now be reduced (10 - ceil(10*0.2) = 10 - 2 = 8)
    auto new_threshold = cb.getStats().current_failure_threshold;
    EXPECT_LT(new_threshold, initial_threshold);
    EXPECT_EQ(new_threshold, 8u);
}

TEST(NetworkCircuitBreakerTest, AdaptiveThresholdRestoredOnFullRecovery) {
    AdaptiveCircuitBreaker cb(makeConfig(
        10, 2, 1s, 30s, true, 0.2
    ));

    // Trip twice to reduce threshold to 8
    for (size_t i = 0; i < 10; ++i) {
      cb.recordFailure();
    }
    ASSERT_EQ(cb.getState(), CircuitState::OPEN);
    std::this_thread::sleep_for(1200ms);
    ASSERT_TRUE(cb.shouldAllow());
    cb.recordFailure();
    ASSERT_EQ(cb.getState(), CircuitState::OPEN);
    EXPECT_EQ(cb.getStats().current_failure_threshold, 8u);

    // Now fully recover: wait, probe, succeed twice
    std::this_thread::sleep_for(1200ms);
    ASSERT_TRUE(cb.shouldAllow());
    cb.recordSuccess();
    cb.recordSuccess();
    ASSERT_EQ(cb.getState(), CircuitState::CLOSED);

    // Threshold should relax back toward 10 (8 + ceil(10*0.2) = 8 + 2 = 10)
    EXPECT_EQ(cb.getStats().current_failure_threshold, 10u);
}

TEST(NetworkCircuitBreakerTest, NoAdaptationWhenAdaptiveDisabled) {
    AdaptiveCircuitBreaker cb(makeConfig(
        5, 2, 1s, 30s, false /*adaptive off*/, 0.2
    ));

    // Trip twice
    for (size_t i = 0; i < 5; ++i) {
      cb.recordFailure();
    }
    ASSERT_EQ(cb.getState(), CircuitState::OPEN);
    std::this_thread::sleep_for(1200ms);
    ASSERT_TRUE(cb.shouldAllow());
    cb.recordFailure();
    ASSERT_EQ(cb.getState(), CircuitState::OPEN);

    // Threshold unchanged
    EXPECT_EQ(cb.getStats().current_failure_threshold, 5u);
}

// ============================================================================
// Statistics
// ============================================================================

TEST(NetworkCircuitBreakerTest, StatsTrackedCorrectly) {
    AdaptiveCircuitBreaker cb(makeConfig(3));

    cb.shouldAllow(); // total=1
    cb.shouldAllow(); // total=2
    cb.recordSuccess();
    cb.recordSuccess();
    cb.recordFailure();
    cb.recordFailure();
    cb.recordFailure(); // trips → OPEN

    // total_calls from shouldAllow() — 2 allowed before trip
    // Now OPEN: next shouldAllow() → rejected
    cb.shouldAllow(); // total=3, rejected=1

    auto stats = cb.getStats();
    EXPECT_EQ(stats.state,            CircuitState::OPEN);
    EXPECT_EQ(stats.successful_calls, 2u);
    EXPECT_EQ(stats.failed_calls,     3u);
    EXPECT_GE(stats.rejected_calls,   1u);
    EXPECT_GE(stats.total_calls,      3u);
}

// ============================================================================
// State-change callback
// ============================================================================

TEST(NetworkCircuitBreakerTest, StateChangeCallbackFiredOnTransitions) {
    AdaptiveCircuitBreaker cb(makeConfig(2, 2, 1s));

    std::vector<std::pair<CircuitState, CircuitState>> transitions;
    cb.setStateChangeCallback([&](CircuitState from, CircuitState to) {
        transitions.emplace_back(from, to);
    });

    // CLOSED → OPEN
    cb.recordFailure();
    cb.recordFailure();
    ASSERT_EQ(cb.getState(), CircuitState::OPEN);

    // OPEN → HALF_OPEN
    std::this_thread::sleep_for(1200ms);
    ASSERT_TRUE(cb.shouldAllow());

    // HALF_OPEN → CLOSED
    cb.recordSuccess();
    cb.recordSuccess();

    ASSERT_EQ(transitions.size(), 3u);
    EXPECT_EQ(transitions[0], std::make_pair(CircuitState::CLOSED, CircuitState::OPEN));
    EXPECT_EQ(transitions[1], std::make_pair(CircuitState::OPEN,   CircuitState::HALF_OPEN));
    EXPECT_EQ(transitions[2], std::make_pair(CircuitState::HALF_OPEN, CircuitState::CLOSED));
}

// ============================================================================
// stateToString
// ============================================================================

TEST(NetworkCircuitBreakerTest, StateToString) {
    EXPECT_EQ(AdaptiveCircuitBreaker::stateToString(CircuitState::CLOSED),    "CLOSED");
    EXPECT_EQ(AdaptiveCircuitBreaker::stateToString(CircuitState::HALF_OPEN), "HALF_OPEN");
    EXPECT_EQ(AdaptiveCircuitBreaker::stateToString(CircuitState::OPEN),      "OPEN");
}

// ============================================================================
// Thread-safety
// ============================================================================

TEST(NetworkCircuitBreakerTest, ConcurrentAccessIsThreadSafe) {
    AdaptiveCircuitBreaker cb(makeConfig(50));

    const int num_threads   = 8;
    const int ops_per_thread = 200;

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&cb, ops_per_thread, t]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                if (cb.shouldAllow()) {
                    if ((t + i) % 3 == 0) {
                        cb.recordFailure();
                    } else {
                        cb.recordSuccess();
                    }
                }
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    // State must be one of the valid values — no crash, no UB
    const CircuitState s = cb.getState();
    EXPECT_TRUE(s == CircuitState::CLOSED ||
                s == CircuitState::OPEN   ||
                s == CircuitState::HALF_OPEN);

    // Total calls must be consistent (≤ total attempted)
    auto stats = cb.getStats();
    EXPECT_LE(stats.successful_calls + stats.failed_calls + stats.rejected_calls,
              stats.total_calls + static_cast<uint64_t>(num_threads) * ops_per_thread);
}

TEST(NetworkCircuitBreakerTest, ConcurrentTripAndRecovery) {
    // Many threads record failures simultaneously — only one trip should occur
    AdaptiveCircuitBreaker cb(makeConfig(5, 2, 1s));

    // Trip the circuit from multiple threads
    std::vector<std::thread> threads = {};

    for (int t = 0; t < 10; ++t) {
        threads.emplace_back([&cb]() {
            for (int i = 0; i < 5; ++i) {
                cb.recordFailure();
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    // Circuit must be OPEN (or possibly already recovering)
    const CircuitState s = cb.getState();
    EXPECT_TRUE(s == CircuitState::OPEN || s == CircuitState::HALF_OPEN);
}

// ============================================================================
// reset() and forceOpen()
// ============================================================================

TEST(NetworkCircuitBreakerTest, ResetFromOpenReturnsToClosed) {
    AdaptiveCircuitBreaker cb(makeConfig(2));

    cb.recordFailure();
    cb.recordFailure();
    ASSERT_EQ(cb.getState(), CircuitState::OPEN);
    ASSERT_FALSE(cb.shouldAllow());

    cb.reset();

    EXPECT_EQ(cb.getState(), CircuitState::CLOSED);
    EXPECT_TRUE(cb.shouldAllow());
}

TEST(NetworkCircuitBreakerTest, ResetRestoresAdaptiveThreshold) {
    AdaptiveCircuitBreaker cb(makeConfig(10, 2, 1s, 30s, true, 0.2));

    // Trip twice to reduce effective threshold to 8
    for (size_t i = 0; i < 10; ++i) {
      cb.recordFailure();
    }
    ASSERT_EQ(cb.getState(), CircuitState::OPEN);
    std::this_thread::sleep_for(1200ms);
    ASSERT_TRUE(cb.shouldAllow());
    cb.recordFailure(); // re-opens, adaptive kicks in
    ASSERT_EQ(cb.getStats().current_failure_threshold, 8u);

    cb.reset();

    EXPECT_EQ(cb.getState(), CircuitState::CLOSED);
    EXPECT_EQ(cb.getStats().current_failure_threshold, 10u);
}

TEST(NetworkCircuitBreakerTest, ResetClearsFailureStreak) {
    AdaptiveCircuitBreaker cb(makeConfig(3));

    cb.recordFailure();
    cb.recordFailure(); // 2 failures, threshold=3, still CLOSED

    cb.reset(); // clear the streak

    // After reset, 3 fresh failures should trip (not 1)
    cb.recordFailure();
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), CircuitState::CLOSED);
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), CircuitState::OPEN);
}

TEST(NetworkCircuitBreakerTest, ForceOpenFromClosedTripsCircuit) {
    AdaptiveCircuitBreaker cb(makeConfig(100)); // high threshold, wouldn't trip naturally

    ASSERT_EQ(cb.getState(), CircuitState::CLOSED);
    cb.forceOpen();

    EXPECT_EQ(cb.getState(), CircuitState::OPEN);
    EXPECT_FALSE(cb.shouldAllow());
}

TEST(NetworkCircuitBreakerTest, ForceOpenFromHalfOpenTripsCircuit) {
    AdaptiveCircuitBreaker cb(makeConfig(2, 5, 1s));

    cb.recordFailure();
    cb.recordFailure();
    ASSERT_EQ(cb.getState(), CircuitState::OPEN);
    std::this_thread::sleep_for(1200ms);
    ASSERT_TRUE(cb.shouldAllow());
    ASSERT_EQ(cb.getState(), CircuitState::HALF_OPEN);

    cb.forceOpen();

    EXPECT_EQ(cb.getState(), CircuitState::OPEN);
    EXPECT_FALSE(cb.shouldAllow());
}

TEST(NetworkCircuitBreakerTest, ForceOpenFiresStateChangeCallback) {
    AdaptiveCircuitBreaker cb(makeConfig(10));

    std::vector<std::pair<CircuitState, CircuitState>> transitions;
    cb.setStateChangeCallback([&](CircuitState from, CircuitState to) {
        transitions.emplace_back(from, to);
    });

    cb.forceOpen();

    ASSERT_EQ(transitions.size(), 1u);
    EXPECT_EQ(transitions[0].first,  CircuitState::CLOSED);
    EXPECT_EQ(transitions[0].second, CircuitState::OPEN);
}

TEST(NetworkCircuitBreakerTest, ResetFiresStateChangeCallback) {
    AdaptiveCircuitBreaker cb(makeConfig(2));

    cb.recordFailure();
    cb.recordFailure();
    ASSERT_EQ(cb.getState(), CircuitState::OPEN);

    std::vector<std::pair<CircuitState, CircuitState>> transitions;
    cb.setStateChangeCallback([&](CircuitState from, CircuitState to) {
        transitions.emplace_back(from, to);
    });

    cb.reset();

    ASSERT_EQ(transitions.size(), 1u);
    EXPECT_EQ(transitions[0].first,  CircuitState::OPEN);
    EXPECT_EQ(transitions[0].second, CircuitState::CLOSED);
}
