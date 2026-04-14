/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            voice_assistant.cpp                                ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 07:07:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     685                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b3d8aa4a55  2026-03-15  refactor: streamline performance statistics retrieval and... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • fc33113125  2026-03-01  feat(voice): implement language detection and auto-locale... ║
    • 49fd402198  2026-03-01  feat(voice): expose speaker verification REST API endpoints ║
    • 75c7c24ea3  2026-03-01  feat(voice): implement voice session playback and search ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file voice_assistant.cpp
 * @brief Voice Assistant Manager Implementation
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

#include "voice/voice_assistant.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace themis {
namespace voice {

VoiceAssistant::VoiceAssistant(const Config& config)
    : config_(config), voice_authenticator_(config.voice_auth_config) {
    // Initialise wake-word detector regardless of the enable flag so that
    // detectWakeWord() is always safe to call; the caller can gate on the flag.
    wake_word_detector_ = std::make_unique<WakeWordDetector>(config_.wake_word_config);
    for (const auto& entry : config_.wake_words) {
        wake_word_detector_->addWakeWord(entry.first, entry.second);
    }
}

VoiceAssistant::~VoiceAssistant() {
    if (initialized_) {
        shutdown();
    }
}

bool VoiceAssistant::initialize() {
    if (initialized_) {
        return true;
    }
    
    try {
        // Initialize STT processor
        stt_processor_ = std::make_unique<content::STTProcessor>();
        content::PluginConfig stt_config;
        json stt_settings;
        stt_settings["model_path"] = config_.stt_model_path;
        stt_settings["model_size"] = config_.stt_model_size;
        stt_settings["language"] = config_.stt_language;
        stt_settings["timestamps"] = true;
        stt_settings["speaker_diarization"] = true;
        stt_config = content::PluginConfig(stt_settings);
        
        if (!stt_processor_->initialize(stt_config)) {
            return false;
        }
        
        // Initialize TTS processor
        tts_processor_ = std::make_unique<content::TTSProcessor>();
        content::PluginConfig tts_config;
        json tts_settings;
        tts_settings["model_path"] = config_.tts_model_path;
        tts_settings["default_voice"] = config_.tts_voice;
        tts_settings["default_speed"] = config_.tts_speed;
        tts_config = content::PluginConfig(tts_settings);
        
        if (!tts_processor_->initialize(tts_config)) {
            return false;
        }
        
        // Initialize LLM using unified LlamaWrapper implementation
        llm::LlamaWrapper::Config llm_config;
        llm_config.n_ctx = config_.llm_n_ctx;
        llm_config.n_gpu_layers = config_.llm_n_gpu_layers;
        llm_config.use_mmap = true;
        llm_config.n_threads = 4;
        llm_config.n_batch = 512;
        
        llm_wrapper_ = std::make_unique<llm::LlamaWrapper>(llm_config);
        
        if (!config_.llm_model_path.empty()) {
            json model_config = {
                {"model_path", config_.llm_model_path},
                {"model_id", "voice-assistant-model"}
            };
            llm_wrapper_->loadModel(model_config);
        }
        
        initialized_ = true;
        return true;
        
    } catch (const std::exception& e) {
        return false;
    }
}

void VoiceAssistant::shutdown() {
    if (!initialized_) {
        return;
    }
    
    if (stt_processor_) {
        stt_processor_->shutdown();
    }
    
    if (tts_processor_) {
        tts_processor_->shutdown();
    }
    
    if (llm_wrapper_) {
        llm_wrapper_->unloadModel();
    }
    
    sessions_.clear();
    initialized_ = false;
}

std::vector<uint8_t> VoiceAssistant::processVoiceCommand(
    const std::vector<uint8_t>& audio_data,
    const std::string& session_id
) {
    if (!initialized_) {
        return {};
    }

    // Voice biometric authentication gate
    if (config_.enable_voice_auth) {
        auto auth_session = getSession(session_id);
        const std::string& uid = auth_session.user_id;
        if (!uid.empty()) {
            auto auth_result = voice_authenticator_.authenticate(uid, audio_data);
            if (!auth_result.authenticated) {
                content::TTSOptions tts_opts;
                tts_opts.voice_id = config_.tts_voice;
                tts_opts.format   = "wav";
                auto tts_result   = tts_processor_->synthesize(
                    "Voice authentication failed. Please try again.",
                    tts_opts);
                return tts_result.audio_data;
            }
        }
    }

    // Get or create session
    auto session = getSession(session_id);
    
    // Transcribe audio to text
    auto transcription = stt_processor_->transcribe(audio_data);
    
    if (!transcription.success) {
        // Return error message as speech
        content::TTSOptions tts_options;
        tts_options.voice_id = config_.tts_voice;
        tts_options.format = "wav";
        
        auto tts_result = tts_processor_->synthesize(
            "I'm sorry, I couldn't understand that. Please try again.",
            tts_options
        );
        
        return tts_result.audio_data;
    }

    // Auto-locale switching: when language is set to "auto", update the session's
    // preferred language from the STT-detected language and adapt TTS accordingly.
    if (config_.stt_language == "auto" && !transcription.detected_language.empty()) {
        session.preferred_language = transcription.detected_language;
    }
    
    // Add to conversation history
    session.history.push_back("User: " + transcription.full_text);
    
    // Generate LLM response
    std::string llm_response = generateLLMResponse(transcription.full_text, session);
    
    // Add to conversation history
    session.history.push_back("Assistant: " + llm_response);
    
    // Update session - persist changes
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_[session_id] = session;
    }
    
