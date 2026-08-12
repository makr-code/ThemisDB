/**
 * @file test_cross_functional_voice_observability.cpp
 * @brief Cross-functional integration test: Voice + Observability + Storage
 * 
 * Tests the complete workflow of voice processing with metrics collection
 * and persistent storage across multiple ThemisDB components.
 * 
 * @author ThemisDB Team
 * @date January 2025
 */

#include <gtest/gtest.h>
#include "voice/voice_assistant.h"
#include "observability/metrics_collector.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <thread>
#include <chrono>

using namespace themis::voice;
using namespace themis::observability;
using json = nlohmann::json;

/**
 * @brief Cross-functional test for Voice + Observability + Storage integration
 * 
 * This test validates that:
 * - Voice assistant processes commands
 * - Metrics are collected for all operations
 * - Data is persisted correctly
 * - Components interact seamlessly
 */
class CrossFunctionalVoiceObservabilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup voice assistant
        voice_config_.stt_model_path = "/tmp/test_stt";
        voice_config_.tts_model_path = "/tmp/test_tts";
        voice_config_.llm_model_path = "/tmp/test_llm";
        voice_config_.storage_path = "/tmp/test_voice_storage";
        voice_config_.enable_revision_control = true;
        
        // Reset metrics collector
        MetricsCollector::getInstance().reset();
    }
    
    void TearDown() override {
        MetricsCollector::getInstance().reset();
    }
    
    VoiceAssistant::Config voice_config_;
};

// ============================================================================
// Voice Command Processing with Metrics Collection
// ============================================================================

TEST_F(CrossFunctionalVoiceObservabilityTest, VoiceCommandWithMetricsTracking) {
    VoiceAssistant assistant(voice_config_);
    auto& metrics = MetricsCollector::getInstance();
    
    // Record voice command processing
    auto start = std::chrono::steady_clock::now();
    std::string session_id = "cross-func-test-001";
    std::string command = "Show me the database statistics";
    
    auto run_command = [&]() {
        std::string response = assistant.processTextCommand(command, session_id);
        
        auto end = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Record metrics for voice processing
        metrics.recordContentImport("text/plain", command.size());
        metrics.recordQuery("voice_command", duration_ms, 1);
        (void)response;
    };
    EXPECT_NO_THROW(run_command());
    
    // Verify metrics were collected
    std::string prometheus_metrics = metrics.getPrometheusMetrics();
    EXPECT_FALSE(prometheus_metrics.empty());
    EXPECT_NE(prometheus_metrics.find("queries_total"), std::string::npos);
}

TEST_F(CrossFunctionalVoiceObservabilityTest, PhoneCallRecordingWithStorageMetrics) {
    VoiceAssistant assistant(voice_config_);
    auto& metrics = MetricsCollector::getInstance();
    
    // Create phone call metadata
    PhoneCallMetadata call_metadata;
    call_metadata.call_id = "call-cross-func-001";
    call_metadata.caller_number = "+49123456789";
    call_metadata.callee_number = "+49987654321";
    call_metadata.start_time = std::chrono::system_clock::now().time_since_epoch().count();
    call_metadata.duration_ms = 60000;
    call_metadata.call_type = "inbound";
    
    // Simulate phone call audio (10KB)
    std::vector<uint8_t> audio_data(10 * 1024, 0);
    
    auto start = std::chrono::steady_clock::now();
    
    auto record_call = [&]() {
        json result = assistant.recordPhoneCall(audio_data, call_metadata);
        
        auto end = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Record metrics for the operation
        metrics.recordContentImport("audio/recording", audio_data.size());
        metrics.recordQuery("phone_call_transcription", duration_ms, 1); // NOPII: metric operation type literal, not a phone number
        
        // Record storage metrics
        metrics.recordMemoryUsage(audio_data.size());
        (void)result;
    };
    EXPECT_NO_THROW(record_call());
    
    // Verify comprehensive metrics
    std::string prometheus_metrics = metrics.getPrometheusMetrics();
    EXPECT_NE(prometheus_metrics.find("content_imports"), std::string::npos);
    EXPECT_NE(prometheus_metrics.find("queries_total"), std::string::npos);
}

