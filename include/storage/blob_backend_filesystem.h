/**
 * @file blob_backend_filesystem.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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

/** @brief Filesystem blob backend implementation. */
class FilesystemBlobBackend : public IBlobStorageBackend {
public:
    /**
     * @brief TBD: Describe FilesystemBlobBackend.
     * @param[in] base_path Input parameter.
     * @return Return value.
     */
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

    /**
     * @brief TBD: Describe computeSHA256.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static std::string computeSHA256(const std::vector<uint8_t>& data);
    /**
     * @brief TBD: Describe getPath.
     * @param[in] blob_id Input parameter.
     * @return Return value.
     */
    std::string getPath(const std::string& blob_id) const;
};

} // namespace storage
} // namespace themis
