/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wal_storage.cpp                                    ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 05:44:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     508                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • 35e7ecae2c  2026-04-13  perf(storage): fix sustained write throughput - decouple ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "storage/wal_storage.h"
#include "utils/error_registry.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <iomanip>

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
static int themis_open_fd(const char* path, int flags, int mode) { return _open(path, flags, mode); }
static int themis_close_fd(int fd) { return _close(fd); }
static int themis_fsync_fd(int fd) { return _commit(fd); }
static themis_ssize_t themis_write_fd(int fd, const void* data, size_t len) {
    return static_cast<themis_ssize_t>(_write(fd, data, static_cast<unsigned int>(len)));
}
#else
using themis_ssize_t = ssize_t;
static int themis_open_fd(const char* path, int flags, int mode) { return ::open(path, flags, mode); }
static int themis_close_fd(int fd) { return ::close(fd); }
static int themis_fsync_fd(int fd) { return ::fsync(fd); }
static themis_ssize_t themis_write_fd(int fd, const void* data, size_t len) {
    return ::write(fd, data, len);
}
#endif

static bool write_all_fd(int fd, const void* data, size_t len) {
    const uint8_t* ptr = static_cast<const uint8_t*>(data);
    size_t remaining = len;
    while (remaining > 0) {
        themis_ssize_t written = themis_write_fd(fd, ptr, remaining);
        if (written <= 0) {
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

// ──────────────────────────────────────────────────────────────────────────────
// CRC32 (simple table-based implementation; no external dependency)
// ──────────────────────────────────────────────────────────────────────────────

static uint32_t crc32_update(uint32_t crc, const void* data, size_t len) {
    // Build the CRC32 table on first call (constexpr-safe, no lambda).
    static uint32_t table[256];
    static bool initialized = false;
    if (!initialized) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            table[i] = c;
        }
        initialized = true;
    }

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

static void encode_u32(uint8_t* buf, uint32_t v) {
    buf[0] = static_cast<uint8_t>(v);
    buf[1] = static_cast<uint8_t>(v >> 8);
    buf[2] = static_cast<uint8_t>(v >> 16);
    buf[3] = static_cast<uint8_t>(v >> 24);
}

static void encode_u64(uint8_t* buf, uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        buf[i] = static_cast<uint8_t>(v >> (8 * i));
    }
}

static uint32_t decode_u32(const uint8_t* buf) {
    return static_cast<uint32_t>(buf[0])
         | (static_cast<uint32_t>(buf[1]) << 8)
         | (static_cast<uint32_t>(buf[2]) << 16)
         | (static_cast<uint32_t>(buf[3]) << 24);
}

static uint64_t decode_u64(const uint8_t* buf) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= (static_cast<uint64_t>(buf[i]) << (8 * i));
    }
    return v;
}

// ──────────────────────────────────────────────────────────────────────────────
// Segment naming
// ──────────────────────────────────────────────────────────────────────────────

std::string WALStorage::segmentName(uint64_t segment_id) {
    std::ostringstream ss;
    ss << "wal_" << std::setw(6) << std::setfill('0') << segment_id << ".log";
    return ss.str();
}

uint64_t WALStorage::parseSegmentId(const std::string& filename) {
    static const std::regex re(R"(wal_(\d+)\.log)");
    std::smatch m;
    if (std::regex_match(filename, m, re)) {
        return std::stoull(m[1]);
    }
    return 0;
}

// ──────────────────────────────────────────────────────────────────────────────
// Constructor / factory
// ──────────────────────────────────────────────────────────────────────────────

WALStorage::WALStorage(const Config& cfg) : config_(cfg) {}

WALStorage::~WALStorage() {
    if (fd_ >= 0) {
        themis_fsync_fd(fd_);
        themis_close_fd(fd_);
        fd_ = -1;
    }
}