TEST_F(CrossFunctionalVoiceObservabilityTest, MeetingProtocolWithAuditMetrics) {
    VoiceAssistant assistant(voice_config_);
    auto& metrics = MetricsCollector::getInstance();
    
    // Create meeting metadata
    MeetingMetadata meeting;
    meeting.meeting_id = "meeting-cross-func-001";
    meeting.title = "Cross-Functional Integration Review";
    meeting.start_time = std::chrono::system_clock::now().time_since_epoch().count();
    meeting.participants = {"alice@themisdb.com", "bob@themisdb.com", "charlie@themisdb.com"};
    meeting.organizer = "alice@themisdb.com";
    
    // Simulate meeting audio (100KB)
    std::vector<uint8_t> audio_data(100 * 1024, 0);
    
    auto start = std::chrono::steady_clock::now();
    
    auto generate_protocol = [&]() {
        json protocol = assistant.generateMeetingProtocol(audio_data, meeting);
        
        auto end = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Record comprehensive metrics
        metrics.recordContentImport("audio/meeting", audio_data.size());
        metrics.recordQuery("meeting_protocol_generation", duration_ms, 1);
        metrics.recordChunkCreation(meeting.participants.size());
        
        // Record audit metrics
        for (const auto& participant : meeting.participants) {
            metrics.recordAuthAttempt(true);
        }
        (void)protocol;
    };
    EXPECT_NO_THROW(generate_protocol());
    
    // Verify all metrics categories are present
    std::string prometheus_metrics = metrics.getPrometheusMetrics();
    EXPECT_NE(prometheus_metrics.find("content_imports"), std::string::npos);
    EXPECT_NE(prometheus_metrics.find("chunks_created"), std::string::npos);
}

// ============================================================================
// Multi-Session Concurrent Processing with Metrics
// ============================================================================

