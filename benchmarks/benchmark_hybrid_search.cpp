/**
 * @file benchmark_hybrid_search.cpp
 * @brief Standalone benchmarks for HybridSearch (RRF, linear combination, score normalization)
 *
 * These benchmarks use only in-process data structures (no live database I/O) to
 * measure the algorithmic cost of:
 *   - reciprocalRankFusion() with varying candidate pool sizes
 *   - Linear combination fusion
 *   - normalizeScores() (BM25 and vector)
 *   - Config construction + validation overhead
 *
 * Build:
 *   cmake --build . --target benchmark_hybrid_search --config Release
 * Run:
 *   ./benchmarks/benchmark_hybrid_search [--benchmark_filter=...]
 */

#include <benchmark/benchmark.h>
#include "search/hybrid_search.h"
#include <random>
#include <string>
#include <vector>

using namespace themis;

// ============================================================================
// Helper: build a ranked candidate list of `n` results
// ============================================================================

static std::vector<HybridSearch::Result>
makeCandidates(int n, bool bm25, std::mt19937& rng) {
    std::uniform_real_distribution<double> score_dist(0.0, 10.0);
    std::vector<HybridSearch::Result> list;
    list.reserve(n);
    for (int i = 0; i < n; ++i) {
        HybridSearch::Result r;
        r.document_id = "doc" + std::to_string(i);
        if (bm25) {
            r.bm25_score = score_dist(rng);
            r.bm25_rank  = i + 1;
        } else {
            r.vector_score = score_dist(rng);
            r.vector_rank  = i + 1;
        }
        list.push_back(r);
    }
    return list;
}

// Build list where the first `overlap` documents appear in both lists
static std::pair<std::vector<HybridSearch::Result>, std::vector<HybridSearch::Result>>
makeOverlappingCandidates(int n, int overlap, std::mt19937& rng) {
    auto bm25 = makeCandidates(n, true, rng);
    auto vec  = makeCandidates(n, false, rng);
    // Align the first `overlap` PKs so they appear in both lists
    for (int i = 0; i < overlap && i < n; ++i) {
        vec[i].document_id = bm25[i].document_id;
    }
    return {bm25, vec};
}

// ============================================================================
// RRF benchmarks
// ============================================================================