Result<std::unique_ptr<WALStorage>> WALStorage::open(
    const Config& config,
    RecoveryCallback on_recover
) {
    auto wal = std::unique_ptr<WALStorage>(new WALStorage(config));
    auto res = wal->openOrCreate(on_recover);
    if (!res) {
        return Err<std::unique_ptr<WALStorage>>(res.error().code(), res.error().context());
    }
    return Ok(std::move(wal));
}

// ──────────────────────────────────────────────────────────────────────────────
// Open / create / replay
// ──────────────────────────────────────────────────────────────────────────────

Result<void> WALStorage::openOrCreate(RecoveryCallback& on_recover) {
    // Create directory if it doesn't exist.
    std::error_code ec;
    fs::create_directories(config_.dir, ec);
    if (ec) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_DISK_FULL,
                       "cannot create WAL directory: " + ec.message());
    }

    // Collect existing segments sorted by ID.
    segments_.clear();
    for (const auto& entry : fs::directory_iterator(config_.dir)) {
        if (!entry.is_regular_file()) continue;
        uint64_t sid = parseSegmentId(entry.path().filename().string());
        if (sid > 0) {
            segments_.push_back(sid);
        }
    }
    std::sort(segments_.begin(), segments_.end());

    // Replay existing segments.
    if (on_recover) {
        for (uint64_t sid : segments_) {
            auto path = config_.dir + "/" + segmentName(sid);
            [[maybe_unused]] auto res = replaySegment(path, on_recover);
            if (!res) {
                // Log but continue – partial entries at end of file are tolerated.
            }
        }
    } else {
        // Still scan to find the highest sequence number.
        for (uint64_t sid : segments_) {
            auto path = config_.dir + "/" + segmentName(sid);
            RecoveryCallback noop = [this](const Entry& e) {
                if (e.sequence >= next_seq_) next_seq_ = e.sequence + 1;
                return true;
            };
            (void)replaySegment(path, noop);
        }
    }

    // Open (or create) the latest segment for appending.
    if (segments_.empty()) {
        segments_.push_back(1);
        current_segment_ = 1;
    } else {
        current_segment_ = segments_.back();
    }

    return openNewSegment();
}

Result<void> WALStorage::replaySegment(const std::string& path,
                                        RecoveryCallback& cb) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
                       "cannot open WAL segment: " + path);
    }

    while (f.good()) {
        // Read fixed header.
        uint8_t hdr[HEADER_SIZE];
        f.read(reinterpret_cast<char*>(hdr), HEADER_SIZE);
        if (f.gcount() == 0) break;                      // clean EOF
        if (static_cast<size_t>(f.gcount()) < HEADER_SIZE) break; // truncated

        uint32_t magic = decode_u32(hdr);
        if (magic != WAL_MAGIC) break; // corruption / partial write

        uint64_t seq   = decode_u64(hdr + 4);
        auto     type  = static_cast<EntryType>(hdr[12]);
        uint32_t klen  = decode_u32(hdr + 13);
        uint32_t vlen  = decode_u32(hdr + 17);

        // Read key and value.
        std::string key(klen, '\0');
        if (klen > 0) {
            f.read(key.data(), klen);
            if (static_cast<uint32_t>(f.gcount()) < klen) break;
        }

        std::string value(vlen, '\0');
        if (vlen > 0) {
            f.read(value.data(), vlen);
            if (static_cast<uint32_t>(f.gcount()) < vlen) break;
        }

        // Read and verify CRC32.
        uint8_t crc_buf[4];
        f.read(reinterpret_cast<char*>(crc_buf), 4);
        if (f.gcount() < 4) break;

        uint32_t stored_crc = decode_u32(crc_buf);
        uint32_t computed   = crc32_update(0, hdr, HEADER_SIZE);
        computed = crc32_update(computed, key.data(), klen);
        computed = crc32_update(computed, value.data(), vlen);

        if (computed != stored_crc) break; // checksum mismatch → truncate here

        // Track highest sequence.
        if (seq >= next_seq_) next_seq_ = seq + 1;

        Entry e{seq, type, std::move(key), std::move(value)};
        if (!cb(e)) break; // caller requested early stop
    }

    return OkVoid();
}

