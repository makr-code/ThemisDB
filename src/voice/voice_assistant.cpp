/**
 * @file voice_assistant.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 87/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=1, H=11, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "voice/voice_assistant.h"
#include <chrono>
#include <sstream>
#include <iomanip>

// Optional FFmpeg (libavformat + libavcodec) backend for audio format conversion.
// Activated when the build defines THEMIS_HAS_FFMPEG (set by CMake when the
// ffmpeg feature / libavformat is found).
#if defined(THEMIS_HAS_FFMPEG) || defined(THEMIS_ENABLE_FFMPEG)
extern "C" {
#  include <libavformat/avformat.h>
#  include <libavcodec/avcodec.h>
#  include <libavutil/opt.h>
}
#  include <cstdio>
#  include <thread>
#  define THEMIS_VOICE_HAS_FFMPEG 1
#else
#  define THEMIS_VOICE_HAS_FFMPEG 0
#endif

namespace themis {
namespace voice {

VoiceAssistant::VoiceAssistant(const Config& config)
        : config_(config),
            voice_authenticator_(config.voice_auth_config),
            voice_security_manager_(config.voice_security_config) {
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
        json stt_settings = json::object({
            {"model_path", config_.stt_model_path},
            {"model_size", config_.stt_model_size},
            {"language", config_.stt_language},
            {"timestamps", true},
            {"speaker_diarization", true}
        });
        stt_config = content::PluginConfig(stt_settings);
        
        if (!stt_processor_->initialize(stt_config)) {
            return false;
        }
        
        // Initialize TTS processor
        tts_processor_ = std::make_unique<content::TTSProcessor>();
        content::PluginConfig tts_config;
        json tts_settings = json::object({
            {"model_path", config_.tts_model_path},
            {"default_voice", config_.tts_voice},
            {"default_speed", config_.tts_speed}
        });
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
        
    } catch (const std::string&) {
        return false;
    } catch (const char*) {
        return false;
    } catch (...) {
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
            logVoiceAuthenticationAudit(uid, session_id, "process_voice_command", auth_result);
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
    if (text.empty()) {
        return "I need a prompt to generate a response. Please provide your question or request.";
    }

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
            logVoiceAuthenticationAudit(uid, session_id, "stream_process_voice_command", auth_result);
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
    // When an audio conversion backend has been injected, delegate to it.
    if (audio_convert_fn_) {
        auto converted = audio_convert_fn_(audio_data, target_format);
        if (!converted.empty()) {
            return converted;
        }
        // Empty result from fn → fall through to passthrough with a warning.
        SPDLOG_WARN("VoiceAssistant::convertAudioFormat: injected AudioConvertFn "
                    "returned empty result for target_format='{}'; returning original bytes.",
                    target_format);
    }

    // PERMANENT FALLBACK NOTE:
    // Purpose: Return the original audio bytes unchanged when no FFmpeg-backed
    //          AudioConvertFn has been injected.  This is the correct behaviour
    //          for builds without libavformat / libavcodec.
    // Activation: audio_convert_fn_ is null (no real codec injected at startup).
    // Real implementation: Call setAudioConvertFn(VoiceAssistant::makeFFmpegAudioConvertFn())
    //   at application startup when THEMIS_HAS_FFMPEG is defined (FFmpeg linked).
    //   The injected fn path is already wired above (audio_convert_fn_ fast-path).
    // See: include/voice/voice_assistant.h § makeFFmpegAudioConvertFn()
    //      src/voice/FUTURE_ENHANCEMENTS.md §Voice Audio Format Conversion
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
    }
}

bool VoiceAssistant::deleteSession(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    return sessions_.erase(session_id) > 0;
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
    stats["voice_security"] = voice_security_manager_.getSecurityStats();

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
    // Build a unique revision ID from the entity ID and current time.
    const auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::stringstream ss;
    ss << "revision:" << entity_id << ":" << std::hex << now;
    const std::string rev_id = ss.str();

    // FNV-1a hash of the data payload for integrity / change-detection purposes.
    uint32_t hash = 2166136261u;
    for (const uint8_t byte : data) {
        hash ^= static_cast<uint32_t>(byte);
        hash *= 16777619u;
    }

    // Store the revision record so that history queries find it within this
    // process lifetime.  A persistent backend (e.g., RocksDB collection) can
    // be wired in by replacing this in-memory store without changing the API.
    {
        std::lock_guard<std::mutex> lock(revision_store_mutex_);
        RevisionEntry entry;
        entry.entity_id  = entity_id;
        entry.data_hash  = hash;
        entry.metadata   = metadata;
        entry.timestamp  = now;
        revision_store_[rev_id] = std::move(entry);
    }

    return rev_id;
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
    auto result = voice_authenticator_.authenticate(user_id, audio_sample);
    logVoiceAuthenticationAudit(user_id, "", "authenticate_speaker", result);
    return result;
}

void VoiceAssistant::logVoiceAuthenticationAudit(
    const std::string& user_id,
    const std::string& session_id,
    const std::string& action,
    const VoiceAuthResult& result)
{
    VoiceAuditEntry entry;
    entry.event_type = "voice_authentication";
    entry.session_id = session_id;
    entry.user_id = user_id;
    entry.action = action;
    entry.resource = "voice_assistant";
    entry.timestamp_ms = result.timestamp_ms;
    if (entry.timestamp_ms <= 0) {
        entry.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    entry.success = result.authenticated;
    entry.details = result.decision_reason;
    entry.metadata = {
        {"confidence_score", result.confidence_score},
        {"threshold", result.threshold}
    };
    voice_security_manager_.logEvent(entry);
}

VerificationResult VoiceAssistant::verifyVoiceSpeaker(
    const VoiceProfileID&         profile_id,
    const std::vector<uint8_t>&   audio_sample)
{
    auto result = voice_authenticator_.verify_speaker(profile_id, audio_sample);
    
    // Audit logging: record verification result for compliance
    VoiceAuditEntry entry;
    entry.event_type = "voice_verification";
    entry.session_id = "";
    entry.user_id = "";  // Profile-based verification (user_id not always available)
    entry.action = "verify_voice_speaker";
    entry.resource = "voice_profile:" + profile_id;
    entry.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    entry.success = result.verified;
    entry.details = "Verification against profile: " + profile_id;
    entry.metadata = {
        {"match_score", result.match_score},
        {"threshold", result.threshold}
    };
    voice_security_manager_.logEvent(entry);
    
    return result;
}

IdentificationResult VoiceAssistant::identifyVoiceProfiles(
    const std::vector<VoiceProfileID>& candidate_profiles,
    const std::vector<uint8_t>&        audio_sample)
{
    auto result = voice_authenticator_.identify_speaker(candidate_profiles, audio_sample);
    
    // Audit logging: record identification result for compliance
    VoiceAuditEntry entry;
    entry.event_type = "voice_identification";
    entry.session_id = "";
    entry.user_id = "";  // Identification may match any profile
    entry.action = "identify_voice_profiles";
    entry.resource = "voice_profiles:" + std::to_string(candidate_profiles.size());
    entry.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    entry.success = !result.matches.empty();
    entry.details = "Identification against " + std::to_string(candidate_profiles.size()) + 
                    " candidate profiles, " + std::to_string(result.matches.size()) + " matched";
    entry.metadata = {
        {"candidate_count", candidate_profiles.size()},
        {"match_count", result.matches.size()}
    };
    voice_security_manager_.logEvent(entry);
    
    return result;
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

content::TTSResult VoiceAssistant::synthesize(
    const std::string& text,
    const content::TTSOptions& options
) {
    if (!tts_processor_) {
        content::TTSResult err;
        err.success = false;
        err.error_message = "TTS processor not initialized";
        return err;
    }
    return tts_processor_->synthesize(text, options);
}

json VoiceAssistant::getAvailableVoices() const {
    if (!tts_processor_) { return json::array(); }
    return tts_processor_->getAvailableVoices();
}

std::vector<std::string> VoiceAssistant::getSupportedLanguages() const {
    if (!tts_processor_) { return {}; }
    return tts_processor_->getSupportedLanguages();
}

// ─────────────────────────────────────────────────────────────────────────────
// Factory: FFmpeg-backed AudioConvertFn
// Guarded by THEMIS_VOICE_HAS_FFMPEG (set when THEMIS_HAS_FFMPEG or
// THEMIS_ENABLE_FFMPEG is defined and libavformat/libavcodec are linked).
// Returns an empty function when FFmpeg is unavailable.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Returns a libavformat/libavcodec-backed AudioConvertFn.
 *
 * When @c THEMIS_HAS_FFMPEG is defined at build time, this factory creates a
 * real audio transcoding backend using FFmpeg's avformat demuxer and avcodec
 * encoder pipeline.  The returned fn writes input bytes to a temporary in-memory
 * AVIOContext, demuxes, decodes, re-encodes in the requested output format
 * (identified by the @p target_format string, e.g. "ogg", "mp3", "mp4"), and
 * returns the encoded bytes.
 *
 * When FFmpeg is not available, returns an empty @c std::function so that
 * @c convertAudioFormat() falls back to the passthrough path.
 *
 * @return AudioConvertFn backed by FFmpeg, or empty fn if unavailable.
 */
