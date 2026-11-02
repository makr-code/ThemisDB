/**
 * @file wal_storage.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.46
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "utils/expected.h"
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace themis {

/**
 * @brief Write-Ahead Log (WAL) for storage durability and crash recovery.
 *
 * WALStorage provides a persistent, append-only log that records every
 * mutation before it is applied to the primary store.  On startup the log
 * is replayed to recover any writes that were not yet flushed.
 *
 * ## Log entry format (binary, little-endian)
 *
 *   [4 bytes] magic marker  (0xDBAB1234)
 *   [8 bytes] sequence number (monotonically increasing)
 *   [1 byte]  entry type    (PUT=1, DELETE=2, CHECKPOINT=3)
 *   [4 bytes] key length
 *   [N bytes] key data
 *   [4 bytes] value length  (0 for DELETE and CHECKPOINT)
 *   [M bytes] value data
 *   [4 bytes] CRC32 checksum over all preceding bytes in this entry
 *
 * ## Log rotation
 *
 * When the active log file exceeds @c rotation_threshold_bytes, a new log
 * segment is created (e.g. wal_000001.log, wal_000002.log …).  After a
 * successful checkpoint the old segments may be deleted.
 *
 * ## Thread safety
 *
 * All public methods are thread-safe. Internally, all access to the file
 * descriptor (fd_), current segment ID, and segment state is protected by
 * mutex_. Exception safety is guaranteed:
 * - **open()**: Factory method; if openOrCreate() throws, the partial object is cleaned up.
 * - **append operations**: If an operation fails, fd_ and segment state remain valid for
 *   retry or further operations. No state corruption on exception.
 * - **Destructor (~WALStorage)**: Guarantees fd_ is closed even during exception unwinding.
 *   Uses RAII (ScopedFileDescriptor) for file descriptor lifecycle.
 * - **Segment open/close**: fd_ transitions are atomic; old segment is fsynced and closed
 *   before new segment is activated. fd_=-1 sentinel prevents fd leaks.
 *
 * ## Exception Safety: Strong Guarantee
 *
 * - **open()**: Returns Result; no exception. Factory success == valid WALStorage object.
 * - **appendPut/appendDelete/appendBatch()**: Return Result. On error, WAL state unchanged
 *   and ready for retry.
 * - **checkpoint()**: Return Result. May write CHECKPOINT entry; fsync is "best-effort"
 *   in timeout cases.
 * - **Destructor**: No-throw; uses lock (may block but never throws).
 */
class WALStorage {
public:
    /** Logical type of a WAL entry. */
    enum class EntryType : uint8_t {
        PUT        = 1,   ///< Key-value write
        DEL        = 2,   ///< Key deletion
        CHECKPOINT = 3,   ///< Flush-point marker
    };

    /** A single recovered WAL entry. */
    struct Entry {
        uint64_t    sequence = 0;
        EntryType   type;
        std::string key;
        std::string value;   // empty for DEL / CHECKPOINT
    };

    /** Recovery status reported by open(). */
    enum class RecoveryStatus {
        Complete,
        TruncatedTail,
        Corrupted,
        TimedOut,
        Failed,
    };

    /**
     * @brief Structured outcome of scanning the WAL during open().
     *
     * @note In skip-corrupted mode, Corrupted can accompany a successful open;
     *       the affected segment is quarantined and skipped_bytes reports its size.
     */
    struct RecoveryResult {
        RecoveryStatus status = RecoveryStatus::Complete;
        uint64_t last_valid_lsn = 0;
        uint64_t skipped_bytes = 0;
        std::string segment;
    };

    /** Policy for handling corrupt WAL segments. */
    enum class RecoveryMode {
        Strict,
        SkipCorrupted,
    };

