/**
 * @file archive_processor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

enum class ArchiveStrategy {
    EXTRACT_AND_INGEST,  // Extract all files and ingest individually (default)
    METADATA_ONLY,       // Store only archive metadata without extraction
    REJECT               // Reject archive uploads
};

enum class EncryptedArchivePolicy {
    REJECT,           // Reject encrypted archives (default)
    METADATA_ONLY,    // Store encrypted archive as blob with metadata
    REQUIRE_PASSWORD  // Accept password parameter for extraction
};

enum class ArchiveFormat {
    ZIP,
    TAR,
    TAR_GZ,
    TAR_BZ2,
    TAR_XZ,
    SEVEN_ZIP,
    UNKNOWN
};

struct ArchiveMember {
    std::string path;                       // Path within archive
    uint64_t uncompressed_size = 0;         ///< Uncompressed size in bytes (CON-019)
    uint64_t compressed_size = 0;           ///< Compressed size in bytes (CON-019)
    bool is_directory = false;              ///< True if this is a directory entry (CON-019)
    bool is_encrypted = false;              ///< True if this member is encrypted (CON-019)
};

struct ArchiveMetadata {
    ArchiveFormat format;
    bool is_encrypted = false;              ///< CON-019
    uint64_t total_uncompressed_size = 0;   ///< CON-019
    uint64_t total_compressed_size = 0;     ///< CON-019
    size_t member_count = 0;               ///< CON-019
    size_t directory_count = 0;            ///< CON-019
    size_t file_count = 0;                 ///< CON-019
    std::vector<ArchiveMember> members;
    std::string comment;  // Archive comment if any
};

struct ArchiveExtractionResult {
    bool success = false;  ///< CON-019
    std::string error_message;
    std::vector<std::string> extracted_files;  // Paths to extracted files in temp directory
    std::string temp_directory;  // Temporary directory used for extraction
};

struct ArchiveProcessorResult {
    bool success = false;  ///< CON-019
    std::string error_message;
    json metadata;
};

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
    
    /**
     * @brief Process.
     * @param[in] blob Input parameter.
     * @param[in] mime_type Input parameter.
     * @param[in] filename Input parameter.
     * @return Return value.
     */
    ArchiveProcessorResult process(
        const std::string& blob,
        const std::string& mime_type,
        const std::string& filename
    );
    
    /**
     * @brief Can Handle.
     * @param[in] mime_type Input parameter.
     * @return True when the operation succeeds.
     */
    bool canHandle(const std::string& mime_type) const;

    /**
     * @brief Is Available.
     * @return True when the operation succeeds.
     * @details Implements isAvailable without additional internal calls.
     */
    static bool isAvailable() {
        #ifdef THEMIS_ENABLE_ARCHIVES
        return true;
        #else
        return true;  // libzip is in dependencies, always available for now
        #endif
    }

    /**
     * @brief Detect Format.
     * @param[in] blob Input parameter.
     * @param[in] filename Input parameter.
     * @return Return value.
     */
    static ArchiveFormat detectFormat(const std::string& blob, const std::string& filename);
    
    /**
     * @brief Extract Metadata.
     * @param[in] blob Input parameter.
     * @param[in] format Input parameter.
     * @return Return value.
     */
    static std::optional<ArchiveMetadata> extractMetadata(
        const std::string& blob,
        ArchiveFormat format
    );

    /**
     * @brief Is Encrypted.
     * @param[in] blob Input parameter.
     * @param[in] format Input parameter.
     * @return True when the operation succeeds.
     */
    static bool isEncrypted(const std::string& blob, ArchiveFormat format);

    ArchiveExtractionResult extractToTemp(
        const std::string& blob,
        ArchiveFormat format,
        const std::string& password = ""
    );

    /**
     * @brief Validate Archive.
     * @param[in] metadata Input parameter.
     * @param[in,out] error_message Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool validateArchive(const ArchiveMetadata& metadata, std::string& error_message) const;

    /**
     * @brief Sanitize Path.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    static std::string sanitizePath(const std::string& path);

    /**
     * @brief Cleanup Temp Directory.
     * @param[in] temp_dir Input parameter.
     */
    static void cleanupTempDirectory(const std::string& temp_dir);

    const ArchiveProcessorConfig& getConfig() const { return config_; }

    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     * @details Calls: std::move().
     */
    void setConfig(ArchiveProcessorConfig config) { config_ = std::move(config); }

    /**
     * @brief Set Security Config.
     * @param[in] security_config Input parameter.
     * @details Calls: setConfig().
     */
    void setSecurityConfig(const ContentSecurityConfig& security_config) {
        security_manager_.setConfig(security_config);
    }

private:
    ArchiveProcessorConfig config_;
    ContentSecurityManager security_manager_;
    
    /**
     * @brief Extract Zip.
     * @param[in] blob Input parameter.
     * @param[in] password Input parameter.
     * @return Return value.
     */
    ArchiveExtractionResult extractZip(const std::string& blob, const std::string& password);
    /**
     * @brief Extract Tar.
     * @param[in] blob Input parameter.
     * @param[in] format Input parameter.
     * @return Return value.
     */
    ArchiveExtractionResult extractTar(const std::string& blob, ArchiveFormat format);
    
    // Helper methods
    /**
     * @brief Generate Temp Directory.
     * @return Return value.
     */
    std::string generateTempDirectory() const;
    /**
     * @brief Check Compression Ratio.
     * @param[in] compressed Input parameter.
     * @param[in] uncompressed Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkCompressionRatio(uint64_t compressed, uint64_t uncompressed) const;
};

} // namespace content
} // namespace themis
