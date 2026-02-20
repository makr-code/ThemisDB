#pragma once

#include <string>
#include <memory>

namespace themis {

namespace sharding {
class PrometheusMetrics;
class SLOMonitor;
}

namespace server {

/**
 * Handler for exposing sharding metrics in Prometheus format.
 * Enhanced in Phase 1.5 to include SLO monitoring.
 */
class ShardingMetricsHandler {
public:
    explicit ShardingMetricsHandler(
        std::shared_ptr<sharding::PrometheusMetrics> metrics,
        std::shared_ptr<sharding::SLOMonitor> slo_monitor = nullptr
    );

    /**
     * Get all sharding metrics in Prometheus format with annotations
     * @return Prometheus-formatted metrics with HELP and TYPE
     */
    std::string getMetrics() const;

    /**
     * Get plain metrics without annotations
     * @return Plain Prometheus metrics
     */
    std::string getMetricsPlain() const;
    
    /**
     * Get SLO status in JSON format
     * @return JSON with SLO compliance and error budgets
     */
    std::string getSLOStatus() const;
    
    /**
     * Get SLO status in Prometheus format
     * @return Prometheus-formatted SLO metrics
     */
    std::string getSLOMetrics() const;

private:
    std::shared_ptr<sharding::PrometheusMetrics> metrics_;
    std::shared_ptr<sharding::SLOMonitor> slo_monitor_;
};

} // namespace server
} // namespace themis
