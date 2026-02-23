/**
 * @file test_auth_anomaly_detection.cpp
 * @brief Tests for authentication anomaly detection:
 *        brute-force and credential-stuffing pattern detection in AuthRateLimiter.
 */

#include <gtest/gtest.h>
#include "auth/auth_rate_limiter.h"
#include <atomic>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>

using namespace themis::auth;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

struct CollectedAuthEvents {
    mutable std::mutex mtx;
    std::vector<AuthAnomalyEvent> events;

    void clear() {
        std::lock_guard<std::mutex> l(mtx);
        events.clear();
    }

    size_t size() const {
        std::lock_guard<std::mutex> l(mtx);
        return events.size();
    }

    AuthAnomalyEvent at(size_t i) const {
        std::lock_guard<std::mutex> l(mtx);
        return events.at(i);
    }

    bool hasType(AuthAnomalyEvent::Type t) const {
        std::lock_guard<std::mutex> l(mtx);
        for (const auto& e : events)
            if (e.type == t) return true;
        return false;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Brute-force / account-lockout anomaly tests
// ─────────────────────────────────────────────────────────────────────────────

class BruteForceAnomalyTest : public ::testing::Test {
protected:
    AuthRateLimitConfig cfg;
    CollectedAuthEvents collected;

    void SetUp() override {
        cfg.lockout_failed_attempts = 3;
        cfg.lockout_window          = std::chrono::minutes(5);
        cfg.lockout_duration        = std::chrono::minutes(10);
        cfg.enable_ip_rate_limiting   = false;
        cfg.enable_user_rate_limiting = false;
        cfg.enable_credential_stuffing_detection = false;
    }
};

TEST_F(BruteForceAnomalyTest, LockoutTriggersAccountLockoutEvent) {
    AuthRateLimiter rl(cfg);
    rl.setAnomalyCallback([&](const AuthAnomalyEvent& ev) {
        std::lock_guard<std::mutex> l(collected.mtx);
        collected.events.push_back(ev);
    });

    const std::string user = "alice";
    const std::string ip   = "10.0.0.1";

    rl.recordFailedAuth(user, ip, "invalid_password");
    rl.recordFailedAuth(user, ip, "invalid_password");
    rl.recordFailedAuth(user, ip, "invalid_password");  // triggers lockout

    EXPECT_TRUE(collected.hasType(AuthAnomalyEvent::Type::ACCOUNT_LOCKOUT_TRIGGERED));
}

TEST_F(BruteForceAnomalyTest, LockoutTriggersBruteForceEvent) {
    AuthRateLimiter rl(cfg);
    rl.setAnomalyCallback([&](const AuthAnomalyEvent& ev) {
        std::lock_guard<std::mutex> l(collected.mtx);
        collected.events.push_back(ev);
    });

    rl.recordFailedAuth("bob", "1.2.3.4", "bad_pw");
    rl.recordFailedAuth("bob", "1.2.3.4", "bad_pw");
    rl.recordFailedAuth("bob", "1.2.3.4", "bad_pw");

    EXPECT_TRUE(collected.hasType(AuthAnomalyEvent::Type::BRUTE_FORCE_DETECTED));
}

TEST_F(BruteForceAnomalyTest, BruteForceEventContainsCorrectFields) {
    AuthRateLimiter rl(cfg);
    rl.setAnomalyCallback([&](const AuthAnomalyEvent& ev) {
        if (ev.type == AuthAnomalyEvent::Type::BRUTE_FORCE_DETECTED) {
            std::lock_guard<std::mutex> l(collected.mtx);
            collected.events.push_back(ev);
        }
    });

    rl.recordFailedAuth("charlie", "5.5.5.5", "bad_cred");
    rl.recordFailedAuth("charlie", "5.5.5.5", "bad_cred");
    rl.recordFailedAuth("charlie", "5.5.5.5", "bad_cred");

    ASSERT_TRUE(collected.hasType(AuthAnomalyEvent::Type::BRUTE_FORCE_DETECTED));
    auto ev = collected.at(0);
    EXPECT_EQ(ev.ip,      "5.5.5.5");
    EXPECT_EQ(ev.user_id, "charlie");
    EXPECT_FALSE(ev.detail.empty());

    auto age = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now() - ev.timestamp).count();
    EXPECT_LT(age, 5);
}

TEST_F(BruteForceAnomalyTest, NoCallbackDoesNotCrash) {
    AuthRateLimiter rl(cfg);
    // No callback registered
    EXPECT_NO_THROW({
        rl.recordFailedAuth("user1", "1.1.1.1", "bad");
        rl.recordFailedAuth("user1", "1.1.1.1", "bad");
        rl.recordFailedAuth("user1", "1.1.1.1", "bad");
    });
}

TEST_F(BruteForceAnomalyTest, CallbackNotFiredBeforeLockoutThreshold) {
    AuthRateLimiter rl(cfg);
    rl.setAnomalyCallback([&](const AuthAnomalyEvent& ev) {
        std::lock_guard<std::mutex> l(collected.mtx);
        collected.events.push_back(ev);
    });

    // Only 2 failures – threshold is 3, so no lockout
    rl.recordFailedAuth("dave", "2.2.2.2", "bad");
    rl.recordFailedAuth("dave", "2.2.2.2", "bad");

    EXPECT_FALSE(collected.hasType(AuthAnomalyEvent::Type::ACCOUNT_LOCKOUT_TRIGGERED));
    EXPECT_FALSE(collected.hasType(AuthAnomalyEvent::Type::BRUTE_FORCE_DETECTED));
}

TEST_F(BruteForceAnomalyTest, CallbackCanBeDeregistered) {
    AuthRateLimiter rl(cfg);
    std::atomic<int> count{0};

    rl.setAnomalyCallback([&](const AuthAnomalyEvent&) { count++; });
    rl.recordFailedAuth("eve", "3.3.3.3", "x");
    rl.recordFailedAuth("eve", "3.3.3.3", "x");
    rl.recordFailedAuth("eve", "3.3.3.3", "x");  // lockout fires → count >= 1
    int fired = count.load();
    EXPECT_GT(fired, 0);

    rl.setAnomalyCallback(nullptr);  // deregister
    // Lock a different account – callback must NOT be called
    rl.recordFailedAuth("frank", "4.4.4.4", "x");
    rl.recordFailedAuth("frank", "4.4.4.4", "x");
    rl.recordFailedAuth("frank", "4.4.4.4", "x");
    EXPECT_EQ(count.load(), fired);  // no new calls
}

// ─────────────────────────────────────────────────────────────────────────────
// Credential-stuffing detection tests
// ─────────────────────────────────────────────────────────────────────────────

class CredentialStuffingTest : public ::testing::Test {
protected:
    AuthRateLimitConfig cfg;
    CollectedAuthEvents collected;

