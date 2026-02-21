/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            filesystem_ingester.h                              ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     173                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "ingestion_manager.h"
#include <string>
#include <vector>
#include <memory>

namespace themis {
namespace ingestion {

/**
 * @brief Supported file formats
 */
enum class FileFormat {
    AUTO,       ///< Auto-detect format
    PDF,        ///< Portable Document Format
    DOCX,       ///< Microsoft Word
    TXT,        ///< Plain text
    HTML,       ///< HTML document
    XML,        ///< XML document
    JSON        ///< JSON document
};

/**
 * @brief OCR configuration
 */
struct OCRConfig {
    bool enabled = false;                ///< Enable OCR for scanned documents
    std::string language = "deu+eng";    ///< Tesseract language codes
    int dpi = 300;                       ///< DPI for image processing
    bool skip_text_pdfs = true;          ///< Skip OCR if PDF has text layer
    
    OCRConfig() = default;
};

/**
 * @brief File filter configuration
 */
struct FileFilter {
    std::vector<std::string> extensions;     ///< File extensions to include (e.g., ".pdf", ".docx")
    std::vector<std::string> exclude_patterns; ///< Patterns to exclude
    size_t min_size_bytes = 0;               ///< Minimum file size
    size_t max_size_bytes = 0;               ///< Maximum file size (0 = unlimited)
    bool recursive = true;                   ///< Recursively scan subdirectories
    
    FileFilter() = default;
};

/**
 * @brief Filesystem data ingester
 * 
 * Reads documents from local filesystem with OCR support for scanned PDFs.
 * Supports multiple formats: PDF, DOCX, TXT, HTML, XML, JSON.
 * 
 * Example usage:
 * @code
 * FileSystemIngester ingester;
 * SourceConfig config{
 *     .source_id = "custom_docs",
 *     .type = SourceType::FILESYSTEM,
 *     .location = "/mnt/verwaltung/vorschriften",
 *     .priority = 10
 * };
 * ingester.initialize(config);
 * 
 * OCRConfig ocr;
 * ocr.enabled = true;
 * ocr.language = "deu";
 * ingester.setOCRConfig(ocr);
 * 
 * FileFilter filter;
 * filter.extensions = {".pdf", ".docx"};
 * ingester.setFileFilter(filter);
 * 
 * auto stats = ingester.ingest("legal_documents", nullptr);
 * @endcode
 */
class FileSystemIngester : public ISourceConnector {
public:
    /**
     * @brief Construct filesystem ingester
     */
    FileSystemIngester();
    
    ~FileSystemIngester() override;
    
    // Delete copy
    FileSystemIngester(const FileSystemIngester&) = delete;
    FileSystemIngester& operator=(const FileSystemIngester&) = delete;
    
    /**
     * @brief Initialize ingester with configuration
     * @param config Source configuration with:
     *        - location: filesystem path (directory or file)
     *        - options["format"]: file format (auto/pdf/docx/txt/html/xml/json)
     *        - options["ocr_enabled"]: "true" to enable OCR
     *        - options["ocr_language"]: Tesseract language codes
     *        - options["recursive"]: "true" to scan subdirectories
     * @return true if initialization successful
     */
    bool initialize(const SourceConfig& config) override;
    
    /**
     * @brief Check if filesystem path is accessible
     * @return true if path exists and is readable
     */
    bool isAvailable() const override;
    
    /**
     * @brief Get total number of documents matching filter
     * @return Document count
     */
    size_t getDocumentCount() const override;
    
    /**
     * @brief Ingest documents from filesystem
     * @param target_collection Target collection in ThemisDB
     * @param progress_callback Optional progress callback
     * @return Ingestion statistics
     */
    IngestionStats ingest(const std::string& target_collection,
                         ProgressCallback progress_callback) override;
    
    /**
     * @brief Set OCR configuration
     * @param config OCR settings
     */
    void setOCRConfig(const OCRConfig& config);
    
    /**
     * @brief Set file filter
     * @param filter File filter settings
     */
    void setFileFilter(const FileFilter& filter);
    
    /**
     * @brief Set file format for processing
     * @param format Target file format (AUTO = auto-detect)
     */
    void setFileFormat(FileFormat format);
    
    /**
     * @brief Enable/disable metadata extraction
     * @param enabled Whether to extract file metadata (author, date, etc.)
     */
    void setMetadataExtraction(bool enabled);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ingestion
} // namespace themis
