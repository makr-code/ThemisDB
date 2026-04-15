/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            i_metrics.h                                        ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:02:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     219                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "core/concerns/lifecycle.h"
#include <string>
#include <map>
#include <memory>
#include <chrono>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Abstract metrics interface for observability.
 * 
 * Provides a unified interface for metrics collection that can be
 * implemented by various metrics backends (Prometheus, StatsD, no-op, etc.).
 * Enables testing with mock metrics and runtime switching of implementations.
 */
class IMetrics {
public:
    using Labels = std::map<std::string, std::string>;

    virtual ~IMetrics() = default;

    // -----------------------------------------------------------------------
    // Counter operations (monotonically increasing)
    // -----------------------------------------------------------------------

    /**
     * @brief Increment a monotonically increasing counter.
     *
     * Counters reset to zero only on process restart.  They are suitable
     * for tracking totals such as request counts and error rates.
     *
     * @param name   Metric name (e.g. "http_requests_total").
     * @param value  Amount to add; must be >= 0. Default is 1.
     * @param labels Key/value label set for cardinality (keep cardinality low).
     */
    virtual void incrementCounter(const std::string& name, int64_t value = 1, const Labels& labels = {}) = 0;

    // -----------------------------------------------------------------------
    // Gauge operations (can go up or down)
    // -----------------------------------------------------------------------

    /**
     * @brief Set a gauge to an absolute value.
     *
     * Gauges represent a current snapshot of a value that can increase or
     * decrease (e.g. memory usage, active connections).
     *
     * @param name   Metric name (e.g. "active_connections").
     * @param value  New absolute value.
     * @param labels Key/value label set.
     */
    virtual void setGauge(const std::string& name, double value, const Labels& labels = {}) = 0;

    /**
     * @brief Increment a gauge by a delta.
     * @param name   Metric name.
     * @param delta  Amount to add (positive).
     * @param labels Key/value label set.
     */
    virtual void incrementGauge(const std::string& name, double delta, const Labels& labels = {}) = 0;

    /**
     * @brief Decrement a gauge by a delta.
     * @param name   Metric name.
     * @param delta  Amount to subtract (positive).
     * @param labels Key/value label set.
     */
    virtual void decrementGauge(const std::string& name, double delta, const Labels& labels = {}) = 0;

    // -----------------------------------------------------------------------
    // Histogram operations (track value distributions)
    // -----------------------------------------------------------------------

    /**
     * @brief Record a single observation in a histogram.
     *
     * Histograms track the statistical distribution of values (e.g. request
     * latencies).  Bucket boundaries are typically pre-configured by the
     * backend.
     *
     * @param name   Metric name (e.g. "request_duration_seconds").
     * @param value  Observed value (in the metric's natural unit).
     * @param labels Key/value label set.
     */
    virtual void observeHistogram(const std::string& name, double value, const Labels& labels = {}) = 0;

    // -----------------------------------------------------------------------
    // High-level convenience methods
    // -----------------------------------------------------------------------

    /**
     * @brief Record an operation latency observation in milliseconds.
     *
     * Convenience wrapper around observeHistogram() with a standardized
     * metric name derived from @p operation.
     *
     * @param operation  Logical operation name (e.g. "db.query").
     * @param latencyMs  Elapsed time in milliseconds.
     * @param labels     Additional key/value labels.
     */
    virtual void recordLatency(const std::string& operation, double latencyMs, const Labels& labels = {}) = 0;

    /**
     * @brief Increment the error counter for an operation.
     * @param operation Logical operation name.
     * @param labels    Additional key/value labels.
     */
    virtual void recordError(const std::string& operation, const Labels& labels = {}) = 0;

    /**
     * @brief Increment the success counter for an operation.
     * @param operation Logical operation name.
     * @param labels    Additional key/value labels.
     */
    virtual void recordSuccess(const std::string& operation, const Labels& labels = {}) = 0;

    // -----------------------------------------------------------------------
    // Export and reset
    // -----------------------------------------------------------------------

    /**
     * @brief Export all metrics in Prometheus text-exposition format.
     *
     * The returned string is suitable for serving on a `/metrics` HTTP
     * endpoint and scraping by a Prometheus server.
     *
     * @return Prometheus-formatted metrics string.
     */
    virtual std::string exportMetrics() const = 0;

    /**
     * @brief Reset all counters, gauges, and histograms to zero.
     *
     * Primarily intended for test isolation.  Do NOT call in production.
     */
    virtual void reset() = 0;

    // Lifecycle hooks
    /**
     * @brief Flush any pending metric observations to the backend.
     *
     * Call before shutdown() to ensure the final snapshot is published.
     * Default is a no-op.
     */
    virtual void flush() noexcept {}

    /**
     * @brief Shut down the metrics backend and release resources.
     *
     * Default is a no-op.
     */
    virtual void shutdown() noexcept {}

    /**
     * @brief Probe whether the metrics binding/exporter is healthy.
     *
     * @return ProbeResult with ok=true when the metrics backend is
     *         operational, ok=false with a descriptive message otherwise.
     */
    virtual ProbeResult isHealthy() const { return ProbeResult::healthy(); }
};

/**
 * @brief RAII helper for automatic latency tracking.
 */
class LatencyTimer {
public:
    LatencyTimer(IMetrics& metrics, const std::string& operation, const IMetrics::Labels& labels = {})
        : metrics_(metrics), operation_(operation), labels_(labels),
          start_(std::chrono::steady_clock::now()) {}

    ~LatencyTimer() {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
        metrics_.recordLatency(operation_, static_cast<double>(duration.count()), labels_);
    }

    double elapsedMs() const noexcept {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_);
        return static_cast<double>(duration.count());
    }

private:
    IMetrics& metrics_;
    std::string operation_;
    IMetrics::Labels labels_;
    std::chrono::steady_clock::time_point start_;
};

} // namespace concerns
} // namespace core
} // namespace themis
