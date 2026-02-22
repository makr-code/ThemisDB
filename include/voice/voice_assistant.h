/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            voice_assistant.h                                  ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     254                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file voice_assistant.h
 * @brief Voice Assistant Manager
 * 
 * Integrates STT, TTS, and LLM for natural language voice interactions.
 * Handles phone call recording, meeting protocols, and voice commands.
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

#pragma once

#include "content/stt_processor.h"
#include "content/tts_processor.h"
#include "llm/llama_wrapper.h"
#include "voice/wake_word_detector.h"
#include <string>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace voice {

using json = nlohmann::json;

/**
 * @brief Voice interaction session
 */
struct VoiceSession {
    std::string session_id;
    std::string user_id;
    int64_t created_at;
    int64_t last_activity;
    json context;  // Conversation context
    std::vector<std::string> history;  // Conversation history
};

/**
 * @brief Phone call metadata
 */
struct PhoneCallMetadata {
    std::string call_id;
    std::string caller_number;
    std::string callee_number;
    int64_t start_time;
    int64_t end_time;
    int64_t duration_ms;
    std::string call_type;  // "inbound", "outbound", "conference"
    json custom_fields;
};

/**
 * @brief Meeting metadata
 */
struct MeetingMetadata {
    std::string meeting_id;
    std::string title;
    int64_t start_time;
    int64_t end_time;
    std::vector<std::string> participants;
    std::string organizer;
    json custom_fields;
};

/**
 * @brief Voice Assistant Manager
 * 
 * Combines STT, TTS, and LLM capabilities for:
 * - Natural language voice interactions (similar to Alexa/Siri)
 * - Phone call recording and transcription
 * - Meeting protocol generation
 * - Voice command processing
 * - Revision-safe storage in ThemisDB
 */
class VoiceAssistant {
public:
    struct Config {
        // STT configuration
        std::string stt_model_path;
        std::string stt_model_size = "base";
        std::string stt_language = "auto";
        
        // TTS configuration
        std::string tts_model_path;
        std::string tts_voice = "default";
        float tts_speed = 1.0f;
        
        // LLM configuration
        std::string llm_model_path;
        int llm_n_ctx = 4096;
        int llm_n_gpu_layers = 0;
        
        // Storage configuration
        std::string storage_path;
        bool enable_revision_control = true;
        bool compress_audio = true;
        std::string audio_format = "ogg";  // ogg, mp3, mp4

        // Wake-word configuration
        bool enable_wake_word = false;
        WakeWordConfig wake_word_config;
        std::vector<std::pair<std::string, std::string>> wake_words = {
            {"hey-themis",   "hey themis"},
            {"themis",       "themis"},
            {"database",     "database"}
        };
    };
    
    VoiceAssistant(const Config& config);
    ~VoiceAssistant();
    
    /**
     * @brief Initialize voice assistant
     */
    bool initialize();
    
    /**
     * @brief Shutdown voice assistant
     */
    void shutdown();
    
    /**
     * @brief Process voice command
     * 
     * @param audio_data Audio input from user
     * @param session_id Session identifier
     * @return Spoken response as audio
     */
    std::vector<uint8_t> processVoiceCommand(
        const std::vector<uint8_t>& audio_data,
        const std::string& session_id
    );
    
    /**
     * @brief Process text command (for testing/API)
     * 
     * @param text Text input from user
     * @param session_id Session identifier
     * @return Text response
     */
    std::string processTextCommand(
        const std::string& text,
        const std::string& session_id
    );

    /**
     * @brief Process voice command with real-time streaming STT
     *
     * Transcribes audio in incremental windows and invokes @p segment_callback
     * for each recognized segment as it becomes available (word-by-word delivery).
     * After full transcription the LLM generates a response and TTS synthesizes it.
     *
     * @param audio_data     Audio input from user (WAV or supported format)
     * @param session_id     Session identifier
     * @param segment_callback Called for every transcribed segment; may be nullptr
     * @return Spoken response as audio bytes (WAV), empty on failure
     */
    std::vector<uint8_t> streamProcessVoiceCommand(
        const std::vector<uint8_t>& audio_data,
        const std::string& session_id,
        std::function<void(const content::TranscriptionSegment&)> segment_callback = nullptr
    );

