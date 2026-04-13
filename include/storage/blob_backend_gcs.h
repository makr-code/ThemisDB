/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            blob_backend_gcs.h                                 ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-13 04:20:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     72                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • edcfeb9848  2026-03-11  feat: add scripts for auditing and reconciling GitHub iss... ║
    • bea3655f53  2026-03-09  fix(storage): address code review comments - path travers... ║
    • 492304352e  2026-03-09  feat(storage): add GCS blob backend, tiered storage, and ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
