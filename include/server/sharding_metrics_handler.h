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

class ShardingMetricsHandler {
public:
    explicit ShardingMetricsHandler(
        std::shared_ptr<sharding::PrometheusMetrics> metrics,
        std::shared_ptr<sharding::SLOMonitor> slo_monitor = nullptr
    );

    /**
     * @brief Set Repair Engine.
     * @param[in] repair_engine Input parameter.
     */
    void setRepairEngine(std::shared_ptr<sharding::ShardRepairEngine> repair_engine);

    /**
     * @brief Get Metrics.
     * @return Return value.
     */
    std::string getMetrics() const;

    /**
     * @brief Get Metrics Plain.
     * @return Return value.
     */
    std::string getMetricsPlain() const;
    
    /**
     * @brief Get SLOStatus.
     * @return Return value.
     */
    std::string getSLOStatus() const;
    
    /**
     * @brief Get SLOMetrics.
     * @return Return value.
     */
    std::string getSLOMetrics() const;

    /**
     * @brief Get Repair Metrics.
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
