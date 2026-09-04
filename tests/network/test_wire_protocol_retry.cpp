/**
 * @file test_wire_protocol_retry.cpp
 * @brief Unit tests for wire-protocol retry logic (P5-S01).
 *
 * Coverage:
 *  1.  RetryPolicy default values
 *  2.  Custom RetryPolicy values
 *  3.  Backoff delay for attempt 0  (base * 2^0 = base)
 *  4.  Backoff delay for attempt 1  (base * 2^1 = 2*base)
 *  5.  Backoff delay for attempt 2  (base * 2^2 = 4*base)
 *  6.  Backoff delay for attempt 3  (base * 2^3 = 8*base)
 *  7.  Jitter stays within [0, jitter_ms]
 *  8.  Backoff never exceeds max_delay_ms
 *  9.  Backoff with zero jitter is deterministic (no randomness)
 * 10.  isTransient: ECONNRESET is retryable
 * 11.  isTransient: ETIMEDOUT is retryable
 * 12.  isTransient: EAGAIN is retryable
 * 13.  isTransient: ENOENT is NOT retryable
 * 14.  isTransient: 0 is NOT retryable
 * 15.  Max-retries semantics: retry counter stops at max_retries
 * 16.  IdempotencyCache: lookup misses on empty cache
 * 17.  IdempotencyCache: store + lookup roundtrip
 * 18.  IdempotencyCache: first-write-wins (second store does not overwrite)
 * 19.  IdempotencyCache: window eviction when full
 * 20.  IdempotencyCache: size() reflects entry count
 * 21.  IdempotencyCache: clear() empties the cache
 * 22.  IdempotencyCache: concurrent store+lookup is race-free
 * 23.  Retry state machine: immediate success (0 retries)
 * 24.  Retry state machine: fail-then-succeed on attempt 2
 * 25.  IdempotencyCache: lookupSnapshot returns stable snapshot after cache clear
 * 26.  IdempotencyCache: zero window disables retention
 * 27.  IdempotencyCache: multiple lookup pointers stay distinct per request id
 */


#include <gtest/gtest.h>
#include "network/wire_protocol_server.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace themis::network;
using namespace std::chrono_literals;

// ============================================================================
// Helpers
// ============================================================================

namespace {

/**
 * Simulate a retry loop using RetryPolicy::computeDelay and isTransient.
 * The delay function is replaced by @p delay_fn so tests run without sleeping.
 *
 * @param policy       Retry policy to use.
 * @param attempt_fn   Called for each attempt; returns errno-style code or 0
 *                     on success.
 * @param delay_fn     Called instead of sleep_for; records delays for
 *                     assertion.
 * @return             Number of attempts made (1 = success on first try).
 */
int runRetryLoop(
    const RetryPolicy& policy,
    std::function<int(uint32_t attempt)> attempt_fn,
    std::function<void(std::chrono::milliseconds)> delay_fn = nullptr)
{
    for (uint32_t i = 0; i <= policy.max_retries; ++i) {
        int err = attempt_fn(i);
        if (err == 0) {
            return static_cast<int>(i + 1); // success at attempt i+1
        }
        if (!RetryPolicy::isTransient(err) || i == policy.max_retries) {
            return -(static_cast<int>(i + 1)); // failure
        }
        auto delay = policy.computeDelay(i);
        if (delay_fn) {
            delay_fn(delay);
        }
    }
    return -1; // should not reach here
}

} // anonymous namespace

// ============================================================================
// 1. RetryPolicy default values
// ============================================================================

TEST(RetryPolicy, DefaultMaxRetriesIsThree) {
    RetryPolicy p;
    EXPECT_EQ(p.max_retries, 3u);
}

TEST(RetryPolicy, DefaultBaseDelayIsHundredMs) {
    RetryPolicy p;
    EXPECT_EQ(p.base_delay_ms, 100u);
}

TEST(RetryPolicy, DefaultJitterIsFiftyMs) {
    RetryPolicy p;
    EXPECT_EQ(p.jitter_ms, 50u);
}

TEST(RetryPolicy, DefaultMaxDelayIs2000Ms) {
    RetryPolicy p;
    EXPECT_EQ(p.max_delay_ms, 2000u);
}

// ============================================================================
// 2. Custom RetryPolicy values
// ============================================================================

TEST(RetryPolicy, CustomValuesRoundTrip) {
    RetryPolicy p;
    p.max_retries   = 5;
    p.base_delay_ms = 200;
    p.jitter_ms     = 0;
    p.max_delay_ms  = 5000;

    EXPECT_EQ(p.max_retries,   5u);
    EXPECT_EQ(p.base_delay_ms, 200u);
    EXPECT_EQ(p.jitter_ms,     0u);
    EXPECT_EQ(p.max_delay_ms,  5000u);
}

// ============================================================================
// 3–6. Backoff calculation (zero jitter for determinism)
// ============================================================================

