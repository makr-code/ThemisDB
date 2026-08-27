#include <gtest/gtest.h>
#include "utils/rate_limiter.h"
#include <thread>
#include <chrono>

using namespace themis::utils;
using namespace std::chrono_literals;

// ============================================================================
// Token bucket behaviour
// ============================================================================

TEST(UtilsRateLimiter, InitialBucketIsFull) {
    RateLimiter rl(10.0, 10.0);
    EXPECT_DOUBLE_EQ(rl.available(), 10.0);
}

TEST(UtilsRateLimiter, TryAcquireSucceedsWhenTokensAvailable) {
    RateLimiter rl(10.0, 10.0);
    EXPECT_TRUE(rl.try_acquire(1.0));
}

TEST(UtilsRateLimiter, TryAcquireFailsWhenInsufficientTokens) {
    RateLimiter rl(1.0, 3.0);
    // Drain all tokens
    EXPECT_TRUE(rl.try_acquire(3.0));
    // Next attempt must fail immediately
    EXPECT_FALSE(rl.try_acquire(1.0));
}

TEST(UtilsRateLimiter, ResetFillsBucketToMax) {
    RateLimiter rl(10.0, 10.0);
    rl.try_acquire(9.0);
    rl.reset();
    EXPECT_DOUBLE_EQ(rl.available(), 10.0);
}

TEST(UtilsRateLimiter, SetRateChangesRefillSpeed) {
    RateLimiter rl(1000.0, 5.0);
    // Drain all tokens
    rl.try_acquire(5.0);
    rl.set_rate(1000000.0); // Very fast refill
    // Short sleep should be enough to refill at the new rate
    std::this_thread::sleep_for(100us);
    EXPECT_GT(rl.available(), 0.0);
}

TEST(UtilsRateLimiter, AcquireBlocksAndCompletes) {
    RateLimiter rl(1000.0, 1.0); // 1000 tokens/s, burst=1
    // Drain
    rl.try_acquire(1.0);
    // Blocking acquire should complete quickly given high rate
    auto start = std::chrono::steady_clock::now();
    EXPECT_TRUE(rl.acquire_with_timeout(1.0, 100ms));
    auto elapsed = std::chrono::steady_clock::now() - start;
    // Must complete within 100 ms (theoretical: ~1 ms at 1000 tok/s)
    EXPECT_LT(elapsed, 100ms);
}

TEST(UtilsRateLimiter, AvailableNeverExceedsBurst) {
    RateLimiter rl(1000.0, 5.0);
    // Wait and then check: tokens should not exceed burst
    std::this_thread::sleep_for(10ms);
    EXPECT_LE(rl.available(), 5.0);
}
