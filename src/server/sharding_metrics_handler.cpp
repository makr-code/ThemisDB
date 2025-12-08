#include "server/sharding_metrics_handler.h"
#include "sharding/prometheus_metrics.h"
#include <sstream>

namespace themis {
namespace server {

ShardingMetricsHandler::ShardingMetricsHandler(
    std::shared_ptr<sharding::PrometheusMetrics> metrics)
    : metrics_(metrics) {
}

std::string ShardingMetricsHandler::getMetrics() const {
    if (!metrics_) {
        return "";
    }
    
    // Get metrics with annotations (HELP and TYPE)
    return metrics_->getMetricsWithAnnotations();
}

std::string ShardingMetricsHandler::getMetricsPlain() const {
    if (!metrics_) {
        return "";
    }
    
    // Get plain metrics without annotations
    return metrics_->getMetrics();
}

} // namespace server
} // namespace themis
