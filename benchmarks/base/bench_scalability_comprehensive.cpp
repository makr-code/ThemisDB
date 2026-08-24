/**
 * @file bench_scalability_comprehensive.cpp
 * @brief Comprehensive scalability benchmarks measuring performance across different data sizes
 * 
 * Tests:
 * - Data size scaling (1K, 10K, 100K records)
 * - Memory bounds and pressure
 * - Query performance with growing datasets
 * - Index scaling behavior
 * 
 * Follows BENCHMARK_BEST_PRACTICES.md with deterministic seeds and warmup phases.
 */

#include <benchmark/benchmark.h>
#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <random>
#include <algorithm>
#include <stdexcept>

#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"

using namespace themis;

// ============================================================================
// Deterministic RNG
// ============================================================================

class DeterministicRNG {
private:
    std::mt19937_64 gen_;

public:
    explicit DeterministicRNG(uint64_t seed = 42) : gen_(seed) {}

    uint64_t next() { return gen_(); }

    std::string generateString(size_t length) {
        static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::string result;
        result.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            result += charset[next() % (sizeof(charset) - 1)];
        }
        return result;
    }

    int64_t generateInt(int64_t min, int64_t max) {
        return min + (next() % (max - min + 1));
    }
};

// ============================================================================
// Scalability Test Fixture
// ============================================================================

class ScalabilityBenchFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        db_path_ = std::filesystem::temp_directory_path() / ("bench_scale_" + std::to_string(reinterpret_cast<uintptr_t>(this)));
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_.string();
        cfg.memtable_size_mb = 512;
        cfg.block_cache_size_mb = 1024;
        cfg.disable_wal_for_benchmark = true;
        cfg.allow_concurrent_memtable_write = true;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) {
            throw std::runtime_error("Failed to open database");
        }

        dataset_size_ = state.range(0);
        populateDataset();
    }

    void TearDown(const ::benchmark::State& state) override {
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::filesystem::path db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    size_t dataset_size_ = 0;

    void populateDataset() {
        DeterministicRNG rng(42);
        for (size_t i = 0; i < dataset_size_; ++i) {
            BaseEntity entity("entity_" + std::to_string(i), BaseEntity::FieldMap{
                {"value", rng.generateInt(0, 1000000)},
                {"data", rng.generateString(100)},
                {"index", static_cast<int64_t>(i)}
            });
            db_->put("entity:" + entity.getPrimaryKey(), entity.serialize());
        }
    }
};

// ============================================================================
// Data Size Scaling Benchmarks
// ============================================================================

