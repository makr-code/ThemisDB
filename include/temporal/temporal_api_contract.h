/**
 * @file temporal_api_contract.h
 * @brief Frozen temporal runtime contract semantics for the active v1.x line.
 *
 * This header defines the normative contract for the temporal module covering
 * bi-temporal data management, interval-tree indexing, snapshot reads, data
 * retention, and point-in-time recovery (PITR).
 *
 * ## Contract Scope
 *
 * The contracts below are binding for all components in the ThemisDB temporal
 * pipeline:
 *   - Bi-temporal insert / query (valid_time, transaction_time)
 *   - Interval tree index (insert, query, delete complexity)
 *   - Snapshot manager (consistent read at timestamp T)
 *   - Retention manager (soft-delete, GC boundary)
 *   - PITR restore (exact committed state recovery)
 *
 * ## Versioning
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump
 * with migration notes and a CHANGELOG entry.
 *
 * @see src/temporal/ROADMAP.md  — Phase 1 contract item
 * @see include/temporal/ROADMAP.md — Phase 6 documentation item
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace themis {
namespace temporal {

// ============================================================================
// § 1  Bi-temporal model contract
//
// Independence guarantee:
//   valid_time and transaction_time are independent dimensions.  A row's
//   valid_time range represents the period during which the fact is true in the
//   modelled world.  transaction_time represents when the database recorded the
//   fact.  Neither dimension is ever null; storage of a null in either column
//   is a contract violation.
//
// Overlap semantics:
//   Two bi-temporal rows overlap when their valid_time intervals intersect AND
//   their transaction_time intervals intersect.  The query engine must surface
//   all overlapping rows for a given (valid_time, transaction_time) probe point.
// ============================================================================

/// Sentinel value for "open" end of a bi-temporal interval (maximum epoch ns).
inline constexpr std::int64_t kTemporalOpenEnd = INT64_MAX;

/// Minimum valid_time / transaction_time value accepted (Unix epoch, ns).
inline constexpr std::int64_t kTemporalMinTimestampNs = 0LL;

// ============================================================================
// § 2  Interval tree index contract
//
// Complexity guarantees:
//   - insert():  amortised O(log n)
//   - query():   O(log n + k) where k = result count
//   - delete():  amortised O(log n)
//
// No stale reads after delete:
//   After delete(key) returns successfully, any subsequent query() for that
//   key's interval must NOT return the deleted row.  Lazy deletion strategies
//   that allow temporary stale reads are prohibited.
// ============================================================================

/// Maximum interval tree depth before a rebalance is forced.
inline constexpr std::size_t kIntervalTreeMaxDepth = 64;

// ============================================================================
// § 3  Snapshot contract
//
// Consistency guarantee:
//   snapshot_at(T) returns all rows whose valid_time interval contains T
//   (i.e., valid_start ≤ T ≤ valid_end) and whose transaction_time interval
//   also contains T.  The result set is a point-in-time consistent view; no
//   write committed after the snapshot is requested is visible.
//
// Concurrent write isolation:
//   A write that begins after snapshot_at(T) is requested must not affect the
//   snapshot result even if it commits before the snapshot read completes.
// ============================================================================

// ============================================================================
// § 4  Retention contract
//
// Soft-delete semantics:
//   Rows whose valid_time end has passed the retention boundary are soft-deleted
//   by setting a deletion marker.  The underlying row data is NOT physically
//   removed until the garbage collector (GC) runs.
//
// GC boundary:
//   GC MUST NOT remove any row whose retention boundary has not yet been
//   reached, regardless of storage pressure.  Early physical deletion is a
//   contract violation.
// ============================================================================

// ============================================================================
// § 5  PITR contract
//
// Exact state recovery:
//   restore_to(T) returns the exact committed database state as of timestamp T.
//   It returns no more (no extra rows that were not committed at T) and no less
//   (no rows missing that were committed at or before T).
//
// Timestamp range:
//   Requests for a T before the oldest available WAL/snapshot anchor return
//   PITR_TIMESTAMP_BEFORE_OLDEST.  No partial restore is attempted.
//
// Future timestamp:
//   Requests for a T in the future (T > now) also return
//   PITR_TIMESTAMP_BEFORE_OLDEST (i.e., the error is the same as requesting an
//   unavailable anchor, not a distinct "future timestamp" error).
// ============================================================================

// ============================================================================
// § 6  Error taxonomy
// ============================================================================

/**
 * @brief Canonical temporal error codes.
 *
 * Codes in range [300, 399] are temporal-specific.
 */
enum class TemporalErrorCode : int {
    /// valid_time or transaction_time range is invalid (start > end, null, etc).
    TEMPORAL_RANGE_INVALID        = 300,

    /// Requested snapshot timestamp is older than the retention horizon.
    SNAPSHOT_EXPIRED              = 301,

    /// Retention policy change would conflict with an existing retention window.
    RETENTION_POLICY_CONFLICT     = 302,

    /// PITR restore timestamp is before the oldest available recovery point.
    PITR_TIMESTAMP_BEFORE_OLDEST  = 303,

    /// Interval tree operation failed due to internal state inconsistency.
    INTERVAL_TREE_INCONSISTENT    = 304,

    /// Bi-temporal row insert would create a null valid_time or transaction_time.
    NULL_TEMPORAL_DIMENSION       = 305,

    /// Unclassified temporal internal error; always treated as hard error.
    INTERNAL_ERROR                = 399,
};

/**
 * @brief Returns true when the error code is a non-retryable hard error.
 */
[[nodiscard]] inline constexpr bool isHardTemporalError(TemporalErrorCode code) noexcept {
    return code == TemporalErrorCode::TEMPORAL_RANGE_INVALID
        || code == TemporalErrorCode::NULL_TEMPORAL_DIMENSION
        || code == TemporalErrorCode::INTERVAL_TREE_INCONSISTENT
        || code == TemporalErrorCode::INTERNAL_ERROR;
}

/**
 * @brief Returns true when the error code is a recoverable lifecycle error
 *        (e.g., expired snapshot, PITR anchor unavailable).
 */
[[nodiscard]] inline constexpr bool isLifecycleError(TemporalErrorCode code) noexcept {
    return code == TemporalErrorCode::SNAPSHOT_EXPIRED
        || code == TemporalErrorCode::RETENTION_POLICY_CONFLICT
        || code == TemporalErrorCode::PITR_TIMESTAMP_BEFORE_OLDEST;
}

// ============================================================================
// § 7  Retention limits
// ============================================================================

/// Default retention period in seconds (90 days).
inline constexpr std::int64_t kDefaultRetentionSeconds = 90LL * 24 * 3600;

/// Minimum allowed retention period in seconds (1 hour).
inline constexpr std::int64_t kMinRetentionSeconds = 3600LL;

/// Maximum number of retention policies active simultaneously per table.
inline constexpr std::size_t kMaxRetentionPoliciesPerTable = 16;

} // namespace temporal
} // namespace themis