TEST(RetryPolicy, BackoffAttempt0IsBaseDelay) {
    RetryPolicy p;
    p.base_delay_ms = 100;
    p.jitter_ms     = 0;
    p.max_delay_ms  = 10000;

    // attempt 0: base * 2^0 = 100 ms
    EXPECT_EQ(p.computeDelay(0), 100ms);
}

TEST(RetryPolicy, BackoffAttempt1IsDoubleBase) {
    RetryPolicy p;
    p.base_delay_ms = 100;
    p.jitter_ms     = 0;
    p.max_delay_ms  = 10000;

    // attempt 1: base * 2^1 = 200 ms
    EXPECT_EQ(p.computeDelay(1), 200ms);
}

TEST(RetryPolicy, BackoffAttempt2IsFourTimesBase) {
    RetryPolicy p;
    p.base_delay_ms = 100;
    p.jitter_ms     = 0;
    p.max_delay_ms  = 10000;

    // attempt 2: base * 2^2 = 400 ms
    EXPECT_EQ(p.computeDelay(2), 400ms);
}

TEST(RetryPolicy, BackoffAttempt3IsEightTimesBase) {
    RetryPolicy p;
    p.base_delay_ms = 100;
    p.jitter_ms     = 0;
    p.max_delay_ms  = 10000;

    // attempt 3: base * 2^3 = 800 ms
    EXPECT_EQ(p.computeDelay(3), 800ms);
}

// ============================================================================
// 7. Jitter stays within [0, jitter_ms]
// ============================================================================

TEST(RetryPolicy, JitterBoundedAboveByJitterMs) {
    RetryPolicy p;
    p.base_delay_ms = 100;
    p.jitter_ms     = 50;
    p.max_delay_ms  = 10000;

    // Run several samples and verify: lower bound = base, upper = base + jitter
    for (int i = 0; i < 200; ++i) {
        auto d = p.computeDelay(0);
        EXPECT_GE(d, 100ms) << "delay below base on sample " << i;
        EXPECT_LE(d, 150ms) << "delay above base+jitter on sample " << i;
    }
}

// ============================================================================
// 8. Backoff never exceeds max_delay_ms
// ============================================================================

TEST(RetryPolicy, BackoffCappedAtMaxDelay) {
    RetryPolicy p;
    p.base_delay_ms = 100;
    p.jitter_ms     = 0;
    p.max_delay_ms  = 500; // cap below the natural backoff at attempt 3

    // attempt 3 = 800 ms without cap → must be capped at 500
    EXPECT_EQ(p.computeDelay(3), 500ms);

    // Large attempt number — overflow-safe
    EXPECT_EQ(p.computeDelay(64), 500ms);
}

// ============================================================================
// 9. Zero jitter → deterministic result
// ============================================================================

TEST(RetryPolicy, ZeroJitterIsDeterministic) {
    RetryPolicy p;
    p.base_delay_ms = 200;
    p.jitter_ms     = 0;
    p.max_delay_ms  = 10000;

    const auto d0 = p.computeDelay(0);
    for (int i = 0; i < 50; ++i) {
        EXPECT_EQ(p.computeDelay(0), d0);
    }
}

// ============================================================================
// 10–14. RetryPolicy::isTransient
// ============================================================================

TEST(RetryPolicy, EconnresetIsTransient) {
    EXPECT_TRUE(RetryPolicy::isTransient(ECONNRESET));
}

TEST(RetryPolicy, EtimedoutIsTransient) {
    EXPECT_TRUE(RetryPolicy::isTransient(ETIMEDOUT));
}

TEST(RetryPolicy, EagainIsTransient) {
    EXPECT_TRUE(RetryPolicy::isTransient(EAGAIN));
}

TEST(RetryPolicy, EnoentIsNotTransient) {
    EXPECT_FALSE(RetryPolicy::isTransient(ENOENT));
}

TEST(RetryPolicy, ZeroErrorIsNotTransient) {
    EXPECT_FALSE(RetryPolicy::isTransient(0));
}

// ============================================================================
// 15. Max-retries stops the retry loop
// ============================================================================

TEST(RetryPolicy, MaxRetriesExceededReturnsFailure) {
    RetryPolicy p;
    p.max_retries   = 3;
    p.base_delay_ms = 0;
    p.jitter_ms     = 0;
    p.max_delay_ms  = 0;

    int attempts = 0;
    int result = runRetryLoop(
        p,
        [&](uint32_t) -> int {
            ++attempts;
            return ECONNRESET; // always fail
        });

    // runRetryLoop returns negative on failure
    EXPECT_LT(result, 0);
    // Should have tried: attempt 0,1,2,3 → 4 total
    EXPECT_EQ(attempts, static_cast<int>(p.max_retries + 1));
}

// ============================================================================
// 16–21. IdempotencyCache
// ============================================================================

TEST(IdempotencyCache, LookupMissOnEmpty) {
    IdempotencyCache cache;
    EXPECT_EQ(cache.lookup("req-1"), nullptr);
}

TEST(IdempotencyCache, StoreAndLookupRoundtrip) {
    IdempotencyCache cache;
    cache.store("req-1", R"({"ok":true})");
    const auto* entry = cache.lookup("req-1");
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->result, R"({"ok":true})");
}

