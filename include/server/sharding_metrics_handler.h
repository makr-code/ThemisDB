/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            sharding_metrics_handler.h                         ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:13:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     94                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

/**
 * Handler for exposing sharding metrics in Prometheus format.
 * Enhanced in Phase 1.5 to include SLO monitoring.
 * Enhanced in v1.5 to include Shard Repair / Anti-Entropy metrics.
 */
class ShardingMetricsHandler {
public:
    explicit ShardingMetricsHandler(
        std::shared_ptr<sharding::PrometheusMetrics> metrics,
        std::shared_ptr<sharding::SLOMonitor> slo_monitor = nullptr
    );

    /**
     * Optionally attach a ShardRepairEngine so that its metrics are
     * appended to the main Prometheus scrape response.
     */
    void setRepairEngine(std::shared_ptr<sharding::ShardRepairEngine> repair_engine);

    /**
     * Get all sharding metrics in Prometheus format with annotations.
     * Includes repair metrics when a ShardRepairEngine has been set.
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

    /**
     * Get repair/anti-entropy metrics in Prometheus format.
     * Returns an empty string when no ShardRepairEngine is attached.
     */
    std::string getRepairMetrics() const;

private:
    std::shared_ptr<sharding::PrometheusMetrics> metrics_;
    std::shared_ptr<sharding::SLOMonitor> slo_monitor_;
    std::shared_ptr<sharding::ShardRepairEngine> repair_engine_;
};

} // namespace server
} // namespace themis
