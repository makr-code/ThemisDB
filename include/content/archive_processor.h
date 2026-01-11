#pragma once

#include "content/content_plugin_interface.h"
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
 * @brief Archive extraction result
 */
struct ExtractionResult {
    bool success;
    std::string error_message;
    std::vector<std::string> extracted_files;  // Paths to extracted files in temp directory
    std::string temp_directory;  // Temporary directory used for extraction
};

/**
 * @brief Archive Processor Plugin
 * 
 * Implements ThemisDB plugin interface for archive processing.
 * Handles compressed archive formats (.zip, .tar, .tar.gz, etc.)
 * 
 * Security Features:
 * - Zip bomb detection (compression ratio check)
 * - Path traversal prevention (sanitizes file paths)
 * - Encrypted archive handling
 * - Size limit enforcement
 * 
 * Plugin Lifecycle:
 * 1. Load: themis_create_plugin()
 * 2. Init: initialize(config)
 * 3. Use: extract(), chunk()
 * 4. Shutdown: shutdown()
 * 5. Unload: themis_destroy_plugin()
 */
class ArchiveProcessorPlugin : public IContentProcessorPlugin {
public:
    ArchiveProcessorPlugin();
    ~ArchiveProcessorPlugin() override = default;

    // IContentProcessorPlugin interface
    PluginInfo getInfo() const override;
    bool initialize(const PluginConfig& config) override;
    void shutdown() override;
    bool canProcess(const std::string& mime_type) const override;
    
    ContentExtractionResult extract(
        const std::vector<uint8_t>& blob,
        const std::string& mime_type,
        const ExtractionOptions& options = {}
    ) override;
    
    std::vector<ContentChunk> chunk(
        const ContentExtractionResult& result,
        int max_tokens,
        int overlap
    ) override;
    
    bool healthCheck() const override;
    json getStatistics() const override;

    /**
     * @brief Detect archive format from blob
     */
    static ArchiveFormat detectFormat(const std::vector<uint8_t>& blob, const std::string& filename);
    
    /**
     * @brief Extract archive metadata without full extraction
     */
    static std::optional<ArchiveMetadata> extractMetadata(
        const std::vector<uint8_t>& blob,
        ArchiveFormat format
    );

    /**
     * @brief Check if archive is encrypted
     */
    static bool isEncrypted(const std::vector<uint8_t>& blob, ArchiveFormat format);

    /**
     * @brief Sanitize file path to prevent path traversal attacks
     */
    static std::string sanitizePath(const std::string& path);

    /**
     * @brief Clean up temporary extraction directory
     */
    static void cleanupTempDirectory(const std::string& temp_dir);

private:
    // Configuration (loaded from plugin config)
    ArchiveStrategy strategy_;
    EncryptedArchivePolicy encrypted_policy_;
    uint64_t max_total_size_;
    uint64_t max_file_size_;
    uint64_t max_compression_ratio_;
    size_t max_file_count_;
    size_t max_path_depth_;
    size_t max_path_length_;
    std::string password_;
    bool verbose_;
    
    // Statistics
    std::atomic<uint64_t> archives_processed_;
    std::atomic<uint64_t> files_extracted_;
    std::atomic<uint64_t> total_bytes_processed_;
    std::atomic<uint64_t> errors_count_;
    
    // Extraction methods
    ExtractionResult extractZip(const std::vector<uint8_t>& blob, const std::string& password);
    ExtractionResult extractTar(const std::vector<uint8_t>& blob, ArchiveFormat format);
    
    // Validation
    bool validateArchive(const ArchiveMetadata& metadata, std::string& error_message) const;
    bool checkCompressionRatio(uint64_t compressed, uint64_t uncompressed) const;
    
    // Helpers
    std::string generateTempDirectory() const;
};

} // namespace content
} // namespace themis
