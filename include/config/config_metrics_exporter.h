/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            config_metrics_exporter.h                          ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-14 11:23:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     100                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • aeb43de031  2026-04-12  feat(config): create include/config/ public header direct... ║
    • 985dc57d92  2026-03-14  fix: harden config metrics exporter compatibility ║
    • 535cee36dc  2026-03-13  feat: export config metrics via Prometheus registry ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 77c590e6e3  2026-02-24  audit(config): fix all gaps found in Prometheus metrics e... ║
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
 *   themis_config_legacy_fallbacks_all_total - counter (aggregate, no category label)
 *   themis_config_new_path_hits_total        - counter (backward compatibility)
 *   themis_config_unmapped_requests_total    - counter
 *   themis_config_cache_hits_total           - counter (backward compatibility)
 *   themis_config_cache_misses_total         - counter (backward compatibility)
 *   themis_config_cache_hit_ratio            - gauge (derived)
 *   themis_config_cache_size                 - gauge (info, backward compatibility)
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
     * This performs a pure read of ConfigPathResolver counters unless
     * registerWithRegistry() has been called with a Prometheus registry,
     * in which case collect() will also update the registered counters
     * using deltas (stateful) before returning serialized text. Suitable
     * for use as a pull-model scrape target.
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
     * handlers can serialize the registry without additional setup. No-op when
     * Prometheus support (THEMIS_HAS_PROMETHEUS) is not available.
     */
    static void registerWithRegistry(const std::shared_ptr<prometheus::Registry>& registry);
};

} // namespace config
} // namespace themis
