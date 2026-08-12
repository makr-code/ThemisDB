/**
 * @file content_fs.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/** @brief Content fs. */
class ContentFS {
public:
    explicit ContentFS(RocksDBWrapper& db) : db_(db) {}
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

    // Get full blob
    // Returns Result<std::vector<uint8_t>> with possible errors:
    // - ERR_STORAGE_FILE_NOT_FOUND: Content not found
    // - ERR_STORAGE_CORRUPTION: Invalid metadata or missing chunk
    Result<std::vector<uint8_t>> get(const std::string& pk) const;

    // Range read [offset, offset+length) (length==0 => to end)
    // Returns Result<std::vector<uint8_t>> with possible errors:
    // - ERR_STORAGE_FILE_NOT_FOUND: Content not found
    // - ERR_API_INVALID_REQUEST: Offset beyond file size
    // - ERR_STORAGE_CORRUPTION: Invalid metadata or missing chunk
    Result<std::vector<uint8_t>> getRange(const std::string& pk, uint64_t offset, uint64_t length) const;

    // Head (metadata only)
    // Returns Result<ContentMeta> with possible errors:
    // - ERR_STORAGE_FILE_NOT_FOUND: Content not found
    // - ERR_STORAGE_CORRUPTION: Invalid metadata
    Result<ContentMeta> head(const std::string& pk) const;

    // Delete blob + meta
    // Returns Result<void> with possible errors:
    // - ERR_STORAGE_FILE_NOT_FOUND: Content not found (warning only, still succeeds)
    Result<void> remove(const std::string& pk);

    // Utility: compute SHA-256 hex for buffer
    static std::string sha256Hex(const std::vector<uint8_t>& data);

private:
    RocksDBWrapper& db_;
    uint64_t chunk_size_bytes_ = kDefaultChunkSize;
    static std::string metaKey(const std::string& pk) { return std::string("content:") + pk + ":meta"; }
    static std::string blobKey(const std::string& pk) { return std::string("content:") + pk + ":blob"; }
    static std::string chunkKey(const std::string& pk, uint64_t idx) {
        return std::string("content:") + pk + ":chunk:" + std::to_string(idx);
    }
public:
    static constexpr uint64_t kDefaultChunkSize = 1024ull * 1024ull; // 1 MiB
};

} // namespace themis
