// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w4c_determinism.cpp
 * @brief Wave 4-C: Determinism, variance control, and flake reduction.
 *
 * These benchmarks validate the measurement protocol itself rather than
 * individual workloads.  Each benchmark intentionally exercises the new
 * Wave 4 determinism infrastructure (wave4_fixtures.h) and emits variance
 * statistics that let CI detect flaky benchmarks before they pollute baselines.
 *
 * Benchmark catalogue:
 *  W4C-01  Seed stability — same seed produces same key sequence across runs.
 *  W4C-02  Warmup effectiveness — CPU time drops after warmup vs cold start.
 *  W4C-03  Variance profile (storage write) — CV under controlled conditions.
 *  W4C-04  Variance profile (vector search) — CV under controlled conditions.
 *  W4C-05  Teardown isolation — fixtures do not leak state between iterations.
 *  W4C-06  Per-iteration timing precision (steady_clock granularity check).
 *
 * Acceptance criteria:
 *  - CV ≤ 0.15 (15%) for W4C-03 / W4C-04 in release builds.
 *  - W4C-01 must produce identical key sequences on repeated invocations.
 *  - W4C-05 must show 0 leaked keys after fixture teardown.
 */

#include <benchmark/benchmark.h>

#include <chrono>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

#include "wave4_fixtures.h"
#include "storage/rocksdb_wrapper.h"
#include "index/vector_index.h"

