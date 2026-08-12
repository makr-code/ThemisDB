/**
 * @file test_aql_circuit_breaker_policy.cpp
 * @brief Phase 5 Unit Tests — Circuit Breaker Policy Behavior
 *
 * Tests circuit breaker state machine transitions and policies:
 * - Failure threshold triggers OPEN state
 * - Success in HALF_OPEN transitions to CLOSED
 * - Timeout in HALF_OPEN transitions back to OPEN
 * - Per-operation-type isolation
 * - HALF_OPEN allows only limited requests
 * - Explicit reset behavior
 *
 * Uses self-contained mock circuit breaker — no real infrastructure required.
 */


#include <gtest/gtest.h>
#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>

#include "aql/aql_error_types.h"

namespace themis {
namespace aql {
namespace testing {

// ============================================================================
// Mock Circuit Breaker
// ============================================================================

class MockCircuitBreaker {
public:
    enum class State { CLOSED, HALF_OPEN, OPEN };

    struct Config {
        int failure_threshold      = 3;    ///< Failures to trip OPEN
        int half_open_max_requests = 2;    ///< Permitted calls in HALF_OPEN before decision
    };

    explicit MockCircuitBreaker(const Config& cfg = Config{})
        : cfg_(cfg), state_(State::CLOSED), failures_(0), half_open_calls_(0) {}

    /// @return true if the request is allowed
    bool allowRequest() {
        std::lock_guard<std::mutex> lk(mu_);
        switch (state_) {
            case State::CLOSED:
                return true;
            case State::OPEN:
                return false;
            case State::HALF_OPEN:
                if (half_open_calls_ < cfg_.half_open_max_requests) {
                    ++half_open_calls_;
                    return true;
                }
                return false;
        }
        return false;
    }

    /// Record a successful request
    void onSuccess() {
        std::lock_guard<std::mutex> lk(mu_);
        if (state_ == State::HALF_OPEN) {
            // Probe succeeded: close the breaker
            state_    = State::CLOSED;
            failures_ = 0;
            half_open_calls_ = 0;
        }
        // CLOSED: success resets failure count
        if (state_ == State::CLOSED) {
            failures_ = 0;
        }
    }

    /// Record a failed request
    void onFailure() {
        std::lock_guard<std::mutex> lk(mu_);
        ++failures_;
        if (state_ == State::HALF_OPEN) {
            // Probe failed: go back to OPEN
            state_ = State::OPEN;
            half_open_calls_ = 0;
        } else if (state_ == State::CLOSED && failures_ >= cfg_.failure_threshold) {
            state_ = State::OPEN;
        }
    }

    /// Manually transition OPEN → HALF_OPEN (simulates timeout expiry)
    void transitionToHalfOpen() {
        std::lock_guard<std::mutex> lk(mu_);
        if (state_ == State::OPEN) {
            state_ = State::HALF_OPEN;
            half_open_calls_ = 0;
        }
    }

    /// Force reset to CLOSED state
    void forceReset() {
        std::lock_guard<std::mutex> lk(mu_);
        state_    = State::CLOSED;
        failures_ = 0;
        half_open_calls_ = 0;
    }

    State  getState()    const { std::lock_guard<std::mutex> lk(mu_); return state_;    }
    int    failures()    const { std::lock_guard<std::mutex> lk(mu_); return failures_; }
    Config getConfig()   const { return cfg_; }

    static const char* stateName(State s) {
        switch (s) {
            case State::CLOSED:    return "CLOSED";
            case State::HALF_OPEN: return "HALF_OPEN";
            case State::OPEN:      return "OPEN";
        }
        return "UNKNOWN";
    }

private:
    mutable std::mutex mu_;
    Config cfg_;
    State  state_;
    int    failures_;
    int    half_open_calls_;
};

/// @brief Registry of per-operation-type circuit breakers
class MockCircuitBreakerRegistry {
public:
    void registerBreaker(const std::string& op, const MockCircuitBreaker::Config& cfg) {
        breakers_.emplace(op, cfg);
    }

    MockCircuitBreaker& get(const std::string& op) {
        return breakers_.at(op);
    }

