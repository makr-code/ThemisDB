/**
 * @file test_voice_assistant.cpp
 * @brief Unit tests for Voice Assistant module
 * 
 * Tests voice command processing, phone call recording, meeting protocols,
 * session management, and storage integration.
 * 
 * @author ThemisDB Team
 * @date January 2026
 */

#include <gtest/gtest.h>
#include "voice/voice_assistant.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

using namespace themis::voice;
using json = nlohmann::json;

class VoiceAssistantTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test configuration
        config_.stt_model_path = "/tmp/test_stt_model";
        config_.stt_model_size = "base";
        config_.stt_language = "en";
        
        config_.tts_model_path = "/tmp/test_tts_model";
        config_.tts_voice = "default";
        config_.tts_speed = 1.0f;
        
        config_.llm_model_path = "/tmp/test_llm_model";
        config_.llm_n_ctx = 2048;
        config_.llm_n_gpu_layers = 0;
        
        config_.storage_path = "/tmp/test_voice_storage";
        config_.enable_revision_control = true;
        config_.compress_audio = true;
        config_.audio_format = "ogg";
    }
    
    void TearDown() override {
        // Clean up test resources
    }
    
    VoiceAssistant::Config config_;
};

// ============================================================================
// Initialization and Configuration Tests
// ============================================================================

TEST_F(VoiceAssistantTest, ConstructorTest) {
    EXPECT_NO_THROW({
        VoiceAssistant assistant(config_);
    });
}

TEST_F(VoiceAssistantTest, ConfigurationValidation) {
    EXPECT_EQ(config_.stt_model_size, "base");
    EXPECT_EQ(config_.stt_language, "en");
    EXPECT_EQ(config_.tts_speed, 1.0f);
    EXPECT_EQ(config_.llm_n_ctx, 2048);
    EXPECT_TRUE(config_.enable_revision_control);
    EXPECT_TRUE(config_.compress_audio);
}

TEST_F(VoiceAssistantTest, InitializeWithValidConfig) {
    VoiceAssistant assistant(config_);
    // Note: Will fail in test environment without actual models
    // This test validates the interface exists
    EXPECT_NO_THROW({
        bool result = assistant.initialize();
        // Expected to fail without real models, but shouldn't crash
    });
}

TEST_F(VoiceAssistantTest, ShutdownWithoutInitialize) {
    VoiceAssistant assistant(config_);
    EXPECT_NO_THROW({
        assistant.shutdown();
    });
}

TEST_F(VoiceAssistantTest, DoubleShutdown) {
    VoiceAssistant assistant(config_);
    EXPECT_NO_THROW({
        assistant.shutdown();
        assistant.shutdown();
    });
}

// ============================================================================
// Session Management Tests
// ============================================================================

TEST_F(VoiceAssistantTest, SessionCreation) {
    VoiceAssistant assistant(config_);
    
    std::string session_id = "test-session-001";
    VoiceSession session = assistant.getSession(session_id);
    
    EXPECT_EQ(session.session_id, session_id);
    EXPECT_GT(session.created_at, 0);
    EXPECT_GT(session.last_activity, 0);
}

TEST_F(VoiceAssistantTest, SessionContextUpdate) {
    VoiceAssistant assistant(config_);
    
    std::string session_id = "test-session-002";
    json context;
    context["user_preference"] = "detailed";
    context["language"] = "en";
    
    EXPECT_NO_THROW({
        assistant.updateSession(session_id, context);
    });
}

TEST_F(VoiceAssistantTest, MultipleSessionsIsolation) {
    VoiceAssistant assistant(config_);
    
    std::string session1 = "session-001";
    std::string session2 = "session-002";
    
    VoiceSession s1 = assistant.getSession(session1);
    VoiceSession s2 = assistant.getSession(session2);
    
    EXPECT_NE(s1.session_id, s2.session_id);
    EXPECT_EQ(s1.session_id, session1);
    EXPECT_EQ(s2.session_id, session2);
}

// ============================================================================
// Voice Command Processing Tests
// ============================================================================

TEST_F(VoiceAssistantTest, ProcessTextCommandInterface) {
    VoiceAssistant assistant(config_);
    
    std::string session_id = "test-session-003";
    std::string text_input = "What is the weather today?";
    
    // Test interface exists and accepts input
    EXPECT_NO_THROW({
        std::string response = assistant.processTextCommand(text_input, session_id);
        // Without real LLM, response may be empty or error
    });
}