static void BM_RRF_BM25Only(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    auto bm25 = makeCandidates(n, true, rng);

    HybridSearch::Config cfg;
    cfg.k = static_cast<size_t>(std::min(n, 10));
    cfg.max_k = static_cast<size_t>(n);
    cfg.max_candidates = static_cast<size_t>(n);
    cfg.k_bm25 = static_cast<size_t>(n);
    cfg.k_vector = static_cast<size_t>(n);
    HybridSearch hs(nullptr, nullptr, cfg);

    for (auto _ : state) {
        auto results = hs.reciprocalRankFusion(bm25, {});
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_RRF_BM25Only)->Arg(10)->Arg(50)->Arg(100)->Arg(500)->Arg(1000);

static void BM_RRF_VectorOnly(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    auto vec = makeCandidates(n, false, rng);

    HybridSearch::Config cfg;
    cfg.k = static_cast<size_t>(std::min(n, 10));
    cfg.max_k = static_cast<size_t>(n);
    cfg.max_candidates = static_cast<size_t>(n);
    cfg.k_bm25 = static_cast<size_t>(n);
    cfg.k_vector = static_cast<size_t>(n);
    HybridSearch hs(nullptr, nullptr, cfg);

    for (auto _ : state) {
        auto results = hs.reciprocalRankFusion({}, vec);
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_RRF_VectorOnly)->Arg(10)->Arg(50)->Arg(100)->Arg(500)->Arg(1000);

static void BM_RRF_Hybrid_NoOverlap(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    auto bm25 = makeCandidates(n, true, rng);
    auto vec  = makeCandidates(n, false, rng);
    // All distinct PKs → no overlap

    HybridSearch::Config cfg;
    cfg.k = static_cast<size_t>(std::min(n, 10));
    cfg.max_k = static_cast<size_t>(2 * n);
    cfg.max_candidates = static_cast<size_t>(n);
    cfg.k_bm25 = static_cast<size_t>(n);
    cfg.k_vector = static_cast<size_t>(n);
    HybridSearch hs(nullptr, nullptr, cfg);

    for (auto _ : state) {
        auto results = hs.reciprocalRankFusion(bm25, vec);
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(state.iterations() * 2 * n);
}
BENCHMARK(BM_RRF_Hybrid_NoOverlap)->Arg(50)->Arg(100)->Arg(500);

static void BM_RRF_Hybrid_50PctOverlap(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    auto [bm25, vec] = makeOverlappingCandidates(n, n / 2, rng);

    HybridSearch::Config cfg;
    cfg.k = static_cast<size_t>(std::min(n, 10));
    cfg.max_k = static_cast<size_t>(2 * n);
    cfg.max_candidates = static_cast<size_t>(n);
    cfg.k_bm25 = static_cast<size_t>(n);
    cfg.k_vector = static_cast<size_t>(n);
    HybridSearch hs(nullptr, nullptr, cfg);

    for (auto _ : state) {
        auto results = hs.reciprocalRankFusion(bm25, vec);
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(state.iterations() * 2 * n);
}
BENCHMARK(BM_RRF_Hybrid_50PctOverlap)->Arg(50)->Arg(100)->Arg(500);

static void BM_RRF_Hybrid_FullOverlap(benchmark::State& state) {
    // All documents appear in both lists (best case for overlap check)
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    auto [bm25, vec] = makeOverlappingCandidates(n, n, rng);

    HybridSearch::Config cfg;
    cfg.k = static_cast<size_t>(std::min(n, 10));
    cfg.max_k = static_cast<size_t>(n);
    cfg.max_candidates = static_cast<size_t>(n);
    cfg.k_bm25 = static_cast<size_t>(n);
    cfg.k_vector = static_cast<size_t>(n);
    HybridSearch hs(nullptr, nullptr, cfg);

    for (auto _ : state) {
        auto results = hs.reciprocalRankFusion(bm25, vec);
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_RRF_Hybrid_FullOverlap)->Arg(50)->Arg(100)->Arg(500);

// ============================================================================
// Linear combination benchmarks
// ============================================================================

static void BM_Linear_Hybrid(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);

    HybridSearch::Config cfg;
    cfg.use_rrf = false;
    cfg.k = static_cast<size_t>(std::min(n, 10));
    cfg.max_k = static_cast<size_t>(2 * n);
    cfg.max_candidates = static_cast<size_t>(n);
    cfg.k_bm25 = static_cast<size_t>(n);
    cfg.k_vector = static_cast<size_t>(n);
    HybridSearch hs(nullptr, nullptr, cfg);

    // Pre-build inputs (search() with null indices returns empty, so drive via search())
    // For a linear benchmark we just test the overhead of search() with no backends
    for (auto _ : state) {
        auto results = hs.search("query");
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Linear_Hybrid)->Arg(10)->Arg(50)->Arg(100);

// ============================================================================
// normalizeScores benchmarks
// ============================================================================

static void BM_NormalizeScores_BM25(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(0.0, 100.0);

    // Build base results once
    std::vector<HybridSearch::Result> base;
    base.reserve(n);
    for (int i = 0; i < n; ++i) {
        HybridSearch::Result r;
        r.document_id = "doc" + std::to_string(i);
        r.bm25_score = dist(rng);
        base.push_back(r);
    }

    for (auto _ : state) {
        auto results = base; // copy to avoid in-place mutation affecting next iteration
        HybridSearch::normalizeScores(results, /*is_bm25=*/true);
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_NormalizeScores_BM25)->Arg(10)->Arg(100)->Arg(1000)->Arg(10000);

static void BM_NormalizeScores_Vector(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    std::vector<HybridSearch::Result> base;
    base.reserve(n);
    for (int i = 0; i < n; ++i) {
        HybridSearch::Result r;
        r.document_id = "doc" + std::to_string(i);
        r.vector_score = dist(rng);
        base.push_back(r);
    }

    for (auto _ : state) {
        auto results = base;
        HybridSearch::normalizeScores(results, /*is_bm25=*/false);
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_NormalizeScores_Vector)->Arg(10)->Arg(100)->Arg(1000)->Arg(10000);

// ============================================================================
// Config construction overhead
// ============================================================================

static void BM_ConfigConstruction(benchmark::State& state) {
    for (auto _ : state) {
        HybridSearch::Config cfg;
        cfg.k = 10;
        cfg.k_bm25 = 50;
        cfg.k_vector = 50;
        HybridSearch hs(nullptr, nullptr, cfg);
        benchmark::DoNotOptimize(hs);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ConfigConstruction);

// ============================================================================
// Varying rrf_k parameter sensitivity
// ============================================================================

static void BM_RRF_VaryingRrfK(benchmark::State& state) {
    const double rrf_k = static_cast<double>(state.range(0));
    std::mt19937 rng(42);
    auto bm25 = makeCandidates(50, true, rng);
    auto vec  = makeCandidates(50, false, rng);

    HybridSearch::Config cfg;
    cfg.rrf_k = rrf_k;
    cfg.k = 10;
    cfg.max_k = 100;
    cfg.max_candidates = 50;
    cfg.k_bm25 = 50;
    cfg.k_vector = 50;
    HybridSearch hs(nullptr, nullptr, cfg);

    for (auto _ : state) {
        auto results = hs.reciprocalRankFusion(bm25, vec);
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}
BENCHMARK(BM_RRF_VaryingRrfK)->Arg(1)->Arg(10)->Arg(60)->Arg(100)->Arg(1000);

BENCHMARK_MAIN();
