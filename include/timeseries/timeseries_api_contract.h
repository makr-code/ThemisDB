/**
 * @file timeseries_api_contract.h
 * @brief Timeseries runtime contract semantics.
 *
 * This header defines the normative contract for the timeseries module covering
 * write ordering, range-query semantics, downsampling determinism, Gorilla
 * compression fidelity, and retention policy enforcement. TSStore timestamps
 * and range bounds are signed Unix epoch milliseconds, matching tsstore.h and
 * the persisted key format; no implicit nanosecond conversion is performed.
 *
 * ## Contract Scope
 *
 * The contracts below are binding for all components in the ThemisDB timeseries
 * pipeline:
 *   - Ingest path (TSStore write, hypertable routing)
 *   - Range-query engine (inclusive bounds, retention awareness)
 *   - Downsampling engine (deterministic output count)
 *   - Gorilla compression codec (lossless IEEE 754 round-trip)
 *   - Retention engine (policy-driven expiry, no early removal)
 *
 * ## Versioning
 *
 * This contract remains stable within v1.x. TSStore's concrete timestamp and
 * ordering behavior is documented here from the implementation: signed epoch
 * milliseconds and configurable late-arrival validation. The former
 * nanosecond/strict-order declarations were not implemented by TSStore. The
 * legacy nanosecond constants remain deprecated for source compatibility and
 * do not constrain TSStore values.
 *
 * @see src/timeseries/ROADMAP.md  — Phase 1 contract item
 * @see include/timeseries/ROADMAP.md — Phase 6 documentation item
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <climits>

namespace themis {
namespace timeseries {

// ============================================================================
// § 1  Write contract
//
// Units:
//   TSStore::DataPoint::timestamp_ms is a signed Unix epoch timestamp in
//   milliseconds. TSStore range-query bounds, Gorilla timestamps, and
//   late_arrival_window_ms use the same millisecond unit. The store does not
//   convert timestamps or reject zero/negative values solely because of sign.
//
// Ordering:
//   TSStore::Config::late_arrival_window_ms controls ordering validation for
//   putDataPoint(), putDataPoints(), and putBatch(). A non-positive value disables the
//   per-instance watermark check and accepts timestamps in any order. A
//   positive value W initializes a per-series watermark on the first write,
//   advances it only when a larger timestamp arrives, accepts timestamps at
//   or above watermark-W (including equality with the watermark), and rejects
//   smaller values with ERR_TIMESERIES_LATE_ARRIVAL. Accepted older points do
//   not move the watermark. This is bounded late-arrival handling, not strict
//   monotonic ordering; duplicate timestamps are not rejected by this check.
//   Watermarks are in memory and are not persisted across TSStore instances.
// ============================================================================

/// Maximum signed millisecond timestamp; also used as an open-ended range sentinel.
inline constexpr std::int64_t kMaxPointTimestampMs = INT64_MAX;

/// @deprecated Not a TSStore timestamp constraint; use millisecond units.
[[deprecated("TSStore timestamps use milliseconds; this nanosecond constant does not apply")]]
inline constexpr std::int64_t kMinPointTimestampNs = 1LL;

/// @deprecated Not a TSStore timestamp constraint; use kMaxPointTimestampMs.
[[deprecated("TSStore timestamps use milliseconds; use kMaxPointTimestampMs")]]
inline constexpr std::int64_t kMaxPointTimestampNs = INT64_MAX;

/// Out-of-order staging-buffer threshold used by TsEdgeCaseHandler, not TSStore.
inline constexpr std::size_t kMaxOutOfOrderBuffer = 10'000u;

// ============================================================================
// § 2  Range-query contract
//
// Inclusive bounds:
//   A range query for [start, end] returns ALL data points P such that
//   start ≤ P.timestamp ≤ end.  The bounds are inclusive on both sides.
//
// Empty result:
//   A query for a range that contains no points returns an empty result set,
//   NOT an error.  SERIES_NOT_FOUND is returned only when the named series
//   does not exist.
//
// Retention boundary:
//   Points whose timestamp is before the active retention boundary MAY have
//   been removed by the retention engine.  A range query that spans the
//   retention boundary returns only the points still present; it does not
//   raise an error.
// ============================================================================

// ============================================================================
// § 3  Downsampling contract
//
// Determinism:
//   For a given input series (same points, same order) and a given resolution,
//   the downsampler MUST produce the same number of output buckets and the same
//   aggregate values.  Non-deterministic downsampling is a contract violation.
//
// Empty input:
//   Downsampling an empty input series returns an empty output, not an error.
//
// Single-point passthrough:
//   A series with exactly one point, regardless of resolution, returns that
//   single point unchanged.
//
// Resolution validity:
//   A resolution of 0 or negative causes DOWNSAMPLING_RESOLUTION_INVALID.
// ============================================================================

/// Minimum accepted downsampling resolution in nanoseconds (1 µs).
inline constexpr std::int64_t kMinDownsamplingResolutionNs = 1'000LL;

// ============================================================================
// § 4  Gorilla compression contract
//
// Lossless round-trip:
//   encode(v) followed by decode() MUST reproduce the exact IEEE 754 binary64
//   representation of v for any finite or non-finite double value, including
//   positive/negative zero, subnormals, NaN (any payload), and ±Inf.
//
// NaN preservation:
//   NaN values are encoded and decoded with the same bit pattern (NaN payload
//   preserved).  Silent NaN canonicalization is prohibited.
//
// Inf preservation:
//   ±Inf values are encoded and decoded identically.
// ============================================================================

// ============================================================================
// § 5  Retention contract
//
// Policy-driven expiry:
//   Data points are eligible for removal only after their timestamp is older
//   than the configured retention boundary (now − retention_duration).  No
//   point is removed before that boundary regardless of storage pressure.
//
// No premature removal:
//   A point whose timestamp is within the retention window MUST be readable
//   until the retention engine's next scheduled GC pass moves it outside the
//   window.  Synchronous write-path removal is prohibited.
// ============================================================================

// ============================================================================
// § 6  Error taxonomy
// ============================================================================

/**
 * @brief Canonical timeseries error codes.
 *
 * Codes in range [400, 499] are timeseries-specific.
 * Codes in range [400, 419] are Phase 1-2 error codes.
 * Codes in range [410, 445] are Phase 3 error codes (new in Phase 3).
 *
 * Phase 3 adds incident-specific codes for:
 *   - Ingest path: BUFFER_PRESSURE, BUFFER_OVERFLOW, FLUSH_TIMEOUT, etc.
 *   - Query path: RANGE_INVALID, RETENTION_BOUNDARY_CROSSED, QUERY_TIMEOUT, etc.
 *   - Lifecycle path: RETENTION_POLICY_VIOLATION, DELETION_FAILED, ENCRYPTION_ROTATION_FAILURE, etc.
 *   - Integration path: REMOTE_WRITE_FAILURE, REMOTE_WRITE_VALIDATION_ERROR, etc.
 *
 * @see include/timeseries/timeseries_incident_taxonomy.h — Phase 3 incident taxonomy
 */
