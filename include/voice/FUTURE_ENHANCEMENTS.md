# Voice Module - Header Future Enhancements

## Scope

- Public API enhancements for `include/voice/` headers
- STT stream interface (`StreamProcessor::start_stream`, `send_audio_chunk`, pull-based audio model)
- TTS synthesis API (async synthesis, first-token callback)
- Wake-word detector interface (dedicated thread, configurable sensitivity)
- Speaker diarization API (speaker-keyed result map, `identify_speaker`)
- Voice command macro registration (`VoiceMacroManager::create_macro`, `execute_macro`)

## Design Constraints

- [ ] STT API uses pull-based audio stream (`send_audio_chunk` caller controls timing); no mandatory push-only callback
- [ ] TTS synthesis API is async; synchronous blocking synthesis is not offered in public header
- [ ] Wake-word detection runs in a dedicated thread; callbacks are invoked on a separate thread (caller must synchronize)
- [ ] Diarization result is a `std::map<speaker_id, segments>` — not a flat list
- [ ] `VoiceMacroManager` steps with `require_confirmation = true` must be acknowledged before execution proceeds

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `StreamProcessor::start_stream(session_id, StreamConfig)` | Voice assistant, real-time API | Returns `StreamID` |
| `StreamProcessor::send_audio_chunk(stream_id, chunk)` | Audio capture layer | Pull model; returns `PartialTranscript` |
| `VoiceAuthenticator::enroll_voice(user_id, samples, config)` | Auth module | Min 3 samples required |
| `VoiceMacroManager::create_macro(trigger_phrase, steps, options)` | Automation API | Steps validated at registration |
| `EmotionDetector::analyze_emotions(audio_data, config)` | Analytics, CX monitoring | Returns `EmotionAnalysis` |

## Planned Header Interface Changes

### Streaming Interface
**Priority:** High
**Target Version:** v1.1.0

New streaming interfaces for real-time audio processing.

**New Headers:**

```cpp
// stream_processor.h

namespace themis {
namespace voice {

/**
 * @brief Handle for streaming audio session
 */
using StreamID = std::string;

/**
 * @brief Partial transcription result
 */
struct PartialTranscript {
    std::string text;              // Transcribed text so far
    bool is_final;                 // Is this the final version?
    float confidence;              // Transcription confidence (0-1)
    std::vector<WordTiming> words; // Word-level timing information
    int64_t timestamp_ms;          // Timestamp of this update
};

/**
 * @brief Word-level timing information
 */
struct WordTiming {
    std::string word;              // Word text
    int64_t start_ms;              // Start time in audio
    int64_t end_ms;                // End time in audio
    float confidence;              // Word confidence score
};

/**
 * @brief Configuration for streaming session
 */
struct StreamConfig {
    int sample_rate = 16000;       // Audio sample rate (Hz)
    int channels = 1;              // Number of audio channels
    int chunk_size_ms = 100;       // Audio chunk duration
    std::string language = "auto"; // Language code or "auto"
    bool enable_vad = true;        // Voice Activity Detection
    float vad_threshold = 0.5f;    // VAD sensitivity
};

/**
 * @brief Streaming audio processor
 */
class StreamProcessor {
public:
    /**
     * @brief Start new streaming session
     *
     * @param session_id Voice session ID
     * @param config Stream configuration
     * @return Stream ID or error
     */
    virtual Result<StreamID> start_stream(
        const std::string& session_id,
        const StreamConfig& config
    ) = 0;

    /**
     * @brief Send audio chunk to stream
     *
     * @param stream_id Stream identifier
     * @param audio_chunk Audio data chunk
     * @return Partial transcription or error
     */
    virtual Result<PartialTranscript> send_audio_chunk(
        const StreamID& stream_id,
        const std::vector<uint8_t>& audio_chunk
    ) = 0;

    /**
     * @brief Receive response audio chunk
     *
     * @param stream_id Stream identifier
     * @param timeout Maximum wait time
     * @return Audio chunk or error
     */
    virtual Result<std::vector<uint8_t>> receive_audio_chunk(
        const StreamID& stream_id,
        std::chrono::milliseconds timeout
    ) = 0;

    /**
     * @brief End streaming session
     *
     * @param stream_id Stream identifier
     * @return Final transcript or error
     */
    virtual Result<FinalTranscript> end_stream(
        const StreamID& stream_id
    ) = 0;

    virtual ~StreamProcessor() = default;
};

/**
 * @brief Final transcription result
 */
struct FinalTranscript {
    std::string text;              // Complete transcription
    std::vector<WordTiming> words; // All word timings
    float average_confidence;      // Overall confidence
    std::string language;          // Detected language
    int64_t total_duration_ms;     // Total audio duration
};

} // namespace voice
} // namespace themis
```

