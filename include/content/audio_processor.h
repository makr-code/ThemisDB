/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            audio_processor.h                                  ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:42:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     107                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file audio_processor.h
 * @brief Audio Content Processor Plugin (FFmpeg-based)
 * 
 * Extracts metadata and optionally transcribes audio files.
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

#pragma once

#include "content_plugin_interface.h"
#include <mutex>
#include <atomic>

namespace themis {
namespace content {

/**
 * @brief Audio Processor Plugin
 * 
 * Uses FFmpeg/libav for audio processing.
 * Extracts:
 * - Audio metadata (duration, codec, bitrate, sample rate, channels)
 * - ID3/Vorbis tags (artist, album, title, etc.)
 * - Waveform data for visualization
 * - Optional speech-to-text transcription
 */
class AudioProcessor : public IContentProcessorPlugin {
public:
    AudioProcessor();
    ~AudioProcessor() override;
    
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
    
private:
    // Configuration
    bool enable_transcription_ = false;
    std::string transcription_model_;
    std::string transcription_language_ = "auto";
    bool extract_waveform_ = false;
    int waveform_samples_ = 1000;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    std::atomic<uint64_t> audio_files_processed_{0};
    std::atomic<uint64_t> total_duration_ms_{0};
    std::atomic<uint64_t> transcriptions_performed_{0};
    std::atomic<uint64_t> errors_{0};
    
    bool initialized_ = false;
    
    // Internal methods
    MediaExtractionData extractMetadata(const std::vector<uint8_t>& blob);
    json extractTags(const std::vector<uint8_t>& blob);
    std::vector<float> extractWaveform(const std::vector<uint8_t>& blob);
    std::string transcribe(const std::vector<uint8_t>& blob);
};

} // namespace content
} // namespace themis
