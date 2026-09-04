// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w5a_production_workloads.cpp
 * @brief Wave 5 / PR B5-A — Production-Critical Workload Coverage
 *
 * Benchmarks production-representative end-to-end workloads across the four
 * critical paths of ThemisDB:
 *   1. CRUD (keyed read/write) — steady-state and burst variants
 *   2. Vector similarity search — approximate nearest-neighbour (ANN)
 *   3. Graph traversal — multi-hop BFS over a sparse adjacency graph
 *   4. Ingest pipeline — batch write under realistic field diversity
 *
 * Design principles (Wave 5 hygiene):
 *   - kW5CanonicalSeed = 42 for all RNG initialisation (reproducible runs)
 *   - All I/O paths use OS temp dir + per-run steady_clock suffix
 *   - I/O-bound registrations call UseRealTime()
 *   - Warmup iterations are performed inside SetUp() before measurement
 *   - Burst variants use Arg(1) > 0 to signal 5× peak concurrency
 *
 * Perf expectations (8-core dev box, NVMe, Release build):
 *   BM_W5A_CRUD_SteadyStateRead    ≥ 150 000 ops/s
 *   BM_W5A_CRUD_SteadyStateWrite   ≥  80 000 ops/s
 *   BM_W5A_CRUD_BurstRead          ≥  60 000 ops/s  (3× threads)
 *   BM_W5A_VectorSearch_1NN        ≤ 200 µs/op  (10k vectors, d=128)
 *   BM_W5A_GraphBFS_2Hop           ≤ 500 µs/op  (5k nodes, 5 edges/node)
 *   BM_W5A_BatchIngest_Sustained   ≥  20 000 rec/s
 *
 * CI output: --benchmark_out=bench_w5a.json --benchmark_out_format=json
 * Baseline: benchmarks/baselines/wave5/bench_w5a_baseline.json
 */

#include <benchmark/benchmark.h>

#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/vector_index.h"
#include "index/graph_index.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace themis;

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr uint64_t kW5CanonicalSeed = 42;
static constexpr int      kWarmupRecords   = 200;
static constexpr int      kDefaultRecords  = 5'000;
static constexpr int      kVectorDim       = 128;
static constexpr int      kDefaultVectors  = 10'000;
static constexpr int      kGraphNodes      = 5'000;
static constexpr int      kEdgesPerNode    = 5;
static constexpr int      kBurstMultiplier = 3;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace w5a {

static fs::path makeTempPath(std::string_view prefix) {
    auto suffix = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return fs::temp_directory_path() / (std::string(prefix) + "_" + suffix);
}

class Rng {
public:
    explicit Rng(uint64_t seed = kW5CanonicalSeed)
        : eng_(static_cast<std::mt19937_64::result_type>(seed)) {}

    std::string key(std::size_t len = 16) {
        static constexpr std::string_view kAlpha =
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::uniform_int_distribution<std::size_t> d(0, kAlpha.size() - 1);
        std::string s(len, ' ');
        for (auto& c : s) {
          c = kAlpha[d(eng_)];
        }
        return s;
    }

    std::vector<float> vec(std::size_t dim = kVectorDim) {
        std::uniform_real_distribution<float> d(-1.0f, 1.0f);
        std::vector<float> v(dim);
        for (auto& x : v) {
          x = d(eng_);
        }
        return v;
    }

    int64_t integer(int64_t lo, int64_t hi) {
        std::uniform_int_distribution<int64_t> d(lo, hi);
        return d(eng_);
    }

private:
    std::mt19937_64 eng_;
};

} // namespace w5a

// ===========================================================================
// 1. CRUD — Steady-State & Burst
// ===========================================================================

/**
 * @brief Fixture for keyed CRUD benchmarks.
 *
 * Provides a pre-populated RocksDB instance with @p kDefaultRecords entities
 * (warmup phase inside SetUp) plus a secondary index for range queries.
 * Burst variant is controlled via Arg(0): 0 = single-threaded, >0 = N threads.
 */
class W5aCrudFixture : public benchmark::Fixture {
public:
    void SetUp(::benchmark::State& /*state*/) override {
        dbPath_ = w5a::makeTempPath("w5a_crud");
        fs::create_directories(dbPath_);

        RocksDBWrapper::Config cfg;
        cfg.db_path            = dbPath_.string();
        cfg.block_cache_size_mb = 128;
        cfg.compression_default = "lz4";
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open())
            throw std::runtime_error("W5aCrudFixture: RocksDB open failed");

        idx_ = std::make_unique<SecondaryIndexManager>(*db_);
        idx_->createIndex("rec", "tag", false);
        idx_->createRangeIndex("rec", "ts");