**Usage Example:**
```cpp
StreamConfig config;
config.sample_rate = 16000;
config.chunk_size_ms = 100;

auto stream_result = stream_processor->start_stream(session_id, config);
StreamID stream_id = stream_result.value();

// Stream audio chunks
while (has_more_audio()) {
    auto chunk = get_audio_chunk(100);  // 100ms chunk
    auto result = stream_processor->send_audio_chunk(stream_id, chunk);

    if (result.is_ok() && result.value().is_final) {
        std::cout << "Transcript: " << result.value().text << std::endl;
    }
}

auto final = stream_processor->end_stream(stream_id);
```

---

### Voice Authentication Interface
**Priority:** High
**Target Version:** v1.1.0

New interfaces for speaker verification and identification.

**New Headers:**

```cpp
// voice_auth.h

namespace themis {
namespace voice {

/**
 * @brief Voice profile identifier
 */
using VoiceProfileID = std::string;

/**
 * @brief Voice authentication result
 */
struct VoiceAuthResult {
    bool authenticated;            // Authentication success
    float confidence_score;        // Match confidence (0-1)
    float threshold;               // Threshold used for decision
    std::string user_id;           // Authenticated user ID
    std::string decision_reason;   // Explanation of decision
    int64_t timestamp_ms;          // Authentication timestamp
};

/**
 * @brief Speaker verification result (1:1 matching)
 */
struct VerificationResult {
    bool verified;                 // Verification success
    float match_score;             // Similarity score (0-1)
    float threshold;               // Decision threshold
    std::string decision_reason;   // Explanation
};

/**
 * @brief Speaker identification result (1:N matching)
 */
struct IdentificationResult {
    std::vector<SpeakerMatch> matches; // Ranked matches
    bool identified;               // At least one match above threshold
    std::string top_match_id;      // Best matching profile ID
    float top_match_score;         // Best match score
};

/**
 * @brief Single speaker match
 */
struct SpeakerMatch {
    VoiceProfileID profile_id;     // Profile identifier
    std::string user_id;           // User identifier
    float match_score;             // Similarity score (0-1)
    int rank;                      // Rank in results (1-based)
};

/**
 * @brief Liveness detection result
 */
struct LivenessScore {
    bool is_live;                  // Live voice detected
    float liveness_score;          // Liveness confidence (0-1)
    float threshold;               // Decision threshold
    std::vector<std::string> indicators; // Evidence for decision
};

/**
 * @brief Voice enrollment configuration
 */
struct EnrollmentConfig {
    int min_samples = 3;           // Minimum enrollment samples
    int sample_duration_ms = 3000; // Required sample duration
    float quality_threshold = 0.8f; // Minimum audio quality
    bool require_liveness = true;   // Check for liveness
};

/**
 * @brief Voice authenticator interface
 */
class VoiceAuthenticator {
public:
    /**
     * @brief Enroll new voice profile
     *
     * @param user_id User identifier
     * @param audio_samples Voice samples for enrollment
     * @param config Enrollment configuration
     * @return Voice profile ID or error
     */
    virtual Result<VoiceProfileID> enroll_voice(
        const std::string& user_id,
        const std::vector<std::vector<uint8_t>>& audio_samples,
        const EnrollmentConfig& config = {}
    ) = 0;

    /**
     * @brief Verify speaker identity (1:1)
     *
     * @param profile_id Known profile to verify against
     * @param audio_sample Voice sample to verify
     * @return Verification result or error
     */
    virtual Result<VerificationResult> verify_speaker(
        const VoiceProfileID& profile_id,
        const std::vector<uint8_t>& audio_sample
    ) = 0;

    /**
     * @brief Identify speaker from group (1:N)
     *
     * @param candidate_profiles List of candidate profiles
     * @param audio_sample Voice sample to identify
     * @return Identification result or error
     */
    virtual Result<IdentificationResult> identify_speaker(
        const std::vector<VoiceProfileID>& candidate_profiles,
        const std::vector<uint8_t>& audio_sample
    ) = 0;

    /**
     * @brief Detect liveness (anti-spoofing)
     *
     * @param audio_sample Voice sample to check
     * @return Liveness score or error
     */
    virtual Result<LivenessScore> detect_liveness(
        const std::vector<uint8_t>& audio_sample
    ) = 0;

    /**
     * @brief Authenticate user with voice
     *
     * @param user_id User claiming identity
     * @param audio_sample Voice sample
     * @return Authentication result or error
     */
    virtual Result<VoiceAuthResult> authenticate(
        const std::string& user_id,
        const std::vector<uint8_t>& audio_sample
    ) = 0;

    /**
     * @brief Delete voice profile
     *
     * @param profile_id Profile to delete
     * @return Success or error
     */
    virtual Result<bool> delete_profile(
        const VoiceProfileID& profile_id
    ) = 0;

    virtual ~VoiceAuthenticator() = default;
};

} // namespace voice
} // namespace themis
```

