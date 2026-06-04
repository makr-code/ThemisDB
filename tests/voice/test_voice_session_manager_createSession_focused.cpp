#include <gtest/gtest.h>
#include "voice/voice_session_manager.h"
#include <memory>

using namespace themis::voice;

class VoiceSessionCreateSessionTest : public ::testing::Test {
protected:
    std::unique_ptr<VoiceSessionManager> manager_;

    void SetUp() override {
        SessionTimeoutConfig timeout_config;
        timeout_config.max_session_duration_ms = 3600000;  // 1 hour
        manager_ = std::make_unique<VoiceSessionManager>(timeout_config);
    }
};

// Test 1: Fail-closed guard rejects empty user_id
TEST_F(VoiceSessionCreateSessionTest, CreateSessionFailsClosedForEmptyUserId) {
    VoiceSessionData session = manager_->createSession("", "device_1");
    
    // Empty session_id indicates fail-closed behavior
    EXPECT_TRUE(session.session_id.empty());
    EXPECT_TRUE(session.user_id.empty());
}

// Test 2: Valid session creation succeeds
TEST_F(VoiceSessionCreateSessionTest, CreateSessionAcceptsValidUserId) {
    VoiceSessionData session = manager_->createSession("user_1", "device_1");
    
    // Valid session should have non-empty session_id and user_id
    EXPECT_FALSE(session.session_id.empty());
    EXPECT_EQ(session.user_id, "user_1");
    EXPECT_EQ(session.device_id, "device_1");
    EXPECT_EQ(session.state, SessionState::ACTIVE);
}

// Test 3: Device ID is optional
TEST_F(VoiceSessionCreateSessionTest, CreateSessionWithoutDeviceId) {
    VoiceSessionData session = manager_->createSession("user_1");
    
    // Session should be created successfully without device_id
    EXPECT_FALSE(session.session_id.empty());
    EXPECT_EQ(session.user_id, "user_1");
    EXPECT_TRUE(session.device_id.empty());  // Default empty
}

// Test 4: Multiple fail-closed guards are independent
TEST_F(VoiceSessionCreateSessionTest, FailClosedGuardsAreIndependent) {
    // First call with empty user_id should fail-close
    VoiceSessionData session1 = manager_->createSession("");
    EXPECT_TRUE(session1.session_id.empty());
    
    // Second call with valid user_id should succeed
    VoiceSessionData session2 = manager_->createSession("user_1");
    EXPECT_FALSE(session2.session_id.empty());
    EXPECT_EQ(session2.user_id, "user_1");
    
    // Third call with another valid user_id should create different session
    VoiceSessionData session3 = manager_->createSession("user_2");
    EXPECT_FALSE(session3.session_id.empty());
    EXPECT_EQ(session3.user_id, "user_2");
    EXPECT_NE(session2.session_id, session3.session_id);
}

// Test 5: Created sessions maintain correct timestamps and state
TEST_F(VoiceSessionCreateSessionTest, CreatedSessionsHaveCorrectStateAndTimestamps) {
    VoiceSessionData session = manager_->createSession("user_1", "device_1");
    
    EXPECT_FALSE(session.session_id.empty());
    EXPECT_EQ(session.user_id, "user_1");
    EXPECT_EQ(session.device_id, "device_1");
    EXPECT_EQ(session.state, SessionState::ACTIVE);
    EXPECT_GT(session.created_at_ms, 0);
    EXPECT_GT(session.last_activity_ms, 0);
    EXPECT_GT(session.expires_at_ms, session.created_at_ms);
    EXPECT_EQ(session.total_turns, 0);
    EXPECT_EQ(session.error_count, 0);
}
