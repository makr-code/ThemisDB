/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            blob_backend_filesystem.h                          ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:58:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     57                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • dc415c458  2026-01-23  Refactor error handling and return types across multiple ... ║
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
