/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            audio_processor.h                                  ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:51:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     110                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • d947853fba  2026-02-28  feat(content): Wire STTProcessor into AudioProcessor for ... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
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
#include "content/stt_processor.h"
#include <memory>
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

    // STT processor for audio transcription
    std::unique_ptr<STTProcessor> stt_processor_;
};

} // namespace content
} // namespace themis