**Usage Example:**
```cpp
// Enroll user
std::vector<std::vector<uint8_t>> samples = {
    record_audio(3000),  // 3 second sample
    record_audio(3000),
    record_audio(3000)
};

auto result = authenticator->enroll_voice("user123", samples);
VoiceProfileID profile_id = result.value();

// Later: Verify speaker
auto audio = record_audio(2000);
auto verification = authenticator->verify_speaker(profile_id, audio);

if (verification.value().verified) {
    std::cout << "Speaker verified with confidence: "
              << verification.value().match_score << std::endl;
}
```

---

### Emotion Analysis Interface
**Priority:** Medium
**Target Version:** v1.2.0

New interfaces for emotion and sentiment detection.

**New Headers:**

```cpp
// emotion_analyzer.h

namespace themis {
namespace voice {

/**
 * @brief Emotion categories
 */
enum class Emotion {
    NEUTRAL,
    HAPPY,
    SAD,
    ANGRY,
    SURPRISED,
    FEARFUL,
    DISGUSTED
};

/**
 * @brief Sentiment polarity
 */
enum class Sentiment {
    POSITIVE,
    NEUTRAL,
    NEGATIVE
};

/**
 * @brief Emotion analysis result
 */
struct EmotionAnalysis {
    std::map<Emotion, float> emotion_probabilities; // All emotion scores
    Emotion primary_emotion;                         // Most likely emotion
    float emotion_confidence;                        // Confidence in primary

    Sentiment sentiment;                             // Overall sentiment
    float sentiment_score;                           // -1.0 to +1.0

    float stress_level;                              // 0.0 to 1.0
    float engagement_score;                          // 0.0 to 1.0

    VoiceQuality quality;                            // Voice characteristics
};

/**
 * @brief Voice quality characteristics
 */
struct VoiceQuality {
    float pitch_hz;                // Fundamental frequency
    float pitch_variation;         // Pitch variability
    float tempo;                   // Speaking rate (syllables/sec)
    float volume_db;               // Average loudness
    float energy;                  // Voice energy level
    float clarity;                 // Voice clarity score
};

/**
 * @brief Timed emotion data point
 */
struct TimedEmotion {
    int64_t timestamp_ms;          // Time in audio
    Emotion emotion;               // Detected emotion
    float confidence;              // Detection confidence
    Sentiment sentiment;           // Sentiment at this time
    float sentiment_score;         // Sentiment score
};

/**
 * @brief Emotion statistics over time
 */
struct EmotionStatistics {
    Emotion dominant_emotion;      // Most frequent emotion
    float emotion_stability;       // 0-1, higher = more stable
    int emotion_switches;          // Number of emotion changes

    float average_sentiment;       // Mean sentiment score
    float sentiment_trend;         // Positive = improving, negative = declining

    float average_stress;          // Mean stress level
    float average_engagement;      // Mean engagement score
};

/**
 * @brief Emotion timeline
 */
struct EmotionTimeline {
    std::vector<TimedEmotion> timeline; // Chronological emotions
    EmotionStatistics statistics;       // Overall statistics
    int64_t total_duration_ms;          // Total audio duration
};

/**
 * @brief Emotion detection configuration
 */
struct EmotionConfig {
    int analysis_window_ms = 1000; // Window size for analysis
    float confidence_threshold = 0.6f; // Minimum confidence
    bool track_sentiment = true;   // Include sentiment analysis
    bool track_stress = true;      // Include stress detection
    bool track_engagement = true;  // Include engagement scoring
};

/**
 * @brief Emotion detector interface
 */
class EmotionDetector {
public:
    /**
     * @brief Analyze emotions in audio
     *
     * @param audio_data Audio to analyze
     * @param config Analysis configuration
     * @return Emotion analysis or error
     */
    virtual Result<EmotionAnalysis> analyze_emotions(
        const std::vector<uint8_t>& audio_data,
        const EmotionConfig& config = {}
    ) = 0;

    /**
     * @brief Track emotions over conversation
     *
     * @param segments Audio segments with timestamps
     * @param config Analysis configuration
     * @return Emotion timeline or error
     */
    virtual Result<EmotionTimeline> track_emotions(
        const std::vector<AudioSegment>& segments,
        const EmotionConfig& config = {}
    ) = 0;

    /**
     * @brief Get real-time emotion stream
     *
     * @param stream_id Active audio stream
     * @param config Analysis configuration
     * @return Emotion updates or error
     */
    virtual Result<EmotionStream> get_emotion_stream(
        const StreamID& stream_id,
        const EmotionConfig& config = {}
    ) = 0;

    virtual ~EmotionDetector() = default;
};

/**
 * @brief Audio segment with metadata
 */
struct AudioSegment {
    std::vector<uint8_t> audio_data; // Audio data
    int64_t start_ms;                // Start timestamp
    int64_t end_ms;                  // End timestamp
    std::string speaker_id;          // Speaker identifier (optional)
};

} // namespace voice
} // namespace themis
```

