#pragma once

#include <string>

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
 *   themis_config_legacy_fallbacks_total     - counter
 *   themis_config_new_path_hits_total        - counter
 *   themis_config_unmapped_requests_total    - counter
 *   themis_config_cache_hits_total           - counter
 *   themis_config_cache_misses_total         - counter
 *   themis_config_cache_hit_ratio            - gauge (derived)
 *   themis_config_cache_size                 - gauge
 *   themis_config_cache_capacity             - gauge (info)
 *   themis_config_cache_ttl_seconds          - gauge (info)
 *   themis_config_legacy_fallbacks_by_category_total{category} - counter (per-category breakdown)
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
};

} // namespace config
} // namespace themis
