/**
 * @file test_adaptive_throttling_comprehensive.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <gtest/gtest.h>
#include "server/rate_limiter.h"
#include <thread>
#include <chrono>

using namespace themis::server;

namespace {

// Create a config that is very easy to trigger adaptive throttling
RateLimitConfig makeAdaptiveConfig(uint32_t threshold = 3,
                                    uint32_t window    = 60,
                                    uint32_t penalty_secs = 120) {
    RateLimitConfig cfg;
    cfg.bucket_capacity  = 2;
    cfg.refill_rate      = 0.001; // Very slow refill – requests exhaust quickly
    cfg.per_ip_enabled   = true;
    cfg.per_user_enabled = false;
    cfg.adaptive_throttling_enabled = true;
    cfg.adaptive_rejection_threshold = threshold;
    cfg.adaptive_window_seconds      = window;
    cfg.adaptive_penalty_factor      = 0.25;
    cfg.adaptive_penalty_duration_seconds = penalty_secs;
    return cfg;
}

} // anonymous namespace

// ============================================================================
// Default config: adaptive throttling disabled
// ============================================================================

TEST(AdaptiveThrottleDefaultTest, DisabledByDefault_NoSideEffects) {
    RateLimitConfig cfg;
    cfg.bucket_capacity = 5;
    cfg.refill_rate = 0.001;
    RateLimiter limiter(cfg);

    // Exhaust bucket
    for (int i = 0; i < 5; ++i) {
      limiter.allowRequest("ip1");
    }
    // Several more rejections
    for (int i = 0; i < 10; ++i) {
      limiter.allowRequest("ip1");
    }

    // Adaptive throttling is disabled – isAdaptivelyThrottled should return false
    EXPECT_FALSE(limiter.isAdaptivelyThrottled("ip1"));
}

// ============================================================================
// Penalty detection tests
// ============================================================================

class AdaptiveThrottleTest : public ::testing::Test {
protected:
    void SetUp() override {
        // threshold=3 means the 3rd rejection in the window triggers penalty
        limiter_ = std::make_unique<RateLimiter>(makeAdaptiveConfig(3));
    }

    std::unique_ptr<RateLimiter> limiter_;
};

TEST_F(AdaptiveThrottleTest, NoPenalty_BeforeThreshold) {
    // Exhaust bucket (2 tokens), causing 1 rejection
    limiter_->allowRequest("suspect");
    limiter_->allowRequest("suspect");
    limiter_->allowRequest("suspect"); // 1 rejection (bucket empty)

    // 1 rejection, threshold is 3 → no penalty yet
    EXPECT_FALSE(limiter_->isAdaptivelyThrottled("suspect"));
}

TEST_F(AdaptiveThrottleTest, PenaltyApplied_AfterThreshold) {
    // Exhaust bucket
    limiter_->allowRequest("attacker");
    limiter_->allowRequest("attacker");
    // Now cause 3 more rejections (bucket stays empty)
    limiter_->allowRequest("attacker"); // reject 1
    limiter_->allowRequest("attacker"); // reject 2
    limiter_->allowRequest("attacker"); // reject 3 → penalty triggered

    EXPECT_TRUE(limiter_->isAdaptivelyThrottled("attacker"));
}

TEST_F(AdaptiveThrottleTest, DifferentIPs_IndependentPenalties) {
    // Trigger penalty only for "bad-ip"
    limiter_->allowRequest("bad-ip");
    limiter_->allowRequest("bad-ip");
    for (int i = 0; i < 3; ++i) limiter_->allowRequest("bad-ip"); // 3 rejections

    EXPECT_TRUE(limiter_->isAdaptivelyThrottled("bad-ip"));
    EXPECT_FALSE(limiter_->isAdaptivelyThrottled("good-ip"));
}

TEST_F(AdaptiveThrottleTest, UnpenalisedIP_StillAllowed_Normally) {
    // Use a fresh limiter with large capacity so "good-ip" is never exhausted
    RateLimitConfig cfg;
    cfg.bucket_capacity = 100;
    cfg.refill_rate = 100.0;
    cfg.per_ip_enabled = true;
    cfg.adaptive_throttling_enabled = true;
    cfg.adaptive_rejection_threshold = 3;
    cfg.adaptive_window_seconds = 60;
    cfg.adaptive_penalty_duration_seconds = 120;
    RateLimiter limiter(cfg);

    // Trigger penalty for "bad"
    // We cannot exhaust the 100-token bucket easily, so use a separate limiter
    // for the penalty part. Here we just verify the normal IP is unaffected.
    EXPECT_TRUE(limiter.allowRequest("good-ip"));
    EXPECT_TRUE(limiter.allowRequest("good-ip"));
    EXPECT_FALSE(limiter.isAdaptivelyThrottled("good-ip"));
}

// ============================================================================
// Reset clears adaptive state
// ============================================================================

TEST_F(AdaptiveThrottleTest, Reset_ClearsAdaptiveState) {
    limiter_->allowRequest("victim");
    limiter_->allowRequest("victim");
    for (int i = 0; i < 3; ++i) {
      limiter_->allowRequest("victim");
    }

    ASSERT_TRUE(limiter_->isAdaptivelyThrottled("victim"));

    limiter_->reset();
    EXPECT_FALSE(limiter_->isAdaptivelyThrottled("victim"));
}

// ============================================================================
// Penalty expiry test (very short penalty window)
// ============================================================================

TEST(AdaptiveThrottlePenaltyExpiryTest, PenaltyExpires_AfterDuration) {
    RateLimitConfig cfg = makeAdaptiveConfig(
        /*threshold=*/3,
        /*window=*/60,
        /*penalty_secs=*/1  // Expires after 1 second
    );
    RateLimiter limiter(cfg);

    // Exhaust + trigger penalty
    limiter.allowRequest("transient");
    limiter.allowRequest("transient");
    for (int i = 0; i < 3; ++i) {
      limiter.allowRequest("transient");
    }

    ASSERT_TRUE(limiter.isAdaptivelyThrottled("transient"));

    // Wait for penalty to expire
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    // Next request clears the expired penalty
    limiter.allowRequest("transient");

    EXPECT_FALSE(limiter.isAdaptivelyThrottled("transient"));
}

