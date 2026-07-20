/**
 * @file test_wire_protocol_retry.cpp
 * @brief Phase 5 P5-S01: Wire Protocol Retry/Backoff Hardening Tests
 *
 * Validates the WireRetryPolicy + RetryContext + retryWithPolicy deliverables:
 *
 *  WPR-01: Policy default construction and named factory methods.
 *  WPR-02: RetryContext canRetry / attempts tracking.
 *  WPR-03: RetryContext nextDelay returns std::nullopt when exhausted.
 *  WPR-04: Permanent errors return std::nullopt immediately.
 *  WPR-05: Exponential delay growth (no jitter).
 *  WPR-06: Delay is capped by max_delay_ms.
 *  WPR-07: retryWithPolicy succeeds on first attempt (zero sleep path).
 *  WPR-08: retryWithPolicy returns false when all attempts fail.
 *  WPR-09: retryWithPolicy invokes on_fail callback with correct arguments.
 *  WPR-10: retryWithPolicy succeeds on 2nd attempt (one transient failure).
 *  WPR-11: Jitter-enabled delays are within [0, computed_delay].
 *  WPR-12: classifyBoostError — EAGAIN → kTransient.
 *  WPR-13: classifyBoostError — EINVAL → kPermanent.
 *  WPR-14: classifyBoostError — success (no error) → kPermanent sentinel.
 *  WPR-15: reset() restarts context without reallocating policy.
 *  WPR-16: forConnectionIO policy has jitter enabled and 5 max_attempts.
 *
 * @see include/network/wire_retry_policy.h
 * @see src/network/wire_retry_policy.cpp
 */

#include <gtest/gtest.h>

#include "network/wire_retry_policy.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>
#include <thread>

using namespace themis::network;

// ==========================================================================
// WPR-01: Named factory methods return expected defaults
// ==========================================================================

