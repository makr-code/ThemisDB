/**
 * @file analytics_api_contract.h
 * @brief Frozen analytics runtime contract semantics for the active v1.x line.
 *
 * This header defines the normative contract for the analytics module covering
 * aggregation, streaming/CEP, OLAP plan stability, forecasting, and anomaly
 * detection surfaces.  All analytics module implementations must honour every
 * contract defined here within the current major release line.
 *
 * ## Contract Scope
 *
 * The contracts below are binding for all components that participate in the
 * ThemisDB analytics pipeline:
 *   - Aggregation engines (GROUP BY, SUM, COUNT, AVG, MIN, MAX)
 *   - Streaming / CEP engines (TumblingWindow, SlidingWindow, SessionWindow)
 *   - OLAP query planner / cost model
 *   - Forecasting inference service (model loading, prediction)
 *   - Anomaly detection pipeline (threshold evaluation, alert emission)
 *
 * ## Versioning
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump
 * with migration notes and a CHANGELOG entry.
 *
 * @see src/analytics/ROADMAP.md  — Phase 1 contract item
 * @see include/analytics/ROADMAP.md — Phase 6 documentation item
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace themis {
namespace analytics {

// ============================================================================
// § 1  Aggregation contract
//
// GROUP BY ordering guarantee:
//   For a given data version (unchanged statistics, unchanged schema) the rows
//   produced by a GROUP BY query are returned in deterministic key-order.
//
// NULL handling:
//   NULLs are EXCLUDED from all aggregate computations (SUM, AVG, MIN, MAX,
//   COUNT(col)) by default.  COUNT(*) counts all rows including those with
//   NULLs in projected columns.  This mirrors SQL-standard NULL semantics.
//
// Overflow handling:
//   A numeric aggregate (SUM, running total) that would overflow the declared
//   result type raises AGGREGATION_OVERFLOW immediately.  Silent truncation or
//   wrap-around is PROHIBITED.
// ============================================================================

/// Maximum number of GROUP BY keys accepted per query.
inline constexpr std::size_t kMaxGroupByKeys = 64;

/// Maximum number of rows that may be buffered in a single aggregation batch
/// before a flush is forced.
inline constexpr std::size_t kMaxAggregationBatchRows = 10'000'000u;

// ============================================================================
// § 2  Streaming / CEP contract
//
// Event ordering:
//   Events within a partition are processed in strict arrival order.
//   Out-of-order delivery across partitions is not guaranteed; consumers must
//   apply their own ordering if cross-partition ordering is required.
//
// Window semantics:
//   - Tumbling windows: non-overlapping, fixed-size time or row count slices.
//   - Sliding windows: overlapping windows advancing by the configured step.
//   - Session windows: close after a configurable gap of inactivity.
//
// Late-arrival handling:
//   Events arriving after the watermark boundary are NOT silently swallowed.
//   The WINDOW_EXPIRED code is returned to the event producer.  The event is
//   discarded; no partial re-aggregation is attempted.
//
// Backpressure contract:
//   When the internal event queue is full the producer receives
//   STREAM_BACKPRESSURE immediately.  No blocking or silent drop occurs.
//
// Alert-per-event guarantee:
//   The anomaly detection subsystem emits exactly one alert per qualifying
//   event.  Under backpressure, the alert is queued or the producer is signalled
//   via STREAM_BACKPRESSURE — events are NEVER silently dropped.
// ============================================================================

/// Default tumbling/sliding window size in milliseconds.
inline constexpr std::int64_t kDefaultWindowSizeMs = 1'000;

/// Maximum number of concurrently open session windows per instance.
inline constexpr std::size_t kMaxOpenSessionWindows = 1'000'000u;

/// Maximum records buffered in any single window before eviction is forced.
inline constexpr std::size_t kMaxWindowRecords = 10'000'000u;

/// Maximum CEP pattern sequence length (event count).
inline constexpr std::size_t kMaxCepPatternLength = 64;

// ============================================================================
// § 3  OLAP plan stability contract
//
// For a given data version (same table statistics, same schema, same
// configuration snapshot) the query planner MUST produce the same plan
// (identical cardinality estimate and operator tree) for the same SQL text.
// Operator reordering is only permitted when equivalence is provable.
// Plan instability across identical inputs is a contract violation.
// ============================================================================

/// Maximum allowed relative deviation in consecutive plan cost estimates for
/// the same query on the same data version (0 = no deviation permitted).
inline constexpr double kPlanCostStabilityTolerance = 0.0;

// ============================================================================
// § 4  Forecasting contract
//
// Inference idempotency:
//   Calling forecast() with the same model handle, the same input series, and
//   the same random seed returns bit-identical results.
//
// NaN / Inf input:
//   Any NaN or Inf value present in the input time-series causes an immediate
//   FORECAST_INPUT_INVALID error.  Silent propagation to the model is PROHIBITED.
//
// Model lifecycle:
//   A call that references a model handle that has not been loaded or has been
//   evicted returns FORECAST_MODEL_NOT_FOUND.  No blocking model reload is
//   attempted inline; the caller must explicitly reload.
// ============================================================================

// ============================================================================
// § 5  Anomaly detection contract
//
// Threshold validity:
//   A threshold of exactly 0.0 is treated as "fire on every event" (valid).
//   A negative threshold value causes ANOMALY_THRESHOLD_INVALID immediately at
//   configuration time.
//
// Alert guarantee:
//   Every event whose anomaly score meets or exceeds the configured threshold
//   produces exactly one alert object.  No deduplication or rate-limiting is
//   applied by the detection engine itself; those concerns belong to the caller.
// ============================================================================

// ============================================================================
// § 6  Error taxonomy
//
// All analytics components must map their internal error states to one of these
// canonical codes.  This enables uniform operator diagnostics and consistent
// retry / fail-closed policy enforcement.
// ============================================================================

/**
 * @brief Canonical analytics error codes.
 *
 * Codes in range [100, 199] are analytics-specific.  Reserve [200+] for
 * future category extensions.
 */
