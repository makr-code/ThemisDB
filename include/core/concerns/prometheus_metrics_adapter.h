/**
 * @file prometheus_metrics_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "core/concerns/i_metrics.h"
#include "observability/metrics_collector.h"

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Prometheus/MetricsCollector adapter implementation of IMetrics.
 *
 * Bridges the generic IMetrics interface to the ThemisDB MetricsCollector
 * singleton, which exposes metrics in Prometheus text-exposition format
 * suitable for scraping by a Prometheus server (e.g. inside a Kubernetes
 * cluster via a ServiceMonitor or PodMonitor CRD).
 *
 * All IMetrics operations are forwarded to the MetricsCollector; the
 * adapter itself is stateless beyond holding a reference to the singleton.
 * The adapter is therefore only as durable as the in-process collector; it
 * does not persist metrics across process restarts.
 */
class PrometheusMetricsAdapter : public IMetrics {
public:
    PrometheusMetricsAdapter()
        : collector_(observability::MetricsCollector::getInstance()) {}

    // -----------------------------------------------------------------------
    // Counter
    // -----------------------------------------------------------------------

    void incrementCounter(const std::string& name, int64_t value = 1,
                          const Labels& labels = {}) override {
        collector_.addCounter(name, value, labels);
    }

    // -----------------------------------------------------------------------
    // Gauge
    // -----------------------------------------------------------------------

    void setGauge(const std::string& name, double value,
                  const Labels& labels = {}) override {
        collector_.setGauge(name, value, labels);
    }

    void incrementGauge(const std::string& name, double delta,
                        const Labels& labels = {}) override {
        collector_.modifyGauge(name, delta, labels);
    }

    void decrementGauge(const std::string& name, double delta,
                        const Labels& labels = {}) override {
        collector_.modifyGauge(name, -delta, labels);
    }

    // -----------------------------------------------------------------------
    // Histogram
    // -----------------------------------------------------------------------

    void observeHistogram(const std::string& name, double value,
                          const Labels& labels = {}) override {
        collector_.observeHistogram(name, value, labels);
    }

    // -----------------------------------------------------------------------
    // Convenience helpers
    // -----------------------------------------------------------------------

    void recordLatency(const std::string& operation, double latencyMs,
                       const Labels& labels = {}) override {
        collector_.observeHistogram(operation + "_latency_ms", latencyMs, labels);
    }

    void recordError(const std::string& operation,
                     const Labels& labels = {}) override {
        collector_.addCounter(operation + "_errors_total", 1, labels);
    }

    void recordSuccess(const std::string& operation,
                       const Labels& labels = {}) override {
        collector_.addCounter(operation + "_success_total", 1, labels);
    }

    // -----------------------------------------------------------------------
    // Export and reset
    // -----------------------------------------------------------------------

    std::string exportMetrics() const override {
        return collector_.getPrometheusMetrics();
    }

    void reset() override {
        collector_.reset();
    }

    // -----------------------------------------------------------------------
    // Lifecycle hooks
    // -----------------------------------------------------------------------

    /**
     * @brief Flush the adapter state.
     *
     * MetricsCollector is pull-based (Prometheus scrapes); there is no
     * network push to force here, so this is intentionally a no-op.
     */
    void flush() noexcept override {
        // MetricsCollector is pull-based (Prometheus scrapes); no push needed.
    }

    /**
     * @brief Reset the in-process collector and release adapter state.
     *
     * After shutdown(), the collector is reset so subsequent scrapes start
     * from an empty metric set unless the process recreates metrics first.
     */
    void shutdown() noexcept override {
        collector_.reset();
    }

    /**
     * @brief Report whether the in-process collector can be used.
     *
     * The adapter is healthy as long as the singleton exists in-process.
     */
    ProbeResult isHealthy() const override {
        return ProbeResult::healthy();
    }

private:
    observability::MetricsCollector& collector_;
};

} // namespace concerns
} // namespace core
} // namespace themis
