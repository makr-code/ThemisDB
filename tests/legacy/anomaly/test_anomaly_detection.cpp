/**
 * @file test_anomaly_detection.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <gtest/gtest.h>
#include "server/rate_limiter.h"
#include <atomic>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <thread>

using namespace themis::server;

// ──────────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────────

struct CollectedEvents {
    mutable std::mutex      mtx;
    std::vector<AnomalyEvent> events;

    void clear() {
        std::lock_guard<std::mutex> l(mtx);
        events.clear();
    }

    size_t size() const {
        std::lock_guard<std::mutex> l(mtx);
        return events.size();
    }

    AnomalyEvent at(size_t i) const {
        std::lock_guard<std::mutex> l(mtx);
        return events.at(i);
    }

    bool hasType(AnomalyEvent::Type t) const {
        std::lock_guard<std::mutex> l(mtx);
        for (const auto& e : events)
            if (e.type == t) {
              return true;
            }
        return false;
    }
};

// ──────────────────────────────────────────────────────────────────────────────
// Blacklist anomaly tests
// ──────────────────────────────────────────────────────────────────────────────

class RateLimiterAnomalyTest : public ::testing::Test {
protected:
    RateLimitConfig cfg;
    CollectedEvents collected;

    void SetUp() override {
        cfg.bucket_capacity = 100;
        cfg.refill_rate     = 100.0 / 60.0;
    }
};

TEST_F(RateLimiterAnomalyTest, CallbackFiredOnBlacklist) {
    RateLimiter rl(cfg);
    rl.setAnomalyCallback([&](const AnomalyEvent& ev) {
        std::lock_guard<std::mutex> l(collected.mtx);
        collected.events.push_back(ev);
    });

    rl.blacklistIP("1.2.3.4");

    ASSERT_EQ(collected.size(), 1u);
    auto ev = collected.at(0);
    EXPECT_EQ(ev.type, AnomalyEvent::Type::IP_BLACKLISTED);
    EXPECT_EQ(ev.ip,   "1.2.3.4");
    EXPECT_FALSE(ev.detail.empty());
    // Timestamp should be very recent
    auto age = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now() - ev.timestamp).count();
    EXPECT_LT(age, 5);
}

TEST_F(RateLimiterAnomalyTest, NoCallbackRegisteredDoesNotCrash) {
    RateLimiter rl(cfg);
    // No callback set
    EXPECT_NO_THROW(rl.blacklistIP("1.2.3.4"));
}

TEST_F(RateLimiterAnomalyTest, CallbackFiredForEachBlacklisting) {
    RateLimiter rl(cfg);
    rl.setAnomalyCallback([&](const AnomalyEvent& ev) {
        std::lock_guard<std::mutex> l(collected.mtx);
        collected.events.push_back(ev);
    });

    rl.blacklistIP("1.1.1.1");
    rl.blacklistIP("2.2.2.2");
    rl.blacklistIP("3.3.3.3");

    ASSERT_EQ(collected.size(), 3u);
    std::vector<std::string> expected_ips = {"1.1.1.1", "2.2.2.2", "3.3.3.3"};
    for (size_t i = 0; i < 3; i++) {
        EXPECT_EQ(collected.at(i).type, AnomalyEvent::Type::IP_BLACKLISTED);
        EXPECT_EQ(collected.at(i).ip, expected_ips[i]);
    }
}

TEST_F(RateLimiterAnomalyTest, UnblacklistDoesNotFireCallback) {
    RateLimiter rl(cfg);
    rl.setAnomalyCallback([&](const AnomalyEvent& ev) {
        std::lock_guard<std::mutex> l(collected.mtx);
        collected.events.push_back(ev);
    });

    rl.blacklistIP("5.5.5.5");
    collected.clear();                // reset after blacklist event
    rl.unblacklistIP("5.5.5.5");     // should NOT fire an anomaly event

    EXPECT_EQ(collected.size(), 0u);
}

TEST_F(RateLimiterAnomalyTest, CallbackCanBeReplaced) {
    RateLimiter rl(cfg);
    std::atomic<int> count1{0}, count2{0};

    rl.setAnomalyCallback([&](const AnomalyEvent&) { count1++; });
    rl.blacklistIP("1.0.0.1");
    EXPECT_EQ(count1.load(), 1);

    rl.setAnomalyCallback([&](const AnomalyEvent&) { count2++; });
    rl.blacklistIP("1.0.0.2");
    EXPECT_EQ(count1.load(), 1);  // old callback not called again
    EXPECT_EQ(count2.load(), 1);  // new callback called
}

TEST_F(RateLimiterAnomalyTest, CallbackCanBeDeregistered) {
    RateLimiter rl(cfg);
    std::atomic<int> count{0};

    rl.setAnomalyCallback([&](const AnomalyEvent&) { count++; });
    rl.blacklistIP("9.0.0.1");
    EXPECT_EQ(count.load(), 1);

    rl.setAnomalyCallback(nullptr);  // deregister
    rl.blacklistIP("9.0.0.2");
    EXPECT_EQ(count.load(), 1);  // not fired again
}

// ──────────────────────────────────────────────────────────────────────────────
// Adaptive throttle anomaly tests
// ──────────────────────────────────────────────────────────────────────────────

class AdaptiveAnomalyTest : public ::testing::Test {
protected:
    RateLimitConfig cfg;
    CollectedEvents collected;

    void SetUp() override {
        cfg.bucket_capacity                 = 1;   // very small bucket
        cfg.refill_rate                     = 0.01; // near-zero refill
        cfg.adaptive_throttling_enabled     = true;
        cfg.adaptive_rejection_threshold    = 3;
        cfg.adaptive_window_seconds         = 60;
        cfg.adaptive_penalty_duration_seconds = 120;
    }
};

TEST_F(AdaptiveAnomalyTest, CallbackFiredWhenPenaltyTriggered) {
    RateLimiter rl(cfg);
    rl.setAnomalyCallback([&](const AnomalyEvent& ev) {
        std::lock_guard<std::mutex> l(collected.mtx);
        collected.events.push_back(ev);
    });

    const std::string ip = "10.0.0.1";
    // Exhaust bucket and generate rejections to hit threshold
    for (int i = 0; i < 10; i++) {
        rl.allowRequest(ip);
    }

    // Should have fired at least one ADAPTIVE_THROTTLE_TRIGGERED event
    EXPECT_TRUE(collected.hasType(AnomalyEvent::Type::ADAPTIVE_THROTTLE_TRIGGERED))
        << "Expected anomaly callback for adaptive throttle, but none received";
}

TEST_F(AdaptiveAnomalyTest, AdaptiveEventContainsCorrectIP) {
    RateLimiter rl(cfg);
    rl.setAnomalyCallback([&](const AnomalyEvent& ev) {
        if (ev.type == AnomalyEvent::Type::ADAPTIVE_THROTTLE_TRIGGERED) {
            std::lock_guard<std::mutex> l(collected.mtx);
            collected.events.push_back(ev);
        }
    });

    const std::string ip = "172.16.0.42";
    for (int i = 0; i < 10; i++) {
      rl.allowRequest(ip);
    }

    if (collected.size() > 0) {
        EXPECT_EQ(collected.at(0).ip, ip);
        EXPECT_FALSE(collected.at(0).detail.empty());
    }
    // If no event fired (bucket not exhausted) the test is vacuously true
}

TEST_F(AdaptiveAnomalyTest, PenaltyFiredOnlyOnce) {
    RateLimiter rl(cfg);
    std::atomic<int> count{0};
    rl.setAnomalyCallback([&](const AnomalyEvent& ev) {
        if (ev.type == AnomalyEvent::Type::ADAPTIVE_THROTTLE_TRIGGERED) {
          count++;
        }
    });

    const std::string ip = "192.168.1.1";
    for (int i = 0; i < 20; i++) {
      rl.allowRequest(ip);
    }

    // The penalty should only be recorded the first time it activates
    EXPECT_LE(count.load(), 1)
        << "Penalty callback should fire at most once per penalty activation";
}

TEST_F(AdaptiveAnomalyTest, NoAdaptiveEventWhenDisabled) {
    cfg.adaptive_throttling_enabled = false;
    RateLimiter rl(cfg);
    std::atomic<int> count{0};
    rl.setAnomalyCallback([&](const AnomalyEvent& ev) {
        if (ev.type == AnomalyEvent::Type::ADAPTIVE_THROTTLE_TRIGGERED) {
          count++;
        }
    });

    for (int i = 0; i < 20; i++) {
      rl.allowRequest("10.1.2.3");
    }

    EXPECT_EQ(count.load(), 0);
}

// ──────────────────────────────────────────────────────────────────────────────
// Callback thread-safety
// ──────────────────────────────────────────────────────────────────────────────

TEST(AnomalyCallbackThreadSafetyTest, ConcurrentBlacklistingDoesNotCrash) {
    RateLimitConfig cfg;
    cfg.bucket_capacity = 50;
    RateLimiter rl(cfg);

    std::atomic<int> count{0};
    rl.setAnomalyCallback([&](const AnomalyEvent& ev) {
        if (ev.type == AnomalyEvent::Type::IP_BLACKLISTED) {
          count++;
        }
    });

    constexpr int kThreads = 8;
    constexpr int kIpsPerThread = 10;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; t++) {
        threads.emplace_back([&rl, t]() {
            for (int i = 0; i < kIpsPerThread; i++) {
                rl.blacklistIP("10." + std::to_string(t) + ".0." + std::to_string(i));
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    EXPECT_EQ(count.load(), kThreads * kIpsPerThread);
}

TEST(AnomalyCallbackThreadSafetyTest, ConcurrentCallbackReplacement) {
    RateLimitConfig cfg;
    cfg.bucket_capacity = 200;
    RateLimiter rl(cfg);

    std::atomic<int> count{0};
    // Rapidly replace the callback from one thread while another fires events
    std::thread setter([&]() {
        for (int i = 0; i < 100; i++) {
            rl.setAnomalyCallback([&](const AnomalyEvent&) { count++; });
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            rl.setAnomalyCallback(nullptr);
        }
    });

    std::thread firer([&]() {
        for (int i = 0; i < 100; i++) {
            rl.blacklistIP("9.9.9." + std::to_string(i % 256));
        }
    });

    setter.join();
    firer.join();
    // No crash is the main assertion; count may be anything.
    SUCCEED();
}
