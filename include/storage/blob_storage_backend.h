/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            blob_storage_backend.h                             ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:05:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     165                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 492304352e  2026-03-09  feat(storage): add GCS blob backend, tiered storage, and ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <cstdint>
#include "utils/expected.h"

namespace themis {
namespace storage {

/**
 * @brief Blob Storage Type
 */
enum class BlobStorageType {
    INLINE,       // RocksDB inline (< 1 MB)
    ROCKSDB_BLOB, // RocksDB BlobDB (1-10 MB)
    FILESYSTEM,   // Local filesystem
    S3,           // AWS S3
    AZURE_BLOB,   // Azure Blob Storage
    GCS,          // Google Cloud Storage
    WEBDAV,       // WebDAV (for ActiveDirectory/SharePoint integration)
    CUSTOM        // User-defined backend
};

/**
 * @brief Blob Reference
 * 
 * Contains metadata about a blob stored in an external backend.
 */
struct BlobRef {
    std::string id;           // Blob ID (UUID)
    BlobStorageType type;     // Storage backend type
    std::string uri;          // Backend-specific URI
    int64_t size_bytes;       // Original size
    std::string hash_sha256;  // Content hash (for integrity)
    
    // Optional metadata
    int64_t created_at = 0;   // Unix timestamp
    bool compressed = false;   // Is blob compressed?
    std::string compression_type; // e.g., "zstd"
};

/**
 * @brief Blob Storage Backend Interface
 * 
 * Abstract interface for external blob storage backends.
 * Implementations include: Filesystem, S3, Azure Blob, WebDAV (ActiveDirectory).
 * 
 * Thread-Safety: Implementations must be thread-safe.
 */
class IBlobStorageBackend {
public:
    virtual ~IBlobStorageBackend() = default;
    
    /**
     * @brief Store a blob
     * @param blob_id Unique blob identifier
     * @param data Blob data
     * @return Result<BlobRef> Reference to stored blob or error
     */
    virtual Result<BlobRef> put(
        const std::string& blob_id,
        const std::vector<uint8_t>& data
    ) = 0;
    
    /**
     * @brief Retrieve a blob
     * @param ref Blob reference
     * @return Result<vector<uint8_t>> Blob data or error if not found
     */
    virtual Result<std::vector<uint8_t>> get(
        const BlobRef& ref
    ) = 0;
    
    /**
     * @brief Delete a blob
     * @param ref Blob reference
     * @return Result<bool> Success or error
     */
    virtual Result<void> remove(const BlobRef& ref) = 0;
    
    /**
     * @brief Check if blob exists
     * @param ref Blob reference
     * @return true if exists
     */
    virtual bool exists(const BlobRef& ref) = 0;
    
    /**
     * @brief Get backend name
     * @return Backend name (e.g., "filesystem", "s3", "webdav")
     */
    virtual std::string name() const = 0;
    
    /**
     * @brief Check if backend is available
     * @return true if backend can be used
     */
    virtual bool isAvailable() const = 0;
};

/**
 * @brief Blob Storage Configuration
 */
struct BlobStorageConfig {
    // Thresholds
    int64_t inline_threshold_bytes = 1024 * 1024;      // 1 MB - inline in RocksDB
    int64_t rocksdb_blob_threshold_bytes = 10 * 1024 * 1024; // 10 MB - RocksDB BlobDB
    
    // Filesystem backend
    bool enable_filesystem = true;
    std::string filesystem_base_path = "./data/blobs";
    
    // S3 backend
    bool enable_s3 = false;
    std::string s3_bucket;
    std::string s3_region = "us-east-1";
    std::string s3_prefix;
    
    // Azure backend
    bool enable_azure = false;
    std::string azure_connection_string;
    std::string azure_container;
    
    // GCS backend
    bool enable_gcs = false;
    std::string gcs_bucket;
    std::string gcs_prefix;
    // Credentials: uses GOOGLE_APPLICATION_CREDENTIALS env var (ADC); fail-closed if absent

    // WebDAV backend (for ActiveDirectory/SharePoint)
    bool enable_webdav = false;
    std::string webdav_base_url;
    std::string webdav_username;
    std::string webdav_password;
    bool webdav_verify_ssl = true;
};

} // namespace storage
} // namespace themis
