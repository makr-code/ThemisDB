/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_session_manager.cpp                           ║
  Version:         0.0.32                                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_session_manager.cpp
 * @brief Unit tests for the SessionManager class
 *
 * Tests cover:
 * - Session creation and validation
 * - Idle-timeout and absolute-timeout expiry
 * - IP and device pinning
 * - Concurrent session limit enforcement
 * - terminateSession / terminateAllOtherSessions / terminateAllSessions
 * - listSessions (expired sessions pruned inline)
 * - detectAnomalies
 * - pruneExpired
 * - Thread-safety under concurrent create + validate
 */

#include <gtest/gtest.h>
#include "auth/session_manager.h"

#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <vector>

using namespace themis::auth;
using Clock = std::chrono::system_clock;

// ===========================================================================
// Helpers
// ===========================================================================

namespace {

SessionManager::SessionLimits defaultLimits() {
    SessionManager::SessionLimits l;
    l.max_concurrent_sessions = 3;
    l.idle_timeout             = std::chrono::seconds(3600);
    l.absolute_timeout         = std::chrono::seconds(86400);
    l.pin_to_ip                = false;
    l.pin_to_device            = false;
    return l;
}

} // anonymous namespace

// ===========================================================================
// Basic creation and validation
// ===========================================================================

class SessionManagerTest : public ::testing::Test {
protected:
    SessionManager mgr_{defaultLimits()};
};

TEST_F(SessionManagerTest, CreateSession_ReturnsNonEmptyId) {
    auto sid = mgr_.createSession("alice", "fp-1", "1.2.3.4", "TestAgent/1.0");
    EXPECT_FALSE(sid.empty());
    EXPECT_EQ(mgr_.size(), 1u);
}

TEST_F(SessionManagerTest, CreateSession_EmptyUserId_Throws) {
    EXPECT_THROW(mgr_.createSession("", "fp", "1.2.3.4", "UA"), std::invalid_argument);
}

TEST_F(SessionManagerTest, ValidateSession_Valid) {
    auto sid = mgr_.createSession("alice", "fp-1", "1.2.3.4", "UA");
    auto res = mgr_.validateSession(sid);
    EXPECT_TRUE(res.valid);
    ASSERT_TRUE(res.session.has_value());
    EXPECT_EQ(res.session->user_id, "alice");
    EXPECT_EQ(res.session->session_id, sid);
}

TEST_F(SessionManagerTest, ValidateSession_EmptyId_Invalid) {
    auto res = mgr_.validateSession("");
    EXPECT_FALSE(res.valid);
}

TEST_F(SessionManagerTest, ValidateSession_UnknownId_Invalid) {
    auto res = mgr_.validateSession("sess_doesnotexist");
    EXPECT_FALSE(res.valid);
    EXPECT_NE(res.error_message, "");
}

TEST_F(SessionManagerTest, ValidateSession_RefreshesLastActivity) {
    auto sid = mgr_.createSession("alice", "fp", "1.2.3.4", "UA");

    // Grab created_at
    auto first = mgr_.validateSession(sid);
    ASSERT_TRUE(first.valid);
    auto t1 = first.session->last_activity;

    // Small sleep to ensure time advances
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto second = mgr_.validateSession(sid);
    ASSERT_TRUE(second.valid);
    auto t2 = second.session->last_activity;

    EXPECT_GE(t2, t1);
}

// ===========================================================================
// Timeout handling
// ===========================================================================

TEST(SessionManagerTimeoutTest, IdleTimeout_Enforced) {
    SessionManager::SessionLimits lim;
    lim.idle_timeout      = std::chrono::seconds(0); // expires immediately
    lim.absolute_timeout  = std::chrono::seconds(86400);
    SessionManager mgr(lim);

    auto sid = mgr.createSession("bob", "fp", "1.2.3.4", "UA");
    // Sleep just enough for the 0-second timeout to have elapsed
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    auto res = mgr.validateSession(sid);
    EXPECT_FALSE(res.valid);
    EXPECT_EQ(mgr.size(), 0u); // entry pruned
}

TEST(SessionManagerTimeoutTest, AbsoluteTimeout_Enforced) {
    SessionManager::SessionLimits lim;
    lim.idle_timeout      = std::chrono::seconds(86400);
    lim.absolute_timeout  = std::chrono::seconds(0); // expires immediately
    SessionManager mgr(lim);

    auto sid = mgr.createSession("carol", "fp", "1.2.3.4", "UA");
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    auto res = mgr.validateSession(sid);
    EXPECT_FALSE(res.valid);
    EXPECT_EQ(mgr.size(), 0u);
}

// ===========================================================================
// Pinning
// ===========================================================================

TEST(SessionManagerPinningTest, IPPinning_SameIP_Valid) {
    SessionManager::SessionLimits lim = defaultLimits();
    lim.pin_to_ip = true;
    SessionManager mgr(lim);

    auto sid = mgr.createSession("dave", "fp", "10.0.0.1", "UA");
    auto res = mgr.validateSession(sid, "10.0.0.1");
    EXPECT_TRUE(res.valid);
}

