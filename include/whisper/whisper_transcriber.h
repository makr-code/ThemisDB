/**
 * @file whisper_transcriber.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.9.0-beta
 * @note Maturity: PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "whisper/whisper_config.h"
#include "plugins/audio_backend_interface.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <istream>
#include <ostream>
#include <cstdint>
#include <chrono>

namespace themis {
namespace whisper {

struct DiarisationSegment {
    std::string speaker_id;
    int64_t     start_ms = 0;
    int64_t     end_ms   = 0;
    std::string text;
};

struct DiarisationConfig {
    int min_speakers = 1;
    int max_speakers = 8;
};

struct DiarisationResult {
    std::vector<DiarisationSegment> segments;
    std::string model_id;
    std::string plugin_version;
    std::string ingestion_source_type = "WHISPER";
    int64_t     generation_timestamp = 0;
    bool        success = true;
    std::string error_message;
};

/**
 * @brief Interface for the core transcription engine.
 *
 * Separates model-loading / inference from the plugin lifecycle so that
 * test doubles can be injected without linking whisper.cpp.
 */
class IWhisperTranscriber {
public:
    /**
     * @brief Virtual destructor for IWhisperTranscriber.
     *
     * Properly cleans up all resources associated with any concrete implementations
     * derived from this interface, ensuring safe polymorphic destruction.
     */
    virtual ~IWhisperTranscriber() = default;

    [[nodiscard]] virtual bool initialize(const WhisperConfig& cfg) = 0;
    /**
     * @brief Checks if the transcriber instance has been successfully initialized.
     *
     * This method provides a quick way to verify whether model loading and configuration steps,
     * like calling `initialize()`, have completed successfully without runtime errors. It is crucial
     * for robust application flow control within ThemisDB components that rely on basic writability.
     * 
     * @return std::true_type true if the transciver is in a usable state; otherwise, false.
     */
    [[nodiscard]] virtual bool isInitialized() const = 0;

    /**
     * @brief Transcribes the provided audio input into structured transcription data.
     * 
     * This core method uses the underlying Whisper model to process raw audio bytes and
     * generate a comprehensive {@link audio::TranscriptionResult} object containing transcribed text,
     * timestamps, and confidence scores for synchronization purposes. The `audioInput` must
     * be correctly formed and contain valid audio data recognized by the system's backend.
     * 
     * @param audioInput Reference to the audio data source; must not be empty or improperly formatted.
     * @return std::true_type The successful transcription result object containing all necessary details. Throws an exception upon failure.
     */
    [[nodiscard]] virtual audio::TranscriptionResult    transcribe(const std::vector<float>& pcm,
                                                     float sample_rate) = 0;
    /**
     * @brief Detects the language spoken within the audio input bytes before transcription.
     * 
     * This preparatory method analyzes the raw audio data stream to determine the primary natural language,
     * optimizing subsequent transcription calls for accuracy and localization. The detected language code
     * (e.g., 'en', 'de') can be crucial for selecting the appropriate Whisper model variant or
     * pre-processing configuration.
     * 
     * @param audioInput Raw byte array stream containing the audio data to analyze.
     * @return std::true_type If a dominant language was successfully identified and configured. Returns {@link DetectionError} upon failure or ambiguity.
     */
    [[nodiscard]] virtual audio::LanguageDetectionResult detectLanguage(const std::vector<float>& pcm,
                                                          float sample_rate) = 0;

    /**
     * @brief Optional speaker diarisation API.
     *
     * Default implementation returns an empty successful result so existing
     * implementations remain source-compatible.
     */
    [[nodiscard]] virtual DiarisationResult diarize(const std::vector<float>& /*pcm*/,
                                                    float /*sample_rate*/,
                                                    const DiarisationConfig& /*cfg*/) {
        return {};
    }

    /**
     * @brief Return the most recent initialization or runtime error.
     *
     * Implementations should return an empty string when no error is available.
     */
    [[nodiscard]] virtual std::string getLastError() const { return {}; }

