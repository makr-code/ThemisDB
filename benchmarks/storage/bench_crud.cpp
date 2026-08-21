#include <benchmark/benchmark.h>
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include <random>
#include <filesystem>
#include <chrono>

namespace {
    // Deterministic, fixed seed for reproducible benchmark measurements.
    // Seeding from std::random_device is intentionally avoided.
    std::string makeRandomString(size_t len) {
        static const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
        static std::mt19937 rng{42};
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
}

class CRUDFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        // Unique path under the OS temp directory prevents collisions between
        // concurrent or repeated benchmark runs.
        const auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
        db_path_ = (std::filesystem::temp_directory_path() /
                    ("themis_bench_crud_" + std::to_string(ts))).string();
        cleanupTestDB(db_path_);

        themis::RocksDBWrapper::Config config;
        config.db_path = db_path_;
        config.compression_default = "lz4";
        config.compression_bottommost = "zstd";
        config.block_cache_size_mb = 256;

        db_ = std::make_unique<themis::RocksDBWrapper>(config);
        if (!db_->open()) {
            throw std::runtime_error("Failed to open RocksDB in CRUDFixture");
        }
        secondary_ = std::make_unique<themis::SecondaryIndexManager>(*db_);

        // Indizes: Regular, Range, Sparse, Geo, TTL, Fulltext
        secondary_->createIndex("Person", "email", true);
        secondary_->createRangeIndex("Person", "age");
        secondary_->createSparseIndex("Person", "nickname", false);
        secondary_->createGeoIndex("Person", "location");
        secondary_->createTTLIndex("Person", "expires_at", 3600);
        secondary_->createFulltextIndex("Person", "bio");

        // Warmup: 100 Entities
        for (size_t i = 0; i < 100; ++i) {
            themis::BaseEntity entity("person_" + std::to_string(i));
            entity.setField("email", makeRandomString(20));
            entity.setField("age", static_cast<int64_t>(25 + (i % 50)));
            entity.setField("nickname", i % 3 == 0 ? themis::Value{} : themis::Value{makeRandomString(8)});
            entity.setField("bio", makeRandomString(200));
            secondary_->put("Person", entity);
        }
    }

    void TearDown(const ::benchmark::State&) override {
        secondary_.reset();
        db_.reset();
        cleanupTestDB(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::SecondaryIndexManager> secondary_;
};

// --- Write Benchmarks ---

BENCHMARK_DEFINE_F(CRUDFixture, InsertWithAllIndexes)(benchmark::State& state) {
    size_t counter = 100;
    for (auto _ : state) {
        themis::BaseEntity entity("person_" + std::to_string(counter++));
        entity.setField("email", makeRandomString(20));
        entity.setField("age", static_cast<int64_t>(25 + (counter % 50)));
        entity.setField("nickname", counter % 3 == 0 ? themis::Value{} : themis::Value{makeRandomString(8)});
        entity.setField("bio", makeRandomString(200));
        secondary_->put("Person", entity);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(CRUDFixture, InsertWithAllIndexes)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

// --- Read Benchmarks ---

BENCHMARK_DEFINE_F(CRUDFixture, LookupBySecondaryIndex)(benchmark::State& state) {
    for (auto _ : state) {
        auto [status, pks] = secondary_->scanKeysEqual("Person", "email", "test42@example.com");
        benchmark::DoNotOptimize(pks);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(CRUDFixture, LookupBySecondaryIndex)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// --- Range-Index Benchmark ---

BENCHMARK_DEFINE_F(CRUDFixture, RangeScanAge)(benchmark::State& state) {
    for (auto _ : state) {
        auto [status, pks] = secondary_->scanKeysRange("Person", "age", std::optional<std::string>("25"), std::optional<std::string>("35"), true, true, 100, false);
        benchmark::DoNotOptimize(pks);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(CRUDFixture, RangeScanAge)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// --- Fulltext-Index Benchmark ---

BENCHMARK_DEFINE_F(CRUDFixture, FulltextSearch)(benchmark::State& state) {
    for (auto _ : state) {
        auto [status, pks] = secondary_->scanFulltext("Person", "bio", "quick brown fox", 100);
        benchmark::DoNotOptimize(pks);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(CRUDFixture, FulltextSearch)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

BENCHMARK_MAIN();
