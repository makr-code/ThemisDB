#include <gtest/gtest.h>
#include "voice/voice_session_manager.h"
#include <memory>

using namespace themis::voice;

namespace themis { namespace voice { 

class VoiceSessionManagerTest : public ::testing::Test {
protected:
    std::unique_ptr<VoiceSessionManager> manager_;

    void SetUp() override {
        // Create session manager with default config and null backend (sufficient for guard testing)
        manager_ = std::make_unique<VoiceSessionManager>();

        // Create a valid session to use in tests
        auto session = manager_->createSession("test_user", "test_device");
        test_session_id_ = session.session_id;
    }

    std::string test_session_id_ = {};
};

// Test 1: Fail-closed guard rejects empty user_msg
TEST_F(VoiceSessionManagerTest, AddConversationTurnFailsClosedForEmptyUserMsg) {
    // Attempt to add conversation turn with empty user_msg
    bool result = manager_->addConversationTurn(test_session_id_, "", "assistant response");
    
    // Verify fail-closed: returns false
    EXPECT_FALSE(result);
}

// Test 2: Fail-closed guard rejects empty assistant_msg
TEST_F(VoiceSessionManagerTest, AddConversationTurnFailsClosedForEmptyAssistantMsg) {
    // Attempt to add conversation turn with empty assistant_msg
    bool result = manager_->addConversationTurn(test_session_id_, "user question", "");
    
    // Verify fail-closed: returns false
    EXPECT_FALSE(result);
}

// Test 3: Valid messages are accepted and turn is added
TEST_F(VoiceSessionManagerTest, AddConversationTurnAcceptsValidMessages) {
    // Add conversation turn with both messages non-empty
    bool result = manager_->addConversationTurn(test_session_id_, "hello", "hi there");
    
    // Verify turn was added
    EXPECT_TRUE(result);
    
    // Verify session has the conversation recorded
    auto session = manager_->getSession(test_session_id_);
    EXPECT_TRUE(session.has_value());
    EXPECT_GT(session->total_turns, 0);
}

// Test 4: Multiple fail-closed guards are independent
TEST_F(VoiceSessionManagerTest, FailClosedGuardsAreIndependent) {
    // First call with empty user_msg should fail-close
    bool result1 = manager_->addConversationTurn(test_session_id_, "", "msg");
    EXPECT_FALSE(result1);
    
    // Second call with empty assistant_msg should also fail-close
    bool result2 = manager_->addConversationTurn(test_session_id_, "msg", "");
    EXPECT_FALSE(result2);
    
    // Third call with both empty should fail-close
    bool result3 = manager_->addConversationTurn(test_session_id_, "", "");
    EXPECT_FALSE(result3);
    
    // Fourth call with valid messages should succeed
    bool result4 = manager_->addConversationTurn(test_session_id_, "hello", "hi");
    EXPECT_TRUE(result4);
}

// Test 5: Conversation history grows only with valid turns
TEST_F(VoiceSessionManagerTest, ConversationHistoryGrowsOnlyWithValidTurns) {
    // Get initial turn count
    auto session1 = manager_->getSession(test_session_id_);
    EXPECT_TRUE(session1.has_value());
    uint32_t initial_turns = session1->total_turns;
    
    // Try to add with empty user_msg (should fail-close)
    bool result1 = manager_->addConversationTurn(test_session_id_, "", "response");
    EXPECT_FALSE(result1);
    
    // Verify turn count unchanged
    auto session2 = manager_->getSession(test_session_id_);
    EXPECT_TRUE(session2.has_value());
    EXPECT_EQ(session2->total_turns, initial_turns);
    
    // Add valid turn
    bool result2 = manager_->addConversationTurn(test_session_id_, "question", "response");
    EXPECT_TRUE(result2);
    
    // Verify turn count increased
    auto session3 = manager_->getSession(test_session_id_);
    EXPECT_TRUE(session3.has_value());
    EXPECT_GT(session3->total_turns, initial_turns);
}
} } // namespace themis::voice