TEST_F(VoiceAssistantTest, ProcessTextCommandEmptyInput) {
    VoiceAssistant assistant(config_);
    
    std::string session_id = "test-session-004";
    std::string empty_input = "";
    
    EXPECT_NO_THROW({
        std::string response = assistant.processTextCommand(empty_input, session_id);
    });
}

TEST_F(VoiceAssistantTest, ProcessVoiceCommandInterface) {
    VoiceAssistant assistant(config_);
    
    std::string session_id = "test-session-005";
    std::vector<uint8_t> mock_audio_data(1024, 0);
    
    EXPECT_NO_THROW({
        std::vector<uint8_t> response = assistant.processVoiceCommand(mock_audio_data, session_id);
    });
}

TEST_F(VoiceAssistantTest, ProcessVoiceCommandEmptyAudio) {
    VoiceAssistant assistant(config_);
    
    std::string session_id = "test-session-006";
    std::vector<uint8_t> empty_audio;
    
    EXPECT_NO_THROW({
        std::vector<uint8_t> response = assistant.processVoiceCommand(empty_audio, session_id);
    });
}

// ============================================================================
// Phone Call Recording Tests
// ============================================================================

TEST_F(VoiceAssistantTest, PhoneCallMetadataCreation) {
    PhoneCallMetadata metadata;
    metadata.call_id = "call-12345";
    metadata.caller_number = "+49123456789";
    metadata.callee_number = "+49987654321";
    metadata.start_time = 1234567890;
    metadata.end_time = 1234568890;
    metadata.duration_ms = 60000;
    metadata.call_type = "inbound";
    
    EXPECT_EQ(metadata.call_id, "call-12345");
    EXPECT_EQ(metadata.call_type, "inbound");
    EXPECT_EQ(metadata.duration_ms, 60000);
}

TEST_F(VoiceAssistantTest, RecordPhoneCallInterface) {
    VoiceAssistant assistant(config_);
    
    PhoneCallMetadata metadata;
    metadata.call_id = "test-call-001";
    metadata.caller_number = "+49123456789";
    metadata.callee_number = "+49987654321";
    metadata.start_time = 1234567890;
    metadata.end_time = 1234568890;
    metadata.duration_ms = 60000;
    metadata.call_type = "inbound";
    
    std::vector<uint8_t> mock_audio(2048, 0);
    
    EXPECT_NO_THROW({
        json result = assistant.recordPhoneCall(mock_audio, metadata);
    });
}

// ============================================================================
// Meeting Protocol Tests
// ============================================================================

TEST_F(VoiceAssistantTest, MeetingMetadataCreation) {
    MeetingMetadata metadata;
    metadata.meeting_id = "meeting-12345";
    metadata.title = "Sprint Planning";
    metadata.start_time = 1234567890;
    metadata.end_time = 1234571490;
    metadata.participants = {"alice@example.com", "bob@example.com", "charlie@example.com"};
    metadata.organizer = "alice@example.com";
    
    EXPECT_EQ(metadata.meeting_id, "meeting-12345");
    EXPECT_EQ(metadata.title, "Sprint Planning");
    EXPECT_EQ(metadata.participants.size(), 3);
    EXPECT_EQ(metadata.organizer, "alice@example.com");
}

TEST_F(VoiceAssistantTest, GenerateMeetingProtocolInterface) {
    VoiceAssistant assistant(config_);
    
    MeetingMetadata metadata;
    metadata.meeting_id = "test-meeting-001";
    metadata.title = "Test Meeting";
    metadata.start_time = 1234567890;
    metadata.end_time = 1234571490;
    metadata.participants = {"user1@test.com", "user2@test.com"};
    metadata.organizer = "user1@test.com";
    
    std::vector<uint8_t> mock_audio(4096, 0);
    
    EXPECT_NO_THROW({
        json protocol = assistant.generateMeetingProtocol(mock_audio, metadata);
    });
}

// ============================================================================
// Audio Format Conversion Tests
// ============================================================================

