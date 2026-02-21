/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prometheus_metrics_adapter.h                       ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:34:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     133                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
 * Wraps the existing MetricsCollector to implement the IMetrics interface.
 */
class PrometheusMetricsAdapter : public IMetrics {
public:
    PrometheusMetricsAdapter()
        : collector_(observability::MetricsCollector::getInstance()) {}

    void incrementCounter(const std::string& name, int64_t value = 1, const Labels& labels = {}) override {
        // Note: MetricsCollector doesn't have generic counter with labels.
        // This adapter provides compatibility by mapping to specific methods
        // or could be extended to add generic counter support.
        // For now, this is a no-op for generic counters.
    }

    void setGauge(const std::string& name, double value, const Labels& labels = {}) override {
        // Map to existing methods where applicable
        if (name == "memory_usage") {
            collector_.recordMemoryUsage(static_cast<size_t>(value));
        } else if (name == "cpu_usage") {
            collector_.recordCPUUsage(value);
        }
    }

    void incrementGauge(const std::string& name, double delta, const Labels& labels = {}) override {
        // Note: MetricsCollector doesn't support generic gauge increment.
        // To fully implement, would need to maintain gauge state or extend MetricsCollector.
        // For now, this provides interface compatibility.
    }

    void decrementGauge(const std::string& name, double delta, const Labels& labels = {}) override {
        // Note: MetricsCollector doesn't support generic gauge decrement.
        // To fully implement, would need to maintain gauge state or extend MetricsCollector.
    }

    void observeHistogram(const std::string& name, double value, const Labels& labels = {}) override {
        // Note: Histogram observations in MetricsCollector are done through specific methods.
        // Generic histogram support would require extending MetricsCollector.
    }

    void recordLatency(const std::string& operation, double latencyMs, const Labels& labels = {}) override {
        // Map to existing methods based on operation type
        if (labels.count("type") > 0) {
            const auto& type = labels.at("type");
            if (type == "query") {
                collector_.recordQuery(operation, latencyMs, 0);
            } else if (type == "tsstore") {
                // Use appropriate TSStore method
            }
        }
    }

    void recordError(const std::string& operation, const Labels& labels = {}) override {
        // Track errors using existing methods
        if (operation == "auth") {
            collector_.recordAuthAttempt(false);
        }
    }

    void recordSuccess(const std::string& operation, const Labels& labels = {}) override {
        // Track successes using existing methods
        if (operation == "auth") {
            collector_.recordAuthAttempt(true);
        }
    }

    std::string exportMetrics() const override {
        return collector_.getPrometheusMetrics();
    }

    void reset() override {
        collector_.reset();
    }

    // Lifecycle hooks
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
