/**
 * @file storage_audit_logger.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.46
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "utils/expected.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

namespace themis {

/**
 * @brief Append-only, rotating audit trail for storage-layer events.
 *
 * StorageAuditLogger records every significant storage mutation (PUT, DEL,
 * CHECKPOINT, RECOVERY, COMPACTION, SNAPSHOT) to a text log file so that
 * compliance tools and post-mortem forensics can reconstruct the history of
 * the database.
 *
 * ## Log entry format (one line per event)
 *
 *   <ISO-8601 timestamp> <sequence> <event> <key> [<extra>]\n
 *
 * Example:
 *
 *   2025-11-12T14:03:22Z 000000042 PUT user:42 bytes=128
 *   2025-11-12T14:03:23Z 000000043 DEL user:42
 *   2025-11-12T14:03:24Z 000000044 CHECKPOINT wal_000003.log seq=1024
 *
 * ## Log rotation
 *
 * When the active log file exceeds @c Config::max_file_bytes, it is closed
 * and a new segment is opened (audit_000001.log, audit_000002.log, …).
 *
 * ## Thread safety
 *
 * All public methods are thread-safe.
 */
class StorageAuditLogger {
public:
    /** Storage event types. */
    enum class Event : uint8_t {
        PUT         = 1,  ///< Key-value write
        DEL         = 2,  ///< Key deletion
        CHECKPOINT  = 3,  ///< WAL checkpoint completed
        RECOVERY    = 4,  ///< WAL replay / crash-recovery completed
        COMPACTION  = 5,  ///< Compaction pass completed
        SNAPSHOT    = 6,  ///< Online snapshot created or restored
    };

    /** Configuration options. */
    struct Config {
        std::string dir;                            ///< Directory for audit log files
        uint64_t    max_file_bytes = 32 * 1024 * 1024; ///< Rotate at 32 MiB
        bool        sync_on_write  = false;         ///< fsync every entry (slower, safer)
    };

    /**
     * @brief Open (or create) an audit log in @p config.dir.
     *
     * Existing segment files are preserved; new entries are appended to a
     * new segment so the previous segments remain immutable.
     *
     * @return Result<unique_ptr<StorageAuditLogger>> on success, Error on failure.
     */
    static Result<std::unique_ptr<StorageAuditLogger>> open(const Config& config);

    ~StorageAuditLogger();

    // Not copyable or movable after construction.
    StorageAuditLogger(const StorageAuditLogger&) = delete;
    StorageAuditLogger& operator=(const StorageAuditLogger&) = delete;

    // ── Logging methods ───────────────────────────────────────────────────

    /** Log a PUT event.  @p extra is an optional detail string (e.g. "bytes=128"). */
    Result<void> logPut(std::string_view key, std::string_view extra = "");

    /** Log a DEL event. */
    Result<void> logDel(std::string_view key, std::string_view extra = "");

    /** Log a CHECKPOINT event. */
    Result<void> logCheckpoint(std::string_view detail = "");

    /** Log a RECOVERY event. */
    Result<void> logRecovery(std::string_view detail = "");

    /** Log a COMPACTION event. */
    Result<void> logCompaction(std::string_view detail = "");

    /** Log a SNAPSHOT event. */
    Result<void> logSnapshot(std::string_view detail = "");

    /** Generic log: log any event with a key and extra detail. */
    Result<void> log(Event event, std::string_view key, std::string_view extra = "");

    // ── Accessors ─────────────────────────────────────────────────────────

    /** Return the sequence number of the last logged entry (0 if none). */
    uint64_t lastSequence() const;

    /** Return the number of log segment files currently on disk. */
    size_t segmentCount() const;

    /** Flush buffered writes to the OS. */
    Result<void> flush();

    // ── Helpers (public for testing) ──────────────────────────────────────

    /** Build the segment file name for a given segment ID. */
    static std::string segmentName(uint64_t segment_id);

    /** Convert an Event enum to its string token (e.g. Event::PUT → "PUT"). */
    static std::string_view eventName(Event e);

private:
    explicit StorageAuditLogger(const Config& cfg);

    Result<void> openOrCreate();
    Result<void> rotateIfNeeded();
    Result<void> openNewSegment();
    Result<void> writeEntry(Event event, std::string_view key, std::string_view extra);
    void syncIfRequired();

    static std::string currentTimestamp();

    Config             config_;
    mutable std::mutex mutex_;
    int                fd_{-1};              // current segment file descriptor
    uint64_t           current_segment_{0};  // index of current segment
    uint64_t           segment_bytes_{0};    // bytes written to current segment
    uint64_t           next_seq_{1};         // next sequence number to assign
    uint64_t           last_seq_{0};         // sequence number of last written entry

    std::vector<uint64_t> segments_;         // segment IDs on disk
};

} // namespace themis

