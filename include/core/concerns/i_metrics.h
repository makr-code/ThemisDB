/**
 * @file i_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 89/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

class IMetrics {
public:
    using Labels = std::map<std::string, std::string>;

    /**
     * @brief IMetrics.
     * @return Return value.
     */
    virtual ~IMetrics() = default;

    // -----------------------------------------------------------------------
    // Counter operations (monotonically increasing)
    // -----------------------------------------------------------------------

    virtual void incrementCounter(const std::string& name, int64_t value = 1, const Labels& labels = {}) = 0;

    // -----------------------------------------------------------------------
    // Gauge operations (can go up or down)
    // -----------------------------------------------------------------------

    virtual void setGauge(const std::string& name, double value, const Labels& labels = {}) = 0;

    virtual void incrementGauge(const std::string& name, double delta, const Labels& labels = {}) = 0;

    virtual void decrementGauge(const std::string& name, double delta, const Labels& labels = {}) = 0;

    // -----------------------------------------------------------------------
    // Histogram operations (track value distributions)
    // -----------------------------------------------------------------------

    virtual void observeHistogram(const std::string& name, double value, const Labels& labels = {}) = 0;

    // -----------------------------------------------------------------------
    // High-level convenience methods
    // -----------------------------------------------------------------------

    virtual void recordLatency(const std::string& operation, double latencyMs, const Labels& labels = {}) = 0;

    virtual void recordError(const std::string& operation, const Labels& labels = {}) = 0;

    virtual void recordSuccess(const std::string& operation, const Labels& labels = {}) = 0;

    // -----------------------------------------------------------------------
    // Export and reset
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual std::string exportMetrics() const = 0;

    /**
     * @brief Reset the modification detection flag.
     */
    virtual void reset() = 0;

    // Lifecycle hooks
    virtual void flush() noexcept {}

    virtual void shutdown() noexcept {}

    virtual ProbeResult isHealthy() const { return ProbeResult::healthy(); }
};

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
