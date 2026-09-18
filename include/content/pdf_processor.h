/**
 * @file pdf_processor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "content/content_processor.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>

#ifdef THEMIS_ENABLE_PDF
#include <poppler/cpp/poppler-page.h>
#endif

namespace themis {
namespace content {

class ContentMetrics;  // forward declaration

struct PDFPageInfo {
    int page_number = 0;   ///< 1-based page index (CON-020)
    std::string text = {};
    int width = 0;         ///< Width in points (1/72 inch) (CON-020)
    int height = 0;        ///< Height in points (CON-020)
    int rotation = 0;      ///< Rotation: 0, 90, 180, or 270 degrees (CON-020)
    std::vector<std::pair<float, float>> text_positions;  // x,y positions of text blocks
};

struct PDFMetadata {
    std::string title;
    std::string author;
    std::string subject;
    std::string keywords;
    std::string creator;
    std::string producer;
    std::string creation_date;      // ISO 8601
    std::string modification_date;  // ISO 8601
    int page_count = 0;            ///< Total number of pages (CON-020)
    bool is_encrypted = false;     ///< True if document is password-protected (CON-020)
    bool is_linearized = false;    ///< True if PDF is web-optimised/linearized (CON-020)
    std::string pdf_version;
};

class PDFProcessor : public IContentProcessor {
public:
    struct Config {
        bool extract_text = true;
        bool extract_metadata = true;
        bool extract_images = false;      // Not yet implemented
        bool maintain_layout = false;     // Maintain text positioning
        bool detect_tables = false;       // Basic table detection
        int max_pages = 0;                // 0 = no limit
        std::string password;             // For encrypted PDFs
        ContentMetrics* metrics = nullptr; // Optional: report pdf_extracted / extract_error counters
    };

    PDFProcessor();
    /**
     * @brief PDFProcessor.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit PDFProcessor(Config config);
    ~PDFProcessor() override = default;

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

    std::string getName() const override { return "PDFProcessor"; }
    
    std::vector<ContentCategory> getSupportedCategories() const override {
        return {ContentCategory::TEXT};  // PDF is categorized as TEXT
    }

    /**
     * @brief Is Available.
     * @return True when the operation succeeds.
     */
    static bool isAvailable();

    /**
     * @brief Get Library Version.
     * @return Return value.
     */
    static std::string getLibraryVersion();

private:
    Config config_;

    // Internal extraction methods
    /**
     * @brief Extract Metadata.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    PDFMetadata extractMetadata(const std::string& blob);
    /**
     * @brief Extract Pages.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    std::vector<PDFPageInfo> extractPages(const std::string& blob);
    /**
     * @brief Extract All Text.
     * @param[in] pages Input parameter.
     * @return Return value.
     */
    std::string extractAllText(const std::vector<PDFPageInfo>& pages);

    /**
     * @brief Count Tokens.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    int countTokens(const std::string& text);

    /**
     * @brief Parse PDFDate.
     * @param[in] pdf_date Input parameter.
     * @return Return value.
     */
    std::string parsePDFDate(const std::string& pdf_date);

    /**
     * @brief Is PDFValid.
     * @param[in] blob Input parameter.
     * @return True when the operation succeeds.
     */
    bool isPDFValid(const std::string& blob);

#ifdef THEMIS_ENABLE_PDF
    // Assemble text from positioned poppler text boxes preserving reading order.
    // Sorts boxes top-to-bottom then left-to-right and inserts newlines at line breaks.
    // Populates positions_out with (x, y) of each box for downstream use.
    static std::string assembleTextWithLayout(
        const std::vector<poppler::text_box>& boxes,
        std::vector<std::pair<float, float>>& positions_out
    );
#endif
};

/**
 * @brief Create PDFProcessor.
 * @return Return value.
 */
std::unique_ptr<IContentProcessor> createPDFProcessor();
/**
 * @brief Create PDFProcessor.
 * @param[in] config Input parameter.
 * @return Return value.
 */
std::unique_ptr<IContentProcessor> createPDFProcessor(
    PDFProcessor::Config config
);

} // namespace content
} // namespace themis