    void SetUp() override {
        cfg.enable_ip_rate_limiting   = false;
        cfg.enable_user_rate_limiting = false;
        cfg.enable_account_lockout    = false;
        cfg.enable_credential_stuffing_detection = true;
        cfg.credential_stuffing_user_threshold   = 5;
        cfg.credential_stuffing_window_seconds   = 60;
    }
};

TEST_F(CredentialStuffingTest, AlertFiredWhenThresholdReached) {
    AuthRateLimiter rl(cfg);
    rl.setAnomalyCallback([&](const AuthAnomalyEvent& ev) {
        std::lock_guard<std::mutex> l(collected.mtx);
        collected.events.push_back(ev);
    });

    const std::string ip = "192.168.1.1";
    // Attempt authentication as 5 distinct users from the same IP
    for (int i = 0; i < 5; ++i) {
        rl.allowAuthAttempt(ip, "user" + std::to_string(i));
    }

    EXPECT_TRUE(collected.hasType(AuthAnomalyEvent::Type::CREDENTIAL_STUFFING_SUSPECTED));
}

TEST_F(CredentialStuffingTest, AlertNotFiredBelowThreshold) {
    AuthRateLimiter rl(cfg);
    rl.setAnomalyCallback([&](const AuthAnomalyEvent& ev) {
        std::lock_guard<std::mutex> l(collected.mtx);
        collected.events.push_back(ev);
    });

    const std::string ip = "10.0.0.1";
    for (int i = 0; i < 4; ++i) {  // threshold is 5
        rl.allowAuthAttempt(ip, "user" + std::to_string(i));
    }

    EXPECT_FALSE(collected.hasType(AuthAnomalyEvent::Type::CREDENTIAL_STUFFING_SUSPECTED));
}

TEST_F(CredentialStuffingTest, AlertContainsCorrectIP) {
    AuthRateLimiter rl(cfg);
    rl.setAnomalyCallback([&](const AuthAnomalyEvent& ev) {
        if (ev.type == AuthAnomalyEvent::Type::CREDENTIAL_STUFFING_SUSPECTED) {
            std::lock_guard<std::mutex> l(collected.mtx);
            collected.events.push_back(ev);
        }
    });

    const std::string ip = "172.16.0.1";
    for (int i = 0; i < 5; ++i) {
        rl.allowAuthAttempt(ip, "victim" + std::to_string(i));
    }

    ASSERT_GT(collected.size(), 0u);
    EXPECT_EQ(collected.at(0).ip, ip);
    EXPECT_FALSE(collected.at(0).detail.empty());
}

TEST_F(CredentialStuffingTest, SameUserRepeatedDoesNotTriggerAlert) {
    AuthRateLimiter rl(cfg);
    rl.setAnomalyCallback([&](const AuthAnomalyEvent& ev) {
        std::lock_guard<std::mutex> l(collected.mtx);
        collected.events.push_back(ev);
    });

    const std::string ip = "10.10.10.1";
    // 10 attempts, all the same username – not credential stuffing
    for (int i = 0; i < 10; ++i) {
        rl.allowAuthAttempt(ip, "singleuser");
    }

    EXPECT_FALSE(collected.hasType(AuthAnomalyEvent::Type::CREDENTIAL_STUFFING_SUSPECTED));
}

TEST_F(CredentialStuffingTest, AlertFiredOncePerEvent) {
    AuthRateLimiter rl(cfg);
    std::atomic<int> count{0};
    rl.setAnomalyCallback([&](const AuthAnomalyEvent& ev) {
        if (ev.type == AuthAnomalyEvent::Type::CREDENTIAL_STUFFING_SUSPECTED) count++;
    });

    const std::string ip = "10.20.30.40";
    for (int i = 0; i < 20; ++i) {
        rl.allowAuthAttempt(ip, "u" + std::to_string(i));
    }

    // Alert should fire exactly once (the first time the threshold is crossed)
    EXPECT_EQ(count.load(), 1);
}

TEST_F(CredentialStuffingTest, DetectionDisabledByConfig) {
    cfg.enable_credential_stuffing_detection = false;
    AuthRateLimiter rl(cfg);
    std::atomic<int> count{0};
    rl.setAnomalyCallback([&](const AuthAnomalyEvent& ev) {
        if (ev.type == AuthAnomalyEvent::Type::CREDENTIAL_STUFFING_SUSPECTED) count++;
    });

    const std::string ip = "1.2.3.4";
    for (int i = 0; i < 20; ++i) {
        rl.allowAuthAttempt(ip, "user" + std::to_string(i));
    }

    EXPECT_EQ(count.load(), 0);
}

TEST_F(CredentialStuffingTest, AlertAlsoFiredFromFailedAuth) {
    AuthRateLimiter rl(cfg);
    rl.setAnomalyCallback([&](const AuthAnomalyEvent& ev) {
        std::lock_guard<std::mutex> l(collected.mtx);
        collected.events.push_back(ev);
    });

    const std::string ip = "11.22.33.44";
    for (int i = 0; i < 5; ++i) {
        rl.recordFailedAuth("victim" + std::to_string(i), ip, "bad_pw");
    }

    EXPECT_TRUE(collected.hasType(AuthAnomalyEvent::Type::CREDENTIAL_STUFFING_SUSPECTED));
}

// ─────────────────────────────────────────────────────────────────────────────
// Thread-safety
// ─────────────────────────────────────────────────────────────────────────────

TEST(AuthAnomalyThreadSafetyTest, ConcurrentRecordFailedAuthDoesNotCrash) {
    AuthRateLimitConfig cfg;
    cfg.lockout_failed_attempts = 3;
    cfg.enable_ip_rate_limiting   = false;
    cfg.enable_user_rate_limiting = false;
    cfg.enable_credential_stuffing_detection = true;
    cfg.credential_stuffing_user_threshold   = 5;

    AuthRateLimiter rl(cfg);
    std::atomic<int> count{0};
    rl.setAnomalyCallback([&](const AuthAnomalyEvent&) { count++; });

    constexpr int kThreads = 4;
    constexpr int kOpsPerThread = 20;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&rl, t]() {
            const std::string ip = "10." + std::to_string(t) + ".0.1";
            for (int i = 0; i < kOpsPerThread; ++i) {
                rl.recordFailedAuth("user_" + std::to_string(t) + "_" + std::to_string(i),
                                    ip, "bad_pw");
            }
        });
    }
    for (auto& th : threads) th.join();

    // Just verify no crash; count may be > 0
    SUCCEED();
}

TEST(AuthAnomalyThreadSafetyTest, ConcurrentCallbackReplacement) {
    AuthRateLimitConfig cfg;
    cfg.lockout_failed_attempts = 2;
    cfg.enable_ip_rate_limiting   = false;
    cfg.enable_user_rate_limiting = false;

    AuthRateLimiter rl(cfg);
    std::atomic<int> count{0};

    std::thread setter([&]() {
        for (int i = 0; i < 50; ++i) {
            rl.setAnomalyCallback([&](const AuthAnomalyEvent&) { count++; });
            std::this_thread::sleep_for(std::chrono::microseconds(50));
            rl.setAnomalyCallback(nullptr);
        }
    });

    std::thread firer([&]() {
        for (int i = 0; i < 50; ++i) {
            rl.recordFailedAuth("u" + std::to_string(i), "9.9.9.9", "x");
            rl.recordFailedAuth("u" + std::to_string(i), "9.9.9.9", "x");
        }
    });

    setter.join();
    firer.join();
    SUCCEED();  // no crash is the assertion
}
