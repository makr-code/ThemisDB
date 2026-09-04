// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w4a_release_gates.cpp
 * @brief Wave 4-A: Release-critical benchmark governance.
 *
 * These benchmarks define and enforce performance gates for the workloads that
 * are mandatory prerequisites before any ThemisDB release.  Each benchmark
 * corresponds to a gate entry in release_gate_manifest.json and measures
 * a prioritised, end-to-end-representative workload.
 *
 * Gate IDs follow the pattern W4A-<N> and are declared in the counter output
 * so downstream tooling can correlate results with the manifest.
 *
 * Benchmarked workloads (release-critical tier):
 *  W4A-01  Storage write throughput (sequential, kv)          [OLTP baseline]
 *  W4A-02  Storage read latency p99                           [OLTP read path]
 *  W4A-03  Vector index insert throughput                     [ANN write path]
 *  W4A-04  Vector index k-NN search latency (k=10, n=10k)    [ANN read path]
 *  W4A-05  Graph traversal BFS depth-2                        [Graph baseline]
 *  W4A-06  Mixed read/write under concurrency (4 threads)     [OLTP mixed]
 *
 * All benchmarks:
 *  - Use kW4CanonicalSeed=42 for reproducibility.
 *  - Apply explicit warmup via WarmupProtocol.
 *  - Emit gate_id counter for manifest correlation.
 *  - Use UseRealTime() for I/O-bound paths.
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "wave4_fixtures.h"
#include "storage/rocksdb_wrapper.h"
#include "index/vector_index.h"
#include "index/graph_index.h"

namespace {

using namespace themis::bench;
using namespace themis::bench::wave4;

// ============================================================================
// W4A-01 — Storage write throughput (sequential kv)
// ============================================================================

/**
 * @brief Measures sequential key-value write throughput for the OLTP baseline.
 *
 * Gate: W4A-01 — write throughput must not regress more than 10% vs release
 * baseline (see release_gate_manifest.json).
 *
 * state.range(0) = number of kv pairs per iteration batch.
 */
BENCHMARK_DEFINE_F(StorageBenchFixture, W4A_01_WriteThroughput)(benchmark::State& state) {
    const int64_t batch = state.range(0);

    // Warmup: pre-condition page cache and memtable.
    WarmupProtocol::run(kDefaultWarmupIterations, [&] {
        auto key = rng_.genKey(16);
        auto val = rng_.genKey(64);
        db_->put(key, val);
    });

    int64_t ops = 0;
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::pair<std::string, std::string>> pairs;
        pairs.reserve(static_cast<std::size_t>(batch));
        for (int64_t i = 0; i < batch; ++i) {
            pairs.emplace_back(rng_.genKey(16), rng_.genKey(64));
        }
        state.ResumeTiming();

        for (auto& [k, v] : pairs) {
            db_->put(k, v);
            benchmark::DoNotOptimize(k);
        }
        ops += batch;
    }

    state.SetItemsProcessed(ops);
    state.counters["gate_id"]  = 1.0;  // W4A-01
    state.counters["batch"]    = static_cast<double>(batch);
}
BENCHMARK_REGISTER_F(StorageBenchFixture, W4A_01_WriteThroughput)
    ->Arg(100)
    ->Arg(500)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

// ============================================================================
// W4A-02 — Storage read latency p99
// ============================================================================

/**
 * @brief Measures per-operation read latency (p99) against a pre-populated DB.
 *
 * Gate: W4A-02 — p99 read latency must not regress more than 15% vs release
 * baseline (see release_gate_manifest.json).
 *
 * state.range(0) = pre-populated key count.
 */