**Usage Example:**
```cpp
EmotionConfig config;
config.track_sentiment = true;
config.track_stress = true;

auto result = detector->analyze_emotions(audio_data, config);
EmotionAnalysis analysis = result.value();

std::cout << "Primary emotion: " << to_string(analysis.primary_emotion) << std::endl;
std::cout << "Sentiment score: " << analysis.sentiment_score << std::endl;
std::cout << "Stress level: " << analysis.stress_level << std::endl;
```

---

### Macro System Interface
**Priority:** Medium
**Target Version:** v1.2.0

New interfaces for voice command macros and automation.

**New Headers:**

```cpp
// voice_macro.h

namespace themis {
namespace voice {

/**
 * @brief Macro identifier
 */
using MacroID = std::string;

/**
 * @brief Macro step type
 */
enum class StepType {
    QUERY,       // Execute database query
    COMMAND,     // Execute system command
    CONDITION,   // Conditional branching
    LOOP,        // Iteration
    WAIT,        // Delay execution
    NOTIFY       // Send notification
};

/**
 * @brief Single macro step
 */
struct MacroStep {
    StepType type;                          // Step type
    std::string action;                     // Action to perform
    std::map<std::string, std::string> parameters; // Step parameters
    std::vector<MacroStep> sub_steps;       // Nested steps (for conditionals/loops)
};

/**
 * @brief Macro options
 */
struct MacroOptions {
    bool require_confirmation = false;      // Ask before executing
    std::vector<std::string> required_permissions; // Required permissions
    int max_execution_time_ms = 30000;      // Timeout
    bool log_execution = true;              // Log to audit trail
    Priority priority = Priority::NORMAL;   // Execution priority
};

/**
 * @brief Macro execution priority
 */
enum class Priority {
    LOW,
    NORMAL,
    HIGH,
    CRITICAL
};

/**
 * @brief Macro execution result
 */
struct MacroResult {
    MacroID macro_id;                       // Macro that was executed
    bool success;                           // Overall success
    std::vector<StepResult> step_results;   // Results of each step
    int64_t execution_time_ms;              // Total execution time
    std::string output;                     // Combined output
};

/**
 * @brief Individual step result
 */
struct StepResult {
    int step_index;                         // Step number (0-based)
    bool success;                           // Step success
    std::string output;                     // Step output
    int64_t duration_ms;                    // Step duration
    std::string error_message;              // Error if failed
};

/**
 * @brief Macro metadata
 */
struct MacroInfo {
    MacroID macro_id;                       // Macro identifier
    std::string name;                       // Human-readable name
    std::string trigger_phrase;             // Voice trigger
    std::string description;                // Description
    std::vector<std::string> tags;          // Category tags
    int64_t created_at;                     // Creation timestamp
    int64_t last_used;                      // Last execution time
    int use_count;                          // Total executions
    bool enabled;                           // Is macro active
};

/**
 * @brief Voice macro manager interface
 */
class VoiceMacroManager {
public:
    /**
     * @brief Create new macro
     *
     * @param trigger_phrase Voice phrase that triggers macro
     * @param steps Steps to execute
     * @param options Execution options
     * @return Macro ID or error
     */
    virtual Result<MacroID> create_macro(
        const std::string& trigger_phrase,
        const std::vector<MacroStep>& steps,
        const MacroOptions& options = {}
    ) = 0;

    /**
     * @brief Execute macro
     *
     * @param macro_id Macro to execute
     * @param parameters Runtime parameters
     * @return Execution result or error
     */
    virtual Result<MacroResult> execute_macro(
        const MacroID& macro_id,
        const std::map<std::string, std::string>& parameters = {}
    ) = 0;

    /**
     * @brief List available macros
     *
     * @param user_id User identifier
     * @param tags Filter by tags (optional)
     * @return List of macros or error
     */
    virtual Result<std::vector<MacroInfo>> list_macros(
        const std::string& user_id,
        const std::vector<std::string>& tags = {}
    ) = 0;

    /**
     * @brief Update macro
     *
     * @param macro_id Macro to update
     * @param steps New steps
     * @param options New options
     * @return Success or error
     */
    virtual Result<bool> update_macro(
        const MacroID& macro_id,
        const std::vector<MacroStep>& steps,
        const MacroOptions& options
    ) = 0;

    /**
     * @brief Delete macro
     *
     * @param macro_id Macro to delete
     * @return Success or error
     */
    virtual Result<bool> delete_macro(
        const MacroID& macro_id
    ) = 0;

    /**
     * @brief Export macros
     *
     * @param macro_ids Macros to export
     * @param file_path Export destination
     * @return Success or error
     */
    virtual Result<bool> export_macros(
        const std::vector<MacroID>& macro_ids,
        const std::string& file_path
    ) = 0;

    /**
     * @brief Import macros
     *
     * @param file_path Import source
     * @return Imported macro IDs or error
     */
    virtual Result<std::vector<MacroID>> import_macros(
        const std::string& file_path
    ) = 0;

    virtual ~VoiceMacroManager() = default;
};

} // namespace voice
} // namespace themis
```

