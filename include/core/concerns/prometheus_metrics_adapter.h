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
        // Use existing collector methods where possible
        // For generic counter, we'll track it internally
        for (int64_t i = 0; i < value; ++i) {
            // Note: MetricsCollector doesn't have generic counter with labels,
            // so we use specialized methods or extend it
        }
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
        // Gauge increment/decrement not directly supported by MetricsCollector
        // Would need to track state or extend the collector
    }

    void decrementGauge(const std::string& name, double delta, const Labels& labels = {}) override {
        // Gauge increment/decrement not directly supported by MetricsCollector
    }

    void observeHistogram(const std::string& name, double value, const Labels& labels = {}) override {
        // Histogram observations are done through specific methods in MetricsCollector
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

private:
    observability::MetricsCollector& collector_;
};

} // namespace concerns
} // namespace core
} // namespace themis