TEST(IdempotencyCache, FirstWriteWins) {
    IdempotencyCache cache;
    cache.store("req-1", "first");
    cache.store("req-1", "second"); // should NOT replace
    const auto* entry = cache.lookup("req-1");
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->result, "first");
}

TEST(IdempotencyCache, WindowEvictsOldestEntry) {
    IdempotencyCache cache(/*window_size=*/2);
    cache.store("req-1", "r1");
    cache.store("req-2", "r2");
    // Window is now full; adding req-3 must evict req-1
    cache.store("req-3", "r3");

    EXPECT_EQ(cache.lookup("req-1"), nullptr) << "req-1 should have been evicted";
    EXPECT_NE(cache.lookup("req-2"), nullptr);
    EXPECT_NE(cache.lookup("req-3"), nullptr);
    EXPECT_EQ(cache.size(), 2u);
}

TEST(IdempotencyCache, SizeReflectsEntryCount) {
    IdempotencyCache cache(256);
    EXPECT_EQ(cache.size(), 0u);
    cache.store("a", "1");
    EXPECT_EQ(cache.size(), 1u);
    cache.store("b", "2");
    EXPECT_EQ(cache.size(), 2u);
}

TEST(IdempotencyCache, ClearEmptiesCache) {
    IdempotencyCache cache;
    cache.store("req-1", "r1");
    cache.store("req-2", "r2");
    cache.clear();
    EXPECT_EQ(cache.size(), 0u);
    EXPECT_EQ(cache.lookup("req-1"), nullptr);
}

TEST(IdempotencyCache, LookupSnapshotReturnsStableSnapshotAfterClear) {
    IdempotencyCache cache;
    cache.store("req-1", "r1");

    auto snapshot = cache.lookupSnapshot("req-1");
    ASSERT_TRUE(snapshot.has_value());

    cache.clear();

    EXPECT_EQ(snapshot->result, "r1");
    EXPECT_EQ(cache.size(), 0u);
}

TEST(IdempotencyCache, ZeroWindowDisablesRetention) {
    IdempotencyCache cache(/*window_size=*/0);
    cache.store("req-1", "r1");

    EXPECT_EQ(cache.size(), 0u);
    EXPECT_EQ(cache.lookup("req-1"), nullptr);
}

TEST(IdempotencyCache, MultipleLookupPointersStayDistinctPerRequestId) {
    IdempotencyCache cache;
    cache.store("req-1", "r1");
    cache.store("req-2", "r2");

    const auto* first = cache.lookup("req-1");
    const auto* second = cache.lookup("req-2");

    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_NE(first, second);
    EXPECT_EQ(first->result, "r1");
    EXPECT_EQ(second->result, "r2");
}

// ============================================================================
// 22. Concurrent store + lookup is race-free
// ============================================================================

TEST(IdempotencyCache, ConcurrentAccessIsRaceFree) {
    IdempotencyCache cache(1024);

    constexpr int kThreads     = 4;
    constexpr int kOpsPerThread = 50;

    std::vector<std::thread> threads;
    std::atomic<int> completed{0};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < kOpsPerThread; ++i) {
                std::string id = "t" + std::to_string(t) + "-" + std::to_string(i);
                cache.store(id, std::to_string(t * 1000 + i));
                cache.lookup(id); // may return nullptr if evicted; that's OK
                ++completed;
            }
        });
    }

    for (auto& th : threads) {
      th.join();
    }

    EXPECT_EQ(completed.load(), kThreads * kOpsPerThread);
    // Cache must be in a consistent state (size ≤ window_size)
    EXPECT_LE(cache.size(), 1024u);
}

// ============================================================================
// 23. Retry state machine: immediate success — no retry issued
// ============================================================================

TEST(RetryStateMachine, ImmediateSuccessNoRetry) {
    RetryPolicy p;
    p.base_delay_ms = 0;
    p.jitter_ms     = 0;

    int delays_fired = 0;
    int result = runRetryLoop(
        p,
        [](uint32_t) -> int { return 0; }, // always success
        [&](std::chrono::milliseconds) { ++delays_fired; });

    EXPECT_EQ(result, 1);   // succeeded on first attempt
    EXPECT_EQ(delays_fired, 0); // no delay was applied
}

// ============================================================================
// 24. Retry state machine: ECONNRESET on attempt 0, success on attempt 1
// ============================================================================

TEST(RetryStateMachine, TransientErrorThenSuccess) {
    RetryPolicy p;
    p.max_retries   = 3;
    p.base_delay_ms = 0;
    p.jitter_ms     = 0;

    int delays_fired = 0;
    int result = runRetryLoop(
        p,
        [](uint32_t attempt) -> int {
            return (attempt == 0) ? ECONNRESET : 0; // fail once, then succeed
        },
        [&](std::chrono::milliseconds) { ++delays_fired; });

    EXPECT_EQ(result, 2);   // succeeded on second attempt
    EXPECT_EQ(delays_fired, 1); // one delay between attempt 0 and attempt 1
}