        // Warmup: load kWarmupRecords before measurement
        w5a::Rng rng(kW5CanonicalSeed);
        for (int i = 0; i < kWarmupRecords; ++i) {
            BaseEntity e("wu_" + std::to_string(i));
            e.setField("tag", rng.key(8));
            e.setField("ts", static_cast<int64_t>(i));
            idx_->put("rec", e);
        }

        // Pre-load keys for read benchmarks
        readKeys_.reserve(kDefaultRecords);
        for (int i = 0; i < kDefaultRecords; ++i) {
            const std::string k = "k_" + std::to_string(i);
            readKeys_.push_back(k);
            BaseEntity e(k);
            e.setField("tag", rng.key(8));
            e.setField("ts", static_cast<int64_t>(1'000'000 + i));
            idx_->put("rec", e);
        }
        writeCounter_.store(kDefaultRecords + kWarmupRecords);
    }

    void TearDown(::benchmark::State& /*state*/) override {
        idx_.reset();
        db_->close();
        db_.reset();
        std::error_code ec;
        fs::remove_all(dbPath_, ec);
    }

protected:
    fs::path                                dbPath_;
    std::unique_ptr<RocksDBWrapper>         db_;
    std::unique_ptr<SecondaryIndexManager>  idx_;
    std::vector<std::string>                readKeys_;
    std::atomic<int>                        writeCounter_{0};
    w5a::Rng                                rng_{kW5CanonicalSeed + 1};
};

/**
 * @brief BM_W5A_CRUD_SteadyStateRead
 *
 * Measures single-threaded point-lookup throughput on a pre-warmed dataset.
 * Access pattern: round-robin across all kDefaultRecords keys to capture
 * both hot-cache and cold-cache behaviour within a single benchmark run.
 */