    // Synthesize response - use the session's (potentially auto-detected) language
    content::TTSOptions tts_options;
    tts_options.voice_id = config_.tts_voice;
    tts_options.speed = config_.tts_speed;
    tts_options.format = "wav";
    tts_options.language = session.preferred_language;
    
    auto tts_result = tts_processor_->synthesize(llm_response, tts_options);
    
    return tts_result.audio_data;
}

std::string VoiceAssistant::processTextCommand(
    const std::string& text,
    const std::string& session_id
) {
    if (!initialized_) {
        return "Voice assistant not initialized";
    }
    
    // Check if the text matches a registered voice macro trigger.
    const MacroID matched_id = macro_manager_.matchTrigger(text);
    if (!matched_id.empty()) {
        MacroResult result = macro_manager_.executeMacro(matched_id);
        if (result.success) {
            return result.output;
        }
        // Fall through to LLM on macro failure.
    }

    // Get or create session
    auto session = getSession(session_id);
    
    // Add to conversation history
    session.history.push_back("User: " + text);
    
    // Generate LLM response
    std::string llm_response = generateLLMResponse(text, session);
    
    // Add to conversation history
    session.history.push_back("Assistant: " + llm_response);
    
    // Update session - persist changes
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_[session_id] = session;
    }
    
    return llm_response;
}