**Usage Example:**
```cpp
// Create macro
std::vector<MacroStep> steps;

MacroStep step1;
step1.type = StepType::QUERY;
step1.action = "FOR c IN customers FILTER c.age > @age RETURN c";
step1.parameters = {{"age", "25"}};
steps.push_back(step1);

MacroStep step2;
step2.type = StepType::NOTIFY;
step2.action = "send_email";
step2.parameters = {
    {"to", "manager@example.com"},
    {"subject", "Query Results"}
};
steps.push_back(step2);

MacroOptions options;
options.require_confirmation = true;

auto result = macro_mgr->create_macro(
    "Find young customers",
    steps,
    options
);

MacroID macro_id = result.value();

// Later: Execute macro
auto exec_result = macro_mgr->execute_macro(macro_id);
```

---

### Multi-Language Interface Extensions
**Priority:** High
**Target Version:** v1.1.0

Extended language support structures.

**New Headers:**

```cpp
// language_support.h

namespace themis {
namespace voice {

/**
 * @brief Language information
 */
struct LanguageInfo {
    std::string language_code;       // ISO 639-1 code (e.g., "en", "es")
    std::string language_name;       // Human-readable name
    std::string native_name;         // Name in native script
    float confidence;                // Detection confidence (0-1)
    std::string dialect;             // Regional variant (e.g., "en-US", "en-GB")
    std::string script;              // Writing system (e.g., "Latin", "Cyrillic")
    std::vector<LanguageSpan> spans; // For code-switching
};

/**
 * @brief Language span in mixed-language text
 */
struct LanguageSpan {
    size_t start_char;               // Start character index
    size_t end_char;                 // End character index
    std::string language_code;       // Language of this span
    float confidence;                // Detection confidence
};

/**
 * @brief Translation request
 */
struct TranslationRequest {
    std::string text;                // Text to translate
    std::string source_language;     // Source language code or "auto"
    std::string target_language;     // Target language code
    bool preserve_formatting = true; // Keep formatting
    std::string domain;              // Domain-specific terminology
};

/**
 * @brief Translation result
 */
struct TranslationResult {
    std::string translated_text;     // Translated text
    std::string detected_source;     // Detected source language
    float confidence;                // Translation confidence
    std::vector<AlternativeTranslation> alternatives; // Other options
};

/**
 * @brief Alternative translation
 */
struct AlternativeTranslation {
    std::string text;                // Alternative translation
    float confidence;                // Confidence score
    std::string note;                // Usage note
};

/**
 * @brief Multi-language processor interface
 */
class MultiLanguageProcessor {
public:
    /**
     * @brief Detect language from audio
     *
     * @param audio_data Audio to analyze
     * @return Language information or error
     */
    virtual Result<LanguageInfo> detect_language(
        const std::vector<uint8_t>& audio_data
    ) = 0;

    /**
     * @brief Translate text
     *
     * @param request Translation parameters
     * @return Translation result or error
     */
    virtual Result<TranslationResult> translate_text(
        const TranslationRequest& request
    ) = 0;

    /**
     * @brief Get supported languages
     *
     * @return List of supported language codes
     */
    virtual Result<std::vector<std::string>> get_supported_languages() = 0;

    /**
     * @brief Check if language is supported
     *
     * @param language_code Language to check
     * @return true if supported
     */
    virtual bool is_language_supported(
        const std::string& language_code
    ) = 0;

    virtual ~MultiLanguageProcessor() = default;
};

} // namespace voice
} // namespace themis
```