BENCHMARK_DEFINE_F(W5aCrudFixture, SteadyStateRead)(benchmark::State& state) {
    std::size_t idx = 0;
    for (auto _ : state) {
        const auto& key = readKeys_[idx % readKeys_.size()];
        auto blob = db_->get(KeySchema::makeRelationalKey("rec", key));
        benchmark::DoNotOptimize(blob);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("steady-state point-lookup, round-robin");
}
BENCHMARK_REGISTER_F(W5aCrudFixture, SteadyStateRead)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Iterations(50'000);

/**
 * @brief BM_W5A_CRUD_SteadyStateWrite
 *
 * Measures single-threaded write throughput with a secondary index maintained
 * on two fields.  Each iteration generates a unique key to avoid merge paths.
 */
BENCHMARK_DEFINE_F(W5aCrudFixture, SteadyStateWrite)(benchmark::State& state) {
    for (auto _ : state) {
        const int id = writeCounter_.fetch_add(1, std::memory_order_relaxed);
        BaseEntity e("w_" + std::to_string(id));
        e.setField("tag", rng_.key(8));
        e.setField("ts", static_cast<int64_t>(id));
        idx_->put("rec", e);
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("steady-state write with 2 secondary indexes");
}
BENCHMARK_REGISTER_F(W5aCrudFixture, SteadyStateWrite)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Iterations(30'000);

/**
 * @brief BM_W5A_CRUD_BurstRead
 *
 * Simulates a read burst: kBurstMultiplier goroutines each performing
 * consecutive point-lookups using the same RocksDB instance.
 * Models short-duration traffic spikes (e.g. cold start, cache invalidation).
 */
BENCHMARK_DEFINE_F(W5aCrudFixture, BurstRead)(benchmark::State& state) {
    std::size_t idx = static_cast<std::size_t>(state.thread_index()) * 7;
    for (auto _ : state) {
        const auto& key = readKeys_[idx % readKeys_.size()];
        auto blob = db_->get(KeySchema::makeRelationalKey("rec", key));
        benchmark::DoNotOptimize(blob);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("burst read (3 threads, staggered access)");
}
BENCHMARK_REGISTER_F(W5aCrudFixture, BurstRead)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Threads(kBurstMultiplier)
    ->Iterations(20'000);

// ===========================================================================
// 2. Vector similarity search
// ===========================================================================

/**
 * @brief Fixture for ANN (1-NN) vector search benchmarks.
 *
 * Populates a VectorIndex with kDefaultVectors d=128 L2-normalised vectors
 * during SetUp.  Query vectors are pre-generated with a different RNG seed
 * so they are not identical to any corpus vector.
 */
class W5aVectorFixture : public benchmark::Fixture {
public:
    static constexpr std::size_t kQueryBatch = 256;

    void SetUp(::benchmark::State& /*state*/) override {
        dbPath_ = w5a::makeTempPath("w5a_vec");
        fs::create_directories(dbPath_);

        RocksDBWrapper::Config cfg;
        cfg.db_path             = dbPath_.string();
        cfg.block_cache_size_mb = 128;
        cfg.compression_default = "lz4";
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open())
            throw std::runtime_error("W5aVectorFixture: RocksDB open failed");

        idx_ = std::make_unique<VectorIndexManager>(*db_);
        const auto initStatus = idx_->init("vec", kVectorDim, VectorIndexManager::Metric::COSINE);
        if (!initStatus.ok)
            throw std::runtime_error("W5aVectorFixture: Vector index init failed: " + initStatus.message);

        w5a::Rng rng(kW5CanonicalSeed);
        for (int i = 0; i < kDefaultVectors; ++i) {
            BaseEntity entity("v_" + std::to_string(i));
            entity.setField("embedding", rng.vec(kVectorDim));
            const auto addStatus = idx_->addEntity(entity, "embedding");
            if (!addStatus.ok)
                throw std::runtime_error("W5aVectorFixture: addEntity failed: " + addStatus.message);
        }

        // Pre-generate query vectors
        w5a::Rng qrng(kW5CanonicalSeed + 99);
        queries_.reserve(kQueryBatch);
        for (std::size_t i = 0; i < kQueryBatch; ++i)
            queries_.push_back(qrng.vec(kVectorDim));
    }

    void TearDown(::benchmark::State& /*state*/) override {
        idx_.reset();
        if (db_) {
            db_->close();
            db_.reset();
        }
        std::error_code ec;
        fs::remove_all(dbPath_, ec);
    }

protected:
    fs::path                            dbPath_;
    std::unique_ptr<RocksDBWrapper>     db_;
    std::unique_ptr<VectorIndexManager> idx_;
    std::vector<std::vector<float>>     queries_;
};

/**
 * @brief BM_W5A_VectorSearch_1NN
 *
 * Measures approximate 1-nearest-neighbour query latency over a corpus of
 * kDefaultVectors (10k) d=128 vectors.  Cycles through pre-generated query
 * vectors to prevent loop folding by the optimiser.
 */
BENCHMARK_DEFINE_F(W5aVectorFixture, Search1NN)(benchmark::State& state) {
    std::size_t qi = 0;
    for (auto _ : state) {
        auto [status, results] = idx_->searchKnn(queries_[qi % queries_.size()], 1);
        benchmark::DoNotOptimize(status.ok);
        benchmark::DoNotOptimize(results);
        ++qi;
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("1-NN ANN, 10k vectors, d=128");
}
BENCHMARK_REGISTER_F(W5aVectorFixture, Search1NN)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Iterations(5'000);

/**
 * @brief BM_W5A_VectorSearch_10NN
 *
 * Top-10 ANN retrieval — representative of recommendation and similarity
 * ranking workloads.  Higher k stresses the priority-queue in the index.
 */
BENCHMARK_DEFINE_F(W5aVectorFixture, Search10NN)(benchmark::State& state) {
    std::size_t qi = 0;
    for (auto _ : state) {
        auto [status, results] = idx_->searchKnn(queries_[qi % queries_.size()], 10);
        benchmark::DoNotOptimize(status.ok);
        benchmark::DoNotOptimize(results);
        ++qi;
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("10-NN ANN, 10k vectors, d=128");
}
BENCHMARK_REGISTER_F(W5aVectorFixture, Search10NN)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Iterations(5'000);

// ===========================================================================
// 3. Graph traversal — multi-hop BFS
// ===========================================================================

/**
 * @brief Fixture for graph traversal benchmarks.
 *
 * Builds a sparse random digraph with kGraphNodes nodes and kEdgesPerNode
 * out-edges per node seeded by kW5CanonicalSeed.
 */
class W5aGraphFixture : public benchmark::Fixture {
public:
    void SetUp(::benchmark::State& /*state*/) override {
        dbPath_ = w5a::makeTempPath("w5a_graph");
        fs::create_directories(dbPath_);

        RocksDBWrapper::Config cfg;
        cfg.db_path             = dbPath_.string();
        cfg.block_cache_size_mb = 128;
        cfg.compression_default = "lz4";
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open())
            throw std::runtime_error("W5aGraphFixture: RocksDB open failed");

        graph_ = std::make_unique<GraphIndexManager>(*db_);

        w5a::Rng rng(kW5CanonicalSeed);
        for (int i = 0; i < kGraphNodes; ++i) {
            for (int e = 0; e < kEdgesPerNode; ++e) {
                int j = static_cast<int>(rng.integer(0, kGraphNodes - 1));
                if (j != i) {
                    const std::string from = "n_" + std::to_string(i);
                    const std::string to = "n_" + std::to_string(j);
                    const std::string edgeId = "e_" + std::to_string(i) + "_" + std::to_string(e) + "_" + std::to_string(j);
                    BaseEntity edge(edgeId);
                    edge.setField("id", edgeId);
                    edge.setField("_from", from);
                    edge.setField("_to", to);
                    edge.setField("_type", "link");
                    const auto status = graph_->addEdge(edge);
                    if (!status.ok)
                        throw std::runtime_error("W5aGraphFixture: addEdge failed: " + status.message);
                }
            }
        }
    }

    void TearDown(::benchmark::State& /*state*/) override {
        graph_.reset();
        if (db_) {
            db_->close();
            db_.reset();
        }
        std::error_code ec;
        fs::remove_all(dbPath_, ec);
    }

protected:
    fs::path                           dbPath_;
    std::unique_ptr<RocksDBWrapper>    db_;
    std::unique_ptr<GraphIndexManager> graph_;
    w5a::Rng                           rng_{kW5CanonicalSeed + 7};
};

/**
 * @brief BM_W5A_GraphBFS_2Hop
 *
 * Measures 2-hop BFS/neighbourhood expansion from random source nodes.
 * Representative of friend-of-a-friend and co-citation queries.
 */
BENCHMARK_DEFINE_F(W5aGraphFixture, BFS2Hop)(benchmark::State& state) {
    int src = 0;
    for (auto _ : state) {
        const std::string node = "n_" + std::to_string(src % kGraphNodes);
        auto [status, neighbours] = graph_->bfs(node, 2);
        benchmark::DoNotOptimize(status.ok);
        benchmark::DoNotOptimize(neighbours);
        ++src;
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("2-hop BFS, 5k nodes, 5 edges/node");
}
BENCHMARK_REGISTER_F(W5aGraphFixture, BFS2Hop)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Iterations(3'000);

// ===========================================================================
// 4. Batch ingest pipeline — sustained write throughput
// ===========================================================================

/**
 * @brief BM_W5A_BatchIngest_Sustained
 *
 * Measures sustained batch-write throughput: 100-record batches committed
 * inside a single transaction.  Arg(0) controls batch size (default 100).
 */
static void BM_W5A_BatchIngest_Sustained(benchmark::State& state) {
    const int batchSize = state.range(0) > 0
        ? static_cast<int>(state.range(0))
        : 100;

    auto dbPath = w5a::makeTempPath("w5a_ingest");
    fs::create_directories(dbPath);

    RocksDBWrapper::Config cfg;
    cfg.db_path             = dbPath.string();
    cfg.block_cache_size_mb = 128;
    cfg.compression_default = "lz4";
    cfg.disable_wal_for_benchmark = false;
    auto db = std::make_unique<RocksDBWrapper>(cfg);
    if (!db->open())
        throw std::runtime_error("BM_W5A_BatchIngest: DB open failed");

    SecondaryIndexManager idx(*db);
    idx.createIndex("evt", "src", false);
    idx.createRangeIndex("evt", "seq");

    w5a::Rng rng(kW5CanonicalSeed + 3);
    int seq = 0;

    // Warmup
    for (int i = 0; i < kWarmupRecords; ++i) {
        BaseEntity e("wu_" + std::to_string(i));
        e.setField("src", rng.key(6));
        e.setField("seq", static_cast<int64_t>(seq++));
        idx.put("evt", e);
    }

    for (auto _ : state) {
        state.PauseTiming();
        std::vector<BaseEntity> batch;
        batch.reserve(batchSize);
        for (int i = 0; i < batchSize; ++i) {
            BaseEntity e("evt_" + std::to_string(seq));
            e.setField("src", rng.key(6));
            e.setField("seq", static_cast<int64_t>(seq));
            batch.push_back(std::move(e));
            ++seq;
        }
        state.ResumeTiming();

        for (auto& entity : batch)
            idx.put("evt", entity);
    }

    state.SetItemsProcessed(state.iterations() * batchSize);
    state.SetLabel("batch ingest, " + std::to_string(batchSize) + " rec/batch");

    db->close();
    std::error_code ec;
    fs::remove_all(dbPath, ec);
}
BENCHMARK(BM_W5A_BatchIngest_Sustained)
    ->Arg(50)
    ->Arg(100)
    ->Arg(500)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();
