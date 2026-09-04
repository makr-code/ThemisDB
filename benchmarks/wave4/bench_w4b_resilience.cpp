// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w4b_resilience.cpp
 * @brief Wave 4-B: Resilience and degradation performance scenarios.
 *
 * Benchmarks in this file measure ThemisDB performance under degraded
 * operating conditions:
 *
 *  W4B-01  Storage writes under artificial latency injection (mild/severe)
 *  W4B-02  Storage reads under backpressure (rate-limited I/O path)
 *  W4B-03  Vector search under resource scarcity (high CPU contention)
 *  W4B-04  Write path throughput under sustained backpressure
 *  W4B-05  Graph traversal under node-failure partial availability
 *  W4B-06  Recovery throughput after simulated partial data loss
 *
 * Design principles:
 *  - All degradation profiles are deterministic and seed-stable.
 *  - p50/p95/p99 and throughput are emitted as counters for regression
 *    comparison against the non-degraded baseline from W4A.
 *  - Degradation mode is encoded in state.range(0) and annotated as a
 *    counter so results can be filtered/compared across runs.
 *  - UseRealTime() on all I/O-bound benchmarks.
 *
 * Degradation modes for DegradedStorageFixture (range(0)):
 *   0 = baseline (no degradation)
 *   1 = mild latency injection   (50–200 µs per op)
 *   2 = severe latency injection (500–2000 µs per op)
 *   3 = backpressure at 500 ops/s
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "wave4_fixtures.h"
#include "storage/base_entity.h"
#include "index/vector_index.h"
#include "index/graph_index.h"

