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
// § 5b  Phase 2 Distributed Tracing SDK constraints
// ============================================================================

/// Maximum number of baggage items per distributed trace context.
inline constexpr std::size_t kMaxBaggageItems = 128;

/// Maximum length of a baggage item key in bytes.
inline constexpr std::size_t kMaxBaggageKeyBytes = 128;

/// Maximum length of a baggage item value in bytes.
inline constexpr std::size_t kMaxBaggageValueBytes = 1024;

/// Maximum trace context header size when serialized (bytes).
inline constexpr std::size_t kMaxTraceContextHeaderBytes = 4096;

// ============================================================================
// § 5c  Phase 2 High-Cardinality Metrics constraints
// ============================================================================

/// Default maximum cardinality per metric family (distinct label sets).
inline constexpr std::size_t kDefaultHighCardinalityLimit = 10'000;

/// Hard maximum cardinality per metric family (safety bound).
inline constexpr std::size_t kMaxHighCardinalityLimit = 100'000;

/// Minimum cardinality threshold before automatic fallback activation.
inline constexpr std::size_t kCardinalityWarningThreshold = 8'000;

/// Special label name for aggregated/dropped label sets.
inline constexpr const char* kCardinalityOtherLabel = "__other";

// ============================================================================
// § 5d  Phase 2 Operator Remediation Engine constraints
// ============================================================================

/// Maximum number of active remediation hints to track.
inline constexpr std::size_t kMaxActiveRemediationHints = 256;

/// Default deduplication time window for remediation hints (seconds).
inline constexpr std::uint32_t kRemediationHintDeduplicationSeconds = 300;

/// Maximum number of remediation actions per hint.
inline constexpr std::size_t kMaxRemediationActionsPerHint = 16;

/// Maximum length of remediation hint description (characters).
inline constexpr std::size_t kMaxRemediationHintLength = 4096;

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

    // === Phase 2: Observability Expansion (Distributed Tracing SDK)  ===
    /// Distributed tracing SDK: invalid trace context format or content.
    DTI_INVALID_TRACE_CONTEXT = 10,

    /// Distributed tracing SDK: baggage item count exceeds kMaxBaggageItems.
    DTI_BAGGAGE_OVERFLOW = 11,

    /// Distributed tracing SDK: unsupported trace propagation format.
    DTI_UNSUPPORTED_FORMAT = 12,

    /// Distributed tracing SDK: HTTP header parse error.
    DTI_HEADER_PARSE_ERROR = 13,

    /// Distributed tracing SDK: context propagation to downstream failed.
    DTI_CONTEXT_PROPAGATION_FAILED = 14,

    /// Distributed tracing SDK: internal SDK error.
    DTI_INTERNAL_ERROR = 15,

    // === Phase 2: Observability Expansion (High-Cardinality Metrics) ===
    /// High-cardinality metrics: metric cardinality limit exceeded.
    HCM_CARDINALITY_LIMIT_EXCEEDED = 16,

    /// High-cardinality metrics: invalid or unimplemented fallback strategy.
    HCM_INVALID_FALLBACK_STRATEGY = 17,

    /// High-cardinality metrics: memory usage exceeds threshold.
    HCM_MEMORY_LIMIT_EXCEEDED = 18,

    /// High-cardinality metrics: unsupported cardinality policy.
    HCM_UNSUPPORTED_POLICY = 19,

    /// High-cardinality metrics: internal tracker error.
    HCM_INTERNAL_ERROR = 20,

    // === Phase 2: Observability Expansion (Operator Remediation Engine) ===
    /// Operator remediation: pattern matching failed.
    ORE_PATTERN_MATCH_ERROR = 26,

    /// Operator remediation: invalid or malformed metric data.
    ORE_INVALID_METRIC_DATA = 27,

    /// Operator remediation: listener notification failed.
    ORE_LISTENER_NOTIFICATION_FAILED = 28,

    /// Operator remediation: duplicate pattern name.
    ORE_DUPLICATE_PATTERN = 29,

    /// Operator remediation: internal engine error.
    ORE_INTERNAL_ERROR = 30,
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
