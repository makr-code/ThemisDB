// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w7a_release_critical_signoff.cpp
 * @brief Wave 7-A: Final Release-Critical Workload Sign-Off Benchmarks.
 *
 * Purpose: Provide reproducible, hardened measurements for the release sign-off
 * decision on the most business-critical paths. Each benchmark fixture uses
 * canonical warmup/iteration counts, deterministic seeding (kW7CanonicalSeed),
 * and standardised dataset sizes. Results feed directly into the W7 release
 * gate manifest (release_gate_manifest_w7.json).
 *
 * Covered workload families (per release-risk triage):
 *   RCS-01  Single-key read (point lookup) – p99 gate
 *   RCS-02  Single-key write (upsert) – throughput gate
 *   RCS-03  Range scan – p99 gate
 *   RCS-04  Batch write – p99 gate
 *   RCS-05  Mixed OLTP read/write (60/40) – combined gate
 *   RCS-06  Vector similarity search (ANN) – p99 gate
 *   RCS-07  Graph neighbourhood traversal – p99 gate
 *   RCS-08  Secondary-index lookup – p99 gate
 *
 * All benchmarks:
 *   - Warm up for kWarmupIterations before measurement begins.
 *   - Use UseRealTime() so I/O wait is included in reported time.
 *   - Seed the PRNG from kW7CanonicalSeed for deterministic key selection.
 *   - Run with Repetitions(kRepetitions) to capture run-to-run variance.
 *
 * Hard gates (evaluated by release_gate_manifest_w7.json):
 *   - RCS-01 p99 ≤ 200 µs
 *   - RCS-02 throughput ≥ 80 000 ops/s
 *   - RCS-03 p99 ≤ 500 µs
 *   - RCS-04 p99 ≤ 5 ms
 *
 * @note This file is intentionally self-contained to keep sign-off runs
 *       reproducible without additional infrastructure dependencies.
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "index/graph_index.h"
#include "index/secondary_index.h"
#include "index/vector_index.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"

namespace fs = std::filesystem;

namespace themis {
namespace bench {
namespace w7a {

// ---------------------------------------------------------------------------
// Constants – deterministic, release-pinned
// ---------------------------------------------------------------------------

/// Canonical PRNG seed shared by all W7 benchmarks.
static constexpr uint64_t kW7CanonicalSeed = 42;

/// Warmup iterations applied before every measurement window.
static constexpr int kWarmupIterations = 500;

/// Repetitions per benchmark for variance estimation.
static constexpr int kRepetitions = 5;

/// Default dataset record count (pre-loaded into DB during SetUp).
static constexpr int kDefaultRecordCount = 50'000;

/// Vector embedding dimension.
static constexpr std::size_t kVecDim = 128;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

void RemoveAll(const std::string& path) {
    std::error_code ec = {};
    fs::remove_all(path, ec);
}

std::string UniqueDbPath(const std::string& tag) {
    using namespace std::chrono;
    auto ts = duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
    return fs::temp_directory_path().string() + "/w7a_" + tag + "_" + std::to_string(ts);
}

RocksDBWrapper::Config DefaultConfig(const std::string& db_path) {
    RocksDBWrapper::Config cfg;
    cfg.db_path                     = db_path;
    cfg.compression_default         = "lz4";
    cfg.compression_bottommost      = "zstd";
    cfg.block_cache_size_mb         = 256;
    cfg.memtable_size_mb            = 128;
    cfg.max_write_buffer_number     = 4;
    cfg.allow_concurrent_memtable_write = true;
    cfg.enable_statistics           = false;
    return cfg;
}

/// Seeded, reproducible key generator for W7 workloads.
class KeyGenerator {
public:
    explicit KeyGenerator(uint64_t seed = kW7CanonicalSeed) : rng_(seed) {}

    std::string NextKey(int upper_bound) {
        std::uniform_int_distribution<int> dist(0, upper_bound - 1);
        return "entity_" + std::to_string(dist(rng_));
    }

    std::vector<float> NextVec(std::size_t dim) {
        std::normal_distribution<float> nd(0.0f, 1.0f);
        std::vector<float> v(dim);
        for (auto& x : v) {
          x = nd(rng_);
        }
        // L2-normalize
        float norm = 0.0f;
        for (float x : v) {
          norm += x * x;
        }
        norm = std::sqrt(norm + 1e-12f);
        for (auto& x : v) {
          x /= norm;
        }
        return v;
    }

private:
    std::mt19937_64 rng_;
};

} // namespace

// ---------------------------------------------------------------------------
// RCS-01 / RCS-02 / RCS-05: CRUD workloads
// ---------------------------------------------------------------------------

/**
 * @brief Fixture for CRUD sign-off benchmarks (point read, upsert, mixed).
 *
 * Uses the live RocksDB backend and canonical BaseEntity serialization so the
 * benchmark reflects the current production storage API instead of a stale
 * mock interface.
 */
class CRUDSignoffFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("crud");
        RemoveAll(db_path_);