    /** Configuration options. */
    struct Config {
        std::string dir;                              ///< Directory for WAL segment files
        uint64_t    rotation_threshold_bytes = 64 * 1024 * 1024; ///< Rotate at 64 MiB
        bool        fsync_on_write = true;            ///< fsync every individual entry (max durability)
        RecoveryMode recovery_mode = RecoveryMode::Strict; ///< Strictly reject corruption unless explicitly overridden
        std::chrono::milliseconds recovery_timeout{0}; ///< Zero means no recovery timeout
    };

    /**
     * @brief A batch entry passed to appendBatch().
     *
     * Using a dedicated BatchEntry (rather than Entry) avoids confusion with
     * the sequence-bearing Entry returned during recovery replay.
     */
    struct BatchEntry {
        EntryType        type;
        std::string_view key;
        std::string_view value;   ///< empty for DEL
    };

    /**
     * @brief Callback type for recovery replay.
     *
     * Called once per WAL entry during open().  Return @c false to abort
     * replay early (e.g. on an unrecoverable application error); in this case
     * open() fails and does not return a usable WALStorage instance.
     */
    using RecoveryCallback = std::function<bool(const Entry&)>;

    /**
     * @brief Open (or create) the WAL in the given directory.
     *
     * On first open, an empty WAL is created.
     * On subsequent opens, existing segments are replayed via @p on_recover.
     *
     * @param config  WAL configuration (directory, rotation, fsync, recovery policy).
     * @param on_recover  Callback invoked for each recovered entry; may be null.
     * @param recovery_result Optional structured status, populated on success and failure.
     * @return Result<std::unique_ptr<WALStorage>> on success, Error on failure.
     *         Corrupt records, recovery timeouts, I/O errors, or a callback
     *         that aborts replay are reported as errors. A physically
     *         incomplete final record is truncated to the last complete record
     *         before the segment is reopened for appends. Corruption is skipped
     *         only in SkipCorrupted mode, after the affected segment is quarantined.
     */
    static Result<std::unique_ptr<WALStorage>> open(
        const Config& config,
        RecoveryCallback on_recover = nullptr,
        RecoveryResult* recovery_result = nullptr
    );

    ~WALStorage();

    // Not copyable or movable after construction.
    WALStorage(const WALStorage&) = delete;
    WALStorage& operator=(const WALStorage&) = delete;

    // ── Write operations ──────────────────────────────────────────────────

    /**
     * @brief Append a PUT entry to the active WAL segment.
     *
     * Stores the key/value pair durably according to the current fsync policy,
     * then assigns the next monotonically increasing sequence number.
     *
     * @param key Logical key to persist. Empty keys are rejected by the WAL layer
     *            and reported as an error in the returned Result.
     * @param value Value payload to store. Empty values are permitted and are
     *              serialized as a valid zero-length payload.
     * @return The assigned sequence number on success, or an error describing
     *         why the append could not be completed.
     * @note This API does not throw; failures are surfaced through Result.
     */
    Result<uint64_t> appendPut(std::string_view key, std::string_view value);

    /**
     * @brief Append a DELETE entry to the active WAL segment.
     *
     * The delete is treated as a durable tombstone and will be replayed during
     * recovery to remove any prior value for @p key.
     *
     * @param key Logical key to remove. Empty keys are rejected and reported as
     *            an error in the returned Result.
     * @return The assigned sequence number on success, or an error describing
     *         why the delete could not be serialized.
     * @note This API does not throw; failures are surfaced through Result.
     */
    Result<uint64_t> appendDelete(std::string_view key);

    /**
     * @brief Group-commit: write multiple entries with a single fsync.
     *
     * All entries in @p entries are serialised and written in sequence, then
     * the WAL file is fsynced exactly once (when Config::fsync_on_write is true).
     * This amortises fsync overhead across many writes, allowing sustained
     * throughput in the 100k+ ops/s range while retaining ACID durability.
     *
     * If the batch spans a segment boundary, a rotation is performed between
     * segments and each segment is individually fsynced before rotation.
     *
     * @param entries  Non-empty span of entries to write atomically.
     * @return The sequence number of the *last* entry in the batch, or an error.
     */
    Result<uint64_t> appendBatch(std::vector<BatchEntry> entries);