std::vector<uint8_t> VoiceAssistant::streamProcessVoiceCommand(
    const std::vector<uint8_t>& audio_data,
    const std::string& session_id,
    std::function<void(const content::TranscriptionSegment&)> segment_callback
) {
    if (!initialized_) {
        return {};
    }

    // Voice biometric authentication gate (mirrors processVoiceCommand)
    if (config_.enable_voice_auth) {
        auto auth_session = getSession(session_id);
        const std::string& uid = auth_session.user_id;
        if (!uid.empty()) {
            auto auth_result = voice_authenticator_.authenticate(uid, audio_data);
            if (!auth_result.authenticated) {
                content::TTSOptions tts_opts;
                tts_opts.voice_id = config_.tts_voice;
                tts_opts.format   = "wav";
                auto tts_result   = tts_processor_->synthesize(
                    "Voice authentication failed. Please try again.",
                    tts_opts);
                return tts_result.audio_data;
            }
        }
    }

    auto session = getSession(session_id);

    // Accumulate all segments delivered by the streaming transcription.
    std::string full_transcript;
    std::mutex transcript_mutex;

    auto on_segment = [&](const content::TranscriptionSegment& seg) {
        {
            std::lock_guard<std::mutex> lock(transcript_mutex);
            if (!full_transcript.empty()) {
                full_transcript += ' ';
            }
            full_transcript += seg.text;
        }
        // Forward to caller's callback if provided.
        if (segment_callback) {
            segment_callback(seg);
        }
    };

    bool ok = stt_processor_->streamTranscribe(audio_data, on_segment);

    // For auto-locale switching: also run batch transcription to get detected_language.
    std::string detected_lang;
    if (!ok || full_transcript.empty()) {
        // Fall back to batch transcription so the pipeline always returns audio.
        auto transcription = stt_processor_->transcribe(audio_data);
        if (transcription.success) {
            full_transcript = transcription.full_text;
            detected_lang = transcription.detected_language;
        }
    } else if (config_.stt_language == "auto") {
        // When language auto-detection is enabled and streaming succeeded, run a
        // lightweight batch transcription to obtain detected_language.  The full
        // streaming pipeline does not expose per-call language information, so
        // this one-shot check is the minimal way to capture the locale.  The
        // overhead is bounded by the same audio length already processed above.
        auto transcription = stt_processor_->transcribe(audio_data);
        if (transcription.success) {
            detected_lang = transcription.detected_language;
        }
    }

    // Auto-locale switching: update session language from STT detection.
    if (config_.stt_language == "auto" && !detected_lang.empty()) {
        session.preferred_language = detected_lang;
    }

    if (full_transcript.empty()) {
        content::TTSOptions tts_options;
        tts_options.voice_id = config_.tts_voice;
        tts_options.format = "wav";
        auto tts_result = tts_processor_->synthesize(
            "I'm sorry, I couldn't understand that. Please try again.",
            tts_options
        );
        return tts_result.audio_data;
    }

    session.history.push_back("User: " + full_transcript);

    std::string llm_response = generateLLMResponse(full_transcript, session);

    session.history.push_back("Assistant: " + llm_response);

    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_[session_id] = session;
    }

    content::TTSOptions tts_options;
    tts_options.voice_id = config_.tts_voice;
    tts_options.speed    = config_.tts_speed;
    tts_options.format   = "wav";
    tts_options.language = session.preferred_language;

    auto tts_result = tts_processor_->synthesize(llm_response, tts_options);
    return tts_result.audio_data;
}

json VoiceAssistant::recordPhoneCall(
    const std::vector<uint8_t>& audio_data,
    const PhoneCallMetadata& metadata
) {
    if (!initialized_) {
        json error;
        error["success"] = false;
        error["error"] = "Voice assistant not initialized";
        return error;
    }
    
    // Transcribe phone call
    json transcription_options;
    transcription_options["language"] = config_.stt_language;
    transcription_options["timestamps"] = true;
    transcription_options["speaker_diarization"] = true;
    
    auto transcription = stt_processor_->transcribe(audio_data, transcription_options);
    
    if (!transcription.success) {
        json error;
        error["success"] = false;
        error["error"] = transcription.error_message;
        return error;
    }
    
    // Build result
    json result;
    result["success"] = true;
    result["call_id"] = metadata.call_id;
    result["transcript"] = transcription.full_text;
    result["language"] = transcription.detected_language;
    result["confidence"] = transcription.average_confidence;
    result["duration_ms"] = transcription.audio_duration_ms;
    
    // Add metadata
    json call_metadata;
    call_metadata["caller"] = metadata.caller_number;
    call_metadata["callee"] = metadata.callee_number;
    call_metadata["start_time"] = metadata.start_time;
    call_metadata["end_time"] = metadata.end_time;
    call_metadata["call_type"] = metadata.call_type;
    call_metadata["custom_fields"] = metadata.custom_fields;
    result["metadata"] = call_metadata;
    
    // Add segments
    json segments = json::array();
    for (const auto& seg : transcription.segments) {
        json seg_json;
        seg_json["text"] = seg.text;
        seg_json["start_ms"] = seg.start_ms;
        seg_json["end_ms"] = seg.end_ms;
        seg_json["confidence"] = seg.confidence;
        if (seg.speaker_id >= 0) {
            seg_json["speaker"] = "Speaker " + std::to_string(seg.speaker_id + 1);
        }
        segments.push_back(seg_json);
    }
    result["segments"] = segments;
    
    // Generate summary using LLM
    auto summary = generateSummary(transcription.full_text);
    result["summary"] = summary;
    
    // Convert audio format if needed
    std::vector<uint8_t> converted_audio;
    if (config_.compress_audio) {
        converted_audio = convertAudioFormat(audio_data, config_.audio_format);
    } else {
        converted_audio = audio_data;
    }
    
    // Store in ThemisDB with revision control
    std::string doc_id = storeRecording(converted_audio, transcription.full_text, result);
    result["document_id"] = doc_id;
    
    return result;
}