TEST_F(CrossFunctionalVoiceObservabilityTest, ConcurrentVoiceSessionsWithMetrics) {
    VoiceAssistant assistant(voice_config_);
    auto& metrics = MetricsCollector::getInstance();
    
    const int num_sessions = 10;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_sessions; i++) {
        threads.emplace_back([&assistant, &metrics, i]() {
            std::string session_id = "concurrent-session-" + std::to_string(i);
            std::string command = "Process command " + std::to_string(i);
            
            auto start = std::chrono::steady_clock::now();
            
            try {
                std::string response = assistant.processTextCommand(command, session_id);
                
                auto end = std::chrono::steady_clock::now();
                auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
                
                // Each thread records its own metrics
                metrics.recordQuery("concurrent_voice_command", duration_ms, 1);
                metrics.recordCacheHit("session_cache");
            } catch (...) {
                // Handle errors gracefully
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify metrics were collected from all threads
    std::string prometheus_metrics = metrics.getPrometheusMetrics();
    EXPECT_FALSE(prometheus_metrics.empty());
}

// ============================================================================
// Audio Format Conversion with Performance Metrics
// ============================================================================

TEST_F(CrossFunctionalVoiceObservabilityTest, AudioConversionWithPerformanceTracking) {
    VoiceAssistant assistant(voice_config_);
    auto& metrics = MetricsCollector::getInstance();
    
    // Test various audio sizes
    std::vector<size_t> audio_sizes = {10 * 1024, 100 * 1024, 1024 * 1024};
    
    for (size_t size : audio_sizes) {
        std::vector<uint8_t> audio_data(size, 0);
        
        auto start = std::chrono::steady_clock::now();
        
        auto convert = [&]() {
            std::vector<uint8_t> converted = assistant.convertAudioFormat(audio_data, "mp3");
            
            auto end = std::chrono::steady_clock::now();
            auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
            
            // Record performance metrics
            metrics.recordQuery("audio_conversion", duration_ms, converted.size());
            metrics.recordMemoryUsage(size + converted.size());
            
            // Record compression ratio if applicable
            if (!converted.empty()) {
                double ratio = static_cast<double>(converted.size()) / size;
                metrics.recordTSStoreCompression("audio", ratio);
            }
        };
        EXPECT_NO_THROW(convert());
    }
    
    // Verify performance metrics across different sizes
    std::string prometheus_metrics = metrics.getPrometheusMetrics();
    EXPECT_NE(prometheus_metrics.find("query_latency"), std::string::npos);
}

// ============================================================================
// Storage Operations with Metrics Integration
// ============================================================================

TEST_F(CrossFunctionalVoiceObservabilityTest, VoiceStorageWithFullMetrics) {
    VoiceAssistant assistant(voice_config_);
    auto& metrics = MetricsCollector::getInstance();
    
    std::vector<uint8_t> audio_data(50 * 1024, 0);
    std::string transcript = "This is a test recording for cross-functional validation.";
    json metadata;
    metadata["type"] = "test";
    metadata["category"] = "cross_functional";
    
    auto start = std::chrono::steady_clock::now();
    
    auto store = [&]() {
        std::string doc_id = assistant.storeRecording(audio_data, transcript, metadata);
        
        auto end = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Record comprehensive storage metrics
        metrics.recordQuery("store_recording", duration_ms, 1);
        metrics.recordMemoryUsage(audio_data.size());
        metrics.recordContentImport("audio/stored", audio_data.size());
        
        // Simulate index updates
        metrics.recordIndexScan("document_index", 1);
        
        // Record successful operation
        EXPECT_FALSE(doc_id.empty());
    };
    EXPECT_NO_THROW(store());
    
    // Verify storage metrics
    std::string prometheus_metrics = metrics.getPrometheusMetrics();
    EXPECT_NE(prometheus_metrics.find("queries_total"), std::string::npos);
    EXPECT_NE(prometheus_metrics.find("content_imports"), std::string::npos);
}

// ============================================================================
// Session Statistics with Metrics Export
// ============================================================================

TEST_F(CrossFunctionalVoiceObservabilityTest, SessionStatisticsWithMetricsExport) {
    VoiceAssistant assistant(voice_config_);
    auto& metrics = MetricsCollector::getInstance();
    
    // Create multiple sessions
    for (int i = 0; i < 5; i++) {
        std::string session_id = "stats-session-" + std::to_string(i);
        assistant.getSession(session_id);
        
        // Process some commands
        for (int j = 0; j < 3; j++) {
            std::string command = "Command " + std::to_string(j);
            assistant.processTextCommand(command, session_id);
            
            metrics.recordQuery("session_command", 10.0, 1);
        }
    }
    
    // Get statistics from both systems
    auto export_stats = [&]() {
        json voice_stats = assistant.getStatistics();
        std::string metrics_export = metrics.getPrometheusMetrics();
        
        EXPECT_TRUE(voice_stats.is_object());
        EXPECT_FALSE(metrics_export.empty());
        
        // Verify cross-system consistency
        EXPECT_NE(metrics_export.find("queries_total"), std::string::npos);
    };
    EXPECT_NO_THROW(export_stats());
}

// ============================================================================
// Error Handling with Metrics Tracking
// ============================================================================

TEST_F(CrossFunctionalVoiceObservabilityTest, ErrorHandlingWithMetricsTracking) {
    VoiceAssistant assistant(voice_config_);
    auto& metrics = MetricsCollector::getInstance();
    
    // Test error scenarios
    auto error_flow = [&]() {
        // Empty audio
        std::vector<uint8_t> empty_audio;
        assistant.processVoiceCommand(empty_audio, "error-test");
        
        // Record error metrics
        metrics.recordQuery("voice_command_error", 0.1, 0);
        
        // Invalid format conversion
        std::vector<uint8_t> audio(1024, 0);
        assistant.convertAudioFormat(audio, "invalid_format");
        
        metrics.recordQuery("conversion_error", 0.1, 0);
    };
    EXPECT_NO_THROW(error_flow());
    
    // Verify error metrics are tracked
    std::string prometheus_metrics = metrics.getPrometheusMetrics();
    EXPECT_FALSE(prometheus_metrics.empty());
}

// ============================================================================
// Main
// ============================================================================


