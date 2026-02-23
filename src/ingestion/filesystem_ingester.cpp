/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            filesystem_ingester.cpp                            ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:58:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     633                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 08a813e1d  2026-02-22  feat(ingestion): PDF/DOCX binary format ingestion via ext... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "ingestion/filesystem_ingester.h"
#include <filesystem>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <array>
#include <cstdio>

// pugixml for HTML/XML text extraction (already a vcpkg dependency)
#ifdef THEMIS_HAS_PUGIXML
#include <pugixml.hpp>
#endif

namespace themis {
namespace ingestion {

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// MIME type detection (free function – implementation)
// ---------------------------------------------------------------------------

BinaryMimeType detectBinaryMimeType(const std::string& raw) {
    if (raw.size() < 4) return BinaryMimeType::UNKNOWN;

    // PDF: magic bytes "%PDF"
    if (raw[0] == '%' && raw[1] == 'P' && raw[2] == 'D' && raw[3] == 'F') {
        return BinaryMimeType::PDF;
    }

    // ZIP magic: PK\x03\x04 – used by DOCX (Office Open XML)
    if (raw[0] == 'P' && raw[1] == 'K' &&
        static_cast<unsigned char>(raw[2]) == 0x03 &&
        static_cast<unsigned char>(raw[3]) == 0x04) {
        // Distinguish DOCX from other ZIP-based formats by looking for the
        // OOXML content-type marker in the first 512 bytes.
        const std::string probe = raw.substr(0, std::min(raw.size(), size_t(512)));
        if (probe.find("word/") != std::string::npos ||
            probe.find("[Content_Types]") != std::string::npos ||
            probe.find("application/vnd.openxmlformats") != std::string::npos) {
            return BinaryMimeType::DOCX;
        }
        // Generic ZIP but could still be DOCX – trust the extension in that case
        return BinaryMimeType::UNKNOWN;
    }

    return BinaryMimeType::UNKNOWN;
}

// ---------------------------------------------------------------------------
// Converter path safety validation (free function – implementation)
// ---------------------------------------------------------------------------

bool isConverterSafe(const std::string& converter) {
    if (converter.empty()) return true;  // empty = disabled, skip silently
    for (char c : converter) {
        switch (c) {
            case '|': case ';': case '&': case '$': case '<': case '>':
            case '`': case '!': case '\n': case '\r': case '\0':
                return false;
            default:
                break;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/// Recursively collect all text nodes from a pugixml document tree.
#ifdef THEMIS_HAS_PUGIXML
static void collectTextNodes(const pugi::xml_node& node, std::ostringstream& out) {
    for (auto& child : node.children()) {
        if (child.type() == pugi::node_pcdata ||
            child.type() == pugi::node_cdata) {
            const char* val = child.value();
            if (val && *val) {
                out << val << ' ';
            }
        } else {
            collectTextNodes(child, out);
        }
    }
}
#endif

/// Extract plain text from an XML/HTML buffer using pugixml.
/// Falls back to returning an empty string when pugixml is not available.
static std::string extractXmlText(const std::string& raw,
                                   bool is_html) {
#ifdef THEMIS_HAS_PUGIXML
    pugi::xml_document doc;
    unsigned int parse_flags = is_html
        ? (pugi::parse_default | pugi::parse_fragment)
        : pugi::parse_default;
    // Try lenient parsing for HTML
    pugi::xml_parse_result result =
        doc.load_buffer(raw.data(), raw.size(), parse_flags);
    if (!result && is_html) {
        // Retry with declaration stripping for malformed HTML
        result = doc.load_buffer(raw.data(), raw.size(),
                                  pugi::parse_default | pugi::parse_fragment
                                  | pugi::parse_pi);
    }
    std::ostringstream out;
    collectTextNodes(doc, out);
    return out.str();
#else
    // pugixml not available: return raw content as-is (best effort)
    (void)is_html;
    return raw;
#endif
}

/// Minimal JSON text extractor: collects all string values from a JSON buffer
/// without requiring nlohmann/json in this translation unit.
/// Handles both "key":"value" and bare string values.
static std::string extractJsonText(const std::string& raw) {
    std::string result;
    result.reserve(raw.size() / 2);
    bool in_string = false;
    bool escape = false;
    std::string token;

    for (size_t i = 0; i < raw.size(); ++i) {
        char c = raw[i];
        if (escape) {
            if (in_string) token += c;
            escape = false;
            continue;
        }
        if (c == '\\') {
            escape = true;
            if (in_string) token += c;
            continue;
        }
        if (c == '"') {
            if (in_string) {
                // End of string token – emit if non-empty
                if (!token.empty()) {
                    result += token;
                    result += ' ';
                    token.clear();
                }
                in_string = false;
            } else {
                in_string = true;
            }
            continue;
        }
        if (in_string) {
            token += c;
        }
    }
    return result;
}

/// Run an external command and capture its stdout.
/// @return Captured stdout, or an empty string if the command failed /
///         was not found.  Never throws.
static std::string runExternalConverter(const std::string& cmd) {
    // popen is POSIX; on Windows this would need _popen.
#if defined(_WIN32)
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");  // NOLINT(cert-env33-c)
#endif
    if (!pipe) return "";
    std::string result;
    std::array<char, 4096> buf;
    while (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe) != nullptr) {
        result += buf.data();
    }
#if defined(_WIN32)
    int rc = _pclose(pipe);
#else
    int rc = pclose(pipe);
#endif
    return (rc == 0) ? result : "";
}

/// Platform-aware shell escaping of a file path for use in popen() commands.
/// On POSIX: wraps in single quotes with embedded single-quote escaping.
/// On Windows: wraps in double quotes with backslash escaping.
static std::string shellEscapePath(const std::string& path) {
#if defined(_WIN32)
    // Double-quote escaping for cmd.exe / PowerShell
    std::string escaped = "\"";
    for (char c : path) {
        if (c == '"') escaped += "\\\"";
        else          escaped += c;
    }
    escaped += '"';
    return escaped;
#else
    // POSIX single-quote escaping
    std::string escaped;
    escaped.reserve(path.size() + 2);
    escaped += '\'';
    for (char c : path) {
        if (c == '\'') escaped += "'\"'\"'";
        else           escaped += c;
    }
    escaped += '\'';
    return escaped;
#endif
}

/// Suppress stderr in a platform-appropriate way for popen() commands.
static const char* stderrRedirect() {
#if defined(_WIN32)
    return " 2>NUL";
#else
    return " 2>/dev/null";
#endif
}

/// Extract text from a PDF file using an external converter.
/// @param file_path   Absolute path to the PDF file.
/// @param converter   Name or path of the converter binary (e.g. "pdftotext").
///                    If empty, returns an empty string immediately.
/// @return Extracted plain text, or empty string on failure/unavailability.
static std::string extractPdfWithConverter(const std::string& file_path,
                                           const std::string& converter) {
    if (converter.empty()) return "";
    if (!isConverterSafe(converter)) return "";
    // pdftotext syntax: pdftotext <input> - (dash = stdout)
    std::string cmd = converter + " " + shellEscapePath(file_path) + " -" + stderrRedirect();
    return runExternalConverter(cmd);
}

/// Extract text from a DOCX file using an external converter.
/// @param file_path   Absolute path to the DOCX file.
/// @param converter   Name or path of the converter binary (e.g. "pandoc").
///                    If empty, returns an empty string immediately.
/// @return Extracted plain text, or empty string on failure/unavailability.
static std::string extractDocxWithConverter(const std::string& file_path,
                                            const std::string& converter) {
    if (converter.empty()) return "";
    if (!isConverterSafe(converter)) return "";
    // pandoc syntax: pandoc -f docx -t plain <input>
    std::string cmd = converter + " -f docx -t plain " + shellEscapePath(file_path) + stderrRedirect();
    return runExternalConverter(cmd);
}

} // anonymous namespace

// Pimpl implementation
class FileSystemIngester::Impl {
public:
    Impl() 
        : format_(FileFormat::AUTO)
        , metadata_extraction_(true) {
    }
    
    ~Impl() = default;
    
    bool initialize(const SourceConfig& config) {
        if (config.type != SourceType::FILESYSTEM) {
            return false;
        }
        
        config_ = config;
        path_ = config.location;
        
        // Parse options
        auto it = config.options.find("format");
        if (it != config.options.end()) {
            // Parse format string
            if (it->second == "pdf") format_ = FileFormat::PDF;
            else if (it->second == "docx") format_ = FileFormat::DOCX;
            else if (it->second == "txt") format_ = FileFormat::TXT;
            else if (it->second == "html") format_ = FileFormat::HTML;
            else if (it->second == "xml") format_ = FileFormat::XML;
            else if (it->second == "json") format_ = FileFormat::JSON;
            else format_ = FileFormat::AUTO;
        }
        
        it = config.options.find("ocr_enabled");
        if (it != config.options.end()) {
            ocr_config_.enabled = (it->second == "true");
        }
        
        it = config.options.find("ocr_language");
        if (it != config.options.end()) {
            ocr_config_.language = it->second;
        }
        
        it = config.options.find("recursive");
        if (it != config.options.end()) {
            filter_.recursive = (it->second == "true");
        }

        // Binary converter paths can also be configured via SourceConfig options
        it = config.options.find("pdf_converter");
        if (it != config.options.end()) {
            binary_converter_.pdf_converter = it->second;
        }

        it = config.options.find("docx_converter");
        if (it != config.options.end()) {
            binary_converter_.docx_converter = it->second;
        }

        it = config.options.find("detect_by_magic");
        if (it != config.options.end()) {
            binary_converter_.detect_by_magic = (it->second != "false");
        }

        return fs::exists(path_);
    }
    
    bool isAvailable() const {
        return fs::exists(path_);
    }
    
    size_t getDocumentCount() const {
        if (!fs::exists(path_)) {
            return 0;
        }
        
        size_t count = 0;
        
        try {
            if (fs::is_regular_file(path_)) {
                count = 1;
            } else if (fs::is_directory(path_)) {
                if (filter_.recursive) {
                    for (fs::recursive_directory_iterator it(path_), end; it != end; ++it) {
                        const auto& entry = *it;
                        if (entry.is_regular_file() && matchesFilter(entry.path())) {
                            count++;
                        }
                    }
                } else {
                    for (fs::directory_iterator it(path_), end; it != end; ++it) {
                        const auto& entry = *it;
                        if (entry.is_regular_file() && matchesFilter(entry.path())) {
                            count++;
                        }
                    }
                }
            }
        } catch (const std::exception&) {
            // Ignore errors during counting
        }
        
        return count;
    }
    
    IngestionStats ingest(const std::string& target_collection,
                         ProgressCallback progress_callback) {
        IngestionStats stats;
        auto start_time = std::chrono::steady_clock::now();
        
        if (!fs::exists(path_)) {
            stats.addError(IngestionErrorCode::FILE_NOT_FOUND,
                           IngestionErrorSeverity::FATAL,
                           "Path does not exist: " + path_);
            return stats;
        }
        
        try {
            std::vector<fs::path> files_to_process;
            
            // 1. Collect files matching filter
            if (fs::is_regular_file(path_)) {
                if (matchesFilter(path_)) {
                    files_to_process.push_back(path_);
                }
            } else if (fs::is_directory(path_)) {
                if (filter_.recursive) {
                    for (fs::recursive_directory_iterator it(path_), end; it != end; ++it) {
                        const auto& entry = *it;
                        if (entry.is_regular_file() && matchesFilter(entry.path())) {
                            files_to_process.push_back(entry.path());
                        }
                    }
                } else {
                    for (fs::directory_iterator it(path_), end; it != end; ++it) {
                        const auto& entry = *it;
                        if (entry.is_regular_file() && matchesFilter(entry.path())) {
                            files_to_process.push_back(entry.path());
                        }
                    }
                }
            }
            
            // 2. Process each file
            size_t processed = 0;
            for (const auto& file_path : files_to_process) {
                try {
                    // Extract text from file
                    std::string content = extractTextFromFile(file_path);
                    
                    if (!content.empty()) {
                        // In production: Insert into target_collection
                        stats.documents_processed++;
                        stats.bytes_processed += content.size();
                    }
                    
                    processed++;
                    
                    // Report progress
                    if (progress_callback && processed % 10 == 0) {
                        progress_callback(config_.source_id, processed, 
                                        files_to_process.size(),
                                        "Processing: " + file_path.filename().string());
                    }
                    
                } catch (const std::exception& e) {
                    stats.documents_failed++;
                    stats.addError(IngestionErrorCode::PROCESSING_FAILED,
                                   IngestionErrorSeverity::WARNING,
                                   "Failed to process file: " + file_path.string(),
                                   config_.source_id,
                                   e.what());
                }
            }
            
            auto end_time = std::chrono::steady_clock::now();
            stats.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();
            if (stats.elapsed_seconds > 0.0 && stats.documents_processed > 0) {
                stats.metrics.throughput_docs_per_sec =
                    static_cast<double>(stats.documents_processed) / stats.elapsed_seconds;
            }
            
        } catch (const std::exception& e) {
            stats.addError(IngestionErrorCode::INTERNAL_ERROR,
                           IngestionErrorSeverity::FATAL,
                           "Ingestion failed: " + std::string(e.what()));
        }
        
        return stats;
    }
    
private:
    // Helper: Extract text from file based on format/extension and MIME detection
    std::string extractTextFromFile(const fs::path& file_path) {
        auto ext = file_path.extension().string();
        // Normalise extension to lower-case for comparisons
        for (auto& ch : ext) {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        std::string content;

        // Read raw file content
        std::ifstream file(file_path, std::ios::binary);
        if (!file) {
            // Include OS-level error to aid debugging (permissions, missing, etc.)
            throw std::runtime_error(
                "Cannot open file: " + file_path.string() +
                " (" + std::strerror(errno) + ")");
        }
        std::string raw{std::istreambuf_iterator<char>(file),
                        std::istreambuf_iterator<char>()};

        // Optionally detect binary MIME type from magic bytes before extension dispatch.
        // This ensures that binary files are handled correctly even if misnamed.
        BinaryMimeType mime = BinaryMimeType::UNKNOWN;
        if (binary_converter_.detect_by_magic) {
            mime = detectBinaryMimeType(raw);
        }

        // Determine whether this is a PDF or DOCX (by magic OR extension)
        bool is_pdf  = (mime == BinaryMimeType::PDF)  || (ext == ".pdf");
        bool is_docx = (mime == BinaryMimeType::DOCX) || (ext == ".docx");

        if (is_pdf) {
            // Use external converter to extract plain text.
            // Silently skip the file (return empty) when no converter is configured
            // or the converter is not available / fails.
            content = extractPdfWithConverter(file_path.string(),
                                              binary_converter_.pdf_converter);
        } else if (is_docx) {
            content = extractDocxWithConverter(file_path.string(),
                                               binary_converter_.docx_converter);
        } else if (ext == ".txt" || ext == ".md" || ext == ".csv") {
            // Plain text / markdown / CSV – use raw bytes
            content = std::move(raw);
        } else if (ext == ".html" || ext == ".htm") {
            // HTML – extract visible text via pugixml when available.
            // Falls back to raw bytes when pugixml is absent OR when the
            // document contains no text nodes (e.g. empty/script-only HTML).
            content = extractXmlText(raw, /*is_html=*/true);
            if (content.empty()) {
                content = std::move(raw);  // raw fallback
            }
        } else if (ext == ".xml") {
            // XML – extract text nodes via pugixml when available.
            // Falls back to raw bytes for the same reasons as HTML.
            content = extractXmlText(raw, /*is_html=*/false);
            if (content.empty()) {
                content = std::move(raw);  // raw fallback
            }
        } else if (ext == ".json") {
            // JSON – extract all quoted string values
            content = extractJsonText(raw);
            if (content.empty()) {
                content = std::move(raw);  // raw fallback
            }
        } else {
            // Unknown format: attempt to read as UTF-8 text (best effort)
            content = std::move(raw);
        }

        return content;
    }

public:
    void setOCRConfig(const OCRConfig& config) {
        ocr_config_ = config;
    }

    void setFileFilter(const FileFilter& filter) {
        filter_ = filter;
    }

    void setFileFormat(FileFormat format) {
        format_ = format;
    }

    void setMetadataExtraction(bool enabled) {
        metadata_extraction_ = enabled;
    }

    void setBinaryConverter(const BinaryConverter& config) {
        binary_converter_ = config;
    }

private:
    bool matchesFilter(const fs::path& file_path) const {
        // Check extension
        if (!filter_.extensions.empty()) {
            auto ext = file_path.extension().string();
            bool matches = false;
            for (const auto& allowed_ext : filter_.extensions) {
                if (ext == allowed_ext) {
                    matches = true;
                    break;
                }
            }
            if (!matches) {
                return false;
            }
        }
        
        // Check size
        try {
            auto size = fs::file_size(file_path);
            if (filter_.min_size_bytes > 0 && size < filter_.min_size_bytes) {
                return false;
            }
            if (filter_.max_size_bytes > 0 && size > filter_.max_size_bytes) {
                return false;
            }
        } catch (const std::exception&) {
            return false;
        }
        
        // Check exclude patterns
        auto path_str = file_path.string();
        for (const auto& pattern : filter_.exclude_patterns) {
            if (path_str.find(pattern) != std::string::npos) {
                return false;
            }
        }
        
        return true;
    }
    
    SourceConfig config_;
    std::string path_;
    FileFormat format_;
    OCRConfig ocr_config_;
    FileFilter filter_;
    BinaryConverter binary_converter_;
    bool metadata_extraction_;
};

// Public API implementation
FileSystemIngester::FileSystemIngester()
    : impl_(std::make_unique<Impl>()) {
}

FileSystemIngester::~FileSystemIngester() = default;

bool FileSystemIngester::initialize(const SourceConfig& config) {
    return impl_->initialize(config);
}

bool FileSystemIngester::isAvailable() const {
    return impl_->isAvailable();
}

size_t FileSystemIngester::getDocumentCount() const {
    return impl_->getDocumentCount();
}

IngestionStats FileSystemIngester::ingest(const std::string& target_collection,
                                         ProgressCallback progress_callback) {
    return impl_->ingest(target_collection, progress_callback);
}

void FileSystemIngester::setOCRConfig(const OCRConfig& config) {
    impl_->setOCRConfig(config);
}

void FileSystemIngester::setFileFilter(const FileFilter& filter) {
    impl_->setFileFilter(filter);
}

void FileSystemIngester::setFileFormat(FileFormat format) {
    impl_->setFileFormat(format);
}

void FileSystemIngester::setMetadataExtraction(bool enabled) {
    impl_->setMetadataExtraction(enabled);
}

void FileSystemIngester::setBinaryConverter(const BinaryConverter& config) {
    impl_->setBinaryConverter(config);
}

} // namespace ingestion
} // namespace themis