json VoiceAssistant::generateMeetingProtocol(
    const std::vector<uint8_t>& audio_data,
    const MeetingMetadata& metadata
) {
    if (!initialized_) {
        json error;
        error["success"] = false;
        error["error"] = "Voice assistant not initialized";
        return error;
    }
    
    // Generate protocol using STT processor
    json protocol_options;
    protocol_options["language"] = config_.stt_language;
    protocol_options["speaker_diarization"] = true;
    protocol_options["format"] = "structured";
    
    auto protocol = stt_processor_->generateMeetingProtocol(audio_data, protocol_options);
    
    // Add meeting metadata
    protocol["meeting_id"] = metadata.meeting_id;
    protocol["title"] = metadata.title;
    protocol["start_time"] = metadata.start_time;
    protocol["end_time"] = metadata.end_time;
    protocol["duration_ms"] = metadata.end_time - metadata.start_time;
    protocol["organizer"] = metadata.organizer;
    
    json participants = json::array();
    for (const auto& participant : metadata.participants) {
        participants.push_back(participant);
    }
    protocol["participants"] = participants;
    protocol["custom_fields"] = metadata.custom_fields;
    
    // Extract key points and action items using LLM
    if (protocol.contains("transcript")) {
        std::string transcript = protocol["transcript"];
        protocol["key_points"] = extractKeyPoints(transcript);
        protocol["action_items"] = extractActionItems(transcript);
    }
    
    // Convert audio format if needed
    std::vector<uint8_t> converted_audio;
    if (config_.compress_audio) {
        converted_audio = convertAudioFormat(audio_data, config_.audio_format);
    } else {
        converted_audio = audio_data;
    }
    
    // Store in ThemisDB with revision control
    std::string doc_id = storeRecording(
        converted_audio,
        protocol.value("transcript", ""),
        protocol
    );
    protocol["document_id"] = doc_id;
    
    return protocol;
}

std::vector<uint8_t> VoiceAssistant::convertAudioFormat(
    const std::vector<uint8_t>& audio_data,
    const std::string& target_format
) {
    // Real implementation would use FFmpeg or similar library
    // to convert between formats (WAV -> OGG/MP3/MP4)
    
    // For now, return original data (placeholder)
    return audio_data;
}

std::string VoiceAssistant::storeRecording(
    const std::vector<uint8_t>& audio_data,
    const std::string& transcript,
    const json& metadata
) {
    // Store in the embedded VoiceAudioStorage so recordings are
    // accessible for playback and transcript search via audioStorage().
    AudioFormat fmt = audio_storage_.detectFormat(audio_data);
    fmt.duration_seconds = metadata.value("duration_seconds", 0.0f);
    return audio_storage_.store(audio_data, fmt, transcript, metadata);
}

VoiceSession VoiceAssistant::getSession(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        // Update last activity
        it->second.last_activity = std::chrono::system_clock::now().time_since_epoch().count();
        return it->second;
    }
    
    // Create new session
    VoiceSession session;
    session.session_id = session_id;
    session.created_at = std::chrono::system_clock::now().time_since_epoch().count();
    session.last_activity = session.created_at;
    session.context = json::object();
    
    sessions_[session_id] = session;
    return session;
}

