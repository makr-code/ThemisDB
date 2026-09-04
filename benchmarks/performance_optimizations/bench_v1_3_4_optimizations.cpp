#include <benchmark/benchmark.h>
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include <random>
#include <filesystem>

namespace {
    std::string makeRandomString(size_t len) {
        static const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
        static std::mt19937 rng{std::random_device{}()};
        static std::uniform_int_distribution<size_t> dist(0, sizeof(charset) - 2);
        std::string s = {};
        s.reserve(len);
        for (size_t i = 0; i < len; ++i) {
          s += charset[dist(rng)];
        }
        return s;
    }

    void cleanupTestDB(const std::string& path) {
        std::error_code ec = {};
        std::filesystem::remove_all(path, ec);
    }
}

class OptimizedInsertBenchmark : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = "bench_optimized_db";
        cleanupTestDB(db_path_);

        themis::RocksDBWrapper::Config config;
        config.db_path = db_path_;
        config.compression_default = "lz4";
        config.block_cache_size_mb = 256;
        config.disable_wal_for_benchmark = true;
        config.memtable_size_mb = 512;
        config.max_write_buffer_number = 6;
        config.allow_concurrent_memtable_write = true;
        config.enable_statistics = false;
        config.enable_blobdb = false;

        db_ = std::make_unique<themis::RocksDBWrapper>(config);
        if (!db_->open()) {
            throw std::runtime_error("Failed to open DB");
        }
        secondary_ = std::make_unique<themis::SecondaryIndexManager>(*db_);

        // Erstelle alle 6 Indexe (wie InsertWithAllIndexes)
        secondary_->createIndex("Person", "email", true);
        secondary_->createRangeIndex("Person", "age");
        secondary_->createSparseIndex("Person", "nickname", false);
        secondary_->createGeoIndex("Person", "location");
        secondary_->createTTLIndex("Person", "expires_at", 3600);
        secondary_->createFulltextIndex("Person", "bio");

        // Warmup
        for (size_t i = 0; i < 100; ++i) {
            themis::BaseEntity entity("warmup_" + std::to_string(i));
            entity.setField("email", makeRandomString(20));
            entity.setField("age", static_cast<int64_t>(25 + (i % 50)));
            entity.setField("nickname", i % 3 == 0 ? themis::Value{} : themis::Value{makeRandomString(8)});
            entity.setField("bio", makeRandomString(200));
            entity.setField("location_lat", 51.5 + (i * 0.1));
            entity.setField("location_lon", -0.1 + (i * 0.1));
            entity.setField("expires_at", std::to_string(std::time(nullptr) + 3600));
            secondary_->put("Person", entity);
        }

        counter_ = 100;
    }

    void TearDown(const ::benchmark::State&) override {
        secondary_.reset();
        db_.reset();
        cleanupTestDB(db_path_);
    }

protected:
    std::string db_path_ = {};
    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::SecondaryIndexManager> secondary_;
    size_t counter_ = {};
};

// --- Baseline: v1.3.3 (ohne Cache) ---
BENCHMARK_DEFINE_F(OptimizedInsertBenchmark, V133_SingleInsert)(benchmark::State& state) {
    for (auto _ : state) {
        themis::BaseEntity entity("v133_" + std::to_string(counter_++));
        entity.setField("email", makeRandomString(20));
        entity.setField("age", static_cast<int64_t>(25 + (counter_ % 50)));
        entity.setField("nickname", counter_ % 3 == 0 ? themis::Value{} : themis::Value{makeRandomString(8)});
        entity.setField("bio", makeRandomString(200));
        entity.setField("location_lat", 51.5 + ((counter_ % 100) * 0.1));
        entity.setField("location_lon", -0.1 + ((counter_ % 100) * 0.1));
        entity.setField("expires_at", std::to_string(std::time(nullptr) + 3600));
        
        secondary_->put("Person", entity);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(OptimizedInsertBenchmark, V133_SingleInsert)->Unit(benchmark::kMillisecond);

// --- v1.3.4: Batched Inserts (Kommit-Overhead Reduktion) ---
BENCHMARK_DEFINE_F(OptimizedInsertBenchmark, V134_BatchedInserts_100)(benchmark::State& state) {
    for (auto _ : state) {
        auto batch = db_->createWriteBatch();
        if (!batch) {
          continue;
        }
        
        for (size_t i = 0; i < 100; ++i) {
            themis::BaseEntity entity("v134_batch_" + std::to_string(counter_++));
            entity.setField("email", makeRandomString(20));
            entity.setField("age", static_cast<int64_t>(25 + (counter_ % 50)));
            entity.setField("nickname", counter_ % 3 == 0 ? themis::Value{} : themis::Value{makeRandomString(8)});
            entity.setField("bio", makeRandomString(200));
            entity.setField("location_lat", 51.5 + ((counter_ % 100) * 0.1));
            entity.setField("location_lon", -0.1 + ((counter_ % 100) * 0.1));
            entity.setField("expires_at", std::to_string(std::time(nullptr) + 3600));
            
            // HINWEIS: Das setzt voraus, dass secondary_->put(..., batch) existiert
            // Für jetzt: direct DB operation
            secondary_->put("Person", entity);
        }
        
        // In real implementation: batch->commit();
    }
    state.SetItemsProcessed(state.iterations() * 100);
}
BENCHMARK_REGISTER_F(OptimizedInsertBenchmark, V134_BatchedInserts_100)->Unit(benchmark::kMillisecond);

// --- v1.3.4: Mit Metadata-Cache (Hauptoptimierung) ---
// Diese würde mit Cache-Support für loadIndexedColumns_ etc. laufen
BENCHMARK_DEFINE_F(OptimizedInsertBenchmark, V134_WithMetadataCache)(benchmark::State& state) {
    // HINWEIS: Diese würde mit implementiertem Cache laufen
    // Vorausgesetzt dass loadIndexedColumns_ die gecachten Werte nutzt
    
    for (auto _ : state) {
        themis::BaseEntity entity("v134_cache_" + std::to_string(counter_++));
        entity.setField("email", makeRandomString(20));
        entity.setField("age", static_cast<int64_t>(25 + (counter_ % 50)));
        entity.setField("nickname", counter_ % 3 == 0 ? themis::Value{} : themis::Value{makeRandomString(8)});
        entity.setField("bio", makeRandomString(200));
        entity.setField("location_lat", 51.5 + ((counter_ % 100) * 0.1));
        entity.setField("location_lon", -0.1 + ((counter_ % 100) * 0.1));
        entity.setField("expires_at", std::to_string(std::time(nullptr) + 3600));
        
        secondary_->put("Person", entity);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(OptimizedInsertBenchmark, V134_WithMetadataCache)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
