/**
 * @file metric_anomaly_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// Bring in the analytics anomaly-detection primitives (DataPoint, AnomalyResult,
// StreamingAnomalyDetector, …).  The analytics module uses namespace
// themisdb::analytics; the observability bridge lives in themis::observability.
#include "analytics/anomaly_detection.h"

namespace themis {
namespace observability {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Convenience aliases for the analytics types we expose in the bridge API
// ---------------------------------------------------------------------------

/// Forwards from the analytics module so callers don't need to include it.
using AnomalyMethod = themisdb::analytics::AnomalyMethod;
using AnomalyResult = themisdb::analytics::AnomalyResult;

// ---------------------------------------------------------------------------
// MetricAnomaly – an anomaly detected on a specific metric series
// ---------------------------------------------------------------------------

/**
 * @brief A single anomaly observation attributed to a named metric.
 *
 * Extends the analytics-layer @c AnomalyResult with the originating metric
 * name and a human-readable severity label.
 */
struct MetricAnomaly {
    /// Metric name this anomaly was detected on (matches @c MonitoredMetric::name).
    std::string metric_name;

    /// Score in [0, 1]: 0 = definitely normal, 1 = definite anomaly.
    double score{0.0};

    /// true when score >= configured threshold.
    bool is_anomaly{false};

    /// The observed value that triggered this result.
    double observed_value{0.0};

    /// Severity bucket: "low" / "medium" / "high" / "critical".
    std::string severity;

    /// When the observation was recorded.
    std::chrono::system_clock::time_point timestamp;

    /// Human-readable explanation.
    std::string description;

    /** Serialize to JSON. */
    json toJson() const;
};

// ---------------------------------------------------------------------------
// MonitoredMetric – configuration for one metric stream
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for a single metric stream to be monitored.
 *
 * ### Example
 * ```cpp
 * MonitoredMetric cfg;
 * cfg.name             = "themis_query_latency_ms";
 * cfg.method           = AnomalyMethod::Z_SCORE;
 * cfg.threshold        = 0.7;
 * cfg.window_size      = 500;
 * cfg.auto_train_after = 100;
 * detector.monitor(cfg);
 * ```
 */
struct MonitoredMetric {
    /// Prometheus-style metric name (e.g. `"themis_query_latency_ms"`).
    std::string name;

    /**
     * @brief Detection algorithm.
     *
     * Supported values (from @c themisdb::analytics::AnomalyMethod):
     * - @c Z_SCORE (default) – fast, Gaussian assumption
     * - @c MODIFIED_Z_SCORE  – robust, MAD-based
     * - @c IQR               – interquartile range fence
     * - @c ISOLATION_FOREST  – unsupervised tree-based
     * - @c LOF               – density-based Local Outlier Factor
     * - @c ENSEMBLE          – weighted combination of all methods
     */
    AnomalyMethod method{AnomalyMethod::Z_SCORE};

    /**
     * @brief Anomaly score threshold in [0, 1].
     *
     * Observations with score >= threshold are classified as anomalies.
     * Default: 0.7.
     */
    double threshold{0.7};

    /**
     * @brief Maximum number of data points in the sliding training window.
     * Default: 500.
     */
    size_t window_size{500};

    /**
     * @brief Minimum number of data points before anomaly detection begins.
     * Default: 50 (allows the detector to establish a baseline).
     */
    size_t auto_train_after{50};

    /**
     * @brief Retrain the model when the window fills up.
     * Default: true.
     */
    bool retrain_on_window{true};
};

// ---------------------------------------------------------------------------
// MetricAnomalyDetector
// ---------------------------------------------------------------------------

/**
 * @brief Observability-layer bridge: feeds @c MetricsCollector observations
 *        into the analytics `StreamingAnomalyDetector` and publishes results
 *        as Prometheus gauges.
 *
 * ### Overview
 * @c MetricAnomalyDetector maintains one @c StreamingAnomalyDetector per
 * registered metric stream.  Callers push scalar metric observations via
 * @c observe(); the detector evaluates them against the trained model and
 * returns an optional @c MetricAnomaly when one is detected.
 *
 * Anomaly metrics are published to @c MetricsCollector under the
 * `themis_anomaly_*` namespace for Prometheus scraping:
 *
 * | Gauge | Description |
 * |-------|-------------|
 * | `themis_anomaly_score{metric="<name>"}` | Latest anomaly score [0–1] |
 * | `themis_anomaly_detected{metric="<name>"}` | 1 if latest point is anomalous, 0 otherwise |
 * | `themis_anomaly_total{metric="<name>"}` | Total anomalies detected since start |
 * | `themis_anomaly_window_size{metric="<name>"}` | Current training window size |
 *
 * ### Thread safety
 * All public methods are thread-safe.
 *
 * ### Example
 * ```cpp
 * MetricAnomalyDetector detector;
 *
 * MonitoredMetric cfg;
 * cfg.name             = "themis_query_latency_ms";
 * cfg.method           = AnomalyMethod::Z_SCORE;
 * cfg.threshold        = 0.7;
 * cfg.window_size      = 200;
 * cfg.auto_train_after = 50;
 * detector.monitor(cfg);
 *
 * // Each time a new latency value arrives:
 * auto anomaly = detector.observe("themis_query_latency_ms", latency_ms);
 * if (anomaly && anomaly->is_anomaly) {
 *     // fire alert …
 * }
 *
 * detector.publishMetrics();
 * ```
 */
