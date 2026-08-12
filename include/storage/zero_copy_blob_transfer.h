/**
 * @file zero_copy_blob_transfer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "storage/blob_storage_backend.h"
#include <string>
#include <cstdint>
#include <functional>
#include <memory>

namespace themis {
namespace storage {

/**
 * @brief Threshold above which memory-mapped reads are preferred (4 MB)
 */
static constexpr int64_t ZERO_COPY_MMAP_THRESHOLD_BYTES = 4LL * 1024 * 1024;

/**
 * @brief Minimum S3 multipart part size (5 MB — S3 lower bound)
 */
static constexpr int64_t S3_MULTIPART_MIN_PART_BYTES = 5LL * 1024 * 1024;

// ─────────────────────────────────────────────────────────────────────────────
// MmapBlobView
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Read-only, memory-mapped view of a blob file.
 *
 * Maps a blob file directly into the calling process's address space so that
 * the kernel can satisfy read requests from its page cache without copying
 * data to user-space buffers — eliminating one full copy compared with the
 * read(2)/fread() path.
 *
 * Usage:
 * @code
 *   MmapBlobView view("/data/blobs/aa/bb/aabb1234.blob");
 *   if (view.valid()) {
 *       process(view.data(), view.size());
 *   }
 * @endcode
 *
 * Thread-Safety: a single MmapBlobView instance is read-only after
 * construction and therefore safe to share across threads.
 */
class MmapBlobView {
public:
    /**
     * @brief Map the file at @p file_path for read-only access.
     * @param file_path        Absolute path to the blob file.
     * @param sequential_hint  When true, hints the kernel via MADV_SEQUENTIAL
     *                         for forward-sequential access patterns.
     *
     * If the mapping fails (file not found, permission denied, etc.) the object
     * is left in an invalid state; check valid() before accessing data().
     */
    explicit MmapBlobView(const std::string& file_path,
                          bool sequential_hint = true);

    ~MmapBlobView();

    // Non-copyable: the mapping is owned by this object.
    MmapBlobView(const MmapBlobView&) = delete;
    MmapBlobView& operator=(const MmapBlobView&) = delete;

    MmapBlobView(MmapBlobView&&) noexcept;
    MmapBlobView& operator=(MmapBlobView&&) noexcept;

    /**
     * @return Pointer to the first mapped byte, or nullptr if !valid().
     */
    const uint8_t* data() const noexcept { return data_; }

    /**
     * @return Number of mapped bytes (0 if !valid()).
     */
    size_t size() const noexcept { return size_; }

    /**
     * @return true when the file is successfully mapped and data() is usable.
     */
    bool valid() const noexcept { return data_ != nullptr; }

    /**
     * @brief Iterate over the mapped data in fixed-size chunks.
     *
     * @param chunk_size  Maximum bytes per callback invocation.
     * @param callback    Called with (ptr, len) for each chunk.
     *                    Return false to stop early.
     */
    void forEach(size_t chunk_size,
                 std::function<bool(const uint8_t*, size_t)> callback) const;

private:
    void releaseResources() noexcept;

    void*    mapping_ = nullptr;  // raw mmap pointer (MAP_FAILED → nullptr)
    uint8_t* data_    = nullptr;
    size_t   size_    = 0;
    int      fd_      = -1;
};

// ─────────────────────────────────────────────────────────────────────────────
// Configuration / Statistics
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Configuration for zero-copy blob transfers.
 */
struct ZeroCopyTransferConfig {
    /// Blobs at or above this size use sendfile()/mmap instead of read+write.
    int64_t zero_copy_threshold_bytes = ZERO_COPY_MMAP_THRESHOLD_BYTES;

    /// Part size for S3 multipart streaming uploads (must be >= 5 MB).
    int64_t s3_multipart_part_size_bytes = S3_MULTIPART_MIN_PART_BYTES;

    /// Hint the kernel with MADV_SEQUENTIAL when mmap-reading blobs.
    bool mmap_sequential_hint = true;
};

/**
 * @brief Statistics collected during a zero-copy transfer operation.
 */
