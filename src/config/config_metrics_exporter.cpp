/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            config_metrics_exporter.cpp                        ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 03:57:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     186                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • db0c5d24a  2026-02-24  feat(config): complete METADATA_TABLE, harden symlink val... ║
    • 77c590e6e  2026-02-24  audit(config): fix all gaps found in Prometheus metrics e... ║
    • 71e2d24ae  2026-02-24  feat(config): implement Prometheus metrics exporter for c... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "config/config_metrics_exporter.h"
#include "config/config_path_resolver.h"
#include "observability/metrics_collector.h"
#include <map>
#include <mutex>
#include <sstream>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/registry.h>
#include <prometheus/text_serializer.h>

namespace themis {
namespace config {

namespace {
struct RegisteredMetrics {
    prometheus::Counter* resolution_hits{nullptr};
    prometheus::Counter* resolution_misses{nullptr};
    prometheus::Counter* legacy_fallbacks{nullptr};
    prometheus::Counter* unmapped_requests{nullptr};
    prometheus::Gauge* cache_hit_ratio{nullptr};
    prometheus::Gauge* cache_capacity{nullptr};
    prometheus::Gauge* cache_ttl_seconds{nullptr};
    std::map<std::string, prometheus::Counter*> legacy_by_category;
};

std::shared_ptr<prometheus::Registry> g_registry;
RegisteredMetrics g_metrics;
std::mutex g_registry_mutex;

// Track last published counter values so we can increment prometheus::Counter
// with the delta rather than setting absolute values.
struct CounterSnapshot {
    uint64_t hits{0};
    uint64_t misses{0};
    uint64_t legacy{0};
    uint64_t unmapped{0};
    std::map<std::string, uint64_t> legacy_by_category;
};

CounterSnapshot g_prev_snapshot;
} // namespace

std::string ConfigMetricsExporter::collect() {
    const auto& m = ConfigPathResolver::metrics();

    const uint64_t hits        = m.resolution_hits.load(std::memory_order_relaxed);
    const uint64_t misses      = m.resolution_misses.load(std::memory_order_relaxed);
    const uint64_t fallbacks   = m.legacy_fallbacks.load(std::memory_order_relaxed);
    const uint64_t unmapped    = m.unmapped_requests.load(std::memory_order_relaxed);
    const uint64_t cache_hits  = m.cache_hits.load(std::memory_order_relaxed);
    const uint64_t cache_misses = m.cache_misses.load(std::memory_order_relaxed);

    const auto cache_stats = ConfigPathResolver::cacheStats();
    const uint64_t cache_total = cache_hits + cache_misses;
    const double cache_hit_ratio =
        cache_total > 0 ? static_cast<double>(cache_hits) / static_cast<double>(cache_total) : 0.0;

    const auto per_category = ConfigPathResolver::legacyFallbacksByCategory();

    auto delta_for = [](uint64_t current, uint64_t previous) {
        if (current >= previous) {
            return current - previous;
        }
        // Counter reset detected; avoid re-adding the current value.
        return static_cast<uint64_t>(0);
    };

    std::lock_guard<std::mutex> lock(g_registry_mutex);
    if (g_registry) {
        // Update counters with deltas to avoid double-counting
        if (g_metrics.resolution_hits) {
            const uint64_t delta = delta_for(hits, g_prev_snapshot.hits);
            g_metrics.resolution_hits->Increment(static_cast<double>(delta));
            g_prev_snapshot.hits = hits;
        }
        if (g_metrics.resolution_misses) {
            const uint64_t delta = delta_for(misses, g_prev_snapshot.misses);
            g_metrics.resolution_misses->Increment(static_cast<double>(delta));
            g_prev_snapshot.misses = misses;
        }
        if (g_metrics.legacy_fallbacks) {
            const uint64_t delta = delta_for(fallbacks, g_prev_snapshot.legacy);
            g_metrics.legacy_fallbacks->Increment(static_cast<double>(delta));
            g_prev_snapshot.legacy = fallbacks;
        }
        if (g_metrics.unmapped_requests) {
            const uint64_t delta = delta_for(unmapped, g_prev_snapshot.unmapped);
            g_metrics.unmapped_requests->Increment(static_cast<double>(delta));
            g_prev_snapshot.unmapped = unmapped;
        }
        if (g_metrics.cache_hit_ratio) {
            g_metrics.cache_hit_ratio->Set(cache_hit_ratio);
        }
        if (g_metrics.cache_capacity) {
            g_metrics.cache_capacity->Set(static_cast<double>(cache_stats.capacity));
        }
        if (g_metrics.cache_ttl_seconds) {
            g_metrics.cache_ttl_seconds->Set(static_cast<double>(ConfigPathResolver::currentCacheConfig().ttl_seconds));
        }

        for (const auto& [category, count] : per_category) {
            auto prev_it = g_prev_snapshot.legacy_by_category.find(category);
            const uint64_t prev = (prev_it == g_prev_snapshot.legacy_by_category.end()) ? 0 : prev_it->second;
            auto metric_it = g_metrics.legacy_by_category.find(category);
            if (metric_it != g_metrics.legacy_by_category.end()) {
                const uint64_t delta = delta_for(count, prev);
                metric_it->second->Increment(static_cast<double>(delta));
            }
            g_prev_snapshot.legacy_by_category[category] = count;
        }

        prometheus::TextSerializer serializer;
        const auto collected = g_registry->Collect();
        std::string serialized = serializer.Serialize(collected);
        if (!serialized.empty() && serialized.back() != '\n') {
            serialized.push_back('\n');
        }
        return serialized;
    }

    std::ostringstream out;

    // Resolution hits
    out << "# HELP themis_config_resolution_hits_total "
           "Total number of successful config path resolutions.\n"
        << "# TYPE themis_config_resolution_hits_total counter\n"
        << "themis_config_resolution_hits_total " << hits << "\n";

    // Resolution misses
    out << "# HELP themis_config_resolution_misses_total "
           "Total number of failed config path resolutions (path not found).\n"
        << "# TYPE themis_config_resolution_misses_total counter\n"
        << "themis_config_resolution_misses_total " << misses << "\n";

    // Legacy fallbacks (per category)
    out << "# HELP themis_config_legacy_fallbacks_total "
           "Total number of times a legacy config path was used as fallback.\n"
        << "# TYPE themis_config_legacy_fallbacks_total counter\n";
    for (const auto& [cat, count] : per_category) {
        out << "themis_config_legacy_fallbacks_total{category=\"" << cat << "\"} "
            << count << "\n";
    }

    // Unmapped requests
    out << "# HELP themis_config_unmapped_requests_total "
           "Total number of resolution requests for paths with no mapping entry.\n"
        << "# TYPE themis_config_unmapped_requests_total counter\n"
        << "themis_config_unmapped_requests_total " << unmapped << "\n";

    // Cache hit ratio (derived gauge)
    out << "# HELP themis_config_cache_hit_ratio "
           "Ratio of cache hits to total cache lookups (0.0–1.0).\n"
        << "# TYPE themis_config_cache_hit_ratio gauge\n"
        << "themis_config_cache_hit_ratio " << cache_hit_ratio << "\n";

    // Cache capacity (info)
    out << "# HELP themis_config_cache_capacity "
           "Maximum capacity of the config path LRU cache.\n"
        << "# TYPE themis_config_cache_capacity gauge\n"
        << "themis_config_cache_capacity " << cache_stats.capacity << "\n";

    // Cache TTL (info) — use runtime value in case env var overrode the default
    const auto cache_cfg = ConfigPathResolver::currentCacheConfig();
    out << "# HELP themis_config_cache_ttl_seconds "
           "Time-to-live of entries in the config path LRU cache, in seconds.\n"
        << "# TYPE themis_config_cache_ttl_seconds gauge\n"
        << "themis_config_cache_ttl_seconds " << cache_cfg.ttl_seconds << "\n";

    return out.str();
}

void ConfigMetricsExporter::updateMetricsCollector() {
#ifdef THEMIS_TEST_BUILD
    // Focused config tests build only the config module; MetricsCollector and
    // its dependencies are not linked in that configuration. This stub keeps
    // the test build lightweight while production builds execute the real sync.
    // Callers in test builds should not expect this function to mutate any
    // global metrics state.
    return;
#else
    auto& collector = observability::MetricsCollector::getInstance();
    const auto& m = ConfigPathResolver::metrics();

    const uint64_t hits        = m.resolution_hits.load(std::memory_order_relaxed);
    const uint64_t misses      = m.resolution_misses.load(std::memory_order_relaxed);
    const uint64_t fallbacks   = m.legacy_fallbacks.load(std::memory_order_relaxed);
    const uint64_t unmapped    = m.unmapped_requests.load(std::memory_order_relaxed);

    const auto cache_stats = ConfigPathResolver::cacheStats();
    const uint64_t cache_total = cache_stats.hits + cache_stats.misses;
    const double cache_hit_ratio =
        cache_total > 0 ? static_cast<double>(cache_stats.hits) / static_cast<double>(cache_total) : 0.0;

    // Push current counter values as gauges in the MetricsCollector so they
    // appear in the server-wide scrape endpoint.  Gauge names use the
    // "themis_config_*_current" convention to distinguish them from the
    // proper Prometheus counter series emitted by collect().
    collector.setGauge("themis_config_resolution_hits_current",    static_cast<double>(hits));
    collector.setGauge("themis_config_resolution_misses_current",  static_cast<double>(misses));
    collector.setGauge("themis_config_legacy_fallbacks_current",   static_cast<double>(fallbacks));
    collector.setGauge("themis_config_unmapped_requests_current",  static_cast<double>(unmapped));
    collector.setGauge("themis_config_cache_hit_ratio",            cache_hit_ratio);
    collector.setGauge("themis_config_cache_capacity",             static_cast<double>(cache_stats.capacity));
    const auto cache_cfg = ConfigPathResolver::currentCacheConfig();
    collector.setGauge("themis_config_cache_ttl_seconds",          static_cast<double>(cache_cfg.ttl_seconds));

    // Per-category legacy fallback gauges
    for (const auto& [cat, count] : ConfigPathResolver::legacyFallbacksByCategory()) {
        collector.setGauge("themis_config_legacy_fallbacks_by_category_current",
                           static_cast<double>(count), {{"category", cat}});
    }
#endif
}

void ConfigMetricsExporter::registerWithRegistry(const std::shared_ptr<prometheus::Registry>& registry) {
    std::lock_guard<std::mutex> lock(g_registry_mutex);
    g_registry = registry;
    g_prev_snapshot = {};
    if (!g_registry) {
        return;
    }

    g_metrics = {};

    auto& hits_family = prometheus::BuildCounter()
        .Name("themis_config_resolution_hits_total")
        .Help("Total number of successful config path resolutions.")
        .Register(*g_registry);
    g_metrics.resolution_hits = &hits_family.Add({});

    auto& misses_family = prometheus::BuildCounter()
        .Name("themis_config_resolution_misses_total")
        .Help("Total number of failed config path resolutions (path not found).")
        .Register(*g_registry);
    g_metrics.resolution_misses = &misses_family.Add({});

    auto& legacy_family = prometheus::BuildCounter()
        .Name("themis_config_legacy_fallbacks_total")
        .Help("Total number of times a legacy config path was used as fallback.")
        .Register(*g_registry);
    for (const auto& category : ConfigPathResolver::legacyFallbackCategories()) {
        g_metrics.legacy_by_category[category] = &legacy_family.Add({{"category", category}});
    }
    // Also expose an aggregate series without label for convenience
    g_metrics.legacy_fallbacks = &legacy_family.Add({});

    auto& unmapped_family = prometheus::BuildCounter()
        .Name("themis_config_unmapped_requests_total")
        .Help("Total number of resolution requests for paths with no mapping entry.")
        .Register(*g_registry);
    g_metrics.unmapped_requests = &unmapped_family.Add({});

    auto& cache_hit_ratio_family = prometheus::BuildGauge()
        .Name("themis_config_cache_hit_ratio")
        .Help("Ratio of cache hits to total cache lookups (0.0–1.0).")
        .Register(*g_registry);
    g_metrics.cache_hit_ratio = &cache_hit_ratio_family.Add({});

    auto& cache_capacity_family = prometheus::BuildGauge()
        .Name("themis_config_cache_capacity")
        .Help("Maximum capacity of the config path LRU cache.")
        .Register(*g_registry);
    g_metrics.cache_capacity = &cache_capacity_family.Add({});

    auto& cache_ttl_family = prometheus::BuildGauge()
        .Name("themis_config_cache_ttl_seconds")
        .Help("Time-to-live of entries in the config path LRU cache, in seconds.")
        .Register(*g_registry);
    g_metrics.cache_ttl_seconds = &cache_ttl_family.Add({});
}

} // namespace config
} // namespace themis
