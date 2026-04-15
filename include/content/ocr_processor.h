/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ocr_processor.h                                    ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 05:33:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     193                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b832e64389  2026-04-12  fix(content): implement OcrProcessor::generateEmbedding a... ║
    • 01d40ae53b  2026-03-11  feat(content): default OCR language-pack path to config/a... ║
    • 2ae4537816  2026-03-11  feat(content): default ocr_processor data_dir to config/a... ║
    • d83358f4c0  2026-03-11  feat(content): add 300-DPI rescaling and adaptive binaris... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file ocr_processor.h
 * @brief OCR Content Processor for ThemisDB
 *
 * Extracts text from images using Tesseract OCR.
 * Supported input formats: JPEG, PNG, TIFF, BMP, GIF.
 *
 * Build with -DTHEMIS_ENABLE_OCR=ON to enable Tesseract support.
 * Without Tesseract, isAvailable() returns false and extract() returns a
 * result with ok=false; the caller may treat this as a skipped stage.
 *
 * @author ThemisDB Team
 * @date February 2026
 */

#pragma once

#include "content/content_processor.h"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace themis {
namespace content {

class ContentMetrics;  // forward declaration

/**
 * @brief OCR Content Processor
 *
 * Wraps Tesseract OCR to extract text from images:
 * - JPEG, PNG, TIFF, BMP, GIF
 * - Configurable language packs (default: "eng")
 * - Graceful fallback when libtesseract is not installed
 *
 * OCR output is stored in ExtractionResult::text and additionally in
 * ExtractionResult::metadata["content_ocr_text"] for pipeline consumers
 * that route OCR results through text_processor.cpp.
 */
class OcrProcessor : public IContentProcessor {
public:
    /**
     * @brief Configuration for OCR processing
     */
    struct Config {
        std::string language = "eng";            ///< Tesseract language pack name
        std::string data_dir;                    ///< Path to tessdata dir (empty = default: config/ai_ml/tesseract_lang/ or Tesseract auto-detect)
        int page_seg_mode = 3;                   ///< PSM: 3 = fully automatic page segmentation
        bool extract_metadata = true;            ///< Store language/confidence in metadata
        bool enable_char_whitelist = false;      ///< Restrict recognized characters
        std::string char_whitelist;              ///< Whitelist string (used when enabled)
        size_t max_text_size = 1024 * 1024;      ///< Maximum OCR output bytes (1 MB)
        ContentMetrics* metrics = nullptr;       ///< Optional metrics sink
        int target_dpi = 300;                    ///< Target resolution for DPI rescaling
        bool enable_dpi_rescaling = true;        ///< Rescale to target_dpi when image DPI is lower
        bool enable_adaptive_binarization = true; ///< Apply adaptive binarisation (Sauvola) before OCR
    };

    OcrProcessor();
    explicit OcrProcessor(Config config);
    ~OcrProcessor() override = default;

    /**
     * @brief Extract text from an image blob via Tesseract OCR
     *
     * @param blob  Raw image bytes (JPEG, PNG, TIFF, BMP, or GIF)
     * @param content_type  Content type info (used for metadata only)
     * @return ExtractionResult with text in result.text and metadata in
     *         result.metadata["content_ocr_text"]; ok=false when OCR is
     *         unavailable or the image format is not supported.
     */
    ExtractionResult extract(
        const std::string& blob,
        const ContentType& content_type
    ) override;

    /**
     * @brief Chunk OCR text into token-bounded segments
     *
     * Splits the extracted text by sentences, accumulating into chunks that
     * stay within chunk_size tokens with optional overlap.
     *
     * @param extraction_result  Result from extract()
     * @param chunk_size  Target chunk size in whitespace-tokens (0 = no split)
     * @param overlap  Overlap in tokens between consecutive chunks
     * @return Vector of JSON chunks with "text", "type", "sequence" keys
     */
    std::vector<json> chunk(
        const ExtractionResult& extraction_result,
        int chunk_size,
        int overlap
    ) override;

    /**
     * @brief Generate embedding for OCR text chunk (stub – delegates to pipeline)
     */
    std::vector<float> generateEmbedding(const std::string& chunk_data) override;

    std::string getName() const override { return "OcrProcessor"; }

    std::vector<ContentCategory> getSupportedCategories() const override {
        return {ContentCategory::IMAGE};
    }

    /**
     * @brief Check whether OCR is available (Tesseract linked at build time)
     */
    static bool isAvailable();

    /**
     * @brief Return Tesseract version string, or a "none" marker if unavailable
     */
    static std::string getTesseractVersion();

    /**
     * @brief Convenience OCR method for use by ImageProcessor::performOCR()
     *
     * Creates a temporary OcrProcessor with the given language/data_dir,
     * runs OCR on the image blob, and returns the extracted text (or empty
     * string when OCR is unavailable or the image cannot be decoded).
     *
     * @param image_blob  Raw image bytes
     * @param language    Tesseract language pack (default: "eng")
     * @param data_dir    Path to tessdata directory (empty = default: config/ai_ml/tesseract_lang/ or Tesseract auto-detect)
     * @return Extracted UTF-8 text, or "" on failure/unavailability
     */
    static std::string performOcr(
        const std::vector<uint8_t>& image_blob,
        const std::string& language = "eng",
        const std::string& data_dir = ""
    );

private:
    Config config_;

    /**
     * @brief Preprocessing metadata populated inside runTesseract().
     *
     * Carries per-call information about DPI detection, rescaling, and
     * binarisation so that extract() can surface it in result.metadata.
     */
    struct PreprocessInfo {
        int  original_dpi = 0;    ///< DPI read from image metadata (0 = unknown)
        bool rescaled     = false; ///< Image was rescaled to Config::target_dpi
        bool binarized    = false; ///< Adaptive (Sauvola) binarisation was applied
    };

    /// Run Tesseract on the image bytes; returns "" when OCR is unavailable.
    /// When preprocess_info is non-null it is filled with rescaling/binarisation details.
    std::string runTesseract(const std::string& blob,
                             PreprocessInfo* preprocess_info = nullptr);

    /// Return true when the blob has a magic-byte signature supported by Leptonica.
    static bool isSupportedImageFormat(const std::string& blob);

    /// Simple whitespace-based token counter.
    static int countTokens(const std::string& text);
};

/**
 * @brief Factory functions for OcrProcessor
 */
std::unique_ptr<IContentProcessor> createOcrProcessor();
std::unique_ptr<IContentProcessor> createOcrProcessor(OcrProcessor::Config config);

}  // namespace content
}  // namespace themis