enum class AnalyticsErrorCode : int {
    /// SUM or other numeric aggregate would overflow the declared result type.
    AGGREGATION_OVERFLOW          = 100,

    /// Window watermark exceeded; the late event has been discarded.
    WINDOW_EXPIRED                = 101,

    /// Stream producer backpressure threshold reached; event was not accepted.
    STREAM_BACKPRESSURE           = 102,

    /// Named forecast model is not loaded or could not be found.
    FORECAST_MODEL_NOT_FOUND      = 103,

    /// NaN or Inf value present in the forecast input series.
    FORECAST_INPUT_INVALID        = 104,

    /// Anomaly detection threshold value is semantically invalid (negative).
    ANOMALY_THRESHOLD_INVALID     = 105,

    /// OLAP query plan cardinality estimate changed for identical input (bug).
    PLAN_CARDINALITY_UNSTABLE     = 106,

    /// CEP pattern evaluation failed; required event sequence is incomplete.
    CEP_PATTERN_INCOMPLETE        = 107,

    /// Unclassified analytics internal error; always treated as hard error.
    INTERNAL_ERROR                = 199,
};

/**
 * @brief Returns true when the error code indicates a non-retryable hard error.
 *
 * Hard errors must not be silently suppressed; callers must propagate them to
 * the operator / observability pipeline.
 */
[[nodiscard]] inline constexpr bool isHardAnalyticsError(AnalyticsErrorCode code) noexcept {
    return code == AnalyticsErrorCode::AGGREGATION_OVERFLOW
        || code == AnalyticsErrorCode::FORECAST_INPUT_INVALID
        || code == AnalyticsErrorCode::ANOMALY_THRESHOLD_INVALID
        || code == AnalyticsErrorCode::PLAN_CARDINALITY_UNSTABLE
        || code == AnalyticsErrorCode::INTERNAL_ERROR;
}

/**
 * @brief Returns true when the error code represents a backpressure / flow
 *        control condition that the producer should handle by slowing down.
 */
[[nodiscard]] inline constexpr bool isBackpressureError(AnalyticsErrorCode code) noexcept {
    return code == AnalyticsErrorCode::STREAM_BACKPRESSURE
        || code == AnalyticsErrorCode::WINDOW_EXPIRED;
}

// ============================================================================
// § 7  Forecast model capability flags
//
// Model adapters must declare their capabilities before any inference call.
// If a declared capability is unavailable, FORECAST_MODEL_NOT_FOUND is
// returned rather than blocking or timing out silently.
// ============================================================================

/**
 * @brief Capability flags that a forecast model adapter may require at runtime.
 */
enum class ForecastModelCapability : unsigned int {
    None           = 0u,
    /// Model requires a GPU device for inference.
    GpuInference   = 1u << 0,
    /// Model requires the ONNX runtime.
    OnnxRuntime    = 1u << 1,
    /// Model requires network access to a remote serving endpoint.
    RemoteEndpoint = 1u << 2,
};