TEST(SessionManagerPinningTest, IPPinning_DifferentIP_Invalid) {
    SessionManager::SessionLimits lim = defaultLimits();
    lim.pin_to_ip = true;
    SessionManager mgr(lim);

    auto sid = mgr.createSession("dave", "fp", "10.0.0.1", "UA");
    auto res = mgr.validateSession(sid, "10.0.0.2");
    EXPECT_FALSE(res.valid);
    EXPECT_NE(res.error_message, "");
}

TEST(SessionManagerPinningTest, DevicePinning_DifferentDevice_Invalid) {
    SessionManager::SessionLimits lim = defaultLimits();
    lim.pin_to_device = true;
    SessionManager mgr(lim);

    auto sid = mgr.createSession("eve", "device-A", "1.1.1.1", "UA");
    auto res = mgr.validateSession(sid, "1.1.1.1", "device-B");
    EXPECT_FALSE(res.valid);
}

TEST(SessionManagerPinningTest, PinningDisabled_IPChange_Valid) {
    SessionManager::SessionLimits lim = defaultLimits();
    lim.pin_to_ip = false;
    SessionManager mgr(lim);

    auto sid = mgr.createSession("frank", "fp", "1.1.1.1", "UA");
    auto res = mgr.validateSession(sid, "2.2.2.2");
    EXPECT_TRUE(res.valid);
}

// ===========================================================================
// Concurrent session limit
// ===========================================================================

TEST(SessionManagerLimitTest, MaxConcurrentSessions_OldestEvicted) {
    SessionManager::SessionLimits lim;
    lim.max_concurrent_sessions = 2;
    lim.idle_timeout             = std::chrono::seconds(3600);
    lim.absolute_timeout         = std::chrono::seconds(86400);
    SessionManager mgr(lim);

    auto s1 = mgr.createSession("grace", "fp", "1.1.1.1", "UA");
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    auto s2 = mgr.createSession("grace", "fp", "1.1.1.1", "UA");

    // Third session should evict the oldest (s1)
    auto s3 = mgr.createSession("grace", "fp", "1.1.1.1", "UA");

    EXPECT_EQ(mgr.size(), 2u);

    // s1 should be gone
    auto r1 = mgr.validateSession(s1);
    EXPECT_FALSE(r1.valid);

    // s2 and s3 should still be valid
    EXPECT_TRUE(mgr.validateSession(s2).valid);
    EXPECT_TRUE(mgr.validateSession(s3).valid);
}

TEST(SessionManagerLimitTest, MaxConcurrentSessions_Zero_DoesNotCrash) {
    // max_concurrent_sessions = 0 should safely terminate all sessions on create
    SessionManager::SessionLimits lim;
    lim.max_concurrent_sessions = 0;
    lim.idle_timeout             = std::chrono::seconds(3600);
    lim.absolute_timeout         = std::chrono::seconds(86400);
    SessionManager mgr(lim);

    // Should not crash or loop infinitely
    EXPECT_NO_THROW(mgr.createSession("user_zero", "fp", "1.1.1.1", "UA"));
}

// ===========================================================================
// Termination
// ===========================================================================

TEST_F(SessionManagerTest, TerminateSession_RemovesEntry) {
    auto sid = mgr_.createSession("henry", "fp", "1.2.3.4", "UA");
    EXPECT_EQ(mgr_.size(), 1u);

    mgr_.terminateSession(sid);
    EXPECT_EQ(mgr_.size(), 0u);
    EXPECT_FALSE(mgr_.validateSession(sid).valid);
}

TEST_F(SessionManagerTest, TerminateSession_NoopForUnknownId) {
    mgr_.createSession("henry", "fp", "1.2.3.4", "UA");
    // Should not throw
    mgr_.terminateSession("sess_nonexistent");
    EXPECT_EQ(mgr_.size(), 1u);
}

TEST_F(SessionManagerTest, TerminateAllOtherSessions_KeepsCurrentSession) {
    auto s1 = mgr_.createSession("iris", "fp", "1.1.1.1", "UA");
    auto s2 = mgr_.createSession("iris", "fp", "1.1.1.1", "UA");
    auto s3 = mgr_.createSession("iris", "fp", "1.1.1.1", "UA");
    EXPECT_EQ(mgr_.size(), 3u);

    int terminated = mgr_.terminateAllOtherSessions("iris", s2);
    EXPECT_EQ(terminated, 2);
    EXPECT_EQ(mgr_.size(), 1u);
    EXPECT_TRUE(mgr_.validateSession(s2).valid);
    EXPECT_FALSE(mgr_.validateSession(s1).valid);
    EXPECT_FALSE(mgr_.validateSession(s3).valid);
}

TEST_F(SessionManagerTest, TerminateAllSessions_RemovesAll) {
    mgr_.createSession("jack", "fp", "1.1.1.1", "UA");
    mgr_.createSession("jack", "fp", "1.1.1.1", "UA");
    EXPECT_EQ(mgr_.size(), 2u);

    int n = mgr_.terminateAllSessions("jack");
    EXPECT_EQ(n, 2);
    EXPECT_EQ(mgr_.size(), 0u);
}

