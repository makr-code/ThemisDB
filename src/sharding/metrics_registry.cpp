#include "sharding/metrics_registry.h"
#include "sharding/prometheus_metrics.h"

namespace themis {
namespace sharding {

void ShardingMetricsRegistry::registerMetrics(std::shared_ptr<PrometheusMetrics> metrics) {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_ = metrics;
}

std::shared_ptr<PrometheusMetrics> ShardingMetricsRegistry::getMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return metrics_;
}

std::string ShardingMetricsRegistry::getMetricsString() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!metrics_) {
        return "";
    }
    return metrics_->getMetricsWithAnnotations();
}

} // namespace sharding
} // namespace themis
