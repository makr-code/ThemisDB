// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w2c_recovery.cpp
 * @brief Wave 2 / B2-C: Recovery and warm-vs-cold path performance benchmarks.
 *
 * Measures the performance difference between warm (cache-hot) and cold
 * (cache-cold, freshly opened) database paths, as well as the latency of
 * restart-adjacent operations like WAL replay and DB open.
 *
 * Scenarios
 * ---------
 *   Scenario 1  – Cold path: open a fresh DB and measure first-N reads.
 *   Scenario 2  – Warm path: after 500-item warm-up, measure next-N reads.
 *   Scenario 3  – DB reopen latency: measure `close() + open()` round-trip.
 *   Scenario 4  – WAL-replay simulation: write batch with WAL enabled, then
 *                 reopen and read back (recovery-adjacent path).
 *   Scenario 5  – Warm vs. cold ANN vector search (kNN on cold vs. warm index).
 *
 * Methodology
 * -----------
 * - Cold benchmarks call `state.PauseTiming()` to exclude data population from
 *   the measurement, then `state.ResumeTiming()` before the timed operation.
 * - Warm benchmarks keep the fixture across iterations (normal benchmark loop).
 * - `state.counters["latency_us"]` captures per-operation mean microseconds.
 *
 * Run (Release build):
 * @code
 *   ./bench_w2c_recovery --benchmark_filter=W2C
 *   ./bench_w2c_recovery --benchmark_format=json --benchmark_out=w2c.json
 * @endcode
 *
 * Interpretation
 * --------------
 * - Warm/cold ratio > 2 indicates significant block-cache benefit.
 * - DB reopen latency scales with WAL size; compare Scenario 3 (no WAL) vs.
 *   Scenario 4 (WAL replay) to quantify recovery overhead.
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "index/vector_index.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"

namespace fs = std::filesystem;
using namespace themis;

// ============================================================================
// Helpers
// ============================================================================

namespace {

std::string uniqueDbPath(const char* tag) {
    auto ns = static_cast<uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return std::string("./data/w2c_") + tag + "_" + std::to_string(ns);
}

/// @brief Open a RocksDB instance.
/// @param wal  true = WAL enabled (durable), false = benchmark mode.
std::unique_ptr<RocksDBWrapper> openDb(const std::string& path, bool wal) {
    fs::create_directories(path);
    RocksDBWrapper::Config cfg;
    cfg.db_path                   = path;
    cfg.enable_wal                = wal;
    cfg.disable_wal_for_benchmark = !wal;
    cfg.memtable_size_mb          = 32;
    cfg.block_cache_size_mb       = 64;
    auto db = std::make_unique<RocksDBWrapper>(cfg);
    if (!db->open()) {
        throw std::runtime_error("W2C: failed to open DB at " + path);
    }
    return db;
}

std::vector<float> randomVec(int dim, std::mt19937& rng) {
    std::uniform_real_distribution<float> dis(-1.f, 1.f);
    std::vector<float> v(static_cast<std::size_t>(dim));
    for (auto& x : v) {
      x = dis(rng);
    }
    float sq = 0.f;
    for (float x : v) {
      sq += x * x;
    }
    float inv = 1.f / std::sqrt(std::max(sq, 1e-12f));
    for (auto& x : v) {
      x *= inv;
    }
    return v;
}

} // namespace

// ============================================================================
// Scenario 1 – Cold path read
// ============================================================================

/**
 * @brief B2-C Scenario 1: Cold-path KV read.
 *
 * Creates a fresh DB, populates @p range(0) keys outside the timed loop,
 * then measures the first read on each key (cache cold).
 */
