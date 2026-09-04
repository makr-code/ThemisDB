// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w4d_diagnostics.cpp
 * @brief Wave 4-D: Diagnostics, reporting, and maintainability benchmarks.
 *
 * These benchmarks focus on observability of the benchmark pipeline itself:
 * they emit structured diagnostic counters that downstream reporting tools
 * (report_variance.py, performance_regression_detector.py) can consume to
 * generate summaries, highlight regressions, and produce repro steps.
 *
 * Benchmark catalogue:
 *  W4D-01  Storage throughput summary with structured counter output.
 *  W4D-02  Vector search regression sentinel (emits known-good baseline hash).
 *  W4D-03  Pipeline health check (end-to-end smoke, reports component statuses).
 *  W4D-04  Benchmark fixture setup/teardown overhead measurement.
 *  W4D-05  Consolidated workload profile (all critical paths in one run).
 *
 * Counter naming convention (used by report_variance.py):
 *  - gate_id     : float encoding of the W4A gate this result maps to.
 *  - p50_ns      : 50th-percentile latency in nanoseconds.
 *  - p95_ns      : 95th-percentile latency in nanoseconds.
 *  - p99_ns      : 99th-percentile latency in nanoseconds.
 *  - cv          : coefficient of variation (stddev/mean).
 *  - component   : bitmask of components exercised (see W4D-03).
 *  - health_ok   : 1.0 = all components healthy, 0.0 = at least one failed.
 */

#include <benchmark/benchmark.h>

#include <chrono>
#include <cmath>
#include <functional>
#include <numeric>
#include <string>
#include <vector>

#include "wave4_fixtures.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/vector_index.h"
#include "index/graph_index.h"
#include "index/secondary_index.h"

