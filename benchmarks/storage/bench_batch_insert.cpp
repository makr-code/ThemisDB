// ThemisDB v1.3.4 Batch Insert Performance Benchmark
// Demonstrates the massive performance improvement from batching inserts
// Expected: 10-100x faster due to single commit overhead

#include <benchmark/benchmark.h>
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include <filesystem>
#include <chrono>
#include <vector>

using namespace themis;

class BatchInsertBenchmark : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        // Unique path under the OS temp directory prevents collisions between
        // concurrent or repeated benchmark runs.
        const auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
        db_path_ = (std::filesystem::temp_directory_path() /
                    ("themis_bench_batch_insert_" + std::to_string(ts))).string();

        // Clean up any leftover from a previous abnormal exit
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
        
        // Configure and open database
        RocksDBWrapper::Config config;
        config.db_path = db_path_;
        config.compression_default = "lz4";
        config.block_cache_size_mb = 256;
        config.disable_wal_for_benchmark = false;  // Keep WAL enabled for TransactionDB
        config.memtable_size_mb = 512;
        config.max_write_buffer_number = 6;
        config.allow_concurrent_memtable_write = true;
        config.enable_statistics = false;
        config.enable_blobdb = false;
        
        db = std::make_unique<RocksDBWrapper>(config);
        if (!db->open()) {
            throw std::runtime_error("Failed to open database");
        }
        
        indexMgr = std::make_unique<SecondaryIndexManager>(*db);
        
        // Create a few indexes to simulate real workload
        indexMgr->createIndex("users", "email", false);
        indexMgr->createIndex("users", "username", true); // unique
        indexMgr->createRangeIndex("users", "created_at");
    }
    
    void TearDown(const ::benchmark::State& state) override {
        indexMgr.reset();
        db->close();
        db.reset();
        
        // Cleanup
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
    }
    
    BaseEntity createTestEntity(int id) {
        BaseEntity entity("user_" + std::to_string(id));
        entity.setField("email", "user" + std::to_string(id) + "@test.com");
        entity.setField("username", "username_" + std::to_string(id));
        entity.setField("created_at", std::to_string(1000000 + id));
        entity.setField("age", std::to_string(20 + (id % 50)));
        entity.setField("status", id % 2 == 0 ? "active" : "inactive");
        return entity;
    }
    
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<SecondaryIndexManager> indexMgr;
};

// Baseline: Individual inserts (current approach)
BENCHMARK_F(BatchInsertBenchmark, SingleInserts_100)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        // Clean table for each iteration
        for (int i = 0; i < 100; ++i) {
            indexMgr->erase("users", "user_" + std::to_string(i));
        }
        state.ResumeTiming();
        
        // Insert 100 entities one by one
        for (int i = 0; i < 100; ++i) {
            auto entity = createTestEntity(i);
            auto status = indexMgr->put("users", entity);
            if (!status.ok) {
                state.SkipWithError(("Insert failed: " + status.message).c_str());
                return;
            }
        }
        
        state.counters["items_per_second"] = benchmark::Counter(
            100, benchmark::Counter::kIsRate);
    }
}

// v1.3.4: Batch Insert API
BENCHMARK_F(BatchInsertBenchmark, BatchInsert_100)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        // Clean table for each iteration
        for (int i = 0; i < 100; ++i) {
            indexMgr->erase("users", "user_" + std::to_string(i));
        }
        
        // Prepare batch
        std::vector<BaseEntity> batch;
        batch.reserve(100);
        for (int i = 0; i < 100; ++i) {
            batch.push_back(createTestEntity(i));
        }
        state.ResumeTiming();
        
        // Single batch insert
        auto status = indexMgr->putBatch("users", batch);
        if (!status.ok) {
            state.SkipWithError(("Batch insert failed: " + status.message).c_str());
            return;
        }
        
        state.counters["items_per_second"] = benchmark::Counter(
            100, benchmark::Counter::kIsRate);
    }
}

// Larger batch: 1000 items
BENCHMARK_F(BatchInsertBenchmark, SingleInserts_1000)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        for (int i = 0; i < 1000; ++i) {
            indexMgr->erase("users", "user_" + std::to_string(i));
        }
        state.ResumeTiming();
        
        for (int i = 0; i < 1000; ++i) {
            auto entity = createTestEntity(i);
            auto status = indexMgr->put("users", entity);
            if (!status.ok) {
                state.SkipWithError(("Insert failed: " + status.message).c_str());
                return;
            }
        }
        
        state.counters["items_per_second"] = benchmark::Counter(
            1000, benchmark::Counter::kIsRate);
    }
}

BENCHMARK_F(BatchInsertBenchmark, BatchInsert_1000)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        for (int i = 0; i < 1000; ++i) {
            indexMgr->erase("users", "user_" + std::to_string(i));
        }
        
        std::vector<BaseEntity> batch;
        batch.reserve(1000);
        for (int i = 0; i < 1000; ++i) {
            batch.push_back(createTestEntity(i));
        }
        state.ResumeTiming();
        
        auto status = indexMgr->putBatch("users", batch);
        if (!status.ok) {
            state.SkipWithError(("Batch insert failed: " + status.message).c_str());
            return;
        }
        
        state.counters["items_per_second"] = benchmark::Counter(
            1000, benchmark::Counter::kIsRate);
    }
}

BENCHMARK_MAIN();
