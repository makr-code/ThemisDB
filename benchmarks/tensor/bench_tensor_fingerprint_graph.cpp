/*
 * bench_tensor_fingerprint_graph.cpp
 *
 * Google Benchmark suite for TensorFingerprintGraph.
 *
 * Targets (include/graph/tensor_fingerprint_graph.h §Performance):
 *   insert()      ≤ 10 ms for graph up to 100 K nodes
 *   findSimilar() ≤ 50 ms for graph up to 100 K nodes
 *   neighbours()  ≤  5 ms  (direct adjacency lookup)
 *
 * Benchmarks:
 *   BM_TFG_Insert_Throughput    – amortised insert latency (batch)
 *   BM_TFG_Insert_SingleNode    – per-node insert latency (parameterised)
 *   BM_TFG_FindSimilar          – findSimilar() at varying graph sizes
 *   BM_TFG_Neighbours           – neighbours() adjacency read
 *   BM_TFG_ConcurrentReads      – parallel findSimilar() (shared_mutex benefit)
 *   BM_TFG_NodeCount            – nodeCount() read under load
 *   BM_TFG_ExportPersistedGraph – full-graph export at 1K / 10K nodes
 */

#include "graph/tensor_fingerprint_graph.h"
#include "storage/tensor_train_decomposer.h"

#include <benchmark/benchmark.h>

#include <atomic>
#include <cmath>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace themis;
using themis::graph::TensorFingerprintGraph;
using themis::storage::TTCore;
using themis::storage::TTTrain;

// ─────────────────────────────────────────────────────────────────────────────
// Synthetic TTTrain factory
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// Build a synthetic Tensor-Train with `order` cores each of shape (1 × mode_n × 1).
// Elements are drawn from a seeded uniform distribution so benchmarks are
// deterministic but varied enough to exercise real similarity calculations.
TTTrain makeSyntheticTrain(std::mt19937& rng,
                           std::size_t order = 4,
                           std::size_t mode_n = 16)
{
    TTTrain train;
    train.mode_sizes.resize(order, mode_n);
    train.original_norm = 1.0;
    train.achieved_eps  = 0.01;

    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    for (std::size_t k = 0; k < order; ++k) {
        TTCore core;
        core.r_left  = 1;
        core.n       = mode_n;
        core.r_right = 1;
        core.data.resize(mode_n);
        for (auto& v : core.data) v = dist(rng);
        train.cores.push_back(std::move(core));
    }
    return train;
}

// Populate an existing graph with `node_count` pre-inserted nodes.
// Uses a fixed seed so construction cost is consistent across benchmark runs.
void populateGraph(TensorFingerprintGraph& graph, std::size_t node_count)
{
    std::mt19937 rng(0xABCD1234u);

    for (std::size_t i = 0; i < node_count; ++i) {
        const auto train = makeSyntheticTrain(rng);
        graph.insert("node_" + std::to_string(i), train, "bench", "tensors", "embedding");
    }
}

// Config with reduced LSH bands to stay within sub-50ms for large graphs.
graph::FingerprintGraphConfig benchConfig()
{
    graph::FingerprintGraphConfig cfg;
    cfg.num_hash_funcs = 64;  // standard
    cfg.num_bands      = 8;   // 8 bands of 8 rows → good recall/precision tradeoff
    cfg.similarity_threshold = 0.75f;
    cfg.top_k = 10;
    return cfg;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// BM_TFG_Insert_Throughput
//
// Inserts `nodes_per_iter` nodes into an empty graph and measures wall-clock
// time per insert. Each iteration creates a fresh graph to avoid measuring
// steady-state overhead vs. initial insertion.
//
// Param: number of nodes inserted per benchmark iteration (state.range(0))
// ─────────────────────────────────────────────────────────────────────────────

static void BM_TFG_Insert_Throughput(benchmark::State& state)
{
    const std::size_t nodes_per_iter = static_cast<std::size_t>(state.range(0));
    const auto cfg = benchConfig();

    std::mt19937 rng(0xDEAD'BEEFu);
    std::vector<TTTrain> trains;
    trains.reserve(nodes_per_iter);
    for (std::size_t i = 0; i < nodes_per_iter; ++i) {
        trains.push_back(makeSyntheticTrain(rng));
    }

    for (auto _ : state) {
        TensorFingerprintGraph graph(cfg);
        for (std::size_t i = 0; i < nodes_per_iter; ++i) {
            graph.insert("n_" + std::to_string(i), trains[i],
                         "bench", "col", "field");
        }
        benchmark::DoNotOptimize(graph.nodeCount());
    }

    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) *
        static_cast<int64_t>(nodes_per_iter));
    state.SetLabel("nodes/iter=" + std::to_string(nodes_per_iter));
}

BENCHMARK(BM_TFG_Insert_Throughput)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.5);

// ─────────────────────────────────────────────────────────────────────────────
// BM_TFG_Insert_SingleNode
//
// Measures the per-node insert latency into an already-populated graph of
// `state.range(0)` nodes (steady-state cost, not amortised).
// Target: ≤ 10 ms per insert at 100 K nodes.
// ─────────────────────────────────────────────────────────────────────────────

static void BM_TFG_Insert_SingleNode(benchmark::State& state)
{
    const std::size_t prefill = static_cast<std::size_t>(state.range(0));
    const auto cfg = benchConfig();
    TensorFingerprintGraph graph(cfg);
    populateGraph(graph, prefill);

    std::mt19937 rng(0x1234ABCDu);
    std::size_t idx = prefill;

    for (auto _ : state) {
        auto train = makeSyntheticTrain(rng);
        graph.insert("extra_" + std::to_string(idx++), train,
                     "bench", "col", "field");
    }

    state.SetLabel("prefill=" + std::to_string(prefill));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_TFG_Insert_SingleNode)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(50000)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.3);

