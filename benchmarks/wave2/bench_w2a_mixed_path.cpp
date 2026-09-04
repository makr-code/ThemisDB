// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w2a_mixed_path.cpp
 * @brief Wave 2 / B2-A: Mixed-path cross-module performance benchmarks.
 *
 * Measures end-to-end latency and throughput of multi-module pipelines that
 * cross the storage, index, and query boundaries:
 *
 *   Pipeline A  – KV ingest → secondary-index update → secondary lookup
 *   Pipeline B  – KV ingest → vector-index insert → ANN kNN query
 *   Pipeline C  – Batch ingest → vector insert → kNN  (batch variant)
 *
 * Methodology
 * -----------
 * - Fixture SetUp populates the database with a deterministic seed dataset so
 *   that each benchmark iteration measures only the *incremental* path.
 * - Warmup records are inserted outside the timed loop; measured inserts and
 *   queries are inside `for (auto _ : state)`.
 * - `benchmark::DoNotOptimize` prevents dead-code elimination of results.
 * - All DB/index resources are torn down in TearDown to avoid crosstalk.
 *
 * Run (Release build required for meaningful numbers):
 * @code
 *   ./bench_w2a_mixed_path --benchmark_filter=W2A
 *   ./bench_w2a_mixed_path --benchmark_format=json --benchmark_out=w2a.json
 * @endcode
 *
 * Interpretation
 * --------------
 * - `items_per_second`: complete pipeline round-trips per second.
 * - `pipeline_ns`:      mean per-iteration latency in nanoseconds.
 * - Compare `W2A_IngestIndexQuery_*` variants to understand how dimension and
 *   batch size affect cross-module overhead relative to single-module cost.
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "index/secondary_index.h"
#include "index/vector_index.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"

namespace fs = std::filesystem;
using namespace themis;

// ============================================================================
// Shared utilities
// ============================================================================

namespace {

/// @brief Deterministic path generation to avoid fixture collision.
std::string uniqueDbPath(const char* tag) {
    auto ns = static_cast<uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return std::string("./data/w2a_") + tag + "_" + std::to_string(ns);
}

/// @brief Generate a normalised random float vector of @p dim dimensions.
std::vector<float> randomVec(std::size_t dim, std::mt19937& rng) {
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    std::vector<float> v(dim);
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

/// @brief Open a RocksDB instance at @p path optimised for benchmark use.
std::unique_ptr<RocksDBWrapper> openBenchDb(const std::string& path) {
    fs::remove_all(path);
    fs::create_directories(path);

    RocksDBWrapper::Config cfg;
    cfg.db_path                    = path;
    cfg.enable_wal                 = false;
    cfg.disable_wal_for_benchmark  = true;
    cfg.memtable_size_mb           = 64;
    cfg.block_cache_size_mb        = 128;
    auto db = std::make_unique<RocksDBWrapper>(cfg);
    if (!db->open()) {
        throw std::runtime_error("W2A: failed to open RocksDB at " + path);
    }
    return db;
}

} // namespace

// ============================================================================
// Pipeline A – KV ingest → secondary index update → secondary lookup
// ============================================================================

/**
 * @brief Fixture for the KV + secondary-index pipeline.
 *
 * SetUp creates the DB, creates the secondary index, and pre-populates
 * @p kWarmupRows rows so that the index is in a realistic warm state before
 * measurement starts.
 */
class W2A_SecondaryPipelineFixture : public benchmark::Fixture {
public:
    static constexpr std::size_t kDim        = 0;       ///< Not used here
    static constexpr int         kWarmupRows = 1'000;   ///< Pre-populated rows
    static constexpr char const* kTable      = "w2a_kv";
    static constexpr char const* kCol        = "category";

    void SetUp(::benchmark::State& state) override {
        if (state.thread_index() != 0) {
          return;
        }
        dbPath_ = uniqueDbPath("sec");
        db_     = openBenchDb(dbPath_);
        idx_    = std::make_unique<SecondaryIndexManager>(*db_);

        // Create the secondary index on the category column
        idx_->createIndex(kTable, kCol);

        // Pre-populate warm data
        std::mt19937 rng(17);
        std::uniform_int_distribution<int> catDist(0, 9);
        for (int i = 0; i < kWarmupRows; ++i) {
            BaseEntity e("w_" + std::to_string(i));
            std::string cat = "cat_" + std::to_string(catDist(rng));
            e.setField(kCol, cat);
            idx_->put(kTable, e);
        }
        nextId_.store(kWarmupRows, std::memory_order_relaxed);
    }

