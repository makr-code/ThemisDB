/**
 * @file test_auth_anomaly_detection.cpp
 * @brief Tests for authentication anomaly detection:
 *        brute-force and credential-stuffing pattern detection in AuthRateLimiter,
 *        and audit logger integration via setAuditLogger().
 */

#include <gtest/gtest.h>
#include "auth/auth_rate_limiter.h"
#include "auth/auth_metrics.h"
#include "utils/audit_logger.h"
#include <atomic>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>

using namespace themis::auth;
using namespace themis::utils;

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
            if (e.type == t) {
              return true;
            }
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
        if (ev.type == AuthAnomalyEvent::Type::CREDENTIAL_STUFFING_SUSPECTED) {
          count++;
        }
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
        if (ev.type == AuthAnomalyEvent::Type::CREDENTIAL_STUFFING_SUSPECTED) {
          count++;
        }
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
    for (auto& th : threads) {
      th.join();
    }

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

// ─────────────────────────────────────────────────────────────────────────────
// Audit logger integration tests
// ─────────────────────────────────────────────────────────────────────────────

namespace {
AuditLoggerConfig makeAnomalyTestConfig(const std::string& log_path) {
    AuditLoggerConfig cfg;
    cfg.enabled             = true;
    cfg.encrypt_then_sign   = false;
    cfg.log_path            = log_path;
    cfg.key_id              = "test-key";
    cfg.enable_hash_chain   = false;
    cfg.enable_siem         = false;
    return cfg;
}

size_t countLogLines(const std::string& path) {
    std::ifstream f(path);
    size_t n = 0;
    std::string line = {};
    while (std::getline(f, line)) {
        if (!line.empty()) {
          ++n;
        }
    }
    return n;
}
} // namespace

class AuditLoggerIntegrationTest : public ::testing::Test {
protected:
    std::filesystem::path tmp_dir_;
    std::string log_path_ = {};

    void SetUp() override {
        tmp_dir_  = std::filesystem::temp_directory_path() / "auth_anomaly_test";
        std::filesystem::create_directories(tmp_dir_);
        log_path_ = (tmp_dir_ / "anomaly.jsonl").string();
    }

    void TearDown() override {
        std::filesystem::remove_all(tmp_dir_);
    }
};

TEST_F(AuditLoggerIntegrationTest, SetAuditLoggerDoesNotCrash) {
    AuditLogger audit_logger(nullptr, nullptr, makeAnomalyTestConfig(log_path_));

    AuthRateLimitConfig cfg;
    cfg.lockout_failed_attempts              = 3;
    cfg.enable_ip_rate_limiting              = false;
    cfg.enable_user_rate_limiting            = false;
    cfg.enable_credential_stuffing_detection = false;

    AuthRateLimiter rl(cfg);
    EXPECT_NO_THROW(rl.setAuditLogger(&audit_logger));
}

TEST_F(AuditLoggerIntegrationTest, BruteForceTriggersAuditLog) {
    AuditLogger audit_logger(nullptr, nullptr, makeAnomalyTestConfig(log_path_));

    AuthRateLimitConfig cfg;
    cfg.lockout_failed_attempts              = 3;
    cfg.enable_ip_rate_limiting              = false;
    cfg.enable_user_rate_limiting            = false;
    cfg.enable_credential_stuffing_detection = false;

    AuthRateLimiter rl(cfg);
    rl.setAuditLogger(&audit_logger);

    rl.recordFailedAuth("audit_user", "10.0.0.1", "bad_pw");
    rl.recordFailedAuth("audit_user", "10.0.0.1", "bad_pw");
    rl.recordFailedAuth("audit_user", "10.0.0.1", "bad_pw");  // triggers lockout
    audit_logger.flush();

    // Both ACCOUNT_LOCKOUT_TRIGGERED and BRUTE_FORCE_DETECTED events are emitted.
    EXPECT_GE(countLogLines(log_path_), 2u);
}

TEST_F(AuditLoggerIntegrationTest, CredentialStuffingTriggersAuditLog) {
    AuditLogger audit_logger(nullptr, nullptr, makeAnomalyTestConfig(log_path_));

    AuthRateLimitConfig cfg;
    cfg.enable_ip_rate_limiting              = false;
    cfg.enable_user_rate_limiting            = false;
    cfg.enable_account_lockout               = false;
    cfg.enable_credential_stuffing_detection = true;
    cfg.credential_stuffing_user_threshold   = 5;

    AuthRateLimiter rl(cfg);
    rl.setAuditLogger(&audit_logger);

    const std::string ip = "192.168.99.1";
    for (int i = 0; i < 5; ++i) {
        rl.allowAuthAttempt(ip, "victim" + std::to_string(i));
    }
    audit_logger.flush();

    EXPECT_GE(countLogLines(log_path_), 1u);
}

