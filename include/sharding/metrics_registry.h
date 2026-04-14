/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            metrics_registry.h                                 ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:56:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     76                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    static ShardingMetricsRegistry& instance() {
        static ShardingMetricsRegistry instance;
        return instance;
    }

    /**
     * Register sharding metrics instance
     * @param metrics Shared pointer to PrometheusMetrics
     */
    void registerMetrics(std::shared_ptr<PrometheusMetrics> metrics);

    /**
     * Get registered metrics
     * @return Shared pointer to PrometheusMetrics, or nullptr if not registered
     */
    std::shared_ptr<PrometheusMetrics> getMetrics() const;

    /**
     * Get metrics in Prometheus format
     * @return Prometheus-formatted metrics string, or empty if no metrics registered
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
