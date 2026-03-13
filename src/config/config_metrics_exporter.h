/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            config_metrics_exporter.h                          ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 03:57:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     82                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 77c590e6e  2026-02-24  audit(config): fix all gaps found in Prometheus metrics e... ║
    • 71e2d24ae  2026-02-24  feat(config): implement Prometheus metrics exporter for c... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <memory>
#include <string>

namespace prometheus {
class Registry;
}

namespace themis {
namespace config {

/**
 * ConfigMetricsExporter exposes ConfigPathResolver metrics in Prometheus
 * text-exposition format and optionally pushes them to the MetricsCollector
 * singleton for integration with the server-wide Prometheus endpoint.
 *
 * All methods are thread-safe: metric reads are performed via the
 * std::atomic counters already maintained by ConfigPathResolver; no
 * additional locking is introduced on the hot path.
 *
 * Exported metric names:
 *   themis_config_resolution_hits_total      - counter
 *   themis_config_resolution_misses_total    - counter
 *   themis_config_legacy_fallbacks_total{category} - counter (per-category breakdown)
 *   themis_config_unmapped_requests_total    - counter
 *   themis_config_cache_hit_ratio            - gauge (derived)
 *   themis_config_cache_capacity             - gauge (info)
 *   themis_config_cache_ttl_seconds          - gauge (info)
 */
class ConfigMetricsExporter {
public:
    ConfigMetricsExporter() = delete;

    /**
     * Collect all config-path-resolution metrics and return them in
     * Prometheus text-exposition format (UTF-8, newline-terminated).
     *
     * This is a pure read: it reads the atomic counters from
     * ConfigPathResolver::metrics() and the LRU cache stats; no state
     * is modified.  Suitable for use as a pull-model scrape target.
     *
     * @return Prometheus text-format string ready to be served on /metrics.
     */
    static std::string collect();

    /**
     * Push the current config metrics into the MetricsCollector singleton
     * so they appear in the server-wide Prometheus scrape endpoint.
     *
     * Call this once per scrape interval (e.g. from the HTTP handler that
     * serves /metrics) to keep the MetricsCollector's values in sync with
     * the atomic counters maintained by ConfigPathResolver.
     */
    static void updateMetricsCollector();

    /**
     * Register Prometheus metric families for config path resolution in the
     * provided registry. Should be invoked during server startup so scrape
     * handlers can serialize the registry without additional setup.
     */
    static void registerWithRegistry(const std::shared_ptr<prometheus::Registry>& registry);
};

} // namespace config
} // namespace themis
