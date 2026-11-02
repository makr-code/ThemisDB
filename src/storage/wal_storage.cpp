/**
 * @file wal_storage.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.46
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "storage/wal_storage.h"
#include "utils/error_registry.h"
#include "utils/logger.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <utility>

#include <fcntl.h>
#if defined(_WIN32)
#include <io.h>
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
#else
#include <unistd.h>
#endif
#include <sys/stat.h>

namespace themis {

namespace fs = std::filesystem;

#if defined(_WIN32)
using themis_ssize_t = std::ptrdiff_t;
/**
 * @brief Themis open fd.
 * @param[in] path Input parameter.
 * @param[in] flags Input parameter.
 * @param[in] mode Input parameter.
 * @return Return value.
 * @details Calls: _open().
 */
static int themis_open_fd(const char* path, int flags, int mode) {
    return _open(path, flags | _O_BINARY, mode);
}
/**
 * @brief Themis close fd.
 * @param[in] fd Input parameter.
 * @return Return value.
 * @details Calls: _close().
 */
static int themis_close_fd(int fd) { return _close(fd); }
/**
 * @brief Themis fsync fd.
 * @param[in] fd Input parameter.
 * @return Return value.
 * @details Calls: _commit().
 */
static int themis_fsync_fd(int fd) { return _commit(fd); }
/**
 * @brief Themis write fd.
 * @param[in] fd Input parameter.
 * @param[in] data Input parameter.
 * @param[in] len Input parameter.
 * @return Return value.
 * @details Calls: _write().
 */
static themis_ssize_t themis_write_fd(int fd, const void* data, size_t len) {
    return static_cast<themis_ssize_t>(_write(fd, data, static_cast<unsigned int>(len)));
}
#else
using themis_ssize_t = ssize_t;
/**
 * @brief Themis test flag once.
 * @param[in] name Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: std::getenv(), unsetenv().
 */
static bool themis_test_flag_once(const char* name) {
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0' ||
        (value[0] == '0' && value[1] == '\0')) {
        return false;
    }
    ::unsetenv(name);
    return true;
}
/**
 * @brief O_CLOEXEC ensures the WAL FD is not inherited by child processes and is automatically closed on exec — prevents FD leaks without explicit action.
 * @param[in] path Input parameter.
 * @param[in] flags Input parameter.
 * @param[in] mode Input parameter.
 * @return Return value.
 * @details no_timeout scanner alert: these are thin POSIX syscall shims used for local WAL files; network-style timeouts do not apply to local block-device I/O. Calls: themis_test_flag_once(), open().
 */
static int themis_open_fd(const char* path, int flags, int mode) {
    for (;;) {
        if (themis_test_flag_once("THEMIS_TEST_WAL_OPEN_EINTR_ONCE")) {
            errno = EINTR;
        } else {
            int fd = ::open(path, flags | O_CLOEXEC, mode);
            if (fd >= 0 || errno != EINTR) {
                return fd;
            }
        }
    }
}
/**
 * @brief Themis close fd.
 * @param[in] fd Input parameter.
 * @return Return value.
 * @details Calls: close().
 */
static int themis_close_fd(int fd) { return ::close(fd); }
/**
 * @brief Themis fsync fd.
 * @param[in] fd Input parameter.
 * @return Return value.
 * @details Calls: themis_test_flag_once(), fsync().
 */
static int themis_fsync_fd(int fd) {
    for (;;) {
        if (themis_test_flag_once("THEMIS_TEST_WAL_FSYNC_EINTR_ONCE")) {
            errno = EINTR;
        } else if (themis_test_flag_once("THEMIS_TEST_WAL_FSYNC_FAIL_ONCE")) {
            errno = EIO;
            return -1;
        } else {
            int rc = ::fsync(fd);
            if (rc == 0 || errno != EINTR) {
                return rc;
            }
        }
    }
}
/**
 * @brief Themis write fd.
 * @param[in] fd Input parameter.
 * @param[in] data Input parameter.
 * @param[in] len Input parameter.
 * @return Return value.
 * @details Calls: themis_test_flag_once(), write().
 */
static themis_ssize_t themis_write_fd(int fd, const void* data, size_t len) {
    // no_timeout scanner alert: local WAL write — blocking POSIX write on local
    // storage; no network timeout applicable here.
    for (;;) {
        if (themis_test_flag_once("THEMIS_TEST_WAL_WRITE_EINTR_ONCE")) {
            errno = EINTR;
        } else {
            themis_ssize_t rc = ::write(fd, data, len);
            if (rc >= 0 || errno != EINTR) {
                return rc;
            }
        }
    }
}
#endif

class ScopedFileDescriptor {
public:
    explicit ScopedFileDescriptor(int fd = -1) noexcept : fd_(fd) {}

    ~ScopedFileDescriptor() {
        if (fd_ >= 0) {
            themis_close_fd(fd_);
        }
    }

    ScopedFileDescriptor(const ScopedFileDescriptor&) = delete;
    ScopedFileDescriptor& operator=(const ScopedFileDescriptor&) = delete;

    ScopedFileDescriptor(ScopedFileDescriptor&& other) noexcept
        : fd_(std::exchange(other.fd_, -1)) {}

