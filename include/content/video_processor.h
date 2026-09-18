/**
 * @file video_processor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
    /**
     * @brief Extract Metadata.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    MediaExtractionData extractMetadata(const std::vector<uint8_t>& blob);
    /**
     * @brief Generate Thumbnail.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> generateThumbnail(const std::vector<uint8_t>& blob);
    /**
     * @brief Extract Subtitles.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    std::string extractSubtitles(const std::vector<uint8_t>& blob);
    /**
     * @brief Detect Scenes.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    std::vector<int64_t> detectScenes(const std::vector<uint8_t>& blob);
    /**
     * @brief Extract Keyframes.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    std::vector<int64_t> extractKeyframes(const std::vector<uint8_t>& blob);
    
#ifdef THEMIS_HAS_FFMPEG
    /**
     * @brief FFmpeg-specific helper methods
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    MediaExtractionData extractMetadataFFmpeg(const std::vector<uint8_t>& blob);
    /**
     * @brief Generate Thumbnail FFmpeg.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> generateThumbnailFFmpeg(const std::vector<uint8_t>& blob);
    /**
     * @brief Extract Keyframes FFmpeg.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    std::vector<int64_t> extractKeyframesFFmpeg(const std::vector<uint8_t>& blob);
    /**
     * @brief Detect Scenes FFmpeg.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    std::vector<int64_t> detectScenesFFmpeg(const std::vector<uint8_t>& blob);
#endif
};

} // namespace content
} // namespace themis
