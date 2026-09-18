/**
 * @file metrics_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <mutex>
#include <string>

namespace themis {
namespace sharding {

class PrometheusMetrics;

/**
 * Global registry for sharding metrics
 * Allows sharding components to register metrics that can be exported via HTTP
 */
class ShardingMetricsRegistry {
public:
    /**
     * @brief TBD: Describe instance.
     * @return Return value.
     * @details Implements instance without additional internal calls.
     */
    static ShardingMetricsRegistry& instance() {
        static ShardingMetricsRegistry instance;
        return instance;
    }

    /**
     * Register sharding metrics instance
     * @param metrics Shared pointer to PrometheusMetrics
     * @brief TBD: Describe registerMetrics.
     */
    void registerMetrics(std::shared_ptr<PrometheusMetrics> metrics);

    /**
     * Get registered metrics
     * @return Shared pointer to PrometheusMetrics, or nullptr if not registered
     * @brief TBD: Describe getMetrics.
     */
    std::shared_ptr<PrometheusMetrics> getMetrics() const;

    /**
     * Get metrics in Prometheus format
     * @return Prometheus-formatted metrics string, or empty if no metrics registered
     * @brief TBD: Describe getMetricsString.
     */
    std::string getMetricsString() const;

private:
    ShardingMetricsRegistry() = default;
    ~ShardingMetricsRegistry() = default;
    
    // Prevent copying
    ShardingMetricsRegistry(const ShardingMetricsRegistry&) = delete;
    ShardingMetricsRegistry& operator=(const ShardingMetricsRegistry&) = delete;

    mutable std::mutex mutex_;
    std::shared_ptr<PrometheusMetrics> metrics_;
};

} // namespace sharding
} // namespace themis