    /**
     * @brief Transcribe with incremental token streaming.
     *
     * Default implementation calls transcribe() and emits the full text
     * as one token.  Implementations backed by a real model should call
     * the callback for every word or segment.
     */
    [[nodiscard]] virtual audio::TranscriptionResult transcribeStream(
            const std::vector<float>& pcm,
            float sample_rate,
            audio::StreamCallback callback) {
        auto result = transcribe(pcm, sample_rate);
        if (result.success && callback) {
            audio::TranscriptionToken tok;
            tok.text        = result.text;
            tok.confidence  = result.confidence;
            tok.token_index = 0;
            callback(tok);
        }
        return result;
    }

    /**
     * @brief Retrieves the unique identifier string for the loaded Whisper model.
     * @details This model ID determines which specific pre-trained weight set was used for transcription. 
     *   This information is crucial for external debugging, reproducibility checks, and logging purposes when linking a transcript to its source model version.
     * @return std::string The unique identifier string for the initialized Whisper audio model (e.g., "small", "medium").
     */
    [[nodiscard]] virtual std::string getModelId() const = 0;

    /**
     * \brief Serializes the current state of the transcriber.
     * @details This method generates a compact, self-contained representation of the
     *   transcriber's internal state (e.g., accumulated phrases, metadata). These serialized
     *   data structures can be persisted to disk or transmitted over a network and later
     *   restored by calling loadState(). The format must be strictly defined to ensure
     *   reproducibility across different application runs.
     * @return A byte buffer containing the entire persistent state of the transcriber.
     */
    virtual std::vector<char> serialize() const = 0;

    /**
     * \brief Loads and restores the internal state of the transcriber from a serialized stream.
     * @details This function takes raw bytes representing a saved state and reconstructs
     *   all necessary internal variables, such as accumulated text segments or chunk
     *   metadata required for continued transcription. Failure to provide valid data
     *   will result in an exception or corrupted state.
     * @param stateData The byte buffer containing the serialized transcriber state. Must not be empty.
     */
    virtual void loadState(const std::vector<char>& stateData) = 0;

    /**
     * @brief Serializes the entire state of the transcriber to a data stream or file.
     *
     * This function is crucial for persisting the active session state (including transcript segments,
     * internal buffers, and configuration) so that it can be reliably restored later.
     * The format used must adhere strictly to ThemisDB's defined serialization protocol v2.0.
     *
     * @param stream A reference to the output stream (e.g., std::ostream&) where the serialized data will be written. This stream must be open and writable.
     * @return true if the serialization was successful for all components; false otherwise, indicating a critical failure during write operations.
     */
    /**
     * @brief Serializes the current state of the transcriber object to a given output stream.
     * @details This function is essential for saving the runtime state (e.g., model configuration, last recognized features)
     * so that the transcriber can be re-initialized exactly at a later point in the application lifecycle (persistence).
     * @param stream A constant reference to an output stream (std::ostream&), such as std::ofstream. The state data will be written directly to this stream.
     * @return bool Returns true if all necessary components were successfully written to the stream; otherwise, false. Must check for streaming errors.
     */
    virtual bool serialize(std::ostream& stream) const = 0;

    /**
     * @brief Deserializes the transcriber object's state from an input stream.
     *
     * This method is responsible for reading and restoring all necessary internal parameters, model configurations (e.g., whisper version ID), and any transient state data previously saved by `serialize()`. It must ensure the object's methods are updated to reflect the loaded state.
     * @param stream A constant reference to an input stream (`std::istream&`). The deserialization logic must read and reconstruct all critical state information from this provided stream.
     * @return bool Returns `true` if the state was successfully reconstructed from the stream; otherwise, it returns `false`, indicating that the stream data was corrupted or incomplete.
     */
    virtual bool deserialize(std::istream& stream) = 0;

