/**
 * @file bench_adaptive_query_cache.cpp
 * @brief Google Benchmark performance tests for AdaptiveQueryCache
 *
 * Covers the Production Readiness Checklist item "Performance benchmarks"
 * for the multi-level L1/L2/L3 adaptive query cache.
 *
 * Benchmarked scenarios:
 * - L1 put / get throughput (hot tier, in-memory)
 * - L2 put / get throughput (warm tier, compressed in-memory)
 * - Cache hit vs miss latency across all tiers
 * - Multi-level pipeline: L1 miss → L2 miss → L3 write-through read
 * - Tenant isolation overhead (namespace key decoration)
 * - Circuit breaker state overhead (CLOSED / OPEN)
 * - Concurrent read/write throughput (multi-threaded)
 * - Pattern-based invalidation performance
 * - generateFingerprint throughput
 *
 * Output: JSON format for CI regression tracking
 *
 * Performance targets (Release build on commodity hardware):
 * - L1 put:    >100K ops/sec
 * - L1 get:    >200K ops/sec
 * - L2 put:    >50K  ops/sec
 * - L2 get:    >100K ops/sec
 * - Fingerprint generation: >500K ops/sec
 */

#include <benchmark/benchmark.h>
#include "cache/adaptive_query_cache.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <random>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <filesystem>

using namespace themis;
using json = nlohmann::json;