enum class TimeseriesErrorCode : int {
    // ── Phase 1-2: Base error codes ──
    
    /// Contract-model timestamp ordering error; TSStore reports its own
    /// ERR_TIMESERIES_LATE_ARRIVAL error through Result.
    TIMESTAMP_OUT_OF_ORDER        = 400,

    /// Named series does not exist or has been dropped.
    SERIES_NOT_FOUND              = 401,

    /// Data point is beyond the retention boundary and has been removed.
    RETENTION_EXPIRED             = 402,

    /// Gorilla (or other codec) encountered an unrecoverable encode/decode error.
    COMPRESSION_FAILED            = 403,

    /// Downsampling resolution is zero or negative.
    DOWNSAMPLING_RESOLUTION_INVALID = 404,

    /// Requested series operation would exceed the per-series or global quota.
    QUOTA_EXCEEDED                = 405,

    // ── Phase 3: Ingest incident codes ──

    /// Buffer capacity reached; backpressure applied to writer.
    BUFFER_PRESSURE               = 411,

    /// Write queue is overflowing despite backpressure.
    BUFFER_OVERFLOW               = 412,

    /// Flush operation failed to complete within timeout.
    FLUSH_TIMEOUT                 = 413,

    // ── Phase 3: Query incident codes ──

    /// Query range is invalid (e.g., start > end).
    RANGE_INVALID                 = 421,