TEST_F(AuditLoggerIntegrationTest, DetachAuditLoggerStopsLogging) {
    AuditLogger audit_logger(nullptr, nullptr, makeAnomalyTestConfig(log_path_));

    AuthRateLimitConfig cfg;
    cfg.lockout_failed_attempts              = 3;
    cfg.enable_ip_rate_limiting              = false;
    cfg.enable_user_rate_limiting            = false;
    cfg.enable_credential_stuffing_detection = false;

    AuthRateLimiter rl(cfg);
    rl.setAuditLogger(&audit_logger);

    rl.recordFailedAuth("u1", "1.1.1.1", "x");
    rl.recordFailedAuth("u1", "1.1.1.1", "x");
    rl.recordFailedAuth("u1", "1.1.1.1", "x");  // triggers lockout + log entry
    audit_logger.flush();

    size_t lines_after_lockout = countLogLines(log_path_);
    EXPECT_GE(lines_after_lockout, 1u);

    // Detach the logger
    rl.setAuditLogger(nullptr);

    // Lock another account – no new log entries expected
    rl.recordFailedAuth("u2", "2.2.2.2", "x");
    rl.recordFailedAuth("u2", "2.2.2.2", "x");
    rl.recordFailedAuth("u2", "2.2.2.2", "x");
    audit_logger.flush();

    EXPECT_EQ(countLogLines(log_path_), lines_after_lockout);
}

// ─────────────────────────────────────────────────────────────────────────────
// Credential-stuffing escalation (persistent breach-count) tests
// ─────────────────────────────────────────────────────────────────────────────

class CredentialStuffingEscalationTest : public ::testing::Test {
protected:
    AuthRateLimitConfig cfg;
    CollectedAuthEvents collected;

    void SetUp() override {
        cfg.enable_ip_rate_limiting              = false;
        cfg.enable_user_rate_limiting            = false;
        cfg.enable_account_lockout               = false;
        cfg.enable_credential_stuffing_detection = true;
        cfg.credential_stuffing_user_threshold   = 3;
        cfg.credential_stuffing_window_seconds   = 60;
        // Use in-memory (no Redis) persistent backend for testing
        cfg.enable_cs_persistent_backend = false;
    }

    // Helper: trigger credential stuffing threshold from a unique IP by sending
    // `threshold` distinct usernames.  Returns the resulting anomaly event.
    //
    // Each call uses its own `ip` argument so IP-level state never needs to be
    // cleared between calls — the breach counter for `user_id` accumulates
    // across calls while the IP windows are independent.  No reset() is used,
    // which lets the per-user breach count grow naturally across successive
    // calls in the same test.
    AuthAnomalyEvent triggerStuffingForUser(AuthRateLimiter& rl,
                                            const std::string& user_id,
                                            const std::string& ip)
    {
        collected.clear();
        rl.setAnomalyCallback([&](const AuthAnomalyEvent& ev) {
            if (ev.type == AuthAnomalyEvent::Type::CREDENTIAL_STUFFING_SUSPECTED) {
                std::lock_guard<std::mutex> l(collected.mtx);
                collected.events.push_back(ev);
            }
        });
        // Use threshold-1 filler users then the target user to cross the threshold.
        // All attempts come from the same `ip` so the IP-level window fires.
        for (size_t i = 0; i + 1 < cfg.credential_stuffing_user_threshold; ++i) {
            rl.recordFailedAuth("filler_" + ip + "_" + std::to_string(i), ip, "bad_pw");
        }
        rl.recordFailedAuth(user_id, ip, "bad_pw");

        // The anomaly callback is invoked synchronously inside recordFailedAuth,
        // so no sleep is needed here.
        std::lock_guard<std::mutex> l(collected.mtx);
        EXPECT_FALSE(collected.events.empty()) << "Expected a stuffing event";
        if (collected.events.empty()) return AuthAnomalyEvent{};
        return collected.events.back();
    }
};

TEST_F(CredentialStuffingEscalationTest, FirstBreachTriggersCaptcha) {
    AuthRateLimiter rl(cfg);
    AuthAnomalyEvent ev = triggerStuffingForUser(rl, "victim_user", "10.0.0.1");
    EXPECT_EQ(ev.cs_outcome, CredentialStuffingOutcome::CAPTCHA_REQUIRED);
    EXPECT_EQ(ev.user_id, "victim_user");
}

TEST_F(CredentialStuffingEscalationTest, SecondBreachTriggersOTP) {
    AuthRateLimiter rl(cfg);
    triggerStuffingForUser(rl, "victim2", "10.1.0.1");  // breach 1
    AuthAnomalyEvent ev = triggerStuffingForUser(rl, "victim2", "10.1.0.2");  // breach 2
    EXPECT_EQ(ev.cs_outcome, CredentialStuffingOutcome::OTP_REQUIRED);
}

