#include "config/config_metrics_exporter.h"
#include "config/config_path_resolver.h"
#include "observability/metrics_collector.h"
#include <map>
#include <sstream>

namespace themis {
namespace config {

std::string ConfigMetricsExporter::collect() {
    const auto& m = ConfigPathResolver::metrics();

    const uint64_t hits        = m.resolution_hits.load(std::memory_order_relaxed);
    const uint64_t misses      = m.resolution_misses.load(std::memory_order_relaxed);
    const uint64_t fallbacks   = m.legacy_fallbacks.load(std::memory_order_relaxed);
    const uint64_t new_hits    = m.new_path_hits.load(std::memory_order_relaxed);
    const uint64_t unmapped    = m.unmapped_requests.load(std::memory_order_relaxed);
    const uint64_t cache_hits  = m.cache_hits.load(std::memory_order_relaxed);
    const uint64_t cache_misses = m.cache_misses.load(std::memory_order_relaxed);

    const auto cache_stats = ConfigPathResolver::cacheStats();
    const uint64_t cache_total = cache_hits + cache_misses;
    const double cache_hit_ratio =
        cache_total > 0 ? static_cast<double>(cache_hits) / static_cast<double>(cache_total) : 0.0;

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

    // Legacy fallbacks
    out << "# HELP themis_config_legacy_fallbacks_total "
           "Total number of times a legacy config path was used as fallback.\n"
        << "# TYPE themis_config_legacy_fallbacks_total counter\n"
        << "themis_config_legacy_fallbacks_total " << fallbacks << "\n";

    // New-path hits
    out << "# HELP themis_config_new_path_hits_total "
           "Total number of times the new (canonical) config path was resolved.\n"
        << "# TYPE themis_config_new_path_hits_total counter\n"
        << "themis_config_new_path_hits_total " << new_hits << "\n";

    // Unmapped requests
    out << "# HELP themis_config_unmapped_requests_total "
           "Total number of resolution requests for paths with no mapping entry.\n"
        << "# TYPE themis_config_unmapped_requests_total counter\n"
        << "themis_config_unmapped_requests_total " << unmapped << "\n";

    // Cache hits
    out << "# HELP themis_config_cache_hits_total "
           "Total number of config path LRU cache hits.\n"
        << "# TYPE themis_config_cache_hits_total counter\n"
        << "themis_config_cache_hits_total " << cache_hits << "\n";

    // Cache misses
    out << "# HELP themis_config_cache_misses_total "
           "Total number of config path LRU cache misses.\n"
        << "# TYPE themis_config_cache_misses_total counter\n"
        << "themis_config_cache_misses_total " << cache_misses << "\n";

    // Cache hit ratio (derived gauge)
    out << "# HELP themis_config_cache_hit_ratio "
           "Ratio of cache hits to total cache lookups (0.0–1.0).\n"
        << "# TYPE themis_config_cache_hit_ratio gauge\n"
        << "themis_config_cache_hit_ratio " << cache_hit_ratio << "\n";

    // Cache current size
    out << "# HELP themis_config_cache_size "
           "Current number of entries in the config path LRU cache.\n"
        << "# TYPE themis_config_cache_size gauge\n"
        << "themis_config_cache_size " << cache_stats.size << "\n";

    // Cache capacity (info)
    out << "# HELP themis_config_cache_capacity "
           "Maximum capacity of the config path LRU cache.\n"
        << "# TYPE themis_config_cache_capacity gauge\n"
        << "themis_config_cache_capacity " << cache_stats.capacity << "\n";

    // Cache TTL (info)
    out << "# HELP themis_config_cache_ttl_seconds "
           "Default time-to-live of entries in the config path LRU cache, in seconds.\n"
        << "# TYPE themis_config_cache_ttl_seconds gauge\n"
        << "themis_config_cache_ttl_seconds " << ConfigPathResolver::kCacheTtlSeconds << "\n";

    // Per-category legacy fallback counters (derived from deprecation aggregator)
    const auto dep_report = ConfigPathResolver::deprecationReport();
    if (!dep_report.empty()) {
        // Aggregate usage_count by category
        std::map<std::string, uint64_t> by_category;
        for (const auto& entry : dep_report) {
            by_category[entry.category.empty() ? "unknown" : entry.category] += entry.usage_count;
        }
        out << "# HELP themis_config_legacy_fallbacks_by_category_total "
               "Total legacy config path fallbacks broken down by config category.\n"
            << "# TYPE themis_config_legacy_fallbacks_by_category_total counter\n";
        for (const auto& [cat, count] : by_category) {
            out << "themis_config_legacy_fallbacks_by_category_total{category=\"" << cat << "\"} "
                << count << "\n";
        }
    }

    return out.str();
}

void ConfigMetricsExporter::updateMetricsCollector() {
    auto& collector = observability::MetricsCollector::getInstance();
    const auto& m = ConfigPathResolver::metrics();

    const uint64_t hits        = m.resolution_hits.load(std::memory_order_relaxed);
    const uint64_t misses      = m.resolution_misses.load(std::memory_order_relaxed);
    const uint64_t fallbacks   = m.legacy_fallbacks.load(std::memory_order_relaxed);
    const uint64_t new_hits    = m.new_path_hits.load(std::memory_order_relaxed);
    const uint64_t unmapped    = m.unmapped_requests.load(std::memory_order_relaxed);
    const uint64_t cache_hits  = m.cache_hits.load(std::memory_order_relaxed);
    const uint64_t cache_misses = m.cache_misses.load(std::memory_order_relaxed);

    const auto cache_stats = ConfigPathResolver::cacheStats();
    const uint64_t cache_total = cache_hits + cache_misses;
    const double cache_hit_ratio =
        cache_total > 0 ? static_cast<double>(cache_hits) / static_cast<double>(cache_total) : 0.0;

    // Push current counter values as gauges in the MetricsCollector so they
    // appear in the server-wide scrape endpoint.  Gauge names use the
    // "themis_config_*_current" convention to distinguish them from the
    // proper Prometheus counter series emitted by collect().
    collector.setGauge("themis_config_resolution_hits_current",    static_cast<double>(hits));
    collector.setGauge("themis_config_resolution_misses_current",  static_cast<double>(misses));
    collector.setGauge("themis_config_legacy_fallbacks_current",   static_cast<double>(fallbacks));
    collector.setGauge("themis_config_new_path_hits_current",      static_cast<double>(new_hits));
    collector.setGauge("themis_config_unmapped_requests_current",  static_cast<double>(unmapped));
    collector.setGauge("themis_config_cache_hits_current",         static_cast<double>(cache_hits));
    collector.setGauge("themis_config_cache_misses_current",       static_cast<double>(cache_misses));
    collector.setGauge("themis_config_cache_hit_ratio",            cache_hit_ratio);
    collector.setGauge("themis_config_cache_size",                 static_cast<double>(cache_stats.size));
    collector.setGauge("themis_config_cache_capacity",             static_cast<double>(cache_stats.capacity));
    collector.setGauge("themis_config_cache_ttl_seconds",          static_cast<double>(ConfigPathResolver::kCacheTtlSeconds));

    // Per-category legacy fallback gauges
    const auto dep_report = ConfigPathResolver::deprecationReport();
    std::map<std::string, uint64_t> by_category;
    for (const auto& entry : dep_report) {
        by_category[entry.category.empty() ? "unknown" : entry.category] += entry.usage_count;
    }
    for (const auto& [cat, count] : by_category) {
        collector.setGauge("themis_config_legacy_fallbacks_by_category_current",
                           static_cast<double>(count), {{"category", cat}});
    }
}

} // namespace config
} // namespace themis
