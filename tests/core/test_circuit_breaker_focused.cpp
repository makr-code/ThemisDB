#include <gtest/gtest.h>
#include "core/concerns/i_circuit_breaker.h"
#include "core/concerns/noop_implementations.h"

#include <atomic>
#include <string>

using namespace themis::core::concerns;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Build a DefaultCircuitBreaker with a low failure threshold and zero timeout
/// so OPEN→HALF_OPEN transitions happen immediately in tests.
static DefaultCircuitBreaker makeBreaker(size_t threshold = 3,
                                         std::chrono::seconds timeout = std::chrono::seconds(0),
                                         size_t success_threshold = 1) {
    ICircuitBreaker::Config cfg;
    cfg.failure_threshold = threshold;
    cfg.timeout           = timeout;
    cfg.success_threshold = success_threshold;
    return DefaultCircuitBreaker(cfg);
}

/// Trip the circuit by calling recordFailure() @p n times.
static void tripBreaker(ICircuitBreaker& cb, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        cb.recordFailure();
    }
}

// ---------------------------------------------------------------------------
// CB_01 — closed state: call() executes fn, not fallback
// ---------------------------------------------------------------------------
TEST(CircuitBreakerTest, CB_01_ClosedStateExecutesFn) {
    auto cb = makeBreaker();

    bool fn_called       = false;
    bool fallback_called = false;

    cb.call(
        [&] { fn_called       = true; return 1; },
        [&] { fallback_called = true; return -1; }
    );

    EXPECT_TRUE(fn_called);
    EXPECT_FALSE(fallback_called);
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::CLOSED);
}

// ---------------------------------------------------------------------------
// CB_02 — after N failures, circuit opens and call() uses fallback
// ---------------------------------------------------------------------------
TEST(CircuitBreakerTest, CB_02_OpenAfterThreshold_CallUsesFallback) {
    auto cb = makeBreaker(/*threshold=*/3);
    tripBreaker(cb, 3); // trips the circuit

    bool fn_called       = false;
    bool fallback_called = false;

    cb.call(
        [&] { fn_called       = true; return 0; },
        [&] { fallback_called = true; return -1; }
    );

    EXPECT_FALSE(fn_called);
    EXPECT_TRUE(fallback_called);
}

// ---------------------------------------------------------------------------
// CB_03 — getState() returns OPEN after threshold failures
// ---------------------------------------------------------------------------
TEST(CircuitBreakerTest, CB_03_GetStateOpenAfterThreshold) {
    auto cb = makeBreaker(/*threshold=*/2);
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::CLOSED);

    tripBreaker(cb, 2);
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::OPEN);
}

// ---------------------------------------------------------------------------
// CB_04 — call() return type matches fn return type (int case)
// ---------------------------------------------------------------------------
TEST(CircuitBreakerTest, CB_04_CallReturnTypeMatchesFn) {
    auto cb = makeBreaker();

    int result = cb.call(
        []() -> int { return 42; },
        []() -> int { return -1; }
    );

    EXPECT_EQ(result, 42);
}

// ---------------------------------------------------------------------------
// CB_05 — call() with void fn and void fallback
// ---------------------------------------------------------------------------
TEST(CircuitBreakerTest, CB_05_VoidFnAndFallback) {
    auto cb = makeBreaker();

    std::atomic<int> fn_count{0};
    std::atomic<int> fallback_count{0};

    // Closed — fn should run
    cb.call(
        [&] { ++fn_count; },
        [&] { ++fallback_count; }
    );
    EXPECT_EQ(fn_count.load(), 1);
    EXPECT_EQ(fallback_count.load(), 0);

    // Trip and verify fallback runs
    tripBreaker(cb, 3);
    cb.call(
        [&] { ++fn_count; },
        [&] { ++fallback_count; }
    );
    EXPECT_EQ(fn_count.load(), 1);   // fn not called again
    EXPECT_EQ(fallback_count.load(), 1);
}

// ---------------------------------------------------------------------------
// CB_06 — reset() closes the circuit
// ---------------------------------------------------------------------------
TEST(CircuitBreakerTest, CB_06_ResetClosesCircuit) {
    auto cb = makeBreaker(/*threshold=*/1);
    tripBreaker(cb, 1);
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::OPEN);

    cb.reset();
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::CLOSED);
    EXPECT_EQ(cb.getFailureCount(), 0u);

    // Verify fn runs after reset
    bool fn_called = false;
    cb.call([&] { fn_called = true; return 0; }, [] { return -1; });
    EXPECT_TRUE(fn_called);
}

// ---------------------------------------------------------------------------
// CB_07 — HALF_OPEN: after timeout elapses, one probe call is allowed
// ---------------------------------------------------------------------------
TEST(CircuitBreakerTest, CB_07_HalfOpenAllowsProbeCall) {
    // timeout = 0s means the circuit transitions to HALF_OPEN immediately
    // when allowRequest() is called after being OPEN.
    auto cb = makeBreaker(/*threshold=*/1, /*timeout=*/std::chrono::seconds(0));

    // Trip to OPEN
    tripBreaker(cb, 1);
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::OPEN);

    // With timeout = 0, the first allowRequest() call should transition to HALF_OPEN
    // and return true (probe allowed).
    bool fn_called = false;
    cb.call(
        [&] { fn_called = true; return 0; },
        [] { return -1; }
    );
    // The probe fn was called (half-open probe)
    EXPECT_TRUE(fn_called);
    // After a successful probe, the circuit should close (success_threshold = 1)
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::CLOSED);
}

// ---------------------------------------------------------------------------
// CB_08 — circuit breaker state transitions are observable via getState()
//         (serves as the "metrics side effect" observability proxy)
// ---------------------------------------------------------------------------
TEST(CircuitBreakerTest, CB_08_StateObservableViaGetState) {
    auto cb = makeBreaker(/*threshold=*/2, /*timeout=*/std::chrono::seconds(0),
                          /*success_threshold=*/2);

    // Initial state
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::CLOSED);

    // Record one failure — should still be closed
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::CLOSED);

    // Record second failure — trips to OPEN
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::OPEN);

    // allowRequest() with 0s timeout triggers OPEN → HALF_OPEN transition
    bool allowed = cb.allowRequest();
    EXPECT_TRUE(allowed);
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::HALF_OPEN);

    // One success — not yet enough to close (success_threshold=2)
    cb.recordSuccess();
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::HALF_OPEN);

    // Second success — closes circuit
    cb.recordSuccess();
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::CLOSED);

    // NoOpCircuitBreaker is always CLOSED and always allows
    NoOpCircuitBreaker noop;
    EXPECT_EQ(noop.getState(), ICircuitBreaker::State::CLOSED);
    EXPECT_TRUE(noop.allowRequest());
    noop.recordFailure();
    EXPECT_EQ(noop.getState(), ICircuitBreaker::State::CLOSED);
}
