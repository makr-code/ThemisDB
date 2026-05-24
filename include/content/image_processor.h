/*
 * ThemisDB | File: image_processor.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=1; TODO=0, Stub=0, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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

#define THEMIS_CONTENT_PLUGIN_IMAGE_PROCESSOR_DEFINED 1

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

    /**
     * @brief Compute a DCT-based 64-bit perceptual hash (pHash) for an image blob.
     *
     * Implements the standard pHash algorithm:
     *  1. Extract a 32×32 grayscale sample grid from the image data.
     *  2. Apply a 2-D DCT and take the top-left 8×8 sub-matrix (64 values).
     *  3. Compute the median of those 64 DCT coefficients.
     *  4. Set bit i if dct[i] > median → yields a 64-bit hash.
     *
     * BMP images (BI_RGB, 24 bpp) are fully decoded to obtain accurate pixel
     * values.  For all other formats the raw byte stream is sampled uniformly
     * as a grayscale proxy, which still captures structural similarity without
     * requiring an external image-decode library.
     *
     * @param blob  Raw image bytes (JPEG, PNG, BMP, etc.).
     * @return 16-character lowercase hex string representing the 64-bit hash,
     *         or an empty string if the blob is too small to hash.
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
