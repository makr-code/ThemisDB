/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            storage_audit_logger.cpp                           ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 20:36:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     302                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "storage/storage_audit_logger.h"
#include "utils/error_registry.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <regex>
#include <sstream>
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

// ──────────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────────

std::string_view StorageAuditLogger::eventName(Event e) {
    switch (e) {
        case Event::PUT:        return "PUT";
        case Event::DEL:        return "DEL";
        case Event::CHECKPOINT: return "CHECKPOINT";
        case Event::RECOVERY:   return "RECOVERY";
        case Event::COMPACTION: return "COMPACTION";
        case Event::SNAPSHOT:   return "SNAPSHOT";
        default:                return "UNKNOWN";
    }
}

/* static */ std::string StorageAuditLogger::segmentName(uint64_t segment_id) {
    std::ostringstream oss;
    oss << "audit_" << std::setw(6) << std::setfill('0') << segment_id << ".log";
    return oss.str();
}

/* static */ std::string StorageAuditLogger::currentTimestamp() {
    std::time_t t = std::time(nullptr);
    std::tm tm_utc{};
#if defined(_WIN32)
    gmtime_s(&tm_utc, &t);
#else
    gmtime_r(&t, &tm_utc);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
    return buf;
}

// ──────────────────────────────────────────────────────────────────────────────
// Construction / open
// ──────────────────────────────────────────────────────────────────────────────

StorageAuditLogger::StorageAuditLogger(const Config& cfg) : config_(cfg) {}

StorageAuditLogger::~StorageAuditLogger() {
    if (fd_ >= 0) {
        themis_fsync_fd(fd_);
        themis_close_fd(fd_);
        fd_ = -1;
    }
}

/* static */
Result<std::unique_ptr<StorageAuditLogger>>
StorageAuditLogger::open(const Config& config) {
    if (config.dir.empty()) {
        return Err<std::unique_ptr<StorageAuditLogger>>(
            errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
            "StorageAuditLogger: config.dir must not be empty");
    }

    std::error_code ec;
    fs::create_directories(config.dir, ec);
    if (ec) {
        return Err<std::unique_ptr<StorageAuditLogger>>(
            errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
            "StorageAuditLogger: cannot create directory: " + ec.message());
    }

    auto logger = std::unique_ptr<StorageAuditLogger>(
        new StorageAuditLogger(config));

    auto res = logger->openOrCreate();
    if (!res.has_value()) {
        return Err<std::unique_ptr<StorageAuditLogger>>(
            errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
            "StorageAuditLogger: open failed: " + res.error().message());
    }

    return Ok(std::move(logger));
}

Result<void> StorageAuditLogger::openOrCreate() {
    // Discover existing segments
    std::vector<uint64_t> found;
    static const std::regex seg_re("audit_(\\d+)\\.log");
    for (const auto& entry : fs::directory_iterator(config_.dir)) {
        std::string fn = entry.path().filename().string();
        std::smatch m;
        if (std::regex_match(fn, m, seg_re)) {
            found.push_back(std::stoull(m[1].str()));
        }
    }
    std::sort(found.begin(), found.end());
    segments_ = found;

    // Always start a new segment so existing ones stay immutable
    uint64_t new_id = segments_.empty() ? 0 : segments_.back() + 1;
    current_segment_ = new_id;
    segments_.push_back(new_id);
    segment_bytes_  = 0;

    std::string path = (fs::path(config_.dir) / segmentName(new_id)).string();
    fd_ = themis_open_fd(path.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
    if (fd_ < 0) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       std::string("StorageAuditLogger: open failed: ") +
                       std::strerror(errno));
    }
    return OkVoid();
}

// ──────────────────────────────────────────────────────────────────────────────
// Logging
// ──────────────────────────────────────────────────────────────────────────────

Result<void> StorageAuditLogger::log(Event event,
                                      std::string_view key,
                                      std::string_view extra) {
    std::lock_guard<std::mutex> lock(mutex_);
    return writeEntry(event, key, extra);
}

Result<void> StorageAuditLogger::logPut(std::string_view key,
                                         std::string_view extra) {
    return log(Event::PUT, key, extra);
}

Result<void> StorageAuditLogger::logDel(std::string_view key,
                                         std::string_view extra) {
    return log(Event::DEL, key, extra);
}

Result<void> StorageAuditLogger::logCheckpoint(std::string_view detail) {
    return log(Event::CHECKPOINT, "", detail);
}

Result<void> StorageAuditLogger::logRecovery(std::string_view detail) {
    return log(Event::RECOVERY, "", detail);
}

Result<void> StorageAuditLogger::logCompaction(std::string_view detail) {
    return log(Event::COMPACTION, "", detail);
}

Result<void> StorageAuditLogger::logSnapshot(std::string_view detail) {
    return log(Event::SNAPSHOT, "", detail);
}

Result<void> StorageAuditLogger::writeEntry(Event event,
                                             std::string_view key,
                                             std::string_view extra) {
    // Rotate before writing if needed
    auto rot = rotateIfNeeded();
    if (!rot.has_value()) return rot;

    // Build line:  <ts> <seq> <event> <key> [<extra>]\n
    std::ostringstream oss;
    oss << currentTimestamp() << ' '
        << std::setw(12) << std::setfill('0') << next_seq_ << ' '
        << eventName(event);
    if (!key.empty())   oss << ' ' << key;
    if (!extra.empty()) oss << ' ' << extra;
    oss << '\n';

    std::string line = oss.str();
    themis_ssize_t written = themis_write_fd(fd_, line.data(), line.size());
    if (written < 0 || static_cast<size_t>(written) != line.size()) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       std::string("StorageAuditLogger: write failed: ") +
                       std::strerror(errno));
    }
    last_seq_ = next_seq_++;
    segment_bytes_ += static_cast<uint64_t>(written);
    syncIfRequired();
    return OkVoid();
}

Result<void> StorageAuditLogger::rotateIfNeeded() {
    if (segment_bytes_ < config_.max_file_bytes) return OkVoid();

    if (fd_ >= 0) {
        themis_fsync_fd(fd_);
        themis_close_fd(fd_);
        fd_ = -1;
    }

    uint64_t new_id = segments_.back() + 1;
    current_segment_ = new_id;
    segments_.push_back(new_id);
    segment_bytes_ = 0;

    std::string path = (fs::path(config_.dir) / segmentName(new_id)).string();
    fd_ = themis_open_fd(path.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
    if (fd_ < 0) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       std::string("StorageAuditLogger: rotate open failed: ") +
                       std::strerror(errno));
    }
    return OkVoid();
}

void StorageAuditLogger::syncIfRequired() {
    if (config_.sync_on_write && fd_ >= 0) {
        themis_fsync_fd(fd_);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Accessors
// ──────────────────────────────────────────────────────────────────────────────

uint64_t StorageAuditLogger::lastSequence() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_seq_;
}

size_t StorageAuditLogger::segmentCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return segments_.size();
}

Result<void> StorageAuditLogger::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (fd_ >= 0 && themis_fsync_fd(fd_) != 0) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       std::string("StorageAuditLogger: fsync failed: ") +
                       std::strerror(errno));
    }
    return OkVoid();
}

} // namespace themis
