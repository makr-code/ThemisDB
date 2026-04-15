/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            zero_copy_blob_transfer.cpp                        ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:10:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     565                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • c0a50c2caf  2026-03-19  Refactor build flags, IO, benchmarks & tests ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "storage/zero_copy_blob_transfer.h"
#include "utils/error_registry.h"
#include "utils/logger.h"

#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <stdexcept>

#if defined(_WIN32)
#  include <io.h>
#endif

// POSIX headers available on Linux and macOS
#if defined(__linux__) || defined(__APPLE__)
#  include <fcntl.h>
#  include <sys/mman.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

// sendfile(2) is Linux-specific
#if defined(__linux__)
#  include <sys/sendfile.h>
#endif

// S3 multipart upload (AWS SDK)
#if defined(THEMIS_HAS_AWS_SDK) && THEMIS_HAS_AWS_SDK && \
    __has_include(<aws/core/Aws.h>)
#  include <aws/core/Aws.h>
#  include <aws/s3/S3Client.h>
#  include <aws/s3/model/CreateMultipartUploadRequest.h>
#  include <aws/s3/model/UploadPartRequest.h>
#  include <aws/s3/model/CompleteMultipartUploadRequest.h>
#  include <aws/s3/model/AbortMultipartUploadRequest.h>
#  include <aws/s3/model/CompletedPart.h>
#  include <aws/s3/model/CompletedMultipartUpload.h>
#  define THEMIS_ZERO_COPY_S3_AVAILABLE 1
#else
#  define THEMIS_ZERO_COPY_S3_AVAILABLE 0
#endif

