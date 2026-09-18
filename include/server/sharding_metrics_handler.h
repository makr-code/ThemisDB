/**
 * @file sharding_metrics_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <memory>

namespace themis {

namespace sharding {
class PrometheusMetrics;
class SLOMonitor;
class ShardRepairEngine;
}

namespace server {

/**
 * Handler for exposing sharding metrics in Prometheus format.
 * Enhanced in Phase 1.5 to include SLO monitoring.
 * Enhanced in v1.5 to include Shard Repair / Anti-Entropy metrics.
 */
class ShardingMetricsHandler {
public:
    explicit ShardingMetricsHandler(
        std::shared_ptr<sharding::PrometheusMetrics> metrics,
        std::shared_ptr<sharding::SLOMonitor> slo_monitor = nullptr
    );

    /**
     * Optionally attach a ShardRepairEngine so that its metrics are
     * appended to the main Prometheus scrape response.
     * @brief TBD: Describe setRepairEngine.
     * @param[in] repair_engine Input parameter.
     */
    void setRepairEngine(std::shared_ptr<sharding::ShardRepairEngine> repair_engine);

    /**
     * Get all sharding metrics in Prometheus format with annotations.
     * Includes repair metrics when a ShardRepairEngine has been set.
     * @return Prometheus-formatted metrics with HELP and TYPE
     * @brief TBD: Describe getMetrics.
     */
    std::string getMetrics() const;

    /**
     * Get plain metrics without annotations
     * @return Plain Prometheus metrics
     * @brief TBD: Describe getMetricsPlain.
     */
    std::string getMetricsPlain() const;
    
    /**
     * Get SLO status in JSON format
     * @return JSON with SLO compliance and error budgets
     * @brief TBD: Describe getSLOStatus.
     */
    std::string getSLOStatus() const;
    
    /**
     * Get SLO status in Prometheus format
     * @return Prometheus-formatted SLO metrics
     * @brief TBD: Describe getSLOMetrics.
     */
    std::string getSLOMetrics() const;

    /**
     * Get repair/anti-entropy metrics in Prometheus format.
     * Returns an empty string when no ShardRepairEngine is attached.
     * @brief TBD: Describe getRepairMetrics.
     * @return Return value.
     */
    std::string getRepairMetrics() const;

private:
    std::shared_ptr<sharding::PrometheusMetrics> metrics_;
    std::shared_ptr<sharding::SLOMonitor> slo_monitor_;
    std::shared_ptr<sharding::ShardRepairEngine> repair_engine_;
};

} // namespace server
} // namespace themis
