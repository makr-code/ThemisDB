#include <benchmark/benchmark.h>
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include <random>
#include <filesystem>
#include <chrono>
#include <iostream>

namespace {
    std::string makeRandomString(size_t len) {
        static const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
        static std::mt19937 rng{std::random_device{}()};
        static std::uniform_int_distribution<size_t> dist(0, sizeof(charset) - 2);
        std::string s;
        s.reserve(len);
        for (size_t i = 0; i < len; ++i) s += charset[dist(rng)];
        return s;
    }

    void cleanupTestDB(const std::string& path) {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }

    // Timing Utilities
    struct Timer {
        std::chrono::high_resolution_clock::time_point start;
        Timer() : start(std::chrono::high_resolution_clock::now()) {}
        double elapsed_us() const {
            auto end = std::chrono::high_resolution_clock::now();
            return std::chrono::duration<double, std::micro>(end - start).count();
        }
    };
}

class ProfiledInsertFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = "bench_profiling_db";
        cleanupTestDB(db_path_);

        themis::RocksDBWrapper::Config config;
        config.db_path = db_path_;
        config.compression_default = "lz4";
        config.compression_bottommost = "zstd";
        config.block_cache_size_mb = 256;
        config.disable_wal_for_benchmark = true;
        config.memtable_size_mb = 512;

        db_ = std::make_unique<themis::RocksDBWrapper>(config);
        if (!db_->open()) {
            throw std::runtime_error("Failed to open RocksDB in ProfiledInsertFixture");
        }
        secondary_ = std::make_unique<themis::SecondaryIndexManager>(*db_);

        // Erstelle EXAKT die Indizes wie in InsertWithAllIndexes
        secondary_->createIndex("Person", "email", true);          // UNIQUE
        secondary_->createRangeIndex("Person", "age");
        secondary_->createSparseIndex("Person", "nickname", false);
        secondary_->createGeoIndex("Person", "location");
        secondary_->createTTLIndex("Person", "expires_at", 3600);
        secondary_->createFulltextIndex("Person", "bio");

        // Warmup: 10 Entities
        for (size_t i = 0; i < 10; ++i) {
            themis::BaseEntity entity("person_" + std::to_string(i));
            entity.setField("email", makeRandomString(20));
            entity.setField("age", static_cast<int64_t>(25 + (i % 50)));
            entity.setField("nickname", i % 3 == 0 ? themis::Value{} : themis::Value{makeRandomString(8)});
            entity.setField("bio", makeRandomString(200));
            entity.setField("location_lat", 51.5 + (i * 0.1));
            entity.setField("location_lon", -0.1 + (i * 0.1));
            entity.setField("expires_at", std::to_string(std::time(nullptr) + 3600));
            secondary_->put("Person", entity);
        }

        // Statistiken sammeln
        counter_ = 10;
        total_time_us_ = 0.0;
        call_count_ = 0;
    }

    void TearDown(const ::benchmark::State&) override {
        if (call_count_ > 0) {
            double avg_us = total_time_us_ / call_count_;
            double throughput = 1e6 / avg_us;
            std::cout << "\n\n=== PROFILING RESULTS ===\n";
            std::cout << "Total calls: " << call_count_ << "\n";
            std::cout << "Total time: " << total_time_us_ << " µs\n";
            std::cout << "Average per insert: " << avg_us << " µs\n";
            std::cout << "Throughput: " << throughput << " items/s\n";
            std::cout << "=========================\n\n";
        }

        secondary_.reset();
        db_.reset();
        cleanupTestDB(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::SecondaryIndexManager> secondary_;
    size_t counter_;
    double total_time_us_;
    size_t call_count_;
};

// --- BASELINE: Raw RocksDB Put (no indexing) ---
BENCHMARK_DEFINE_F(ProfiledInsertFixture, RawRocksDBPut)(benchmark::State& state) {
    for (auto _ : state) {
        Timer t;
        
        themis::BaseEntity entity("raw_" + std::to_string(counter_++));
        entity.setField("email", makeRandomString(20));
        entity.setField("age", static_cast<int64_t>(25 + (counter_ % 50)));
        entity.setField("nickname", counter_ % 3 == 0 ? themis::Value{} : themis::Value{makeRandomString(8)});
        entity.setField("bio", makeRandomString(200));
        
        // Nur Direct DB Put, kein Indexing
        std::string key = "raw:" + entity.getPrimaryKey();
        db_->put(key, entity.serialize());
        
        double us = t.elapsed_us();
        total_time_us_ += us;
        call_count_++;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(ProfiledInsertFixture, RawRocksDBPut)->Unit(benchmark::kMicrosecond);

// --- TEST 1: nur Regular Index (no other indexes) ---
BENCHMARK_DEFINE_F(ProfiledInsertFixture, IndexInsert_RegularOnly)(benchmark::State& state) {
    // TODO: Implement variant with ONLY regular index
    for (auto _ : state) {
        Timer t;
        
        themis::BaseEntity entity("test1_" + std::to_string(counter_++));
        entity.setField("email", makeRandomString(20));
        entity.setField("age", static_cast<int64_t>(25 + (counter_ % 50)));
        
        // This needs secondary_ with ONLY regular index
        // Simplified: use full put() for now
        secondary_->put("Person", entity);
        
        double us = t.elapsed_us();
        total_time_us_ += us;
        call_count_++;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(ProfiledInsertFixture, IndexInsert_RegularOnly)->Unit(benchmark::kMicrosecond);

// --- TEST 2: All Indexes (as in InsertWithAllIndexes) ---
BENCHMARK_DEFINE_F(ProfiledInsertFixture, IndexInsert_AllIndexes)(benchmark::State& state) {
    for (auto _ : state) {
        Timer t;
        
        themis::BaseEntity entity("test_all_" + std::to_string(counter_++));
        entity.setField("email", makeRandomString(20));
        entity.setField("age", static_cast<int64_t>(25 + (counter_ % 50)));
        entity.setField("nickname", counter_ % 3 == 0 ? themis::Value{} : themis::Value{makeRandomString(8)});
        entity.setField("bio", makeRandomString(200));
        entity.setField("location_lat", 51.5 + ((counter_ % 100) * 0.1));
        entity.setField("location_lon", -0.1 + ((counter_ % 100) * 0.1));
        entity.setField("expires_at", std::to_string(std::time(nullptr) + 3600));
        
        secondary_->put("Person", entity);
        
        double us = t.elapsed_us();
        total_time_us_ += us;
        call_count_++;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(ProfiledInsertFixture, IndexInsert_AllIndexes)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