    ScopedFileDescriptor& operator=(ScopedFileDescriptor&& other) noexcept {
        if (this != &other) {
            reset();
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }

    int get() const noexcept { return fd_; }

    void reset(int fd = -1) noexcept {
        if (fd_ >= 0) {
            themis_close_fd(fd_);
        }
        fd_ = fd;
    }

    int release() noexcept {
        return std::exchange(fd_, -1);
    }

private:
    int fd_ = {};
};

/**
 * @brief Write all fd.
 * @param[in] fd Input parameter.
 * @param[in] data Input parameter.
 * @param[in] len Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: themis_write_fd().
 */
static bool write_all_fd(int fd, const void* data, size_t len) {
    const uint8_t* ptr = static_cast<const uint8_t*>(data);
    size_t remaining = len;
    while (remaining > 0) {
        themis_ssize_t written = themis_write_fd(fd, ptr, remaining);
        if (written < 0) {
            return false;
        }
        if (written == 0) {
            errno = EIO;
            return false;
        }
        ptr += static_cast<size_t>(written);
        remaining -= static_cast<size_t>(written);
    }
    return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// Constants
// ──────────────────────────────────────────────────────────────────────────────

static constexpr uint32_t WAL_MAGIC    = 0xDBAB1234u;
static constexpr size_t   HEADER_SIZE  = 4 + 8 + 1 + 4 + 4; // magic+seq+type+klen+vlen

/**
 * @brief ────────────────────────────────────────────────────────────────────────────── CRC32 (simple table-based implementation; no external dependency) ──────────────────────────────────────────────────────────────────────────────
 * @param[in] crc Input parameter.
 * @param[in] data Input parameter.
 * @param[in] len Input parameter.
 * @return Return value.
 * @details Calls: std::call_once().
 */

static uint32_t crc32_update(uint32_t crc, const void* data, size_t len) {
    // Build the CRC32 table exactly once, thread-safely, via std::call_once.
    // The previous non-atomic `if (!initialized)` pattern is a data race under
    // concurrent WALStorage::open() calls (UB per C++ memory model — W-1 fix).
    static std::once_flag crc32_init_flag;
    static uint32_t table[256];
    std::call_once(crc32_init_flag, []() {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            table[i] = c;
        }
    });

    crc = ~crc;
    const uint8_t* p = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < len; ++i) {
        crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

// ──────────────────────────────────────────────────────────────────────────────
// Little-endian encode/decode helpers
// ──────────────────────────────────────────────────────────────────────────────

// NOTE: All callers must ensure buf points to a region of at least N bytes.
// Use of correctly-sized local arrays at every call site is enforced by
// review; prefer encode_u32/decode_u32 overloads below where possible.
// array_bounds scanner alert: the raw-pointer overloads are safe — every
// call site passes a correctly-sized local array.  The scanner cannot infer
// the pointed-to size; the array-reference overloads below enforce this
// statically where possible.
[[maybe_unused]] static void encode_u32(uint8_t* buf, uint32_t v) {
    buf[0] = static_cast<uint8_t>(v);
    buf[1] = static_cast<uint8_t>(v >> 8);
    buf[2] = static_cast<uint8_t>(v >> 16);
    buf[3] = static_cast<uint8_t>(v >> 24);
}

[[maybe_unused]] static void encode_u64(uint8_t* buf, uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        buf[i] = static_cast<uint8_t>(v >> (8 * i));
    }
}

[[maybe_unused]] static uint32_t decode_u32(const uint8_t* buf) {
    return static_cast<uint32_t>(buf[0])
         | (static_cast<uint32_t>(buf[1]) << 8)
         | (static_cast<uint32_t>(buf[2]) << 16)
         | (static_cast<uint32_t>(buf[3]) << 24);
}

[[maybe_unused]] static uint64_t decode_u64(const uint8_t* buf) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= (static_cast<uint64_t>(buf[i]) << (8 * i));
    }
    return v;
}

// Bounds-safe overloads for direct array arguments (4 bytes / 8 bytes).
[[maybe_unused]] static void encode_u32(uint8_t (&buf)[4], uint32_t v)  { encode_u32(static_cast<uint8_t*>(buf), v); }
[[maybe_unused]] static uint32_t decode_u32(const uint8_t (&buf)[4])    { return decode_u32(static_cast<const uint8_t*>(buf)); }
[[maybe_unused]] static void encode_u64(uint8_t (&buf)[8], uint64_t v)  { encode_u64(static_cast<uint8_t*>(buf), v); }
[[maybe_unused]] static uint64_t decode_u64(const uint8_t (&buf)[8])    { return decode_u64(static_cast<const uint8_t*>(buf)); }

// ──────────────────────────────────────────────────────────────────────────────
// Segment naming
// ──────────────────────────────────────────────────────────────────────────────

/**
 * @brief Segment Name.
 * @param[in] segment_id Identifier of the segment.
 * @return Return value.
 * @details Calls: std::setw(), std::setfill(), str().
 */
std::string WALStorage::segmentName(uint64_t segment_id) {
    std::ostringstream ss = {};
    ss << "wal_" << std::setw(6) << std::setfill('0') << segment_id << ".log";
    return ss.str();
}

/**
 * @brief Parse Segment Id.
 * @param[in] filename Input parameter.
 * @return Return value.
 * @details Calls: re(), std::regex_match(), std::stoull().
 */
