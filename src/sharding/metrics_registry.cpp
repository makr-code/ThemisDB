/**
 * @file metrics_registry.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


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
