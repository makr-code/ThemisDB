/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            temporal_cdc.h                                     ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:47:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     260                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 48fbf5b222  2026-03-21  Update search, temporal, and build artifacts ║
    • c5ff147e9f  2026-03-20  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Temporal Change Data Capture (CDC)
 *
 * Version-aware change event streaming for system-versioned tables.
 *
 * ## Features
 * - Typed ChangeEvent with before/after payloads and temporal metadata
 * - Named, persistent subscriptions (subscribe/unsubscribe)
 * - Fire-and-forget event emission (publishEvent) from write paths
 * - In-process event log with bounded ring-buffer (default: 65536 events)
 * - replayChanges() for point-in-time backfill / audit replay
 * - Thread-safe: all public methods safe for concurrent callers
 *
 * ## Usage
 * ```cpp
 * TemporalCDC cdc;
 * std::string sub_id = cdc.subscribeToChanges("employees",
 *     [](const ChangeEvent& ev) {
 *         // handle ev.type, ev.after_value, ev.transaction_time …
 *     });
 *
 * // Emit events from the write path:
 * ChangeEvent ev;
 * ev.type           = ChangeType::INSERT;
 * ev.table_name     = "employees";
 * ev.entity_id      = "emp:42";
 * ev.after_value    = doc;
 * ev.transaction_time = now();
 * cdc.publishEvent(ev);
 *
 * // Later replay:
 * auto events = cdc.replayChanges("employees", {t_start, t_end});
 * cdc.unsubscribe(sub_id);
 * ```
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "temporal/temporal_types.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themisdb {
namespace temporal {

// ============================================================================
// ChangeType
// ============================================================================

/**
 * Policy applied when the ring-buffer event log reaches `max_log_size`.
 *
 * | Policy    | Behaviour                                                    |
 * |-----------|--------------------------------------------------------------|
 * | OVERWRITE | (default) The oldest event in the ring-buffer is silently    |
 * |           | discarded to make room for the new event.  This is a        |
 * |           | circular-overwrite (FIFO eviction) strategy: every new event |
 * |           | always succeeds, but old events may be lost.                 |
 * | BLOCK     | `publishEvent` blocks (under the internal mutex) until a     |
 * |           | consumer drains at least one slot.  Use with caution in      |
 * |           | latency-sensitive write paths; reserved for future use.      |
 * | DROP      | The new event is silently dropped; the ring-buffer contents  |
 * |           | are preserved.  `overflowCount()` is incremented for every   |
 * |           | dropped event, giving the caller a way to detect back-        |
 * |           | pressure without consuming CPU on lock contention.           |
 *
 * The current implementation supports OVERWRITE and DROP.  BLOCK is accepted
 * as a constructor argument but falls back to OVERWRITE with a debug assertion.
 */
enum class OverflowPolicy {
    OVERWRITE, ///< Evict oldest event to make room (default, never blocks)
    BLOCK,     ///< Block until a consumer frees space (future / reserved)
    DROP       ///< Silently discard new events when the buffer is full
};

/** Discriminator for the kind of change captured by a ChangeEvent. */
enum class ChangeType {
    INSERT,          ///< A new row was inserted (no before_value)
    UPDATE,          ///< An existing row was updated
    DELETE,          ///< A row was logically deleted (no after_value)
    VERSION_CREATED  ///< A new historical version was closed (sys_end set)
};

// ============================================================================
// ChangeEvent
// ============================================================================

/**
 * A single versioned change event emitted by the temporal CDC subsystem.
 *
 * `before_value` is empty for INSERT events.
 * `after_value`  is empty for DELETE events.
 * `transaction_time` records when the change was committed (system time).
 * `valid_from` / `valid_to` carry the application-time period when present.
 */
struct ChangeEvent {
    ChangeType  type{ChangeType::INSERT};
    std::string table_name;
    std::string entity_id;

    Document before_value;  ///< Previous payload (empty for INSERT)
    Document after_value;   ///< New payload (empty for DELETE)

    Timestamp transaction_time{0};  ///< Commit timestamp (ms since epoch)
    Timestamp valid_from{kMinTimestamp};
    Timestamp valid_to{kMaxTimestamp};

    std::string user_id;   ///< User / node that made the change (optional)

    /** Serialise to JSON for transport or storage. */
    nlohmann::json toJson() const;

