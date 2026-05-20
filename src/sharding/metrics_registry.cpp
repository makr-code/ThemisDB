/*
 * ThemisDB | File: metrics_registry.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=5, M=1, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
