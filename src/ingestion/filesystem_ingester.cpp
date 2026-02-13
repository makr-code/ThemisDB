#include "ingestion/filesystem_ingester.h"
#include <filesystem>
#include <stdexcept>
#include <fstream>
#include <chrono>

namespace themis {
namespace ingestion {

namespace fs = std::filesystem;

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
            stats.error_message = "Path does not exist: " + path_;
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
                        // For now, just count as processed
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
                    // Continue with next file
                }
            }
            
            auto end_time = std::chrono::steady_clock::now();
            stats.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();
            
        } catch (const std::exception& e) {
            stats.error_message = "Ingestion failed: " + std::string(e.what());
        }
        
        return stats;
    }
    
private:
    // Helper: Extract text from file based on format
    std::string extractTextFromFile(const fs::path& file_path) {
        auto ext = file_path.extension().string();
        std::string content;
        
        // Read file content
        std::ifstream file(file_path, std::ios::binary);
        if (!file) {
            return content;
        }
        
        // For now, simple text extraction
        if (ext == ".txt") {
            // Plain text - read directly
            content.assign(std::istreambuf_iterator<char>(file),
                          std::istreambuf_iterator<char>());
        } else if (ext == ".pdf") {
            // PDF - would use PDF library or OCR
            // For now, return placeholder
            content = "PDF content extraction not yet implemented";
            
            if (ocr_config_.enabled && !ocr_config_.skip_text_pdfs) {
                // OCR processing would go here
                // content = performOCR(file_path);
            }
        } else if (ext == ".docx") {
            // DOCX - would use docx library
            content = "DOCX content extraction not yet implemented";
        } else {
            // Try reading as text
            content.assign(std::istreambuf_iterator<char>(file),
                          std::istreambuf_iterator<char>());
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

} // namespace ingestion
} // namespace themis
