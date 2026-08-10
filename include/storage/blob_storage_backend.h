/**
 * @file blob_storage_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: blob_storage_backend.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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

// ─────────────────────────────────────────────────────────────────────────────
// Server-Side Encryption configuration
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Server-Side Encryption algorithm selector.
 *
 * Controls which SSE mechanism the backend applies when storing blobs.
 */
enum class SseAlgorithm {
    NONE,       ///< No SSE — plaintext at rest (default)
    AES256,     ///< AWS SSE-S3 / AES-256 managed key
    AWS_KMS,    ///< AWS SSE-KMS with customer-managed CMK
    AZURE_CMK,  ///< Azure customer-managed key (CMK) via Key Vault
    GCS_CSEK,   ///< GCS customer-supplied encryption key (CSEK, 256-bit)
};

/**
 * @brief Per-upload server-side encryption configuration.
 *
 * Attach an instance to the relevant sub-config in BlobStorageConfig.
 * Backends that do not support a given algorithm return an error at runtime.
 */
struct SseConfig {
    SseAlgorithm algorithm  = SseAlgorithm::NONE;
    std::string  kms_key_id;   ///< AWS KMS ARN or Azure CMK resource ID
    std::string  csek_base64;  ///< Base64-encoded 256-bit key for GCS CSEK
};

/**
 * @brief Retry backoff strategy selector.
 *
 * Controls how the backend computes the delay between consecutive retries of a
 * failed operation.
 */
enum class RetryBackoffStrategy {
    EXPONENTIAL,  ///< Delay doubles on each attempt (with optional full jitter)
    FIXED,        ///< Constant delay between retries
};

/**
 * @brief Configurable retry policy for blob storage backends.
 *
 * Each backend maps this policy to its SDK-native retry mechanism as closely as
 * possible.  Backends that do not expose fine-grained retry configuration will
 * honour @p max_retries and ignore the timing parameters.
 *
 * Default values follow AWS SDK conventions (3 retries, 100 ms initial backoff,
 * 20 000 ms ceiling).
 */
struct RetryPolicy {
    /// Maximum number of retry attempts (0 = no retries).  Must be ≤ 10.
    int                  max_retries         = 3;
    /// Initial delay before the first retry, in milliseconds.
    int64_t              initial_backoff_ms  = 100;
    /// Upper bound on the computed per-attempt delay, in milliseconds.
    int64_t              max_backoff_ms      = 20'000;
    /// Backoff strategy to apply between retry attempts.
    RetryBackoffStrategy backoff_strategy    = RetryBackoffStrategy::EXPONENTIAL;
};

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
    [[nodiscard]] virtual Result<BlobRef> put(
        const std::string& blob_id,
        const std::vector<uint8_t>& data
    ) = 0;
    
    /**
     * @brief Retrieve a blob
     * @param ref Blob reference
     * @return Result<vector<uint8_t>> Blob data or error if not found
     */
    [[nodiscard]] virtual Result<std::vector<uint8_t>> get(
        const BlobRef& ref
    ) = 0;
    
    /**
     * @brief Delete a blob
     * @param ref Blob reference
     * @return Result<bool> Success or error
     */
    [[nodiscard]] virtual Result<void> remove(const BlobRef& ref) = 0;
    
    /**
     * @brief Check if blob exists
     * @param ref Blob reference
     * @return true if exists
     */
    [[nodiscard]] virtual bool exists(const BlobRef& ref) = 0;
    
    /**
     * @brief Get backend name
     * @return Backend name (e.g., "filesystem", "s3", "webdav")
     */
    [[nodiscard]] virtual std::string name() const = 0;
    
    /**
     * @brief Check if backend is available
     * @return true if backend can be used
     */
    [[nodiscard]] virtual bool isAvailable() const = 0;

    /**
     * @brief Generate a presigned URL for direct client access to a blob.
     *
     * Returns a time-limited URL that can be used by an HTTP client to
     * download (GET) the blob without presenting ThemisDB credentials.
     *
     * @param ref      Blob reference previously returned by put().
     * @param expiry_s URL validity period in seconds (must be > 0 and ≤ 604800).
     * @return Presigned URL string, or an error if the backend does not
     *         support presigned URLs or the signing operation fails.
     *
     * @note The default implementation returns an error. Backends that support
     *       presigned URLs override this method.
     */
    [[nodiscard]] virtual Result<std::string> presignedUrl(
        const BlobRef& /*ref*/,
        int64_t        /*expiry_s*/)
    {
        return Err<std::string>(
            errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
            "presigned URLs not supported by this backend");
    }
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
    std::string s3_endpoint_override;  ///< Optional custom S3 endpoint (e.g. MinIO host:port)
    bool        s3_force_path_style = false;  ///< Use path-style addressing for S3-compatible emulators
    SseConfig   s3_sse_config;  ///< SSE config for S3 (default: SseAlgorithm::NONE)
    
    // Azure backend
    bool enable_azure = false;
    std::string azure_connection_string;
    std::string azure_container;
    SseConfig   azure_sse_config;  ///< SSE config for Azure (default: SseAlgorithm::NONE)
    
    // GCS backend
    bool enable_gcs = false;
    std::string gcs_bucket;
    std::string gcs_prefix;
    SseConfig   gcs_sse_config;  ///< SSE config for GCS (default: SseAlgorithm::NONE)
    // Credentials: uses GOOGLE_APPLICATION_CREDENTIALS env var (ADC); fail-closed if absent

    // WebDAV backend (for ActiveDirectory/SharePoint)
    bool enable_webdav = false;
    std::string webdav_base_url;
    std::string webdav_username;
    std::string webdav_password;
    bool webdav_verify_ssl = true;

    // Retry policy — applied uniformly to all enabled backends.
    // Individual backends may expose finer-grained knobs via their own SDK; this
    // policy provides the authoritative user-visible configuration surface.
    RetryPolicy retry_policy;  ///< Retry and backoff configuration (default: 3 retries, exponential)
};

} // namespace storage
} // namespace themis
