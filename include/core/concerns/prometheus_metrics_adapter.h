/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prometheus_metrics_adapter.h                       ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:06:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     140                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