    /**
     * @brief Write a CHECKPOINT entry and optionally delete old segments.
     *
     * After a successful checkpoint the primary store is guaranteed to
     * contain all mutations up to the returned sequence number.
     * Old WAL segments whose highest sequence number is ≤ the checkpoint
     * sequence may be safely removed.
     *
     * @param delete_old_segments  If true, remove fully-checkpointed segments.
     * @return The sequence number of the checkpoint entry.
     */
    Result<uint64_t> checkpoint(bool delete_old_segments = true);

    // ── Accessors ─────────────────────────────────────────────────────────

    /**
      * @brief Return the highest sequence number assigned to any WAL entry.
      *
      * This value reflects the latest successfully appended record and is used to
      * determine how far the log has advanced.
      *
      * @return The most recent sequence number, or 0 when the WAL is empty.
      */
    uint64_t lastSequence() const;

    /**
      * @brief Return the number of WAL segment files currently present on disk.
      *
      * This count includes the active segment and any retained historical segments
      * that have not yet been cleaned up by checkpoint rotation.
      *
      * @return Number of WAL segment files currently present for this instance.
      */
    size_t segmentCount() const;

    /**
      * @brief Flush buffered writes to the OS file layer.
      *
      * The call guarantees that all pending writes are handed off to the OS, but
      * it does not guarantee the underlying storage device has physically
      * committed the data to stable media unless the fsync policy is enabled.
      *
      * @return Empty success result, or an error if the flush could not complete.
      */
    Result<void> flush();

    // ── Key encoding helpers (public for testing) ─────────────────────────

    /**
      * @brief Build the canonical on-disk file name for a WAL segment.
      *
      * Segment IDs are zero-padded to six digits to provide stable ordering in
      * filesystem listings.
      *
      * @param segment_id Monotonic segment identifier to encode into the filename.
      * @return Canonical segment filename in the form `wal_000001.log`.
      */
    static std::string segmentName(uint64_t segment_id);

    /**
      * @brief Parse the segment identifier from a WAL filename.
      *
      * The helper accepts the canonical naming scheme emitted by
      * segmentName() and returns the numeric segment ID. Invalid or unexpected
      * names return 0.
      *
      * @param filename Candidate WAL filename to parse.
      * @return Parsed segment ID, or 0 when the filename is malformed.
      */
    static uint64_t parseSegmentId(const std::string& filename);

private:
    explicit WALStorage(const Config& cfg);

    Result<void> openOrCreate(RecoveryCallback& on_recover,
                              RecoveryResult& recovery_result);
    Result<void> replaySegment(const std::string& path,
                               RecoveryCallback& cb,
                               RecoveryResult& recovery_result,
                               std::chrono::steady_clock::time_point recovery_started_at);
    Result<void> rotateIfNeeded();
    Result<void> openNewSegment(uint64_t segment_id);
    Result<uint64_t> appendEntry(EntryType type,
                                  std::string_view key,
                                  std::string_view value);
    // Write one entry to fd_ without taking the mutex or calling syncIfRequired().
    // Caller must hold mutex_ and handle rotation/fsync externally.
    Result<uint64_t> appendEntryLocked(EntryType type,
                                        std::string_view key,
                                        std::string_view value);
    Result<void> syncIfRequired();

    Config                config_;
    mutable std::mutex    mutex_;
    int                   fd_{-1};            // current segment file descriptor
    uint64_t              current_segment_{0};// index of current segment file
    uint64_t              segment_bytes_{0};  // bytes written to current segment
    uint64_t              next_seq_{1};       // next sequence number to assign

    // Track segment IDs present on disk for cleanup
    std::vector<uint64_t> segments_;
};

} // namespace themis
