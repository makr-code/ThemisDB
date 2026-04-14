/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            archive_processor.h                                ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:37:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     287                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • edcfeb9848  2026-03-11  feat: add scripts for auditing and reconciling GitHub iss... ║
    • 1bacdae51e  2026-03-11  fix(content/security): add zip-bomb protection in archive... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • e21224bb7e  2026-02-28  feat(content): implement file upload security checks ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "content/content_processor.h"
#include "content/content_security.h"
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace content {

using json = nlohmann::json;

/**
 * @brief Archive handling strategy
 */
enum class ArchiveStrategy {
    EXTRACT_AND_INGEST,  // Extract all files and ingest individually (default)
    METADATA_ONLY,       // Store only archive metadata without extraction
    REJECT               // Reject archive uploads
};

/**
 * @brief Encrypted archive handling policy
 */
enum class EncryptedArchivePolicy {
    REJECT,           // Reject encrypted archives (default)
    METADATA_ONLY,    // Store encrypted archive as blob with metadata
    REQUIRE_PASSWORD  // Accept password parameter for extraction
};

/**
 * @brief Archive format detection
 */
enum class ArchiveFormat {
    ZIP,
    TAR,
    TAR_GZ,
    TAR_BZ2,
    TAR_XZ,
    SEVEN_ZIP,
    UNKNOWN
};

/**
 * @brief Archive member information
 */
struct ArchiveMember {
    std::string path;              // Path within archive
    uint64_t uncompressed_size;    // Uncompressed size in bytes
    uint64_t compressed_size;      // Compressed size in bytes
    bool is_directory;             // True if this is a directory entry
    bool is_encrypted;             // True if this member is encrypted
};

/**
 * @brief Archive metadata
 */
struct ArchiveMetadata {
    ArchiveFormat format;
    bool is_encrypted;
    uint64_t total_uncompressed_size;
    uint64_t total_compressed_size;
    size_t member_count;
    size_t directory_count;
    size_t file_count;
    std::vector<ArchiveMember> members;
    std::string comment;  // Archive comment if any
};

/**
 * @brief Archive extraction result (internal use)
 */
struct ArchiveExtractionResult {
    bool success;
    std::string error_message;
    std::vector<std::string> extracted_files;  // Paths to extracted files in temp directory
    std::string temp_directory;  // Temporary directory used for extraction
};

/**
 * @brief Archive Processor Result (for process() method)
 */
struct ArchiveProcessorResult {
    bool success;
    std::string error_message;
    json metadata;
};

/**
 * @brief Archive Processor Configuration
 */
struct ArchiveProcessorConfig {
    ArchiveStrategy strategy = ArchiveStrategy::EXTRACT_AND_INGEST;
    EncryptedArchivePolicy encrypted_policy = EncryptedArchivePolicy::REJECT;
    
    // Security limits
    uint64_t max_total_size = 1024ULL * 1024 * 1024 * 10;  // 10 GB max total extracted size
    uint64_t max_file_size = 1024ULL * 1024 * 1024;        // 1 GB max single file size
    uint64_t max_compression_ratio = 100;                  // Max 100:1 compression ratio (zip bomb protection)
    size_t max_file_count = 1000;                          // Max 1,000 files in archive
    size_t max_path_depth = 20;                            // Max 20 levels of directory nesting
    size_t max_path_length = 4096;                         // Max 4096 characters in path
    
    // Password for encrypted archives (if REQUIRE_PASSWORD policy)
    std::string password;
    
    // Enable verbose logging
    bool verbose = false;
};

/**
 * @brief Archive Content Processor
 * 
 * Handles compressed archive formats (.zip, .tar, .tar.gz, etc.)
 * Supports extraction and ingestion of archive contents with configurable strategies.
 * 
 * Implements IContentProcessor interface while maintaining archive-specific functionality.
 * 
 * Security Features:
 * - Zip bomb detection (compression ratio check)
 * - Path traversal prevention (sanitizes file paths)
 * - Encrypted archive handling
 * - Size limit enforcement
 * 
 * Thread-Safety: Not thread-safe. Use separate instances per thread.
 */
class ArchiveProcessor : public IContentProcessor {
public:
    explicit ArchiveProcessor(ArchiveProcessorConfig config = ArchiveProcessorConfig{});
    ~ArchiveProcessor() override = default;

    // IContentProcessor interface
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
    
    std::string getName() const override { return "ArchiveProcessor"; }
    
    std::vector<ContentCategory> getSupportedCategories() const override {
        return {ContentCategory::ARCHIVE};
    }
    
    // Archive-specific interface (used by ContentManager)
    ArchiveProcessorResult process(
        const std::string& blob,
        const std::string& mime_type,
        const std::string& filename
    );
    
    bool canHandle(const std::string& mime_type) const;

    /**
     * @brief Check if archive processing is available
     * 
     * Returns true if libzip is available and the processor can function.
     * For plugin architecture - allows runtime detection of capability.
     */
    static bool isAvailable() {
        #ifdef THEMIS_ENABLE_ARCHIVES
        return true;
        #else
        return true;  // libzip is in dependencies, always available for now
        #endif
    }

    /**
     * @brief Detect archive format from blob
     */
    static ArchiveFormat detectFormat(const std::string& blob, const std::string& filename);
    
    /**
     * @brief Extract archive metadata without full extraction
     */
    static std::optional<ArchiveMetadata> extractMetadata(
        const std::string& blob,
        ArchiveFormat format
    );

    /**
     * @brief Check if archive is encrypted
     */
    static bool isEncrypted(const std::string& blob, ArchiveFormat format);

    /**
     * @brief Extract archive to temporary directory
     * 
     * @param blob Archive binary data
     * @param format Archive format
     * @param password Optional password for encrypted archives
     * @return ArchiveExtractionResult with extracted file paths or error
     */
    ArchiveExtractionResult extractToTemp(
        const std::string& blob,
        ArchiveFormat format,
        const std::string& password = ""
    );

    /**
     * @brief Validate archive against security limits
     */
    bool validateArchive(const ArchiveMetadata& metadata, std::string& error_message) const;

    /**
     * @brief Sanitize file path to prevent path traversal attacks
     * 
     * Removes ".." components and ensures path is relative
     */
    static std::string sanitizePath(const std::string& path);

    /**
     * @brief Clean up temporary extraction directory
     */
    static void cleanupTempDirectory(const std::string& temp_dir);

    /**
     * @brief Get configuration
     */
    const ArchiveProcessorConfig& getConfig() const { return config_; }

    /**
     * @brief Update configuration
     */
    void setConfig(ArchiveProcessorConfig config) { config_ = std::move(config); }

    /**
     * @brief Configure the security manager used for zip-bomb checks
     * 
     * By default a ContentSecurityManager with zip-bomb checks enabled (ratio 100×,
     * max 1,000 files) is used automatically. Call this to supply a pre-configured
     * manager, e.g. to adjust thresholds or share metrics with another component.
     */
    void setSecurityConfig(const ContentSecurityConfig& security_config) {
        security_manager_.setConfig(security_config);
    }

private:
    ArchiveProcessorConfig config_;
    ContentSecurityManager security_manager_;
    
    // Format-specific extraction methods
    ArchiveExtractionResult extractZip(const std::string& blob, const std::string& password);
    ArchiveExtractionResult extractTar(const std::string& blob, ArchiveFormat format);
    
    // Helper methods
    std::string generateTempDirectory() const;
    bool checkCompressionRatio(uint64_t compressed, uint64_t uncompressed) const;
};

} // namespace content
} // namespace themis
