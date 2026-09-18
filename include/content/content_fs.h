/**
 * @file content_fs.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include "storage/rocksdb_wrapper.h"
#include "utils/expected.h"

namespace themis {

struct ContentMeta {
    std::string pk;
    std::string mime;
    uint64_t size = 0;
    std::string sha256_hex; // optional: empty if not provided
    uint64_t chunk_size = 0; // 0 => ungechunked (vollständiger Blob)
    uint64_t chunks = 0;     // Anzahl der Chunks (0 => ungechunked)
};

class ContentFS {
public:
    explicit ContentFS(RocksDBWrapper& db) : db_(db) {}
    /**
     * @brief Set Chunk Size Bytes.
     * @param[in] sz Input parameter.
     * @details Implements setChunkSizeBytes without additional internal calls.
     */
    void setChunkSizeBytes(uint64_t sz) { chunk_size_bytes_ = sz == 0 ? kDefaultChunkSize : sz; }
    uint64_t getChunkSizeBytes() const { return chunk_size_bytes_; }

    // Store entire blob in one value under content:<pk>:blob, metadata in content:<pk>:meta
    // Returns Result<void> with possible errors:
    // - ERR_API_INVALID_REQUEST: Empty pk or checksum mismatch
    // - ERR_STORAGE_DISK_FULL: Failed to write chunk/blob/meta
    Result<void> put(const std::string& pk,
               const std::vector<uint8_t>& data,
               const std::string& mime,
               const std::optional<std::string>& sha256_expected_hex = std::nullopt);

    /**
     * @brief Get.
     * @param[in] pk Input parameter.
     * @return Return value.
     */
    Result<std::vector<uint8_t>> get(const std::string& pk) const;

    /**
     * @brief Get Range.
     * @param[in] pk Input parameter.
     * @param[in] offset Input parameter.
     * @param[in] length Input parameter.
     * @return Return value.
     */
    Result<std::vector<uint8_t>> getRange(const std::string& pk, uint64_t offset, uint64_t length) const;

    /**
     * @brief Head.
     * @param[in] pk Input parameter.
     * @return Return value.
     */
    Result<ContentMeta> head(const std::string& pk) const;

    /**
     * @brief Remove.
     * @param[in] pk Input parameter.
     * @return Return value.
     */
    Result<void> remove(const std::string& pk);

    /**
     * @brief Sha256 Hex.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static std::string sha256Hex(const std::vector<uint8_t>& data);

private:
    RocksDBWrapper& db_;
    uint64_t chunk_size_bytes_ = kDefaultChunkSize;
    /**
     * @brief Meta Key.
     * @param[in] pk Input parameter.
     * @return Return value.
     * @details Calls: std::string().
     */
    static std::string metaKey(const std::string& pk) { return std::string("content:") + pk + ":meta"; }
    /**
     * @brief Blob Key.
     * @param[in] pk Input parameter.
     * @return Return value.
     * @details Calls: std::string().
     */
    static std::string blobKey(const std::string& pk) { return std::string("content:") + pk + ":blob"; }
    /**
     * @brief Chunk Key.
     * @param[in] pk Input parameter.
     * @param[in] idx Input parameter.
     * @return Return value.
     * @details Calls: std::string(), std::to_string().
     */
    static std::string chunkKey(const std::string& pk, uint64_t idx) {
        return std::string("content:") + pk + ":chunk:" + std::to_string(idx);
    }
public:
    static constexpr uint64_t kDefaultChunkSize = 1024ull * 1024ull; // 1 MiB
};

} // namespace themis