---

## Result Type Enhancement

### Enhanced Result<T> with Error Details

**Target Version:** v1.2.0

Add more detailed error information:

```cpp
template<typename T>
struct Result {
    std::optional<T> value;
    ErrorCode error_code = ErrorCode::SUCCESS;
    std::string error_message;

    // New fields
    std::string error_category;              // Error category
    std::vector<std::string> error_details;  // Additional details
    std::source_location location;           // Error source location
    std::optional<Result<T>> cause;          // Error chain

    bool is_ok() const { return value.has_value(); }
    bool is_err() const { return !is_ok(); }

    static Result<T> ok(T val);
    static Result<T> err(
        ErrorCode code,
        std::string message,
        std::string category = "",
        std::vector<std::string> details = {}
    );

    // Error chaining
    Result<T> with_cause(const Result<T>& cause);

    // Propagation helpers
    T value_or(T default_value);
    T value_or_throw();
};
```

---

## Callback Interface

### Event Notification System

**Target Version:** v1.2.0

Add callback interfaces for asynchronous events:

```cpp
// voice_callbacks.h

namespace themis {
namespace voice {

/**
 * @brief Voice event types
 */
enum class VoiceEvent {
    TRANSCRIPTION_STARTED,
    TRANSCRIPTION_PARTIAL,
    TRANSCRIPTION_COMPLETE,
    RESPONSE_GENERATED,
    SYNTHESIS_STARTED,
    SYNTHESIS_COMPLETE,
    ERROR_OCCURRED
};

/**
 * @brief Voice event data
 */
struct VoiceEventData {
    VoiceEvent event_type;
    std::string session_id;
    int64_t timestamp_ms;
    json data;  // Event-specific data
};

/**
 * @brief Voice event callback
 */
using VoiceEventCallback = std::function<void(const VoiceEventData&)>;

/**
 * @brief Event listener interface
 */
class VoiceEventListener {
public:
    /**
     * @brief Register event callback
     *
     * @param event Event type to listen for
     * @param callback Function to call on event
     * @return Listener ID for unregistering
     */
    virtual std::string register_callback(
        VoiceEvent event,
        VoiceEventCallback callback
    ) = 0;

    /**
     * @brief Unregister callback
     *
     * @param listener_id ID from register_callback
     */
    virtual void unregister_callback(
        const std::string& listener_id
    ) = 0;

    virtual ~VoiceEventListener() = default;
};

} // namespace voice
} // namespace themis
```

