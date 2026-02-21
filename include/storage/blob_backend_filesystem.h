/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            blob_backend_filesystem.h                          ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:07:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     57                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