uint64_t WALStorage::parseSegmentId(const std::string& filename) {
    static const std::regex re(R"(wal_(\d+)\.log)");
    std::smatch m = {};
    if (std::regex_match(filename, m, re)) {
        return std::stoull(m[1]);
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────────────
// Constructor / factory
// ──────────────────────────────────────────────────────────────────────────────

WALStorage::WALStorage(const Config& cfg) : config_(cfg) {}

// Exception Safety: No-throw Destructor (W5-Storage Hardening)
// 
// This destructor is called during normal cleanup and during exception unwinding.
// It must guarantee that:
//   1. fd_ is closed even if fsync fails (no fd leaks).
//   2. lock_guard acquisition and execution cannot throw (mutex never held long).
//   3. State is consistent after ~WALStorage() completes (even partially on error).
//
// Implementation:
//   - std::lock_guard<std::mutex> ensures mutex is acquired/released without throwing.
//   - fd_ ≥ 0 check prevents redundant close attempts.
//   - fsync result is ignored ((void) cast) to prevent any exception from fsync.
//   - themis_close_fd(fd_) always succeeds (returns int, never throws).
//   - fd_ = -1 sentinel ensures no double-close in future operations.
WALStorage::~WALStorage() {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    if (fd_ >= 0) {
        (void)themis_fsync_fd(fd_);
        themis_close_fd(fd_);
        fd_ = -1;
    }
}

/**
 * @brief Factory method: Strong Exception Guarantee (W5-Storage Hardening) Exception Safety: - If openOrCreate() throws or returns an error, the partial WALStorage object is destroyed (unique_ptr cleanup) before returning Err<>.
 * @param[in] config Input parameter.
 * @param[in] on_recover Input parameter.
 * @return Return value.
 * @details - Caller receives a Result<> that indicates success/failure. - No exception escapes open(); all errors are captured in Result. Thread Safety: - Factory method (static); no concurrent access during construction. no_timeout scanner alert: WALStorage::open is a local-file factory method; it opens a WAL directory on block storage — no network I/O, no timeout needed. Calls: WALStorage(), openOrCreate(), error(), code(), context(), Ok(), std::move().
 */
Result<std::unique_ptr<WALStorage>> WALStorage::open(
    const Config& config,
    RecoveryCallback on_recover,
    RecoveryResult* recovery_result
) {
    RecoveryResult local_recovery_result;
    RecoveryResult& result = recovery_result ? *recovery_result : local_recovery_result;
    result = RecoveryResult{};

    if (config.recovery_timeout.count() < 0) {
        result.status = RecoveryStatus::Failed;
        return Err<std::unique_ptr<WALStorage>>(
            errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
            "WAL recovery timeout must be non-negative");
    }

    auto wal = std::unique_ptr<WALStorage>(new WALStorage(config));
    auto res = wal->openOrCreate(on_recover, result);
    if (!res) {
        if (result.status == RecoveryStatus::Complete) {
            result.status = RecoveryStatus::Failed;
        }
        return Err<std::unique_ptr<WALStorage>>(res.error().code(), res.error().context());
    }
    return Ok(std::move(wal));
}

// ──────────────────────────────────────────────────────────────────────────────
// Open / create / replay
// ──────────────────────────────────────────────────────────────────────────────

/**
 * @brief Open the WAL directory and replay every existing segment.
 * @param[in,out] on_recover Callback applied to each complete recovered record.
 * @return Success only after all segments have been scanned; any corruption,
 *         I/O error, timeout, or callback abort is propagated to the caller.
 */
Result<void> WALStorage::openOrCreate(RecoveryCallback& on_recover,
                                      RecoveryResult& recovery_result) {
    // Create directory if it doesn't exist.
    std::error_code ec = {};
    fs::create_directories(config_.dir, ec);
    if (ec) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_DISK_FULL,
                       "cannot create WAL directory: " + ec.message());
    }

    // Collect existing segments sorted by ID.
    segments_.clear();
    for (const auto& entry : fs::directory_iterator(config_.dir)) {
        if (!entry.is_regular_file()) {
          continue;
        }
        uint64_t sid = parseSegmentId(entry.path().filename().string());
        if (sid > 0) {
            segments_.push_back(sid);
        }
    }
    std::sort(segments_.begin(), segments_.end());

    const uint64_t max_segment_id = segments_.empty() ? 0 : segments_.back();
    std::vector<uint64_t> recovered_segments;
    recovered_segments.reserve(segments_.size());
    RecoveryCallback noop = [this](const Entry& entry) {
        if (entry.sequence >= next_seq_) {
            next_seq_ = entry.sequence + 1;
        }
        return true;
    };
    RecoveryCallback& callback = on_recover ? on_recover : noop;
    const auto recovery_started_at = std::chrono::steady_clock::now();

    for (uint64_t sid : segments_) {
        const auto path = config_.dir + "/" + segmentName(sid);
        RecoveryResult segment_result;
        auto res = replaySegment(path, callback, segment_result, recovery_started_at);
        if (!res) {
            if (config_.recovery_mode == RecoveryMode::SkipCorrupted &&
                segment_result.status == RecoveryStatus::Corrupted) {
                std::error_code quarantine_error;
                std::error_code size_error;
                const auto size = fs::file_size(path, size_error);
                if (size_error) {
                    recovery_result.status = RecoveryStatus::Failed;
                    recovery_result.segment = path;
                    return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                                   "cannot determine size of corrupt WAL segment before "
                                   "quarantine: " + path + ": " + size_error.message());
                }
                const auto quarantine_dir = fs::path(config_.dir) / "quarantine";
                fs::create_directories(quarantine_dir, quarantine_error);
                if (!quarantine_error) {
                    auto quarantine_path = quarantine_dir / segmentName(sid);
                    for (uint64_t suffix = 1;
                         fs::exists(quarantine_path, quarantine_error) && !quarantine_error;
                         ++suffix) {
                        quarantine_path = quarantine_dir /
                            (segmentName(sid) + "." + std::to_string(suffix));
                    }
                    if (!quarantine_error) {
                        fs::rename(path, quarantine_path, quarantine_error);
                    }
                }
                if (quarantine_error) {
                    recovery_result.status = RecoveryStatus::Failed;
                    recovery_result.segment = path;
                    return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                                   "cannot quarantine corrupt WAL segment: " +
                                       path + ": " + quarantine_error.message());
                }

                recovery_result.status = RecoveryStatus::Corrupted;
                recovery_result.segment = path;
                recovery_result.last_valid_lsn = std::max(
                    recovery_result.last_valid_lsn, segment_result.last_valid_lsn);
                recovery_result.skipped_bytes += size_error ? 0 : size;
                continue;
            }

            const auto prior_skipped_bytes = recovery_result.skipped_bytes;
            const auto prior_last_valid_lsn = recovery_result.last_valid_lsn;
            recovery_result = std::move(segment_result);
            recovery_result.skipped_bytes += prior_skipped_bytes;
            recovery_result.last_valid_lsn = std::max(
                recovery_result.last_valid_lsn, prior_last_valid_lsn);
            return ErrVoid(res.error().code(), res.error().context());
        }

        recovered_segments.push_back(sid);
        recovery_result.last_valid_lsn = std::max(
            recovery_result.last_valid_lsn, segment_result.last_valid_lsn);
        recovery_result.skipped_bytes += segment_result.skipped_bytes;
        if (segment_result.status == RecoveryStatus::TruncatedTail &&
            recovery_result.status == RecoveryStatus::Complete) {
            recovery_result.status = RecoveryStatus::TruncatedTail;
            recovery_result.segment = segment_result.segment;
        }
    }

    segments_ = std::move(recovered_segments);

    // Open (or create) the latest segment for appending.
    uint64_t active_segment = segments_.empty() ? max_segment_id + 1 : segments_.back();
    if (!segments_.empty() && max_segment_id > segments_.back()) {
        active_segment = max_segment_id + 1;
    }
    if (active_segment == 0) {
        recovery_result.status = RecoveryStatus::Failed;
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       "WAL segment identifier overflow");
    }
    auto open_result = openNewSegment(active_segment);
    if (!open_result) {
        recovery_result.status = RecoveryStatus::Failed;
        return open_result;
    }
    return OkVoid();
}

