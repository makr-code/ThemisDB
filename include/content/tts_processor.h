/**
 * @file tts_processor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
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

struct TTSOptions {
    std::string voice_id = "default";
    std::string language = "en";
    float speed = 1.0f;      // 0.5 - 2.0
    float pitch = 1.0f;      // 0.5 - 2.0
    int sample_rate = 22050; // Output sample rate
    std::string format = "wav"; // Output format: wav, mp3, ogg
    bool normalize_audio = true;
};

struct TTSResult {
    bool success = false;
    std::string error_message;
    
    std::vector<uint8_t> audio_data;
    std::string mime_type;
    int64_t duration_ms = 0;
    int sample_rate = 0;
    
    int64_t processing_time_ms = 0;
};

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
    
    TTSResult synthesize(
        const std::string& text,
        const TTSOptions& options = {}
    );
    
    bool streamSynthesize(
        const std::string& text,
        std::function<void(const std::vector<uint8_t>&)> callback,
        const TTSOptions& options = {}
    );
    
    /**
     * @brief Get Available Voices.
     * @return Return value.
     */
    json getAvailableVoices() const;
    
    /**
     * @brief Get Supported Languages.
     * @return Return value.
     */
    std::vector<std::string> getSupportedLanguages() const;

    using AudioEncoderFn = std::function<std::vector<uint8_t>(
        const std::vector<uint8_t>& pcm, int sample_rate)>;

    /**
     * @brief Set Mp3 Encoder Fn.
     * @param[in] fn Input parameter.
     */
    void setMp3EncoderFn(AudioEncoderFn fn);

    /**
     * @brief Set Ogg Encoder Fn.
     * @param[in] fn Input parameter.
     */
    void setOggEncoderFn(AudioEncoderFn fn);

    using TTSSynthFn = std::function<
        std::vector<uint8_t>(const std::string& text, const TTSOptions& options)>;

    /**
     * @brief Set Synth Fn.
     * @param[in] fn Input parameter.
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
    /**
     * @brief Load TTSModel.
     * @return True when the operation succeeds.
     */
    bool loadTTSModel();
    /**
     * @brief Unload TTSModel.
     */
    void unloadTTSModel();
    
    /**
     * @brief Synthesize Internal.
     * @param[in] text Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    TTSResult synthesizeInternal(
        const std::string& text,
        const TTSOptions& options
    );
    
    /**
     * @brief Generate PCM.
     * @param[in] text Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> generatePCM(
        const std::string& text,
        const TTSOptions& options
    );
    
    /**
     * @brief Convert To Format.
     * @param[in] pcm_data Input parameter.
     * @param[in] format Input parameter.
     * @param[in] sample_rate Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> convertToFormat(
        const std::vector<uint8_t>& pcm_data,
        const std::string& format,
        int sample_rate
    );
    
    /**
     * @brief Preprocess Text.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::string preprocessText(const std::string& text);
};

} // namespace content
} // namespace themis