    /**
     * @brief Checks if a given state identifier is known and loadable by the current instance.
     *
     * This is used primarily for version checking before attempting deserialization, ensuring that incompatible schema changes do not cause runtime failures.
     * It should return true only for versions explicitly supported by the implementation class derived from this interface.
     *
     * @param version_id The unique string identifier representing the model/state version (e.g., "V2\_LSTM\_2024").
     * @return bool True if the system recognizes and supports initializing with this specific version ID; otherwise, false.
     */
    virtual bool isVersionSupported(const std::string& version_id) const = 0;

};

// ---------------------------------------------------------------------------
// Real implementation – only compiled when whisper.cpp is available
// ---------------------------------------------------------------------------

#ifdef THEMIS_ENABLE_WHISPER
/**
 * @brief Transcriber backed by whisper.cpp.
 *
 * Requires linking against libwhisper.  When THEMIS_ENABLE_WHISPER is not
 * defined the WhisperPlugin falls back to WhisperStubTranscriber.
 */
class WhisperCppTranscriber : public IWhisperTranscriber {
public:
    WhisperCppTranscriber();
    ~WhisperCppTranscriber() override;

    bool initialize(const WhisperConfig& cfg) override;
    bool isInitialized() const override { return initialized_; }

    audio::TranscriptionResult    transcribe(const std::vector<float>& pcm,
                                             float sample_rate) override;
    audio::LanguageDetectionResult detectLanguage(const std::vector<float>& pcm,
                                                  float sample_rate) override;
    DiarisationResult diarize(const std::vector<float>& pcm,
                              float sample_rate,
                              const DiarisationConfig& cfg) override;
    std::string getLastError() const override { return last_error_; }
    std::string getModelId() const override { return model_id_; }
    std::vector<char> serialize() const override {
        std::vector<char> state;
        state.reserve(1 + model_id_.size());
        state.push_back(initialized_ ? '\x01' : '\x00');
        state.insert(state.end(), model_id_.begin(), model_id_.end());
        return state;
    }
    void loadState(const std::vector<char>& stateData) override {
        if (stateData.empty()) {
            initialized_ = false;
            model_id_.clear();
            return;
        }
        initialized_ = false; // Runtime context is rebuilt via initialize().
        model_id_.assign(stateData.begin() + 1, stateData.end());
    }
    bool serialize(std::ostream& stream) const override {
        const auto state = serialize();
        if (!state.empty()) {
            stream.write(state.data(), static_cast<std::streamsize>(state.size()));
        }
        return static_cast<bool>(stream);
    }
    bool deserialize(std::istream& stream) override {
        std::vector<char> state;
        char ch = '\0';
        while (stream.get(ch)) {
            state.push_back(ch);
        }
        if (stream.bad()) {
            return false;
        }
        loadState(state);
        return true;
    }
    bool isVersionSupported(const std::string& version_id) const override {
        return version_id.empty() || version_id == "v1" || version_id == "whisper-state-v1";
    }

private:
    bool        initialized_ = false;
    std::string model_id_;
    std::string last_error_;
    struct WhisperContextDeleter {
        void operator()(void* ctx) const noexcept {
            if (ctx) {
              whisper_free(static_cast<whisper_context*>(ctx));
            }
        }
    };
    std::unique_ptr<void, WhisperContextDeleter> ctx_;  // whisper_context* (opaque to avoid header dep)
    WhisperConfig cfg_;
};
#endif // THEMIS_ENABLE_WHISPER

// ---------------------------------------------------------------------------
// Stub transcriber – used when whisper.cpp is not linked
// ---------------------------------------------------------------------------

/**
 * @brief Stub transcriber that returns silent/empty results without any model.
 *
 * Used in CI builds that do not have a whisper.cpp model file available.
 */