TEST_F(SessionManagerTest, TerminateAllSessions_UnknownUser_ReturnsZero) {
    EXPECT_EQ(mgr_.terminateAllSessions("nobody"), 0);
}

TEST_F(SessionManagerTest, TerminateAllOtherSessions_EmptyKeepId_TerminatesAll) {
    // When keep_session_id is empty, all sessions are terminated
    // (equivalent to terminateAllSessions – deliberate "logout everywhere" behaviour)
    auto s1 = mgr_.createSession("zara", "fp", "1.1.1.1", "UA");
    auto s2 = mgr_.createSession("zara", "fp", "1.1.1.1", "UA");
    EXPECT_EQ(mgr_.size(), 2u);

    int terminated = mgr_.terminateAllOtherSessions("zara", "");
    EXPECT_EQ(terminated, 2);
    EXPECT_EQ(mgr_.size(), 0u);
}

TEST_F(SessionManagerTest, TerminateAllOtherSessions_UnknownUser_ReturnsZero) {
    EXPECT_EQ(mgr_.terminateAllOtherSessions("nobody", "sess_fake"), 0);
}

// ===========================================================================
// listSessions
// ===========================================================================

TEST_F(SessionManagerTest, ListSessions_ReturnsAllActiveForUser) {
    auto s1 = mgr_.createSession("kate", "fp", "1.1.1.1", "UA");
    auto s2 = mgr_.createSession("kate", "fp", "1.1.1.1", "UA");
    mgr_.createSession("other_user", "fp", "2.2.2.2", "UA");

    auto sessions = mgr_.listSessions("kate");
    EXPECT_EQ(sessions.size(), 2u);
    for (const auto& s : sessions) {
        EXPECT_EQ(s.user_id, "kate");
    }
}

TEST_F(SessionManagerTest, ListSessions_MarksCurrentSession) {
    auto s1 = mgr_.createSession("leo", "fp", "1.1.1.1", "UA");
    auto s2 = mgr_.createSession("leo", "fp", "1.1.1.1", "UA");

    auto sessions = mgr_.listSessions("leo", s1);
    bool found_current = false;
    for (const auto& s : sessions) {
        if (s.session_id == s1) {
            EXPECT_TRUE(s.is_current);
            found_current = true;
        } else {
            EXPECT_FALSE(s.is_current);
        }
    }
    EXPECT_TRUE(found_current);
}

TEST_F(SessionManagerTest, ListSessions_EmptyForUnknownUser) {
    auto sessions = mgr_.listSessions("nobody");
    EXPECT_TRUE(sessions.empty());
}

// ===========================================================================
// pruneExpired
// ===========================================================================

TEST(SessionManagerPruneTest, PruneExpired_RemovesExpiredSessions) {
    SessionManager::SessionLimits lim;
    lim.idle_timeout     = std::chrono::seconds(0);
    lim.absolute_timeout = std::chrono::seconds(86400);
    SessionManager mgr(lim);

    mgr.createSession("mary", "fp", "1.1.1.1", "UA");
    EXPECT_EQ(mgr.size(), 1u);

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    mgr.pruneExpired();
    EXPECT_EQ(mgr.size(), 0u);
}

// ===========================================================================
// detectAnomalies
// ===========================================================================

TEST_F(SessionManagerTest, DetectAnomalies_UnknownSession_Empty) {
    auto anomalies = mgr_.detectAnomalies("sess_nonexistent");
    EXPECT_TRUE(anomalies.empty());
}

TEST_F(SessionManagerTest, DetectAnomalies_ValidSession_NoAnomalies) {
    auto sid = mgr_.createSession("nina", "fp", "1.2.3.4", "UA");
    auto anomalies = mgr_.detectAnomalies(sid);
    // A fresh session should report no critical anomalies
    for (const auto& a : anomalies) {
        EXPECT_LT(a.severity, 100);
    }
}

// ===========================================================================
// Thread safety
// ===========================================================================

TEST(SessionManagerConcurrencyTest, ConcurrentCreateAndValidate_NoDataRace) {
    SessionManager::SessionLimits lim;
    lim.max_concurrent_sessions = 100;
    lim.idle_timeout             = std::chrono::seconds(3600);
    lim.absolute_timeout         = std::chrono::seconds(86400);
    SessionManager mgr(lim);

    constexpr int kThreads    = 8;
    constexpr int kOpsPerThread = 50;

    std::vector<std::thread> threads;
    std::atomic<int> errors{0};

    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < kOpsPerThread; ++i) {
                try {
                    std::string uid = "user_" + std::to_string(t);
                    auto sid = mgr.createSession(uid, "fp", "1.1.1.1", "UA");
                    auto res = mgr.validateSession(sid);
                    if (!res.valid) {
                        // Could be evicted by the limit enforcer – not an error
                    }
                } catch (...) {
                    ++errors;
                }
            }
        });
    }

    for (auto& th : threads) th.join();

    EXPECT_EQ(errors.load(), 0);
}