    bool hasBreaker(const std::string& op) const {
        return breakers_.count(op) > 0;
    }

private:
    std::unordered_map<std::string, MockCircuitBreaker> breakers_;
};

// ============================================================================
// Test Cases
// ============================================================================

/**
 * @test CircuitBreakerPolicy_FailureThresholdTriggersOpen
 *
 * Verify that failure_threshold consecutive failures transition
 * the circuit breaker from CLOSED to OPEN.
 */
TEST(CircuitBreakerPolicy, FailureThresholdTriggersOpen) {
    MockCircuitBreaker cb({.failure_threshold = 3, .half_open_max_requests = 1});

    EXPECT_EQ(cb.getState(), MockCircuitBreaker::State::CLOSED);

    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(cb.allowRequest());
        cb.onFailure();
    }

    EXPECT_EQ(cb.getState(), MockCircuitBreaker::State::OPEN);
    EXPECT_FALSE(cb.allowRequest());

    // Verify that an AQLErrorContext correctly represents this
    AQLErrorContext ctx(
        "provider",
        ProviderError::CircuitBreakerOpen,
        "infer_circuit_breaker",
        "[TRANSLATION:ProviderUnavailable] Circuit breaker is OPEN after 3 failures"
    );
    ctx.setRetryCount(3);
    ctx.setRecoverable(false);

