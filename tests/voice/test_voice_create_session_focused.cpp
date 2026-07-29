#include <gtest/gtest.h>
#include "voice/voice_session_manager.h"
#include <memory>

using namespace themis::voice;

namespace themis { namespace voice { 

class VoiceSessionManagerCreateSessionTest : public ::testing::Test {
protected:
    std::unique_ptr<VoiceSessionManager> manager_;

    void SetUp() override {
        manager_ = std::make_unique<VoiceSessionManager>();
    }
};

// Test 1: Fail-closed guard rejects empty user_id
TEST_F(VoiceSessionManagerCreateSessionTest, CreateSessionFailsClosedForEmptyUserId) {
    // Attempt to create session with empty user_id
    VoiceSessionData session = manager_->createSession("", "device_1");
    
    // Verify fail-closed: returns session with empty session_id
    EXPECT_TRUE(session.session_id.empty());
    EXPECT_TRUE(session.user_id.empty());
}

// Test 2: Valid user_id creates session with non-empty session_id
TEST_F(VoiceSessionManagerCreateSessionTest, CreateSessionAcceptsValidUserId) {
    // Create session with valid user_id
    VoiceSessionData session = manager_->createSession("user_123", "device_1");
    
    // Verify session was created
    EXPECT_FALSE(session.session_id.empty());
    EXPECT_EQ(session.user_id, "user_123");
    EXPECT_EQ(session.device_id, "device_1");
}

// Test 3: Device_id is correctly stored
TEST_F(VoiceSessionManagerCreateSessionTest, DeviceIdIsCorrectlyStored) {
    // Create session with specific device_id
    VoiceSessionData session = manager_->createSession("user_456", "my_device");
    
    // Verify device_id matches
    EXPECT_EQ(session.device_id, "my_device");
    
    // Retrieve session and verify device_id persists
    auto retrieved = manager_->getSession(session.session_id);
    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->device_id, "my_device");
}

// Test 4: Multiple fail-closed guards are independent
TEST_F(VoiceSessionManagerCreateSessionTest, FailClosedGuardsAreIndependent) {
    // First call with empty user_id should fail-close
    VoiceSessionData session1 = manager_->createSession("", "device_1");
    EXPECT_TRUE(session1.session_id.empty());
    
    // Second call with valid user_id should succeed
    VoiceSessionData session2 = manager_->createSession("user_2", "device_2");
    EXPECT_FALSE(session2.session_id.empty());
    
    // Third call with empty user_id should fail-close again
    VoiceSessionData session3 = manager_->createSession("", "device_3");
    EXPECT_TRUE(session3.session_id.empty());
    
    // Fourth call with valid user_id should succeed again
    VoiceSessionData session4 = manager_->createSession("user_4", "device_4");
    EXPECT_FALSE(session4.session_id.empty());
}

// Test 5: Session is stored in active_cache and retrievable
TEST_F(VoiceSessionManagerCreateSessionTest, CreatedSessionIsRetrievable) {
    // Create session
    VoiceSessionData created = manager_->createSession("user_789", "device_789");
    EXPECT_FALSE(created.session_id.empty());
    
    // Attempt to retrieve session
    auto retrieved = manager_->getSession(created.session_id);
    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->user_id, "user_789");
    EXPECT_EQ(retrieved->device_id, "device_789");
    EXPECT_EQ(retrieved->session_id, created.session_id);
    
    // Verify state is ACTIVE
    EXPECT_EQ(retrieved->state, SessionState::ACTIVE);
}
} } // namespace themis::voice