    /** Deserialise from JSON. */
    static ChangeEvent fromJson(const nlohmann::json& j);
};

// ============================================================================
// TemporalCDC
// ============================================================================

/**
 * @brief In-process temporal change data capture engine.
 *
 * `TemporalCDC` decouples change producers (write paths) from change
 * consumers (replication, audit, analytics) via a pub/sub model backed by
 * a bounded in-memory event log.
 *
 * ### Subscription lifecycle
 * 1. Call `subscribeToChanges(table_name, callback)` → returns a `sub_id`.
 * 2. Events matching `table_name` (or all tables if `table_name` is empty)
 *    are delivered synchronously to `callback` from inside `publishEvent`.
 * 3. Call `unsubscribe(sub_id)` to remove the subscription.
 *
 * ### Event log
 * All published events are appended to a ring-buffer with capacity
 * `max_log_size` (default 65536).  `replayChanges()` returns matching
 * events from this log.  Events older than the ring-buffer window are lost.
 *
 * ### Ring-buffer overflow semantics
 *
 * The ring-buffer operates with **OVERWRITE** (oldest-eviction) policy:
 *
 *   - The buffer is a circular deque (`std::deque`) capped at `max_log_size`
 *     entries.
 *   - When a new event arrives and the buffer is at capacity, the **oldest**
 *     event (front of the deque) is silently discarded to make room.
 *   - There is no back-pressure on the caller, no blocking, and no error
 *     is returned from `publishEvent()`.
 *   - Consumers that require guaranteed delivery MUST use the subscription
 *     API (`subscribeToChanges`) rather than `replayChanges()`, because
 *     in-flight callbacks are invoked *before* the event is appended —
 *     i.e. they are unaffected by overflow.
 *   - The overflow count (total events lost since construction) is available
 *     via `overflowCount()`.
 *
 * **Capacity guidance**:
 *   - Default capacity 65 536 events ≈ 8–16 MiB depending on payload size.
 *   - Increase `max_log_size` in the constructor for replay-heavy workloads.
 *   - A future version will support BLOCK and DROP policies for strict
 *     back-pressure (see `include/temporal/ROADMAP.md`, CDC v1.8.0 items).
 *
 * ### Thread-safety
 * All public methods are thread-safe.  Callbacks are invoked under a
 * shared lock so they must not call `subscribeToChanges` or `unsubscribe`
 * (doing so would deadlock).
 */
class TemporalCDC {
public:
    static constexpr size_t kDefaultMaxLogSize = 65536;

    /**
     * Construct a CDC instance.
     *
     * @param max_log_size  Ring-buffer capacity (number of events).
     * @param policy        What to do when the buffer is full.
     *                      Defaults to OVERWRITE (circular eviction).
     */
    explicit TemporalCDC(size_t max_log_size = kDefaultMaxLogSize,
                         OverflowPolicy policy = OverflowPolicy::OVERWRITE);

    // Non-copyable; movable
    TemporalCDC(const TemporalCDC&)            = delete;
    TemporalCDC& operator=(const TemporalCDC&) = delete;
    TemporalCDC(TemporalCDC&&)                 = default;
    TemporalCDC& operator=(TemporalCDC&&)      = default;

    ~TemporalCDC() = default;

    // ── Subscription management ───────────────────────────────────────────────

    /**
     * Subscribe to change events for a specific table.
     *
     * @param table_name  Table to monitor.  Pass an empty string to receive
     *                    events for ALL tables.
     * @param callback    Invoked synchronously for each matching event.
     * @return            Subscription ID for use with unsubscribe().
     */
    std::string subscribeToChanges(
        const std::string& table_name,
        std::function<void(const ChangeEvent&)> callback);

    /**
     * Cancel a subscription.
     * @return true if the subscription was found and removed.
     */
    bool unsubscribe(const std::string& sub_id);

    /** Return the number of active subscriptions. */
    size_t subscriptionCount() const;

    // ── Event emission ────────────────────────────────────────────────────────

    /**
     * Publish a change event.
     *
     * 1. Appends the event to the in-process log (evicting the oldest event
     *    if the ring-buffer is full).
     * 2. Invokes all matching subscribers synchronously.
     *
     * This method is intended to be called from write paths (insert/update/
     * delete) inside the temporal module.
     */
    void publishEvent(const ChangeEvent& event);

    // ── Replay ───────────────────────────────────────────────────────────────

    /**
     * Replay historical change events from the in-process log.
     *
     * Returns events whose `transaction_time` falls in the half-open interval
     * [range.start, range.end) and whose `table_name` matches.
     * Pass an empty `table_name` to replay events for all tables.
     *
     * @note Only events retained in the ring-buffer are available.
     *       Events evicted due to log overflow are permanently lost.
     */
    std::vector<ChangeEvent> replayChanges(
        const std::string& table_name,
        const TimeRange& range) const;

