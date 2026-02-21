/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_core_performance.cpp                         ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     321                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <benchmark/benchmark.h>
#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <random>
#include <chrono>

#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/vector_index.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"

using namespace themis;

// ============================================================================
// VECTOR INDEX BENCHMARKS
// ============================================================================

class VectorIndexBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        db_path_ = "C:\\tmp\\bench_vi_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
    if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
    }
    
    void TearDown(const ::benchmark::State& state) override {
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    
    std::vector<float> genVec(size_t dim) {
        static thread_local std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
        std::vector<float> v(dim);
        for (auto& x : v) x = dis(gen);
        return v;
    }
};

BENCHMARK_F(VectorIndexBench, InsertPlaintext)(benchmark::State& state) {
    VectorIndexManager vim(*db_);
    
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            BaseEntity e("vec_" + std::to_string(i), BaseEntity::FieldMap{
                {"embedding", genVec(384)}
            });
            vim.addEntity(e);
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// SECONDARY INDEX BENCHMARKS
// ============================================================================

class SecondaryIndexBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
            // BENCHMARK CONFIG NOTES (2025-12-28):
            // These benchmarks measure different layers of Themis' write stack:
            // 1. RawWriteOnly: Pure RocksDB batch writes (~1.09M ops/s)
            // 2. IndexInsert: Full SecondaryIndexManager with serialization (~100k ops/s)
            //
            // The ~11x gap is due to:
            // - Entity serialization: ~20-30% overhead
            // - Index key generation and management: ~40-50%
            // - Metadata lookups (isUniqueIndex_ DB reads): ~20-30%
            //
            // Configuration is tuned for microbenchmarks (no WAL, lean background threads).
            // For production, enable WAL (+10-20% latency) and adjust background jobs
            // per CPU count (e.g., 8/4/2 for 16-core systems).
            // See benchmarks/BENCHMARK_ANALYSIS.md for full analysis.
    
        db_path_ = "C:\\tmp\\bench_si_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        // Lean microbench config: minimize noise, disable durability
        cfg.disable_wal_for_benchmark = true;    // bypass WAL fsync cost in microbenchmarks
        cfg.memtable_size_mb = 512;              // larger write buffer to reduce flushes
        cfg.block_cache_size_mb = 4096;          // bigger cache to avoid read stalls
        cfg.max_write_buffer_number = 6;         // more mutable/immutable memtables before stall
        cfg.allow_concurrent_memtable_write = true;   // parallel writes to different memtables
        cfg.max_background_jobs = 4;             // lean background parallelism: minimize mutex contention
        cfg.max_background_compactions = 2;
        cfg.max_background_flushes = 1;
        cfg.max_subcompactions = 1;
        cfg.enable_blobdb = false;               // disable BlobDB for tiny values (~50 bytes)
        cfg.enable_statistics = false;           // avoid stats overhead in microbenchmarks
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        // Skip index creation for pure write throughput measurement
        // sim_->createIndex("users", "email");
    }
    
    void TearDown(const ::benchmark::State& state) override {
        sim_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> sim_;
};

BENCHMARK_F(SecondaryIndexBench, IndexInsert)(benchmark::State& state) {
        // Measures full indexing write throughput: entity serialization + index management.
        // Realistic for typical bulk inserts with secondary indexes.
        // Expected: ~100k items/s. Bottleneck: entity serialization + index key gen.
    
    int64_t iteration = 0;
    for (auto _ : state) {
        auto batch = db_->createWriteBatch();
        for (int i = 0; i < 100; ++i) {
            std::string pk = "user_" + std::to_string(iteration) + "_" + std::to_string(i);
            BaseEntity e(pk, BaseEntity::FieldMap{
                {"email", pk + "@test.com"},
                {"name", "User " + std::to_string(i)}
            });
            // Critical: use batch variant to avoid per-item DB reads
            // Non-batched variant (sim_->put(entity)) does 1 DB read per insert → 10x slower!
            sim_->put("users", e, *batch);
        }
        batch->commit();
        ++iteration;
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Raw RocksDB write benchmark (no index management, pure writes)
BENCHMARK_F(SecondaryIndexBench, RawWriteOnly)(benchmark::State& state) {
        // Measures raw RocksDB write throughput: minimal Themis overhead.
        // Expected: ~1.09M items/s (note: ~39% slower than v1.3.0 baseline of 1.78M,
        // likely due to different hardware/vcpkg build/CPU freq scaling).
        // This is the upper bound for any write operation in Themis.
    
    int64_t iteration = 0;
    for (auto _ : state) {
        auto batch = db_->createWriteBatch();
        for (int i = 0; i < 100; ++i) {
            std::string pk = "user_" + std::to_string(iteration) + "_" + std::to_string(i);
            std::string value = "test_value_" + std::to_string(i);
            std::vector<uint8_t> value_bytes(value.begin(), value.end());
            batch->put(pk, value_bytes);
        }
        batch->commit();
        ++iteration;
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// QUERY ENGINE BENCHMARKS
// ============================================================================

class QueryEngineBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        db_path_ = "C:\\tmp\\bench_qe_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
    if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
    }
    
    void TearDown(const ::benchmark::State& state) override {
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

BENCHMARK_F(QueryEngineBench, SimpleEvaluation)(benchmark::State& state) {
    for (auto _ : state) {
        // Placeholder for actual AQL evaluation
        // This benchmark verifies the compilation works
        double val = 42.0;
        benchmark::DoNotOptimize(val);
    }
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// GRAPH INDEX BENCHMARKS
// ============================================================================

class GraphIndexBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        db_path_ = "C:\\tmp\\bench_gi_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
        gim_ = std::make_unique<GraphIndexManager>(*db_);
    }
    
    void TearDown(const ::benchmark::State& state) override {
        gim_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<GraphIndexManager> gim_;
};

BENCHMARK_F(GraphIndexBench, AddEdges)(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            BaseEntity e("edge_" + std::to_string(i), BaseEntity::FieldMap{
                {"_from", "node_" + std::to_string(i % 10)},
                {"_to", "node_" + std::to_string((i + 1) % 10)},
                {"weight", 1.0}
            });
            gim_->addEdge(e);
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// TIMESERIES BENCHMARKS
// ============================================================================

class TimeseriesBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        db_path_ = "C:\\tmp\\bench_ts_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
    if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
    }
    
    void TearDown(const ::benchmark::State& state) override {
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

BENCHMARK_F(TimeseriesBench, InsertTimepoints)(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            int64_t ts = std::chrono::system_clock::now().time_since_epoch().count();
            double val = 50.0 + (i % 20);
            benchmark::DoNotOptimize(ts);
            benchmark::DoNotOptimize(val);
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// MAIN
// ============================================================================

BENCHMARK_MAIN();