        db_ = std::make_unique<RocksDBWrapper>(DefaultConfig(db_path_));
        if (!db_->open()) {
            throw std::runtime_error("W7A: failed to open RocksDB for CRUD sign-off");
        }

        for (int i = 0; i < kDefaultRecordCount; ++i) {
            BaseEntity e("entity_" + std::to_string(i));
            e.setField("idx", std::to_string(i));
            e.setField("status", (i % 3 == 0) ? "active" : "inactive");
            e.setField("score", std::to_string((i * 7) % 1000));
            db_->put(e.getPrimaryKey(), e.serialize());
        }

        KeyGenerator warmup_kg(kW7CanonicalSeed + 1);
        for (int i = 0; i < kWarmupIterations; ++i) {
            std::string key = warmup_kg.NextKey(kDefaultRecordCount);
            std::string val = {};
            db_->get(key, val);
        }
    }

    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
    }

protected:
    std::string db_path_ = {};
    std::unique_ptr<RocksDBWrapper> db_;
};

/// RCS-01: Point read p99 gate (≤ 200 µs).
BENCHMARK_F(CRUDSignoffFixture, RCS01_PointRead)(benchmark::State& state) {
    KeyGenerator kg(kW7CanonicalSeed);
    for (auto _ : state) {
        const std::string key = kg.NextKey(kDefaultRecordCount);
        std::string val = {};
        benchmark::DoNotOptimize(db_->get(key, val));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(CRUDSignoffFixture, RCS01_PointRead)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W7A/RCS01_PointRead_p99_gate_200us");

/// RCS-02: Upsert throughput gate (≥ 80 000 ops/s).
BENCHMARK_F(CRUDSignoffFixture, RCS02_UpsertThroughput)(benchmark::State& state) {
    KeyGenerator kg(kW7CanonicalSeed + 2);
    int counter = 0;
    for (auto _ : state) {
        const std::string key = kg.NextKey(kDefaultRecordCount * 2);
        db_->put(key, "val_" + std::to_string(counter++));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(CRUDSignoffFixture, RCS02_UpsertThroughput)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W7A/RCS02_UpsertThroughput_gate_80k_ops_s");

/// RCS-05: Mixed OLTP 60% read / 40% write.
BENCHMARK_F(CRUDSignoffFixture, RCS05_MixedOLTP)(benchmark::State& state) {
    KeyGenerator kg(kW7CanonicalSeed + 5);
    std::mt19937_64 mix_rng(kW7CanonicalSeed + 55);
    std::uniform_int_distribution<int> mix(0, 99);
    int write_ctr = 0;
    for (auto _ : state) {
        if (mix(mix_rng) < 40) {
            const std::string key = kg.NextKey(kDefaultRecordCount * 2);
            db_->put(key, "w_" + std::to_string(write_ctr++));
        } else {
            const std::string key = kg.NextKey(kDefaultRecordCount);
            std::string val = {};
            benchmark::DoNotOptimize(db_->get(key, val));
        }
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(CRUDSignoffFixture, RCS05_MixedOLTP)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W7A/RCS05_MixedOLTP_60r40w");

// ---------------------------------------------------------------------------
// RCS-03 / RCS-04: Range scan and batch write
// ---------------------------------------------------------------------------

class RangeBatchSignoffFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("range");
        RemoveAll(db_path_);

        db_ = std::make_unique<RocksDBWrapper>(DefaultConfig(db_path_));
        if (!db_->open()) {
            throw std::runtime_error("W7A: failed to open RocksDB for range/batch sign-off");
        }

        idx_ = std::make_unique<SecondaryIndexManager>(*db_);
        idx_->createRangeIndex("Item", "score");

        for (int i = 0; i < kDefaultRecordCount; ++i) {
            BaseEntity e("item_" + std::to_string(i));
            e.setField("score", std::to_string(i % 10000));
            db_->put(e.getPrimaryKey(), e.serialize());
            idx_->put("Item", e);
        }

        KeyGenerator wkg(kW7CanonicalSeed + 10);
        for (int i = 0; i < kWarmupIterations; ++i) {
            std::string val = {};
            db_->get(wkg.NextKey(kDefaultRecordCount), val);
        }
    }

    void TearDown(const ::benchmark::State&) override {
        idx_.reset();
        db_.reset();
        RemoveAll(db_path_);
    }

protected:
    std::string db_path_ = {};
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> idx_;
};

/// RCS-03: Range scan p99 gate (≤ 500 µs, window 100 records).
BENCHMARK_F(RangeBatchSignoffFixture, RCS03_RangeScan)(benchmark::State& state) {
    std::mt19937_64 rng(kW7CanonicalSeed + 3);
    std::uniform_int_distribution<int> dist(0, kDefaultRecordCount - 100);
    for (auto _ : state) {
        const int lo = dist(rng);
        const auto [status, results] = idx_->scanKeysRange(
            "Item",
            "score",
            std::optional<std::string>(std::to_string(lo)),
            std::optional<std::string>(std::to_string(lo + 100)),
            true,
            true,
            100,
            false);
        benchmark::DoNotOptimize(status);
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(RangeBatchSignoffFixture, RCS03_RangeScan)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W7A/RCS03_RangeScan_p99_gate_500us");

/// RCS-04: Batch write p99 gate (≤ 5 ms, batch size 500).
BENCHMARK_F(RangeBatchSignoffFixture, RCS04_BatchWrite)(benchmark::State& state) {
    KeyGenerator kg(kW7CanonicalSeed + 4);
    int counter = 0;
    for (auto _ : state) {
        std::vector<RocksDBWrapper::KeyValuePair> batch;
        batch.reserve(500);
        for (int i = 0; i < 500; ++i) {
            const std::string key = kg.NextKey(kDefaultRecordCount * 2);
            const std::string payload = "bv_" + std::to_string(counter++);
            RocksDBWrapper::KeyValuePair kv{};
            kv.key = key;
            kv.value.assign(payload.begin(), payload.end());
            batch.push_back(kv);
        }
        db_->putBatch(batch);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * 500);
}
BENCHMARK_REGISTER_F(RangeBatchSignoffFixture, RCS04_BatchWrite)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W7A/RCS04_BatchWrite_p99_gate_5ms");

// ---------------------------------------------------------------------------
// RCS-06: Vector ANN sign-off
// ---------------------------------------------------------------------------

class VectorSignoffFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("vec");
        RemoveAll(db_path_);

        db_ = std::make_unique<RocksDBWrapper>(DefaultConfig(db_path_));
        if (!db_->open()) {
            throw std::runtime_error("W7A: failed to open RocksDB for vector sign-off");
        }

        vidx_ = std::make_unique<VectorIndexManager>(*db_);
        if (!vidx_->init("w7a_vec", static_cast<int>(kVecDim), VectorIndexManager::Metric::COSINE).ok) {
            throw std::runtime_error("W7A: failed to initialize vector index");
        }

        KeyGenerator kg(kW7CanonicalSeed + 6);
        for (int i = 0; i < 10'000; ++i) {
            BaseEntity e("vec_" + std::to_string(i));
            e.setField("embedding", kg.NextVec(kVecDim));
            if (!vidx_->addEntity(e).ok) {
                throw std::runtime_error("W7A: failed to add vector entity");
            }
        }

        for (int i = 0; i < kWarmupIterations; ++i) {
            auto q = kg.NextVec(kVecDim);
            auto [status, results] = vidx_->searchKnn(q, 10);
            benchmark::DoNotOptimize(status);
            benchmark::DoNotOptimize(results);
        }
    }

    void TearDown(const ::benchmark::State&) override {
        vidx_.reset();
        db_.reset();
        RemoveAll(db_path_);
    }

protected:
    std::string db_path_ = {};
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<VectorIndexManager> vidx_;
};

/// RCS-06: ANN top-10 search p99 gate (≤ 200 µs).
BENCHMARK_F(VectorSignoffFixture, RCS06_AnnSearch)(benchmark::State& state) {
    KeyGenerator kg(kW7CanonicalSeed + 60);
    for (auto _ : state) {
        auto q = kg.NextVec(kVecDim);
        auto [status, results] = vidx_->searchKnn(q, 10);
        benchmark::DoNotOptimize(status);
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(VectorSignoffFixture, RCS06_AnnSearch)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W7A/RCS06_AnnSearch_p99_gate_200us");

// ---------------------------------------------------------------------------
// RCS-07: Graph traversal sign-off
// ---------------------------------------------------------------------------

class GraphSignoffFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("graph");
        RemoveAll(db_path_);

        db_ = std::make_unique<RocksDBWrapper>(DefaultConfig(db_path_));
        if (!db_->open()) {
            throw std::runtime_error("W7A: failed to open RocksDB for graph sign-off");
        }

        gidx_ = std::make_unique<GraphIndexManager>(*db_);
        constexpr int kNodes = 5'000;
        constexpr int kEdges = 4;
        std::mt19937_64 rng(kW7CanonicalSeed + 7);
        std::uniform_int_distribution<int> nd(0, kNodes - 1);

        for (int i = 0; i < kNodes; ++i) {
            BaseEntity node("n" + std::to_string(i));
            node.setField("kind", "node");
            db_->put(node.getPrimaryKey(), node.serialize());
        }

        for (int i = 0; i < kNodes * kEdges; ++i) {
            const std::string from = "n" + std::to_string(nd(rng));
            const std::string to = "n" + std::to_string(nd(rng));
            BaseEntity edge("edge_" + std::to_string(i));
            edge.setField("id", edge.getPrimaryKey());
            edge.setField("_from", from);
            edge.setField("_to", to);
            if (!gidx_->addEdge(edge).ok) {
                throw std::runtime_error("W7A: failed to add graph edge");
            }
        }

        for (int i = 0; i < kWarmupIterations; ++i) {
            const std::string node_id = "n" + std::to_string(i % kNodes);
            auto [status, neighbors] = gidx_->outNeighbors(node_id);
            benchmark::DoNotOptimize(status);
            benchmark::DoNotOptimize(neighbors);
        }
    }

    void TearDown(const ::benchmark::State&) override {
        gidx_.reset();
        db_.reset();
        RemoveAll(db_path_);
    }

protected:
    std::string db_path_ = {};
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<GraphIndexManager> gidx_;
};

/// RCS-07: Graph neighbourhood lookup p99 gate (≤ 500 µs).
BENCHMARK_F(GraphSignoffFixture, RCS07_GraphNeighbourhood)(benchmark::State& state) {
    constexpr int kNodes = 5'000;
    std::mt19937_64 rng(kW7CanonicalSeed + 70);
    std::uniform_int_distribution<int> nd(0, kNodes - 1);
    for (auto _ : state) {
        const std::string node_id = "n" + std::to_string(nd(rng));
        auto [status, neighbors] = gidx_->outNeighbors(node_id);
        benchmark::DoNotOptimize(status);
        benchmark::DoNotOptimize(neighbors);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(GraphSignoffFixture, RCS07_GraphNeighbourhood)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W7A/RCS07_GraphNeighbourhood_p99_gate_500us");

// ---------------------------------------------------------------------------
// RCS-08: Secondary-index lookup sign-off
// ---------------------------------------------------------------------------

class SecondaryIndexSignoffFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("idx");
        RemoveAll(db_path_);

        db_ = std::make_unique<RocksDBWrapper>(DefaultConfig(db_path_));
        if (!db_->open()) {
            throw std::runtime_error("W7A: failed to open RocksDB for index sign-off");
        }

        idx_ = std::make_unique<SecondaryIndexManager>(*db_);
        idx_->createIndex("User", "email", true);
        idx_->createRangeIndex("User", "age");

        for (int i = 0; i < kDefaultRecordCount; ++i) {
            BaseEntity e("user_" + std::to_string(i));
            e.setField("email", "u" + std::to_string(i) + "@test.com");
            e.setField("age", std::to_string(18 + (i % 60)));
            db_->put(e.getPrimaryKey(), e.serialize());
            idx_->put("User", e);
        }

        for (int i = 0; i < kWarmupIterations; ++i) {
            const int id = i % kDefaultRecordCount;
            auto [status, keys] = idx_->scanKeysEqual("User", "email", "u" + std::to_string(id) + "@test.com");
            benchmark::DoNotOptimize(status);
            benchmark::DoNotOptimize(keys);
        }
    }

    void TearDown(const ::benchmark::State&) override {
        idx_.reset();
        db_.reset();
        RemoveAll(db_path_);
    }

protected:
    std::string db_path_ = {};
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> idx_;
};

/// RCS-08: Unique secondary-index lookup p99 gate (≤ 200 µs).
BENCHMARK_F(SecondaryIndexSignoffFixture, RCS08_SecondaryIndexLookup)(benchmark::State& state) {
    std::mt19937_64 rng(kW7CanonicalSeed + 8);
    std::uniform_int_distribution<int> dist(0, kDefaultRecordCount - 1);
    for (auto _ : state) {
        const int id = dist(rng);
        auto [status, keys] = idx_->scanKeysEqual("User", "email", "u" + std::to_string(id) + "@test.com");
        benchmark::DoNotOptimize(status);
        benchmark::DoNotOptimize(keys);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(SecondaryIndexSignoffFixture, RCS08_SecondaryIndexLookup)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W7A/RCS08_SecondaryIndexLookup_p99_gate_200us");

} // namespace w7a
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