void VoiceAssistant::updateSession(const std::string& session_id, const json& context) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        it->second.context = context;
        it->second.last_activity = std::chrono::system_clock::now().time_since_epoch().count();
    } else {
        // Log warning: attempting to update non-existent session
        // In production, this should be logged properly
    }
}

json VoiceAssistant::getStatistics() const {
    json stats;
    
    if (stt_processor_) {
        stats["stt"] = stt_processor_->getStatistics();
    }
    
    if (tts_processor_) {
        stats["tts"] = tts_processor_->getStatistics();
    }
    
    if (llm_wrapper_) {
        stats["llm"] = llm_wrapper_->getPerformanceStats();
    }
    
    if (wake_word_detector_) {
        stats["wake_word"] = wake_word_detector_->getStatistics();
    }

    stats["voice_auth"] = voice_authenticator_.get_statistics();

    stats["macros"] = macro_manager_.getStatistics();

    {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(sessions_mutex_));
        stats["active_sessions"] = sessions_.size();
    }
    
    return stats;
}

// Private implementation methods

std::string VoiceAssistant::createRevisionEntry(
    const std::string& entity_id,
    const std::vector<uint8_t>& data,
    const json& metadata
) {
    // Real implementation would:
    // 1. Create revision entry in ThemisDB
    // 2. Store previous version
    // 3. Update current version
    // 4. Add audit log entry
    // 5. Return revision ID
    
    // Placeholder
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::stringstream ss;
    ss << "revision:" << std::hex << now;
    return ss.str();
}

// ---------------------------------------------------------------------------
// Voice biometric authentication
// ---------------------------------------------------------------------------

bool VoiceAssistant::enrollSpeaker(
    const std::string&                        user_id,
    const std::vector<std::vector<uint8_t>>& audio_samples,
    VoiceProfileID&                           out_profile_id,
    const EnrollmentConfig&                   config)
{
    return voice_authenticator_.enroll_voice(user_id, audio_samples, out_profile_id, config);
}

VoiceAuthResult VoiceAssistant::authenticateSpeaker(
    const std::string&          user_id,
    const std::vector<uint8_t>& audio_sample)
{
    return voice_authenticator_.authenticate(user_id, audio_sample);
}

VerificationResult VoiceAssistant::verifyVoiceSpeaker(
    const VoiceProfileID&         profile_id,
    const std::vector<uint8_t>&   audio_sample)
{
    return voice_authenticator_.verify_speaker(profile_id, audio_sample);
}

IdentificationResult VoiceAssistant::identifyVoiceProfiles(
    const std::vector<VoiceProfileID>& candidate_profiles,
    const std::vector<uint8_t>&        audio_sample)
{
    return voice_authenticator_.identify_speaker(candidate_profiles, audio_sample);
}

bool VoiceAssistant::deleteVoiceProfile(const VoiceProfileID& profile_id)
{
    return voice_authenticator_.delete_profile(profile_id);
}

std::vector<VoiceProfileID> VoiceAssistant::listVoiceProfiles() const
{
    return voice_authenticator_.list_profiles();
}


WakeWordDetectionResult VoiceAssistant::detectWakeWord(
    const std::vector<uint8_t>& audio_chunk
) {
    return wake_word_detector_->processAudioChunk(audio_chunk);
}

void VoiceAssistant::setWakeWordCallback(WakeWordDetector::DetectionCallback callback) {
    wake_word_detector_->setDetectionCallback(std::move(callback));
}

VoiceMacroManager& VoiceAssistant::macroManager() {
    return macro_manager_;
}

const VoiceMacroManager& VoiceAssistant::macroManager() const {
    return macro_manager_;
}

VoiceAudioStorage& VoiceAssistant::audioStorage() {
    return audio_storage_;
}

const VoiceAudioStorage& VoiceAssistant::audioStorage() const {
    return audio_storage_;
}

} // namespace voice
} // namespace themis