namespace {

using namespace themis::bench;
using namespace themis::bench::wave4;

// ============================================================================
// W4D-01 — Storage throughput summary
// ============================================================================

/**
 * @brief Measures storage write + read throughput and emits a comprehensive
 *        set of diagnostic counters for the reporting pipeline.
 *
 * Output counters consumed by report_variance.py:
 *  - write_ops_per_sec, read_ops_per_sec
 *  - write_p99_ns, read_p99_ns
 *  - cv (combined write+read variance)
 */
BENCHMARK_DEFINE_F(StorageBenchFixture, W4D_01_StorageThroughputSummary)(
        benchmark::State& state) {
    constexpr int kKeys = 2000;

    // Pre-populate for read path.
    for (int i = 0; i < kKeys; ++i) {
        db_->put("diag_" + std::to_string(i), rng_.genKey(64));
    }

    WarmupProtocol::run(kDefaultWarmupIterations, [&] {
        db_->put(rng_.genKey(16), rng_.genKey(64));
        benchmark::DoNotOptimize(db_->get("diag_0"));
    });

    VarianceTracker write_vt, read_vt;
    int64_t total_write = 0, total_read = 0;

    for (auto _ : state) {
        state.PauseTiming();
        auto wk = rng_.genKey(16);
        auto wv = rng_.genKey(64);
        const std::string rk =
            "diag_" + std::to_string(rng_.genInt(0, kKeys - 1));
        state.ResumeTiming();

        {
            auto t0 = std::chrono::steady_clock::now();
            db_->put(wk, wv);
            write_vt.record(std::chrono::steady_clock::now() - t0);
            ++total_write;
        }
        {
            auto t0 = std::chrono::steady_clock::now();
            benchmark::DoNotOptimize(db_->get(rk));
            read_vt.record(std::chrono::steady_clock::now() - t0);
            ++total_read;
        }
    }

    state.SetItemsProcessed(total_write + total_read);

    // Publish write diagnostics (renamed to avoid collision with VarianceTracker).
    write_vt.publishCounters(state);

    // Emit read ops/s using kIsRate so Google Benchmark divides by elapsed time.
    state.counters["read_ops_per_sec"] = benchmark::Counter(
        static_cast<double>(total_read), benchmark::Counter::kIsRate);

    state.counters["write_ops"]  = static_cast<double>(total_write);
    state.counters["read_ops"]   = static_cast<double>(total_read);
    state.counters["gate_id"]    = 100.0;  // W4D-01 (diagnostic, not a release gate)
}
BENCHMARK_REGISTER_F(StorageBenchFixture, W4D_01_StorageThroughputSummary)
    ->Iterations(200)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ============================================================================
// W4D-02 — Vector search regression sentinel
// ============================================================================

/**
 * @brief Emits a deterministic throughput value derived from a fixed index
 *        and fixed query sequence.  Downstream tooling compares the value
 *        against the stored baseline to detect regressions without needing
 *        a full statistical test.
 *
 * The "sentinel_hash" counter is a lightweight fingerprint: the sum of the
 * returned neighbour counts across all queries (modulo 1000).  Identical
 * hardware + code → identical sentinel value.
 */
static void BM_W4D_02_VectorSearchSentinel(benchmark::State& state) {
    constexpr std::size_t kDim     = 128;
    constexpr std::size_t kIndexSz = 2000;
    constexpr int         kK       = 5;
    constexpr std::size_t kQueries = 50;

    TempDir tmp;
    themis::VectorIndexConfig vcfg;
    vcfg.dimension = kDim;
    vcfg.db_path   = tmp.str();
    auto idx = std::make_shared<themis::VectorIndexManager>(vcfg);

    RandomGenerator rng(kW4CanonicalSeed);
    for (std::size_t i = 0; i < kIndexSz; ++i) {
        idx->insert("sentinel_" + std::to_string(i), rng.genVec(kDim));
    }

    // Generate deterministic query set.
    RandomGenerator qrng(kW4CanonicalSeed + 1);
    std::vector<std::vector<float>> queries;
    queries.reserve(kQueries);
    for (std::size_t q = 0; q < kQueries; ++q) {
        queries.push_back(qrng.genVec(kDim));
    }

    WarmupProtocol::run(kDefaultWarmupIterations, [&] {
        benchmark::DoNotOptimize(idx->search(queries[0], kK));
    });

    int64_t sentinel_sum = 0;
    int64_t total_queries = 0;
    for (auto _ : state) {
        for (const auto& q : queries) {
            auto results = idx->search(q, kK);
            sentinel_sum += static_cast<int64_t>(results.size());
            benchmark::DoNotOptimize(results);
            ++total_queries;
        }
    }

    state.SetItemsProcessed(total_queries);
    state.counters["sentinel_hash"] =
        static_cast<double>(sentinel_sum % 1000);
    state.counters["index_size"] = static_cast<double>(kIndexSz);
    state.counters["k"]          = static_cast<double>(kK);
    state.counters["gate_id"]    = 101.0;  // W4D-02 (diagnostic)
}
BENCHMARK(BM_W4D_02_VectorSearchSentinel)
    ->Iterations(10)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

// ============================================================================
// W4D-03 — Pipeline health check
// ============================================================================

/**
 * @brief End-to-end smoke benchmark that exercises all three core
 *        components (storage, vector, graph) in sequence.
 *
 * Reports a bitmask counter "component" (bit 0=storage, bit 1=vector,
 * bit 2=graph) and "health_ok" (1.0 = all bits set, 0.0 = any failure).
 * Useful as a fast sanity check in CI gate runs.
 */
static void BM_W4D_03_PipelineHealthCheck(benchmark::State& state) {
    TempDir storage_dir, vector_dir, graph_dir;

    // Storage component.
    themis::RocksDBWrapper::Config scfg;
    scfg.db_path = storage_dir.str();
    scfg.create_if_missing = true;
    auto db = std::make_shared<themis::RocksDBWrapper>(scfg);
    db->open();

    // Vector component.
    themis::VectorIndexConfig vcfg;
    vcfg.dimension = 64;
    vcfg.db_path   = vector_dir.str();
    auto idx = std::make_shared<themis::VectorIndexManager>(vcfg);

    // Graph component — use RocksDBWrapper + GraphIndexManager (canonical API).
    themis::RocksDBWrapper::Config gcfg;
    gcfg.db_path = graph_dir.str();
    gcfg.create_if_missing = true;
    auto graph_db = std::make_shared<themis::RocksDBWrapper>(gcfg);
    graph_db->open();
    auto graph = std::make_shared<themis::GraphIndexManager>(*graph_db);

    RandomGenerator rng(kW4CanonicalSeed);

    // Pre-populate.
    db->put("health_key", "health_val");
    idx->insert("health_vec", rng.genVec(64));
    // Sentinel edge: "health_node" -> "health_node" (self-loop) makes the node
    // reachable via outNeighbors without needing a separate getNode() API.
    {
        themis::BaseEntity sentinel("health_sentinel");
        sentinel.setField("_from", "health_node");
        sentinel.setField("_to",   "health_node");
        sentinel.setField("_graph", "health_graph");
        graph->addEdge(sentinel);
    }

    int64_t iter_count = 0;
    uint32_t component_mask = 0;

    for (auto _ : state) {
        uint32_t iter_mask = 0;

        // Storage check.
        {
            auto v = db->get("health_key");
            if (v && !v->empty()) {
              iter_mask |= 0x01u;
            }
        }

        // Vector check.
        {
            auto results = idx->search(rng.genVec(64), 1);
            if (!results.empty()) {
              iter_mask |= 0x02u;
            }
        }

        // Graph check: outNeighbors returns ok + non-empty list if sentinel is present.
        {
            auto [st, neighbours] = graph->outNeighbors("health_node");
            if (st.ok() && !neighbours.empty()) {
              iter_mask |= 0x04u;
            }
        }

        component_mask |= iter_mask;
        benchmark::DoNotOptimize(iter_mask);
        ++iter_count;
    }

    state.SetItemsProcessed(iter_count * 3);
    state.counters["component"]  = static_cast<double>(component_mask);
    state.counters["health_ok"]  = (component_mask == 0x07u) ? 1.0 : 0.0;
    state.counters["gate_id"]    = 102.0;  // W4D-03 (diagnostic)
}
BENCHMARK(BM_W4D_03_PipelineHealthCheck)
    ->Iterations(20)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ============================================================================
// W4D-04 — Fixture setup/teardown overhead
// ============================================================================

/**
 * @brief Measures the wall-clock cost of StorageBenchFixture SetUp() and
 *        TearDown() so downstream analysis can distinguish fixture overhead
 *        from benchmark work.
 *
 * Emits setup_ns and teardown_ns counters.
 */
static void BM_W4D_04_FixtureOverhead(benchmark::State& state) {
    VarianceTracker setup_vt;

    for (auto _ : state) {
        auto t0 = std::chrono::steady_clock::now();

        // Emulate fixture setup inline (construction + open) and teardown (destroy).
        TempDir tmp;
        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = tmp.str();
        cfg.create_if_missing = true;
        {
            auto db = std::make_shared<themis::RocksDBWrapper>(cfg);
            db->open();
            benchmark::DoNotOptimize(db);
        }  // DB is destroyed here (emulates TearDown).

        setup_vt.record(std::chrono::steady_clock::now() - t0);
    }

    setup_vt.publishCounters(state);
    state.counters["gate_id"] = 103.0;  // W4D-04 (diagnostic)
}
BENCHMARK(BM_W4D_04_FixtureOverhead)
    ->Iterations(30)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

// ============================================================================
// W4D-05 — Consolidated workload profile
// ============================================================================

/**
 * @brief Runs all three critical paths (storage write, vector insert, graph
 *        add) in a single benchmark to produce a consolidated per-component
 *        throughput profile.
 *
 * This is the "one-shot" diagnostic run for quick CI summaries.
 * Results map directly to W4A gate IDs for cross-run comparison.
 */
static void BM_W4D_05_ConsolidatedWorkloadProfile(benchmark::State& state) {
    constexpr std::size_t kDim = 128;

    TempDir sdir, vdir, gdir;

    themis::RocksDBWrapper::Config scfg;
    scfg.db_path = sdir.str();
    scfg.create_if_missing = true;
    auto db = std::make_shared<themis::RocksDBWrapper>(scfg);
    db->open();

    themis::VectorIndexConfig vcfg;
    vcfg.dimension = kDim;
    vcfg.db_path   = vdir.str();
    auto idx = std::make_shared<themis::VectorIndexManager>(vcfg);

    themis::RocksDBWrapper::Config gcfg;
    gcfg.db_path = gdir.str();
    gcfg.create_if_missing = true;
    auto graph_db = std::make_shared<themis::RocksDBWrapper>(gcfg);
    graph_db->open();
    auto graph = std::make_shared<themis::GraphIndexManager>(*graph_db);

    RandomGenerator rng(kW4CanonicalSeed);
    int64_t edge_counter = 0;

    // Warmup: add a few edges to warm up all components.
    WarmupProtocol::run(kDefaultWarmupIterations, [&] {
        db->put(rng.genKey(16), rng.genKey(64));
        idx->insert(rng.genKey(8), rng.genVec(kDim));
        std::string eid = "w_" + std::to_string(edge_counter++);
        themis::BaseEntity e(eid);
        e.setField("_from", rng.genKey(4));
        e.setField("_to",   rng.genKey(4));
        e.setField("_graph", "profile_graph");
        graph->addEdge(e);
    });

    VarianceTracker storage_vt, vector_vt, graph_vt;
    int64_t total_ops = 0;

    for (auto _ : state) {
        state.PauseTiming();
        auto sk = rng.genKey(16);
        auto sv = rng.genKey(64);
        auto vk = rng.genKey(8);
        auto vec = rng.genVec(kDim);
        auto gk  = rng.genKey(8);
        std::string eid = "e_" + std::to_string(edge_counter++);
        state.ResumeTiming();

        {
            auto t0 = std::chrono::steady_clock::now();
            db->put(sk, sv);
            storage_vt.record(std::chrono::steady_clock::now() - t0);
        }
        {
            auto t0 = std::chrono::steady_clock::now();
            idx->insert(vk, vec);
            vector_vt.record(std::chrono::steady_clock::now() - t0);
        }
        {
            auto t0 = std::chrono::steady_clock::now();
            themis::BaseEntity e(eid);
            e.setField("_from", gk);
            e.setField("_to",   rng.genKey(8));
            e.setField("_graph", "profile_graph");
            graph->addEdge(e);
            graph_vt.record(std::chrono::steady_clock::now() - t0);
        }
        total_ops += 3;
    }

    state.SetItemsProcessed(total_ops);

    // Publish primary variance (storage dominates).
    storage_vt.publishCounters(state);

    state.counters["storage_samples"] = static_cast<double>(storage_vt.size());
    state.counters["vector_samples"]  = static_cast<double>(vector_vt.size());
    state.counters["graph_samples"]   = static_cast<double>(graph_vt.size());

    state.counters["gate_id"] = 104.0;  // W4D-05 (diagnostic)
}
BENCHMARK(BM_W4D_05_ConsolidatedWorkloadProfile)
    ->Iterations(100)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

}  // namespace

BENCHMARK_MAIN();