    /**
     * @brief Scan an audio chunk for registered wake words.
     *
     * Feed raw 16-bit PCM audio to the internal WakeWordDetector.  The
     * detector maintains a rolling buffer internally so callers may stream
     * small chunks continuously.
     *
     * @param audio_chunk  Raw PCM audio (16-bit LE, mono, at the sample rate
     *                     configured in Config::wake_word_config).
     * @return Detection result.  result.detected == true when a wake word fires.
     */
    WakeWordDetectionResult detectWakeWord(
        const std::vector<uint8_t>& audio_chunk
    );

    /**
     * @brief Register an optional callback invoked on every wake-word detection.
     *
     * The callback runs synchronously inside processAudioChunk() on the calling
     * thread.  Pass nullptr to remove.
     *
     * @param callback  Function to call when a wake word is detected.
     */
    void setWakeWordCallback(WakeWordDetector::DetectionCallback callback);
    
    /**
     * @brief Record and transcribe phone call
     * 
     * @param audio_data Phone call recording
     * @param metadata Call metadata
     * @return Transcription with metadata
     */
    json recordPhoneCall(
        const std::vector<uint8_t>& audio_data,
        const PhoneCallMetadata& metadata
    );
    
    /**
     * @brief Generate meeting protocol
     * 
     * @param audio_data Meeting recording
     * @param metadata Meeting metadata
     * @return Structured meeting protocol
     */
    json generateMeetingProtocol(
        const std::vector<uint8_t>& audio_data,
        const MeetingMetadata& metadata
    );
    
    /**
     * @brief Convert audio to different format
     * 
     * @param audio_data Input audio
     * @param target_format Target format (ogg, mp3, mp4)
     * @return Converted audio
     */
    std::vector<uint8_t> convertAudioFormat(
        const std::vector<uint8_t>& audio_data,
        const std::string& target_format
    );
    
    /**
     * @brief Store recording in ThemisDB with revision control
     * 
     * @param audio_data Audio recording
     * @param transcript Transcription text
     * @param metadata Additional metadata
     * @return Document ID in ThemisDB
     */
    std::string storeRecording(
        const std::vector<uint8_t>& audio_data,
        const std::string& transcript,
        const json& metadata
    );
    
    /**
     * @brief Get or create session
     */
    VoiceSession getSession(const std::string& session_id);
    
    /**
     * @brief Update session context
     */
    void updateSession(const std::string& session_id, const json& context);
    
    /**
     * @brief Get statistics
     */
    json getStatistics() const;

private:
    Config config_;
    
    // Processors
    std::unique_ptr<content::STTProcessor> stt_processor_;
    std::unique_ptr<content::TTSProcessor> tts_processor_;
    std::unique_ptr<llm::LlamaWrapper> llm_wrapper_;  // Changed from LlamaCppInferenceEngine
    
    // Wake-word detector
    std::unique_ptr<WakeWordDetector> wake_word_detector_;
    
    // Session management
    std::map<std::string, VoiceSession> sessions_;
    std::mutex sessions_mutex_;
    
    bool initialized_ = false;
    
    // Internal methods
    std::string generateLLMResponse(
        const std::string& user_input,
        const VoiceSession& session
    );
    
    json generateSummary(const std::string& transcript);
    json extractKeyPoints(const std::string& transcript);
    json extractActionItems(const std::string& transcript);
    
    std::string createRevisionEntry(
        const std::string& entity_id,
        const std::vector<uint8_t>& data,
        const json& metadata
    );
};

} // namespace voice
} // namespace themis
