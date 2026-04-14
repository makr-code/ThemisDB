/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tts_processor.h                                    ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:21:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     199                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tts_processor.h
 * @brief Text-to-Speech (TTS) Processor Plugin
 * 
 * Provides speech synthesis capabilities for natural language output.
 * Integrates with voice assistant for spoken responses.
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

#pragma once

#include "content_plugin_interface.h"
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