BENCHMARK_F(ScalabilityBenchFixture, RandomRead_ScalingDataSize)(benchmark::State& state) {
    DeterministicRNG rng(42);

    for (auto _ : state) {
        size_t id = rng.generateInt(0, dataset_size_ - 1);
        std::string key = "entity:entity_" + std::to_string(id);
        auto value = db_->get(key);
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["DatasetSize"] = dataset_size_;
}
BENCHMARK_REGISTER_F(ScalabilityBenchFixture, RandomRead_ScalingDataSize)
    ->Unit(benchmark::kMicrosecond)
    ->Arg(1000)->Arg(10000)->Arg(100000);

BENCHMARK_F(ScalabilityBenchFixture, SequentialRead_ScalingDataSize)(benchmark::State& state) {
    size_t counter = 0;

    for (auto _ : state) {
        size_t id = counter++ % dataset_size_;
        std::string key = "entity:entity_" + std::to_string(id);
        auto value = db_->get(key);
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["DatasetSize"] = dataset_size_;
}
BENCHMARK_REGISTER_F(ScalabilityBenchFixture, SequentialRead_ScalingDataSize)
    ->Unit(benchmark::kMicrosecond)
    ->Arg(1000)->Arg(10000)->Arg(100000);

BENCHMARK_F(ScalabilityBenchFixture, RangeScan_ScalingDataSize)(benchmark::State& state) {
    DeterministicRNG rng(42);
    const size_t scan_size = 100;

    for (auto _ : state) {
        size_t start_id = rng.generateInt(0, dataset_size_ - scan_size);
        std::vector<BaseEntity::Blob> results;
        results.reserve(scan_size);

        for (size_t i = 0; i < scan_size; ++i) {
            std::string key = "entity:entity_" + std::to_string(start_id + i);
            auto value = db_->get(key);
            if (value.has_value()) {
                results.push_back(value.value());
            }
        }

        benchmark::DoNotOptimize(results);
    }

    state.SetItemsProcessed(state.iterations() * scan_size);
    state.counters["DatasetSize"] = dataset_size_;
}
BENCHMARK_REGISTER_F(ScalabilityBenchFixture, RangeScan_ScalingDataSize)
    ->Unit(benchmark::kMicrosecond)
    ->Arg(1000)->Arg(10000)->Arg(100000);

// ============================================================================
// Write Scaling Benchmarks
// ============================================================================

BENCHMARK_F(ScalabilityBenchFixture, AppendWrite_ScalingDataSize)(benchmark::State& state) {
    DeterministicRNG rng(42);
    size_t write_counter = dataset_size_;

    for (auto _ : state) {
        std::string key = "append_" + std::to_string(write_counter++);
        BaseEntity entity(key, BaseEntity::FieldMap{
            {"value", rng.generateInt(0, 1000000)},
            {"data", rng.generateString(100)}
        });
        db_->put("entity:" + key, entity.serialize());
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["InitialDatasetSize"] = dataset_size_;
}
BENCHMARK_REGISTER_F(ScalabilityBenchFixture, AppendWrite_ScalingDataSize)
    ->Unit(benchmark::kMicrosecond)
    ->Arg(1000)->Arg(10000)->Arg(100000);

BENCHMARK_F(ScalabilityBenchFixture, UpdateWrite_ScalingDataSize)(benchmark::State& state) {
    DeterministicRNG rng(42);

    for (auto _ : state) {
        size_t id = rng.generateInt(0, dataset_size_ - 1);
        std::string key = "entity_" + std::to_string(id);
        BaseEntity entity(key, BaseEntity::FieldMap{
            {"value", rng.generateInt(0, 1000000)},
            {"data", rng.generateString(100)},
            {"updated", static_cast<int64_t>(1)}
        });
        db_->put("entity:" + key, entity.serialize());
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["DatasetSize"] = dataset_size_;
}
BENCHMARK_REGISTER_F(ScalabilityBenchFixture, UpdateWrite_ScalingDataSize)
    ->Unit(benchmark::kMicrosecond)
    ->Arg(1000)->Arg(10000)->Arg(100000);

// ============================================================================
// Memory Bounds Benchmarks
// ============================================================================

static void BM_LargeRecordInsertion(benchmark::State& state) {
    auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    auto db_path = std::filesystem::temp_directory_path() / ("bench_large_records_" + std::to_string(timestamp));
    std::filesystem::remove_all(db_path);
    std::filesystem::create_directories(db_path);

    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path.string();
    cfg.memtable_size_mb = 256;
    cfg.block_cache_size_mb = 512;
    cfg.disable_wal_for_benchmark = true;

    RocksDBWrapper db(cfg);
    if (!db.open()) {
        state.SkipWithError("Failed to open database");
        return;
    }

    const size_t record_size = state.range(0);
    DeterministicRNG rng(42);
    int counter = 0;

    for (auto _ : state) {
        std::string key = "large_" + std::to_string(counter++);
        BaseEntity entity(key, BaseEntity::FieldMap{
            {"data", rng.generateString(record_size)}
        });
        db.put("entity:" + key, entity.serialize());
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * record_size);
    state.counters["RecordSize"] = record_size;

    std::filesystem::remove_all(db_path);
}
BENCHMARK(BM_LargeRecordInsertion)
    ->Unit(benchmark::kMillisecond)
    ->Arg(1024)       // 1KB
    ->Arg(10240)      // 10KB
    ->Arg(102400)     // 100KB
    ->Arg(1024000);   // 1MB

// ============================================================================
// Batch Size Scaling
// ============================================================================

static void BM_BatchSizeScaling(benchmark::State& state) {
    auto db_path = std::filesystem::temp_directory_path() / "bench_batch_scale";
    std::filesystem::remove_all(db_path);
    std::filesystem::create_directories(db_path);

    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path.string();
    cfg.disable_wal_for_benchmark = true;

    RocksDBWrapper db(cfg);
    if (!db.open()) {
        state.SkipWithError("Failed to open database");
        return;
    }

    const int batch_size = state.range(0);
    DeterministicRNG rng(42);
    int counter = 0;

    for (auto _ : state) {
        for (int i = 0; i < batch_size; ++i) {
            std::string key = "batch_" + std::to_string(counter++);
            BaseEntity entity(key, BaseEntity::FieldMap{
                {"value", rng.generateInt(0, 1000000)}
            });
            db.put("entity:" + key, entity.serialize());
        }
    }

    state.SetItemsProcessed(state.iterations() * batch_size);
    state.counters["BatchSize"] = batch_size;

    std::filesystem::remove_all(db_path);
}
BENCHMARK(BM_BatchSizeScaling)
    ->Unit(benchmark::kMillisecond)
    ->Arg(1)->Arg(10)->Arg(100)->Arg(1000)->Arg(10000);

// ============================================================================
// Query Complexity Scaling
// ============================================================================

static void BM_FullScanScaling(benchmark::State& state) {
    auto db_path = std::filesystem::temp_directory_path() / "bench_fullscan";
    std::filesystem::remove_all(db_path);
    std::filesystem::create_directories(db_path);

    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path.string();
    cfg.disable_wal_for_benchmark = true;

    RocksDBWrapper db(cfg);
    if (!db.open()) {
        state.SkipWithError("Failed to open database");
        return;
    }

    // Populate with specified number of records
    const size_t dataset_size = state.range(0);
    DeterministicRNG rng(42);
    for (size_t i = 0; i < dataset_size; ++i) {
        BaseEntity entity("scan_" + std::to_string(i), BaseEntity::FieldMap{
            {"value", rng.generateInt(0, 1000000)}
        });
        db.put("entity:" + entity.getPrimaryKey(), entity.serialize());
    }

    for (auto _ : state) {
        size_t count = 0;
        // Simulate full scan by reading all keys
        for (size_t i = 0; i < dataset_size; ++i) {
            auto value = db.get("entity:scan_" + std::to_string(i));
            if (value.has_value()) count++;
        }
        benchmark::DoNotOptimize(count);
    }

    state.SetItemsProcessed(state.iterations() * dataset_size);
    state.counters["DatasetSize"] = dataset_size;

    std::filesystem::remove_all(db_path);
}
BENCHMARK(BM_FullScanScaling)
    ->Unit(benchmark::kMillisecond)
    ->Arg(100)->Arg(1000)->Arg(10000);

// ============================================================================
// Memory Pressure Simulation
// ============================================================================

static void BM_MemoryPressure(benchmark::State& state) {
    auto db_path = std::filesystem::temp_directory_path() / "bench_mem_pressure";
    std::filesystem::remove_all(db_path);
    std::filesystem::create_directories(db_path);

    // Configure with limited memory to simulate pressure
    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path.string();
    cfg.memtable_size_mb = 16;  // Small memtable
    cfg.block_cache_size_mb = 32;  // Small cache
    cfg.disable_wal_for_benchmark = true;

    RocksDBWrapper db(cfg);
    if (!db.open()) {
        state.SkipWithError("Failed to open database");
        return;
    }

    DeterministicRNG rng(42);
    int counter = 0;

    // Pre-fill to create memory pressure
    for (int i = 0; i < 10000; ++i) {
        BaseEntity entity("prefill_" + std::to_string(i), BaseEntity::FieldMap{
            {"data", rng.generateString(1000)}
        });
        db.put("entity:" + entity.getPrimaryKey(), entity.serialize());
    }

    for (auto _ : state) {
        // Mixed workload under memory pressure
        for (int i = 0; i < 100; ++i) {
            if (i % 3 == 0) {
                // Write
                std::string key = "pressure_" + std::to_string(counter++);
                BaseEntity entity(key, BaseEntity::FieldMap{
                    {"data", rng.generateString(1000)}
                });
                db.put("entity:" + key, entity.serialize());
            } else {
                // Read
                int id = rng.generateInt(0, 9999);
                auto value = db.get("entity:prefill_" + std::to_string(id));
                benchmark::DoNotOptimize(value);
            }
        }
    }

    state.SetItemsProcessed(state.iterations() * 100);

    std::filesystem::remove_all(db_path);
}
BENCHMARK(BM_MemoryPressure)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Data Growth Over Time
// ============================================================================

static void BM_ContinuousGrowth(benchmark::State& state) {
    auto db_path = std::filesystem::temp_directory_path() / "bench_growth";
    std::filesystem::remove_all(db_path);
    std::filesystem::create_directories(db_path);

    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path.string();
    cfg.disable_wal_for_benchmark = true;

    RocksDBWrapper db(cfg);
    if (!db.open()) {
        state.SkipWithError("Failed to open database");
        return;
    }

    DeterministicRNG rng(42);
    size_t total_records = 0;

    for (auto _ : state) {
        // Insert 1000 new records per iteration
        for (int i = 0; i < 1000; ++i) {
            std::string key = "growth_" + std::to_string(total_records++);
            BaseEntity entity(key, BaseEntity::FieldMap{
                {"value", rng.generateInt(0, 1000000)},
                {"data", rng.generateString(100)}
            });
            db.put("entity:" + key, entity.serialize());
        }

        // Perform some reads to simulate real workload
        for (int i = 0; i < 100; ++i) {
            if (total_records > 0) {
                size_t id = rng.generateInt(0, total_records - 1);
                auto value = db.get("entity:growth_" + std::to_string(id));
                benchmark::DoNotOptimize(value);
            }
        }
    }

    state.SetItemsProcessed(state.iterations() * 1100);
    state.counters["TotalRecords"] = total_records;

    std::filesystem::remove_all(db_path);
}
BENCHMARK(BM_ContinuousGrowth)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(10);  // Run 10 iterations to show growth trend