    void TearDown(::benchmark::State& state) override {
        if (state.thread_index() != 0) {
          return;
        }
        idx_.reset();
        db_->close();
        db_.reset();
        fs::remove_all(dbPath_);
    }

protected:
    std::string                              dbPath_;
    std::unique_ptr<RocksDBWrapper>          db_;
    std::unique_ptr<SecondaryIndexManager>   idx_;
    std::atomic<int>                         nextId_{0};
};

/**
 * @brief B2-A – Pipeline A: ingest one document, update secondary index,
 *        then lookup by the indexed field.
 *
 * Measured: insert + secondaryIndex.insert + secondaryIndex.lookup
 */
BENCHMARK_DEFINE_F(W2A_SecondaryPipelineFixture, IngestIndexLookup)(benchmark::State& state) {
    std::mt19937 rng(99 + static_cast<uint32_t>(state.thread_index()));
    std::uniform_int_distribution<int> catDist(0, 9);

    for (auto _ : state) {
        int id = nextId_.fetch_add(1, std::memory_order_relaxed);
        std::string key = "e_" + std::to_string(id);
        std::string cat = "cat_" + std::to_string(catDist(rng));

        // Ingest: write raw KV
        bool ok = db_->put(key, cat);
        benchmark::DoNotOptimize(ok);

        // Index update
        BaseEntity e(key);
        e.setField(kCol, cat);
        auto st = idx_->put(kTable, e);
        benchmark::DoNotOptimize(st.ok);

        // Query: secondary lookup for the same category
        auto [qst, results] = idx_->scanKeysEqual(kTable, kCol, cat);
        benchmark::DoNotOptimize(qst.ok);
        benchmark::DoNotOptimize(results);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["pipeline_ns"] = benchmark::Counter(
        static_cast<double>(state.iterations()),
        benchmark::Counter::kIsRate | benchmark::Counter::kInvert,
        benchmark::Counter::OneK::kIs1000);
}

BENCHMARK_REGISTER_F(W2A_SecondaryPipelineFixture, IngestIndexLookup)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Pipeline B – KV ingest → vector index insert → ANN kNN query
// ============================================================================

/**
 * @brief Fixture for the KV + vector-index pipeline.
 *
 * SetUp initialises a VectorIndexManager with the given dimension (range(0))
 * and pre-populates @p kWarmupVectors vectors so that HNSW is already layered.
 */
class W2A_VectorPipelineFixture : public benchmark::Fixture {
public:
    static constexpr int kWarmupVectors = 2'000;
    static constexpr int kTopK          = 10;

    void SetUp(::benchmark::State& state) override {
        dim_    = static_cast<int>(state.range(0) > 0 ? state.range(0) : 128);
        dbPath_ = uniqueDbPath("vec");
        db_     = openBenchDb(dbPath_);
        vix_    = std::make_unique<VectorIndexManager>(*db_);

        auto st = vix_->init("w2a_vecs", dim_, VectorIndexManager::Metric::COSINE,
                             /*M=*/16, /*efConstruction=*/100, /*ef=*/64);
        if (!st.ok) {
            throw std::runtime_error("W2A: VectorIndex init failed: " + st.message);
        }

        // Pre-populate warm vectors
        std::mt19937 rng(42);
        for (int i = 0; i < kWarmupVectors; ++i) {
            BaseEntity e("w_" + std::to_string(i));
            e.setField("embedding", Value{randomVec(static_cast<std::size_t>(dim_), rng)});
            vix_->addEntity(e);
        }
        nextId_.store(kWarmupVectors, std::memory_order_relaxed);
    }

    void TearDown(::benchmark::State& /*state*/) override {
        vix_.reset();
        db_->close();
        db_.reset();
        fs::remove_all(dbPath_);
    }

protected:
    int                                   dim_{128};
    std::string                           dbPath_;
    std::unique_ptr<RocksDBWrapper>       db_;
    std::unique_ptr<VectorIndexManager>   vix_;
    std::atomic<int>                      nextId_{0};
};

/**
 * @brief B2-A – Pipeline B: ingest one raw KV entry, insert into vector index,
 *        then execute an ANN kNN query.
 *
 * Measured: put + vix.addEntity + vix.searchKnn
 */
BENCHMARK_DEFINE_F(W2A_VectorPipelineFixture, IngestIndexQuery)(benchmark::State& state) {
    std::mt19937 rng(7 + static_cast<uint32_t>(state.thread_index()));

    for (auto _ : state) {
        int id = nextId_.fetch_add(1, std::memory_order_relaxed);
        auto vec = randomVec(static_cast<std::size_t>(dim_), rng);

        // Ingest raw KV
        std::string key = "v_" + std::to_string(id);
        db_->put(key, std::string(reinterpret_cast<const char*>(vec.data()),
                                  vec.size() * sizeof(float)));

        // Index update
        BaseEntity e(key);
        e.setField("embedding", Value{vec});
        auto insertStatus = vix_->addEntity(e);
        benchmark::DoNotOptimize(insertStatus.ok);

        // Query: kNN for the freshly inserted vector
        auto query = randomVec(static_cast<std::size_t>(dim_), rng);
        auto [qst, results] = vix_->searchKnn(query, kTopK);
        benchmark::DoNotOptimize(qst.ok);
        benchmark::DoNotOptimize(results);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["items_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()), benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(W2A_VectorPipelineFixture, IngestIndexQuery)
    ->Arg(64)
    ->Arg(128)
    ->Arg(256)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Pipeline C – Batch ingest → batch vector insert → kNN query
// ============================================================================

/**
 * @brief B2-A – Pipeline C: batch-ingest N docs, batch-insert into vector index,
 *        then run a single kNN query.
 *
 * Separates setup (fixture population) from the measured batch pipeline.
 * Parametrised over batch size via range(0) and vector dimension via range(1).
 */
static void BM_W2A_BatchIngestIndexQuery(benchmark::State& state) {
    const int batchSize = static_cast<int>(state.range(0));
    const int dim       = static_cast<int>(state.range(1));
    const int topK      = 10;

    const std::string dbPath = uniqueDbPath("batch");
    auto db = openBenchDb(dbPath);

    auto vix = std::make_unique<VectorIndexManager>(*db);
    {
        auto st = vix->init("w2a_batch", dim, VectorIndexManager::Metric::L2,
                            /*M=*/16, /*efConstruction=*/100, /*ef=*/64);
        if (!st.ok) {
            state.SkipWithError(("W2A batch: init failed: " + st.message).c_str());
            return;
        }
    }

    // Warmup: pre-populate outside measured loop
    std::mt19937 warmRng(55);
    for (int i = 0; i < 500; ++i) {
        BaseEntity e("warm_" + std::to_string(i));
        e.setField("embedding", Value{randomVec(static_cast<std::size_t>(dim), warmRng)});
        vix->addEntity(e);
    }

    std::atomic<int> nextId{500};
    for (auto _ : state) {
        state.PauseTiming();
        std::mt19937 rng(static_cast<uint32_t>(nextId.load()));
        std::vector<std::pair<std::string, std::vector<float>>> batch;
        batch.reserve(static_cast<std::size_t>(batchSize));
        for (int i = 0; i < batchSize; ++i) {
            int id = nextId.fetch_add(1, std::memory_order_relaxed);
            batch.emplace_back("b_" + std::to_string(id),
                               randomVec(static_cast<std::size_t>(dim), rng));
        }
        state.ResumeTiming();

        // Measured: batch-ingest + batch-index
        for (auto& [key, vec] : batch) {
            db->put(key, std::string(reinterpret_cast<const char*>(vec.data()),
                                     vec.size() * sizeof(float)));
            BaseEntity e(key);
            e.setField("embedding", Value{vec});
            benchmark::DoNotOptimize(vix->addEntity(e).ok);
        }

        // Measured: one kNN query across the now-updated index
        auto query = randomVec(static_cast<std::size_t>(dim), rng);
        auto [qst, results] = vix->searchKnn(query, topK);
        benchmark::DoNotOptimize(qst.ok);
        benchmark::DoNotOptimize(results);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
    state.counters["batch_size"]  = static_cast<double>(batchSize);
    state.counters["dim"]         = static_cast<double>(dim);
    state.counters["items_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations() * batchSize),
        benchmark::Counter::kIsRate);

    vix.reset();
    db->close();
    db.reset();
    fs::remove_all(dbPath);
}

BENCHMARK(BM_W2A_BatchIngestIndexQuery)
    ->Args({8,  64})
    ->Args({32, 64})
    ->Args({8,  128})
    ->Args({32, 128})
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
