/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            metrics_registry.cpp                               ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:29:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     52                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