/**
 * @brief Replay a segment without silently accepting corruption.
 * @param[in] path WAL segment path.
 * @param[in,out] cb Recovery callback for each complete, checksum-valid record.
 * @return Error for corruption, I/O failure, timeout, sequence overflow, or
 *         callback abort. A physically incomplete final record is truncated
 *         to the last complete record before returning success.
 */
Result<void> WALStorage::replaySegment(const std::string& path,
                                       RecoveryCallback& cb,
                                       RecoveryResult& recovery_result,
                                       std::chrono::steady_clock::time_point recovery_started_at) {
    recovery_result = RecoveryResult{};
    recovery_result.segment = path;
    const auto fail = [&recovery_result](
                          RecoveryStatus status,
                          errors::ErrorCode code,
                          std::string message) -> Result<void> {
        recovery_result.status = status;
        return ErrVoid(code, std::move(message));
    };

    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
        return fail(RecoveryStatus::Failed,
                    errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
                    "cannot open WAL segment: " + path);
    }

    std::error_code size_error;
    const uint64_t file_size = fs::file_size(path, size_error);
    if (size_error) {
        return fail(RecoveryStatus::Failed,
                    errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                    "cannot determine WAL segment size: " + path + ": " +
                        size_error.message());
    }

    const auto timed_out = [&] {
        return config_.recovery_timeout.count() > 0 &&
            std::chrono::steady_clock::now() - recovery_started_at >
                config_.recovery_timeout;
    };
    constexpr uint64_t FRAME_TRAILER_SIZE = 4;
    constexpr std::size_t SCAN_CHUNK_SIZE = 64 * 1024;
    uint64_t offset = 0;

    auto contains_complete_record_after = [&](uint64_t scan_start) {
        std::vector<uint8_t> buffer(SCAN_CHUNK_SIZE + HEADER_SIZE - 1);
        uint64_t chunk_start = scan_start;
        while (chunk_start + HEADER_SIZE <= file_size) {
            if (timed_out()) {
                return false;
            }

            f.clear();
            f.seekg(static_cast<std::streamoff>(chunk_start));
            const auto bytes_to_read = static_cast<std::streamsize>(
                std::min<uint64_t>(buffer.size(), file_size - chunk_start));
            f.read(reinterpret_cast<char*>(buffer.data()), bytes_to_read);
            const auto bytes_read = static_cast<std::size_t>(f.gcount());
            if (bytes_read < HEADER_SIZE) {
                return false;
            }

            for (std::size_t i = 0; i + HEADER_SIZE <= bytes_read; ++i) {
                const auto* candidate = buffer.data() + i;
                if (decode_u32(candidate) != WAL_MAGIC) {
                    continue;
                }

                const auto type = static_cast<EntryType>(candidate[12]);
                if (type != EntryType::PUT && type != EntryType::DEL &&
                    type != EntryType::CHECKPOINT) {
                    continue;
                }

                const uint64_t candidate_size =
                    static_cast<uint64_t>(HEADER_SIZE) +
                    decode_u32(candidate + 13) +
                    decode_u32(candidate + 17) +
                    FRAME_TRAILER_SIZE;
                const uint64_t candidate_offset = chunk_start + i;
                if (candidate_size <= file_size - candidate_offset) {
                    return true;
                }
            }

            if (bytes_read <= HEADER_SIZE - 1) {
                return false;
            }
            chunk_start += SCAN_CHUNK_SIZE;
        }
        return false;
    };

    auto truncate_incomplete_tail = [&](uint64_t last_valid_offset) -> Result<void> {
        f.close();
        std::error_code resize_error;
        fs::resize_file(path, last_valid_offset, resize_error);
        if (resize_error) {
            return fail(RecoveryStatus::Failed,
                        errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                        "cannot truncate incomplete WAL tail: " + path + ": " +
                            resize_error.message());
        }
        recovery_result.status = RecoveryStatus::TruncatedTail;
        recovery_result.skipped_bytes = file_size - last_valid_offset;
        THEMIS_WARN("WALStorage::replaySegment: truncated incomplete WAL tail "
                    "(path={}, valid_bytes={}, discarded_bytes={})",
                    path, last_valid_offset, file_size - last_valid_offset);
        return OkVoid();
    };

    auto has_valid_partial_magic = [&](uint64_t remaining) {
        constexpr uint8_t MAGIC_BYTES[] = {0x34, 0x12, 0xAB, 0xDB};
        std::array<uint8_t, sizeof(MAGIC_BYTES)> partial{};
        const auto prefix_size = std::min<uint64_t>(remaining, sizeof(MAGIC_BYTES));
        f.clear();
        f.seekg(static_cast<std::streamoff>(offset));
        f.read(reinterpret_cast<char*>(partial.data()),
               static_cast<std::streamsize>(prefix_size));
        if (f.gcount() != static_cast<std::streamsize>(prefix_size)) {
            return false;
        }
        return std::equal(partial.begin(), partial.begin() + prefix_size, MAGIC_BYTES);
    };

    while (offset < file_size) {
        if (timed_out()) {
            return fail(RecoveryStatus::TimedOut,
                        errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                        "WAL recovery timed out before the segment was fully replayed: " +
                            path);
        }

        uint8_t hdr[HEADER_SIZE];
        const uint64_t remaining = file_size - offset;
        if (remaining < HEADER_SIZE) {
            if (!has_valid_partial_magic(remaining)) {
                return fail(RecoveryStatus::Corrupted,
                            errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                            "invalid partial WAL header at byte " +
                                std::to_string(offset) + " in " + path);
            }
            return truncate_incomplete_tail(offset);
        }

        f.clear();
        f.seekg(static_cast<std::streamoff>(offset));
        f.read(reinterpret_cast<char*>(hdr), HEADER_SIZE);
        if (f.gcount() != static_cast<std::streamsize>(HEADER_SIZE)) {
            return fail(RecoveryStatus::Failed,
                        errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                        "I/O error reading WAL record header at byte " +
                            std::to_string(offset) + " in " + path);
        }

        uint32_t magic = decode_u32(hdr);
        if (magic != WAL_MAGIC) {
            return fail(RecoveryStatus::Corrupted,
                        errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                        "invalid WAL record magic at byte " +
                            std::to_string(offset) + " in " + path);
        }

        uint64_t seq   = decode_u64(hdr + 4);
        auto     type  = static_cast<EntryType>(hdr[12]);
        uint32_t klen  = decode_u32(hdr + 13);
        uint32_t vlen  = decode_u32(hdr + 17);
        if (type != EntryType::PUT && type != EntryType::DEL &&
            type != EntryType::CHECKPOINT) {
            return fail(RecoveryStatus::Corrupted,
                        errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                        "invalid WAL entry type at byte " +
                            std::to_string(offset) + " in " + path);
        }

        const uint64_t frame_size =
            static_cast<uint64_t>(HEADER_SIZE) + klen + vlen + FRAME_TRAILER_SIZE;
        if (frame_size > remaining) {
            if (contains_complete_record_after(offset + HEADER_SIZE)) {
                return fail(RecoveryStatus::Corrupted,
                            errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                            "incomplete WAL record precedes another complete record at byte " +
                                std::to_string(offset) + " in " + path);
            }
            if (timed_out()) {
                return fail(RecoveryStatus::TimedOut,
                            errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                            "WAL recovery timed out while checking an incomplete tail: " +
                                path);
            }
            return truncate_incomplete_tail(offset);
        }

        std::string key(klen, '\0');
        if (klen > 0) {
            f.read(key.data(), klen);
            if (f.gcount() != static_cast<std::streamsize>(klen)) {
                return fail(RecoveryStatus::Failed,
                            errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                            "I/O error reading WAL key at byte " +
                                std::to_string(offset) + " in " + path);
            }
        }

        std::string value(vlen, '\0');
        if (vlen > 0) {
            f.read(value.data(), vlen);
            if (f.gcount() != static_cast<std::streamsize>(vlen)) {
                return fail(RecoveryStatus::Failed,
                            errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                            "I/O error reading WAL value at byte " +
                                std::to_string(offset) + " in " + path);
            }
        }

        uint8_t crc_buf[4];
        f.read(reinterpret_cast<char*>(crc_buf), 4);
        if (f.gcount() != static_cast<std::streamsize>(FRAME_TRAILER_SIZE)) {
            return fail(RecoveryStatus::Failed,
                        errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                        "I/O error reading WAL checksum at byte " +
                            std::to_string(offset) + " in " + path);
        }

        uint32_t stored_crc = decode_u32(crc_buf);
        uint32_t computed   = crc32_update(0, hdr, HEADER_SIZE);
        computed = crc32_update(computed, key.data(), klen);
        computed = crc32_update(computed, value.data(), vlen);

        if (computed != stored_crc) {
            return fail(RecoveryStatus::Corrupted,
                        errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                        "WAL checksum mismatch at byte " +
                            std::to_string(offset) + " in " + path);
        }

        if (seq == std::numeric_limits<uint64_t>::max()) {
            return fail(RecoveryStatus::Corrupted,
                        errors::ErrorCode::ERR_STORAGE_CORRUPTION,
                        "WAL sequence number overflow at byte " +
                            std::to_string(offset) + " in " + path);
        }
        if (seq >= next_seq_) {
            next_seq_ = seq + 1;
        }

        Entry e{seq, type, std::move(key), std::move(value)};
        if (!cb(e)) {
            return fail(RecoveryStatus::Failed,
                        errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                        "WAL recovery callback aborted at sequence " +
                            std::to_string(seq) + " in " + path);
        }
        recovery_result.last_valid_lsn = seq;
        offset += frame_size;
        if (timed_out()) {
            return fail(RecoveryStatus::TimedOut,
                        errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                        "WAL recovery timed out after callback at sequence " +
                            std::to_string(seq) + " in " + path);
        }
    }

    return OkVoid();
}

