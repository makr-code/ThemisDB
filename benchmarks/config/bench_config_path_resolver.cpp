// Benchmark: ConfigPathResolver cache hit rate and resolution latency
// Measures the performance of path resolution with warm and cold caches,
// legacy-path mapping lookups, deprecation aggregator overhead, and the
// Prometheus metrics scrape path.

#include "config/config_path_resolver.h"
#include "config/config_metrics_exporter.h"
#include <benchmark/benchmark.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace themis {
namespace config {
namespace bench {

namespace fs = std::filesystem;

// ============================================================================
// Helpers
// ============================================================================

static void writeFile(const fs::path& p, const char* content = "key: value\n") {
    fs::create_directories(p.parent_path());
    std::ofstream f(p);
    f << content;
}

// ============================================================================
// Fixture
// ============================================================================

/**
 * Sets up a temporary directory tree that mirrors the config path layout used
 * by ConfigPathResolver.
 *
 * Paths created (relative to temp_root_):
 *   config/lora_training_config.yaml            <- legacy key (mapped)
 *   config/ai_ml/lora_training_config.yaml      <- new mapped location
 *   config/pii_patterns.yaml                    <- legacy fallback only (new not created)
 *   config/unmapped_bench.yaml                  <- no PATH_MAPPING entry
 *
 * The benchmark changes the process working directory to temp_root_ so that
 * ConfigPathResolver's relative-path filesystem::exists() calls resolve
 * against the synthetic tree.  The original CWD is restored in TearDown.
 */
class ConfigPathResolverBenchFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*state*/) override {
        original_cwd_ = fs::current_path();

        temp_root_ = fs::temp_directory_path() /
                     ("themisdb_bench_" + std::to_string(
                          std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(temp_root_);

        // New mapped path: both legacy key and new location exist
        writeFile(temp_root_ / "config" / "lora_training_config.yaml");
        writeFile(temp_root_ / "config" / "ai_ml" / "lora_training_config.yaml");

        // Legacy fallback: only the legacy path exists (no new path file)
        writeFile(temp_root_ / "config" / "pii_patterns.yaml");

        // Unmapped path: absolute path that has no PATH_MAPPING entry
        writeFile(temp_root_ / "config" / "unmapped_bench.yaml");

        fs::current_path(temp_root_);

        ConfigPathResolver::resetMetrics();
        ConfigPathResolver::clearCache();
        ConfigPathResolver::setCachingEnabled(true);

        // Warm the cache for the mapped-path cache-hit benchmarks.
        mapped_key_   = "config/lora_training_config.yaml";
        unmapped_abs_ = (temp_root_ / "config" / "unmapped_bench.yaml").string();

        ConfigPathResolver::tryResolve(mapped_key_);   // populates cache
        ConfigPathResolver::tryResolve(unmapped_abs_); // populates cache
    }

    void TearDown(const benchmark::State& /*state*/) override {
        fs::current_path(original_cwd_);
        fs::remove_all(temp_root_);
        ConfigPathResolver::clearCache();
        ConfigPathResolver::resetMetrics();
    }

protected:
    fs::path    original_cwd_;
    fs::path    temp_root_;
    std::string mapped_key_;     ///< key that is in PATH_MAPPING and has a file on disk
    std::string unmapped_abs_;   ///< absolute path to a file with no PATH_MAPPING entry
};

// ============================================================================
// 1. Cache Hit – mapped path  (target: < 1 µs)
// ============================================================================

BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, CacheHit_MappedPath)(
        benchmark::State& state) {
    // Cache already warm from SetUp.
    for (auto _ : state) {
        auto result = ConfigPathResolver::tryResolve(mapped_key_);
        benchmark::DoNotOptimize(result);
    }
    auto stats = ConfigPathResolver::cacheStats();
    state.counters["cache_hit_rate"] = benchmark::Counter(
        stats.hit_rate * 100.0, benchmark::Counter::kDefaults);
    state.SetLabel("target < 1 us");
}

BENCHMARK_REGISTER_F(ConfigPathResolverBenchFixture, CacheHit_MappedPath)
    ->Unit(benchmark::kNanosecond)
    ->MinTime(0.5);

// ============================================================================
// 2. Cache Hit – unmapped absolute path  (target: < 1 µs)
// ============================================================================

BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, CacheHit_UnmappedPath)(
        benchmark::State& state) {
    for (auto _ : state) {
        auto result = ConfigPathResolver::tryResolve(unmapped_abs_);
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("target < 1 us");
}

BENCHMARK_REGISTER_F(ConfigPathResolverBenchFixture, CacheHit_UnmappedPath)
    ->Unit(benchmark::kNanosecond)
    ->MinTime(0.5);

// ============================================================================
// 3. Cache Miss – mapped path with new file on disk  (target: < 200 µs)
// ============================================================================

BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, CacheMiss_MappedPath)(
        benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        ConfigPathResolver::clearCache();
        state.ResumeTiming();

        auto result = ConfigPathResolver::tryResolve(mapped_key_);
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("target < 200 us");
}

