/*
 * ThemisDB | File: prometheus_metrics_adapter.h | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 126
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: #2677 [core] Prometheus-compatible metrics adapter (2026-03-12T05:54:54Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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

    void flush() noexcept override {
        // MetricsCollector is pull-based (Prometheus scrapes); no push needed.
    }

    void shutdown() noexcept override {
        collector_.reset();
    }

    ProbeResult isHealthy() const override {
        // The Prometheus adapter is healthy as long as the collector singleton
        // is accessible (it is always constructed in-process).
        return ProbeResult::healthy();
    }

private:
    observability::MetricsCollector& collector_;
};

} // namespace concerns
} // namespace core
} // namespace themis