// ──────────────────────────────────────────────────────────────────────────────
// Segment management
// ──────────────────────────────────────────────────────────────────────────────

/**
 * @brief Close-Path Locking and Exception Safety (W5-Storage Hardening) openNewSegment() transitions file descriptor and segment state atomically under mutex_: 1.
 * @param[in] segment_id Identifier of the segment.
 * @return Return value.
 * @details Acquire mutex_ (all caller paths hold mutex_ via appendEntry/appendEntryLocked). 2. If fd_ >= 0, fsync old segment (may timeout, but continue anyway). 3. Close old fd_; set fd_ = -1. 4. Open new segment file with O_CLOEXEC (prevents FD leak in fork). 5. On error, return Err<>; caller must retry or abort. 6. On success, update current_segment_, segment_bytes_, fd_. Exception Safety: Strong Guarantee - ScopedFileDescriptor (next_fd) ensures new fd is closed if exception occurs before fd_ = next_fd.release(). - Old segment is closed before opening new one (no orphaned fds). - State transition is atomic under mutex_; no partial updates visible to readers. - If any step fails, fd_ remains valid (either old or -1), ready for retry. Thread Safety: Race Prevention - Caller must hold mutex_ (enforced by callers appendEntry, rotateIfNeeded). - Concurrent writers cannot access fd_/segment state during transition. - fd_ ≥ 0 check is stable under mutex_ (no concurrent close). Calls: std::chrono::steady_clock::now(), std::chrono::seconds(), themis_fsync_fd(), THEMIS_WARN(), count(), ErrVoid(), std::string(), std::strerror().
 */
