/**
 * @file high_cardinality_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟡 BETA (Phase 2 Observability Expansion)
 * @note Score: 0/100 (implementation in progress)
 * @note Status: High-cardinality metrics tracking with safety bounds
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include <memory>

namespace themis {
namespace observability {

/**
 * @brief Cardinality tracking policy for metric time series.
 *
 * Defines what action to take when a metric's cardinality approaches or
 * exceeds the configured limit.
 */
enum class CardinalityExceededPolicy {
    /// Drop new label sets that would exceed the limit (fail-closed).
    DROP_NEW_SETS,

    /// Aggregate new label sets into a catch-all "__other" series.
    AGGREGATE_TO_OTHER,

    /// Log a warning but continue accepting new label sets (permissive).
    WARN_ONLY,
};

/**
 * @brief Cardinality limit configuration for a single metric or metric family.
 *
 * Defines the maximum number of distinct label-set combinations (time series)
 * allowed for a metric, and how to handle when the limit is exceeded.
 */
struct CardinalityLimit {
    /// Maximum distinct label-set combinations for this metric/family.
    std::size_t max_series{10'000};

    /// Action to take when limit is exceeded.
    CardinalityExceededPolicy policy{CardinalityExceededPolicy::DROP_NEW_SETS};

    /// Whether to emit diagnostic counters for cardinality events.
    bool emit_diagnostics{true};

    /// Grace period (in seconds) between cardinality explosion alerts.
    std::uint32_t alert_grace_period_seconds{60};
};

/**
 * @brief Cardinality statistics for a single metric.
 *
 * Snapshot of cardinality information at a point in time.
 */
struct CardinalityStats {
    /// Current number of distinct label-set combinations.
    std::size_t current_series_count{0};

    /// Configured maximum cardinality.
    std::size_t limit{0};

    /// Number of times a new label-set was rejected due to cardinality limit.
    std::uint64_t rejected_sets_total{0};

    /// Number of times new label-sets were aggregated into "__other".
    std::uint64_t aggregated_sets_total{0};

    /// Timestamp when cardinality was last updated.
    std::int64_t last_updated_ns{0};

    /// Whether the metric is currently at or above the cardinality limit.
    bool at_limit{false};

    /// Percentage of cardinality limit currently in use (0-100).
    double utilization_percent{0.0};
};

/**
 * @brief Fallback strategy for handling cardinality overflow.
 *
 * When a metric's cardinality exceeds its limit, a fallback strategy
 * determines how to handle new, previously-unseen label sets.
 */
class CardinalityFallbackStrategy {
public:
    /**
     * @brief Apply the fallback strategy to a label set.
     *
     * Returns a modified label set suitable for recording the metric
     * when cardinality is exceeded.
     *
     * @param original_labels The original label set.
     * @return Modified label set (may include synthetic labels like "__other").
     */
    virtual std::map<std::string, std::string> apply(
        const std::map<std::string, std::string>& original_labels) = 0;

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~CardinalityFallbackStrategy() = default;
};

/**
 * @brief High-cardinality metrics tracker for bounded telemetry collection.
 *
 * The HighCardinalityMetrics class provides utilities for:
 * - Tracking distinct label-set combinations per metric
 * - Enforcing cardinality limits with configurable overflow policies
 * - Automatically aggregating or dropping excessive label sets
 * - Emitting diagnostic alerts when cardinality limits are exceeded
 * - Supporting per-metric and per-family cardinality configuration
 *
 * ## Integration Pattern
 *
 * ```cpp
 * // Create a tracker with per-metric cardinality limits
 * auto tracker = std::make_unique<HighCardinalityMetricsTracker>();
 *
 * // Configure cardinality limit for "http_request_duration_ms"
 * CardinalityLimit limit;
 * limit.max_series = 5000;  // Allow up to 5000 distinct label sets
 * limit.policy = CardinalityExceededPolicy::AGGREGATE_TO_OTHER;
 * tracker->setCardinalityLimit("http_request_duration_ms", limit);
 *
 * // When recording a metric, check cardinality first
 * auto labels = build_labels_from_request(request);
 * if (tracker->canAcceptLabelSet("http_request_duration_ms", labels)) {
 *     // Record the metric with original labels
 *     metrics.observeHistogram("http_request_duration_ms", duration_ms, labels);
 * } else {
 *     // Get fallback labels (may include "__other" marker)
 *     auto fallback_labels = tracker->getFallbackLabels("http_request_duration_ms", labels);
 *     metrics.observeHistogram("http_request_duration_ms", duration_ms, fallback_labels);
 * }
 * ```
 *
 * ## Error Codes (Observability Phase 2 Extension)
 *
 * - HCM_CARDINALITY_LIMIT_EXCEEDED = 16
 * - HCM_INVALID_FALLBACK_STRATEGY = 17
 * - HCM_MEMORY_LIMIT_EXCEEDED = 18
 * - HCM_UNSUPPORTED_POLICY = 19
 * - HCM_INTERNAL_ERROR = 20
 */
class HighCardinalityMetricsTracker {
public:
    /**
     * @brief Construct a high-cardinality metrics tracker.
     */
    HighCardinalityMetricsTracker() = default;

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~HighCardinalityMetricsTracker() = default;