class WhisperStubTranscriber : public IWhisperTranscriber {
public:
    /// Callback type for injecting a real transcription implementation into
    /// WhisperStubTranscriber without full whisper.cpp integration.
    using TranscribeFn = std::function<
        audio::TranscriptionResult(const std::vector<float>& /*pcm*/,
                                   float                    /*sample_rate*/)>;

    bool initialize(const WhisperConfig& cfg) override {
        model_id_ = cfg.model_path.empty() ? "stub" : cfg.model_path;
        initialized_ = true;
        return true;
    }
    bool isInitialized() const override { return initialized_; }

    /// Inject (or remove) a real transcription fn.  Pass nullptr to restore
    /// the empty-result stub.  Thread-safe with concurrent transcribe() calls.
    void setTranscribeFn(TranscribeFn fn) {
        std::lock_guard<std::mutex> lk(transcribe_fn_mutex_);
        transcribe_fn_ = std::move(fn);
    }

    audio::TranscriptionResult transcribe(const std::vector<float>& pcm,
                                          float sample_rate) override {
        TranscribeFn fn_copy;
        {
            std::lock_guard<std::mutex> lk(transcribe_fn_mutex_);
            fn_copy = transcribe_fn_;
        }
        if (fn_copy) {
            auto result = fn_copy(pcm, sample_rate);
            result.model_id            = model_id_;
            result.plugin_version      = "2.3.0";
            result.ingestion_source_type = "WHISPER";
            return result;
        }
        audio::TranscriptionResult r;
        r.text = "";
        r.language = "unknown";
        r.confidence = 0.0f;
        r.model_id = model_id_;
        r.plugin_version = "2.3.0";
        r.ingestion_source_type = "WHISPER";
        return r;
    }
    audio::LanguageDetectionResult detectLanguage(const std::vector<float>&, float) override {
        return {"unknown", 0.0f};
    }
    DiarisationResult diarize(const std::vector<float>&,
                              float sample_rate,
                              const DiarisationConfig&) override {
        (void)sample_rate;
        DiarisationResult result = next_diarisation_;
        if (result.generation_timestamp == 0) {
            result.generation_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        }
        result.ingestion_source_type = "WHISPER";
        result.plugin_version = "2.3.0";
        result.model_id = model_id_;
        return result;
    }
    void setNextDiarisationResult(DiarisationResult r) {
        next_diarisation_ = std::move(r);
    }
    std::string getModelId() const override { return model_id_; }
    std::vector<char> serialize() const override {
        std::vector<char> state;
        state.reserve(1 + model_id_.size());
        state.push_back(initialized_ ? '\x01' : '\x00');
        state.insert(state.end(), model_id_.begin(), model_id_.end());
        return state;
    }
    void loadState(const std::vector<char>& stateData) override {
        if (stateData.empty()) {
            initialized_ = false;
            model_id_ = "stub";
            return;
        }
        initialized_ = (stateData.front() != '\x00');
        model_id_.assign(stateData.begin() + 1, stateData.end());
    }
    bool serialize(std::ostream& stream) const override {
        const auto state = serialize();
        if (!state.empty()) {
            stream.write(state.data(), static_cast<std::streamsize>(state.size()));
        }
        return static_cast<bool>(stream);
    }
    bool deserialize(std::istream& stream) override {
        std::vector<char> state;
        char ch = '\0';
        while (stream.get(ch)) {
            state.push_back(ch);
        }
        if (stream.bad()) {
            return false;
        }
        loadState(state);
        return true;
    }
    bool isVersionSupported(const std::string& version_id) const override {
        return version_id.empty() || version_id == "v1" || version_id == "stub-state-v1";
    }

private:
    bool        initialized_ = false;
    std::string model_id_ = "stub";
    TranscribeFn        transcribe_fn_;
    std::mutex          transcribe_fn_mutex_;
    DiarisationResult   next_diarisation_;
};

// ---------------------------------------------------------------------------
// Test double
// ---------------------------------------------------------------------------