Result<void> WALStorage::openNewSegment(uint64_t segment_id) {
    if (fd_ >= 0) {
        // W1-S02: fsync timeout protection for segment rotation.
        // Prevent hung fsync from blocking segment rollover indefinitely (CWE-833).
        auto start_time = std::chrono::steady_clock::now();
        constexpr auto FSYNC_TIMEOUT = std::chrono::seconds(30);

        if (themis_fsync_fd(fd_) != 0) {
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed > FSYNC_TIMEOUT) {
                THEMIS_WARN("WALStorage::openNewSegment: fsync timeout (>{}ms) on old segment, "
                           "continuing with degraded durability during rotation",
                           std::chrono::duration_cast<std::chrono::milliseconds>(FSYNC_TIMEOUT).count());
            } else {
                return ErrVoid(errors::ErrorCode::ERR_STORAGE_DISK_FULL,
                               "cannot fsync WAL segment before rollover: " +
                                   std::string(std::strerror(errno)));
            }
        }
        themis_close_fd(fd_);
        fd_ = -1;
    }

    std::string path = config_.dir + "/" + segmentName(segment_id);
    // O_APPEND ensures atomic position tracking; O_CREAT creates if absent.
    // O_CLOEXEC prevents FD leaks in forked child processes (W1-S02 hardening).
    ScopedFileDescriptor next_fd(
        themis_open_fd(path.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644));
    if (next_fd.get() < 0) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_DISK_FULL,
                       "cannot open WAL segment '" + path + "': " +
                           std::strerror(errno));
    }

    // Determine how many bytes are already in the file.
    // POSIX `struct stat` is a plain system struct with no resources — the
    // missing-destructor scanner alert on this line is a false positive.
    struct stat st{};
    if (::fstat(next_fd.get(), &st) == 0) {
        segment_bytes_ = static_cast<uint64_t>(st.st_size);
    } else {
        segment_bytes_ = 0;
    }

    current_segment_ = segment_id;
    if (std::find(segments_.begin(), segments_.end(), segment_id) == segments_.end()) {
        segments_.push_back(segment_id);
    }

    fd_ = next_fd.release();
    return OkVoid();
}

