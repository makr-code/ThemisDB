/**
 * @file office_processor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "content/content_processor.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <map>

namespace themis {
namespace content {

class ContentMetrics;  // forward declaration

/**
 * @brief Office Document Type
 */
enum class OfficeDocumentType {
    UNKNOWN,
    DOCX,      // Word 2007+
    XLSX,      // Excel 2007+
    PPTX,      // PowerPoint 2007+
    DOC,       // Legacy Word (not fully supported)
    XLS,       // Legacy Excel (not fully supported)
    PPT,       // Legacy PowerPoint (not fully supported)
    ODT,       // OpenDocument Text
    ODS,       // OpenDocument Spreadsheet
    ODP,       // OpenDocument Presentation
    RTF        // Rich Text Format
};

/**
 * @brief Word Document Structure
 */
struct WordDocumentInfo {
    std::string text = {};
    std::vector<std::string> paragraphs;
    std::vector<std::string> headings;
    std::vector<std::pair<std::string, std::string>> comments;  // author, text
    int word_count = 0;       ///< CON-021
    int paragraph_count = 0;  ///< CON-021
    int page_count = 0;       ///< CON-021
};

/**
 * @brief Excel Workbook Structure
 */
struct ExcelWorkbookInfo {
    struct Sheet {
        std::string name;
        int row_count = 0;  ///< CON-021
        int col_count = 0;  ///< CON-021
        std::vector<std::vector<std::string>> cells;  // row-major
        std::vector<std::string> formulas;
    };
    std::vector<Sheet> sheets;
    std::vector<std::string> defined_names;
};

/**
 * @brief PowerPoint Presentation Structure
 */
struct PowerPointInfo {
    struct Slide {
        int slide_number = 0;  ///< 1-based slide index (CON-021)
        std::string title;
        std::string text;
        std::vector<std::string> notes;
    };
    std::vector<Slide> slides;
    int slide_count = 0;
};

/**
 * @brief Office Document Metadata
 */
struct OfficeMetadata {
    std::string title;
    std::string author;
    std::string last_modified_by;
    std::string subject;
    std::string keywords;
    std::string category;
    std::string description;
    std::string created_date;      // ISO 8601
    std::string modified_date;     // ISO 8601
    std::string application;       // e.g., "Microsoft Word 2019"
    int revision = 0;              ///< Document revision number (CON-021)
    int edit_time_minutes = 0;     ///< Total editing time in minutes (CON-021)
};

/**
 * @brief Office Content Processor
 * 
 * Handles Office document extraction:
 * - DOCX: Paragraphs, headings, comments, track changes
 * - XLSX: Sheets, cells, formulas, charts (metadata only)
 * - PPTX: Slides, speaker notes, transitions (metadata only)
 * - ODF: Basic text and metadata extraction
 * 
 * VCC-URN Compliant: Uses content-addressable storage for embedded media.
 */
class OfficeProcessor : public IContentProcessor {
public:
    /**
     * @brief Configuration for Office processing
     */
    struct Config {
        bool extract_text = true;
        bool extract_metadata = true;
        bool extract_comments = true;
        bool extract_formulas = true;      // XLSX only
        bool extract_speaker_notes = true; // PPTX only
        bool include_hidden_text = false;
        int max_cell_count = 1000000;      // XLSX: limit cells to extract
        std::string password;              // For encrypted documents
        ContentMetrics* metrics = nullptr; // Optional: report office_extracted / extract_error counters

        // LibreOffice headless fallback (DOC/XLS/PPT legacy formats).
        // Sandboxing relies on POSIX_SPAWN_RESETIDS (drops SUID/SGID bits) and
        // POSIX_SPAWN_SETPGROUP (isolated process group for clean kill on timeout).
        // For additional OS-level isolation run the ThemisDB process itself under
        // a restricted system account with no write access to the data directory.
        std::string libreoffice_path;          // Absolute path to soffice binary; default: /usr/bin/soffice
        int libreoffice_timeout_seconds = 30;  // Hard timeout in seconds; subprocess is killed on expiry
    };

    OfficeProcessor();
    explicit OfficeProcessor(Config config);
    ~OfficeProcessor() override = default;

    /**
     * @brief Extract text and metadata from Office document
     * 
     * @param blob Raw document bytes (ZIP-based OOXML or ODF)
     * @param content_type Content type info
      * @return ExtractionResult with text and metadata.
      *         Returns ok=false for empty payloads and for payloads above
      *         the internal safety size limit.
     */
    ExtractionResult extract(
        const std::string& blob,
        const ContentType& content_type
    ) override;

    /**
     * @brief Chunk Office document into sections
     * 
     * - DOCX: Chunks by paragraph/heading
     * - XLSX: Chunks by sheet or row range
     * - PPTX: Chunks by slide
     * 
     * @param extraction_result Extracted document data
     * @param chunk_size Target chunk size in tokens
     * @param overlap Overlap between chunks
     * @return Vector of chunks with section metadata
     */
    std::vector<json> chunk(
        const ExtractionResult& extraction_result,
        int chunk_size,
        int overlap
    ) override;

    /**
     * @brief Generate embedding for Office chunk
     * 
     * @param chunk_data Chunk text
     * @return Embedding vector
     */
    std::vector<float> generateEmbedding(const std::string& chunk_data) override;

    std::string getName() const override { return "OfficeProcessor"; }
    
    std::vector<ContentCategory> getSupportedCategories() const override {
        return {ContentCategory::TEXT, ContentCategory::STRUCTURED};
    }

    /**
     * @brief Check if Office processing is available
     */
    static bool isAvailable();

    /**
     * @brief Detect Office document type from bytes
     * 
     * @param blob Document bytes
     * @return Detected document type
     */
    static OfficeDocumentType detectDocumentType(const std::string& blob);

private:
    Config config_;

    // Type-specific extractors
    ExtractionResult extractDOCX(const std::string& blob);
    ExtractionResult extractXLSX(const std::string& blob);
    ExtractionResult extractPPTX(const std::string& blob);
    ExtractionResult extractODF(const std::string& blob, OfficeDocumentType type);

    // LibreOffice headless fallback for legacy OLE formats (DOC/XLS/PPT)
    // Spawns soffice --headless via posix_spawn with a configurable timeout.
    ExtractionResult extractLegacyViaLibreOffice(const std::string& blob, OfficeDocumentType doc_type);

    // OOXML helpers
    std::string readZipEntry(const std::string& zip_blob, const std::string& entry_path);
    OfficeMetadata extractOOXMLMetadata(const std::string& zip_blob);
    std::vector<std::string> listZipEntries(const std::string& zip_blob);

    // XML text extraction
    std::string extractTextFromXML(const std::string& xml_content);
    std::vector<std::string> extractParagraphsFromXML(const std::string& xml_content);

    // Token counting
    int countTokens(const std::string& text);

    // Validate ZIP/OOXML structure
    bool isValidOOXML(const std::string& blob);
    bool isValidODF(const std::string& blob);
};

/**
 * @brief Factory function for Office Processor.
 * @return Unique pointer to OfficeProcessor.
 */
std::unique_ptr<IContentProcessor> createOfficeProcessor();
/**
 * @brief Factory function for Office Processor.
 * @param config Optional configuration.
 * @return Unique pointer to OfficeProcessor.
 */
std::unique_ptr<IContentProcessor> createOfficeProcessor(
    OfficeProcessor::Config config
);

} // namespace content
} // namespace themis