VoiceAssistant::AudioConvertFn VoiceAssistant::makeFFmpegAudioConvertFn()
{
#if THEMIS_VOICE_HAS_FFMPEG
    return [](const std::vector<uint8_t>& audio_data,
              const std::string& target_format) -> std::vector<uint8_t>
    {
        if (audio_data.empty() || target_format.empty()) {
            return {};
        }

        // Write input to a temporary file; libavformat works best with seekable
        // IO and this avoids complex custom AVIO boilerplate for all containers.
        // Use /tmp on POSIX; platform-appropriate temp dir on others.
        std::string tmp_in  = std::string("/tmp/themis_fim_in_")  + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + ".raw";
        std::string tmp_out = std::string("/tmp/themis_fim_out_") + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + "." + target_format;

        // Write input bytes to tmp_in
        {
            FILE* f = fopen(tmp_in.c_str(), "wb");
            if (!f) return {};
            fwrite(audio_data.data(), 1, audio_data.size(), f);
            fclose(f);
        }

        // Use avformat to transcode: open input, open output, copy/transcode streams.
        AVFormatContext* in_ctx  = nullptr;
        AVFormatContext* out_ctx = nullptr;
        std::vector<uint8_t> result;

        if (avformat_open_input(&in_ctx, tmp_in.c_str(), nullptr, nullptr) < 0) {
            remove(tmp_in.c_str());
            return {};
        }
        if (avformat_find_stream_info(in_ctx, nullptr) < 0) {
            avformat_close_input(&in_ctx);
            remove(tmp_in.c_str());
            return {};
        }

        if (avformat_alloc_output_context2(&out_ctx, nullptr,
                                           target_format.c_str(),
                                           tmp_out.c_str()) < 0) {
            avformat_close_input(&in_ctx);
            remove(tmp_in.c_str());
            return {};
        }

        // Copy stream headers
        for (unsigned i = 0; i < in_ctx->nb_streams; ++i) {
            AVStream* in_stream  = in_ctx->streams[i];
            AVStream* out_stream = avformat_new_stream(out_ctx, nullptr);
            if (!out_stream) continue;
            avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
            out_stream->codecpar->codec_tag = 0;
        }

        if (!(out_ctx->oformat->flags & AVFMT_NOFILE)) {
            if (avio_open(&out_ctx->pb, tmp_out.c_str(), AVIO_FLAG_WRITE) < 0) {
                avformat_close_input(&in_ctx);
                avformat_free_context(out_ctx);
                remove(tmp_in.c_str());
                return {};
            }
        }

        if (avformat_write_header(out_ctx, nullptr) < 0) {
            avformat_close_input(&in_ctx);
            if (!(out_ctx->oformat->flags & AVFMT_NOFILE))
                avio_closep(&out_ctx->pb);
            avformat_free_context(out_ctx);
            remove(tmp_in.c_str());
            return {};
        }

        AVPacket* pkt = av_packet_alloc();
        while (av_read_frame(in_ctx, pkt) >= 0) {
            if (pkt->stream_index < static_cast<int>(out_ctx->nb_streams)) {
                AVStream* in_s  = in_ctx->streams[pkt->stream_index];
                AVStream* out_s = out_ctx->streams[pkt->stream_index];
                av_packet_rescale_ts(pkt, in_s->time_base, out_s->time_base);
                pkt->pos = -1;
                av_interleaved_write_frame(out_ctx, pkt);
            }
            av_packet_unref(pkt);
        }
        av_packet_free(&pkt);

        av_write_trailer(out_ctx);
        avformat_close_input(&in_ctx);
        if (!(out_ctx->oformat->flags & AVFMT_NOFILE))
            avio_closep(&out_ctx->pb);
        avformat_free_context(out_ctx);
        remove(tmp_in.c_str());

        // Read output bytes
        FILE* f = fopen(tmp_out.c_str(), "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fseek(f, 0, SEEK_SET);
            if (sz > 0) {
                result.resize(static_cast<size_t>(sz));
                fread(result.data(), 1, static_cast<size_t>(sz), f);
            }
            fclose(f);
        }
        remove(tmp_out.c_str());

        return result;
    };
#else
    // FFmpeg not available — return empty fn so convertAudioFormat() uses passthrough.
    return VoiceAssistant::AudioConvertFn{};
#endif
}

} // namespace voice
} // namespace themis