TEST_F(CredentialStuffingEscalationTest, ThirdBreachLocksAccount24h) {
    // Enable account lockout so the force-lock can be verified
    cfg.enable_account_lockout = true;
    AuthRateLimiter rl(cfg);

    triggerStuffingForUser(rl, "victim3", "10.2.0.1");  // breach 1
    triggerStuffingForUser(rl, "victim3", "10.2.0.2");  // breach 2
    AuthAnomalyEvent ev = triggerStuffingForUser(rl, "victim3", "10.2.0.3");  // breach 3

    EXPECT_EQ(ev.cs_outcome, CredentialStuffingOutcome::ACCOUNT_LOCKED_24H);
    EXPECT_TRUE(rl.isAccountLocked("victim3"));
}

TEST_F(CredentialStuffingEscalationTest, AnomalyEventCarriesUserIdOnFailedAuth) {
    AuthRateLimiter rl(cfg);
    rl.setAnomalyCallback([&](const AuthAnomalyEvent& ev) {
        std::lock_guard<std::mutex> l(collected.mtx);
        collected.events.push_back(ev);
    });

    const std::string ip      = "10.3.0.1";
    const std::string victim  = "alice_victim";

    for (size_t i = 0; i + 1 < cfg.credential_stuffing_user_threshold; ++i) {
        rl.recordFailedAuth("fill" + std::to_string(i), ip, "bad");
    }
    rl.recordFailedAuth(victim, ip, "bad");

    ASSERT_TRUE(collected.hasType(AuthAnomalyEvent::Type::CREDENTIAL_STUFFING_SUSPECTED));
    // The anomaly event must carry the targeted user_id (not empty)
    bool found = false;
    {
        std::lock_guard<std::mutex> l(collected.mtx);
        for (const auto& ev : collected.events) {
            if (ev.type == AuthAnomalyEvent::Type::CREDENTIAL_STUFFING_SUSPECTED &&
                ev.user_id == victim) {
                found = true;
                break;
            }
        }
    }
    EXPECT_TRUE(found) << "Expected anomaly event with user_id='" << victim << "'";
}

TEST_F(CredentialStuffingEscalationTest, InMemoryBreachCountClearedByReset) {
    // When enable_cs_persistent_backend=false (in-memory), the breach count
    // is stored in the in-process map and is cleared by reset().
    // After a reset the breach count starts fresh.
    AuthRateLimiter rl(cfg);
    AuthAnomalyEvent ev1 = triggerStuffingForUser(rl, "reset_victim", "10.4.0.1");
    EXPECT_EQ(ev1.cs_outcome, CredentialStuffingOutcome::CAPTCHA_REQUIRED);

    // A full reset clears the in-memory breach count
    rl.reset();

    // Re-register callback
    rl.setAnomalyCallback([&](const AuthAnomalyEvent& ev) {
        if (ev.type == AuthAnomalyEvent::Type::CREDENTIAL_STUFFING_SUSPECTED) {
            std::lock_guard<std::mutex> l(collected.mtx);
            collected.events.push_back(ev);
        }
    });

    // Trigger stuffing again – count should restart from 1 → CAPTCHA
    const std::string ip = "10.4.0.2";
    for (size_t i = 0; i + 1 < cfg.credential_stuffing_user_threshold; ++i) {
        rl.recordFailedAuth("f_" + std::to_string(i), ip, "bad");
    }
    rl.recordFailedAuth("reset_victim", ip, "bad");

    {
        std::lock_guard<std::mutex> l(collected.mtx);
        ASSERT_FALSE(collected.events.empty());
        EXPECT_EQ(collected.events.back().cs_outcome,
                  CredentialStuffingOutcome::CAPTCHA_REQUIRED);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AuthMetrics: credential_stuffing_attempts_total
// ─────────────────────────────────────────────────────────────────────────────

TEST(AuthMetricsCredentialStuffingTest, RecordIncreasesInternalCounter) {
    AuthMetrics metrics;
    metrics.recordCredentialStuffingAttempt("user1", "1.2.3.4", "captcha_required");
    metrics.recordCredentialStuffingAttempt("user2", "1.2.3.5", "otp_required");
    metrics.recordCredentialStuffingAttempt("user3", "1.2.3.6", "account_locked_24h");
    EXPECT_EQ(metrics.getCredentialStuffingTotal(), 3u);
}

TEST(AuthMetricsCredentialStuffingTest, MetricsWiredToRateLimiter) {
    AuthRateLimitConfig cfg;
    cfg.enable_ip_rate_limiting              = false;
    cfg.enable_user_rate_limiting            = false;
    cfg.enable_account_lockout               = false;
    cfg.enable_credential_stuffing_detection = true;
    cfg.credential_stuffing_user_threshold   = 3;

    AuthMetrics metrics;
    AuthRateLimiter rl(cfg);
    rl.setMetrics(&metrics);

    const auto before_total = metrics.getCredentialStuffingTotal();

    const std::string ip = "10.9.0.1";
    for (size_t i = 0; i + 1 < cfg.credential_stuffing_user_threshold; ++i) {
        rl.recordFailedAuth("fill" + std::to_string(i), ip, "bad");
    }
    rl.recordFailedAuth("wired_victim", ip, "bad");  // crosses threshold → metric recorded

    EXPECT_GT(metrics.getCredentialStuffingTotal(), before_total);
}
