/**
 * @file bench_retrieval_dedicated_gates.cpp
 * @brief Wave D — Retrieval Engine Dedicated Benchmark Gates (RT-BM-01..04).
 *
 * Benchmark gates for the ThemisDB retrieval engine shard routing and GPU
 * advisory path. These benchmarks are registered as Wave D acceptance gates.
 *
 * Gate IDs:
 *   RT-BM-01  Single-shard exact routing throughput (CPU)
 *   RT-BM-02  Multi-shard routing throughput (16 shards, 8 workers)
 *   RT-BM-03  GPU advisory path throughput vs CPU baseline
 *   RT-BM-04  High-cardinality routing (100 K unique keys)
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_RETRIEVAL_ENGINE.md
 * @see src/retrieval/ROADMAP.md — Wave D Contribution
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// In-process stubs
// ─────────────────────────────────────────────────────────────────────────────

static int routeKey(const std::string& key, int num_shards) {
    return static_cast<int>(std::hash<std::string>{}(key) % static_cast<size_t>(num_shards));
}

// ─────────────────────────────────────────────────────────────────────────────
// RT-BM-01: Single-shard exact routing throughput
// ─────────────────────────────────────────────────────────────────────────────
static void BM_RT_BM_01_SingleShardRouting(benchmark::State& state) {
    const int num_shards = 1;
    std::mt19937_64 rng(0xdeadbeef);
    std::uniform_int_distribution<int> key_dist(0, 99999);

    for (auto _ : state) {
        std::string key = "key:" + std::to_string(key_dist(rng));
        int shard = routeKey(key, num_shards);
        benchmark::DoNotOptimize(shard);
    }
    state.SetLabel("RT-BM-01: single-shard exact routing");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RT_BM_01_SingleShardRouting)->Name("RT-BM-01/SingleShardExactRouting");

// ─────────────────────────────────────────────────────────────────────────────
// RT-BM-02: Multi-shard routing throughput (16 shards)
// ─────────────────────────────────────────────────────────────────────────────
static void BM_RT_BM_02_MultiShardRouting(benchmark::State& state) {
    const int num_shards = 16;
    std::mt19937_64 rng(0xcafe1234);
    std::uniform_int_distribution<int> key_dist(0, 999999);

    for (auto _ : state) {
        std::string key = "ms:" + std::to_string(key_dist(rng));
        int shard = routeKey(key, num_shards);
        benchmark::DoNotOptimize(shard);
    }
    state.SetLabel("RT-BM-02: multi-shard routing (16 shards)");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RT_BM_02_MultiShardRouting)->Name("RT-BM-02/MultiShardRouting16")->Threads(8);

// ─────────────────────────────────────────────────────────────────────────────
// RT-BM-03: GPU advisory path throughput vs CPU baseline
// ─────────────────────────────────────────────────────────────────────────────
static bool stubGpuQuery(const std::vector<float>& vec) {
    // Simulate advisory GPU path (in-process stub — no real GPU).
    float sum = 0.0f;
    for (float v : vec) { sum += v; }
    return sum > -1e30f; // always true
}

static bool stubCpuQuery(const std::vector<float>& vec) {
    float sum = 0.0f;
    for (float v : vec) { sum += v; }
    return sum > -1e30f;
}

static void BM_RT_BM_03_GpuAdvisoryThroughput(benchmark::State& state) {
    std::vector<float> vec(128, 1.0f);
    for (auto _ : state) {
        bool ok = stubGpuQuery(vec);
        benchmark::DoNotOptimize(ok);
    }
    state.SetLabel("RT-BM-03: GPU advisory path (in-process stub)");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RT_BM_03_GpuAdvisoryThroughput)->Name("RT-BM-03/GpuAdvisoryPathThroughput");

static void BM_RT_BM_03_CpuBaseline(benchmark::State& state) {
    std::vector<float> vec(128, 1.0f);
    for (auto _ : state) {
        bool ok = stubCpuQuery(vec);
        benchmark::DoNotOptimize(ok);
    }
    state.SetLabel("RT-BM-03: CPU baseline (for GPU break-even comparison)");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RT_BM_03_CpuBaseline)->Name("RT-BM-03/CpuBaselineThroughput");

// ─────────────────────────────────────────────────────────────────────────────
// RT-BM-04: High-cardinality routing (100 K unique keys)
// ─────────────────────────────────────────────────────────────────────────────
static void BM_RT_BM_04_HighCardinalityRouting(benchmark::State& state) {
    const int num_shards = 32;

    // Pre-generate 100 K keys.
    constexpr int kKeys = 100'000;
    std::vector<std::string> keys;
    keys.reserve(kKeys);
    for (int i = 0; i < kKeys; ++i) {
        keys.push_back("hc:key:" + std::to_string(i));
    }

    std::mt19937 rng(0xbadcafe);
    std::uniform_int_distribution<int> idx_dist(0, kKeys - 1);

    for (auto _ : state) {
        const std::string& key = keys[static_cast<size_t>(idx_dist(rng))];
        int shard = routeKey(key, num_shards);
        benchmark::DoNotOptimize(shard);
    }
    state.SetLabel("RT-BM-04: high-cardinality routing (100K keys, 32 shards)");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RT_BM_04_HighCardinalityRouting)->Name("RT-BM-04/HighCardinalityRouting");

BENCHMARK_MAIN();