BENCHMARK_REGISTER_F(ConfigPathResolverBenchFixture, CacheMiss_MappedPath)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.5);

// ============================================================================
// 4. Cache Miss – unmapped absolute path  (baseline; no PATH_MAPPING lookup)
// ============================================================================

BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, CacheMiss_UnmappedPath)(
        benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        ConfigPathResolver::clearCache();
        state.ResumeTiming();

        auto result = ConfigPathResolver::tryResolve(unmapped_abs_);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_REGISTER_F(ConfigPathResolverBenchFixture, CacheMiss_UnmappedPath)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.5);

// ============================================================================
// 5. Legacy Fallback path  (only legacy file exists)
// ============================================================================

BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, LegacyFallback)(
        benchmark::State& state) {
    const std::string legacy_key = "config/pii_patterns.yaml";

    for (auto _ : state) {
        state.PauseTiming();
        ConfigPathResolver::clearCache();
        state.ResumeTiming();

        auto result = ConfigPathResolver::tryResolve(legacy_key);
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("legacy path fallback");
}

BENCHMARK_REGISTER_F(ConfigPathResolverBenchFixture, LegacyFallback)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.5);

// ============================================================================
// 6. mapLegacyToNew() – pure in-memory hash-map lookup  (target: < 1 µs)
// ============================================================================

static void BM_MapLegacyToNew(benchmark::State& state) {
    // Use a path that IS in the mapping table.
    const std::string key = "config/lora_training_config.yaml";
    for (auto _ : state) {
        auto result = ConfigPathResolver::mapLegacyToNew(key);
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("target < 1 us");
}
BENCHMARK(BM_MapLegacyToNew)->Unit(benchmark::kNanosecond)->MinTime(0.5);

// ============================================================================
// 7. mapLegacyToNew() – path NOT in mapping table (miss case)
// ============================================================================

static void BM_MapLegacyToNew_Miss(benchmark::State& state) {
    const std::string key = "config/does_not_exist_bench.yaml";
    for (auto _ : state) {
        auto result = ConfigPathResolver::mapLegacyToNew(key);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_MapLegacyToNew_Miss)->Unit(benchmark::kNanosecond)->MinTime(0.5);

// ============================================================================
// 8. Deprecation Aggregator hot-path overhead  (target: < 50 ns)
//
//    Measured as the incremental cost of a legacy-fallback resolution over
//    a cache-miss resolution of a mapped path where the new file exists.
//    The deprecation aggregator's incrementUsage() fires only on the legacy
//    branch, so the difference of BM_LegacyFallback_Incremental vs
//    BM_MappedPath_Incremental isolates that overhead.
//
//    We also expose a direct throughput benchmark of resolve() on the
//    legacy-only path with aggregation enabled.
// ============================================================================

BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, DeprecationAggregator_Overhead)(
        benchmark::State& state) {
    // Enable aggregation so per-call log spam is suppressed.
    ConfigPathResolver::setAggregationEnabled(true, 3600);
    const std::string legacy_key = "config/pii_patterns.yaml";

    for (auto _ : state) {
        state.PauseTiming();
        ConfigPathResolver::clearCache();
        state.ResumeTiming();

        // This call goes through the legacy fallback branch →
        // increments the DeprecationAggregator usage counter.
        auto result = ConfigPathResolver::tryResolve(legacy_key);
        benchmark::DoNotOptimize(result);
    }

    state.PauseTiming();
    ConfigPathResolver::setAggregationEnabled(false);
    state.ResumeTiming();

    state.SetLabel("aggregator overhead included; target incremental < 50 ns");
}

BENCHMARK_REGISTER_F(ConfigPathResolverBenchFixture, DeprecationAggregator_Overhead)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.5);

// ============================================================================
// 8b. deprecationReport() generation with 60 pre-populated legacy paths
//     (target: < 1 ms)
//
//     AC-7: "Report generation for 60 legacy paths completes in < 1 ms
//     (in-memory map iteration)."
//
//     Seeds the aggregator with up to 60 distinct legacy paths from
//     PATH_MAPPING, then measures pure report generation time
//     (map snapshot + sort in getReport()).
// ============================================================================

BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, DeprecationReport_60Paths)(
        benchmark::State& state) {
    // CWD is already temp_root_ (set by fixture SetUp).
    ConfigPathResolver::resetMetrics();
    ConfigPathResolver::clearCache();

    // Pre-seed the aggregator: create the legacy file for each PATH_MAPPING
    // entry (without its new counterpart) so tryResolve() takes the fallback
    // branch and calls aggregator_.incrementUsage().
    const auto& mappings = ConfigPathResolver::legacyPathMappings();
    int seeded = 0;
    for (const auto& [legacy, new_path] : mappings) {
        (void)new_path;  // new_path not needed; we intentionally omit it on disk
        if (seeded >= 60) {
          break;
        }
        fs::path legacy_file(legacy);
        if (!fs::exists(legacy_file)) {
            writeFile(legacy_file);
        }
        ConfigPathResolver::clearCache();
        ConfigPathResolver::tryResolve(legacy);
        ++seeded;
    }

    // Measure pure deprecationReport() generation (map snapshot + sort).
    std::vector<ConfigPathResolver::DeprecationEntry> last_report = {};

    for (auto _ : state) {
        last_report = ConfigPathResolver::deprecationReport();
        benchmark::DoNotOptimize(last_report);
    }
    state.counters["paths_in_report"] = benchmark::Counter(
        static_cast<double>(last_report.size()));
    state.SetLabel("report generation; target < 1 ms for 60 paths");
}

BENCHMARK_REGISTER_F(ConfigPathResolverBenchFixture, DeprecationReport_60Paths)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.5);

// ============================================================================
// 9. Metrics read (Prometheus text scrape)  (target: < 1 ms)
// ============================================================================

BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, MetricsScrape)(
        benchmark::State& state) {
    // Pre-populate metrics so the exporter has real non-zero values.
    for (int i = 0; i < 200; ++i) {
        ConfigPathResolver::tryResolve(mapped_key_);   // cache hits after first
    }
    ConfigPathResolver::clearCache();
    ConfigPathResolver::tryResolve(mapped_key_);  // one more cache miss

    for (auto _ : state) {
        auto text = ConfigMetricsExporter::collect();
        benchmark::DoNotOptimize(text);
    }
    state.SetLabel("Prometheus text scrape; target < 1 ms");
}

BENCHMARK_REGISTER_F(ConfigPathResolverBenchFixture, MetricsScrape)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.5);

// ============================================================================
// 10. Cache hit rate – bulk warmup then measure
//     Report the observed hit rate as a counter.
// ============================================================================

BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, CacheHitRate_BulkAccess)(
        benchmark::State& state) {
    // Access a set of paths repeatedly to drive up the hit rate.
    const std::vector<std::string> paths = {
        mapped_key_,
        unmapped_abs_,
    };

    ConfigPathResolver::clearCache();
    ConfigPathResolver::resetMetrics();

    // Warm the cache with one pass.
    for (const auto& p : paths) {
        ConfigPathResolver::tryResolve(p);
    }

    for (auto _ : state) {
        // All subsequent accesses are cache hits.
        for (const auto& p : paths) {
            auto result = ConfigPathResolver::tryResolve(p);
            benchmark::DoNotOptimize(result);
        }
    }

    // Report the observed cache hit rate.
    const auto& m = ConfigPathResolver::metrics();
    uint64_t hits   = m.cache_hits.load();
    uint64_t misses = m.cache_misses.load();
    uint64_t total  = hits + misses;
    double   rate   = total > 0 ? static_cast<double>(hits) / static_cast<double>(total) : 0.0;

    state.counters["cache_hits"]   = benchmark::Counter(static_cast<double>(hits));
    state.counters["cache_misses"] = benchmark::Counter(static_cast<double>(misses));
    state.counters["hit_rate_pct"] = benchmark::Counter(rate * 100.0);
}

BENCHMARK_REGISTER_F(ConfigPathResolverBenchFixture, CacheHitRate_BulkAccess)
    ->Unit(benchmark::kNanosecond)
    ->MinTime(1.0);

// ============================================================================
// 11. Concurrent cache hit throughput (multi-threaded)
// ============================================================================

BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, ConcurrentCacheHit)(
        benchmark::State& state) {
    // Cache is pre-warmed in SetUp().
    for (auto _ : state) {
        auto result = ConfigPathResolver::tryResolve(mapped_key_);
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("multi-thread cache hit throughput");
}

BENCHMARK_REGISTER_F(ConfigPathResolverBenchFixture, ConcurrentCacheHit)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Unit(benchmark::kNanosecond)
    ->MinTime(0.5);

// ============================================================================
// 12. resolve() – same as tryResolve() cache hit via the throwing overload
// ============================================================================

BENCHMARK_DEFINE_F(ConfigPathResolverBenchFixture, Resolve_CacheHit)(
        benchmark::State& state) {
    for (auto _ : state) {
        auto result = ConfigPathResolver::resolve(mapped_key_);
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("target < 1 us");
}

BENCHMARK_REGISTER_F(ConfigPathResolverBenchFixture, Resolve_CacheHit)
    ->Unit(benchmark::kNanosecond)
    ->MinTime(0.5);

} // namespace bench
} // namespace config
} // namespace themis

BENCHMARK_MAIN();