namespace {

// ═══════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════

/**
 * @brief Build a minimal cache config that avoids touching RocksDB (no L3)
 *        unless explicitly required, so benchmarks run fast and in-process.
 */
AdaptiveQueryCache::Config makeL1L2Config(size_t l1_entries = 100000,
                                          size_t l2_entries = 200000) {
    AdaptiveQueryCache::Config cfg;
    cfg.l1_max_entries    = l1_entries;
    cfg.l2_max_entries    = l2_entries;
    cfg.l1_ttl_seconds    = 300;
    cfg.l2_ttl_seconds    = 1800;
    cfg.l3_db_path        = "";       // empty → no RocksDB initialised
    cfg.enable_circuit_breaker = false;
    cfg.enable_rate_limiting   = false;
    cfg.enable_tenant_isolation = false;
    return cfg;
}

/**
 * @brief Build a config with tenant isolation enabled.
 */
AdaptiveQueryCache::Config makeTenantConfig(size_t l1_entries = 100000) {
    auto cfg = makeL1L2Config(l1_entries);
    cfg.enable_tenant_isolation = true;
    cfg.per_tenant_max_bytes    = 512ULL * 1024 * 1024; // 512 MB
    return cfg;
}

/**
 * @brief Build a cache with circuit breaker enabled.
 */
AdaptiveQueryCache::Config makeCBConfig() {
    auto cfg = makeL1L2Config();
    cfg.enable_circuit_breaker   = true;
    cfg.cb_failure_threshold     = 5;
    cfg.cb_timeout_ms            = 60000;
    return cfg;
}

/** Pre-warm a cache with `count` entries so benchmarks measure steady state. */
void warmCache(AdaptiveQueryCache& cache, size_t count,
               const std::string& tenant_id = "") {
    json params;
    for (size_t i = 0; i < count; ++i) {
        std::string q  = "SELECT * FROM t WHERE id=" + std::to_string(i);
        auto fp        = cache.generateFingerprint(q, params, tenant_id);
        json result    = {{"rows", i}, {"data", "value_" + std::to_string(i)}};
        cache.put(fp, params, result, tenant_id);
    }
}

// ═══════════════════════════════════════════════════════════
// Fingerprint generation
// ═══════════════════════════════════════════════════════════

/**
 * Target: >500K fingerprints/sec
 * Measures SHA-256 + JSON-serialisation cost.
 */
static void BM_Cache_GenerateFingerprint(benchmark::State& state) {
    auto cfg = makeL1L2Config();
    AdaptiveQueryCache cache(cfg);

    json params = {{"id", 42}, {"limit", 100}};
    int counter = 0;

    for (auto _ : state) {
        std::string q = "SELECT * FROM users WHERE id=?_" + std::to_string(counter++);
        auto fp = cache.generateFingerprint(q, params);
        benchmark::DoNotOptimize(fp.data());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("fingerprint_generation");
}
BENCHMARK(BM_Cache_GenerateFingerprint);

// ═══════════════════════════════════════════════════════════
// L1 (HOT) tier put/get
// ═══════════════════════════════════════════════════════════

/**
 * L1 put throughput with small (<1 KB) payloads.
 * Target: >100K ops/sec
 */
static void BM_Cache_L1_Put(benchmark::State& state) {
    auto cfg = makeL1L2Config();
    AdaptiveQueryCache cache(cfg);

    json params;
    int counter = 0;

    for (auto _ : state) {
        std::string q   = "SELECT id FROM t WHERE k=" + std::to_string(counter++);
        auto fp         = cache.generateFingerprint(q, params);
        json result     = {{"rows", counter}};
        bool ok         = cache.put(fp, params, result);
        benchmark::DoNotOptimize(ok);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("l1_put");
}
BENCHMARK(BM_Cache_L1_Put);

/**
 * L1 get throughput (100% hit rate, pre-warmed cache).
 * Target: >200K ops/sec
 */
static void BM_Cache_L1_Get_Hit(benchmark::State& state) {
    size_t cache_size = static_cast<size_t>(state.range(0));
    auto cfg = makeL1L2Config(cache_size * 2);
    AdaptiveQueryCache cache(cfg);
    warmCache(cache, cache_size);

    // Pre-compute fingerprints to avoid measuring hashing in the hot loop
    std::vector<std::string> fingerprints;
    fingerprints.reserve(cache_size);
    json params;
    for (size_t i = 0; i < cache_size; ++i) {
        std::string q = "SELECT * FROM t WHERE id=" + std::to_string(i);
        fingerprints.push_back(cache.generateFingerprint(q, params));
    }

    size_t idx = 0;
    for (auto _ : state) {
        auto result = cache.get(fingerprints[idx % cache_size]);
        benchmark::DoNotOptimize(result.has_value());
        benchmark::ClobberMemory();
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("l1_get_hit");
}
BENCHMARK(BM_Cache_L1_Get_Hit)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(50000);

/**
 * Cache miss throughput (cold cache, no entries present).
 * Measures overhead of a complete miss path (L1→L2→L3 search).
 */
static void BM_Cache_Get_Miss(benchmark::State& state) {
    auto cfg = makeL1L2Config();
    AdaptiveQueryCache cache(cfg);

    json params;
    int counter = 0;

    for (auto _ : state) {
        // Generate unique queries that were never put → always miss
        std::string q = "MISS_" + std::to_string(counter++);
        auto fp       = cache.generateFingerprint(q, params);
        auto result   = cache.get(fp);
        benchmark::DoNotOptimize(result.has_value());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("cache_miss");
}
BENCHMARK(BM_Cache_Get_Miss);

// ═══════════════════════════════════════════════════════════
// L2 (WARM) tier put/get
// ═══════════════════════════════════════════════════════════

/**
 * L2 put throughput: payloads between 1 KB and 10 KB trigger L2 storage.
 * Target: >50K ops/sec
 */
static void BM_Cache_L2_Put(benchmark::State& state) {
    size_t payload_bytes = static_cast<size_t>(state.range(0));
    auto cfg = makeL1L2Config();
    AdaptiveQueryCache cache(cfg);

    // Build a payload that exceeds L1 size limit (1 KB) to land in L2
    std::string value(payload_bytes, 'x');
    json params;
    int counter = 0;

    for (auto _ : state) {
        std::string q = "SELECT data FROM blobs WHERE id=" + std::to_string(counter++);
        auto fp       = cache.generateFingerprint(q, params);
        json result   = {{"data", value}, {"row", counter}};
        bool ok       = cache.put(fp, params, result);
        benchmark::DoNotOptimize(ok);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("l2_put_" + std::to_string(payload_bytes) + "B");
}
BENCHMARK(BM_Cache_L2_Put)
    ->Arg(1200)   // just above L1 limit
    ->Arg(5120)   // 5 KB
    ->Arg(9000);  // near L2 limit

// ═══════════════════════════════════════════════════════════
// Mixed read/write (realistic workload)
// ═══════════════════════════════════════════════════════════

/**
 * Mixed 80% reads / 20% writes on a pre-warmed cache.
 * Simulates a typical OLAP read-heavy workload.
 */
static void BM_Cache_Mixed_ReadWrite(benchmark::State& state) {
    size_t cache_size = static_cast<size_t>(state.range(0));
    auto cfg = makeL1L2Config(cache_size * 2);
    AdaptiveQueryCache cache(cfg);
    warmCache(cache, cache_size);

    std::vector<std::string> fingerprints;
    fingerprints.reserve(cache_size);
    json params;
    for (size_t i = 0; i < cache_size; ++i) {
        std::string q = "SELECT * FROM t WHERE id=" + std::to_string(i);
        fingerprints.push_back(cache.generateFingerprint(q, params));
    }

    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> idx_dist(0, cache_size - 1);
    std::uniform_int_distribution<int>    op_dist(0, 9);  // 0-7 = read, 8-9 = write
    int write_counter = static_cast<int>(cache_size);

    for (auto _ : state) {
        if (op_dist(rng) < 8) {
            // Read (80%)
            auto result = cache.get(fingerprints[idx_dist(rng)]);
            benchmark::DoNotOptimize(result.has_value());
        } else {
            // Write (20%)
            std::string q = "INSERT_" + std::to_string(write_counter++);
            auto fp       = cache.generateFingerprint(q, params);
            json result   = {{"rows", write_counter}};
            bool ok       = cache.put(fp, params, result);
            benchmark::DoNotOptimize(ok);
        }
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("mixed_80r_20w");
}
BENCHMARK(BM_Cache_Mixed_ReadWrite)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(50000);

// ═══════════════════════════════════════════════════════════
// Tenant isolation overhead
// ═══════════════════════════════════════════════════════════

/**
 * put() with tenant isolation enabled vs disabled.
 * Measures the overhead of per-tenant namespace key decoration and quota check.
 */
static void BM_Cache_TenantIsolation_Put(benchmark::State& state) {
    bool with_tenant = state.range(0) != 0;
    AdaptiveQueryCache::Config cfg = with_tenant ? makeTenantConfig() : makeL1L2Config();
    AdaptiveQueryCache cache(cfg);

    std::string tenant_id = with_tenant ? "tenant_acme" : "";
    json params;
    int counter = 0;

    for (auto _ : state) {
        std::string q = "SELECT id FROM t WHERE k=" + std::to_string(counter++);
        auto fp       = cache.generateFingerprint(q, params, tenant_id);
        json result   = {{"rows", counter}};
        bool ok       = cache.put(fp, params, result, tenant_id);
        benchmark::DoNotOptimize(ok);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel(with_tenant ? "tenant_isolation_on" : "tenant_isolation_off");
}
BENCHMARK(BM_Cache_TenantIsolation_Put)
    ->Arg(0)   // isolation off
    ->Arg(1);  // isolation on

/**
 * get() with tenant isolation enabled.
 * Measures key-decoration + namespace-check overhead on hit path.
 */
static void BM_Cache_TenantIsolation_Get_Hit(benchmark::State& state) {
    size_t cache_size = 10000;
    auto cfg = makeTenantConfig(cache_size * 2);
    AdaptiveQueryCache cache(cfg);
    std::string tenant_id = "tenant_acme";
    warmCache(cache, cache_size, tenant_id);

    std::vector<std::string> fingerprints;
    fingerprints.reserve(cache_size);
    json params;
    for (size_t i = 0; i < cache_size; ++i) {
        std::string q = "SELECT * FROM t WHERE id=" + std::to_string(i);
        fingerprints.push_back(cache.generateFingerprint(q, params, tenant_id));
    }

    size_t idx = 0;
    for (auto _ : state) {
        auto result = cache.get(fingerprints[idx % cache_size], tenant_id);
        benchmark::DoNotOptimize(result.has_value());
        benchmark::ClobberMemory();
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("tenant_get_hit");
}
BENCHMARK(BM_Cache_TenantIsolation_Get_Hit);

// ═══════════════════════════════════════════════════════════
// Circuit breaker overhead (L3 disabled; CB path tested)
// ═══════════════════════════════════════════════════════════

/**
 * put() with circuit breaker in CLOSED state (normal operation).
 * Should be close to put() without circuit breaker.
 */
static void BM_Cache_CircuitBreaker_Closed(benchmark::State& state) {
    auto cfg = makeCBConfig();
    AdaptiveQueryCache cache(cfg);

    json params;
    int counter = 0;

    for (auto _ : state) {
        std::string q = "SELECT id FROM t WHERE k=" + std::to_string(counter++);
        auto fp       = cache.generateFingerprint(q, params);
        json result   = {{"rows", counter}};
        bool ok       = cache.put(fp, params, result);
        benchmark::DoNotOptimize(ok);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("cb_closed");
}
BENCHMARK(BM_Cache_CircuitBreaker_Closed);

// ═══════════════════════════════════════════════════════════
// Invalidation performance
// ═══════════════════════════════════════════════════════════

/**
 * Pattern-based invalidation on various cache sizes.
 * Measures the cost of scanning L1/L2 for a regex pattern.
 * Target: invalidate() completes in <10ms for 50K entries.
 */
static void BM_Cache_Invalidate_Pattern(benchmark::State& state) {
    size_t cache_size = static_cast<size_t>(state.range(0));
    auto cfg = makeL1L2Config(cache_size * 2);

    for (auto _ : state) {
        state.PauseTiming();
        AdaptiveQueryCache cache(cfg);
        warmCache(cache, cache_size);
        state.ResumeTiming();

        // Invalidate half the entries via a pattern match
        size_t removed = cache.invalidate("SELECT.*FROM t WHERE id=[0-9]+");
        benchmark::DoNotOptimize(removed);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("invalidate_pattern_" + std::to_string(cache_size));
}
BENCHMARK(BM_Cache_Invalidate_Pattern)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(50000);

/**
 * Tenant-scoped invalidation performance.
 * Invalidates all entries for one tenant; other tenants are unaffected.
 */
static void BM_Cache_InvalidateTenant(benchmark::State& state) {
    size_t cache_size = static_cast<size_t>(state.range(0));
    auto cfg = makeTenantConfig(cache_size * 3);

    for (auto _ : state) {
        state.PauseTiming();
        AdaptiveQueryCache cache(cfg);
        // Populate two tenants
        warmCache(cache, cache_size, "tenant_a");
        warmCache(cache, cache_size, "tenant_b");
        state.ResumeTiming();

        size_t removed = cache.invalidateTenant("tenant_a");
        benchmark::DoNotOptimize(removed);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("invalidate_tenant_" + std::to_string(cache_size));
}
BENCHMARK(BM_Cache_InvalidateTenant)
    ->Arg(500)
    ->Arg(5000)
    ->Arg(20000);

// ═══════════════════════════════════════════════════════════
// Concurrent access throughput
// ═══════════════════════════════════════════════════════════

/**
 * Concurrent read throughput using Google Benchmark threads.
 * Pre-warms the cache, then hammers get() from N threads.
 * Target: near-linear scaling up to 4 threads on L1 hot path.
 */
static void BM_Cache_Concurrent_Read(benchmark::State& state) {
    // Shared state across threads – use static so it's initialised once.
    static AdaptiveQueryCache* shared_cache = nullptr;
    static std::vector<std::string>* shared_fps = nullptr;
    static constexpr size_t kCacheSize = 50000;

    if (state.thread_index() == 0) {
        static AdaptiveQueryCache s_cache(makeL1L2Config(kCacheSize * 2));
        static std::vector<std::string> s_fps;
        if (s_fps.empty()) {
            warmCache(s_cache, kCacheSize);
            json params;
            s_fps.reserve(kCacheSize);
            for (size_t i = 0; i < kCacheSize; ++i) {
                std::string q = "SELECT * FROM t WHERE id=" + std::to_string(i);
                s_fps.push_back(s_cache.generateFingerprint(q, params));
            }
        }
        shared_cache = &s_cache;
        shared_fps   = &s_fps;
    }

    size_t idx = static_cast<size_t>(state.thread_index());
    for (auto _ : state) {
        auto result = shared_cache->get((*shared_fps)[idx % kCacheSize]);
        benchmark::DoNotOptimize(result.has_value());
        benchmark::ClobberMemory();
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
    if (state.thread_index() == 0) {
        state.SetLabel("concurrent_read");
    }
}
BENCHMARK(BM_Cache_Concurrent_Read)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4);

/**
 * Concurrent mixed read/write throughput.
 * 80% reads / 20% writes from N threads concurrently.
 */
static void BM_Cache_Concurrent_Mixed(benchmark::State& state) {
    static AdaptiveQueryCache* shared_cache2 = nullptr;
    static std::vector<std::string>* shared_fps2 = nullptr;
    static constexpr size_t kCacheSize2 = 20000;

    if (state.thread_index() == 0) {
        static AdaptiveQueryCache s_cache2(makeL1L2Config(kCacheSize2 * 2));
        static std::vector<std::string> s_fps2;
        if (s_fps2.empty()) {
            warmCache(s_cache2, kCacheSize2);
            json params;
            s_fps2.reserve(kCacheSize2);
            for (size_t i = 0; i < kCacheSize2; ++i) {
                std::string q = "SELECT * FROM t WHERE id=" + std::to_string(i);
                s_fps2.push_back(s_cache2.generateFingerprint(q, params));
            }
        }
        shared_cache2 = &s_cache2;
        shared_fps2   = &s_fps2;
    }

    std::mt19937 rng(static_cast<uint32_t>(state.thread_index() + 1));
    std::uniform_int_distribution<int> op_dist(0, 9);
    json params;
    int write_ctr = static_cast<int>(kCacheSize2)
                  + state.thread_index() * 1000000;
    size_t read_idx = static_cast<size_t>(state.thread_index());

    for (auto _ : state) {
        if (op_dist(rng) < 8) {
            auto result = shared_cache2->get((*shared_fps2)[read_idx % kCacheSize2]);
            benchmark::DoNotOptimize(result.has_value());
            ++read_idx;
        } else {
            std::string q = "WRITE_" + std::to_string(write_ctr++);
            auto fp       = shared_cache2->generateFingerprint(q, params);
            json result   = {{"rows", write_ctr}};
            bool ok       = shared_cache2->put(fp, params, result);
            benchmark::DoNotOptimize(ok);
        }
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
    if (state.thread_index() == 0) {
        state.SetLabel("concurrent_mixed_80r_20w");
    }
}
BENCHMARK(BM_Cache_Concurrent_Mixed)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4);

// ═══════════════════════════════════════════════════════════
// getStats / getDetailedInfo overhead
// ═══════════════════════════════════════════════════════════

/**
 * Measures the cost of collecting per-tier statistics.
 * Should be < 1 ms even with large caches.
 */
static void BM_Cache_GetStats(benchmark::State& state) {
    size_t cache_size = static_cast<size_t>(state.range(0));
    auto cfg = makeL1L2Config(cache_size * 2);
    AdaptiveQueryCache cache(cfg);
    warmCache(cache, cache_size);

    for (auto _ : state) {
        auto stats = cache.getStats();
        benchmark::DoNotOptimize(stats.l1_hits);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("get_stats");
}
BENCHMARK(BM_Cache_GetStats)
    ->Arg(1000)
    ->Arg(50000);

// ═══════════════════════════════════════════════════════════
// warmupFromLog – parallel bulk load throughput
// ═══════════════════════════════════════════════════════════

namespace {

/// Base64 alphabet (RFC 4648) – mirrors warmup.cpp helper.
static const std::string kB64BenchChars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string b64EncodeBench(const std::string& data) {
    std::string out = {};
    out.reserve(((data.size() + 2) / 3) * 4);
    uint32_t buf = 0;
    int bits = 0;
    for (unsigned char c : data) {
        buf = (buf << 8) | c;
        bits += 8;
        while (bits >= 6) {
            bits -= 6;
            out.push_back(kB64BenchChars[(buf >> bits) & 0x3F]);
        }
    }
    if (bits > 0) {
        buf <<= (6 - bits);
        out.push_back(kB64BenchChars[buf & 0x3F]);
    }
    while (out.size() % 4 != 0) {
      out.push_back('=');
    }
    return out;
}

/// Build a 64-char lowercase hex key from an integer seed.
static std::string makeHexKey64(int n) {
    std::ostringstream ss = {};
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < 8; ++i) {
      ss << std::setw(8) << n;
    }
    return ss.str();
}

/**
 * Write `entry_count` NDJSON warmup lines to `path`.
 * Each line: {"key":"<sha256_hex>","value_b64":"<b64>","ttl_remaining_s":300}
 */
static void writeWarmupLog(const std::string& path, size_t entry_count) {
    std::ofstream f(path, std::ios::trunc);
    for (size_t i = 0; i < entry_count; ++i) {
        json val = {{"idx", static_cast<int>(i)},
                    {"data", std::string(32, static_cast<char>('a' + (i % 26)))}};
        json rec;
        rec["key"]            = makeHexKey64(static_cast<int>(i));
        rec["value_b64"]      = b64EncodeBench(val.dump());
        rec["ttl_remaining_s"] = 300;
        f << rec.dump() << '\n';
    }
}

} // namespace (warmup bench helpers)

/**
 * @brief Benchmark warmupFromLog() throughput with varying worker counts.
 *
 * Performance target (Issue #244): ≥ 500 K entries/s on a 4-core machine
 * with a 5 M entry log.
 *
 * State args:
 *  range(0) – number of warmup log entries
 *  range(1) – number of parallel workers (max_parallel_workers)
 */
static void BM_WarmupFromLog(benchmark::State& state) {
    const size_t   entry_count = static_cast<size_t>(state.range(0));
    const uint32_t num_workers = static_cast<uint32_t>(state.range(1));

    // Write the log once; reuse across iterations.
    const std::string log_path =
        "/tmp/bench_warmup_" + std::to_string(entry_count) + "_w" +
        std::to_string(num_workers) + ".ndjson";

    if (!std::filesystem::exists(log_path)) {
        writeWarmupLog(log_path, entry_count);
    }

    AdaptiveQueryCache::Config cfg = makeL1L2Config(entry_count * 2, entry_count * 4);
    cfg.max_parallel_workers = num_workers;

    for (auto _ : state) {
        AdaptiveQueryCache cache(cfg);
        auto result = cache.warmupFromLog(log_path);
        benchmark::DoNotOptimize(result.entries_loaded);
        benchmark::ClobberMemory();
    }
    // Report total entries processed across all iterations.
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(entry_count));

    std::error_code ec = {};
    std::filesystem::remove(log_path, ec);

    state.SetLabel("workers=" + std::to_string(num_workers) +
                   " entries=" + std::to_string(entry_count));
}

// Baseline: 10 K entries, 1 worker (single-threaded reference)
BENCHMARK(BM_WarmupFromLog)
    ->Args({10000, 1})
    ->Args({10000, 2})
    ->Args({10000, 4})
    ->Unit(benchmark::kMillisecond);

// Scale: 100 K entries with various worker counts
BENCHMARK(BM_WarmupFromLog)
    ->Args({100000, 1})
    ->Args({100000, 2})
    ->Args({100000, 4})
    ->Unit(benchmark::kMillisecond);

} // namespace

// ═══════════════════════════════════════════════════════════
// Main – JSON output for CI regression tracking
// ═══════════════════════════════════════════════════════════

BENCHMARK_MAIN();