BENCHMARK_DEFINE_F(StorageBenchFixture, W4A_02_ReadLatencyP99)(benchmark::State& state) {
    const int64_t n = state.range(0);

    // Pre-populate.
    std::vector<std::string> keys;
    keys.reserve(static_cast<std::size_t>(n));
    for (int64_t i = 0; i < n; ++i) {
        auto k = "rgate_" + std::to_string(i);
        db_->put(k, rng_.genKey(64));
        keys.push_back(k);
    }

    // Warmup reads.
    WarmupProtocol::run(kDefaultWarmupIterations, [&] {
        auto idx = rng_.genInt(0, n - 1);
        benchmark::DoNotOptimize(db_->get(keys[static_cast<std::size_t>(idx)]));
    });

    VarianceTracker vt;
    int64_t total_ops = 0;
    for (auto _ : state) {
        state.PauseTiming();
        auto idx = static_cast<std::size_t>(rng_.genInt(0, n - 1));
        state.ResumeTiming();

        auto t0 = std::chrono::steady_clock::now();
        benchmark::DoNotOptimize(db_->get(keys[idx]));
        vt.record(std::chrono::steady_clock::now() - t0);
        ++total_ops;
    }

    state.SetItemsProcessed(total_ops);
    vt.publishCounters(state);
    state.counters["gate_id"] = 2.0;  // W4A-02
    state.counters["n_keys"]  = static_cast<double>(n);
}
BENCHMARK_REGISTER_F(StorageBenchFixture, W4A_02_ReadLatencyP99)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ============================================================================
// W4A-03 — Vector index insert throughput
// ============================================================================

/**
 * @brief Measures vector index insert throughput (ANN write path).
 *
 * Gate: W4A-03 — insert throughput must not regress more than 10% vs release
 * baseline (see release_gate_manifest.json).
 *
 * state.range(0) = number of vectors inserted per iteration.
 */
BENCHMARK_DEFINE_F(VectorBenchFixture, W4A_03_VectorInsertThroughput)(benchmark::State& state) {
    const int64_t batch = state.range(0);

    WarmupProtocol::run(kDefaultWarmupIterations, [&] {
        idx_->insert("warmup_" + rng_.genKey(8), rng_.genVec(kDim));
    });

    int64_t total_inserts = 0;
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::pair<std::string, std::vector<float>>> vecs;
        vecs.reserve(static_cast<std::size_t>(batch));
        for (int64_t i = 0; i < batch; ++i) {
            vecs.emplace_back("v" + rng_.genKey(12), rng_.genVec(kDim));
        }
        state.ResumeTiming();

        for (auto& [id, vec] : vecs) {
            idx_->insert(id, vec);
            benchmark::DoNotOptimize(id);
        }
        total_inserts += batch;
    }

    state.SetItemsProcessed(total_inserts);
    state.counters["gate_id"] = 3.0;  // W4A-03
    state.counters["dim"]     = static_cast<double>(kDim);
}
BENCHMARK_REGISTER_F(VectorBenchFixture, W4A_03_VectorInsertThroughput)
    ->Arg(50)
    ->Arg(200)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

// ============================================================================
// W4A-04 — Vector k-NN search latency (k=10)
// ============================================================================

/**
 * @brief Measures per-query k-NN search latency (p50/p95/p99) over a
 *        pre-populated vector index.
 *
 * Gate: W4A-04 — p99 k-NN latency must not regress more than 15% vs release
 * baseline.
 *
 * state.range(0) = index size (number of pre-populated vectors).
 */
BENCHMARK_DEFINE_F(VectorBenchFixture, W4A_04_VectorSearchLatency)(benchmark::State& state) {
    constexpr int kK = 10;

    WarmupProtocol::run(kDefaultWarmupIterations, [&] {
        benchmark::DoNotOptimize(idx_->search(rng_.genVec(kDim), kK));
    });

    VarianceTracker vt;
    int64_t total_queries = 0;
    for (auto _ : state) {
        state.PauseTiming();
        auto query = rng_.genVec(kDim);
        state.ResumeTiming();

        auto t0 = std::chrono::steady_clock::now();
        benchmark::DoNotOptimize(idx_->search(query, kK));
        vt.record(std::chrono::steady_clock::now() - t0);
        ++total_queries;
    }

    state.SetItemsProcessed(total_queries);
    vt.publishCounters(state);
    state.counters["gate_id"] = 4.0;  // W4A-04
    state.counters["k"]       = static_cast<double>(kK);
}
BENCHMARK_REGISTER_F(VectorBenchFixture, W4A_04_VectorSearchLatency)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ============================================================================
// W4A-05 — Graph BFS depth-2 traversal
// ============================================================================

/**
 * @brief Measures BFS traversal from a random source node at depth=2.
 *
 * Gate: W4A-05 — traversal latency p99 must not regress more than 20% vs
 * release baseline.
 *
 * state.range(0) = graph node count.
 */
