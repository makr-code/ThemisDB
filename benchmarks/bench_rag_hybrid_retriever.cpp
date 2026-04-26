/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_rag_hybrid_retriever.cpp                     ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:43:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     219                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file bench_rag_hybrid_retriever.cpp
 * @brief Google Benchmark suite for HybridRetriever (RAG Phase 3)
 *
 * Benchmarks the algorithmic cost of RRF and linear-combination fusion
 * at varying candidate pool sizes.  All benchmarks are in-process (no
 * database I/O) to isolate fusion overhead.
 *
 * Performance targets:
 *   - RRF mode:    < 1 ms for 100 candidates
 *   - Linear mode: < 1 ms for 100 candidates
 *
 * Build:
 *   cmake --build . --target bench_rag_hybrid_retriever --config Release
 * Run:
 *   ./benchmarks/bench_rag_hybrid_retriever [--benchmark_filter=...]
 */

#include <benchmark/benchmark.h>
#include "rag/hybrid_retriever.h"
#include <random>
#include <string>
#include <vector>

using namespace themis::rag;
using namespace themis::rag::judge;

// ============================================================================
// Helper: build a candidate list of `n` documents
// ============================================================================

static std::vector<RetrievedDocument>
makeCandidates(int n, double base_score, std::mt19937& rng) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    std::vector<RetrievedDocument> list;
    list.reserve(n);
    for (int i = 0; i < n; ++i) {
        RetrievedDocument d;
        d.id               = "doc" + std::to_string(i);
        d.content          = "Candidate document number " + std::to_string(i);
        d.similarity_score = base_score - static_cast<double>(i) * 0.001 + dist(rng) * 0.0001;
        list.push_back(d);
    }
    return list;
}

// ============================================================================
// RRF benchmarks: varying candidate pool size
// ============================================================================

static void BM_RRF_Balanced(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    auto bm25 = makeCandidates(n, 0.9, rng);
    auto vec  = makeCandidates(n, 0.8, rng);

    HybridRetrieverConfig cfg;
    cfg.bm25_weight   = 0.5;
    cfg.vector_weight = 0.5;
    cfg.use_rrf       = true;
    cfg.top_k         = 10;
    HybridRetriever retriever(cfg);

    for (auto _ : state) {
        auto result = retriever.fuse(bm25, vec);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * n * 2);
    state.SetLabel(std::to_string(n) + " candidates each");
}
BENCHMARK(BM_RRF_Balanced)->Arg(10)->Arg(50)->Arg(100)->Arg(500)->Arg(1000);

static void BM_RRF_BM25Only(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    auto bm25 = makeCandidates(n, 0.9, rng);

    HybridRetrieverConfig cfg;
    cfg.bm25_weight   = 1.0;
    cfg.vector_weight = 0.0;
    cfg.use_rrf       = true;
    cfg.top_k         = 10;
    HybridRetriever retriever(cfg);

    for (auto _ : state) {
        auto result = retriever.fuse(bm25, {});
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel("BM25-only, " + std::to_string(n) + " candidates");
}
BENCHMARK(BM_RRF_BM25Only)->Arg(10)->Arg(100)->Arg(1000);

static void BM_RRF_VectorOnly(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    auto vec = makeCandidates(n, 0.9, rng);

    HybridRetrieverConfig cfg;
    cfg.bm25_weight   = 0.0;
    cfg.vector_weight = 1.0;
    cfg.use_rrf       = true;
    cfg.top_k         = 10;
    HybridRetriever retriever(cfg);

    for (auto _ : state) {
        auto result = retriever.fuse({}, vec);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel("vector-only, " + std::to_string(n) + " candidates");
}
BENCHMARK(BM_RRF_VectorOnly)->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// RRF – disjoint candidate lists (worst case: all unique IDs, max map size)
// ============================================================================

static void BM_RRF_Disjoint(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    // Disjoint: BM25 uses doc0..doc(n-1), vector uses docB0..docB(n-1)
    auto bm25 = makeCandidates(n, 0.9, rng);
    std::vector<RetrievedDocument> vec = makeCandidates(n, 0.8, rng);
    for (auto& d : vec) { d.id = "b_" + d.id; }

    HybridRetrieverConfig cfg;
    cfg.use_rrf = true;
    cfg.top_k   = 10;
    HybridRetriever retriever(cfg);

    for (auto _ : state) {
        auto result = retriever.fuse(bm25, vec);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * n * 2);
    state.SetLabel("disjoint, " + std::to_string(n) + " each");
}
BENCHMARK(BM_RRF_Disjoint)->Arg(10)->Arg(100)->Arg(500);

// ============================================================================
// Linear combination benchmarks
// ============================================================================

static void BM_Linear_Balanced(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::mt19937 rng(42);
    auto bm25 = makeCandidates(n, 0.9, rng);
    auto vec  = makeCandidates(n, 0.8, rng);

    HybridRetrieverConfig cfg;
    cfg.bm25_weight      = 0.5;
    cfg.vector_weight    = 0.5;
    cfg.use_rrf          = false;
    cfg.normalize_scores = true;
    cfg.top_k            = 10;
    HybridRetriever retriever(cfg);

    for (auto _ : state) {
        auto result = retriever.fuse(bm25, vec);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * n * 2);
    state.SetLabel(std::to_string(n) + " candidates each");
}
BENCHMARK(BM_Linear_Balanced)->Arg(10)->Arg(50)->Arg(100)->Arg(500);

// ============================================================================
// Config construction overhead
// ============================================================================

static void BM_ConfigConstruction(benchmark::State& state) {
    for (auto _ : state) {
        HybridRetrieverConfig cfg;
        cfg.bm25_weight   = 0.4;
        cfg.vector_weight = 0.6;
        cfg.rrf_k         = 60.0;
        cfg.top_k         = 10;
        HybridRetriever retriever(cfg);
        benchmark::DoNotOptimize(retriever);
    }
}
BENCHMARK(BM_ConfigConstruction);

// ============================================================================
// Factory helpers overhead
// ============================================================================

static void BM_FactoryCreateBalanced(benchmark::State& state) {
    for (auto _ : state) {
        auto r = HybridRetrieverFactory::createBalanced(10);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_FactoryCreateBalanced);

BENCHMARK_MAIN();
