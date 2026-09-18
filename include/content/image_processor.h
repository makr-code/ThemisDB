/**
 * @file image_processor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "content_plugin_interface.h"
#include <mutex>
#include <atomic>

namespace themis {
namespace content {

#define THEMIS_CONTENT_PLUGIN_IMAGE_PROCESSOR_DEFINED 1

class ImageProcessor : public IContentProcessorPlugin {
public:
    ImageProcessor();
    ~ImageProcessor() override;
    
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
     * @brief Compute PHash.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    static std::string computePHash(const std::vector<uint8_t>& blob);

private:
    // Configuration
    int thumbnail_max_width_ = 256;
    int thumbnail_max_height_ = 256;
    bool enable_ocr_ = true;
    std::string ocr_language_ = "eng";
    bool enable_color_analysis_ = true;
    int dominant_colors_count_ = 5;
    bool enable_face_detection_ = false;
    bool enable_object_detection_ = false;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    std::atomic<uint64_t> images_processed_{0};
    std::atomic<uint64_t> ocr_performed_{0};
    std::atomic<uint64_t> faces_detected_{0};
    std::atomic<uint64_t> errors_{0};
    
    bool initialized_ = false;
    
    // Internal methods
    /**
     * @brief Extract Exif Metadata.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    json extractExifMetadata(const std::vector<uint8_t>& blob);
    /**
     * @brief Extract Xmp Metadata.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    json extractXmpMetadata(const std::vector<uint8_t>& blob);
    /**
     * @brief Generate Thumbnail.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> generateThumbnail(const std::vector<uint8_t>& blob);
    /**
     * @brief Perform OCR.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    std::string performOCR(const std::vector<uint8_t>& blob);
    std::vector<std::array<uint8_t, 3>> extractDominantColors(const std::vector<uint8_t>& blob);
    /**
     * @brief Detect Faces.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    json detectFaces(const std::vector<uint8_t>& blob);
    /**
     * @brief Detect Objects.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    json detectObjects(const std::vector<uint8_t>& blob);
};

} // namespace content
} // namespace themis