    /// Query execution exceeded timeout.
    QUERY_TIMEOUT                 = 423,

    /// Data outside retention boundary may be missing.
    RETENTION_BOUNDARY_CROSSED    = 422,

    // ── Phase 3: Lifecycle incident codes ──

    /// Retention policy violation detected.
    RETENTION_POLICY_VIOLATION    = 431,

    /// Disk error during safe deletion.
    DELETION_FAILED               = 432,

    /// Key rotation failed; encryption state invalid.
    ENCRYPTION_ROTATION_FAILURE   = 433,

    /// Encryption key not found for decryption.
    ENCRYPTION_KEY_NOT_FOUND      = 434,

    /// Encryption state validation failed.
    ENCRYPTION_STATE_INVALID      = 435,

    // ── Phase 3: Integration incident codes ──

    /// Remote write validation failed.
    REMOTE_WRITE_VALIDATION_ERROR = 443,

    /// Retry limit exceeded for remote-write operation.
    REMOTE_WRITE_RETRIES_EXHAUSTED = 444,

    /// Unclassified timeseries internal error.
    INTERNAL_ERROR                = 499,
};

/**
 * @brief Returns true when the error code is a non-retryable hard error.
 */
[[nodiscard]] inline constexpr bool isHardTimeseriesError(TimeseriesErrorCode code) noexcept {
    return code == TimeseriesErrorCode::TIMESTAMP_OUT_OF_ORDER
        || code == TimeseriesErrorCode::COMPRESSION_FAILED
        || code == TimeseriesErrorCode::DOWNSAMPLING_RESOLUTION_INVALID
        || code == TimeseriesErrorCode::RANGE_INVALID
        || code == TimeseriesErrorCode::ENCRYPTION_KEY_NOT_FOUND
        || code == TimeseriesErrorCode::ENCRYPTION_STATE_INVALID
        || code == TimeseriesErrorCode::INTERNAL_ERROR;
}

/**
 * @brief Returns true when the error code indicates a data-lifecycle condition
 *        (series not found, retention expired) that the caller should handle
 *        distinctly from hard errors.
 */
[[nodiscard]] inline constexpr bool isLifecycleError(TimeseriesErrorCode code) noexcept {
    return code == TimeseriesErrorCode::SERIES_NOT_FOUND
        || code == TimeseriesErrorCode::RETENTION_EXPIRED
        || code == TimeseriesErrorCode::RETENTION_BOUNDARY_CROSSED;
}

/**
 * @brief Returns true when the error code indicates backpressure that writer should handle.
 *
 * Backpressure errors indicate buffer/queue congestion. Writers should
 * implement exponential backoff or rate limiting.
 */
[[nodiscard]] inline constexpr bool isBackpressureError(TimeseriesErrorCode code) noexcept {
    return code == TimeseriesErrorCode::BUFFER_PRESSURE
        || code == TimeseriesErrorCode::BUFFER_OVERFLOW;
}

/**
 * @brief Returns true when the error code is potentially retryable.
 *
 * Transient errors include buffer congestion, timeouts, and remote errors.
 */
[[nodiscard]] inline constexpr bool isTransientError(TimeseriesErrorCode code) noexcept {
    return code == TimeseriesErrorCode::BUFFER_PRESSURE
        || code == TimeseriesErrorCode::BUFFER_OVERFLOW
        || code == TimeseriesErrorCode::FLUSH_TIMEOUT
        || code == TimeseriesErrorCode::QUERY_TIMEOUT
        || code == TimeseriesErrorCode::REMOTE_WRITE_RETRIES_EXHAUSTED;
}

// ============================================================================
// § 7  Resource limits
// ============================================================================

/// Maximum number of series in a single TSStore instance.
inline constexpr std::size_t kMaxSeriesPerStore = 10'000'000u;

/// Maximum number of data points per write batch.
inline constexpr std::size_t kMaxWriteBatchSize = 100'000u;

/// Default retention period in seconds (30 days).
inline constexpr std::int64_t kDefaultRetentionSeconds = 30LL * 24 * 3600;

} // namespace timeseries
} // namespace themis