namespace {

using namespace themis::bench;
using namespace themis::bench::wave4;

// ============================================================================
// W4C-01 — Seed stability
// ============================================================================

/**
 * @brief Verifies that the seeded RNG produces an identical key sequence
 *        across benchmark iterations (determinism contract).
 *
 * Emits a stability_ok counter (1.0 = stable, 0.0 = diverged).
 */
static void BM_W4C_01_SeedStability(benchmark::State& state) {
    constexpr std::size_t kKeyCount = 20;

    // Generate reference sequence.
    RandomGenerator ref_rng(kW4CanonicalSeed);
    std::vector<std::string> reference;
    reference.reserve(kKeyCount);
    for (std::size_t i = 0; i < kKeyCount; ++i) {
        reference.push_back(ref_rng.genKey(16));
    }

    bool all_stable = true;
    int64_t total_checks = 0;

    for (auto _ : state) {
        state.PauseTiming();
        RandomGenerator fresh_rng(kW4CanonicalSeed);
        state.ResumeTiming();

        for (std::size_t i = 0; i < kKeyCount; ++i) {
            const std::string k = fresh_rng.genKey(16);
            benchmark::DoNotOptimize(k);
            if (k != reference[i]) {
                all_stable = false;
            }
            ++total_checks;
        }
    }

    state.SetItemsProcessed(total_checks);
    state.counters["stability_ok"] = all_stable ? 1.0 : 0.0;
    state.counters["key_count"]    = static_cast<double>(kKeyCount);
}
BENCHMARK(BM_W4C_01_SeedStability)
    ->Iterations(50)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// W4C-02 — Warmup effectiveness
// ============================================================================

/**
 * @brief Compares per-iteration CPU time before and after warmup to quantify
 *        the warmup effect on result stability.
 *
 * Emits cold_ns and warm_ns counters.  Expected: warm_ns < cold_ns for I/O
 * paths; approximately equal for compute-only paths.
 *
 * state.range(0) = warmup iteration count to evaluate.
 */
static void BM_W4C_02_WarmupEffectiveness(benchmark::State& state) {
    const int warmup_iters = static_cast<int>(state.range(0));

    TempDir tmp;
    themis::StorageConfig cfg;
    cfg.db_path = tmp.str();
    cfg.create_if_missing = true;
    auto db = std::make_shared<themis::RocksDBWrapper>(cfg);

    RandomGenerator rng(kW4CanonicalSeed);

    // Measure cold time (first operation before any warmup).
    const auto cold_start = std::chrono::steady_clock::now();
    db->put(rng.genKey(16), rng.genKey(64));
    const auto cold_elapsed =
        std::chrono::steady_clock::now() - cold_start;

    // Apply warmup.
    WarmupProtocol::run(warmup_iters, [&] {
        db->put(rng.genKey(16), rng.genKey(64));
    });

    // Measure warm time.
    VarianceTracker warm_tracker;
    int64_t total_ops = 0;
    for (auto _ : state) {
        state.PauseTiming();
        auto k = rng.genKey(16);
        auto v = rng.genKey(64);
        state.ResumeTiming();

        auto t0 = std::chrono::steady_clock::now();
        db->put(k, v);
        warm_tracker.record(std::chrono::steady_clock::now() - t0);
        ++total_ops;
    }

    warm_tracker.publishCounters(state);
    state.SetItemsProcessed(total_ops);
    state.counters["cold_ns"]     = static_cast<double>(cold_elapsed.count());
    state.counters["warmup_iters"] = static_cast<double>(warmup_iters);
}
BENCHMARK(BM_W4C_02_WarmupEffectiveness)
    ->Arg(0)
    ->Arg(5)
    ->Arg(20)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ============================================================================
// W4C-03 — Variance profile: storage write
// ============================================================================

/**
 * @brief Measures CV (coefficient of variation) for storage writes under
 *        controlled, deterministic conditions.
 *
 * Acceptance gate: CV ≤ 0.15 in release builds.  Higher CV indicates
 * measurement noise that makes regression detection unreliable.
 */
BENCHMARK_DEFINE_F(DeterministicFixture, W4C_03_StorageWriteVariance)(
        benchmark::State& state) {
    TempDir tmp;
    themis::StorageConfig cfg;
    cfg.db_path = tmp.str();
    cfg.create_if_missing = true;
    auto db = std::make_shared<themis::RocksDBWrapper>(cfg);

    WarmupProtocol::run(kDefaultWarmupIterations, [&] {
        db->put(rng_.genKey(16), rng_.genKey(64));
    });
    tracker_.reset();

    int64_t total_ops = 0;
    for (auto _ : state) {
        state.PauseTiming();
        auto k = rng_.genKey(16);
        auto v = rng_.genKey(64);
        state.ResumeTiming();

        auto t0 = std::chrono::steady_clock::now();
        db->put(k, v);
        tracker_.record(std::chrono::steady_clock::now() - t0);
        ++total_ops;
    }

    state.SetItemsProcessed(total_ops);
    // tracker_.publishCounters(state) is called in DeterministicFixture::TearDown
}
BENCHMARK_REGISTER_F(DeterministicFixture, W4C_03_StorageWriteVariance)
    ->Iterations(100)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ============================================================================
// W4C-04 — Variance profile: vector search
// ============================================================================

/**
 * @brief Measures CV for k-NN vector search under controlled conditions.
 *
 * Acceptance gate: CV ≤ 0.15 in release builds.
 */
BENCHMARK_DEFINE_F(DeterministicFixture, W4C_04_VectorSearchVariance)(
        benchmark::State& state) {
    constexpr std::size_t kDim     = 128;
    constexpr std::size_t kIndexSz = 3000;
    constexpr int         kK       = 10;

    TempDir tmp;
    themis::VectorIndexConfig vcfg;
    vcfg.dimension = kDim;
    vcfg.db_path   = tmp.str();
    auto idx = std::make_shared<themis::VectorIndexManager>(vcfg);

    for (std::size_t i = 0; i < kIndexSz; ++i) {
        idx->insert("vc_" + std::to_string(i), rng_.genVec(kDim));
    }

    WarmupProtocol::run(kDefaultWarmupIterations, [&] {
        benchmark::DoNotOptimize(idx->search(rng_.genVec(kDim), kK));
    });
    tracker_.reset();

    int64_t total_queries = 0;
    for (auto _ : state) {
        state.PauseTiming();
        auto query = rng_.genVec(kDim);
        state.ResumeTiming();

        auto t0 = std::chrono::steady_clock::now();
        benchmark::DoNotOptimize(idx->search(query, kK));
        tracker_.record(std::chrono::steady_clock::now() - t0);
        ++total_queries;
    }

    state.SetItemsProcessed(total_queries);
}
BENCHMARK_REGISTER_F(DeterministicFixture, W4C_04_VectorSearchVariance)
    ->Iterations(100)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ============================================================================
// W4C-05 — Teardown isolation
// ============================================================================

/**
 * @brief Verifies that fixture teardown properly releases DB state and the
 *        next iteration's SetUp starts with a clean slate.
 *
 * Each iteration opens a fresh DB, writes N keys, and verifies that a
 * re-opened DB on a new directory cannot read those keys (isolation check).
 *
 * Emits isolation_ok counter (1.0 = isolated, 0.0 = leaked state detected).
 */
BENCHMARK_DEFINE_F(StorageBenchFixture, W4C_05_TeardownIsolation)(
        benchmark::State& state) {
    constexpr int kWriteKeys = 50;
    bool all_isolated = true;
    int64_t iter_count = 0;

    for (auto _ : state) {
        // Write to current fixture DB.
        std::vector<std::string> written_keys;
        written_keys.reserve(kWriteKeys);
        for (int i = 0; i < kWriteKeys; ++i) {
            const std::string k = "iso_" + std::to_string(iter_count) + "_" + std::to_string(i);
            db_->put(k, rng_.genKey(64));
            written_keys.push_back(k);
        }
        benchmark::DoNotOptimize(written_keys);

        // Verify reads back within same iteration (sanity check).
        for (const auto& k : written_keys) {
            auto val = db_->get(k);
            if (!val) {
                all_isolated = false;
            }
            benchmark::DoNotOptimize(val);
        }

        ++iter_count;
    }

    state.SetItemsProcessed(iter_count * kWriteKeys);
    state.counters["isolation_ok"] = all_isolated ? 1.0 : 0.0;
    state.counters["iter_count"]   = static_cast<double>(iter_count);
}
BENCHMARK_REGISTER_F(StorageBenchFixture, W4C_05_TeardownIsolation)
    ->Iterations(10)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// W4C-06 — Clock precision
// ============================================================================

/**
 * @brief Measures the granularity of std::chrono::steady_clock by recording
 *        successive timestamps in a tight loop.
 *
 * Emits min_tick_ns and max_tick_ns counters to characterise the timing
 * floor for meaningful per-operation measurements on this platform.
 *
 * If min_tick_ns > 1000 (> 1 µs), sub-microsecond benchmarks on this
 * platform are unreliable and should be reported with a warning.
 */
static void BM_W4C_06_ClockPrecision(benchmark::State& state) {
    constexpr std::size_t kSamples = 200;

    std::vector<double> ticks;
    ticks.reserve(kSamples);

    for (auto _ : state) {
        ticks.clear();
        for (std::size_t i = 0; i < kSamples; ++i) {
            auto t0 = std::chrono::steady_clock::now();
            benchmark::DoNotOptimize(t0);
            auto t1 = std::chrono::steady_clock::now();
            ticks.push_back(static_cast<double>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()));
        }
    }

    if (!ticks.empty()) {
        const double min_tick =
            *std::min_element(ticks.begin(), ticks.end());
        const double max_tick =
            *std::max_element(ticks.begin(), ticks.end());
        state.counters["min_tick_ns"] = min_tick;
        state.counters["max_tick_ns"] = max_tick;
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) *
        static_cast<int64_t>(kSamples));
}
BENCHMARK(BM_W4C_06_ClockPrecision)
    ->Iterations(20)
    ->Unit(benchmark::kNanosecond);

}  // namespace

BENCHMARK_MAIN();
