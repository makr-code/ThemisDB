#pragma once

#include <string>
#include <memory>

namespace themis {

namespace sharding {
class PrometheusMetrics;
}

namespace server {

/**
 * Handler for exposing sharding metrics in Prometheus format
 */
class ShardingMetricsHandler {
public:
    explicit ShardingMetricsHandler(
        std::shared_ptr<sharding::PrometheusMetrics> metrics
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

private:
    std::shared_ptr<sharding::PrometheusMetrics> metrics_;
};

} // namespace server
} // namespace themis
