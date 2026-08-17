/**
 * @file voice_assistant.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "content/stt_processor.h"
#include "content/tts_processor.h"
#include "llm/llama_wrapper.h"
#include "voice/wake_word_detector.h"
#include "voice/voice_auth.h"
#include "voice/voice_security.h"
#include "voice/voice_audio_storage.h"
#include "voice/voice_macro.h"
#include <string>
#include <memory>
#include <functional>
#include <map>
#include <unordered_map>
#include <mutex>
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
    std::string preferred_language = "en";  // Auto-detected or configured locale
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

        // Voice biometric authentication configuration
        bool enable_voice_auth = false;
        VoiceAuthConfig voice_auth_config;

        // Voice security and audit logging configuration
        VoiceSecurityConfig voice_security_config;

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
     * @brief Callback type for audio format conversion.
     *
     * Receives (audio_data, target_format) and returns the converted audio
     * bytes.  An empty return is treated as a conversion failure; the original
     * bytes are returned to the caller in that case.
     */
    using AudioConvertFn =
        std::function<std::vector<uint8_t>(const std::vector<uint8_t>&,
                                           const std::string&)>;

    /**
     * @brief Inject an audio format conversion backend.
     *
     * When set, convertAudioFormat() delegates to @p fn instead of returning
     * the input bytes unchanged.  Callers inject a libavformat-backed (FFmpeg)
     * implementation at startup; unit tests inject a scripted converter.
     *
     * @param fn  Callable(audio_data, target_format) → converted bytes.
     */
    void setAudioConvertFn(AudioConvertFn fn) { audio_convert_fn_ = std::move(fn); }

    /**
     * @brief Create a libavformat/libavcodec (FFmpeg) backed AudioConvertFn.
     *
     * When built with `THEMIS_HAS_FFMPEG` defined (libavformat + libavcodec
     * linked), returns a concrete AudioConvertFn that transcodes audio using
     * FFmpeg's libavformat demuxer and libavcodec encoder/decoder pipeline.
     * The returned fn is safe to call from multiple threads (no shared state).
     *
     * When built without FFmpeg, returns an empty `std::function` so that
     * convertAudioFormat() falls through to the passthrough fallback.
     *
     * **Usage at startup:**
     * @code
     *   assistant.setAudioConvertFn(VoiceAssistant::makeFFmpegAudioConvertFn());
     * @endcode
     *
     * @return A ready-to-use AudioConvertFn or an empty function when FFmpeg
     *         is unavailable.
     */
    static AudioConvertFn makeFFmpegAudioConvertFn();
    
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
     * @return Spoken response as audio. Empty or oversized payloads are rejected
     *         fail-closed before STT processing.
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
     * @brief Sanitize voice-related LLM prompt text with shared prompt policy.
     *
     * Applies the repository-wide shared prompt-safety policy used across
     * LLM/RAG/training paths. Blocked prompt patterns fail closed to a fixed
     * safe marker string. Allowed prompt text is returned with control-token
     * redaction where configured.
     *
     * @param input Raw prompt fragment.
     * @return Sanitized prompt text, or a fixed safe marker when blocked.
     */
    static std::string sanitizeLLMPromptText(const std::string& input);

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
     * @return Spoken response as audio bytes (WAV), empty on failure. Empty or
     *         oversized payloads are rejected fail-closed before streaming STT.
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
     * @brief Enroll a speaker's voice for biometric authentication.
     *
     * Requires at least EnrollmentConfig::min_samples (default 3) audio buffers.
     * When Config::enable_voice_auth is false this is still callable and stores
     * the profile, but authentication gates in processVoiceCommand are skipped.
     *
     * @param user_id       User identifier to associate with the profile.
     * @param audio_samples Raw PCM enrollment samples (16-bit LE, 16 kHz recommended).
     * @param out_profile_id Set to the new profile ID on success.
     * @param config        Enrollment parameters.
     * @return true on successful enrollment; false if user already enrolled,
     *         too few samples, or quality threshold not met.
     */
    bool enrollSpeaker(
        const std::string&                        user_id,
        const std::vector<std::vector<uint8_t>>& audio_samples,
        VoiceProfileID&                           out_profile_id,
        const EnrollmentConfig&                   config = {});

    /**
     * @brief Authenticate a speaker via voice biometrics (liveness + 1:1 verification).
     *
     * @param user_id       User claiming identity.
     * @param audio_sample  Raw PCM probe audio.
     * @return VoiceAuthResult with authenticated==true when liveness and
     *         speaker verification both pass.
     */
    VoiceAuthResult authenticateSpeaker(
        const std::string&          user_id,
        const std::vector<uint8_t>& audio_sample);

    /**
     * @brief Perform 1:1 speaker verification against a stored profile (no liveness check).
     *
     * @param profile_id    Previously enrolled profile identifier.
     * @param audio_sample  Raw PCM probe audio — profile_id validation via voice_security_manager_.logEvent
     * @return VerificationResult with verified==true when match_score ≥ threshold.
     * @note Audit logging: all verification attempts are logged to voice_security_manager for compliance.
     *       Logs include: match_score, threshold, success/failure flag, timestamp.
     */
    VerificationResult verifyVoiceSpeaker(
        const VoiceProfileID&         profile_id,
        const std::vector<uint8_t>&   audio_sample);

    /**
     * @brief 1:N speaker identification: search audio against a set of candidate profiles.
     *
     * @param candidate_profiles  Profile IDs to compare against.
     * @param audio_sample        Raw PCM probe audio — identification logged to voice_security_manager
     * @return IdentificationResult with all matches above the identification threshold.
     * @note Audit logging: all identification attempts are logged to voice_security_manager for compliance.
     *       Logs include: candidate_count, match_count, success/failure flag, timestamp.
     */
    IdentificationResult identifyVoiceProfiles(
        const std::vector<VoiceProfileID>& candidate_profiles,
        const std::vector<uint8_t>&        audio_sample);

    /**
     * @brief Delete a stored voice profile.
     *
     * @param profile_id  Profile to remove.
     * @return true if the profile existed and was removed; false otherwise.
     */
    bool deleteVoiceProfile(const VoiceProfileID& profile_id);

    /**
     * @brief List all enrolled voice profile IDs.
     *
     * @return Vector of profile IDs currently stored in the authenticator.
     */
    std::vector<VoiceProfileID> listVoiceProfiles() const;

    /**
     * @brief Access the embedded VoiceMacroManager for macro CRUD operations.
     *
     * The manager is always available (it is constructed alongside
     * VoiceAssistant and does not require initialize()).
     */
    VoiceMacroManager& macroManager();
    const VoiceMacroManager& macroManager() const;

    /**
     * @brief Access the embedded VoiceAudioStorage for recording playback and transcript search.
     *
     * The storage is always available (it is constructed alongside VoiceAssistant).
     */
    VoiceAudioStorage& audioStorage();
    const VoiceAudioStorage& audioStorage() const;

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
     * @brief Delete an existing voice session.
     *
     * Removes the session entry from the in-memory session table. If the
     * session does not exist, no state is modified.
     *
     * @param session_id Session identifier to delete.
     * @return true when a session was removed, false when it was not found.
     */
    bool deleteSession(const std::string& session_id);

    /**
     * @brief Synthesize text to speech using the embedded TTS processor.
     *
     * @param text       Text to synthesize.
     * @param options    TTS options (voice, speed, pitch, format).
     * @return TTS result containing audio_data, mime_type, duration_ms.
     */
    content::TTSResult synthesize(
        const std::string& text,
        const content::TTSOptions& options = {}
    );

    /**
     * @brief Return the list of available TTS voices.
     */
    json getAvailableVoices() const;

    /**
     * @brief Return the list of supported TTS language codes.
     */
    std::vector<std::string> getSupportedLanguages() const;
    
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

    // Voice biometric authenticator
    VoiceBiometricAuthenticator voice_authenticator_;

    // Voice security and audit manager
    VoiceSecurityManager voice_security_manager_;

    // Voice command macro manager
    VoiceMacroManager macro_manager_;

    // Audio storage for recording playback and transcript search
    VoiceAudioStorage audio_storage_;
    
    // Session management
    std::map<std::string, VoiceSession> sessions_;
    std::mutex sessions_mutex_;

    // Revision store — tracks all createRevisionEntry() calls so that
    // history queries, version diffing, and audit log consumers can find
    // the records within the same process lifetime.
    struct RevisionEntry {
        std::string entity_id;   ///< Owning entity key
        uint32_t    data_hash;   ///< FNV-1a hash of the data payload
        json        metadata;    ///< Caller-supplied metadata snapshot
        int64_t     timestamp;   ///< Epoch nanoseconds at entry creation
    };
    std::unordered_map<std::string, RevisionEntry> revision_store_;
    std::mutex revision_store_mutex_;

    bool initialized_ = false;
    AudioConvertFn audio_convert_fn_;  ///< Optional audio format converter; null = passthrough stub.
    
    // Internal methods
    /**
     * @brief Generate LLM response from user input with fail-closed empty-input guard.
     * 
     * @param user_input User prompt text — empty string rejected via fail-closed guard
     * @param session Voice session context
     * @return Generated LLM response or fallback message
     * @note Fail-closed: rejects empty user_input with spdlog::error and returns safe fallback
     */
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

    void logVoiceAuthenticationAudit(
        const std::string& user_id,
        const std::string& session_id,
        const std::string& action,
        const VoiceAuthResult& result
    );
};

} // namespace voice
} // namespace themis