    /**
     * Return the total number of events in the log (≤ max_log_size).
     */
    size_t logSize() const;

    /**
     * Return the total number of events ever published (monotonically
     * increasing, wraps on overflow).
     */
    uint64_t totalPublished() const noexcept;

    /**
     * Return the total number of events that have been silently discarded
     * due to ring-buffer overflow since construction.
     *
     * An overflow occurs when `publishEvent()` is called while the in-process
     * log already holds `max_log_size` events.  The oldest event is evicted
     * (OVERWRITE policy) and this counter is incremented.
     *
     * A non-zero value indicates that `replayChanges()` may no longer return
     * the complete history.  Consumers that require guaranteed delivery should
     * use the subscription API instead.
     */
    uint64_t overflowCount() const noexcept;

    /** Clear the in-process event log.  Active subscriptions are unaffected. */
    void clearLog();

    /**
     * Return the number of events dropped or overwritten due to ring-buffer
     * overflow since this CDC instance was constructed.
     *
     * - Under OVERWRITE policy: incremented each time the oldest entry is
     *   evicted to make room for a new event.
     * - Under DROP policy: incremented each time a new event is discarded
     *   because the buffer is full.
     *
     * This counter is monotonically increasing and never resets automatically.
     * It gives callers a lightweight way to detect back-pressure without
     * inspecting individual events.
     */
    uint64_t overflowCount() const noexcept;

    // ── Public Helpers ────────────────────────────────────────────────────────

    /** Convert ChangeType enum to string representation. */
    static std::string changeTypeName(ChangeType ct);

    /** Convert string to ChangeType enum. */
    static ChangeType changeTypeFromString(const std::string& s);

private:
    // ── Subscription entry ────────────────────────────────────────────────────

    struct Subscription {
        std::string sub_id;
        std::string table_filter;  ///< Empty = match all tables
        std::function<void(const ChangeEvent&)> callback;
    };

    // ── State ─────────────────────────────────────────────────────────────────

    size_t                       max_log_size_;
    OverflowPolicy               overflow_policy_;
    std::vector<ChangeEvent>     log_;         ///< Ring-buffer (front = oldest)
    std::atomic<uint64_t>        total_published_{0};
    std::atomic<uint64_t>        overflow_count_{0};  ///< Events evicted by OVERWRITE policy

    std::unordered_map<std::string, Subscription> subscriptions_;

    mutable std::mutex mutex_;
    std::atomic<uint64_t> next_sub_id_{0};
};

// ============================================================================
// CDCPersistentLog
// ============================================================================

/**
 * @brief Persistent, WAL-backed log for CDC events.
 *
 * `CDCPersistentLog` durably stores `ChangeEvent` instances in append-only
 * WAL segment files under a caller-supplied directory.  Segment files are
 * named `cdc_wal_XXXXXXXX.seg` and are rotated when a configurable byte
 * threshold is reached (default: 64 MiB).
 *
 * ### Record format (binary, little-endian)
 * ```
 * [uint32_t crc32][uint32_t payload_len][payload_len bytes of JSON]
 * ```
 * The CRC-32 covers the raw payload bytes.  On replay, records whose CRC
 * does not match are skipped and counted via `corruptedSegmentCount()`.
 *
 * ### Usage
 * ```cpp
 * CDCPersistentLog log;
 * log.open("/var/lib/themisdb/cdc");
 * log.append(ev);
 * auto events = log.replay("employees", {t0, t1});
 * log.close();
 * ```
 *
 * ### Thread-safety
 * All public methods are thread-safe.
 */
class CDCPersistentLog {
public:
    /// Default maximum segment size before rotation (64 MiB).
    static constexpr uint64_t kDefaultSegmentMaxBytes = 64ULL * 1024 * 1024;

    struct Config {
        std::string directory;
        uint64_t    segment_max_bytes{64ULL * 1024 * 1024}; ///< == kDefaultSegmentMaxBytes
    };

    /// Construct with default configuration (no directory set; call open() after setting directory).
    CDCPersistentLog();

    /// Construct with explicit configuration.
    explicit CDCPersistentLog(Config cfg);
    ~CDCPersistentLog();