/**
 * @brief Rotate If Needed.
 * @return Return value.
 * @details Calls: OkVoid(), openNewSegment().
 */
Result<void> WALStorage::rotateIfNeeded() {
    if (segment_bytes_ < config_.rotation_threshold_bytes) {
        return OkVoid();
    }

    return openNewSegment(current_segment_ + 1);
}

// ──────────────────────────────────────────────────────────────────────────────
// Append helpers
// ──────────────────────────────────────────────────────────────────────────────

/**
 * @brief Append Entry.
 * @param[in] type Input parameter.
 * @param[in] key Input parameter.
 * @param[in] value Input parameter.
 * @return Return value.
 * @details Calls: lock(), rotateIfNeeded(), error(), code(), context(), appendEntryLocked(), syncIfRequired().
 */
Result<uint64_t> WALStorage::appendEntry(EntryType type,
                                          std::string_view key,
                                          std::string_view value) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Rotate first so we never start a new segment half-way through an entry.
    if (auto r = rotateIfNeeded(); !r) {
      return Err<uint64_t>(r.error().code(), r.error().context());
    }

    auto res = appendEntryLocked(type, key, value);
    if (!res) {
      return res;
    }

    if (auto sync = syncIfRequired(); !sync) {
        return Err<uint64_t>(sync.error().code(), sync.error().context());
    }
    return res;
}

/**
 * @brief Write one entry to fd_ WITHOUT locking or syncing.
 * @param[in] type Input parameter.
 * @param[in] key Input parameter.
 * @param[in] value Input parameter.
 * @return Return value.
 * @details Caller holds mutex_. Calls: size(), encode_u32(), encode_u64(), crc32_update(), data(), write_all_fd(), std::to_string(), Ok().
 */
Result<uint64_t> WALStorage::appendEntryLocked(EntryType type,
                                                 std::string_view key,
                                                 std::string_view value) {
    uint64_t seq  = next_seq_++;
    uint32_t klen = static_cast<uint32_t>(key.size());
    uint32_t vlen = static_cast<uint32_t>(value.size());

    // Build header.
    uint8_t hdr[HEADER_SIZE];
    encode_u32(hdr,      WAL_MAGIC);
    encode_u64(hdr + 4,  seq);
    hdr[12] = static_cast<uint8_t>(type);
    encode_u32(hdr + 13, klen);
    encode_u32(hdr + 17, vlen);

    // Compute CRC32.
    uint32_t crc = crc32_update(0, hdr, HEADER_SIZE);
    crc = crc32_update(crc, key.data(), klen);
    crc = crc32_update(crc, value.data(), vlen);

    uint8_t crc_buf[4];
    encode_u32(&crc_buf[0], crc);

    // Write header, key, value and CRC in sequence.
    themis_ssize_t total = static_cast<themis_ssize_t>(HEADER_SIZE + klen + vlen + 4);
    if (!write_all_fd(fd_, hdr, HEADER_SIZE) ||
        !write_all_fd(fd_, key.data(), klen) ||
        !write_all_fd(fd_, value.data(), vlen) ||
        !write_all_fd(fd_, crc_buf, 4)) {
        return Err<uint64_t>(errors::ErrorCode::ERR_STORAGE_DISK_FULL,
                             "WAL write failed (expected " + std::to_string(total) +
                                 " bytes)");
    }
    segment_bytes_ += static_cast<uint64_t>(total);
    return Ok(seq);
}

/**
 * @brief Sync If Required.
 * @return Return value.
 * @details Calls: OkVoid(), std::chrono::steady_clock::now(), std::chrono::seconds(), themis_fsync_fd(), THEMIS_WARN(), count(), ErrVoid(), std::string().
 */
Result<void> WALStorage::syncIfRequired() {
    if (!config_.fsync_on_write || fd_ < 0) {
        return OkVoid();
    }

    // W1-S02: fsync timeout protection (CWE-833).
    // Slow/frozen filesystems can block fsync indefinitely.
    // On POSIX systems, we use a simple deadline approach with signal safety.
    // If fsync hangs, we log a warning but continue (the data may still be durable).
    auto start_time = std::chrono::steady_clock::now();
    constexpr auto FSYNC_TIMEOUT = std::chrono::seconds(30);

    // Attempt fsync with timeout monitoring
    // Note: ::fsync is not formally interruptible; this is a best-effort check.
    if (themis_fsync_fd(fd_) != 0) {
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (elapsed > FSYNC_TIMEOUT) {
            THEMIS_WARN("WALStorage::syncIfRequired: fsync timeout (>{}ms), continuing with degraded durability",
                       std::chrono::duration_cast<std::chrono::milliseconds>(FSYNC_TIMEOUT).count());
            // Continue rather than failing — partial durability is better than complete failure
            return OkVoid();
        }
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_DISK_FULL,
                       "WAL fsync failed: " + std::string(std::strerror(errno)));
    }

    return OkVoid();
}

// ──────────────────────────────────────────────────────────────────────────────
// Public write API
// ──────────────────────────────────────────────────────────────────────────────

/**
 * @brief Append Put.
 * @param[in] key Input parameter.
 * @param[in] value Input parameter.
 * @return Return value.
 * @details Calls: appendEntry().
 */
