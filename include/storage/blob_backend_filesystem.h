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

    BlobRef put(const std::string& blob_id, const std::vector<uint8_t>& data) override;
    std::optional<std::vector<uint8_t>> get(const BlobRef& ref) override;
    bool remove(const BlobRef& ref) override;
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