    CDCPersistentLog(const CDCPersistentLog&)            = delete;
    CDCPersistentLog& operator=(const CDCPersistentLog&) = delete;
    CDCPersistentLog(CDCPersistentLog&&)                 = default;
    CDCPersistentLog& operator=(CDCPersistentLog&&)      = default;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /**
     * Open the WAL in the configured directory.
     * Creates the directory if it does not exist.
     * Determines the next segment index from existing `.seg` files.
     *
     * @throws std::runtime_error if the directory cannot be created or the
     *         segment file cannot be opened for writing.
     */
    void open();

    /** Close the current segment file and release resources. */
    void close() noexcept;

    /** Return true if the log has been successfully opened. */
    [[nodiscard]] bool isOpen() const noexcept;

    // ── Write path ────────────────────────────────────────────────────────────

    /**
     * Append a change event to the WAL.
     * Rotates the current segment if it would exceed `segment_max_bytes`.
     *
     * @throws std::runtime_error on disk-full or I/O error.
     * @throws std::logic_error   if the log is not open.
     */
    void append(const ChangeEvent& event);

    // ── Read path ─────────────────────────────────────────────────────────────

    /**
     * Replay events from all WAL segments.
     *
     * Filters by `table_name` (empty = all tables) and by
     * `transaction_time ∈ [range.start, range.end)`.  Segments with CRC
     * errors are counted but the valid records within them are still returned.
     *
     * @return Matching events in the order they were written.
     */
    [[nodiscard]] std::vector<ChangeEvent> replay(
        const std::string& table_name,
        const TimeRange&   range) const;

    // ── Diagnostics ───────────────────────────────────────────────────────────

    /** Number of records that failed CRC validation during replay. */
    [[nodiscard]] uint32_t corruptedRecordCount() const noexcept;

    /** Number of WAL segment files in the directory. */
    [[nodiscard]] size_t segmentCount() const;

    /** Total bytes appended since the log was last opened. */
    [[nodiscard]] uint64_t totalBytesWritten() const noexcept;

private:
    // ── Helpers ───────────────────────────────────────────────────────────────

    static uint32_t crc32(const uint8_t* data, size_t len) noexcept;
    void rotateSegment();
    [[nodiscard]] std::string segmentPath(size_t idx) const;

    // ── State ─────────────────────────────────────────────────────────────────

    Config                   cfg_;
    bool                     is_open_{false};
    size_t                   current_segment_{0};
    uint64_t                 current_segment_bytes_{0};
    std::unique_ptr<std::ofstream> current_file_;
    uint64_t                 total_bytes_written_{0};
    mutable uint32_t         corrupted_count_{0};
    mutable std::mutex       mutex_;
};

} // namespace temporal
} // namespace themisdb

// ============================================================================
// CDCPersistentLog  (v1.8.0)
// ============================================================================

/**
 * @brief Append-only, WAL-backed persistent CDC log.
 *
 * `CDCPersistentLog` implements the `CDCListener` interface (via
 * `TemporalCDC::subscribeToChanges`) and durably persists each `ChangeEvent`
 * to a binary append-only Write-Ahead Log (WAL) on the file-system.
 *
 * ## WAL segment format
 *
 * Each segment is an append-only binary file named
 * `<segment_dir>/<prefix>_<segment_seq>.wal`.  A segment header is written
 * once at creation time, followed by zero or more fixed-layout records:
 *
 * ```
 * Segment file layout
 * ───────────────────
 * [HEADER]  magic(4) | version(2) | segment_seq(8) | created_at_ms(8)
 * [RECORD]* payload_len(4) | crc32(4) | payload(<payload_len bytes JSON)
 * ```
 *
 * `magic` = 0x54444357  ('TDCW' in little-endian)
 * `version` = 0x0100 (major=1, minor=0)
 *
 * A new segment is created automatically when the current segment reaches or
 * exceeds `max_segment_bytes` (default 64 MiB).
 *
 * ## CRC-32 validation
 *
 * Each record carries a CRC-32/ISO-HDLC checksum of the JSON payload bytes.
 * On open (`open()`), the log scans all records in the segment directory and
 * discards any tail record whose CRC does not match — providing truncation
 * recovery without data loss on abrupt shutdown.
 *
 * ## Thread-safety
 *
 * All public methods are thread-safe.
 *
 * ## Usage
 * ```cpp
 * CDCPersistentLog wal("/var/lib/themisdb/cdc", "orders");
 * wal.open();
 *
 * TemporalCDC cdc;
 * cdc.subscribeToChanges("orders", [&](const ChangeEvent& ev) {
 *     wal.append(ev);
 * });
 *
 * // Replay all durable events:
 * auto events = wal.replayAll();
 * wal.close();
 * ```
 */