// ============================================================================
// Statistics: penalty count in getStatistics()
// ============================================================================

TEST_F(AdaptiveThrottleTest, Statistics_ReflectsPenaltyCount) {
    limiter_->allowRequest("p1");
    limiter_->allowRequest("p1");
    for (int i = 0; i < 3; ++i) {
      limiter_->allowRequest("p1");
    }
    ASSERT_TRUE(limiter_->isAdaptivelyThrottled("p1"));

    auto stats = limiter_->getStatistics();
    EXPECT_GE(stats.adaptive_throttle_penalties, 1u);
}

TEST_F(AdaptiveThrottleTest, Statistics_NoPenaltiesWhenNoneTriggered) {
    limiter_->allowRequest("harmless");
    limiter_->allowRequest("harmless");
    // Only 0 rejections yet

    auto stats = limiter_->getStatistics();
    EXPECT_EQ(stats.adaptive_throttle_penalties, 0u);
}

// ============================================================================
// Adaptive config field defaults
// ============================================================================

TEST(AdaptiveConfigTest, DefaultConfig_AdaptiveDisabled) {
    RateLimitConfig cfg;
    EXPECT_FALSE(cfg.adaptive_throttling_enabled);
}

TEST(AdaptiveConfigTest, DefaultThreshold_IsReasonable) {
    RateLimitConfig cfg;
    EXPECT_GT(cfg.adaptive_rejection_threshold, 0u);
    EXPECT_GT(cfg.adaptive_window_seconds, 0u);
    EXPECT_GT(cfg.adaptive_penalty_duration_seconds, 0u);
    EXPECT_GT(cfg.adaptive_penalty_factor, 0.0);
    EXPECT_LT(cfg.adaptive_penalty_factor, 1.0);
}
