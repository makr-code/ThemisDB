/**
 * @file office_processor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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

struct WordDocumentInfo {
    std::string text = {};
    std::vector<std::string> paragraphs;
    std::vector<std::string> headings;
    std::vector<std::pair<std::string, std::string>> comments;  // author, text
    int word_count = 0;       ///< CON-021
    int paragraph_count = 0;  ///< CON-021
    int page_count = 0;       ///< CON-021
};

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

class OfficeProcessor : public IContentProcessor {
public:
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
    /**
     * @brief Office Processor.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit OfficeProcessor(Config config);
    ~OfficeProcessor() override = default;

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

    std::string getName() const override { return "OfficeProcessor"; }
    
    std::vector<ContentCategory> getSupportedCategories() const override {
        return {ContentCategory::TEXT, ContentCategory::STRUCTURED};
    }

    /**
     * @brief Is Available.
     * @return True when the operation succeeds.
     */
    static bool isAvailable();

    /**
     * @brief Detect Document Type.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    static OfficeDocumentType detectDocumentType(const std::string& blob);

private:
    Config config_;

    // Type-specific extractors
    /**
     * @brief Extract DOCX.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    ExtractionResult extractDOCX(const std::string& blob);
    /**
     * @brief Extract XLSX.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    ExtractionResult extractXLSX(const std::string& blob);
    /**
     * @brief Extract PPTX.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    ExtractionResult extractPPTX(const std::string& blob);
    /**
     * @brief Extract ODF.
     * @param[in] blob Input parameter.
     * @param[in] type Input parameter.
     * @return Return value.
     */
    ExtractionResult extractODF(const std::string& blob, OfficeDocumentType type);

    /**
     * @brief LibreOffice headless fallback for legacy OLE formats (DOC/XLS/PPT) Spawns soffice --headless via posix_spawn with a configurable timeout.
     * @param[in] blob Input parameter.
     * @param[in] doc_type Input parameter.
     * @return Return value.
     */
    ExtractionResult extractLegacyViaLibreOffice(const std::string& blob, OfficeDocumentType doc_type);

    // OOXML helpers
    /**
     * @brief Read Zip Entry.
     * @param[in] zip_blob Input parameter.
     * @param[in] entry_path Path to the entry.
     * @return Return value.
     */
    std::string readZipEntry(const std::string& zip_blob, const std::string& entry_path);
    /**
     * @brief Extract OOXMLMetadata.
     * @param[in] zip_blob Input parameter.
     * @return Return value.
     */
    OfficeMetadata extractOOXMLMetadata(const std::string& zip_blob);
    /**
     * @brief List Zip Entries.
     * @param[in] zip_blob Input parameter.
     * @return Return value.
     */
    std::vector<std::string> listZipEntries(const std::string& zip_blob);

    // XML text extraction
    /**
     * @brief Extract Text From XML.
     * @param[in] xml_content Input parameter.
     * @return Return value.
     */
    std::string extractTextFromXML(const std::string& xml_content);
    /**
     * @brief Extract Paragraphs From XML.
     * @param[in] xml_content Input parameter.
     * @return Return value.
     */
    std::vector<std::string> extractParagraphsFromXML(const std::string& xml_content);

    // Token counting
    /**
     * @brief Count Tokens.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    int countTokens(const std::string& text);

    /**
     * @brief Validate ZIP/OOXML structure
     * @param[in] blob Input parameter.
     * @return True when the operation succeeds.
     */
    bool isValidOOXML(const std::string& blob);
    /**
     * @brief Is Valid ODF.
     * @param[in] blob Input parameter.
     * @return True when the operation succeeds.
     */
    bool isValidODF(const std::string& blob);
};

/**
 * @brief Create Office Processor.
 * @return Return value.
 */
std::unique_ptr<IContentProcessor> createOfficeProcessor();
/**
 * @brief Create Office Processor.
 * @param[in] config Input parameter.
 * @return Return value.
 */
std::unique_ptr<IContentProcessor> createOfficeProcessor(
    OfficeProcessor::Config config
);

} // namespace content
} // namespace themis