class CDCPersistentLog {
public:
    /** Byte capacity at which a segment is rotated. Default: 64 MiB. */
    static constexpr uint64_t kDefaultMaxSegmentBytes = 64ULL * 1024 * 1024;

    /**
     * Construct a CDCPersistentLog.
     *
     * @param segment_dir       Directory in which WAL segments are stored.
     *                          Created automatically if it does not exist.
     * @param log_prefix        Prefix for segment file names (e.g. "cdc" or
     *                          a table name).  Must not be empty.
     * @param max_segment_bytes Rotate to a new segment after this many bytes.
     */
    explicit CDCPersistentLog(std::string segment_dir,
                               std::string log_prefix = "cdc",
                               uint64_t max_segment_bytes = kDefaultMaxSegmentBytes);

    ~CDCPersistentLog();

    // Non-copyable, non-movable (file handles)
    CDCPersistentLog(const CDCPersistentLog&)            = delete;
    CDCPersistentLog& operator=(const CDCPersistentLog&) = delete;

    /**
     * Open the log: scan existing segments in `segment_dir`, recover any
     * truncated tail record, and position the write head at the first valid
     * segment for appending.
     *
     * Must be called exactly once before `append()`.
     * Idempotent: calling open() again after close() re-opens the log.
     *
     * @throws std::runtime_error  on I/O errors during directory creation or
     *                             segment scanning.
     */
    void open();

    /**
     * Flush and close the current segment file handle.
     * Safe to call multiple times.
     */
    void close();

    /**
     * Append a ChangeEvent to the persistent log.
     *
     * The event is JSON-serialised, CRC-32 checked, and written atomically
     * (length-prefixed) to the current segment.  If the segment exceeds
     * `max_segment_bytes` after the write, a new segment is rotated.
     *
     * @throws std::runtime_error  when called before `open()`, or on I/O
     *                             errors.
     */
    void append(const ChangeEvent& event);

    /**
     * Replay all persisted events from all segments in chronological order.
     *
     * Records with invalid CRC are silently skipped (truncation recovery).
     * This is a read-only scan and can be called concurrently with `append()`.
     *
     * @throws std::runtime_error  on segment directory I/O errors.
     */
    std::vector<ChangeEvent> replayAll() const;

    /**
     * Replay events from a specific segment (by 0-based sequence number).
     *
     * @param segment_seq  Segment sequence number.
     * @throws std::out_of_range   when @p segment_seq >= segmentCount().
     * @throws std::runtime_error  on I/O errors.
     */
    std::vector<ChangeEvent> replaySegment(uint64_t segment_seq) const;

    /** Number of WAL segments that have been created (including active). */
    uint64_t segmentCount() const noexcept;

    /** Total bytes written to all segments (approximation, not fsynced). */
    uint64_t totalBytesWritten() const noexcept;

    /** Total events successfully appended since open(). */
    uint64_t totalEventsAppended() const noexcept;

    /** true when the log is currently open for writing. */
    bool isOpen() const noexcept;

private:
    // ── WAL constants ─────────────────────────────────────────────────────────
    static constexpr uint32_t kMagic   = 0x54444357u;  // 'TDCW'
    static constexpr uint16_t kVersion = 0x0100u;

    // ── State ─────────────────────────────────────────────────────────────────
    std::string  segment_dir_;
    std::string  log_prefix_;
    uint64_t     max_segment_bytes_;

    mutable std::mutex mutex_;

    // Active segment
    std::FILE*   active_fd_{nullptr};
    uint64_t     active_seq_{0};       ///< Sequence number of active segment
    uint64_t     active_bytes_{0};     ///< Bytes written to active segment
    bool         is_open_{false};

    std::atomic<uint64_t> total_events_{0};
    std::atomic<uint64_t> total_bytes_{0};

    // ── Private helpers ───────────────────────────────────────────────────────

    /** Build the full path for a segment with the given sequence number. */
    std::string segmentPath(uint64_t seq) const;

    /** Scan segment_dir_ for existing .wal files and return sorted seqs. */
    std::vector<uint64_t> listSegmentSeqs() const;

    /** Write the 22-byte segment header to @p fd. */
    static void writeSegmentHeader(std::FILE* fd, uint64_t seq);

    /** Validate the header of a segment file; return true on success. */
    static bool validateSegmentHeader(std::FILE* fd);

    /** Compute CRC-32/ISO-HDLC of @p data. */
    static uint32_t crc32(const std::string& data) noexcept;

    /** Rotate: close active segment, increment seq, open new segment. */
    void rotate();
};