[[nodiscard]] inline constexpr ForecastModelCapability operator|(
        ForecastModelCapability a, ForecastModelCapability b) noexcept {
    return static_cast<ForecastModelCapability>(
        static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

[[nodiscard]] inline constexpr bool hasModelCapability(
        ForecastModelCapability set, ForecastModelCapability flag) noexcept {
    return (static_cast<unsigned int>(set) & static_cast<unsigned int>(flag)) != 0u;
}

// ============================================================================
// § 8  Bounded Execution Policy
//
// Defines resource limits applied uniformly to ML-serving inference calls
// (MLServingClient::infer()) and analytics export operations
// (IAnalyticsExporter::exportToFile()).  Enforcement uses the existing
// circuit-breaker infrastructure as the back-end.
//
// Target: Wave B / Q4 2026.
// ============================================================================

/**
 * @brief Resource and latency limits applied to bounded-execution operations.
 *
 * Callers embed a `BoundedExecutionPolicy` in their request or configuration
 * to declare the acceptable resource envelope for a single operation.  The
 * enforcing layer (circuit-breaker or semaphore) rejects calls that would
 * violate the limits before dispatching to the back-end.
 *
 * ### Field semantics
 * - `max_latency_ms`           — wall-clock timeout in milliseconds; 0 = no timeout.
 * - `max_concurrent_requests`  — maximum in-flight requests sharing one policy
 *                                 instance; 0 = unlimited.
 * - `queue_depth`              — maximum number of requests allowed to wait when
 *                                 the concurrency limit is reached; 0 = no queuing
 *                                 (reject immediately on saturation).
 *
 * ### Enforcement contract
 * When a call is rejected due to policy limits, the enforcing component MUST
 * return an appropriate status code (`MLServingStatus::TIMEOUT` /
 * `AnalyticsErrorCode::STREAM_BACKPRESSURE`) and MUST NOT block the caller
 * beyond `max_latency_ms` milliseconds.
 */
struct BoundedExecutionPolicy {
    /// Wall-clock timeout in milliseconds.  0 means no timeout enforced.
    uint32_t max_latency_ms          = 0u;
    /// Maximum number of concurrent operations.  0 means unlimited.
    uint32_t max_concurrent_requests = 0u;
    /// Maximum depth of the waiting queue when the concurrency limit is full.
    /// 0 means callers are rejected immediately on saturation (no queuing).
    uint32_t queue_depth             = 0u;

    /// Returns true when this policy imposes at least one limit.
    [[nodiscard]] constexpr bool isConstrained() const noexcept {
        return max_latency_ms != 0u
            || max_concurrent_requests != 0u
            || queue_depth != 0u;
    }
};

// ============================================================================
// § 9  Streaming Runtime Limits (Phase 2 / Wave B — Q4 2026)
//
// Applies back-pressure and memory guards to AnalyticsStreamingEngine and
// DistributedCoordinator streaming paths.  When a limit is reached the engine
// applies the configured BackPressureMode instead of silently dropping events
// or blocking indefinitely.
// ============================================================================

/// Action taken when a streaming limit is exceeded.
enum class BackPressureMode : uint8_t {
    /// Discard the incoming event; increment an internal drop counter.
    DROP   = 0,
    /// Block the producer until the engine drains below the limit.
    /// Warning: may cause the producer thread to stall indefinitely if the
    /// consumer is slower than the producer.
    BLOCK  = 1,
    /// Shed load by discarding the oldest buffered events to make room for new
    /// arrivals.  Maintains freshness at the cost of completeness.
    SHED   = 2,
};

/**
 * @brief Runtime limits for streaming and CEP pipeline paths.
 *
 * Embed in streaming engine or coordinator configuration to bound memory
 * and event-rate usage.  When all fields are zero the engine operates in
 * unconstrained mode (legacy behaviour).
 *
 * ### Field semantics
 * - `max_events_per_window`   — maximum events buffered per window before the
 *                               back-pressure action fires; 0 = unlimited.
 * - `max_window_memory_bytes` — maximum bytes allocated for all open windows
 *                               combined; 0 = unlimited.
 * - `back_pressure_mode`      — action taken when a limit is exceeded.
 *
 * ### Contract
 * When `back_pressure_mode == DROP` and the event limit is hit, the engine
 * MUST return `AnalyticsErrorCode::STREAM_BACKPRESSURE` to the producer
 * instead of blocking or silently discarding.
 *
 * @since Wave B / Q4 2026
 */
struct StreamingRuntimeLimits {
    /// Maximum events buffered per window; 0 = unlimited.
    std::size_t max_events_per_window   = 0u;
    /// Maximum total memory for open windows (bytes); 0 = unlimited.
    std::size_t max_window_memory_bytes = 0u;
    /// Action taken when a limit is exceeded.
    BackPressureMode back_pressure_mode = BackPressureMode::DROP;

    /// Returns true when at least one limit is active.
    [[nodiscard]] constexpr bool isConstrained() const noexcept {
        return max_events_per_window != 0u || max_window_memory_bytes != 0u;
    }
};

} // namespace analytics
} // namespace themis
