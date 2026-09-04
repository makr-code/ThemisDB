/**
 * @file config_metrics_exporter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=8; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=1, Debt=0, C=3, H=6, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "config/config_metrics_exporter.h"
#include <stdexcept>
#include "config/config_path_resolver.h"
#include "observability/metrics_collector.h"
#include <map>
#include <mutex>
#include <sstream>
#ifdef THEMIS_HAS_PROMETHEUS
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/registry.h>
#include <prometheus/text_serializer.h>
#endif

namespace themis {
namespace config {

namespace {
#ifdef THEMIS_HAS_PROMETHEUS
struct RegisteredMetrics {
    prometheus::Counter* resolution_hits{nullptr};
    prometheus::Counter* resolution_misses{nullptr};
    prometheus::Counter* legacy_fallbacks{nullptr};
    prometheus::Counter* new_path_hits{nullptr};
    prometheus::Counter* cache_hits{nullptr};
    prometheus::Counter* cache_misses{nullptr};
    prometheus::Counter* unmapped_requests{nullptr};
    prometheus::Family<prometheus::Counter>* legacy_family{nullptr};
    prometheus::Gauge* cache_hit_ratio{nullptr};
    prometheus::Gauge* cache_size{nullptr};
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
    uint64_t new_path_hits{0};
    uint64_t unmapped{0};
    uint64_t cache_hits{0};
    uint64_t cache_misses{0};
    std::map<std::string, uint64_t> legacy_by_category;
};

CounterSnapshot g_prev_snapshot;

uint64_t counterDelta(uint64_t current, uint64_t& previous) {
    if (current >= previous) {
        const auto delta = current - previous;
        previous = current;
        return delta;
    }
    // Counter reset detected; treat current as the new baseline without
    // emitting a delta to avoid double-counting.
    previous = current;
    return 0;
}
#endif // THEMIS_HAS_PROMETHEUS

} // namespace

std::string ConfigMetricsExporter::collect() {
    const auto& m = ConfigPathResolver::metrics();

    const uint64_t hits        = m.resolution_hits.load(std::memory_order_relaxed);
    const uint64_t misses      = m.resolution_misses.load(std::memory_order_relaxed);
    const uint64_t fallbacks   = m.legacy_fallbacks.load(std::memory_order_relaxed);
    const uint64_t new_path_hits = m.new_path_hits.load(std::memory_order_relaxed);
    const uint64_t unmapped    = m.unmapped_requests.load(std::memory_order_relaxed);
    const uint64_t cache_hits  = m.cache_hits.load(std::memory_order_relaxed);
    const uint64_t cache_misses = m.cache_misses.load(std::memory_order_relaxed);

    const auto cache_stats = ConfigPathResolver::cacheStats();
    const uint64_t cache_total = cache_hits + cache_misses;
    const double cache_hit_ratio =
        cache_total > 0 ? static_cast<double>(cache_hits) / static_cast<double>(cache_total) : 0.0;

    const auto per_category = ConfigPathResolver::legacyFallbacksByCategory();

#ifdef THEMIS_HAS_PROMETHEUS
    std::lock_guard<std::mutex> lock(g_registry_mutex);
    if (g_registry) {
        // Update counters with deltas to avoid double-counting
        if (g_metrics.resolution_hits) {
            const uint64_t delta = counterDelta(hits, g_prev_snapshot.hits);
            g_metrics.resolution_hits->Increment(static_cast<double>(delta));
        }
        if (g_metrics.resolution_misses) {
            const uint64_t delta = counterDelta(misses, g_prev_snapshot.misses);
            g_metrics.resolution_misses->Increment(static_cast<double>(delta));
        }
        if (g_metrics.legacy_fallbacks) {
            const uint64_t delta = counterDelta(fallbacks, g_prev_snapshot.legacy);
            g_metrics.legacy_fallbacks->Increment(static_cast<double>(delta));
        }
        if (g_metrics.new_path_hits) {
            const uint64_t delta = counterDelta(new_path_hits, g_prev_snapshot.new_path_hits);
            g_metrics.new_path_hits->Increment(static_cast<double>(delta));
        }
        if (g_metrics.unmapped_requests) {
            const uint64_t delta = counterDelta(unmapped, g_prev_snapshot.unmapped);
            g_metrics.unmapped_requests->Increment(static_cast<double>(delta));
        }
        if (g_metrics.cache_hits) {
            const uint64_t delta = counterDelta(cache_hits, g_prev_snapshot.cache_hits);
            g_metrics.cache_hits->Increment(static_cast<double>(delta));
        }
        if (g_metrics.cache_misses) {
            const uint64_t delta = counterDelta(cache_misses, g_prev_snapshot.cache_misses);
            g_metrics.cache_misses->Increment(static_cast<double>(delta));
        }
        if (g_metrics.cache_hit_ratio) {
            g_metrics.cache_hit_ratio->Set(cache_hit_ratio);
        }
        if (g_metrics.cache_size) {
            g_metrics.cache_size->Set(static_cast<double>(cache_stats.size));
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
            if (metric_it == g_metrics.legacy_by_category.end() && g_metrics.legacy_family) {
                metric_it = g_metrics.legacy_by_category.emplace(
                    category, &g_metrics.legacy_family->Add({{"category", category}})
                ).first;
            }
            if (metric_it != g_metrics.legacy_by_category.end()) {
                uint64_t prev_copy = prev;
                const uint64_t delta = counterDelta(count, prev_copy);
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
#endif

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
    out << "# HELP themis_config_legacy_fallbacks_all_total "
           "Total number of legacy config path fallbacks (aggregate across all categories).\n"
        << "# TYPE themis_config_legacy_fallbacks_all_total counter\n"
        << "themis_config_legacy_fallbacks_all_total " << fallbacks << "\n";

    // New-path hits (backward compatibility)
    out << "# HELP themis_config_new_path_hits_total "
           "Total number of times the new (canonical) config path was resolved.\n"
        << "# TYPE themis_config_new_path_hits_total counter\n"
        << "themis_config_new_path_hits_total " << new_path_hits << "\n";

    // Unmapped requests
    out << "# HELP themis_config_unmapped_requests_total "
           "Total number of resolution requests for paths with no mapping entry.\n"
        << "# TYPE themis_config_unmapped_requests_total counter\n"
        << "themis_config_unmapped_requests_total " << unmapped << "\n";

    // Cache hits/misses (backward compatibility)
    out << "# HELP themis_config_cache_hits_total "
           "Total number of cache hits for config path resolution.\n"
        << "# TYPE themis_config_cache_hits_total counter\n"
        << "themis_config_cache_hits_total " << cache_hits << "\n";

    out << "# HELP themis_config_cache_misses_total "
           "Total number of cache misses for config path resolution.\n"
        << "# TYPE themis_config_cache_misses_total counter\n"
        << "themis_config_cache_misses_total " << cache_misses << "\n";

    // Cache hit ratio (derived gauge)
    out << "# HELP themis_config_cache_hit_ratio "
           "Ratio of cache hits to total cache lookups (0.0–1.0).\n"
        << "# TYPE themis_config_cache_hit_ratio gauge\n"
        << "themis_config_cache_hit_ratio " << cache_hit_ratio << "\n";

    // Cache size (info)
    out << "# HELP themis_config_cache_size "
           "Current size of the config path LRU cache.\n"
        << "# TYPE themis_config_cache_size gauge\n"
        << "themis_config_cache_size " << cache_stats.size << "\n";

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
    const auto& m = ConfigPathResolver::metrics();
    const uint64_t hits        = m.resolution_hits.load(std::memory_order_relaxed);
    const uint64_t misses      = m.resolution_misses.load(std::memory_order_relaxed);
    const uint64_t fallbacks   = m.legacy_fallbacks.load(std::memory_order_relaxed);
    const uint64_t new_path_hits = m.new_path_hits.load(std::memory_order_relaxed);
    const uint64_t unmapped    = m.unmapped_requests.load(std::memory_order_relaxed);
    const uint64_t cache_hits  = m.cache_hits.load(std::memory_order_relaxed);
    const uint64_t cache_misses = m.cache_misses.load(std::memory_order_relaxed);

    const auto cache_stats = ConfigPathResolver::cacheStats();
    const uint64_t cache_total = cache_stats.hits + cache_stats.misses;
    const double cache_hit_ratio =
        cache_total > 0 ? static_cast<double>(cache_stats.hits) / static_cast<double>(cache_total) : 0.0;
    const auto cache_cfg = ConfigPathResolver::currentCacheConfig();

    const auto emitToOptionalSink = [&]() {
        GaugeSinkFn fn;
        {
            std::lock_guard<std::mutex> lk(ConfigMetricsExporter::gaugeSinkFnMutex());
            fn = ConfigMetricsExporter::gaugeSinkFnStorage();
        }
        if (!fn) {
            return;
        }

        const auto emit = [&](const std::string& name, double value) {
            try {
                fn(name, value);
            } catch (const std::exception &) {
                // Fail closed: metrics sinks are optional integration hooks and
                // must not affect production collector updates.
            } catch (const std::string &) {
                // Fail closed: metrics sinks are optional integration hooks and
                // must not affect production collector updates.
            } catch (const char *) {
                // Fail closed: metrics sinks are optional integration hooks and
                // must not affect production collector updates.
            }
        };

        emit("themis_config_resolution_hits_current", static_cast<double>(hits));
        emit("themis_config_resolution_misses_current", static_cast<double>(misses));
        emit("themis_config_legacy_fallbacks_current", static_cast<double>(fallbacks));
        emit("themis_config_new_path_hits_current", static_cast<double>(new_path_hits));
        emit("themis_config_unmapped_requests_current", static_cast<double>(unmapped));
        emit("themis_config_cache_hits_current", static_cast<double>(cache_hits));
        emit("themis_config_cache_misses_current", static_cast<double>(cache_misses));
        emit("themis_config_cache_hit_ratio", cache_hit_ratio);
        emit("themis_config_cache_size", static_cast<double>(cache_stats.size));
        emit("themis_config_cache_capacity", static_cast<double>(cache_stats.capacity));
        emit("themis_config_cache_ttl_seconds", static_cast<double>(cache_cfg.ttl_seconds));
    };

#ifdef THEMIS_TEST_BUILD
    // PERMANENT FALLBACK NOTE (ConfigMetricsExporter test build):
    // Purpose: Allow focused/unit test binaries to compile `ConfigMetricsExporter`
    //   without linking `observability::MetricsCollector` or the full Prometheus
    //   client library.  When `THEMIS_TEST_BUILD` is defined, this function
    //   returns immediately (early exit) without reading any counters or calling
    //   `MetricsCollector::setGauge()`.
    // Activation: `THEMIS_TEST_BUILD` defined at compile time (set by CMake for
    //   focused unit test targets to keep them lightweight).
    // Production Delta: `syncFromPathResolver()` does not update any gauge in the
    //   server-wide scrape endpoint.  The Prometheus `/metrics` page will not
    //   reflect config-path resolution counters (cache hits, legacy fallbacks,
    //   resolution misses) while this flag is active.
    // Note: Production CMake targets must NOT define `THEMIS_TEST_BUILD`.

    // When built with THEMIS_TEST_BUILD (used by focused/unit test binaries),
    // the MetricsCollector and its dependencies are typically not linked. This
    // stub keeps those test builds lightweight while production builds execute
    // the real synchronization. Callers in test builds should not expect this
    // function to mutate any global metrics state.
    emitToOptionalSink();
    return;
#else
    auto& collector = observability::MetricsCollector::getInstance();

    // Push current counter values as gauges in the MetricsCollector so they
    // appear in the server-wide scrape endpoint.  Gauge names use the
    // "themis_config_*_current" convention to distinguish them from the
    // proper Prometheus counter series emitted by collect().
    // Gauges use *_current naming; *_aggregate variants provide counter-like totals, while *_total aliases are preserved for backward compatibility (non-counter gauges; planned deprecation in v1.9.0).
    collector.setGauge("themis_config_resolution_hits_current",    static_cast<double>(hits));
    collector.setGauge("themis_config_resolution_misses_current",  static_cast<double>(misses));
    collector.setGauge("themis_config_legacy_fallbacks_current",        static_cast<double>(fallbacks));
    collector.setGauge("themis_config_legacy_fallbacks_all_current",    static_cast<double>(fallbacks));
    collector.setGauge("themis_config_legacy_fallbacks_all_aggregate",  static_cast<double>(fallbacks));
    collector.setGauge("themis_config_legacy_fallbacks_all_total",      static_cast<double>(fallbacks)); // compatibility gauge
    collector.setGauge("themis_config_new_path_hits_current",           static_cast<double>(new_path_hits));
    collector.setGauge("themis_config_new_path_hits_aggregate",         static_cast<double>(new_path_hits));
    collector.setGauge("themis_config_new_path_hits_total",             static_cast<double>(new_path_hits)); // compatibility gauge
    collector.setGauge("themis_config_unmapped_requests_current",       static_cast<double>(unmapped));
    collector.setGauge("themis_config_cache_hits_current",              static_cast<double>(cache_hits));
    collector.setGauge("themis_config_cache_hits_aggregate",            static_cast<double>(cache_hits));
    collector.setGauge("themis_config_cache_hits_total",                static_cast<double>(cache_hits)); // compatibility gauge
    collector.setGauge("themis_config_cache_misses_current",            static_cast<double>(cache_misses));
    collector.setGauge("themis_config_cache_misses_aggregate",          static_cast<double>(cache_misses));
    collector.setGauge("themis_config_cache_misses_total",              static_cast<double>(cache_misses)); // compatibility gauge
    collector.setGauge("themis_config_cache_hit_ratio",            cache_hit_ratio);
    collector.setGauge("themis_config_cache_size",                 static_cast<double>(cache_stats.size));
    collector.setGauge("themis_config_cache_capacity",             static_cast<double>(cache_stats.capacity));
    collector.setGauge("themis_config_cache_ttl_seconds",          static_cast<double>(cache_cfg.ttl_seconds));

    // Per-category legacy fallback gauges
    for (const auto& [cat, count] : ConfigPathResolver::legacyFallbacksByCategory()) {
        collector.setGauge("themis_config_legacy_fallbacks_by_category_current",
                           static_cast<double>(count), {{"category", cat}});
    }
    emitToOptionalSink();
#endif
}

void ConfigMetricsExporter::registerWithRegistry(const std::shared_ptr<prometheus::Registry>& registry) {
#ifdef THEMIS_HAS_PROMETHEUS
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
    g_metrics.legacy_family = &legacy_family;
    for (const auto& category : ConfigPathResolver::legacyFallbackCategories()) {
        g_metrics.legacy_by_category[category] = &legacy_family.Add({{"category", category}});
    }
    // Aggregate series exposed via dedicated metric; use this instead of summing the per-category series to avoid double-counting.
    auto& legacy_total_family = prometheus::BuildCounter()
        .Name("themis_config_legacy_fallbacks_all_total")
        .Help("Total number of legacy config path fallbacks (aggregate across all categories).")
        .Register(*g_registry);
    g_metrics.legacy_fallbacks = &legacy_total_family.Add({});

    auto& unmapped_family = prometheus::BuildCounter()
        .Name("themis_config_unmapped_requests_total")
        .Help("Total number of resolution requests for paths with no mapping entry.")
        .Register(*g_registry);
    g_metrics.unmapped_requests = &unmapped_family.Add({});

    auto& new_path_hits_family = prometheus::BuildCounter()
        .Name("themis_config_new_path_hits_total")
        .Help("Total number of times the new (canonical) config path was resolved.")
        .Register(*g_registry);
    g_metrics.new_path_hits = &new_path_hits_family.Add({});

    auto& cache_hits_family = prometheus::BuildCounter()
        .Name("themis_config_cache_hits_total")
        .Help("Total number of cache hits for config path resolution.")
        .Register(*g_registry);
    g_metrics.cache_hits = &cache_hits_family.Add({});

    auto& cache_misses_family = prometheus::BuildCounter()
        .Name("themis_config_cache_misses_total")
        .Help("Total number of cache misses for config path resolution.")
        .Register(*g_registry);
    g_metrics.cache_misses = &cache_misses_family.Add({});

    auto& cache_hit_ratio_family = prometheus::BuildGauge()
        .Name("themis_config_cache_hit_ratio")
        .Help("Ratio of cache hits to total cache lookups (0.0–1.0).")
        .Register(*g_registry);
    g_metrics.cache_hit_ratio = &cache_hit_ratio_family.Add({});

    auto& cache_size_family = prometheus::BuildGauge()
        .Name("themis_config_cache_size")
        .Help("Current size of the config path LRU cache.")
        .Register(*g_registry);
    g_metrics.cache_size = &cache_size_family.Add({});

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
#else
#endif
}

} // namespace config
} // namespace themis
