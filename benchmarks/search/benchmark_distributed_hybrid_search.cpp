/**
 * @file benchmark_distributed_hybrid_search.cpp
 * @brief Benchmarks for DistributedHybridSearch (cross-shard RRF merge, search dispatch)
 *
 * These benchmarks measure the algorithmic cost of:
 *   - mergeShardResults() with varying shard count and result pool sizes
 *   - Cross-shard RRF with varying document overlap ratios
 *   - Config construction + validation overhead
 *
 * All benchmarks are in-process (no network I/O) so they measure pure algorithm cost.
 *
 * Build:
 *   cmake --build . --target benchmark_distributed_hybrid_search --config Release
 * Run:
 *   ./benchmarks/benchmark_distributed_hybrid_search [--benchmark_filter=...]
 */

#include <benchmark/benchmark.h>
#include "search/distributed_hybrid_search.h"
#include <random>
#include <string>
#include <vector>

using namespace themis;

// ============================================================================
// Helpers
// ============================================================================

static DistributedHybridSearch::Config makeCfg(size_t k = 10) {
    DistributedHybridSearch::Config cfg;
    cfg.k = k;
    cfg.rrf_k = 60.0;
    cfg.max_concurrent_shards = 8;
    return cfg;
}

// Build a result list where document IDs are "doc_<shard>_<rank>"
// with optional overlap: the first `overlap` documents share the same ID
// prefix "doc_shared_<rank>" across shards.
static DistributedHybridSearch::ShardSearchResult
makeShardResult(const std::string& shard_id, int n, int overlap,
                std::mt19937& rng) {
    std::uniform_real_distribution<double> score_dist(0.0, 1.0);

    DistributedHybridSearch::ShardSearchResult sr;
    sr.shard_id = shard_id;
    sr.success  = true;
    sr.results.reserve(n);

    for (int i = 0; i < n; ++i) {
        HybridSearch::Result r;
        if (i < overlap) {
            r.document_id = "doc_shared_" + std::to_string(i);
        } else {
            r.document_id = "doc_" + shard_id + "_" + std::to_string(i);
        }
        r.hybrid_score = score_dist(rng);
        r.bm25_score   = score_dist(rng);
        r.vector_score = score_dist(rng);
        r.bm25_rank    = i + 1;
        r.vector_rank  = i + 1;
        sr.results.push_back(std::move(r));
    }
    return sr;
}

// ============================================================================
// mergeShardResults benchmarks
// ============================================================================

// BM_MergeShardResults_ShardCount: vary number of shards, fixed results per shard
static void BM_MergeShardResults_ShardCount(benchmark::State& state) {
    const int num_shards      = static_cast<int>(state.range(0));
    const int results_per_shard = 100;
    const int overlap         = 10;   // 10 shared documents across all shards

    std::mt19937 rng(42);

    std::vector<DistributedHybridSearch::ShardSearchResult> shard_results;
    shard_results.reserve(num_shards);
    for (int s = 0; s < num_shards; ++s) {
        shard_results.push_back(
            makeShardResult("shard_" + std::to_string(s),
                            results_per_shard, overlap, rng));
    }

    DistributedHybridSearch dhs(nullptr, nullptr, nullptr, makeCfg(20));

    for (auto _ : state) {
        auto merged = dhs.mergeShardResults(shard_results);
        benchmark::DoNotOptimize(merged);
    }
    state.SetItemsProcessed(
        state.iterations() * num_shards * results_per_shard);
    state.counters["shards"] = num_shards;
    state.counters["docs_total"] = num_shards * results_per_shard;
}
BENCHMARK(BM_MergeShardResults_ShardCount)
    ->Arg(2)->Arg(4)->Arg(8)->Arg(16)->Arg(32);

// BM_MergeShardResults_ResultsPerShard: vary result count, fixed 4 shards
static void BM_MergeShardResults_ResultsPerShard(benchmark::State& state) {
    const int results_per_shard = static_cast<int>(state.range(0));
    const int num_shards        = 4;
    const int overlap           = results_per_shard / 10;  // 10% overlap

    std::mt19937 rng(42);

    std::vector<DistributedHybridSearch::ShardSearchResult> shard_results;
    shard_results.reserve(num_shards);
    for (int s = 0; s < num_shards; ++s) {
        shard_results.push_back(
            makeShardResult("shard_" + std::to_string(s),
                            results_per_shard, overlap, rng));
    }

    DistributedHybridSearch dhs(nullptr, nullptr, nullptr,
                                 makeCfg(static_cast<size_t>(
                                     std::min(results_per_shard, 20))));

    for (auto _ : state) {
        auto merged = dhs.mergeShardResults(shard_results);
        benchmark::DoNotOptimize(merged);
    }
    state.SetItemsProcessed(
        state.iterations() * num_shards * results_per_shard);
    state.counters["results_per_shard"] = results_per_shard;
}
BENCHMARK(BM_MergeShardResults_ResultsPerShard)
    ->Arg(10)->Arg(50)->Arg(100)->Arg(500)->Arg(1000);

