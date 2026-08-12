/**
 * @file filesystem_ingester.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ingestion_manager.h"
#include "ingestion/file_format.h"
#include <string>
#include <vector>
#include <memory>

namespace themis {
namespace ingestion {

/**
 * @brief Detected binary MIME type based on magic-byte inspection
 *
 * Used by `detectBinaryMimeType()` to identify binary document formats
 * before dispatching to the appropriate external converter.
 */
enum class BinaryMimeType {
    UNKNOWN,  ///< Not a known binary type (may be text)
    PDF,      ///< Portable Document Format (magic: %PDF)
    DOCX      ///< Office Open XML / DOCX (magic: PK 0x03 0x04 ZIP with OOXML marker)
};

/**
 * @brief Detect the binary MIME type of a buffer via magic-byte inspection
 *
 * Reads the first 8 bytes of the provided raw buffer and identifies
 * known binary formats:
 * - PDF:  starts with `%PDF`
 * - DOCX: starts with the ZIP magic `PK 0x03 0x04` and contains the OOXML
 *         content-type marker in the first 512 bytes
 *
 * @param raw   First bytes of the file (minimum 4 bytes required; fewer
 *              bytes always return `BinaryMimeType::UNKNOWN`)
 * @return Detected type or `BinaryMimeType::UNKNOWN`
 */
BinaryMimeType detectBinaryMimeType(const std::string& raw);

/**
 * @brief Check whether a converter program name/path is safe to use in a shell command
 *
 * Rejects any value that contains shell metacharacters (`|`, `;`, `&`, `$`,
 * `<`, `>`, `` ` ``, `!`, newlines, or NUL) that could enable command injection
 * through the `popen()` call used to invoke external converters.
 *
 * An empty string returns `true` because an empty converter path means conversion
 * is disabled (the file is silently skipped without spawning any process).
 *
 * @param converter  Converter name or path as stored in `BinaryConverter`
 * @return `true` if safe to pass to popen(); `false` if dangerous characters found
 */
bool isConverterSafe(const std::string& converter);

/**
 * @brief Check whether a filesystem path is free of directory-traversal sequences
 *
 * Iterates the path components and returns `false` if any component equals `..`.
 * This prevents a malicious or misconfigured `SourceConfig::location` from being
 * used to escape an intended base directory (e.g. `"../../etc/passwd"`).
 *
 * An empty path returns `true` (no traversal sequence present; the caller is
 * responsible for checking existence / accessibility separately).
 *
 * @param path  Filesystem path to validate (absolute or relative)
 * @return `true` if no `..` component is present; `false` if traversal detected
 */
bool isPathTraversalSafe(const std::string& path);

/**
 * @brief Configuration for external binary-format converters
 *
 * When set on `FileSystemIngester`, the ingester calls the specified
 * command-line tools to extract plain text from PDF and DOCX files.
 * An empty converter path disables conversion for that format (the
 * file is silently skipped without raising an error).
 *
 * Converter invocations:
 * - PDF:  `<pdf_converter> "<file>" -`   (pdftotext writes to stdout)
 * - DOCX: `<docx_converter> -f docx -t plain "<file>"` (pandoc)
 *
 * When a converter is not found on PATH or exits with a non-zero status,
 * the file is silently skipped (no error added to `IngestionStats`).
 *
 * Converter paths are validated with `isConverterSafe()` before use;
 * values containing shell metacharacters are rejected silently.
 */
struct BinaryConverter {
    std::string pdf_converter  = "pdftotext";  ///< Converter for PDF (e.g. "pdftotext", "/usr/bin/pdftotext", or "")
    std::string docx_converter = "pandoc";     ///< Converter for DOCX (e.g. "pandoc", "/usr/bin/pandoc", or "")
    bool detect_by_magic = true;               ///< Detect type by magic bytes in addition to file extension

    BinaryConverter() = default;
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

    /**
     * @brief Set external binary-format converter paths
     *
     * When configured, PDF and DOCX files are converted to plain text by
     * spawning the specified command-line tools.  An empty converter path
     * disables conversion for that format and the file is silently skipped.
     *
     * @param config Converter paths and detection settings
     */
    void setBinaryConverter(const BinaryConverter& config);

    /**
     * @brief Inject a per-document validator called before each write.
     *
     * When set, the validator is called for every extracted document.
     * Documents that fail validation are counted as failed (not processed).
     * Pass an empty `DocumentValidatorFn` to remove a previously set validator.
     *
     * @param validator Validator callback; empty = disable
     */
    void setDocumentValidator(DocumentValidatorFn validator) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ingestion
} // namespace themis
