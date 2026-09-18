/**
 * @file ocr_processor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 90/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

class OcrProcessor : public IContentProcessor {
public:
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
    /**
     * @brief Ocr Processor.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit OcrProcessor(Config config);
    ~OcrProcessor() override = default;

    ExtractionResult extract(
        const std::string& blob,
        const ContentType& content_type
    ) override;

    std::vector<json> chunk(
        const ExtractionResult& extraction_result,
        int chunk_size,
        int overlap
    ) override;

    std::vector<float> generateEmbedding(const std::string& chunk_data) override;

    std::string getName() const override { return "OcrProcessor"; }

    std::vector<ContentCategory> getSupportedCategories() const override {
        return {ContentCategory::IMAGE};
    }

    /**
     * @brief Is Available.
     * @return True when the operation succeeds.
     */
    static bool isAvailable();

    /**
     * @brief Get Tesseract Version.
     * @return Return value.
     */
    static std::string getTesseractVersion();

    static std::string performOcr(
        const std::vector<uint8_t>& image_blob,
        const std::string& language = "eng",
        const std::string& data_dir = ""
    );

private:
    Config config_;

    struct PreprocessInfo {
        int  original_dpi = 0;    ///< DPI read from image metadata (0 = unknown)
        bool rescaled     = false; ///< Image was rescaled to Config::target_dpi
        bool binarized    = false; ///< Adaptive (Sauvola) binarisation was applied
    };

    std::string runTesseract(const std::string& blob,
                             PreprocessInfo* preprocess_info = nullptr);

    /**
     * @brief Is Supported Image Format.
     * @param[in] blob Input parameter.
     * @return True when the operation succeeds.
     */
    static bool isSupportedImageFormat(const std::string& blob);

    /**
     * @brief Count Tokens.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    static int countTokens(const std::string& text);
};

/**
 * @brief Create Ocr Processor.
 * @return Return value.
 */
std::unique_ptr<IContentProcessor> createOcrProcessor();
/**
 * @brief Create Ocr Processor.
 * @param[in] config Input parameter.
 * @return Return value.
 */
std::unique_ptr<IContentProcessor> createOcrProcessor(OcrProcessor::Config config);

}  // namespace content
}  // namespace themis