static void BM_W2C_ColdRead(benchmark::State& state) {
    const int numKeys = static_cast<int>(state.range(0));
    const std::string dbPath = uniqueDbPath("cold");
    fs::remove_all(dbPath);

    // Populate data (not timed)
    {
        auto db = openDb(dbPath, /*wal=*/false);
        for (int i = 0; i < numKeys; ++i) {
            db->put("k_" + std::to_string(i), std::string(128, 'c'));
        }
        db->close();
    }

    std::mt19937 rng(77);
    std::uniform_int_distribution<int> keyDist(0, numKeys - 1);

    for (auto _ : state) {
        // Re-open DB each iteration to flush OS cache effects (cold start)
        state.PauseTiming();
        auto db = openDb(dbPath, /*wal=*/false);
        state.ResumeTiming();

        // Timed: cold read
        std::string key = "k_" + std::to_string(keyDist(rng));
        std::string out = {};
        bool found = db->get(key, out);
        benchmark::DoNotOptimize(found);
        benchmark::DoNotOptimize(out);

        state.PauseTiming();
        db->close();
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["latency_us"] = benchmark::Counter(
        static_cast<double>(state.iterations()),
        benchmark::Counter::kIsRate | benchmark::Counter::kInvert,
        benchmark::Counter::OneK::kIs1000);

    fs::remove_all(dbPath);
}

BENCHMARK(BM_W2C_ColdRead)
    ->Arg(500)
    ->Arg(2000)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Scenario 2 – Warm path read
// ============================================================================

/**
 * @brief Fixture for the warm-path KV read scenario.
 *
 * SetUp populates @p kWarmKeys keys and executes @p kWarmupReads reads to
 * heat the block cache before measurement starts.
 */
class W2C_WarmReadFixture : public benchmark::Fixture {
public:
    static constexpr int kWarmKeys    = 2'000;
    static constexpr int kWarmupReads = 500;

    void SetUp(::benchmark::State& /*state*/) override {
        dbPath_ = uniqueDbPath("warm");
        db_     = openDb(dbPath_, /*wal=*/false);

        for (int i = 0; i < kWarmKeys; ++i) {
            db_->put("k_" + std::to_string(i), std::string(128, 'w'));
        }
        // Warm the block cache
        for (int i = 0; i < kWarmupReads; ++i) {
            std::string out = {};
            db_->get("k_" + std::to_string(i % kWarmKeys), out);
        }
    }

    void TearDown(::benchmark::State& /*state*/) override {
        db_->close();
        db_.reset();
        fs::remove_all(dbPath_);
    }

protected:
    std::string                     dbPath_;
    std::unique_ptr<RocksDBWrapper> db_;
};

/**
 * @brief B2-C Scenario 2: Warm-path KV read.
 *
 * Block cache is hot; measures cache-hit read latency.
 * Compare with BM_W2C_ColdRead to quantify the warm/cold delta.
 */
BENCHMARK_DEFINE_F(W2C_WarmReadFixture, WarmRead)(benchmark::State& state) {
    std::mt19937 rng(88);
    std::uniform_int_distribution<int> keyDist(0, kWarmKeys - 1);

    for (auto _ : state) {
        std::string key = "k_" + std::to_string(keyDist(rng));
        std::string out = {};
        bool found = db_->get(key, out);
        benchmark::DoNotOptimize(found);
        benchmark::DoNotOptimize(out);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["latency_us"] = benchmark::Counter(
        static_cast<double>(state.iterations()),
        benchmark::Counter::kIsRate | benchmark::Counter::kInvert,
        benchmark::Counter::OneK::kIs1000);
}

BENCHMARK_REGISTER_F(W2C_WarmReadFixture, WarmRead)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Scenario 3 – DB reopen latency (no WAL)
// ============================================================================

/**
 * @brief B2-C Scenario 3: DB close + reopen latency without WAL.
 *
 * The full `close() + open()` cycle is timed.  This is the best-case restart
 * latency with no WAL recovery required.
 */
static void BM_W2C_ReopenLatency_NoWAL(benchmark::State& state) {
    const int numKeys = static_cast<int>(state.range(0));
    const std::string dbPath = uniqueDbPath("reopen_nowal");
    fs::remove_all(dbPath);

    // Populate data outside timed loop
    {
        auto db = openDb(dbPath, /*wal=*/false);
        for (int i = 0; i < numKeys; ++i) {
            db->put("r_" + std::to_string(i), std::string(64, 'r'));
        }
        db->close();
    }

    std::unique_ptr<RocksDBWrapper> db = {};

    for (auto _ : state) {
        // Timed: full close + reopen round-trip
        if (db) { db->close(); db.reset(); }
        db = openDb(dbPath, /*wal=*/false);
        benchmark::DoNotOptimize(db.get());
    }

    if (db) { db->close(); db.reset(); }
    fs::remove_all(dbPath);
}

BENCHMARK(BM_W2C_ReopenLatency_NoWAL)
    ->Arg(100)
    ->Arg(1'000)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Scenario 4 – WAL replay simulation (recovery-adjacent)
// ============================================================================

/**
 * @brief B2-C Scenario 4: Write with WAL, then reopen and read back.
 *
 * Simulates a restart-adjacent recovery path:
 *   1. Write @p range(0) keys with WAL enabled (durable).
 *   2. Close DB (WAL persisted).
 *   3. Timed: reopen and read first key (WAL replay + first read).
 *
 * This quantifies the overhead of WAL recovery relative to Scenario 3.
 */
static void BM_W2C_WalReplayRead(benchmark::State& state) {
    const int numKeys = static_cast<int>(state.range(0));
    const std::string dbPath = uniqueDbPath("wal_replay");

    for (auto _ : state) {
        // Setup: write with WAL outside timed section
        state.PauseTiming();
        fs::remove_all(dbPath);
        {
            auto db = openDb(dbPath, /*wal=*/true);
            for (int i = 0; i < numKeys; ++i) {
                db->put("wal_" + std::to_string(i), std::string(64, 'd'));
            }
            db->close();
        }
        state.ResumeTiming();

        // Timed: reopen (WAL replay) + first read
        auto db = openDb(dbPath, /*wal=*/true);
        std::string out = {};
        bool found = db->get("wal_0", out);
        benchmark::DoNotOptimize(found);
        benchmark::DoNotOptimize(out);

        state.PauseTiming();
        db->close();
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["wal_keys_written"] = static_cast<double>(numKeys);

    fs::remove_all(dbPath);
}

BENCHMARK(BM_W2C_WalReplayRead)
    ->Arg(100)
    ->Arg(500)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Scenario 5 – ANN vector search: cold vs. warm index
// ============================================================================

/**
 * @brief B2-C Scenario 5a: Cold ANN kNN query (index freshly initialised).
 *
 * Measures kNN latency on a freshly opened VectorIndexManager without
 * any prior search warm-up.  Baseline for cold-start ANN cost.
 */
static void BM_W2C_AnnSearch_Cold(benchmark::State& state) {
    const int dim     = static_cast<int>(state.range(0));
    const int numVecs = 1'000;
    const int topK    = 10;

    for (auto _ : state) {
        state.PauseTiming();
        const std::string dbPath = uniqueDbPath("anncold");
        fs::remove_all(dbPath);
        auto db = openDb(dbPath, /*wal=*/false);
        auto vix = std::make_unique<VectorIndexManager>(*db);
        {
            auto st = vix->init("cold", dim, VectorIndexManager::Metric::COSINE,
                                /*M=*/16, /*efConstruction=*/100, /*ef=*/64);
            if (!st.ok) { state.SkipWithError(st.message.c_str()); return; }
        }
        std::mt19937 rng(11);
        for (int i = 0; i < numVecs; ++i) {
            BaseEntity e("c_" + std::to_string(i));
            e.setField("embedding", Value{randomVec(dim, rng)});
            vix->addEntity(e);
        }
        auto query = randomVec(dim, rng);
        state.ResumeTiming();

        // Timed: cold kNN
        auto [qst, results] = vix->searchKnn(query, topK);
        benchmark::DoNotOptimize(qst.ok);
        benchmark::DoNotOptimize(results);

        state.PauseTiming();
        vix.reset();
        db->close();
        db.reset();
        fs::remove_all(dbPath);
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_W2C_AnnSearch_Cold)
    ->Arg(64)
    ->Arg(128)
    ->Unit(benchmark::kMicrosecond);

/**
 * @brief Fixture for the warm ANN search scenario.
 *
 * SetUp populates and then runs @p kWarmupSearches queries to heat the HNSW
 * internal caches before measurement begins.
 */
class W2C_WarmAnnFixture : public benchmark::Fixture {
public:
    static constexpr int kDim            = 128;
    static constexpr int kNumVecs        = 1'000;
    static constexpr int kWarmupSearches = 100;
    static constexpr int kTopK           = 10;

    void SetUp(::benchmark::State& /*state*/) override {
        dbPath_ = uniqueDbPath("annwarm");
        db_     = openDb(dbPath_, /*wal=*/false);
        vix_    = std::make_unique<VectorIndexManager>(*db_);

        auto st = vix_->init("warm", kDim, VectorIndexManager::Metric::COSINE,
                             /*M=*/16, /*efConstruction=*/100, /*ef=*/64);
        if (!st.ok) {
            throw std::runtime_error("W2C WarmAnn init: " + st.message);
        }
        std::mt19937 rng(22);
        for (int i = 0; i < kNumVecs; ++i) {
            BaseEntity e("w_" + std::to_string(i));
            e.setField("embedding", Value{randomVec(kDim, rng)});
            vix_->addEntity(e);
        }
        // Warmup searches
        for (int i = 0; i < kWarmupSearches; ++i) {
            auto q = randomVec(kDim, rng);
            benchmark::DoNotOptimize(vix_->searchKnn(q, kTopK));
        }
    }

    void TearDown(::benchmark::State& /*state*/) override {
        vix_.reset();
        db_->close();
        db_.reset();
        fs::remove_all(dbPath_);
    }

protected:
    std::string                         dbPath_;
    std::unique_ptr<RocksDBWrapper>     db_;
    std::unique_ptr<VectorIndexManager> vix_;
};

/**
 * @brief B2-C Scenario 5b: Warm ANN kNN query.
 *
 * HNSW caches are hot; measures steady-state kNN latency.
 * Compare with BM_W2C_AnnSearch_Cold to quantify the warm/cold ANN delta.
 */
BENCHMARK_DEFINE_F(W2C_WarmAnnFixture, AnnSearch_Warm)(benchmark::State& state) {
    std::mt19937 rng(44 + static_cast<uint32_t>(state.thread_index()));

    for (auto _ : state) {
        auto query = randomVec(kDim, rng);
        auto [qst, results] = vix_->searchKnn(query, kTopK);
        benchmark::DoNotOptimize(qst.ok);
        benchmark::DoNotOptimize(results);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["latency_us"] = benchmark::Counter(
        static_cast<double>(state.iterations()),
        benchmark::Counter::kIsRate | benchmark::Counter::kInvert,
        benchmark::Counter::OneK::kIs1000);
}

BENCHMARK_REGISTER_F(W2C_WarmAnnFixture, AnnSearch_Warm)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
