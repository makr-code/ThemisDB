/**
 * @file pdf_processor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief PDF Page Information
 */
struct PDFPageInfo {
    int page_number = 0;   ///< 1-based page index (CON-020)
    std::string text = {};
    int width = 0;         ///< Width in points (1/72 inch) (CON-020)
    int height = 0;        ///< Height in points (CON-020)
    int rotation = 0;      ///< Rotation: 0, 90, 180, or 270 degrees (CON-020)
    std::vector<std::pair<float, float>> text_positions;  // x,y positions of text blocks
};

/**
 * @brief PDF Document Metadata
 */
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

/**
 * @brief PDF Content Processor
 * 
 * Handles PDF document extraction:
 * - Text extraction with layout awareness
 * - Metadata extraction (title, author, keywords, etc.)
 * - Page-by-page chunking for RAG
 * - Table detection (basic)
 * - Image extraction (placeholder)
 * 
 * VCC-URN Compliant: Uses content-addressable storage for embedded resources.
 */
class PDFProcessor : public IContentProcessor {
public:
    /**
     * @brief Configuration for PDF processing
     */
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
    explicit PDFProcessor(Config config);
    ~PDFProcessor() override = default;

    /**
     * @brief Extract text and metadata from PDF
     * 
     * @param blob Raw PDF bytes
     * @param content_type Content type info
     * @return ExtractionResult with text and metadata
     */
    ExtractionResult extract(
        const std::string& blob,
        const ContentType& content_type
    ) override;

    /**
     * @brief Chunk PDF into pages or sections
     * 
     * @param extraction_result Extracted PDF data
     * @param chunk_size Target chunk size in tokens
     * @param overlap Overlap between chunks
     * @return Vector of chunks with page/section metadata
     */
    std::vector<json> chunk(
        const ExtractionResult& extraction_result,
        int chunk_size,
        int overlap
    ) override;

    /**
     * @brief Generate embedding for PDF chunk
     * 
     * Delegates to embedding service (CLIP-like or text model).
     * 
     * @param chunk_data Chunk text
     * @return Embedding vector
     */
    std::vector<float> generateEmbedding(const std::string& chunk_data) override;

    std::string getName() const override { return "PDFProcessor"; }
    
    std::vector<ContentCategory> getSupportedCategories() const override {
        return {ContentCategory::TEXT};  // PDF is categorized as TEXT
    }

    /**
     * @brief Check if PDF processing is available
     * 
     * Returns true if poppler or PoDoFo library is linked.
     */
    static bool isAvailable();

    /**
     * @brief Get library version
     */
    static std::string getLibraryVersion();

private:
    Config config_;

    // Internal extraction methods
    PDFMetadata extractMetadata(const std::string& blob);
    std::vector<PDFPageInfo> extractPages(const std::string& blob);
    std::string extractAllText(const std::vector<PDFPageInfo>& pages);

    // Token counting (simple whitespace-based)
    int countTokens(const std::string& text);

    // Helper for PDF date format -> ISO 8601
    std::string parsePDFDate(const std::string& pdf_date);

    // Check PDF header/signature
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
 * @brief Factory function for PDF Processor.
 * @return Unique pointer to PDFProcessor.
 */
std::unique_ptr<IContentProcessor> createPDFProcessor();
/**
 * @brief Factory function for PDF Processor.
 * @param config Optional configuration.
 * @return Unique pointer to PDFProcessor.
 */
std::unique_ptr<IContentProcessor> createPDFProcessor(
    PDFProcessor::Config config
);

} // namespace content
} // namespace themis
