/**
 * @file blob_backend_s3.h
 * @brief AWS S3 (and S3-compatible) Blob Storage backend for ThemisDB.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Production Ready
 */

#pragma once

#include "storage/blob_storage_backend.h"
#include <string>
#include <vector>
#include <memory>

namespace themis {
namespace storage {

/**
 * @brief AWS S3 Blob Storage Backend
 *
 * Stores blobs in an AWS S3 bucket (or any S3-compatible object store) using
 * the AWS SDK for C++ v3.
 *
 * The backend is conditionally compiled behind THEMIS_HAS_AWS_SDK. When the
 * SDK is absent the constructor marks the instance as unavailable and all
 * operations return an error, so no unimplemented path is reachable at runtime.
 *
 * Authentication uses the standard AWS credential chain (environment variables,
 * ~/.aws/credentials, instance-profile metadata).
 *
 * Thread-Safety: All methods are thread-safe.
 */
class S3BlobBackend : public IBlobStorageBackend {
public:
    /**
     * @brief Construct an S3 Blob Storage backend.
     * @param bucket        Name of the S3 bucket.
     * @param region        AWS region (e.g. "us-east-1").
     * @param prefix        Optional object-key prefix (e.g. "blobs/").
     * @param sse_config    Server-side encryption configuration (default: no SSE).
     * @param retry_policy  Retry and backoff configuration (default: 3 retries, exponential).
     * @param endpoint_override Optional S3-compatible endpoint override for local
     *        emulators such as MinIO. Accepts either `host:port` or a full
     *        `http[s]://host:port` URL.
     * @param force_path_style When true, use path-style addressing instead of
     *        virtual-host style. Required by many local S3 emulators.
     */
    explicit S3BlobBackend(const std::string& bucket,
                            const std::string& region,
                            const std::string& prefix = "",
                            const SseConfig&   sse_config = SseConfig{},
                            const RetryPolicy& retry_policy = RetryPolicy{},
                            const std::string& endpoint_override = "",
                            bool               force_path_style = false);

    ~S3BlobBackend() override;

    /**
     * @brief Store a blob in the S3 bucket.
     * @param blob_id Unique blob identifier (used as the S3 object key).
     * @param data    Raw blob data.
     * @return BlobRef on success, or an error if the SDK is unavailable or the
     *         upload fails.
     */
    [[nodiscard]] Result<BlobRef> put(const std::string& blob_id,
                                      const std::vector<uint8_t>& data) override;

    /**
     * @brief Retrieve a blob from the S3 bucket.
     * @param ref Blob reference previously returned by put().
     * @return Blob data on success, or an error if the object does not exist or
     *         the download fails.
     */
    [[nodiscard]] Result<std::vector<uint8_t>> get(const BlobRef& ref) override;

    /**
     * @brief Delete a blob from the S3 bucket.
     * @param ref Blob reference previously returned by put().
     * @return void on success, or an error if deletion fails.
     */
    [[nodiscard]] Result<void> remove(const BlobRef& ref) override;

    /**
     * @brief Check whether a blob exists in the bucket.
     * @param ref Blob reference to check.
     * @return true if the object exists.
     */
    [[nodiscard]] bool exists(const BlobRef& ref) override;

    /**
     * @brief Return the backend name ("s3").
     */
    [[nodiscard]] std::string name() const override;

    /**
     * @brief Check whether the S3 backend is operational.
     *
     * Returns false when the AWS SDK was not compiled in, when credentials are
     * missing, or when the bucket cannot be reached.
     */
    [[nodiscard]] bool isAvailable() const override;

    /**
     * @brief Generate a presigned GET URL for the given blob.
     * @param ref      Blob reference previously returned by put().
     * @param expiry_s URL validity in seconds (must be > 0 and ≤ 604800).
     * @return Presigned URL string, or an error if the SDK is unavailable or
     *         signing fails.
     */
    [[nodiscard]] Result<std::string> presignedUrl(const BlobRef& ref,
                                                    int64_t expiry_s) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    static std::string computeSHA256(const std::vector<uint8_t>& data);
    std::string getS3Key(const std::string& blob_id) const;
};

} // namespace storage
} // namespace themis