namespace themis {
namespace storage {

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Portable fd write helpers (mirrors wal_storage.cpp pattern)
// ─────────────────────────────────────────────────────────────────────────────

#if defined(_WIN32)
using themis_zc_ssize_t = std::ptrdiff_t;
static themis_zc_ssize_t themis_zc_write_fd(int fd, const void* data, size_t len) {
    return static_cast<themis_zc_ssize_t>(
        _write(fd, data, static_cast<unsigned int>(len)));
}
#elif defined(_POSIX_VERSION)
using themis_zc_ssize_t = ssize_t;
static themis_zc_ssize_t themis_zc_write_fd(int fd, const void* data, size_t len) {
    return ::write(fd, data, len);
}
#else
using themis_zc_ssize_t = std::ptrdiff_t;
static themis_zc_ssize_t themis_zc_write_fd(int /*fd*/, const void* /*data*/, size_t /*len*/) {
    return -1;  // unsupported platform
}
#endif

/// Write exactly @p len bytes, retrying on short writes.  Returns false on error.
static bool zc_write_all(int fd, const void* data, size_t len) {
    const uint8_t* ptr       = static_cast<const uint8_t*>(data);
    size_t         remaining = len;
    while (remaining > 0) {
        themis_zc_ssize_t written = themis_zc_write_fd(fd, ptr, remaining);
        if (written <= 0) {
            return false;
        }
        ptr       += static_cast<size_t>(written);
        remaining -= static_cast<size_t>(written);
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// AWS SDK one-time initialisation (Fix 3)
// ─────────────────────────────────────────────────────────────────────────────

#if THEMIS_ZERO_COPY_S3_AVAILABLE
static std::once_flag g_aws_sdk_init_flag;
static void ensureAwsSdkInitialized() {
    std::call_once(g_aws_sdk_init_flag, []() {
        Aws::SDKOptions options;
        options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Warn;
        Aws::InitAPI(options);
        THEMIS_INFO("ZeroCopyBlobTransfer: AWS SDK initialized");
    });
}
#endif

// ─────────────────────────────────────────────────────────────────────────────
// MmapBlobView
// ─────────────────────────────────────────────────────────────────────────────

MmapBlobView::MmapBlobView(const std::string& file_path, bool sequential_hint) {
#if defined(__linux__) || defined(__APPLE__)
    fd_ = ::open(file_path.c_str(), O_RDONLY);
    if (fd_ < 0) {
        THEMIS_WARN("MmapBlobView: cannot open '{}': {}", file_path, ::strerror(errno));
        return;
    }

    struct stat st{};
    if (::fstat(fd_, &st) != 0 || st.st_size == 0) {
        ::close(fd_);
        fd_ = -1;
        return;
    }

    size_ = static_cast<size_t>(st.st_size);

    void* ptr = ::mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (ptr == MAP_FAILED) {
        THEMIS_WARN("MmapBlobView: mmap failed for '{}': {}", file_path, ::strerror(errno));
        ::close(fd_);
        fd_   = -1;
        size_ = 0;
        return;
    }

    mapping_ = ptr;
    data_    = static_cast<uint8_t*>(ptr);

    // Optionally hint the kernel about our access pattern
    if (sequential_hint) {
        ::madvise(ptr, size_, MADV_SEQUENTIAL);
    }
#else
    // hint unused on non-POSIX platforms
    // Non-POSIX: fall back to reading the file into a heap buffer.
    // The "zero-copy" goal is not met, but correctness is preserved.
    try {
        std::ifstream ifs(file_path, std::ios::binary | std::ios::ate);
        if (!ifs) {
            THEMIS_WARN("MmapBlobView: cannot open '{}' for reading", file_path);
            return;
        }
        size_  = static_cast<size_t>(ifs.tellg());
        ifs.seekg(0);
        std::vector<uint8_t> buf(size_);
        ifs.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(size_));
        // Transfer ownership: store a heap-allocated copy as the "mapping"
        auto* heap_buf = new uint8_t[size_];
        std::memcpy(heap_buf, buf.data(), size_);
        mapping_ = heap_buf;
        data_    = heap_buf;
    } catch (const std::exception& e) {
        THEMIS_ERROR("MmapBlobView: fallback read failed for '{}': {}", file_path, e.what());
        mapping_ = nullptr;
        data_    = nullptr;
        size_    = 0;
    }
#endif
}

MmapBlobView::~MmapBlobView() {
#if defined(__linux__) || defined(__APPLE__)
    if (mapping_ != nullptr) {
        ::munmap(mapping_, size_);
        mapping_ = nullptr;
        data_    = nullptr;
    }
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
#else
    delete[] static_cast<uint8_t*>(mapping_);
    mapping_ = nullptr;
    data_    = nullptr;
#endif
}

MmapBlobView::MmapBlobView(MmapBlobView&& other) noexcept
    : mapping_(other.mapping_)
    , data_(other.data_)
    , size_(other.size_)
    , fd_(other.fd_)
{
    other.mapping_ = nullptr;
    other.data_    = nullptr;
    other.size_    = 0;
    other.fd_      = -1;
}

MmapBlobView& MmapBlobView::operator=(MmapBlobView&& other) noexcept {
    if (this != &other) {
        // Release current resources before taking ownership of other's resources
        this->~MmapBlobView();
        // Transfer ownership using member-by-member swap (safe after explicit destruction)
        mapping_       = other.mapping_;
        data_          = other.data_;
        size_          = other.size_;
        fd_            = other.fd_;
        other.mapping_ = nullptr;
        other.data_    = nullptr;
        other.size_    = 0;
        other.fd_      = -1;
    }
    return *this;
}

void MmapBlobView::forEach(
    size_t                                        chunk_size,
    std::function<bool(const uint8_t*, size_t)>  callback) const
{
    if (!valid() || chunk_size == 0) {
        return;
    }
    const uint8_t* ptr = data_;
    size_t         remaining = size_;
    while (remaining > 0) {
        size_t batch = std::min(remaining, chunk_size);
        if (!callback(ptr, batch)) {
            break;
        }
        ptr       += batch;
        remaining -= batch;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ZeroCopyBlobTransfer — constructor
// ─────────────────────────────────────────────────────────────────────────────

ZeroCopyBlobTransfer::ZeroCopyBlobTransfer(const ZeroCopyTransferConfig& config)
    : config_(config)
{
    // Clamp s3_multipart_part_size_bytes to the S3 lower bound
    if (config_.s3_multipart_part_size_bytes < S3_MULTIPART_MIN_PART_BYTES) {
        THEMIS_WARN("ZeroCopyBlobTransfer: s3_multipart_part_size_bytes {} < minimum {}; "
                    "clamping to minimum",
                    config_.s3_multipart_part_size_bytes,
                    S3_MULTIPART_MIN_PART_BYTES);
        config_.s3_multipart_part_size_bytes = S3_MULTIPART_MIN_PART_BYTES;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// sendfileTransfer
// ─────────────────────────────────────────────────────────────────────────────

Result<ZeroCopyTransferStats> ZeroCopyBlobTransfer::sendfileTransfer(
    const std::string& source_path,
    int                dest_fd,
    int64_t            offset,
    int64_t            length)
{
    auto t0 = std::chrono::steady_clock::now();

    if (!fs::exists(source_path)) {
        return Err<ZeroCopyTransferStats>(
            errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
            "sendfileTransfer: source not found: " + source_path);
    }

    int64_t file_size = static_cast<int64_t>(fs::file_size(source_path));
    if (length == 0) {
        length = file_size - offset;
    }
    if (length <= 0 || offset < 0 || offset >= file_size) {
        return Err<ZeroCopyTransferStats>(
            errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
            "sendfileTransfer: invalid offset/length for: " + source_path);
    }

#if defined(__linux__)
    // ── Linux: true zero-copy via sendfile(2) ────────────────────────────
    int src_fd = ::open(source_path.c_str(), O_RDONLY);
    if (src_fd < 0) {
        return Err<ZeroCopyTransferStats>(
            errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
            "sendfileTransfer: open failed for " + source_path +
                ": " + ::strerror(errno));
    }

    ZeroCopyTransferStats stats;
    stats.used_sendfile = true;

    off_t off  = static_cast<off_t>(offset);
    size_t remaining = static_cast<size_t>(length);

    while (remaining > 0) {
        ssize_t sent = ::sendfile(dest_fd, src_fd, &off, remaining);
        if (sent < 0) {
            ::close(src_fd);
            return Err<ZeroCopyTransferStats>(
                errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                std::string("sendfileTransfer: sendfile(2) error: ") +
                    ::strerror(errno));
        }
        if (sent == 0) {
            break;  // EOF
        }
        stats.bytes_transferred += sent;
        remaining               -= static_cast<size_t>(sent);
    }

    ::close(src_fd);

    auto t1 = std::chrono::steady_clock::now();
    stats.duration_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

    THEMIS_DEBUG("ZeroCopyBlobTransfer: sendfile {} bytes from '{}' ({}µs)",
                 stats.bytes_transferred, source_path, stats.duration_us);
    return Ok(std::move(stats));

#else
    // ── Non-Linux: portable fallback ─────────────────────────────────────
    return fallbackTransfer(source_path, dest_fd, offset, length);
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// fallbackTransfer  (portable read+write)
// ─────────────────────────────────────────────────────────────────────────────

Result<ZeroCopyTransferStats> ZeroCopyBlobTransfer::fallbackTransfer(
    const std::string& source_path,
    int                dest_fd,
    int64_t            offset,
    int64_t            length)
{
    auto t0 = std::chrono::steady_clock::now();

    std::ifstream ifs(source_path, std::ios::binary);
    if (!ifs) {
        return Err<ZeroCopyTransferStats>(
            errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
            "fallbackTransfer: cannot open: " + source_path);
    }

    ifs.seekg(offset);

    constexpr size_t BUF_SIZE = 256 * 1024; // 256 KB
    std::vector<char> buf(BUF_SIZE);

    ZeroCopyTransferStats stats;
    int64_t remaining = length;

    while (remaining > 0) {
        int64_t batch = std::min(remaining, static_cast<int64_t>(BUF_SIZE));
        ifs.read(buf.data(), batch);
        std::streamsize got = ifs.gcount();
        if (got <= 0) {
            break;
        }
        if (!zc_write_all(dest_fd, buf.data(), static_cast<size_t>(got))) {
            return Err<ZeroCopyTransferStats>(
                errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                "fallbackTransfer: write error writing to dest fd");
        }
        stats.bytes_transferred += got;
        remaining               -= got;
    }

    auto t1 = std::chrono::steady_clock::now();
    stats.duration_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

    THEMIS_DEBUG("ZeroCopyBlobTransfer: fallback wrote {} bytes from '{}' ({}µs)",
                 stats.bytes_transferred, source_path, stats.duration_us);
    return Ok(std::move(stats));
}

// ─────────────────────────────────────────────────────────────────────────────
// openMmap
// ─────────────────────────────────────────────────────────────────────────────

MmapBlobView ZeroCopyBlobTransfer::openMmap(const std::string& file_path) const {
    return MmapBlobView(file_path, config_.mmap_sequential_hint);
}

// ─────────────────────────────────────────────────────────────────────────────
// s3MultipartUpload
// ─────────────────────────────────────────────────────────────────────────────

Result<ZeroCopyTransferStats> ZeroCopyBlobTransfer::s3MultipartUpload(
    const std::string& bucket,
    const std::string& s3_key,
    const std::string& source_path,
    const std::string& blob_id)
{
#if THEMIS_ZERO_COPY_S3_AVAILABLE
    auto t0 = std::chrono::steady_clock::now();

    if (!fs::exists(source_path)) {
        return Err<ZeroCopyTransferStats>(
            errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
            "s3MultipartUpload: source not found: " + source_path);
    }

    const int64_t file_size  = static_cast<int64_t>(fs::file_size(source_path));
    const int64_t part_size  = config_.s3_multipart_part_size_bytes;

    // Ensure the AWS SDK is initialized exactly once per process
    ensureAwsSdkInitialized();

    // Use the default credential provider chain (env vars / ~/.aws / IAM role)
    Aws::Client::ClientConfiguration aws_config;
    Aws::S3::S3Client client(aws_config);

    // Step 1 – CreateMultipartUpload
    Aws::S3::Model::CreateMultipartUploadRequest create_req;
    create_req.SetBucket(bucket);
    create_req.SetKey(s3_key);
    create_req.SetServerSideEncryption(Aws::S3::Model::ServerSideEncryption::AES256);

    auto create_outcome = client.CreateMultipartUpload(create_req);
    if (!create_outcome.IsSuccess()) {
        return Err<ZeroCopyTransferStats>(
            errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
            "s3MultipartUpload: CreateMultipartUpload failed for " + blob_id +
                ": " + create_outcome.GetError().GetMessage());
    }

    const std::string upload_id = create_outcome.GetResult().GetUploadId();
    THEMIS_DEBUG("ZeroCopyBlobTransfer: S3 multipart upload started for blob '{}', "
                 "upload_id={}", blob_id, upload_id);

    // Step 2 – UploadPart (one part per part_size chunk)
    Aws::S3::Model::CompletedMultipartUpload completed_upload;
    ZeroCopyTransferStats stats;
    stats.used_s3_multipart = true;

    auto abort_upload = [&]() {
        Aws::S3::Model::AbortMultipartUploadRequest abort_req;
        abort_req.SetBucket(bucket);
        abort_req.SetKey(s3_key);
        abort_req.SetUploadId(upload_id);
        client.AbortMultipartUpload(abort_req);
        THEMIS_WARN("ZeroCopyBlobTransfer: aborted S3 multipart upload for blob '{}'", blob_id);
    };

    // Open the source file once; read it in streaming part-sized chunks
    std::ifstream ifs(source_path, std::ios::binary);
    if (!ifs) {
        abort_upload();
        return Err<ZeroCopyTransferStats>(
            errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
            "s3MultipartUpload: cannot open source: " + source_path);
    }

    int    part_number = 1;
    int64_t offset     = 0;

    while (offset < file_size) {
        int64_t this_part = std::min(part_size, file_size - offset);

        // Read this part into a buffer (one part at a time → bounded memory use)
        std::vector<char> part_buf(static_cast<size_t>(this_part));
        ifs.read(part_buf.data(), this_part);
        if (ifs.gcount() != this_part) {
            abort_upload();
            return Err<ZeroCopyTransferStats>(
                errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                "s3MultipartUpload: short read at offset " +
                    std::to_string(offset) + " for blob " + blob_id);
        }

        auto part_stream = Aws::MakeShared<Aws::StringStream>("UploadPart");
        part_stream->write(part_buf.data(), this_part);

        Aws::S3::Model::UploadPartRequest part_req;
        part_req.SetBucket(bucket);
        part_req.SetKey(s3_key);
        part_req.SetUploadId(upload_id);
        part_req.SetPartNumber(part_number);
        part_req.SetContentLength(this_part);
        part_req.SetBody(part_stream);

        auto part_outcome = client.UploadPart(part_req);
        if (!part_outcome.IsSuccess()) {
            abort_upload();
            return Err<ZeroCopyTransferStats>(
                errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                "s3MultipartUpload: UploadPart " + std::to_string(part_number) +
                    " failed for blob " + blob_id + ": " +
                    part_outcome.GetError().GetMessage());
        }

        Aws::S3::Model::CompletedPart completed_part;
        completed_part.SetPartNumber(part_number);
        completed_part.SetETag(part_outcome.GetResult().GetETag());
        completed_upload.AddParts(completed_part);

        stats.bytes_transferred += this_part;
        ++stats.s3_parts_uploaded;
        ++part_number;
        offset += this_part;

        THEMIS_DEBUG("ZeroCopyBlobTransfer: uploaded part {} ({} bytes) for blob '{}'",
                     part_number - 1, this_part, blob_id);
    }

    // Step 3 – CompleteMultipartUpload
    Aws::S3::Model::CompleteMultipartUploadRequest complete_req;
    complete_req.SetBucket(bucket);
    complete_req.SetKey(s3_key);
    complete_req.SetUploadId(upload_id);
    complete_req.SetMultipartUpload(completed_upload);

    auto complete_outcome = client.CompleteMultipartUpload(complete_req);
    if (!complete_outcome.IsSuccess()) {
        abort_upload();
        return Err<ZeroCopyTransferStats>(
            errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
            "s3MultipartUpload: CompleteMultipartUpload failed for blob " + blob_id +
                ": " + complete_outcome.GetError().GetMessage());
    }

    auto t1 = std::chrono::steady_clock::now();
    stats.duration_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

    THEMIS_INFO("ZeroCopyBlobTransfer: S3 multipart upload complete for blob '{}': "
                "{} parts, {} bytes, {}µs",
                blob_id, stats.s3_parts_uploaded, stats.bytes_transferred, stats.duration_us);

    return Ok(std::move(stats));

#else // !THEMIS_ZERO_COPY_S3_AVAILABLE
    return Err<ZeroCopyTransferStats>(
        errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
        "s3MultipartUpload: AWS SDK not available; "
        "rebuild with THEMIS_ENABLE_S3=ON");
#endif
}

} // namespace storage
} // namespace themis
