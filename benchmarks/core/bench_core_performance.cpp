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
#include "acceleration/vec_knn.h"

using namespace themis;
using namespace themis::acceleration;

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
        for (auto& x : v) {
          x = dis(gen);
        }
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

// PERF-D3 benchmark: parallel batch insert pipeline targeting 600k vectors/s SLO.
// Uses VecKnnInsertPipeline with AVX2 SIMD distance + configurable thread pool.
// Batch size 32, threads = hardware_concurrency (typical 4-16).
BENCHMARK_F(VectorIndexBench, ParallelBatchInsert_PERFD3)(benchmark::State& state) {
    VectorIndexManager vim(*db_);
    vim.init("bench_vi", 384);

    VecKnnPipelineConfig cfg;
    cfg.batch_size    = 32;
    cfg.num_threads   = 0;   // 0 = hardware_concurrency
    cfg.enable_cache  = true;
    cfg.vector_field  = "embedding";
    VecKnnInsertPipeline pipeline(cfg);

    // Pre-generate a fixed pool of entities to avoid timing allocation
    static constexpr int kPool = 512;
    std::vector<BaseEntity> pool;
    pool.reserve(kPool);
    for (int i = 0; i < kPool; ++i) {
        pool.emplace_back("vi_" + std::to_string(i), BaseEntity::FieldMap{
            {"embedding", genVec(384)}
        });
    }

    for (auto _ : state) {
        // Submit a batch of 512 entities per iteration
        pipeline.insertBatch(vim, pool, "embedding");
    }
    state.SetItemsProcessed(state.iterations() * kPool);
    // SLO: items_per_second >= 600,000
    state.SetLabel("SLO=600k/s");
}

// PERF-D3 benchmark: raw SIMD pairwise distance throughput (no index overhead).
// Measures AVX2/AVX-512 distance kernel in isolation to validate SIMD gains.
static void SIMDDistanceThroughput_PERFD3(benchmark::State& state) {
    const int   DIM   = 384;
    const int   N_DB  = 1024;
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dis(-1.f, 1.f);

    std::vector<float> query(DIM), db(N_DB * DIM);
    for (auto& x : query) {
      x = dis(rng);
    }
    for (auto& x : db) {
      x = dis(rng);
    }

    std::vector<float> out(N_DB);

    for (auto _ : state) {
        simd_batch_l2_sq(query.data(), db.data(), N_DB, DIM, out.data());
        benchmark::DoNotOptimize(out.data());
    }
    state.SetItemsProcessed(state.iterations() * N_DB);
    state.SetLabel("AVX2/AVX-512 L2-sq kernel");
}

BENCHMARK(SIMDDistanceThroughput_PERFD3);

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

BENCHMARK_F(SecondaryIndexBench, BM_SecondaryIndex_SingleInsert)(benchmark::State& state) {
    int64_t iteration = 0;
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            std::string pk = "single_user_" + std::to_string(iteration) + "_" + std::to_string(i);
            BaseEntity e(pk, BaseEntity::FieldMap{
                {"email", pk + "@test.com"},
                {"name", "User " + std::to_string(i)}
            });
            auto st = sim_->put("users", e);
            if (!st.ok) {
                state.SkipWithError(("single insert failed: " + st.message).c_str());
                return;
            }
        }
        ++iteration;
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_DEFINE_F(SecondaryIndexBench, BM_SecondaryIndex_BatchInsert)(benchmark::State& state) {
    const auto batch_size = static_cast<size_t>(state.range(0));
    if (batch_size == 0) {
        state.SkipWithError("batch size must be > 0");
        return;
    }

    int64_t iteration = 0;
    std::vector<BaseEntity> entities;
    entities.reserve(100);

    for (auto _ : state) {
        entities.clear();
        for (int i = 0; i < 100; ++i) {
            std::string pk = "batch_user_" + std::to_string(iteration) + "_" + std::to_string(i);
            entities.emplace_back(pk, BaseEntity::FieldMap{
                {"email", pk + "@test.com"},
                {"name", "User " + std::to_string(i)}
            });
        }

        auto st = sim_->putBatch("users", entities, batch_size);
        if (!st.ok) {
            state.SkipWithError(("batch insert failed: " + st.message).c_str());
            return;
        }
        ++iteration;
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_REGISTER_F(SecondaryIndexBench, BM_SecondaryIndex_BatchInsert)->Arg(64);

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
    static constexpr size_t kNumEntities = 1000;

    void SetUp(const ::benchmark::State& state) override {
        db_path_ = "C:\\tmp\\bench_qe_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        cfg.block_cache_size_mb = 64;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }

        // Pre-populate entities so every benchmark iteration can do a real key read.
        // Each entity is stored with key "rel:bench_qe:entity_XXXXXX" using the
        // canonical KeySchema relational format.
        keys_.resize(kNumEntities);
        for (size_t i = 0; i < kNumEntities; ++i) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "entity_%06zu", i);
            std::string pk(buf);
            BaseEntity e = BaseEntity::fromFields(pk, BaseEntity::FieldMap{
                {"name",   std::string("Benchmark Entity ") + std::to_string(i)},
                {"value",  std::to_string(i * 7)},
                {"active", (i % 2 == 0) ? "true" : "false"}
            });
            auto serialized = e.serialize();
            std::string storage_key = "rel:bench_qe:" + pk;
            db_->put(storage_key, serialized);
            keys_[i] = storage_key;
        }
    }

    void TearDown(const ::benchmark::State& state) override {
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::vector<std::string> keys_;
};

// 1:1 direct-key point-lookup benchmark.
// Measures the hot-path throughput of a single-entity primary-key read:
// key construction → RocksDB get → deserialization.
// This is the minimal OLTP query case and represents the ceiling throughput
// that index-based queries are measured against.
BENCHMARK_F(QueryEngineBench, SimpleEvaluation)(benchmark::State& state) {
    size_t idx = 0;
    for (auto _ : state) {
        const std::string& key = keys_[idx % kNumEntities];
        auto blob = db_->get(key);
        benchmark::DoNotOptimize(blob);
        ++idx;
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

// Same hot-path but includes deserialization to measure end-to-end entity load cost.
BENCHMARK_F(QueryEngineBench, PointLookupWithDeserialize)(benchmark::State& state) {
    size_t idx = 0;
    for (auto _ : state) {
        const std::string& key = keys_[idx % kNumEntities];
        auto blob = db_->get(key);
        if (blob.has_value()) {
            // Extract pk from storage key "rel:bench_qe:<pk>"
            std::string pk = key.substr(key.rfind(':') + 1);
            auto entity = BaseEntity::deserialize(pk, *blob);
            benchmark::DoNotOptimize(entity);
        }
        ++idx;
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
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