TEST(WireProtocolRetry, WPR01_FactoryDefaults) {
    const auto bind_policy = WireRetryPolicy::forBindListen();
    EXPECT_EQ(bind_policy.max_attempts,  3u);
    EXPECT_EQ(bind_policy.base_delay_ms, 100u);
    EXPECT_EQ(bind_policy.max_delay_ms,  2'000u);
    EXPECT_DOUBLE_EQ(bind_policy.multiplier, 2.0);
    EXPECT_FALSE(bind_policy.enable_jitter);

    const auto io_policy = WireRetryPolicy::forConnectionIO();
    EXPECT_EQ(io_policy.max_attempts,  5u);
    EXPECT_EQ(io_policy.base_delay_ms, 50u);
    EXPECT_TRUE(io_policy.enable_jitter);

    const auto test_policy = WireRetryPolicy::forTesting();
    EXPECT_EQ(test_policy.max_attempts,  3u);
    EXPECT_EQ(test_policy.base_delay_ms, 0u);
    EXPECT_EQ(test_policy.max_delay_ms,  0u);
    EXPECT_FALSE(test_policy.enable_jitter);
}

// ==========================================================================
// WPR-02: RetryContext tracks attempt count correctly
// ==========================================================================

TEST(WireProtocolRetry, WPR02_AttemptTracking) {
    const auto policy = WireRetryPolicy::forTesting(); // max_attempts=3, 0ms delay
    RetryContext ctx(policy);

    EXPECT_EQ(ctx.attempts(), 0u);
    EXPECT_TRUE(ctx.canRetry());

    ctx.nextDelay(); // attempt 1
    EXPECT_EQ(ctx.attempts(), 1u);

    ctx.nextDelay(); // attempt 2
    EXPECT_EQ(ctx.attempts(), 2u);

    ctx.nextDelay(); // attempt 3 (last)
    EXPECT_EQ(ctx.attempts(), 3u);

    EXPECT_FALSE(ctx.canRetry());
}

// ==========================================================================
// WPR-03: nextDelay returns nullopt when max_attempts exhausted
// ==========================================================================

TEST(WireProtocolRetry, WPR03_ExhaustedReturnsNullopt) {
    WireRetryPolicy p = WireRetryPolicy::forTesting();
    p.max_attempts = 2;
    RetryContext ctx(p);

    EXPECT_TRUE(ctx.nextDelay().has_value());  // attempt 1
    EXPECT_TRUE(ctx.nextDelay().has_value());  // attempt 2
    EXPECT_FALSE(ctx.nextDelay().has_value()); // exhausted
    EXPECT_FALSE(ctx.canRetry());
}

// ==========================================================================
// WPR-04: Permanent error class short-circuits retry immediately
// ==========================================================================

TEST(WireProtocolRetry, WPR04_PermanentErrorNullopt) {
    const auto policy = WireRetryPolicy::forTesting();
    RetryContext ctx(policy);

    EXPECT_FALSE(ctx.nextDelay(WireErrorClass::kPermanent).has_value());
    // attempt counter must not have been incremented for permanent failures
    EXPECT_EQ(ctx.attempts(), 0u);
}

// ==========================================================================
// WPR-05: Exponential delay growth (no jitter)
// ==========================================================================

TEST(WireProtocolRetry, WPR05_ExponentialDelayGrowth) {
    WireRetryPolicy p;
    p.max_attempts  = 4;
    p.base_delay_ms = 100;
    p.max_delay_ms  = 100'000;
    p.multiplier    = 2.0;
    p.enable_jitter = false;

    RetryContext ctx(p);

    // attempt 0 → base * 2^0 = 100 ms
    auto d0 = ctx.nextDelay();
    ASSERT_TRUE(d0.has_value());
    EXPECT_EQ(d0->count(), 100);

    // attempt 1 → base * 2^1 = 200 ms
    auto d1 = ctx.nextDelay();
    ASSERT_TRUE(d1.has_value());
    EXPECT_EQ(d1->count(), 200);

    // attempt 2 → base * 2^2 = 400 ms
    auto d2 = ctx.nextDelay();
    ASSERT_TRUE(d2.has_value());
    EXPECT_EQ(d2->count(), 400);
}

// ==========================================================================
// WPR-06: Delay is capped by max_delay_ms
// ==========================================================================

TEST(WireProtocolRetry, WPR06_DelayCappedAtMax) {
    WireRetryPolicy p;
    p.max_attempts  = 10;
    p.base_delay_ms = 1000;
    p.max_delay_ms  = 500; // lower than base — all delays must be capped
    p.multiplier    = 2.0;
    p.enable_jitter = false;

    RetryContext ctx(p);
    for (uint32_t i = 0; i < p.max_attempts; ++i) {
        auto d = ctx.nextDelay();
        ASSERT_TRUE(d.has_value()) << "attempt " << i;
        EXPECT_LE(d->count(), static_cast<int64_t>(p.max_delay_ms))
            << "delay exceeded cap at attempt " << i;
    }
}

// ==========================================================================
// WPR-07: retryWithPolicy succeeds immediately (single attempt)
// ==========================================================================

TEST(WireProtocolRetry, WPR07_SucceedsFirstAttempt) {
    const auto policy = WireRetryPolicy::forTesting();
    int call_count = 0;
    const bool ok = retryWithPolicy(policy, [&]() -> bool {
        ++call_count;
        return true; // succeed immediately
    });
    EXPECT_TRUE(ok);
    EXPECT_EQ(call_count, 1);
}

// ==========================================================================
// WPR-08: retryWithPolicy returns false when all attempts fail
// ==========================================================================

TEST(WireProtocolRetry, WPR08_AllAttemptsFail) {
    WireRetryPolicy p = WireRetryPolicy::forTesting();
    p.max_attempts = 3;

    int call_count = 0;
    const bool ok = retryWithPolicy(p, [&]() -> bool {
        ++call_count;
        return false;
    });

    EXPECT_FALSE(ok);
    // first attempt + max_attempts retries = max_attempts + 1 total calls
    EXPECT_EQ(call_count, static_cast<int>(p.max_attempts) + 1);
}

// ==========================================================================
// WPR-09: on_fail callback invoked with correct attempt/delay arguments
// ==========================================================================

TEST(WireProtocolRetry, WPR09_OnFailCallback) {
    WireRetryPolicy p = WireRetryPolicy::forTesting();
    p.max_attempts = 2;

    std::vector<std::pair<uint32_t, int64_t>> recorded;
    const bool ok = retryWithPolicy(
        p,
        []() -> bool { return false; },
        [&](uint32_t attempt, int64_t delay_ms) {
            recorded.emplace_back(attempt, delay_ms);
        });

    EXPECT_FALSE(ok);
    ASSERT_EQ(recorded.size(), 2u); // one per retry attempt
    EXPECT_EQ(recorded[0].first, 1u);
    EXPECT_EQ(recorded[1].first, 2u);
    // With base_delay_ms=0 and max_delay_ms=0, all delays should be 0
    for (const auto& [attempt, delay] : recorded) {
        EXPECT_EQ(delay, 0) << "Expected 0-ms delay for test policy at attempt " << attempt;
    }
}

// ==========================================================================
// WPR-10: retryWithPolicy succeeds on second attempt
// ==========================================================================

TEST(WireProtocolRetry, WPR10_SucceedsSecondAttempt) {
    WireRetryPolicy p = WireRetryPolicy::forTesting();
    p.max_attempts = 3;

    int call_count = 0;
    const bool ok = retryWithPolicy(p, [&]() -> bool {
        return ++call_count >= 2; // fail once, succeed on 2nd call
    });

    EXPECT_TRUE(ok);
    EXPECT_EQ(call_count, 2);
}

// ==========================================================================
// WPR-11: Jitter-enabled delays are within [0, computed_delay]
// ==========================================================================

TEST(WireProtocolRetry, WPR11_JitterInRange) {
    WireRetryPolicy p;
    p.max_attempts  = 100;
    p.base_delay_ms = 1000;
    p.max_delay_ms  = 5000;
    p.multiplier    = 1.0; // keep delay constant at 1000ms for easy bounding
    p.enable_jitter = true;

    RetryContext ctx(p);
    for (uint32_t i = 0; i < 50; ++i) {
        auto d = ctx.nextDelay();
        ASSERT_TRUE(d.has_value());
        EXPECT_GE(d->count(), 0)   << "jitter delay negative at attempt " << i;
        EXPECT_LE(d->count(), 1000) << "jitter delay exceeds base at attempt " << i;
    }
}

// ==========================================================================
// WPR-12: classifyBoostError — EAGAIN → kTransient
// ==========================================================================

TEST(WireProtocolRetry, WPR12_EagainIsTransient) {
    const boost::system::error_code ec(
        EAGAIN, boost::system::generic_category());
    EXPECT_EQ(classifyBoostError(ec), WireErrorClass::kTransient);
}

// ==========================================================================
// WPR-13: classifyBoostError — EINVAL → kPermanent
// ==========================================================================

TEST(WireProtocolRetry, WPR13_EinvalIsPermanent) {
    const boost::system::error_code ec(
        EINVAL, boost::system::generic_category());
    EXPECT_EQ(classifyBoostError(ec), WireErrorClass::kPermanent);
}

// ==========================================================================
// WPR-14: classifyBoostError — no error → kPermanent sentinel
// ==========================================================================

TEST(WireProtocolRetry, WPR14_NoErrorIsPermanentSentinel) {
    const boost::system::error_code ec; // success / no error
    // By convention, success is mapped to kPermanent (caller should not retry success).
    EXPECT_EQ(classifyBoostError(ec), WireErrorClass::kPermanent);
}

// ==========================================================================
// WPR-15: reset() restarts sequence without re-allocating policy
// ==========================================================================

TEST(WireProtocolRetry, WPR15_ResetRestartsSequence) {
    WireRetryPolicy p = WireRetryPolicy::forTesting();
    p.max_attempts = 2;
    RetryContext ctx(p);

    ctx.nextDelay(); // attempt 1
    ctx.nextDelay(); // attempt 2 — exhausted
    EXPECT_FALSE(ctx.canRetry());

    ctx.reset();
    EXPECT_EQ(ctx.attempts(), 0u);
    EXPECT_TRUE(ctx.canRetry());

    // After reset, delays are computed fresh.
    EXPECT_TRUE(ctx.nextDelay().has_value());
}

// ==========================================================================
// WPR-16: forConnectionIO policy has jitter + 5 max_attempts
// ==========================================================================

TEST(WireProtocolRetry, WPR16_ConnectionIOPolicy) {
    const auto p = WireRetryPolicy::forConnectionIO();
    EXPECT_EQ(p.max_attempts, 5u);
    EXPECT_TRUE(p.enable_jitter);
    EXPECT_GE(p.max_delay_ms, p.base_delay_ms);
}