    /**
     * @brief Set cardinality limit for a specific metric.
     *
     * @param metric_name Fully-qualified metric name (e.g., "http_request_duration_ms").
     * @param limit Cardinality limit configuration for this metric.
     * @return true on success, false if metric name is invalid.
     *
     * @note This function is thread-safe.
     */
    virtual bool setCardinalityLimit(
        const std::string& metric_name,
        const CardinalityLimit& limit) = 0;

    /**
     * @brief Set cardinality limit for all metrics matching a prefix.
     *
     * @param metric_prefix Metric name prefix (e.g., "http_" for all "http_*" metrics).
     * @param limit Cardinality limit configuration for matching metrics.
     * @return Number of metrics updated.
     *
     * @note This function is thread-safe.
     */
    virtual std::size_t setCardinalityLimitByPrefix(
        const std::string& metric_prefix,
        const CardinalityLimit& limit) = 0;

    /**
     * @brief Get the current cardinality configuration for a metric.
     *
     * @param metric_name Fully-qualified metric name.
     * @return CardinalityLimit configuration, or default if not explicitly set.
     *
     * @note This function is thread-safe.
     */
    virtual CardinalityLimit getCardinalityLimit(const std::string& metric_name) = 0;

    /**
     * @brief Check whether a label set can be accepted for a metric.
     *
     * Returns true if recording a metric with the given label set would not
     * exceed the cardinality limit for that metric.
     *
     * @param metric_name Fully-qualified metric name.
     * @param labels Label set (map of label name -> value).
     * @return true if label set is within cardinality limit, false otherwise.
     *
     * @note This function is thread-safe.
     */
    virtual bool canAcceptLabelSet(
        const std::string& metric_name,
        const std::map<std::string, std::string>& labels) = 0;

    /**
     * @brief Record a new label set for a metric.
     *
     * Registers a label set with the tracker. If the metric is at cardinality
     * limit, the registered labels may be modified according to the configured
     * fallback policy.
     *
     * @param metric_name Fully-qualified metric name.
     * @param labels Label set to register.
     * @return The registered labels (may differ from input if fallback was applied).
     *
     * @note This function is thread-safe and idempotent (re-registering the
     *       same label set is safe).
     */
    virtual std::map<std::string, std::string> recordLabelSet(
        const std::string& metric_name,
        const std::map<std::string, std::string>& labels) = 0;

    /**
     * @brief Get fallback labels when cardinality limit is exceeded.
     *
     * Returns a modified label set suitable for recording when the metric
     * has reached its cardinality limit.
     *
     * @param metric_name Fully-qualified metric name.
     * @param original_labels Original label set.
     * @return Modified label set (e.g., with "__other" marker added).
     *
     * @note This function is thread-safe.
     */
    virtual std::map<std::string, std::string> getFallbackLabels(
        const std::string& metric_name,
        const std::map<std::string, std::string>& original_labels) = 0;

    /**
     * @brief Get current cardinality statistics for a metric.
     *
     * @param metric_name Fully-qualified metric name.
     * @return CardinalityStats snapshot (may be stale, see caveats).
     *
     * @note This function is thread-safe but results are snapshots and
     *       may become stale immediately after returning.
     */
    virtual CardinalityStats getCardinalityStats(const std::string& metric_name) = 0;

    /**
     * @brief Get cardinality statistics for all tracked metrics.
     *
     * @return Map of metric name -> CardinalityStats.
     *
     * @note This function is thread-safe but results are snapshots and
     *       may become stale immediately after returning.
     */
    virtual std::map<std::string, CardinalityStats> getAllCardinalityStats() = 0;

    /**
     * @brief Register a custom fallback strategy for a metric.
     *
     * @param metric_name Fully-qualified metric name.
     * @param strategy Custom strategy implementation (takes ownership).
     * @return true on success, false if metric name is invalid.
     *
     * @note This function is thread-safe.
     */
    virtual bool setFallbackStrategy(
        const std::string& metric_name,
        std::unique_ptr<CardinalityFallbackStrategy> strategy) = 0;

    /**
     * @brief Clear all recorded label sets for a metric.
     *
     * Resets the cardinality tracking state for a metric, clearing all
     * previously-recorded label sets and counters.
     *
     * @param metric_name Fully-qualified metric name.
     * @return true on success, false if metric not found.
     *
     * @note This function is thread-safe.
     */
    virtual bool resetMetricCardinality(const std::string& metric_name) = 0;

    /**
     * @brief Clear all cardinality tracking state.
     *
     * Resets the entire tracker to empty state.
     *
     * @note This function is thread-safe.
     */
    virtual void resetAll() = 0;

    /**
     * @brief Enable/disable cardinality tracking for a metric.
     *
     * When disabled, the tracker allows unlimited cardinality for a metric.
     *
     * @param metric_name Fully-qualified metric name.
     * @param enabled true to enable tracking, false to disable.
     * @return true on success, false if metric not found.
     *
     * @note This function is thread-safe.
     */
    virtual bool setTrackingEnabled(const std::string& metric_name, bool enabled) = 0;

    /**
     * @brief Check whether cardinality tracking is enabled for a metric.
     *
     * @param metric_name Fully-qualified metric name.
     * @return true if tracking is enabled, false otherwise.
     *
     * @note This function is thread-safe.
     */
    virtual bool isTrackingEnabled(const std::string& metric_name) = 0;
};

} // namespace observability
} // namespace themis
