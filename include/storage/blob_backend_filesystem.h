/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            blob_backend_filesystem.h                          ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:42:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     57                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "storage/blob_storage_backend.h"
#include <string>
#include <vector>
#include <optional>

namespace themis {
namespace storage {

class FilesystemBlobBackend : public IBlobStorageBackend {
public:
    explicit FilesystemBlobBackend(const std::string& base_path);
    ~FilesystemBlobBackend() override = default;

    Result<BlobRef> put(const std::string& blob_id, const std::vector<uint8_t>& data) override;
    Result<std::vector<uint8_t>> get(const BlobRef& ref) override;
    Result<void> remove(const BlobRef& ref) override;
    bool exists(const BlobRef& ref) override;
    std::string name() const override;
    bool isAvailable() const override;

private:
    std::string base_path_;

    static std::string computeSHA256(const std::vector<uint8_t>& data);
    std::string getPath(const std::string& blob_id) const;
};

} // namespace storage
} // namespace themis