    EXPECT_EQ(ctx.getCategory(), ProviderError::CircuitBreakerOpen);
    EXPECT_FALSE(ctx.isRecoverable());
}

/**
 * @test CircuitBreakerPolicy_SuccessTransitionsFromHalfOpenToClosed
 *
 * Verify that a successful request in HALF_OPEN state transitions
 * the circuit breaker back to CLOSED.
 */
TEST(CircuitBreakerPolicy, SuccessTransitionsFromHalfOpenToClosed) {
    MockCircuitBreaker cb({.failure_threshold = 2, .half_open_max_requests = 1});

    // Trip the breaker
    cb.allowRequest(); cb.onFailure();
    cb.allowRequest(); cb.onFailure();
    EXPECT_EQ(cb.getState(), MockCircuitBreaker::State::OPEN);

    // Simulate timeout expiry: transition to HALF_OPEN
    cb.transitionToHalfOpen();
    EXPECT_EQ(cb.getState(), MockCircuitBreaker::State::HALF_OPEN);

    // Probe succeeds → CLOSED
    EXPECT_TRUE(cb.allowRequest());
    cb.onSuccess();
    EXPECT_EQ(cb.getState(), MockCircuitBreaker::State::CLOSED);

    // Now normal requests are allowed again
    EXPECT_TRUE(cb.allowRequest());
}

/**
 * @test CircuitBreakerPolicy_TimeoutTransitionsFromHalfOpenToOpen
 *
 * Verify that a failing probe in HALF_OPEN state transitions the
 * circuit breaker back to OPEN.
 */
TEST(CircuitBreakerPolicy, TimeoutTransitionsFromHalfOpenToOpen) {
    MockCircuitBreaker cb({.failure_threshold = 2, .half_open_max_requests = 1});

    // Trip then transition to HALF_OPEN
    cb.allowRequest(); cb.onFailure();
    cb.allowRequest(); cb.onFailure();
    cb.transitionToHalfOpen();
    EXPECT_EQ(cb.getState(), MockCircuitBreaker::State::HALF_OPEN);

    // Probe fails → back to OPEN
    EXPECT_TRUE(cb.allowRequest());
    cb.onFailure();
    EXPECT_EQ(cb.getState(), MockCircuitBreaker::State::OPEN);
    EXPECT_FALSE(cb.allowRequest());

    // Build AQLErrorContext for this scenario
    AQLErrorContext ctx(
        "provider",
        ProviderError::CircuitBreakerOpen,
        "infer_cb",
        "[TRANSLATION:ProviderUnavailable] Probe failed in HALF_OPEN; returning to OPEN"
    );
    ctx.addDiagnosticHint("Wait for next cooldown interval before retrying");
    ctx.setRecoverable(false);

    EXPECT_TRUE(ctx.formatForLogging().find("CircuitBreakerOpen") != std::string::npos);
}

/**
 * @test CircuitBreakerPolicy_PerOperationTypeIsolation
 *
 * Verify that circuit breakers for different operation types are
 * independent — failure in one does not affect others.
 */
TEST(CircuitBreakerPolicy, PerOperationTypeIsolation) {
    MockCircuitBreakerRegistry registry;
    registry.registerBreaker("infer",  {.failure_threshold = 2});
    registry.registerBreaker("rag",    {.failure_threshold = 2});
    registry.registerBreaker("embed",  {.failure_threshold = 2});

    // Trip the 'infer' breaker
    auto& infer = registry.get("infer");
    infer.allowRequest(); infer.onFailure();
    infer.allowRequest(); infer.onFailure();
    EXPECT_EQ(infer.getState(), MockCircuitBreaker::State::OPEN);

    // 'rag' and 'embed' should still be CLOSED and allowing requests
    EXPECT_EQ(registry.get("rag").getState(),   MockCircuitBreaker::State::CLOSED);
    EXPECT_EQ(registry.get("embed").getState(), MockCircuitBreaker::State::CLOSED);
    EXPECT_TRUE(registry.get("rag").allowRequest());
    EXPECT_TRUE(registry.get("embed").allowRequest());
}

/**
 * @test CircuitBreakerPolicy_HalfOpenAllowsLimitedRequests
 *
 * Verify that in HALF_OPEN state, only half_open_max_requests
 * calls are permitted before the decision is made.
 */
TEST(CircuitBreakerPolicy, HalfOpenAllowsLimitedRequests) {
    MockCircuitBreaker cb({.failure_threshold = 2, .half_open_max_requests = 2});

    // Trip breaker
    cb.allowRequest(); cb.onFailure();
    cb.allowRequest(); cb.onFailure();
    cb.transitionToHalfOpen();

    // First 2 requests in HALF_OPEN are allowed
    EXPECT_TRUE(cb.allowRequest());   // probe call 1
    EXPECT_TRUE(cb.allowRequest());   // probe call 2

    // Third request in HALF_OPEN is blocked
    EXPECT_FALSE(cb.allowRequest());

    // Resolve the half-open state with success
    cb.onSuccess();
    EXPECT_EQ(cb.getState(), MockCircuitBreaker::State::CLOSED);
    EXPECT_TRUE(cb.allowRequest());
}

/**
 * @test CircuitBreakerPolicy_ExplicitResetBehavior
 *
 * Verify that forceReset() transitions any state back to CLOSED
 * and clears failure counters, allowing immediate resumption of traffic.
 */
TEST(CircuitBreakerPolicy, ExplicitResetBehavior) {
    MockCircuitBreaker cb({.failure_threshold = 2, .half_open_max_requests = 1});

    // Trip the breaker
    cb.allowRequest(); cb.onFailure();
    cb.allowRequest(); cb.onFailure();
    EXPECT_EQ(cb.getState(), MockCircuitBreaker::State::OPEN);
    EXPECT_EQ(cb.failures(), 2);

    // Force reset
    cb.forceReset();
    EXPECT_EQ(cb.getState(), MockCircuitBreaker::State::CLOSED);
    EXPECT_EQ(cb.failures(), 0);
    EXPECT_TRUE(cb.allowRequest());

    // Accumulate fewer failures than threshold — should remain CLOSED
    cb.allowRequest(); cb.onFailure();   // 1 failure
    EXPECT_EQ(cb.getState(), MockCircuitBreaker::State::CLOSED);

    // Force reset again from CLOSED is idempotent
    cb.forceReset();
    EXPECT_EQ(cb.getState(), MockCircuitBreaker::State::CLOSED);
    EXPECT_EQ(cb.failures(), 0);

    // AQLErrorContext for explicit reset event
    AQLErrorContext ctx(
        "provider",
        ProviderError::CircuitBreakerOpen,
        "admin_reset",
        "Circuit breaker reset by operator"
    );
    ctx.addDiagnosticHint("Manual reset applied; traffic resumed immediately");
    ctx.setRecoverable(true);

    EXPECT_TRUE(ctx.isRecoverable());
    EXPECT_TRUE(ctx.formatForLogging().find("reset") != std::string::npos);
}

}  // namespace testing
}  // namespace aql
}  // namespace themis