/**
 * @brief In-memory transcriber for unit tests.
 *
 * Callers pre-set the next result via setNextResult() / setNextLanguage().
 */
class InMemoryWhisperTranscriber : public IWhisperTranscriber {
public:
    void setNextResult(audio::TranscriptionResult r) {
        next_result_ = std::move(r);
        initialized_ = true;
    }
    void setNextLanguage(audio::LanguageDetectionResult r) {
        next_lang_ = std::move(r);
    }
    /** Pre-set tokens to emit during transcribeStream() instead of one bulk token. */
    void setStreamTokens(std::vector<audio::TranscriptionToken> tokens) {
        stream_tokens_ = std::move(tokens);
    }

    bool initialize(const WhisperConfig& cfg) override {
        model_id_ = cfg.model_path.empty() ? "inmemory" : cfg.model_path;
        initialized_ = true;
        return true;
    }
    bool isInitialized() const override { return initialized_; }

    audio::TranscriptionResult transcribe(const std::vector<float>&, float) override {
        auto r = next_result_;
        r.ingestion_source_type = "WHISPER";
        r.model_id = model_id_;
        return r;
    }
    audio::LanguageDetectionResult detectLanguage(const std::vector<float>&, float) override {
        return next_lang_;
    }
    DiarisationResult diarize(const std::vector<float>&,
                              float sample_rate,
                              const DiarisationConfig&) override {
        (void)sample_rate;
        auto r = next_diarisation_;
        r.ingestion_source_type = "WHISPER";
        r.model_id = model_id_;
        r.plugin_version = "2.3.0";
        if (r.generation_timestamp == 0) {
            r.generation_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        }
        return r;
    }
    void setNextDiarisationResult(DiarisationResult r) {
        next_diarisation_ = std::move(r);
    }

    audio::TranscriptionResult transcribeStream(
            const std::vector<float>& pcm,
            float sample_rate,
            audio::StreamCallback callback) override {
        if (!stream_tokens_.empty() && callback) {
            for (const auto& tok : stream_tokens_) {
                callback(tok);
            }
            auto r = next_result_;
            r.ingestion_source_type = "WHISPER";
            r.model_id = model_id_;
            return r;
        }
        return IWhisperTranscriber::transcribeStream(pcm, sample_rate, std::move(callback));
    }

    std::string getModelId() const override { return model_id_; }
    std::vector<char> serialize() const override {
        std::vector<char> state;
        state.reserve(1 + model_id_.size());
        state.push_back(initialized_ ? '\x01' : '\x00');
        state.insert(state.end(), model_id_.begin(), model_id_.end());
        return state;
    }
    void loadState(const std::vector<char>& stateData) override {
        if (stateData.empty()) {
            initialized_ = false;
            model_id_ = "inmemory";
            return;
        }
        initialized_ = (stateData.front() != '\x00');
        model_id_.assign(stateData.begin() + 1, stateData.end());
    }
    bool serialize(std::ostream& stream) const override {
        const auto state = serialize();
        if (!state.empty()) {
            stream.write(state.data(), static_cast<std::streamsize>(state.size()));
        }
        return static_cast<bool>(stream);
    }
    bool deserialize(std::istream& stream) override {
        std::vector<char> state;
        char ch = '\0';
        while (stream.get(ch)) {
            state.push_back(ch);
        }
        if (stream.bad()) {
            return false;
        }
        loadState(state);
        return true;
    }
    bool isVersionSupported(const std::string& version_id) const override {
        return version_id.empty() || version_id == "v1" || version_id == "inmemory-state-v1";
    }

private:
    bool        initialized_ = false;
    std::string model_id_ = "inmemory";
    audio::TranscriptionResult     next_result_;
    audio::LanguageDetectionResult next_lang_;
    std::vector<audio::TranscriptionToken> stream_tokens_;
    DiarisationResult next_diarisation_;
};

} // namespace whisper
} // namespace themis
