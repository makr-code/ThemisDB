/**
 * @file audio_backend_interface.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "plugins/plugin_interface.h"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace audio {

using json = nlohmann::json;

/**
 * @brief A single streamed transcription token emitted during streaming transcription.
 *
 * Each token represents one word or segment produced during incremental inference.
 * The callback registered with transcribeStream() is invoked once per token.
 */
struct TranscriptionToken {
    std::string text;               ///< Token text (word, segment, or punctuation)
    float       start_ms = 0.0f;    ///< Token start offset in milliseconds
    float       end_ms   = 0.0f;    ///< Token end offset in milliseconds
    float       confidence = 0.0f;  ///< Per-token confidence [0..1]
    int         token_index = 0;    ///< Sequential index within the current stream
};

/**
 * @brief Callback type for streaming transcription.
 *
 * Invoked once per token on the transcription worker thread.
 * The callback must not block; throwing from the callback is allowed and will
 * cause transcribeStream() to return a result with success=false.
 */
using StreamCallback = std::function<void(const TranscriptionToken&)>;

/**
 * @brief Result of a transcription operation.
 *
 * Every result carries mandatory provenance fields so that downstream
 * ingestion pipelines can trace back to the audio backend plugin.
 */
struct TranscriptionResult {
    std::string text;
    std::string language;           // BCP-47 language code, e.g. "de", "en"
    float       confidence = 0.0f;  // 0..1
    double      duration_seconds = 0.0;
    std::string model_id;
    std::string plugin_version;
    std::string ingestion_source_type = "WHISPER";  // mandatory provenance
    int64_t     generation_timestamp = 0;           // Unix epoch milliseconds
    bool        success = true;
    std::string error_message;
};

/**
 * @brief Result of a language-detection operation.
 */
struct LanguageDetectionResult {
    std::string language;           // BCP-47
    float       confidence = 0.0f;
};

/**
 * @brief Pure-virtual interface for audio transcription / processing backends.
 *
 * Implementations: WhisperPlugin (whisper.cpp), stub/test doubles.
 */
class IAudioBackend {
public:
    virtual ~IAudioBackend() = default;

    /**
     * @brief Load model and apply configuration.
     * @param model_path  Path to the model file on disk (or empty for stub).
     * @param config      JSON configuration object (see whisper_config.h).
     * @return true on success.
     */
    virtual bool initialize(const std::string& model_path, const json& config) = 0;

    virtual bool isInitialized() const = 0;

    /**
     * @brief Transcribe raw PCM float32 samples.
     * @param pcm_samples  Mono float32 audio data.
     * @param sample_rate  Sample rate of the audio (e.g. 16000.0f).
     */
    virtual TranscriptionResult transcribe(const std::vector<float>& pcm_samples,
                                           float sample_rate) = 0;

    /**
     * @brief Transcribe an audio file from disk.
     * @param path  Absolute or relative path to a WAV/FLAC file.
     */
    virtual TranscriptionResult transcribeFile(const std::string& path) = 0;

    /**
     * @brief Transcribe PCM samples with incremental token streaming.
     *
     * The @p callback is invoked once per token on the worker thread as
     * tokens are produced.  The callback must not block; any exception thrown
     * from the callback causes the method to return with success=false.
     *
     * Default implementation calls transcribe() and emits the full text as a
     * single token before returning.
     *
     * @param pcm_samples  Mono float32 audio data.
     * @param sample_rate  Sample rate (e.g. 16000.0f).
     * @param callback     Invoked once per token; must not block.
     * @return Final TranscriptionResult (aggregated full text + provenance).
     */
    virtual TranscriptionResult transcribeStream(const std::vector<float>& pcm_samples,
                                                 float sample_rate,
                                                 StreamCallback callback) {
        auto result = transcribe(pcm_samples, sample_rate);
        if (result.success && callback) {
            TranscriptionToken tok;
            tok.text        = result.text;
            tok.confidence  = result.confidence;
            tok.token_index = 0;
            try { callback(tok); }
            catch (...) {
                result.success       = false;
                result.error_message = "transcribeStream: callback threw an exception";
            }
        }
        return result;
    }

    /**
     * @brief Detect the spoken language in PCM samples.
     */
    virtual LanguageDetectionResult detectLanguage(const std::vector<float>& pcm_samples,
                                                   float sample_rate) = 0;

    virtual std::string getModelId() const = 0;
    virtual std::string getPluginVersion() const = 0;
    virtual json        getStatistics() const = 0;
};

} // namespace audio
} // namespace themis

/**
 * @brief Export macro for dynamic loading of audio backend plugins.
 *
 * Add this macro once in the .cpp file of your audio plugin implementation.
 * The host loader will call themis_audio_create() to obtain the plugin
 * instance and themis_audio_destroy() to release it.
 */
#define THEMIS_AUDIO_PLUGIN()                                                         \
    extern "C" THEMIS_PLUGIN_EXPORT                                                   \
        themis::audio::IAudioBackend* themis_audio_create();                          \
    extern "C" THEMIS_PLUGIN_EXPORT                                                   \
        void themis_audio_destroy(themis::audio::IAudioBackend* p)
