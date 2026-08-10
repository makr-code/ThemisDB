/**
 * @file blob_backend_azure.h
 * @brief Azure Blob Storage backend for ThemisDB.
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
 * @brief Azure Blob Storage Backend
 *
 * Stores blobs in an Azure Blob Storage container using the Azure SDK for C++.
 *
 * The backend is conditionally compiled behind THEMIS_HAS_AZURE_STORAGE. When
 * the Azure SDK is absent the constructor marks the instance as unavailable and
 * all operations return an error, so no unimplemented path is reachable at
 * runtime.
 *
 * Authentication uses the Azure connection string supplied at construction time.
 *
 * Thread-Safety: All methods are thread-safe.
 */
class AzureBlobBackend : public IBlobStorageBackend {
public:
    /**
     * @brief Construct an Azure Blob Storage backend.
     * @param connection_string Azure Storage connection string.
     * @param container_name    Name of the target container.
     * @param prefix            Optional object-name prefix (e.g. "blobs/").
     * @param sse_config        Server-side encryption configuration (default: no SSE).
     */
    explicit AzureBlobBackend(const std::string& connection_string,
                               const std::string& container_name,
                               const std::string& prefix = "",
                               const SseConfig&   sse_config = SseConfig{});

    ~AzureBlobBackend() override;

    /**
     * @brief Store a blob in Azure Blob Storage.
     * @param blob_id Unique blob identifier (used as blob name).
     * @param data    Raw blob data.
     * @return BlobRef on success, or an error if the SDK is unavailable or the
     *         upload fails.
     */
    [[nodiscard]] Result<BlobRef> put(const std::string& blob_id,
                                      const std::vector<uint8_t>& data) override;

    /**
     * @brief Retrieve a blob from Azure Blob Storage.
     * @param ref Blob reference previously returned by put().
     * @return Blob data on success, or an error if the blob does not exist or
     *         the download fails.
     */
    [[nodiscard]] Result<std::vector<uint8_t>> get(const BlobRef& ref) override;

    /**
     * @brief Delete a blob from Azure Blob Storage.
     * @param ref Blob reference previously returned by put().
     * @return void on success, or an error if deletion fails.
     */
    [[nodiscard]] Result<void> remove(const BlobRef& ref) override;

    /**
     * @brief Check whether a blob exists in the container.
     * @param ref Blob reference to check.
     * @return true if the blob exists.
     */
    [[nodiscard]] bool exists(const BlobRef& ref) override;

    /**
     * @brief Return the backend name ("azure").
     */
    [[nodiscard]] std::string name() const override;

    /**
     * @brief Check whether the Azure backend is operational.
     *
     * Returns false when the Azure SDK was not compiled in, when the
     * connection string is invalid, or when the container cannot be reached.
     */
    [[nodiscard]] bool isAvailable() const override;

    /**
     * @brief Generate a presigned (SAS) URL for the given blob.
     * @param ref      Blob reference previously returned by put().
     * @param expiry_s URL validity in seconds (must be > 0 and ≤ 604800).
     * @return SAS URL string, or an error if the SDK is unavailable or
     *         SAS generation fails.
     */
    [[nodiscard]] Result<std::string> presignedUrl(const BlobRef& ref,
                                                    int64_t expiry_s) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    static std::string computeSHA256(const std::vector<uint8_t>& data);
    std::string getBlobName(const std::string& blob_id) const;
};

} // namespace storage
} // namespace themis
