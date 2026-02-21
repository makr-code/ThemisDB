/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            image_processor.h                                  ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:07:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     114                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file image_processor.h
 * @brief Image Content Processor Plugin (libvips-based)
 * 
 * Processes image files with advanced analysis capabilities.
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
 * @brief Image Processor Plugin
 * 
 * Uses libvips for high-performance image processing.
 * Extracts:
 * - EXIF/XMP/IPTC metadata
 * - Color analysis (dominant colors, histogram)
 * - OCR text (via Tesseract integration)
 * - Object detection labels (optional ML)
 * - Face detection
 */
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
    json extractExifMetadata(const std::vector<uint8_t>& blob);
    json extractXmpMetadata(const std::vector<uint8_t>& blob);
    std::vector<uint8_t> generateThumbnail(const std::vector<uint8_t>& blob);
    std::string performOCR(const std::vector<uint8_t>& blob);
    std::vector<std::array<uint8_t, 3>> extractDominantColors(const std::vector<uint8_t>& blob);
    json detectFaces(const std::vector<uint8_t>& blob);
    json detectObjects(const std::vector<uint8_t>& blob);
};

} // namespace content
} // namespace themis