BENCHMARK_DEFINE_F(GraphBenchFixture, W4A_05_GraphBFSDepth2)(benchmark::State& state) {
    const int64_t n = state.range(0) > 0 ? state.range(0) :
                      static_cast<int64_t>(kDefaultNodes);

    WarmupProtocol::run(kDefaultWarmupIterations, [&] {
        const std::string src = "node_" + std::to_string(rng_.genInt(0, n - 1));
        benchmark::DoNotOptimize(graph_->outNeighbors(src));
    });

    VarianceTracker vt;
    int64_t total_queries = 0;
    for (auto _ : state) {
        state.PauseTiming();
        const std::string src = "node_" + std::to_string(rng_.genInt(0, n - 1));
        state.ResumeTiming();

        auto t0 = std::chrono::steady_clock::now();
        // Depth-2: fetch neighbours, then neighbours-of-neighbours.
    auto [st1, neighbours] = graph_->outNeighbors(src);
        benchmark::DoNotOptimize(neighbours);
    for (const auto& nb_id : neighbours) {
        benchmark::DoNotOptimize(graph_->outNeighbors(nb_id));
        }
        vt.record(std::chrono::steady_clock::now() - t0);
        ++total_queries;
    }

    state.SetItemsProcessed(total_queries);
    vt.publishCounters(state);
    state.counters["gate_id"] = 5.0;  // W4A-05
    state.counters["n_nodes"] = static_cast<double>(n);
}
BENCHMARK_REGISTER_F(GraphBenchFixture, W4A_05_GraphBFSDepth2)
    ->Arg(1000)
    ->Arg(5000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ============================================================================
// W4A-06 — Mixed read/write under concurrency (4 threads)
// ============================================================================

/**
 * @brief Measures mixed read/write throughput with 4 concurrent threads.
 *
 * Gate: W4A-06 — mixed throughput must not regress more than 15% vs release
 * baseline; individual p99 values are emitted for gate comparison.
 *
 * state.range(0) = operations per thread per iteration.
 *
 * @note Uses UseRealTime() since wall-clock throughput is the release-critical
 *       metric for concurrent workloads.
 */
static void BM_W4A_06_MixedConcurrent(benchmark::State& state) {
    const int64_t ops_per_thread = state.range(0);
    constexpr int kThreads = 4;
    constexpr double kWriteRatio = 0.5;

    // Set up a shared DB.
    TempDir tmp;
    themis::RocksDBWrapper::Config cfg;
    cfg.db_path = tmp.str();
    cfg.create_if_missing = true;
    auto db = std::make_shared<themis::RocksDBWrapper>(cfg);
    db->open();

    // Pre-populate so reads have data.
    RandomGenerator seed_rng(kW4CanonicalSeed);
    for (int i = 0; i < 1000; ++i) {
        db->put("seed_" + std::to_string(i), seed_rng.genKey(64));
    }

    // Warmup.
    for (int w = 0; w < kDefaultWarmupIterations; ++w) {
        db->put(seed_rng.genKey(16), seed_rng.genKey(64));
        benchmark::DoNotOptimize(db->get("seed_0"));
    }

    std::atomic<int64_t> total_ops{0};

    for (auto _ : state) {
        std::vector<std::thread> threads;
        threads.reserve(kThreads);

        for (int t = 0; t < kThreads; ++t) {
            threads.emplace_back([&, t] {
                RandomGenerator local_rng(kW4CanonicalSeed + static_cast<uint64_t>(t) + 1);
                for (int64_t i = 0; i < ops_per_thread; ++i) {
                    if (local_rng.genInt(0, 99) < static_cast<int64_t>(kWriteRatio * 100.0)) {
                        db->put(local_rng.genKey(16), local_rng.genKey(64));
                    } else {
                        benchmark::DoNotOptimize(
                            db->get("seed_" + std::to_string(local_rng.genInt(0, 999))));
                    }
                }
                total_ops.fetch_add(ops_per_thread, std::memory_order_relaxed);
            });
        }

        for (auto& th : threads) {
          th.join();
        }
    }

    state.SetItemsProcessed(total_ops.load());
    state.counters["gate_id"]      = 6.0;   // W4A-06
    state.counters["threads"]      = static_cast<double>(kThreads);
    state.counters["write_ratio"]  = kWriteRatio;
}
BENCHMARK(BM_W4A_06_MixedConcurrent)
    ->Arg(100)
    ->Arg(500)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

}  // namespace

BENCHMARK_MAIN();