// ─────────────────────────────────────────────────────────────────────────────
// BM_TFG_FindSimilar
//
// Measures findSimilar() latency against a pre-populated graph.
// Target: p95 ≤ 80 ms, p99 ≤ 140 ms for 10k nodes (Q3 2026).
// ─────────────────────────────────────────────────────────────────────────────

static void BM_TFG_FindSimilar(benchmark::State& state)
{
    const std::size_t prefill = static_cast<std::size_t>(state.range(0));
    const std::size_t top_k   = static_cast<std::size_t>(state.range(1));
    const auto cfg = benchConfig();
    TensorFingerprintGraph graph(cfg);
    populateGraph(graph, prefill);

    std::mt19937 rng(0x9876543u);
    auto query = makeSyntheticTrain(rng);

    for (auto _ : state) {
        auto results = graph.findSimilar(query, top_k);
        benchmark::DoNotOptimize(results.size());
    }

    state.SetLabel("nodes=" + std::to_string(prefill) +
                   " top_k=" + std::to_string(top_k));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_TFG_FindSimilar)
    ->Args({100,   10})
    ->Args({1000,  10})
    ->Args({10000, 10})
    ->Args({50000, 10})
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.5)
    ->Repetitions(5)
    ->UseRealTime();

// ─────────────────────────────────────────────────────────────────────────────
// BM_TFG_Neighbours
//
// Measures neighbours() adjacency-list lookup.
// Target: ≤ 5 ms (direct hash-map lookup, should be O(1)).
// ─────────────────────────────────────────────────────────────────────────────

static void BM_TFG_Neighbours(benchmark::State& state)
{
    const std::size_t prefill = static_cast<std::size_t>(state.range(0));
    const auto cfg = benchConfig();
    TensorFingerprintGraph graph(cfg);
    populateGraph(graph, prefill);

    // Query a node known to be in the graph (node_0 always inserted first).
    const std::string query_id = "node_0";

    for (auto _ : state) {
        auto nbrs = graph.neighbours(query_id);
        benchmark::DoNotOptimize(nbrs.size());
    }

    state.SetLabel("nodes=" + std::to_string(prefill));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_TFG_Neighbours)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(0.3)
    ->Repetitions(5);

// ─────────────────────────────────────────────────────────────────────────────
// BM_TFG_ConcurrentReads
//
// Measures aggregate findSimilar() throughput with N concurrent reader
// threads. With shared_mutex, throughput should scale near-linearly.
// Param: number of reader threads (state.range(0)).
// ─────────────────────────────────────────────────────────────────────────────

static void BM_TFG_ConcurrentReads(benchmark::State& state)
{
    const int n_readers = static_cast<int>(state.range(0));
    const auto cfg = benchConfig();
    TensorFingerprintGraph graph(cfg);
    populateGraph(graph, 5000);

    std::mt19937 rng(0xFEEDFACEu);
    auto query = makeSyntheticTrain(rng);

    for (auto _ : state) {
        std::atomic<int> barrier{n_readers};
        std::vector<std::thread> threads;
        threads.reserve(static_cast<std::size_t>(n_readers));

        for (int t = 0; t < n_readers; ++t) {
            threads.emplace_back([&]() {
                // Spin-wait until all threads are ready (reduce scheduling skew).
                barrier.fetch_sub(1, std::memory_order_release);
                while (barrier.load(std::memory_order_acquire) > 0) {
                    std::this_thread::yield();
                }
                auto results = graph.findSimilar(query, 10);
                benchmark::DoNotOptimize(results.size());
            });
        }
        for (auto& th : threads) th.join();
    }

    state.SetLabel("readers=" + std::to_string(n_readers));
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * n_readers);
}

BENCHMARK(BM_TFG_ConcurrentReads)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.5)
    ->UseRealTime();

// ─────────────────────────────────────────────────────────────────────────────
// BM_TFG_NodeCount
//
// nodeCount() under a populated graph. Should be O(1) / very fast since
// nodeCount() holds a shared_lock and reads an unordered_map::size().
// ─────────────────────────────────────────────────────────────────────────────

static void BM_TFG_NodeCount(benchmark::State& state)
{
    const std::size_t prefill = static_cast<std::size_t>(state.range(0));
    const auto cfg = benchConfig();
    TensorFingerprintGraph graph(cfg);
    populateGraph(graph, prefill);

    for (auto _ : state) {
        auto n = graph.nodeCount();
        benchmark::DoNotOptimize(n);
    }

    state.SetLabel("nodes=" + std::to_string(prefill));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_TFG_NodeCount)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kNanosecond)
    ->MinTime(0.1);

// ─────────────────────────────────────────────────────────────────────────────
// BM_TFG_ExportPersistedGraph
//
// Full-graph serialisation (exportPersistedGraph). Used by persistence/
// checkpoint paths. Measures cost proportional to node count.
// ─────────────────────────────────────────────────────────────────────────────

static void BM_TFG_ExportPersistedGraph(benchmark::State& state)
{
    const std::size_t prefill = static_cast<std::size_t>(state.range(0));
    const auto cfg = benchConfig();
    TensorFingerprintGraph graph(cfg);
    populateGraph(graph, prefill);

    for (auto _ : state) {
        auto snapshot = graph.exportPersistedGraph();
        benchmark::DoNotOptimize(snapshot.nodes.size());
    }

    state.SetLabel("nodes=" + std::to_string(prefill));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_TFG_ExportPersistedGraph)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(5000)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.3);

// ─────────────────────────────────────────────────────────────────────────────

BENCHMARK_MAIN();