// BM_MergeShardResults_Overlap: vary overlap ratio, fixed 4 shards × 100 results
static void BM_MergeShardResults_Overlap(benchmark::State& state) {
    const int overlap_pct       = static_cast<int>(state.range(0));
    const int results_per_shard = 100;
    const int num_shards        = 4;
    const int overlap = results_per_shard * overlap_pct / 100;

    std::mt19937 rng(42);

    std::vector<DistributedHybridSearch::ShardSearchResult> shard_results;
    shard_results.reserve(num_shards);
    for (int s = 0; s < num_shards; ++s) {
        shard_results.push_back(
            makeShardResult("shard_" + std::to_string(s),
                            results_per_shard, overlap, rng));
    }

    DistributedHybridSearch dhs(nullptr, nullptr, nullptr, makeCfg(20));

    for (auto _ : state) {
        auto merged = dhs.mergeShardResults(shard_results);
        benchmark::DoNotOptimize(merged);
    }
    state.SetItemsProcessed(
        state.iterations() * num_shards * results_per_shard);
    state.counters["overlap_pct"] = overlap_pct;
}
BENCHMARK(BM_MergeShardResults_Overlap)
    ->Arg(0)->Arg(10)->Arg(25)->Arg(50)->Arg(75)->Arg(100);

// BM_MergeShardResults_KLimit: vary k, fixed 4 shards × 100 results
static void BM_MergeShardResults_KLimit(benchmark::State& state) {
    const int k                 = static_cast<int>(state.range(0));
    const int results_per_shard = 100;
    const int num_shards        = 4;

    std::mt19937 rng(42);

    std::vector<DistributedHybridSearch::ShardSearchResult> shard_results;
    shard_results.reserve(num_shards);
    for (int s = 0; s < num_shards; ++s) {
        shard_results.push_back(
            makeShardResult("shard_" + std::to_string(s),
                            results_per_shard, /*overlap=*/10, rng));
    }

    DistributedHybridSearch dhs(nullptr, nullptr, nullptr,
                                 makeCfg(static_cast<size_t>(k)));

    for (auto _ : state) {
        auto merged = dhs.mergeShardResults(shard_results);
        benchmark::DoNotOptimize(merged);
    }
    state.SetItemsProcessed(
        state.iterations() * num_shards * results_per_shard);
    state.counters["k"] = k;
}
BENCHMARK(BM_MergeShardResults_KLimit)
    ->Arg(5)->Arg(10)->Arg(20)->Arg(50)->Arg(100);

// ============================================================================
// Fault tolerance overhead: failed shards
// ============================================================================

static void BM_MergeShardResults_WithFailures(benchmark::State& state) {
    const int num_shards      = 8;
    const int fail_count      = static_cast<int>(state.range(0));
    const int results_per_shard = 50;

    std::mt19937 rng(42);

    std::vector<DistributedHybridSearch::ShardSearchResult> shard_results;
    shard_results.reserve(num_shards);
    for (int s = 0; s < num_shards; ++s) {
        if (s < fail_count) {
            DistributedHybridSearch::ShardSearchResult sr;
            sr.shard_id  = "shard_" + std::to_string(s);
            sr.success   = false;
            sr.error_msg = "timeout";
            shard_results.push_back(std::move(sr));
        } else {
            shard_results.push_back(
                makeShardResult("shard_" + std::to_string(s),
                                results_per_shard, /*overlap=*/5, rng));
        }
    }

    DistributedHybridSearch dhs(nullptr, nullptr, nullptr, makeCfg(20));

    for (auto _ : state) {
        auto merged = dhs.mergeShardResults(shard_results);
        benchmark::DoNotOptimize(merged);
    }
    state.counters["failed_shards"] = fail_count;
    state.counters["success_shards"] = num_shards - fail_count;
}
BENCHMARK(BM_MergeShardResults_WithFailures)
    ->Arg(0)->Arg(1)->Arg(2)->Arg(4)->Arg(7);

// ============================================================================
// Config construction
// ============================================================================

static void BM_ConfigConstruction(benchmark::State& state) {
    for (auto _ : state) {
        DistributedHybridSearch::Config cfg;
        cfg.k = 20;
        cfg.rrf_k = 60.0;
        cfg.max_concurrent_shards = 8;
        DistributedHybridSearch dhs(nullptr, nullptr, nullptr, cfg);
        benchmark::DoNotOptimize(dhs);
    }
}
BENCHMARK(BM_ConfigConstruction);

BENCHMARK_MAIN();