struct ZeroCopyTransferStats {
    int64_t bytes_transferred  = 0;
    bool    used_sendfile      = false;
    bool    used_mmap          = false;
    bool    used_s3_multipart  = false;
    int64_t duration_us        = 0;   // wall-clock microseconds
    int     s3_parts_uploaded  = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// ZeroCopyBlobTransfer
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Zero-Copy Blob Transfer Utility  (v1.7.0, Issue #231)
 *
 * Eliminates memory copies when transferring blobs between storage backends
 * using three complementary techniques:
 *
 *  1. **sendfile()** — kernel-level copy from a blob file to any writable fd
 *     without staging data in user space.  Falls back to a read+write loop on
 *     platforms where sendfile() is unavailable (macOS, Windows).
 *
 *  2. **Memory-mapped reads** — maps the blob file into the process address
 *     space via mmap(2) so the kernel's page cache is accessed directly.
 *     Avoids an extra heap allocation for blobs above the configurable
 *     threshold (default: 4 MB).
 *
 *  3. **S3 multipart upload** — streams a local blob file to S3 in parallel
 *     parts without first loading the entire file into a single buffer.
 *     Each part is either mmap-ed or read in a fixed-size chunk
 *     (default: 5 MB, the S3 minimum part size).
 *
 * Expected improvement: **40-60% faster blob transfers** compared with the
 * baseline read-into-vector → write path.
 *
 * Thread-Safety: ZeroCopyBlobTransfer itself is stateless after construction
 * and therefore safe to share across threads.
 */
class ZeroCopyBlobTransfer {
public:
    explicit ZeroCopyBlobTransfer(
        const ZeroCopyTransferConfig& config = ZeroCopyTransferConfig{});

    ~ZeroCopyBlobTransfer() = default;

    // ── sendfile() ────────────────────────────────────────────────────────

    /**
     * @brief Transfer bytes from @p source_path to @p dest_fd via sendfile().
     *
     * On Linux the transfer is performed entirely in kernel space.  On other
     * platforms a portable read+write fallback is used automatically.
     *
     * @param source_path  Absolute path to the source blob file.
     * @param dest_fd      Open, writable file descriptor to receive the data.
     * @param offset       Byte offset in source_path to start from (0 = BOF).
     * @param length       Number of bytes to transfer (0 = full file).
     * @return Result<ZeroCopyTransferStats>
     *         ERR_STORAGE_FILE_NOT_FOUND   – source file missing
     *         ERR_UTIL_FILE_OPERATION_FAILED – sendfile/write syscall error
     */
    Result<ZeroCopyTransferStats> sendfileTransfer(
        const std::string& source_path,
        int                dest_fd,
        int64_t            offset = 0,
        int64_t            length = 0
    );

    // ── Memory-mapped reads ───────────────────────────────────────────────

    /**
     * @brief Open a read-only, memory-mapped view of @p file_path.
     *
     * The caller should check MmapBlobView::valid() before dereferencing
     * the data pointer.
     *
     * @param file_path  Absolute path to the blob file.
     * @return MmapBlobView
     */
    MmapBlobView openMmap(const std::string& file_path) const;

    // ── S3 multipart streaming upload ─────────────────────────────────────

    /**
     * @brief Upload @p source_path to S3 using multipart streaming.
     *
     * The file is read in parts of @c config.s3_multipart_part_size_bytes
     * (minimum 5 MB as required by S3).  Each part is uploaded independently;
     * the multipart upload is finalised once all parts succeed.
     *
     * Available only when THEMIS_HAS_AWS_SDK is defined.  Returns
     * ERR_UTIL_FILE_OPERATION_FAILED with a descriptive message otherwise.
     *
     * @param bucket      S3 bucket name.
     * @param s3_key      S3 object key (e.g. "prefix/blob_id.blob").
     * @param source_path Absolute path to the local blob file.
     * @param blob_id     Logical blob ID (stored in BlobRef.id).
     * @return Result<ZeroCopyTransferStats>
     */
    Result<ZeroCopyTransferStats> s3MultipartUpload(
        const std::string& bucket,
        const std::string& s3_key,
        const std::string& source_path,
        const std::string& blob_id
    );

    // ── Helpers ───────────────────────────────────────────────────────────

    /**
     * @return true when @p size_bytes is above the configured threshold and
     *         therefore qualifies for zero-copy treatment.
     */
    bool shouldUseZeroCopy(int64_t size_bytes) const noexcept {
        return size_bytes >= config_.zero_copy_threshold_bytes;
    }

    const ZeroCopyTransferConfig& config() const noexcept { return config_; }

private:
    ZeroCopyTransferConfig config_;

    /// Portable fallback used when sendfile() is not available.
    Result<ZeroCopyTransferStats> fallbackTransfer(
        const std::string& source_path,
        int                dest_fd,
        int64_t            offset,
        int64_t            length
    );
};

} // namespace storage
} // namespace themis
