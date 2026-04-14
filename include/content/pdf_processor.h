/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            pdf_processor.h                                    ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:24:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     210                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 33a86557ed  2026-02-23  Fix triple PDF loading regression + add content_pdf_extra... ║
    • be51d5459d  2026-02-22  Add PDF text extraction with layout preservation using po... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file pdf_processor.h
 * @brief PDF Content Processor for ThemisDB
 * 
 * Extracts text, metadata, and structure from PDF documents.
 * Uses poppler-cpp or PoDoFo for PDF parsing.
 * 
 * @author ThemisDB Team
 * @date December 2025
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
    int page_number;
    std::string text;
    int width;           // Points (1/72 inch)
    int height;          // Points
    int rotation;        // 0, 90, 180, 270
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
    int page_count;
    bool is_encrypted;
    bool is_linearized;
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
 * @brief Factory function for PDF Processor
 * 
 * @param config Optional configuration
 * @return Unique pointer to PDFProcessor
 */
std::unique_ptr<IContentProcessor> createPDFProcessor();
std::unique_ptr<IContentProcessor> createPDFProcessor(
    PDFProcessor::Config config
);

} // namespace content
} // namespace themis