// ──────────────────────────────────────────────────────────────────────────────
// Segment management
// ──────────────────────────────────────────────────────────────────────────────

Result<void> WALStorage::openNewSegment() {
    if (fd_ >= 0) {
        themis_fsync_fd(fd_);
        themis_close_fd(fd_);
        fd_ = -1;
    }

    std::string path = config_.dir + "/" + segmentName(current_segment_);
    // O_APPEND ensures atomic position tracking; O_CREAT creates if absent.
    fd_ = themis_open_fd(path.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
    if (fd_ < 0) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_DISK_FULL,
                       "cannot open WAL segment '" + path + "': " +
                           std::strerror(errno));
    }

    // Determine how many bytes are already in the file.
    struct stat st{};
    if (::fstat(fd_, &st) == 0) {
        segment_bytes_ = static_cast<uint64_t>(st.st_size);
    } else {
        segment_bytes_ = 0;
    }

    return OkVoid();
}

Result<void> WALStorage::rotateIfNeeded() {
    if (segment_bytes_ < config_.rotation_threshold_bytes) {
        return OkVoid();
    }

    ++current_segment_;
    segments_.push_back(current_segment_);
    segment_bytes_ = 0;

    return openNewSegment();
}

// ──────────────────────────────────────────────────────────────────────────────
// Append helpers
// ──────────────────────────────────────────────────────────────────────────────

Result<uint64_t> WALStorage::appendEntry(EntryType type,
                                          std::string_view key,
                                          std::string_view value) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Rotate first so we never start a new segment half-way through an entry.
    if (auto r = rotateIfNeeded(); !r) return Err<uint64_t>(r.error().code(), r.error().context());

    auto res = appendEntryLocked(type, key, value);
    if (!res) return res;

    syncIfRequired();
    return res;
}

// Write one entry to fd_ WITHOUT locking or syncing.  Caller holds mutex_.
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
    encode_u32(crc_buf, crc);

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

void WALStorage::syncIfRequired() {
    if (config_.fsync_on_write) {
        themis_fsync_fd(fd_);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Public write API
// ──────────────────────────────────────────────────────────────────────────────

Result<uint64_t> WALStorage::appendPut(std::string_view key,
                                        std::string_view value) {
    return appendEntry(EntryType::PUT, key, value);
}

Result<uint64_t> WALStorage::appendDelete(std::string_view key) {
    return appendEntry(EntryType::DEL, key, {});
}

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
            return Err<uint64_t>(r.error().code(), r.error().context());
        }

        auto res = appendEntryLocked(e.type, e.key, e.value);
        if (!res) return res;
        last_seq = *res;
    }

    // Single fsync for the entire batch — the key advantage over N individual
    // appendEntry() calls when fsync_on_write is enabled.
    syncIfRequired();
    return Ok(last_seq);
}

Result<uint64_t> WALStorage::checkpoint(bool delete_old_segments) {
    auto res = appendEntry(EntryType::CHECKPOINT, {}, {});
    if (!res) return res;

    if (delete_old_segments) {
        std::lock_guard<std::mutex> lock(mutex_);
        // Remove all segments except the current one.
        std::vector<uint64_t> to_remove;
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
    std::lock_guard<std::mutex> lock(mutex_);
    return next_seq_ > 1 ? next_seq_ - 1 : 0;
}

size_t WALStorage::segmentCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return segments_.size();
}

Result<void> WALStorage::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (fd_ >= 0 && themis_fsync_fd(fd_) != 0) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_DISK_FULL,
                       "WAL fsync failed: " + std::string(std::strerror(errno)));
    }
    return OkVoid();
}

} // namespace themis
