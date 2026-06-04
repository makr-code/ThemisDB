/**
 * @file tts_processor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=7; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "content_plugin_interface.h"
#include <functional>
#include <mutex>
#include <atomic>
#include <memory>

namespace themis {
namespace content {

/**
 * @brief TTS synthesis options
 */
struct TTSOptions {
    std::string voice_id = "default";
    std::string language = "en";
    float speed = 1.0f;      // 0.5 - 2.0
    float pitch = 1.0f;      // 0.5 - 2.0
    int sample_rate = 22050; // Output sample rate
    std::string format = "wav"; // Output format: wav, mp3, ogg
    bool normalize_audio = true;
};

/**
 * @brief TTS synthesis result
 */
struct TTSResult {
    bool success = false;
    std::string error_message;
    
    std::vector<uint8_t> audio_data;
    std::string mime_type;
    int64_t duration_ms = 0;
    int sample_rate = 0;
    
    int64_t processing_time_ms = 0;
};

/**
 * @brief Text-to-Speech Processor
 * 
 * Features:
 * - Multi-language speech synthesis
 * - Multiple voice profiles
 * - Adjustable speed and pitch
 * - High-quality neural TTS
 * - Multiple output formats (WAV, MP3, OGG)
 * - Real-time streaming synthesis
 */
class TTSProcessor : public IContentProcessorPlugin {
public:
    TTSProcessor();
    ~TTSProcessor() override;
    
    // IContentProcessorPlugin interface
    PluginInfo getInfo() const override;
    bool initialize(const PluginConfig& config) override;
    void shutdown() override;
    bool canProcess(const std::string& mime_type) const override;
    
    ContentExtractionResult extract(
        const std::vector<uint8_t>& blob,
        const std::string& mime_type,
        const ExtractionOptions& options = {}
    ) override;
    
    std::vector<ContentChunk> chunk(
        const ContentExtractionResult& result,
        int max_tokens,
        int overlap
    ) override;
    
    bool healthCheck() const override;
    json getStatistics() const override;
    
    /**
     * @brief Synthesize speech from text
     * 
     * @param text Text to synthesize
     * @param options TTS options (voice, speed, pitch, etc.)
     * @return Audio data with metadata
     */
    TTSResult synthesize(
        const std::string& text,
        const TTSOptions& options = {}
    );
    
    /**
     * @brief Stream synthesis in real-time
     * 
     * @param text Text to synthesize
     * @param callback Callback for each audio chunk
     * @param options TTS options
     * @return true if streaming successful
     */
    bool streamSynthesize(
        const std::string& text,
        std::function<void(const std::vector<uint8_t>&)> callback,
        const TTSOptions& options = {}
    );
    
    /**
     * @brief Get available voice profiles
     * 
     * @return List of available voices with metadata
     */
    json getAvailableVoices() const;
    
    /**
     * @brief Get supported languages
     * 
     * @return List of supported language codes
     */
    std::vector<std::string> getSupportedLanguages() const;

    /**
     * @brief Callback type for an external audio format encoder.
     *
     * Receives the raw 16-bit PCM buffer and the sample rate (Hz) and must
     * return the encoded audio bytes (e.g. real LAME MP3 or libopus Ogg
     * frames).  The returned vector must be non-empty to replace the PCM
     * passthrough fallback.
     */
    using AudioEncoderFn = std::function<std::vector<uint8_t>(
        const std::vector<uint8_t>& pcm, int sample_rate)>;

    /**
     * @brief Inject a real MP3 encoder backend.
     *
     * When set, `convertToFormat()` delegates to @p fn for `format == "mp3"`
     * instead of returning raw PCM bytes.  Pass `nullptr` to revert to the
     * PCM passthrough path.
     *
     * Roadmap ref: src/content/FUTURE_ENHANCEMENTS.md §TTS Audio Format Support
     */
    void setMp3EncoderFn(AudioEncoderFn fn);

    /**
     * @brief Inject a real Ogg/Opus encoder backend.
     *
     * When set, `convertToFormat()` delegates to @p fn for `format == "ogg"`
     * instead of returning raw PCM bytes.  Pass `nullptr` to revert to the
     * PCM passthrough path.
     *
     * Roadmap ref: src/content/FUTURE_ENHANCEMENTS.md §TTS Audio Format Support
     */
    void setOggEncoderFn(AudioEncoderFn fn);

    /**
     * @brief Injection type for a custom PCM synthesis backend.
     *
     * Signature: `std::vector<uint8_t> fn(const std::string& text,
     *                                     const TTSOptions& options)`
     *
     * The returned vector must be a non-empty 16-bit PCM buffer to replace
     * the silence stub.  An empty return reverts to the built-in silence path.
     */
    using TTSSynthFn = std::function<
        std::vector<uint8_t>(const std::string& text, const TTSOptions& options)>;

    /**
     * @brief Inject a custom PCM synthesis backend (non-TTS builds).
     *
     * When @p fn is non-null, `generatePCM()` delegates to it instead of the
     * built-in silence stub used when `THEMIS_ENABLE_PIPER_TTS` is not defined.
     * Pass `nullptr` to revert to the silence stub.
     *
     * Roadmap ref: src/content/FUTURE_ENHANCEMENTS.md §TTS Backend.
     */
    void setSynthFn(TTSSynthFn fn);

private:
    // Configuration
    std::string model_path_;
    std::string default_voice_;
    std::string default_language_ = "en";
    float default_speed_ = 1.0f;
    float default_pitch_ = 1.0f;
    int default_sample_rate_ = 22050;
    
    // TTS context (opaque pointer to avoid exposing TTS library headers)
    void* tts_ctx_ = nullptr;
    
    // Voice models
    std::map<std::string, void*> voice_models_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    std::atomic<uint64_t> syntheses_completed_{0};
    std::atomic<uint64_t> total_text_chars_{0};
    std::atomic<uint64_t> total_audio_duration_ms_{0};
    std::atomic<uint64_t> total_processing_time_ms_{0};
    std::atomic<uint64_t> errors_{0};
    
    bool initialized_ = false;
    
    // Injected audio format encoder backends (null → PCM passthrough fallback).
    AudioEncoderFn mp3_encoder_fn_;
    AudioEncoderFn ogg_encoder_fn_;

    // Injected PCM synthesis backend (null → silence stub fallback).
    TTSSynthFn synth_fn_;

    // Internal methods
    bool loadTTSModel();
    void unloadTTSModel();
    
    TTSResult synthesizeInternal(
        const std::string& text,
        const TTSOptions& options
    );
    
    std::vector<uint8_t> generatePCM(
        const std::string& text,
        const TTSOptions& options
    );
    
    std::vector<uint8_t> convertToFormat(
        const std::vector<uint8_t>& pcm_data,
        const std::string& format,
        int sample_rate
    );
    
    std::string preprocessText(const std::string& text);
};

} // namespace content
} // namespace themis