namespace {

using namespace themis::bench;
using namespace themis::bench::wave4;

// ============================================================================
// W4B-01 — Storage writes under latency injection
// ============================================================================

/**
 * @brief Measures kv-write latency distribution (p50/p95/p99) under three
 *        degradation profiles: baseline, mild, and severe latency injection.
 *
 * Compares against W4A-01 baseline to quantify degradation impact.
 *
 * state.range(0) = degradation mode (0/1/2/3; see file header).
 */
BENCHMARK_DEFINE_F(DegradedStorageFixture, W4B_01_WritesUnderLatencyInjection)(
        benchmark::State& state) {
    const int mode = static_cast<int>(state.range(0));

    WarmupProtocol::run(kDefaultWarmupIterations, [&] {
        applyDegradation();
        db_->put(rng_.genKey(16), rng_.genKey(64));
    });

    VarianceTracker vt;
    int64_t total_ops = 0;
    for (auto _ : state) {
        state.PauseTiming();
        auto k = rng_.genKey(16);
        auto v = rng_.genKey(64);
        state.ResumeTiming();

        applyDegradation();
        auto t0 = std::chrono::steady_clock::now();
        db_->put(k, v);
        vt.record(std::chrono::steady_clock::now() - t0);
        ++total_ops;
    }

    state.SetItemsProcessed(total_ops);
    vt.publishCounters(state);
    state.counters["degradation_mode"] = static_cast<double>(mode);
    state.counters["gate_ref"]         = 1.0;  // references W4A-01
}
BENCHMARK_REGISTER_F(DegradedStorageFixture, W4B_01_WritesUnderLatencyInjection)
    ->Arg(0)   // baseline
    ->Arg(1)   // mild latency
    ->Arg(2)   // severe latency
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ============================================================================
// W4B-02 — Storage reads under backpressure
// ============================================================================

/**
 * @brief Measures read throughput and p99 latency when a token-bucket
 *        backpressure limiter caps the operation rate.
 *
 * state.range(0) = degradation mode (0 = baseline, 3 = 500 ops/s backpressure).
 */
BENCHMARK_DEFINE_F(DegradedStorageFixture, W4B_02_ReadsUnderBackpressure)(
        benchmark::State& state) {
    const int mode = static_cast<int>(state.range(0));

    // Pre-populate.
    constexpr int kKeys = 1000;
    for (int i = 0; i < kKeys; ++i) {
        db_->put("bp_key_" + std::to_string(i), rng_.genKey(64));
    }

    WarmupProtocol::run(kDefaultWarmupIterations, [&] {
        applyDegradation();
        benchmark::DoNotOptimize(
            db_->get("bp_key_" + std::to_string(rng_.genInt(0, kKeys - 1))));
    });

    VarianceTracker vt;
    int64_t total_ops = 0;
    for (auto _ : state) {
        state.PauseTiming();
        const std::string key =
            "bp_key_" + std::to_string(rng_.genInt(0, kKeys - 1));
        state.ResumeTiming();

        applyDegradation();
        auto t0 = std::chrono::steady_clock::now();
        benchmark::DoNotOptimize(db_->get(key));
        vt.record(std::chrono::steady_clock::now() - t0);
        ++total_ops;
    }

    state.SetItemsProcessed(total_ops);
    vt.publishCounters(state);
    state.counters["degradation_mode"] = static_cast<double>(mode);
    state.counters["gate_ref"]         = 2.0;  // references W4A-02
}
BENCHMARK_REGISTER_F(DegradedStorageFixture, W4B_02_ReadsUnderBackpressure)
    ->Arg(0)   // baseline
    ->Arg(3)   // 500 ops/s backpressure
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ============================================================================
// W4B-03 — Vector search under CPU contention
// ============================================================================

/**
 * @brief Measures k-NN search latency (p50/p95/p99) when background
 *        CPU-intensive threads are running simultaneously.
 *
 * The contention is simulated by spinning @p kContenderThreads threads
 * doing busy-work during the search iterations.
 *
 * state.range(0) = number of contender threads (0 = no contention, 2 = mild, 4 = heavy).
 */
static void BM_W4B_03_VectorSearchUnderContention(benchmark::State& state) {
    constexpr std::size_t kDim     = 128;
    constexpr std::size_t kIndexSz = 5000;
    constexpr int         kK       = 10;
    const int contenders = static_cast<int>(state.range(0));

    TempDir tmp;
    themis::VectorIndexConfig cfg;
    cfg.dimension = kDim;
    cfg.db_path   = tmp.str();
    auto idx = std::make_shared<themis::VectorIndexManager>(cfg);

    RandomGenerator rng(kW4CanonicalSeed);
    for (std::size_t i = 0; i < kIndexSz; ++i) {
        idx->insert("cv_" + std::to_string(i), rng.genVec(kDim));
    }

    // Warmup before starting contenders.
    WarmupProtocol::run(kDefaultWarmupIterations, [&] {
        benchmark::DoNotOptimize(idx->search(rng.genVec(kDim), kK));
    });

    // Start background contenders.
    std::atomic<bool> stop_contenders{false};
    std::vector<std::thread> contender_threads;
    contender_threads.reserve(static_cast<std::size_t>(contenders));
    for (int t = 0; t < contenders; ++t) {
        contender_threads.emplace_back([&stop_contenders] {
            volatile double acc = 0.0;
            while (!stop_contenders.load(std::memory_order_relaxed)) {
                for (int i = 0; i < 10000; ++i) {
                  acc += static_cast<double>(i);
                }
            }
            benchmark::DoNotOptimize(acc);
        });
    }

    VarianceTracker vt;
    int64_t total_queries = 0;
    for (auto _ : state) {
        state.PauseTiming();
        auto query = rng.genVec(kDim);
        state.ResumeTiming();

        auto t0 = std::chrono::steady_clock::now();
        benchmark::DoNotOptimize(idx->search(query, kK));
        vt.record(std::chrono::steady_clock::now() - t0);
        ++total_queries;
    }

    stop_contenders.store(true, std::memory_order_relaxed);
    for (auto& th : contender_threads) {
      th.join();
    }

    state.SetItemsProcessed(total_queries);
    vt.publishCounters(state);
    state.counters["contenders"] = static_cast<double>(contenders);
    state.counters["gate_ref"]   = 4.0;  // references W4A-04
}
BENCHMARK(BM_W4B_03_VectorSearchUnderContention)
    ->Arg(0)   // no contention
    ->Arg(2)   // mild contention
    ->Arg(4)   // heavy contention
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ============================================================================
// W4B-04 — Write throughput under sustained backpressure
// ============================================================================

/**
 * @brief Measures write throughput when the operation rate is capped by
 *        backpressure at configurable limits.
 *
 * Emits throughput delta vs uncapped baseline to quantify backpressure cost.
 *
 * state.range(0) = target ops/s limit (0 = uncapped, 200 = heavy pressure,
 *                 1000 = moderate pressure).
 */
static void BM_W4B_04_WriteThroughputUnderBackpressure(benchmark::State& state) {
    const int64_t ops_per_sec_limit = state.range(0);

    TempDir tmp;
    themis::RocksDBWrapper::Config cfg;
    cfg.db_path = tmp.str();
    cfg.create_if_missing = true;
    auto db = std::make_shared<themis::RocksDBWrapper>(cfg);
    db->open();

    RandomGenerator rng(kW4CanonicalSeed);

    std::unique_ptr<BackpressureSimulator> bp;
    if (ops_per_sec_limit > 0) {
        bp = std::make_unique<BackpressureSimulator>(
            static_cast<double>(ops_per_sec_limit));
    }

    WarmupProtocol::run(kDefaultWarmupIterations, [&] {
        db->put(rng.genKey(16), rng.genKey(64));
    });

    int64_t total_ops = 0;
    for (auto _ : state) {
        if (bp) {
          bp->acquire();
        }

        db->put(rng.genKey(16), rng.genKey(64));
        ++total_ops;
    }

    state.SetItemsProcessed(total_ops);
    state.counters["ops_per_sec_limit"] = static_cast<double>(ops_per_sec_limit);
    state.counters["gate_ref"]          = 1.0;  // references W4A-01
}
BENCHMARK(BM_W4B_04_WriteThroughputUnderBackpressure)
    ->Arg(0)      // uncapped
    ->Arg(1000)   // moderate backpressure
    ->Arg(200)    // heavy backpressure
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

// ============================================================================
// W4B-05 — Graph traversal under partial node unavailability
// ============================================================================

/**
 * @brief Measures BFS depth-2 traversal latency when a fraction of edges
 *        point to "unavailable" nodes (simulating partial failure).
 *
 * Unavailability is simulated by attempting to look up a missing node ID;
 * the graph implementation returns an empty neighbour list for missing nodes.
 *
 * state.range(0) = failure fraction in percent (0 = no failure, 20 = 20% missing).
 */
static void BM_W4B_05_GraphTraversalPartialFailure(benchmark::State& state) {
    constexpr std::size_t kNodes = 3000;
    const int64_t failure_pct = state.range(0);

    TempDir tmp;
    themis::RocksDBWrapper::Config dbcfg;
    dbcfg.db_path = tmp.str();
    auto db = std::make_unique<themis::RocksDBWrapper>(dbcfg);
    db->open();
    themis::GraphIndexManager graph(*db);

    RandomGenerator rng(kW4CanonicalSeed);

    // Build graph — add edges between available nodes only.
    // Simulate partial failure: skip (failure_pct)% of node IDs when adding edges.
    for (std::size_t i = 0; i < kNodes; ++i) {
        const bool available = rng.genInt(0, 99) >= failure_pct;
        if (!available) {
          continue;
        }
        for (int e = 0; e < 3; ++e) {
            auto j = static_cast<std::size_t>(rng.genInt(0, static_cast<int64_t>(kNodes) - 1));
            if (j == i) {
              continue;
            }
            std::string edge_id =
                "e_" + std::to_string(i) + "_" + std::to_string(j);
            themis::BaseEntity edge(edge_id);
            edge.setField("_from", "node_" + std::to_string(i));
            edge.setField("_to",   "node_" + std::to_string(j));
            edge.setField("_graph", "pf_graph");
            graph.addEdge(edge);
        }
    }

    WarmupProtocol::run(kDefaultWarmupIterations, [&] {
        const std::string src = "node_" + std::to_string(rng.genInt(0, static_cast<int64_t>(kNodes) - 1));
        benchmark::DoNotOptimize(graph.outNeighbors(src));
    });

    VarianceTracker vt;
    int64_t total_queries = 0;
    for (auto _ : state) {
        state.PauseTiming();
        const std::string src =
            "node_" + std::to_string(rng.genInt(0, static_cast<int64_t>(kNodes) - 1));
        state.ResumeTiming();

        auto t0 = std::chrono::steady_clock::now();
        auto [st1, neighbours] = graph.outNeighbors(src);
        benchmark::DoNotOptimize(neighbours);
        for (const auto& nb_id : neighbours) {
            benchmark::DoNotOptimize(graph.outNeighbors(nb_id));
        }
        vt.record(std::chrono::steady_clock::now() - t0);
        ++total_queries;
    }

    state.SetItemsProcessed(total_queries);
    vt.publishCounters(state);
    state.counters["failure_pct"] = static_cast<double>(failure_pct);
    state.counters["gate_ref"]    = 5.0;  // references W4A-05
}
BENCHMARK(BM_W4B_05_GraphTraversalPartialFailure)
    ->Arg(0)    // no failure
    ->Arg(10)   // 10% node unavailability
    ->Arg(20)   // 20% node unavailability
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ============================================================================
// W4B-06 — Recovery throughput after partial data loss
// ============================================================================

/**
 * @brief Simulates a partial-loss scenario by deleting a fraction of stored
 *        keys and measuring the re-ingestion throughput to restore full state.
 *
 * state.range(0) = fraction of keys deleted before recovery (in percent).
 */
BENCHMARK_DEFINE_F(StorageBenchFixture, W4B_06_RecoveryThroughput)(
        benchmark::State& state) {
    constexpr int kTotalKeys = 2000;
    const int64_t delete_pct = state.range(0);

    // Pre-populate.
    std::vector<std::string> keys;
    keys.reserve(kTotalKeys);
    for (int i = 0; i < kTotalKeys; ++i) {
        const std::string k = "recover_" + std::to_string(i);
        db_->put(k, rng_.genKey(64));
        keys.push_back(k);
    }

    // Simulate partial data loss: delete delete_pct% of keys.
    const int64_t delete_count = (kTotalKeys * delete_pct) / 100;
    for (int64_t i = 0; i < delete_count; ++i) {
        db_->del(keys[static_cast<std::size_t>(i)]);
    }

    WarmupProtocol::run(kDefaultWarmupIterations, [&] {
        db_->put(rng_.genKey(16), rng_.genKey(64));
    });

    int64_t total_ops = 0;
    for (auto _ : state) {
        state.PauseTiming();
        // Identify keys that need recovery (deleted prefix).
        const std::size_t batch = static_cast<std::size_t>(delete_count);
        std::vector<std::pair<std::string, std::string>> recovery_batch;
        recovery_batch.reserve(batch);
        for (std::size_t i = 0; i < batch; ++i) {
            recovery_batch.emplace_back(keys[i], rng_.genKey(64));
        }
        state.ResumeTiming();

        for (auto& [k, v] : recovery_batch) {
            db_->put(k, v);
        }
        total_ops += static_cast<int64_t>(batch);
    }

    state.SetItemsProcessed(total_ops);
    state.counters["delete_pct"] = static_cast<double>(delete_pct);
    state.counters["gate_ref"]   = 1.0;  // references W4A-01 (write path)
}
BENCHMARK_REGISTER_F(StorageBenchFixture, W4B_06_RecoveryThroughput)
    ->Arg(10)   // 10% data loss
    ->Arg(25)   // 25% data loss
    ->Arg(50)   // 50% data loss
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

}  // namespace

BENCHMARK_MAIN();
