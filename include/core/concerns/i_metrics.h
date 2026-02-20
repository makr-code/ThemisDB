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

    // Counter operations (monotonically increasing)
    virtual void incrementCounter(const std::string& name, int64_t value = 1, const Labels& labels = {}) = 0;

    // Gauge operations (can go up or down)
    virtual void setGauge(const std::string& name, double value, const Labels& labels = {}) = 0;
    virtual void incrementGauge(const std::string& name, double delta, const Labels& labels = {}) = 0;
    virtual void decrementGauge(const std::string& name, double delta, const Labels& labels = {}) = 0;

    // Histogram operations (track distributions)
    virtual void observeHistogram(const std::string& name, double value, const Labels& labels = {}) = 0;

    // High-level metrics methods
    virtual void recordLatency(const std::string& operation, double latencyMs, const Labels& labels = {}) = 0;
    virtual void recordError(const std::string& operation, const Labels& labels = {}) = 0;
    virtual void recordSuccess(const std::string& operation, const Labels& labels = {}) = 0;

    // Export metrics (e.g., Prometheus text format)
    virtual std::string exportMetrics() const = 0;

    // Reset metrics (for testing)
    virtual void reset() = 0;

    // Lifecycle hooks
    /**
     * @brief Flush any pending metric observations to the backend.
     *
     * Call before shutdown() to ensure the final snapshot is published.
     * Default is a no-op.
     */
    virtual void flush() {}

    /**
     * @brief Shut down the metrics backend and release resources.
     *
     * Default is a no-op.
     */
    virtual void shutdown() {}

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

    double elapsedMs() const {
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
