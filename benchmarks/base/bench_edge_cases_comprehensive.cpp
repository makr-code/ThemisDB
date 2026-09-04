/**
 * @file bench_edge_cases_comprehensive.cpp
 * @brief Comprehensive edge case and boundary condition benchmarks
 * 
 * Tests:
 * - Empty datasets and single element operations
 * - Boundary values (min/max integers, empty strings)
 * - Extreme key/value sizes
 * - Corner cases (null operations, duplicate keys)
 * - Stress conditions (rapid operations, overflow scenarios)
 * 
 * Follows BENCHMARK_BEST_PRACTICES.md with deterministic testing.
 */

#include <benchmark/benchmark.h>
#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <random>
#include <limits>
#include <chrono>

#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"

using namespace themis;

// Helper to create unique temp directory names
static std::string getUniqueTempPath(const std::string& base_name) {
    auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    return base_name + "_" + std::to_string(timestamp);
}

// ============================================================================
// Deterministic RNG
// ============================================================================

class DeterministicRNG {
private:
    std::mt19937_64 gen_ = {};

public:
    explicit DeterministicRNG(uint64_t seed = 42) : gen_(seed) {}

    uint64_t next() { return gen_(); }

    std::string generateString(size_t length) {
        static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::string result = {};
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
// Empty Dataset Benchmarks
// ============================================================================

static void BM_ReadFromEmptyDatabase(benchmark::State& state) {
    auto db_path = std::filesystem::temp_directory_path() / getUniqueTempPath("bench_empty_read");
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

    // Database is empty - all reads should miss
    DeterministicRNG rng(42);
    for (auto _ : state) {
        std::string key = "nonexistent_" + std::to_string(rng.next());
        auto value = db.get(key);
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(state.iterations());
    std::filesystem::remove_all(db_path);
}
BENCHMARK(BM_ReadFromEmptyDatabase)
    ->Unit(benchmark::kMicrosecond);

static void BM_FirstInsertToEmptyDatabase(benchmark::State& state) {
    DeterministicRNG rng(42);

    for (auto _ : state) {
        state.PauseTiming();
        auto db_path = std::filesystem::temp_directory_path() / ("bench_first_insert_" + std::to_string(rng.next()));
        std::filesystem::create_directories(db_path);

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path.string();
        cfg.disable_wal_for_benchmark = true;

        RocksDBWrapper db(cfg);
        if (!db.open()) {
            state.SkipWithError("Failed to open database");
            return;
        }
        state.ResumeTiming();

        // First insert to empty database
        BaseEntity entity("first", BaseEntity::FieldMap{
            {"value", static_cast<int64_t>(1)}
        });
        db.put("entity:first", entity.serialize());

        state.PauseTiming();
        std::filesystem::remove_all(db_path);
        state.ResumeTiming();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_FirstInsertToEmptyDatabase)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Single Element Operations
// ============================================================================

static void BM_SingleElementDatabase(benchmark::State& state) {
    auto db_path = std::filesystem::temp_directory_path() / getUniqueTempPath("bench_single_elem");
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

    // Insert single element
    BaseEntity entity("only", BaseEntity::FieldMap{
        {"value", static_cast<int64_t>(42)}
    });
    db.put("entity:only", entity.serialize());

    for (auto _ : state) {
        auto value = db.get("entity:only");
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(state.iterations());
    std::filesystem::remove_all(db_path);
}
BENCHMARK(BM_SingleElementDatabase)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Boundary Value Benchmarks
// ============================================================================

static void BM_MinMaxIntegerValues(benchmark::State& state) {
    auto db_path = std::filesystem::temp_directory_path() / getUniqueTempPath("bench_minmax");
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

    int counter = 0;
    for (auto _ : state) {
        std::string key = "minmax_" + std::to_string(counter++);
        BaseEntity entity(key, BaseEntity::FieldMap{
            {"min", std::numeric_limits<int64_t>::min()},
            {"max", std::numeric_limits<int64_t>::max()},
            {"zero", static_cast<int64_t>(0)}
        });
        db.put("entity:" + key, entity.serialize());
    }

    state.SetItemsProcessed(state.iterations());
    std::filesystem::remove_all(db_path);
}
BENCHMARK(BM_MinMaxIntegerValues)
    ->Unit(benchmark::kMicrosecond);

static void BM_EmptyStringValues(benchmark::State& state) {
    auto db_path = std::filesystem::temp_directory_path() / getUniqueTempPath("bench_empty_str");
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

    int counter = 0;
    for (auto _ : state) {
        std::string key = "empty_" + std::to_string(counter++);
        BaseEntity entity(key, BaseEntity::FieldMap{
            {"empty_str", std::string("")},
            {"value", static_cast<int64_t>(1)}
        });
        db.put("entity:" + key, entity.serialize());
    }

    state.SetItemsProcessed(state.iterations());
    std::filesystem::remove_all(db_path);
}
BENCHMARK(BM_EmptyStringValues)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Extreme Key/Value Sizes
// ============================================================================

static void BM_VeryShortKeys(benchmark::State& state) {
    auto db_path = std::filesystem::temp_directory_path() / getUniqueTempPath("bench_short_keys");
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

    int counter = 0;
    for (auto _ : state) {
        std::string key = std::to_string(counter++ % 10); // Single digit keys
        BaseEntity entity(key, BaseEntity::FieldMap{
            {"value", static_cast<int64_t>(counter)}
        });
        db.put(key, entity.serialize());
    }

    state.SetItemsProcessed(state.iterations());
    std::filesystem::remove_all(db_path);
}
BENCHMARK(BM_VeryShortKeys)
    ->Unit(benchmark::kMicrosecond);

static void BM_VeryLongKeys(benchmark::State& state) {
    auto db_path = std::filesystem::temp_directory_path() / getUniqueTempPath("bench_long_keys");
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
    int counter = 0;
    for (auto _ : state) {
        std::string key = rng.generateString(1000) + "_" + std::to_string(counter++);
        BaseEntity entity(key, BaseEntity::FieldMap{
            {"value", static_cast<int64_t>(counter)}
        });
        db.put(key, entity.serialize());
    }

    state.SetItemsProcessed(state.iterations());
    std::filesystem::remove_all(db_path);
}
BENCHMARK(BM_VeryLongKeys)
    ->Unit(benchmark::kMicrosecond);

static void BM_MaximumValueSize(benchmark::State& state) {
    auto db_path = std::filesystem::temp_directory_path() / getUniqueTempPath("bench_max_value");
    std::filesystem::remove_all(db_path);
    std::filesystem::create_directories(db_path);

    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path.string();
    cfg.disable_wal_for_benchmark = true;
    cfg.memtable_size_mb = 512; // Need larger memtable for huge values

    RocksDBWrapper db(cfg);
    if (!db.open()) {
        state.SkipWithError("Failed to open database");
        return;
    }

    DeterministicRNG rng(42);
    const size_t huge_size = 10 * 1024 * 1024; // 10MB values
    int counter = 0;

    for (auto _ : state) {
        std::string key = "huge_" + std::to_string(counter++);
        BaseEntity entity(key, BaseEntity::FieldMap{
            {"huge_data", rng.generateString(huge_size)}
        });
        db.put("entity:" + key, entity.serialize());
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * huge_size);
    std::filesystem::remove_all(db_path);
}
BENCHMARK(BM_MaximumValueSize)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(10); // Limit iterations due to size

// ============================================================================
// Duplicate Key Operations
// ============================================================================

static void BM_DuplicateKeyWrites(benchmark::State& state) {
    auto db_path = std::filesystem::temp_directory_path() / getUniqueTempPath("bench_dup_keys");
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

    const std::string fixed_key = "duplicate_key";
    DeterministicRNG rng(42);

    for (auto _ : state) {
        // Keep overwriting the same key
        BaseEntity entity(fixed_key, BaseEntity::FieldMap{
            {"value", rng.generateInt(0, 1000000)},
            {"timestamp", static_cast<int64_t>(state.iterations())}
        });
        db.put("entity:" + fixed_key, entity.serialize());
    }

    state.SetItemsProcessed(state.iterations());
    std::filesystem::remove_all(db_path);
}
BENCHMARK(BM_DuplicateKeyWrites)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Special Character Keys
// ============================================================================

static void BM_SpecialCharacterKeys(benchmark::State& state) {
    auto db_path = std::filesystem::temp_directory_path() / getUniqueTempPath("bench_special_keys");
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

    std::vector<std::string> special_keys = {
        "key:with:colons",
        "key/with/slashes",
        "key\\with\\backslashes",
        "key with spaces",
        "key\twith\ttabs",
        "key_with_ü_ñ_ü_c_ö_d_é",
        "key_with_emoji_🚀_🎉_✨",
        "key-with-dashes-and-numbers-123",
        "key.with.dots.456",
        "key@with@at@signs"
    };

    int counter = 0;
    for (auto _ : state) {
        std::string key = special_keys[counter % special_keys.size()] + "_" + std::to_string(counter++);
        BaseEntity entity(key, BaseEntity::FieldMap{
            {"value", static_cast<int64_t>(counter)}
        });
        db.put("entity:" + key, entity.serialize());
    }

    state.SetItemsProcessed(state.iterations());
    std::filesystem::remove_all(db_path);
}
BENCHMARK(BM_SpecialCharacterKeys)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Rapid Sequential Operations
// ============================================================================

static void BM_RapidSuccessiveWrites(benchmark::State& state) {
    auto db_path = std::filesystem::temp_directory_path() / getUniqueTempPath("bench_rapid_writes");
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

    const int rapid_count = 1000;
    DeterministicRNG rng(42);

    for (auto _ : state) {
        for (int i = 0; i < rapid_count; ++i) {
            std::string key = "rapid_" + std::to_string(i);
            BaseEntity entity(key, BaseEntity::FieldMap{
                {"value", rng.generateInt(0, 1000000)}
            });
            db.put("entity:" + key, entity.serialize());
        }
    }

    state.SetItemsProcessed(state.iterations() * rapid_count);
    std::filesystem::remove_all(db_path);
}
BENCHMARK(BM_RapidSuccessiveWrites)
    ->Unit(benchmark::kMillisecond);

static void BM_RapidSuccessiveReads(benchmark::State& state) {
    auto db_path = std::filesystem::temp_directory_path() / getUniqueTempPath("bench_rapid_reads");
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

    // Pre-populate
    DeterministicRNG rng(42);
    for (int i = 0; i < 1000; ++i) {
        BaseEntity entity("rapid_" + std::to_string(i), BaseEntity::FieldMap{
            {"value", rng.generateInt(0, 1000000)}
        });
        db.put("entity:" + entity.getPrimaryKey(), entity.serialize());
    }

    const int rapid_count = 1000;
    for (auto _ : state) {
        for (int i = 0; i < rapid_count; ++i) {
            auto value = db.get("entity:rapid_" + std::to_string(i));
            benchmark::DoNotOptimize(value);
        }
    }

    state.SetItemsProcessed(state.iterations() * rapid_count);
    std::filesystem::remove_all(db_path);
}
BENCHMARK(BM_RapidSuccessiveReads)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Alternating Operations
// ============================================================================

static void BM_AlternatingWriteRead(benchmark::State& state) {
    auto db_path = std::filesystem::temp_directory_path() / getUniqueTempPath("bench_alternating");
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
    int counter = 0;

    for (auto _ : state) {
        // Write
        std::string key = "alt_" + std::to_string(counter);
        BaseEntity entity(key, BaseEntity::FieldMap{
            {"value", rng.generateInt(0, 1000000)}
        });
        db.put("entity:" + key, entity.serialize());

        // Immediately read it back
        auto value = db.get("entity:" + key);
        benchmark::DoNotOptimize(value);

        counter++;
    }

    state.SetItemsProcessed(state.iterations() * 2); // Count both write and read
    std::filesystem::remove_all(db_path);
}
BENCHMARK(BM_AlternatingWriteRead)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Zero and Negative Values
// ============================================================================

static void BM_ZeroValues(benchmark::State& state) {
    auto db_path = std::filesystem::temp_directory_path() / getUniqueTempPath("bench_zero");
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

    int counter = 0;
    for (auto _ : state) {
        std::string key = "zero_" + std::to_string(counter++);
        BaseEntity entity(key, BaseEntity::FieldMap{
            {"zero_int", static_cast<int64_t>(0)},
            {"zero_str", std::string("")}
        });
        db.put("entity:" + key, entity.serialize());
    }

    state.SetItemsProcessed(state.iterations());
    std::filesystem::remove_all(db_path);
}
BENCHMARK(BM_ZeroValues)
    ->Unit(benchmark::kMicrosecond);

static void BM_NegativeValues(benchmark::State& state) {
    auto db_path = std::filesystem::temp_directory_path() / getUniqueTempPath("bench_negative");
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
    int counter = 0;
    for (auto _ : state) {
        std::string key = "negative_" + std::to_string(counter++);
        BaseEntity entity(key, BaseEntity::FieldMap{
            {"negative", -static_cast<int64_t>(rng.generateInt(1, 1000000))}
        });
        db.put("entity:" + key, entity.serialize());
    }

    state.SetItemsProcessed(state.iterations());
    std::filesystem::remove_all(db_path);
}
BENCHMARK(BM_NegativeValues)
    ->Unit(benchmark::kMicrosecond);
