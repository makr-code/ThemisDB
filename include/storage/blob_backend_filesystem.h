/**
 * @file blob_backend_filesystem.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
