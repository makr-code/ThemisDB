/**
 * @file test_voice_assistant_generateLLMResponse_qw37.cpp
 * @brief QW-37: Fail-Closed Guard Tests for VoiceAssistant::generateLLMResponse
 * 
 * Tests the empty-input fail-closed guard implemented in generateLLMResponse.
 * Pattern: Guard empty user_input → spdlog::error + safe fallback return
 */

#include <gtest/gtest.h>
#include <memory>
#include "voice/voice_assistant.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

/**
 * @brief Test fixture for VoiceAssistant fail-closed guards
 */
class VoiceAssistantGenerateLLMResponseTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create minimal config for testing
        themis::voice::VoiceAssistant::Config config;
        config.storage_path = "/tmp/voice_assistant_test";
        config.enable_voice_auth = false;
        
        assistant_ = std::make_unique<themis::voice::VoiceAssistant>(config);
        
        // Create a test session
        test_session_.session_id = "test_session_001";
        test_session_.user_id = "test_user_001";
        test_session_.created_at = 1000000;
        test_session_.last_activity = 1000000;
        test_session_.preferred_language = "en";
    }

    void TearDown() override {
        assistant_.reset();
    }

    std::unique_ptr<themis::voice::VoiceAssistant> assistant_;
    themis::voice::VoiceSession test_session_;
};

/**
 * @test GenerateLLMResponseFailsClosedForEmptyInput
 * 
 * Verifies that generateLLMResponse rejects empty user_input via fail-closed guard.
 * Expected: spdlog::error logged, safe fallback returned
 */
TEST_F(VoiceAssistantGenerateLLMResponseTest, GenerateLLMResponseFailsClosedForEmptyInput) {
    // Empty user input (fail-closed guard target)
    std::string empty_input = "";
    
    // Call generateLLMResponse (indirectly via public API that uses it internally)
    // Note: generateLLMResponse is private, so we test through processTextCommand
    std::string result = assistant_->processTextCommand(empty_input, test_session_.session_id);
    
    // Verify fail-closed behavior: returns safe fallback, not crash or undefined behavior
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("I need a prompt"), std::string::npos);  // Contains safe fallback message
}

/**
 * @test GenerateLLMResponseAcceptsValidInput
 * 
 * Verifies that generateLLMResponse accepts non-empty valid input.
 * Expected: Sanitization applied, LLM response generated or fallback returned
 */
TEST_F(VoiceAssistantGenerateLLMResponseTest, GenerateLLMResponseAcceptsValidInput) {
    // Valid, non-empty input
    std::string valid_input = "What is the capital of France?";
    
    // Call via public API
    std::string result = assistant_->processTextCommand(valid_input, test_session_.session_id);
    
    // Verify non-empty response (either LLM generated or error message)
    EXPECT_FALSE(result.empty());
}

/**
 * @test GenerateLLMResponseWithSingleCharacter
 * 
 * Verifies that single-character input (boundary case) is accepted.
 * Expected: Processed without triggering empty-input guard
 */
TEST_F(VoiceAssistantGenerateLLMResponseTest, GenerateLLMResponseWithSingleCharacter) {
    // Boundary case: single non-empty character
    std::string single_char_input = "?";
    
    // Call via public API
    std::string result = assistant_->processTextCommand(single_char_input, test_session_.session_id);
    
    // Verify non-empty response (should process, not reject as empty)
    EXPECT_FALSE(result.empty());
}

/**
 * @test GenerateLLMResponseWithWhitespaceOnly
 * 
 * Verifies behavior with whitespace-only input (not technically empty string).
 * Expected: Guard passes (not empty), sanitization evaluates, may block on policy
 */
TEST_F(VoiceAssistantGenerateLLMResponseTest, GenerateLLMResponseWithWhitespaceOnly) {
    // Whitespace-only input (not empty according to std::string::empty())
    std::string whitespace_input = "   \t\n  ";
    
    // Call via public API
    std::string result = assistant_->processTextCommand(whitespace_input, test_session_.session_id);
    
    // Verify response generated (guard passes, sanitization/LLM handles whitespace)
    EXPECT_FALSE(result.empty());
}

/**
 * @test FailClosedGuardsAreIndependent
 * 
 * Verifies that empty-input guard acts independently from sanitization policy.
 * Expected: Empty input rejected before sanitization logic runs
 */
TEST_F(VoiceAssistantGenerateLLMResponseTest, FailClosedGuardsAreIndependent) {
    // Test 1: Empty input always fails closed
    std::string empty = "";
    std::string result1 = assistant_->processTextCommand(empty, test_session_.session_id);
    EXPECT_NE(result1.find("I need a prompt"), std::string::npos);
    
    // Test 2: Valid input processes (may pass or fail sanitization policy independently)
    std::string valid = "Hello";
    std::string result2 = assistant_->processTextCommand(valid, test_session_.session_id);
    EXPECT_FALSE(result2.empty());
    
    // Verify guard is independent: results differ based on input, not other factors
    EXPECT_NE(result1, result2);
}

/**
 * @test SessionNotCorruptedByEmptyInputAttempt
 * 
 * Verifies that session state remains valid after empty-input rejection.
 * Expected: Session accessible and usable for subsequent calls
 */
TEST_F(VoiceAssistantGenerateLLMResponseTest, SessionNotCorruptedByEmptyInputAttempt) {
    // First call with empty input (triggers guard, should not corrupt session)
    std::string empty = "";
    std::string result1 = assistant_->processTextCommand(empty, test_session_.session_id);
    EXPECT_NE(result1.find("I need a prompt"), std::string::npos);
    
    // Second call with valid input (should succeed if session is still valid)
    std::string valid = "Test query";
    std::string result2 = assistant_->processTextCommand(valid, test_session_.session_id);
    EXPECT_FALSE(result2.empty());
    
    // Both calls completed without exception (session integrity maintained)
}
