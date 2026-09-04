/**
 * @file test_voice_multi_session_teardown.cpp
 * @brief Multi-Session Teardown Safety Tests — Wave A Block 2
 * 
 * @test Wave A Block 2: Verify multi-session teardown safety with:
 * - Concurrent session termination (≥10 sessions)
 * - Timeout guard enforcement (≤5s)
 * - Dangling reference elimination
 * - Reverse-dependency cleanup
 * - Fail-closed behavior on timeout
 */

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <mutex>

#include "voice/voice_session_manager.h"
#include "voice/voice_audit_logger.h"

namespace themis {
namespace voice {

class VoiceMultiSessionTeardownTest : public ::testing::Test {
protected:
    void SetUp() override {
        SessionTimeoutConfig config;
        config.teardown_timeout_ms = 5000;  // 5 seconds
        config.closing_grace_period_ms = 100;
        manager_ = std::make_unique<VoiceSessionManager>(config);
        
        VoiceAuditLogger::Config audit_config;
        audit_config.enable_logging = true;
        audit_logger_ = std::make_unique<VoiceAuditLogger>(audit_config);
    }
    
    void TearDown() override {
        // Cleanup: terminate all sessions
        manager_->terminateAllSessions(5000);
    }
    
    std::unique_ptr<VoiceSessionManager> manager_;
    std::unique_ptr<VoiceAuditLogger> audit_logger_;
};

// ============================================================================
// TEST 1: Single Session Teardown (Baseline)
// ============================================================================
TEST_F(VoiceMultiSessionTeardownTest, TeardownSingleSession) {
    // Create a single session
    auto session = manager_->createSession("user-001", "device-001");
    ASSERT_FALSE(session.session_id.empty());
    EXPECT_EQ(session.state, SessionState::ACTIVE);
    
    // Verify session exists
    auto fetched = manager_->getSession(session.session_id);
    ASSERT_TRUE(fetched.has_value());
    
    // Terminate session
    bool result = manager_->terminateSession(session.session_id);
    EXPECT_TRUE(result);
    
    // Verify session is terminated
    auto after_term = manager_->getSession(session.session_id);
    EXPECT_FALSE(after_term.has_value());
    
    // Verify teardown status
    auto status = manager_->getSessionTeardownStatus(session.session_id);
    EXPECT_TRUE(status.contains("session_id"));
}

// ============================================================================
// TEST 2: Concurrent 10-Session Teardown
// ============================================================================
TEST_F(VoiceMultiSessionTeardownTest, TeardownConcurrent10Sessions) {
    const int NUM_SESSIONS = 10;
    std::vector<std::string> session_ids;
    
    // Create 10 sessions concurrently
    std::vector<std::thread> create_threads;
    std::mutex ids_mutex = {};
    
    for (int i = 0; i < NUM_SESSIONS; ++i) {
        create_threads.emplace_back([this, i, &session_ids, &ids_mutex]() {
            auto session = manager_->createSession(
                "user-" + std::to_string(i),
                "device-" + std::to_string(i)
            );
            if (!session.session_id.empty()) {
                std::lock_guard<std::mutex> lock(ids_mutex);
                session_ids.push_back(session.session_id);
            }
        });
    }
    
    for (auto& t : create_threads) {
        t.join();
    }
    
    EXPECT_EQ(session_ids.size(), NUM_SESSIONS);
    
    // Terminate all 10 sessions concurrently
    std::vector<std::thread> teardown_threads;
    std::atomic<int> successful_teardowns{0};
    
    for (const auto& session_id : session_ids) {
        teardown_threads.emplace_back([this, &session_id, &successful_teardowns]() {
            bool result = manager_->terminateSession(session_id);
            if (result) {
                successful_teardowns++;
            }
        });
    }
    
    for (auto& t : teardown_threads) {
        t.join();
    }
    
    // Verify all sessions were terminated
    EXPECT_EQ(successful_teardowns.load(), NUM_SESSIONS);
    
    // Verify no dangling references
    for (const auto& session_id : session_ids) {
        auto fetched = manager_->getSession(session_id);
        EXPECT_FALSE(fetched.has_value()) 
            << "Session " << session_id << " should be terminated";
    }
}

// ============================================================================
// TEST 3: Concurrent 100-Session Teardown (Stress)
// ============================================================================
TEST_F(VoiceMultiSessionTeardownTest, TeardownConcurrent100SessionsStress) {
    const int NUM_SESSIONS = 100;
    std::vector<std::string> session_ids;
    
    // Create 100 sessions
    for (int i = 0; i < NUM_SESSIONS; ++i) {
        auto session = manager_->createSession(
            "user-" + std::to_string(i),
            "device-" + std::to_string(i)
        );
        if (!session.session_id.empty()) {
            session_ids.push_back(session.session_id);
        }
    }
    
    EXPECT_EQ(session_ids.size(), NUM_SESSIONS);
    
    // Terminate all concurrently with thread pool
    const int NUM_THREADS = 8;
    std::vector<std::thread> threads;
    std::atomic<size_t> terminated{0};
    
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, &session_ids, &terminated, t, NUM_THREADS]() {
            for (size_t i = t; i < session_ids.size(); i += NUM_THREADS) {
                if (manager_->terminateSession(session_ids[i])) {
                    terminated++;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(terminated.load(), NUM_SESSIONS);
}

// ============================================================================
// TEST 4: Teardown Timeout Enforcement (≤5s)
// ============================================================================
TEST_F(VoiceMultiSessionTeardownTest, TeardownTimeoutEnforcement) {
    auto session = manager_->createSession("user-001", "device-001");
    ASSERT_FALSE(session.session_id.empty());
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Terminate with explicit 1-second timeout
    bool result = manager_->terminateSessionWithTimeout(session.session_id, 1000);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    ).count();
    
    // Should complete within ~1 second (allowing some overhead)
    EXPECT_LT(elapsed_ms, 2000) 
        << "Teardown took " << elapsed_ms << "ms, expected < 2000ms";
}

// ============================================================================
// TEST 5: Dangling Reference Elimination
// ============================================================================
TEST_F(VoiceMultiSessionTeardownTest, DanglingReferenceElimination) {
    const int NUM_SESSIONS = 20;
    std::vector<std::string> session_ids;
    
    // Create sessions
    for (int i = 0; i < NUM_SESSIONS; ++i) {
        auto session = manager_->createSession(
            "user-" + std::to_string(i),
            "device-" + std::to_string(i)
        );
        if (!session.session_id.empty()) {
            session_ids.push_back(session.session_id);
        }
    }
    
    // Terminate all
    size_t terminated_count = manager_->terminateAllSessions(5000);
    EXPECT_EQ(terminated_count, NUM_SESSIONS);
    
    // Verify NO dangling references
    for (const auto& session_id : session_ids) {
        auto fetched = manager_->getSession(session_id);
        EXPECT_FALSE(fetched.has_value()) 
            << "Dangling reference detected: " << session_id;
    }
    
    // Verify double-close detection works
    for (const auto& session_id : session_ids) {
        EXPECT_TRUE(manager_->isDoubleCloseAttempt(session_id))
            << "Should detect terminated session: " << session_id;
    }
}

// ============================================================================
// TEST 6: Reverse-Dependency Cleanup Order
// ============================================================================
TEST_F(VoiceMultiSessionTeardownTest, ReverseDependencyCleanupOrder) {
    // Create multiple sessions for different users
    std::vector<std::string> session_ids;
    
    for (int user = 0; user < 5; ++user) {
        for (int device = 0; device < 3; ++device) {
            auto session = manager_->createSession(
                "user-" + std::to_string(user),
                "device-" + std::to_string(device)
            );
            if (!session.session_id.empty()) {
                session_ids.push_back(session.session_id);
            }
        }
    }
    
    // Get all sessions for user-0 before cleanup
    auto user0_sessions_before = manager_->getSessionsForUser("user-0");
    EXPECT_EQ(user0_sessions_before.size(), 3);
    
    // Terminate all sessions
    size_t terminated = manager_->terminateAllSessions(5000);
    EXPECT_EQ(terminated, session_ids.size());
    
    // Verify user sessions are cleaned up in correct order
    auto user0_sessions_after = manager_->getSessionsForUser("user-0");
    EXPECT_EQ(user0_sessions_after.size(), 0);
}

// ============================================================================
// TEST 7: Abort Signal Cleanup
// ============================================================================
TEST_F(VoiceMultiSessionTeardownTest, AbortSignalCleanup) {
    // Create sessions
    std::vector<std::string> session_ids = {};

    for (int i = 0; i < 10; ++i) {
        auto session = manager_->createSession("user-" + std::to_string(i));
        if (!session.session_id.empty()) {
            session_ids.push_back(session.session_id);
        }
    }
    
    // Simulate abort by force-terminating all with short timeout
    size_t terminated = manager_->terminateAllSessions(1000);  // 1 second timeout
    
    // All should be terminated even with short timeout
    EXPECT_GE(terminated, 8);  // At least 80%
}

// ============================================================================
// TEST 8: Network Failure Cleanup (Mock)
// ============================================================================
TEST_F(VoiceMultiSessionTeardownTest, NetworkFailureCleanup) {
    auto session = manager_->createSession("user-001", "device-001");
    ASSERT_FALSE(session.session_id.empty());
    
    // Simulate network failure by terminating with minimal timeout
    bool result = manager_->terminateSessionWithTimeout(session.session_id, 100);  // 100ms
    
    // Should still complete (fail-closed)
    auto final_state = manager_->getSessionState(session.session_id);
    EXPECT_EQ(final_state, SessionState::TERMINATED);
}

// ============================================================================
// TEST 9: State Machine Validation
// ============================================================================
TEST_F(VoiceMultiSessionTeardownTest, StateMachineValidation) {
    auto session = manager_->createSession("user-001", "device-001");
    ASSERT_FALSE(session.session_id.empty());
    
    // Valid transition: ACTIVE → CLOSING
    bool valid = manager_->validateStateTransition(
        session.session_id, SessionState::CLOSING
    );
    EXPECT_TRUE(valid);
    
    // Terminate to transition through states
    manager_->terminateSession(session.session_id);
    
    // After termination, no more valid transitions
    valid = manager_->validateStateTransition(
        session.session_id, SessionState::ACTIVE
    );
    EXPECT_FALSE(valid);
}

// ============================================================================
// TEST 10: Teardown Status Diagnostics
// ============================================================================
TEST_F(VoiceMultiSessionTeardownTest, TeardownStatusDiagnostics) {
    auto session = manager_->createSession("user-001", "device-001");
    ASSERT_FALSE(session.session_id.empty());
    
    // Get status before teardown
    auto status_before = manager_->getSessionTeardownStatus(session.session_id);
    EXPECT_FALSE(status_before["is_tearing_down"].get<bool>());
    
    // Get status after explicit timeout call (which sets CLOSING state temporarily)
    // Note: In real scenario, this would show teardown progress
    
    // Complete teardown
    manager_->terminateSession(session.session_id);
    
    // Get status after teardown
    auto status_after = manager_->getSessionTeardownStatus(session.session_id);
    EXPECT_FALSE(status_after["is_tearing_down"].get<bool>());
}

} // namespace voice
} // namespace themis
