/**
 * @file blob_backend_gcs.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: blob_backend_gcs.h | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "storage/blob_storage_backend.h"
#include <string>
#include <vector>

namespace themis {
namespace storage {

/**
 * @brief Google Cloud Storage (GCS) Blob Backend
 *
 * Stores blobs in a GCS bucket using the google-cloud-cpp Storage SDK.
 *
 * Authentication uses Application Default Credentials (ADC) via the
 * GOOGLE_APPLICATION_CREDENTIALS environment variable.  The constructor
 * fails-closed (marks the backend unavailable) when no valid credentials
 * are present so that THEMIS_PRODUCTION_MODE safety guarantees are upheld.
 *
 * Thread-Safety: All methods are thread-safe.
 */
class GCSBlobBackend : public IBlobStorageBackend {
public:
    /**
     * @param bucket   GCS bucket name
     * @param prefix   Optional object-name prefix (e.g. "blobs/")
     */
    explicit GCSBlobBackend(const std::string& bucket, const std::string& prefix = "");
    ~GCSBlobBackend() override;

    Result<BlobRef>              put(const std::string& blob_id,
                                    const std::vector<uint8_t>& data) override;
    Result<std::vector<uint8_t>> get(const BlobRef& ref) override;
    Result<void>                 remove(const BlobRef& ref) override;
    bool                         exists(const BlobRef& ref) override;
    std::string                  name() const override;
    bool                         isAvailable() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    static std::string computeSHA256(const std::vector<uint8_t>& data);
    std::string objectName(const std::string& blob_id) const;
};

} // namespace storage
} // namespace themis