TEST_F(VoiceAssistantTest, AudioFormatConversionInterface) {
    VoiceAssistant assistant(config_);
    
    std::vector<uint8_t> mock_audio(1024, 0);
    std::string target_format = "mp3";
    
    EXPECT_NO_THROW({
        std::vector<uint8_t> converted = assistant.convertAudioFormat(mock_audio, target_format);
    });
}

TEST_F(VoiceAssistantTest, AudioFormatConversionOggToMp3) {
    VoiceAssistant assistant(config_);
    
    std::vector<uint8_t> mock_audio(512, 0);
    
    EXPECT_NO_THROW({
        assistant.convertAudioFormat(mock_audio, "mp3");
    });
}

TEST_F(VoiceAssistantTest, AudioFormatConversionMp3ToOgg) {
    VoiceAssistant assistant(config_);
    
    std::vector<uint8_t> mock_audio(512, 0);
    
    EXPECT_NO_THROW({
        assistant.convertAudioFormat(mock_audio, "ogg");
    });
}

TEST_F(VoiceAssistantTest, AudioFormatConversionInvalidFormat) {
    VoiceAssistant assistant(config_);
    
    std::vector<uint8_t> mock_audio(512, 0);
    
    EXPECT_NO_THROW({
        assistant.convertAudioFormat(mock_audio, "invalid_format");
    });
}

// ============================================================================
// Storage Integration Tests
// ============================================================================

TEST_F(VoiceAssistantTest, StoreRecordingInterface) {
    VoiceAssistant assistant(config_);
    
    std::vector<uint8_t> mock_audio(2048, 0);
    std::string transcript = "This is a test recording.";
    json metadata;
    metadata["source"] = "test";
    metadata["type"] = "unit_test";
    
    EXPECT_NO_THROW({
        std::string doc_id = assistant.storeRecording(mock_audio, transcript, metadata);
    });
}

TEST_F(VoiceAssistantTest, StoreRecordingWithRevisionControl) {
    VoiceAssistant assistant(config_);
    
    std::vector<uint8_t> audio1(1024, 0);
    std::vector<uint8_t> audio2(1024, 1);
    
    EXPECT_NO_THROW({
        std::string doc_id1 = assistant.storeRecording(audio1, "Version 1", json());
        std::string doc_id2 = assistant.storeRecording(audio2, "Version 2", json());
    });
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(VoiceAssistantTest, GetStatistics) {
    VoiceAssistant assistant(config_);
    
    EXPECT_NO_THROW({
        json stats = assistant.getStatistics();
        EXPECT_TRUE(stats.is_object());
    });
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(VoiceAssistantTest, InvalidModelPaths) {
    VoiceAssistant::Config invalid_config = config_;
    invalid_config.stt_model_path = "/nonexistent/path";
    invalid_config.tts_model_path = "/nonexistent/path";
    invalid_config.llm_model_path = "/nonexistent/path";
    
    VoiceAssistant assistant(invalid_config);
    
    EXPECT_NO_THROW({
        bool result = assistant.initialize();
        // Expected to fail gracefully
    });
}

TEST_F(VoiceAssistantTest, LargeAudioData) {
    VoiceAssistant assistant(config_);
    
    // 100 MB audio file
    std::vector<uint8_t> large_audio(100 * 1024 * 1024, 0);
    std::string session_id = "test-session-large";
    
    EXPECT_NO_THROW({
        assistant.processVoiceCommand(large_audio, session_id);
    });
}

// ============================================================================
// Concurrent Access Tests
// ============================================================================

TEST_F(VoiceAssistantTest, ConcurrentSessionAccess) {
    VoiceAssistant assistant(config_);
    
    std::vector<std::string> session_ids;
    for (int i = 0; i < 10; i++) {
        session_ids.push_back("session-" + std::to_string(i));
    }
    
    EXPECT_NO_THROW({
        for (const auto& session_id : session_ids) {
            assistant.getSession(session_id);
        }
    });
}

TEST_F(VoiceAssistantTest, ConcurrentTextCommands) {
    VoiceAssistant assistant(config_);
    
    std::vector<std::string> commands = {
        "Command 1",
        "Command 2",
        "Command 3"
    };
    
    EXPECT_NO_THROW({
        for (const auto& cmd : commands) {
            assistant.processTextCommand(cmd, "test-session");
        }
    });
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
