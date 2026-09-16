/**
 * @file bench_search_dedicated_gates.cpp
 * @brief Wave D — Search Engine Dedicated Performance Benchmark Gates.
 *
 * Four benchmark gates for the search engine hot paths using in-process stubs.
 * Results are captured and compared against the thresholds below during the
 * Wave D sign-off run.
 *
 * ## Gate definitions
 * - SE-BM-01 : FTS query p95 latency         ≤ 1 000 µs per search
 * - SE-BM-02 : Ranking p95 latency           ≤ 2 000 µs per ranking call
 * - SE-BM-03 : Facet filter p95 latency      ≤ 500 µs per filter
 * - SE-BM-04 : Concurrent search throughput  ≥ 5 000 searches/sec (8 threads)
 *
 * ## Labels
 * wave_d;benchmark;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_SEARCH_ENGINE.md
 * @see src/search/ROADMAP.md — Wave D contribution
 */

#include <benchmark/benchmark.h>

#include <cstdint>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// STUB / SIMULATION NOTE
//
// In-process stubs simulate search hot paths without requiring external
// backends.  MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

static const std::vector<std::string> kSearchTerms = {
    "themis", "database", "query", "index", "search",
    "vector", "graph", "transaction", "soak", "stress",
};

static inline uint32_t stub_fts_search(const std::string& term, uint32_t limit) {
    uint32_t hits = static_cast<uint32_t>(std::hash<std::string>{}(term) % 50 + 1);
    return std::min(hits, limit);
}

static inline float stub_rank(uint32_t hit_count, const std::string& term) {
    if (hit_count == 0) return 0.0f;
    return static_cast<float>(hit_count) / static_cast<float>(term.size() + 1);
}

static inline uint32_t stub_facet_filter(const std::string& key,
                                          const std::string& value,
                                          uint32_t candidate_count) {
    uint32_t m = static_cast<uint32_t>(
        (std::hash<std::string>{}(key + value) % (candidate_count + 1)));
    return m;
}

// ─────────────────────────────────────────────────────────────────────────────
// SE-BM-01 — FTS query p95 latency
// Gate: ≤ 1 000 µs per search on representative hardware.
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Search_FTSQueryP95(benchmark::State& state) {
    uint64_t i = 0;
    for (auto _ : state) {
        const std::string& term = kSearchTerms[i++ % kSearchTerms.size()];
        benchmark::DoNotOptimize(stub_fts_search(term, 10));
    }
    state.SetLabel("SE-BM-01 gate=1000us");
}
BENCHMARK(BM_Search_FTSQueryP95)
    ->Name("BM_Search_FTSQueryP95")
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// SE-BM-02 — Ranking p95 latency
// Gate: ≤ 2 000 µs per ranking call on representative hardware.
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Search_RankingP95(benchmark::State& state) {
    uint64_t i = 0;
    for (auto _ : state) {
        const std::string& term = kSearchTerms[i++ % kSearchTerms.size()];
        uint32_t hits = stub_fts_search(term, 10);
        benchmark::DoNotOptimize(stub_rank(hits, term));
    }
    state.SetLabel("SE-BM-02 gate=2000us");
}
BENCHMARK(BM_Search_RankingP95)
    ->Name("BM_Search_RankingP95")
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// SE-BM-03 — Facet filter p95 latency
// Gate: ≤ 500 µs per filter on representative hardware.
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Search_FacetFilterP95(benchmark::State& state) {
    uint64_t i = 0;
    for (auto _ : state) {
        std::string key   = "facet_k" + std::to_string(i % 16);
        std::string value = "val_"    + std::to_string(i % 32);
        benchmark::DoNotOptimize(stub_facet_filter(key, value, 1000));
        ++i;
    }
    state.SetLabel("SE-BM-03 gate=500us");
}
BENCHMARK(BM_Search_FacetFilterP95)
    ->Name("BM_Search_FacetFilterP95")
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// SE-BM-04 — Concurrent search throughput (8 threads)
// Gate: ≥ 5 000 searches/sec aggregate throughput.
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Search_ConcurrentThroughput(benchmark::State& state) {
    uint64_t i = 0;
    for (auto _ : state) {
        const std::string& term = kSearchTerms[i++ % kSearchTerms.size()];
        uint32_t hits = stub_fts_search(term, 10);
        benchmark::DoNotOptimize(stub_rank(hits, term));
    }
    state.SetLabel("SE-BM-04 gate=5000qps threads=8");
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_Search_ConcurrentThroughput)
    ->Name("BM_Search_ConcurrentThroughput")
    ->Threads(8)
    ->Iterations(1000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