class MetricAnomalyDetector {
public:
    /**
     * @brief Optional callback invoked on every detected anomaly.
     *
     * Signature: `void callback(const MetricAnomaly& anomaly)`
     */
    using AnomalyCallback = std::function<void(const MetricAnomaly&)>;

    MetricAnomalyDetector() = default;
    ~MetricAnomalyDetector() = default;

    // Non-copyable
    MetricAnomalyDetector(const MetricAnomalyDetector&)            = delete;
    MetricAnomalyDetector& operator=(const MetricAnomalyDetector&) = delete;

    /**
     * @brief Register a metric stream for anomaly monitoring.
     *
     * If a stream with the same name already exists it is replaced (the
     * accumulated training window is discarded).
     *
     * @param config  Configuration for the new stream.
     */
    void monitor(const MonitoredMetric& config);

    /**
     * @brief Remove a previously registered metric stream.
     *
     * Accumulated history and the trained model are discarded.
     * No-op if the name is not registered.
     */
    void unmonitor(const std::string& metric_name);

    /**
     * @brief Push a new scalar observation for a monitored metric.
     *
     * @param metric_name  Must match a registered @c MonitoredMetric::name.
     * @param value        The observed metric value.
     * @param timestamp    Observation timestamp (defaults to now).
     *
     * @return A @c MetricAnomaly if the detector was sufficiently trained and
     *         the point was scored; @c std::nullopt during warm-up or if the
     *         metric is not registered.
     */
    std::optional<MetricAnomaly> observe(
        const std::string& metric_name,
        double value,
        std::chrono::system_clock::time_point timestamp =
            std::chrono::system_clock::now());

    /**
     * @brief Return all anomalies detected for a named metric since monitoring
     *        started (or since the last @c clearAnomalies() call).
     *
     * @throws std::out_of_range if the metric name is not registered.
     */
    std::vector<MetricAnomaly> getAnomalies(const std::string& metric_name) const;

    /**
     * @brief Return anomalies across all registered metrics.
     */
    std::vector<MetricAnomaly> getAllAnomalies() const;

    /**
     * @brief Clear stored anomaly history for a single metric.
     *
     * @throws std::out_of_range if the metric name is not registered.
     */
    void clearAnomalies(const std::string& metric_name);

    /**
     * @brief Clear stored anomaly history across all metrics.
     */
    void clearAllAnomalies();

    /**
     * @brief Set a callback to be invoked whenever an anomaly is detected.
     *
     * Only one callback may be registered at a time; calling @c setCallback
     * replaces any previous one.  Pass an empty @c std::function to clear.
     *
     * @param cb  Callback function; may be empty (@c !cb) to remove.
     */
    void setCallback(AnomalyCallback cb);

    /**
     * @brief Publish current anomaly metrics to @c MetricsCollector.
     *
     * Safe to call from any thread; acquires the internal mutex briefly.
     */
    void publishMetrics() const;

    /**
     * @brief Generate a human-readable anomaly summary report.
     */
    std::string generateReport() const;

    /**
     * @brief Generate a JSON anomaly summary report.
     */
    json generateReportJson() const;

    /**
     * @brief Return the number of registered metric streams.
     */
    size_t monitoredCount() const;

    /**
     * @brief Return the names of all registered metric streams.
     */
    std::vector<std::string> monitoredNames() const;

private:
    // ------------------------------------------------------------------
    // Per-stream state
    // ------------------------------------------------------------------

    struct StreamState {
        MonitoredMetric                           config;
        themisdb::analytics::StreamingAnomalyDetector sad;
        std::vector<MetricAnomaly>                anomalies;
        size_t                                    points_seen{0};

        explicit StreamState(const MonitoredMetric& cfg);
    };

    // ------------------------------------------------------------------
    // Helpers
    // ------------------------------------------------------------------

    static std::string scoreSeverity(double score) noexcept;

    // ------------------------------------------------------------------
    // State
    // ------------------------------------------------------------------
    mutable std::mutex            mutex_;
    std::map<std::string, std::unique_ptr<StreamState>> streams_;
    AnomalyCallback               callback_;
};

} // namespace observability
} // namespace themis

