/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            video_processor.h                                  ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:09:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     123                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 374b05b6ac  2026-02-28  Implement video frame extraction and scene detection (key... ║
    • 42d597244a  2026-02-26  fix(content): wire up extract_keyframes option and update... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file video_processor.h
 * @brief Video Content Processor Plugin (FFmpeg-based)
 * 
 * Extracts metadata, keyframes, and generates thumbnails from video files.
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

#pragma once

#include "content_plugin_interface.h"
#include <chrono>
#include <mutex>
#include <atomic>

// FFmpeg forward declarations to avoid header pollution
#ifdef THEMIS_HAS_FFMPEG
struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct SwsContext;
#endif

namespace themis {
namespace content {

/**
 * @brief Video Processor Plugin
 * 
 * Uses FFmpeg/libav for video processing.
 * Extracts:
 * - Video/audio metadata (duration, resolution, codecs, bitrate)
 * - Keyframe thumbnails
 * - Scene detection
 * - Subtitle extraction (if embedded)
 */
class VideoProcessor : public IContentProcessorPlugin {
public:
    VideoProcessor();
    ~VideoProcessor() override;
    
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
    int max_thumbnail_width_ = 320;
    int max_thumbnail_height_ = 240;
    int max_keyframes_ = 10;
    bool extract_subtitles_ = true;
    bool enable_scene_detection_ = false;
    double scene_detection_threshold_ = 0.4;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    std::atomic<uint64_t> videos_processed_{0};
    std::atomic<uint64_t> total_duration_ms_{0};
    std::atomic<uint64_t> errors_{0};
    
    bool initialized_ = false;
    
    // Internal methods
    MediaExtractionData extractMetadata(const std::vector<uint8_t>& blob);
    std::vector<uint8_t> generateThumbnail(const std::vector<uint8_t>& blob);
    std::string extractSubtitles(const std::vector<uint8_t>& blob);
    std::vector<int64_t> detectScenes(const std::vector<uint8_t>& blob);
    std::vector<int64_t> extractKeyframes(const std::vector<uint8_t>& blob);
    
#ifdef THEMIS_HAS_FFMPEG
    // FFmpeg-specific helper methods
    MediaExtractionData extractMetadataFFmpeg(const std::vector<uint8_t>& blob);
    std::vector<uint8_t> generateThumbnailFFmpeg(const std::vector<uint8_t>& blob);
    std::vector<int64_t> extractKeyframesFFmpeg(const std::vector<uint8_t>& blob);
    std::vector<int64_t> detectScenesFFmpeg(const std::vector<uint8_t>& blob);
#endif
};

} // namespace content
} // namespace themis