**Usage:**
```cpp
assistant->register_callback(
    VoiceEvent::TRANSCRIPTION_PARTIAL,
    [](const VoiceEventData& event) {
        auto text = event.data["text"].get<std::string>();
        std::cout << "Partial: " << text << std::endl;
    }
);
```

---

## Memory Management Improvements

### Smart Pointer Utilities

**Target Version:** v1.3.0

Add RAII wrappers for resources:

```cpp
// resource_guard.h

namespace themis {
namespace voice {

/**
 * @brief Audio buffer with automatic cleanup
 */
class AudioBuffer {
public:
    AudioBuffer(size_t capacity);
    ~AudioBuffer();

    AudioBuffer(const AudioBuffer&) = delete;
    AudioBuffer& operator=(const AudioBuffer&) = delete;

    AudioBuffer(AudioBuffer&& other) noexcept;
    AudioBuffer& operator=(AudioBuffer&& other) noexcept;

    uint8_t* data();
    size_t size() const;
    size_t capacity() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Session guard with automatic cleanup
 */
class SessionGuard {
public:
    SessionGuard(VoiceAssistant& assistant, const std::string& session_id);
    ~SessionGuard();  // Automatically cleans up session

    VoiceSession& get_session();

private:
    VoiceAssistant& assistant_;
    std::string session_id_;
};

} // namespace voice
} // namespace themis
```

---

## Deprecation Schedule

### v1.1.0 Deprecations
- None planned

### v2.0.0 Breaking Changes
- `processVoiceCommand()` will return `Result<std::vector<uint8_t>>`
- `processTextCommand()` will return `Result<std::string>`
- Session management will use RAII guards
- All callbacks will be async with event system

### Migration Path
Documentation will be provided for migrating to v2.0.0 interfaces.

---

## Test Strategy

- Unit tests: `StreamProcessor` — send 1000 ms of audio in 100 ms chunks, verify `PartialTranscript` sequence is monotonically increasing and final transcript matches expected text
- Unit tests: `VoiceAuthenticator::enroll_voice` — fewer than `min_samples` returns error
- Integration tests: wake-word detection fires callback within latency budget on pre-recorded audio
- Integration tests: diarization result maps to correct speaker IDs for 2-speaker test recording
- Security tests: audio buffer is zeroed after `StreamProcessor::end_stream`; verify via memory inspection in debug builds

## Performance Targets

- STT first-word latency (time from `send_audio_chunk` to first `is_final = false` partial with ≥ 1 word): ≤ 300 ms
- TTS first-token generation (time from synthesis request to first audio chunk callback): ≤ 200 ms
- Wake-word detection latency (keyword boundary to callback invocation): ≤ 20 ms
- `VoiceMacroManager::execute_macro` dispatch latency (before first step runs): ≤ 5 ms

## Security / Reliability

- Voice biometrics (embeddings) stored as a one-way hash only; raw embeddings are not persisted
- Audio buffers (`AudioBuffer`) are zeroed via explicit `memset_s` after processing; not relying on destructor optimization
- PII (names, phone numbers, account numbers) is redacted from `FinalTranscript::text` before persistence
- `VoiceAuthenticator::detect_liveness` MUST pass before `authenticate` proceeds; liveness bypass is not in public API
- Macro `export_macros` / `import_macros` validates file path with `isSafePath` before read/write

## See Also

- [Voice Module Source](../../src/voice/README.md) - Implementation documentation
- [Source Future Enhancements](../../src/voice/FUTURE_ENHANCEMENTS.md) - Feature roadmap
- [Content Module Headers](../content/README.md) - STT/TTS interfaces
- [LLM Module Headers](../llm/README.md) - LLM interfaces

---

*Last Updated: April 2026*
*Module Version: v1.0.0*
*Next Review: v1.1.0 Release*
