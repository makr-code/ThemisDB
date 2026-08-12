/**
 * @file config_metrics_exporter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <functional>
#include <mutex>

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
    using GaugeSinkFn = std::function<void(const std::string& name, double value)>;

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

    /// Register an optional gauge sink used by lightweight test builds.
    /// Thread-safe; sink exceptions are ignored by updateMetricsCollector().
    static void setGaugeSinkFn(GaugeSinkFn fn) {
        std::lock_guard<std::mutex> lk(gaugeSinkFnMutex());
        gaugeSinkFnStorage() = std::move(fn);
    }

private:
    static std::mutex& gaugeSinkFnMutex() {
        static std::mutex m;
        return m;
    }
    static GaugeSinkFn& gaugeSinkFnStorage() {
        static GaugeSinkFn fn;
        return fn;
    }
};

} // namespace config
} // namespace themis
