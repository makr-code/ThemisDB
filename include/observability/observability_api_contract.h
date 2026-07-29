/*
 * ThemisDB | File: observability_api_contract.h | Version: 1.0.0
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY | Status: Phase 1 — Frozen Contract
 * Purpose: Frozen observability (metrics/tracing/logging/SLO/export) contract semantics for v1.x.
 */

/**
 * @file observability_api_contract.h
 * @brief Frozen observability contract for counter/gauge/histogram, tracing, logging, SLO, and export.
 *
 * This header defines the normative contract for the observability module.
 * All observability components (metrics collectors, tracers, loggers, SLO reporters,
 * and exporters) must honour the semantics defined here within the v1.x major line.
 *
 * ## Contract Scope
 *
 *   - MetricsCollector (metrics_collector.h) — counter/gauge/histogram semantics
 *   - OpenTelemetryTracer (opentelemetry_tracer.h) — span parent-child linkage
 *   - LogAggregator (log_aggregator.h) — structured log immutability
 *   - SloReporter (slo_reporter.h) — SLO measurement window closure
 *   - Prometheus/OTLP exporters — push/pull protocol contract
 *
 * ## Metrics Contract
 *
 * Counters MUST be monotonically non-decreasing.  Any reset must be explicit
 * and communicated to consumers via a `counter_reset` annotation.
 * Gauges may move in both directions.  Histogram bucket boundaries are
 * immutable after the histogram is created.
 *
 * ## Tracing Contract
 *
 * Span parent-child relationships are deterministic: a child span's
 * `parent_span_id` always matches an existing ancestor's `span_id` within
 * the same trace.  Spans are not lost under normal load (non-degraded path).
 * Dropped spans (overload path) surface the TRACE_SPAN_DROPPED error code.
 *
 * ## Logging Contract
 *
 * Structured log entries are immutable after being written.  Fields must
 * not be mutated, reordered, or truncated post-write.
 *
 * ## SLO Contract
 *
 * An SLO measurement window is closed (sealed) at its configured boundary.
 * Late updates after window closure are rejected with SLO_WINDOW_INVALID.
 *
 * ## Versioning
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump.
 *
 * @see src/observability/ROADMAP.md — Phase 1 item
 * @see include/observability/metrics_collector.h
 * @see include/observability/opentelemetry_tracer.h
 * @see include/observability/slo_reporter.h
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace themis {
namespace observability {

// ============================================================================
// § 1  Metric constraints
// ============================================================================

/// Maximum number of labels (key-value pairs) allowed per metric time series.
inline constexpr std::size_t kMaxMetricLabels = 32;

/// Maximum label key length in bytes.
inline constexpr std::size_t kMaxLabelKeyBytes = 128;

/// Maximum label value length in bytes.
inline constexpr std::size_t kMaxLabelValueBytes = 256;

/// Maximum cardinality (distinct label-set combinations) per metric family.
inline constexpr std::size_t kMaxMetricCardinality = 100'000;

// ============================================================================
// § 2  Histogram contract
// ============================================================================

/// Maximum number of explicit bucket boundaries per histogram.
inline constexpr std::size_t kMaxHistogramBuckets = 256;

/// Histogram bucket boundaries MUST be strictly increasing.
/// This sentinel marks an uninitialised boundary value.
inline constexpr double kHistogramBucketSentinel = -1.0;

// ============================================================================
// § 3  Tracing constraints
// ============================================================================

/// Maximum depth of parent-child span nesting within a single trace.
inline constexpr std::size_t kMaxSpanNestingDepth = 128;

/// Maximum number of attributes per span.
inline constexpr std::size_t kMaxSpanAttributes = 64;

/// Maximum trace ID length in bytes (128-bit = 16 bytes).
inline constexpr std::size_t kTraceIdBytes = 16;

/// Maximum span ID length in bytes (64-bit = 8 bytes).
inline constexpr std::size_t kSpanIdBytes = 8;

// ============================================================================
// § 4  SLO window contract
// ============================================================================

/// Minimum SLO measurement window duration.
inline constexpr std::chrono::seconds kMinSloWindowDuration{60};

/// Maximum SLO measurement window duration.
inline constexpr std::chrono::hours kMaxSloWindowDuration{24 * 30};

/// Grace period after window close during which a late-update warning is emitted
/// (rather than an immediate error).  After this period, SLO_WINDOW_INVALID applies.
inline constexpr std::chrono::seconds kSloWindowGracePeriod{5};

// ============================================================================
// § 5  Export contract
// ============================================================================

/// Default Prometheus scrape timeout.
inline constexpr std::chrono::seconds kPrometheusDefaultScrapeTimeout{10};

/// OTLP export batch size limit.
inline constexpr std::size_t kOtlpMaxBatchSize = 512;

/// Maximum number of metrics exported per Prometheus scrape response.
inline constexpr std::size_t kMaxScrapeMetrics = 50'000;

// ============================================================================
// § 6  Error taxonomy
// ============================================================================

/**
 * @brief Canonical error codes for the observability module.
 *
 * Components must map all internal error conditions to one of these codes.
 * New codes must be appended; existing values must never be renumbered.
 */
enum class ObservabilityErrorCode : int {
    /// No error.
    OK = 0,

    /// A counter value attempted to decrease (monotonicity violation).
    METRIC_OVERFLOW = 1,

    /// A trace span was dropped due to overload or buffer exhaustion.
    TRACE_SPAN_DROPPED = 2,

    /// A structured log entry could not be written (I/O failure or buffer full).
    LOG_WRITE_FAILED = 3,

    /// An update arrived for an SLO window that has already been closed.
    SLO_WINDOW_INVALID = 4,

    /// The configured exporter (Prometheus / OTLP) endpoint is unreachable.
    EXPORTER_UNAVAILABLE = 5,

    /// Histogram bucket boundaries are not strictly increasing.
    HISTOGRAM_BUCKET_ORDER_INVALID = 6,

    /// Metric cardinality limit exceeded; new label-set rejected.
    METRIC_CARDINALITY_EXCEEDED = 7,

    /// Span nesting depth exceeds kMaxSpanNestingDepth.
    SPAN_DEPTH_EXCEEDED = 8,

    /// Internal observability error.
    INTERNAL_ERROR = 9,
};

/// Returns true for codes where data loss (drop/miss) has already occurred.
[[nodiscard]] inline constexpr bool isDataLossCode(ObservabilityErrorCode code) noexcept {
    return code == ObservabilityErrorCode::TRACE_SPAN_DROPPED
        || code == ObservabilityErrorCode::LOG_WRITE_FAILED;
}

// ============================================================================
// § 7  Counter monotonicity contract
//
// Implementations MUST guarantee:
//   counter.value(t2) >= counter.value(t1)  for all t2 > t1
//
// The only exception is an explicit reset (e.g. process restart), which MUST
// be signalled to consumers out-of-band.
// ============================================================================

/// Returns true when a counter transition from @p prev to @p next is valid.
[[nodiscard]] inline constexpr bool isValidCounterTransition(
        std::int64_t prev, std::int64_t next) noexcept {
    return next >= prev;
}

// ============================================================================
// § 8  Structured log immutability contract
//
// Once a log entry is appended to the write-ahead buffer, its field set is
// frozen.  Consumers downstream of the pipeline receive the exact byte sequence
// produced at write time.
// ============================================================================

/// Maximum size of a single structured log entry in bytes.
inline constexpr std::size_t kMaxLogEntryBytes = 64 * 1024;

/// Maximum number of structured fields per log entry.
inline constexpr std::size_t kMaxLogFields = 64;

} // namespace observability
} // namespace themis
