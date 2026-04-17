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

    explicit TemporalCDC(size_t max_log_size = kDefaultMaxLogSize);

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
    std::vector<ChangeEvent>     log_;         ///< Ring-buffer (front = oldest)
    std::atomic<uint64_t>        total_published_{0};
    std::atomic<uint64_t>        overflow_count_{0};  ///< Events evicted by OVERWRITE policy

    std::unordered_map<std::string, Subscription> subscriptions_;

    mutable std::mutex mutex_;
    std::atomic<uint64_t> next_sub_id_{0};
};

} // namespace temporal
} // namespace themisdb