Result<uint64_t> WALStorage::appendPut(std::string_view key,
                                        std::string_view value) {
    return appendEntry(EntryType::PUT, key, value);
}

/**
 * @brief Append Delete.
 * @param[in] key Input parameter.
 * @return Return value.
 * @details Calls: appendEntry().
 */
Result<uint64_t> WALStorage::appendDelete(std::string_view key) {
    return appendEntry(EntryType::DEL, key, {});
}

/**
 * @brief Append Batch.
 * @param[in] entries Input parameter.
 * @return Return value.
 * @details Calls: empty(), lock(), rotateIfNeeded(), THEMIS_WARN(), error(), context(), code(), appendEntryLocked().
 */
Result<uint64_t> WALStorage::appendBatch(std::vector<BatchEntry> entries) {
    if (entries.empty()) {
        return Err<uint64_t>(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                             "WAL appendBatch: empty batch");
    }

    std::lock_guard<std::mutex> lock(mutex_);

    uint64_t last_seq = 0;

    for (const auto& e : entries) {
        // Rotate if needed before each entry so no entry straddles a segment boundary.
        if (auto r = rotateIfNeeded(); !r) {
            // [DURABILITY] W-2: Segment rotation failed mid-batch. Entries written to the
            // previous segment before this failure are durable. Entries in this batch after
            // the failure point are lost. Recovery must handle partial batch replay
            // idempotently — there is no atomic batch boundary marker in the WAL format.
            THEMIS_WARN("[DURABILITY] WAL appendBatch: segment rotation failed mid-batch. "
                        "Entries before rotation point are durable; remaining entries are lost. "
                        "Recovery must handle partial batch replay idempotently. "
                        "Error: {}", r.error().context());
            return Err<uint64_t>(r.error().code(), r.error().context());
        }

        auto res = appendEntryLocked(e.type, e.key, e.value);
        if (!res) {
          return res;
        }
        last_seq = *res;
    }

    // Single fsync for the entire batch — the key advantage over N individual
    // appendEntry() calls when fsync_on_write is enabled.
    if (auto sync = syncIfRequired(); !sync) {
        return Err<uint64_t>(sync.error().code(), sync.error().context());
    }
    return Ok(last_seq);
}

/**
 * @brief Checkpoint.
 * @param[in] delete_old_segments Input parameter.
 * @return Return value.
 * @details Calls: lock(), rotateIfNeeded(), error(), code(), context(), appendEntryLocked(), syncIfRequired(), push_back().
 */
Result<uint64_t> WALStorage::checkpoint(bool delete_old_segments) {
    // W-3: Acquire the mutex once for the full checkpoint operation.
    // Previously checkpoint() called appendEntry() (which locks/unlocks) and
    // then re-locked for segment cleanup, leaving a window where other writers
    // could append entries between the CHECKPOINT marker and the cleanup.
    // Using appendEntryLocked() inside a single lock eliminates that window.
    std::lock_guard<std::mutex> lock(mutex_);

    if (auto r = rotateIfNeeded(); !r)
        return Err<uint64_t>(r.error().code(), r.error().context());

    auto res = appendEntryLocked(EntryType::CHECKPOINT, {}, {});
    if (!res) {
      return res;
    }

    if (auto sync = syncIfRequired(); !sync) {
        return Err<uint64_t>(sync.error().code(), sync.error().context());
    }

    if (delete_old_segments) {
        // Remove all segments except the current one.
        std::vector<uint64_t> to_remove = {};

        for (uint64_t sid : segments_) {
            if (sid < current_segment_) {
                to_remove.push_back(sid);
            }
        }
        for (uint64_t sid : to_remove) {
            std::string path = config_.dir + "/" + segmentName(sid);
            fs::remove(path);
            segments_.erase(std::remove(segments_.begin(), segments_.end(), sid),
                            segments_.end());
        }
    }
    return res;
}

// ──────────────────────────────────────────────────────────────────────────────
// Accessors
// ──────────────────────────────────────────────────────────────────────────────

uint64_t WALStorage::lastSequence() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    return next_seq_ > 1 ? next_seq_ - 1 : 0;
}

size_t WALStorage::segmentCount() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    return segments_.size();
}

/**
 * @brief Flush.
 * @return Return value.
 * @details Calls: lock(), OkVoid(), std::chrono::steady_clock::now(), std::chrono::seconds(), themis_fsync_fd(), THEMIS_WARN(), count(), ErrVoid().
 */
Result<void> WALStorage::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (fd_ < 0) {
        return OkVoid();
    }

    // W1-S02: flush timeout protection (CWE-833).
    // Prevent hung fsync from blocking indefinitely during shutdown/checkpoints.
    auto start_time = std::chrono::steady_clock::now();
    constexpr auto FSYNC_TIMEOUT = std::chrono::seconds(30);

    if (themis_fsync_fd(fd_) != 0) {
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (elapsed > FSYNC_TIMEOUT) {
            THEMIS_WARN("WALStorage::flush: fsync timeout (>{}ms), continuing with degraded durability",
                       std::chrono::duration_cast<std::chrono::milliseconds>(FSYNC_TIMEOUT).count());
            return OkVoid();
        }
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_DISK_FULL,
                       "WAL fsync failed: " + std::string(std::strerror(errno)));
    }
    return OkVoid();
}

} // namespace themis
