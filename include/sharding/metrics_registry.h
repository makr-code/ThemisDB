/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            metrics_registry.h                                 ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     80                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
