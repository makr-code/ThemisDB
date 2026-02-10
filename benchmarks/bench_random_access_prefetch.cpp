/**
 * @file bench_random_access_prefetch.cpp
 * @brief Google Benchmark tests for random access performance with CPU prefetch hints
 * 
 * Tests random access performance with and without prefetch optimizations:
 * - Point reads (get) with/without prefetch
 * - Batch reads (multiGet) with/without prefetch
 * - Prefix scanning with/without prefetch
 * - Range scanning with/without prefetch
 * 
 * Measures:
 * - Latency reduction from prefetch hints
 * - Cache hit rates improvement
 * - Throughput gains for batch operations
 * 
 * Expected improvements:
 * - 15-25% latency reduction for random access
 * - 20-40% throughput increase for batch operations
 * - Higher cache line utilization
 * 
 * @author ThemisDB Team
 * @date February 2026
 */

#include <benchmark/benchmark.h>
#include "storage/rocksdb_wrapper.h"
#include "performance/prefetch_hints.h"
#include <vector>
#include <string>
#include <random>
#include <cstring>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <filesystem>

using namespace themis;
using namespace themis::performance;

namespace {

// Test data configuration
constexpr size_t NUM_KEYS = 10000;
constexpr size_t VALUE_SIZE = 256;
constexpr size_t BATCH_SIZE = 100;

// ═══════════════════════════════════════════════════════════
// Test Data Generators
// ═══════════════════════════════════════════════════════════

std::vector<std::string> generateKeys(size_t count) {
    std::vector<std::string> keys;
    keys.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        keys.push_back("bench_key_" + std::to_string(i));
    }
    return keys;
}

std::vector<uint8_t> generateValue(size_t size) {
    static std::mt19937 rng(42);
    static std::uniform_int_distribution<int> dist(0, 255);
    
    std::vector<uint8_t> value;
    value.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        value.push_back(dist(rng));
    }
    return value;
}

std::vector<size_t> generateRandomIndices(size_t count, size_t max_index) {
    std::vector<size_t> indices;
    indices.reserve(count);
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(0, max_index - 1);
    for (size_t i = 0; i < count; ++i) {
        indices.push_back(dist(rng));
    }
    return indices;
}

// ═══════════════════════════════════════════════════════════
// Benchmark Fixture
// ═══════════════════════════════════════════════════════════

class RandomAccessFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        // Create temporary database
        test_db_path_ = "./bench_random_access_" + std::to_string(state.thread_index());
        std::filesystem::remove_all(test_db_path_);
        
        // Configure database
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.memtable_size_mb = 128;
        config.block_cache_size_mb = 256;
        // NOTE: WAL disabled for faster benchmark setup only
        // Production benchmarks should enable WAL (enable_wal = true) for realistic measurements
        config.enable_wal = false;
        config.enable_cpu_prefetch = (state.range(0) == 1);  // Control prefetch via parameter
        config.prefetch_distance = 2;
        config.prefetch_min_batch_size = 4;
        
        db_ = std::make_unique<RocksDBWrapper>(config);
        
        if (!db_->open()) {
            throw std::runtime_error("Failed to open database");
        }
        
        // Populate with test data
        all_keys_ = generateKeys(NUM_KEYS);
        for (const auto& key : all_keys_) {
            auto value = generateValue(VALUE_SIZE);
            db_->put(key, value);
        }
        
        // Generate random access pattern
        random_indices_ = generateRandomIndices(BATCH_SIZE, NUM_KEYS);
    }
    
    void TearDown(const ::benchmark::State& state) override {
        db_->close();
        db_.reset();
        std::filesystem::remove_all(test_db_path_);
    }
    
protected:
    std::unique_ptr<RocksDBWrapper> db_;
    std::vector<std::string> all_keys_;
    std::vector<size_t> random_indices_;
    std::string test_db_path_;
};

// ═══════════════════════════════════════════════════════════
// Point Read Benchmarks (get)
// ═══════════════════════════════════════════════════════════

/**
 * Baseline: Random point reads without prefetch
 * Target: <1us per read (typical RocksDB performance)
 */
BENCHMARK_DEFINE_F(RandomAccessFixture, PointRead_NoPrefetch)(benchmark::State& state) {
    for (auto _ : state) {
        for (size_t idx : random_indices_) {
            auto result = db_->get(all_keys_[idx]);
            benchmark::DoNotOptimize(result);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * random_indices_.size());
    state.SetLabel("no_prefetch");
}
BENCHMARK_REGISTER_F(RandomAccessFixture, PointRead_NoPrefetch)
    ->Arg(0)  // Prefetch disabled
    ->Unit(benchmark::kMicrosecond);

/**
 * Optimized: Random point reads with CPU prefetch hints
 * Target: 15-25% latency reduction
 */
BENCHMARK_DEFINE_F(RandomAccessFixture, PointRead_WithPrefetch)(benchmark::State& state) {
    for (auto _ : state) {
        for (size_t idx : random_indices_) {
            auto result = db_->get(all_keys_[idx]);
            benchmark::DoNotOptimize(result);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * random_indices_.size());
    state.SetLabel("with_prefetch");
}
BENCHMARK_REGISTER_F(RandomAccessFixture, PointRead_WithPrefetch)
    ->Arg(1)  // Prefetch enabled
    ->Unit(benchmark::kMicrosecond);

// ═══════════════════════════════════════════════════════════
// Batch Read Benchmarks (multiGet)
// ═══════════════════════════════════════════════════════════

/**
 * Baseline: Batch reads without prefetch
 * Target: <100us per batch of 100 keys
 */
BENCHMARK_DEFINE_F(RandomAccessFixture, BatchRead_NoPrefetch)(benchmark::State& state) {
    std::vector<std::string> batch_keys;
    batch_keys.reserve(BATCH_SIZE);
    
    for (auto _ : state) {
        state.PauseTiming();
        batch_keys.clear();
        for (size_t idx : random_indices_) {
            batch_keys.push_back(all_keys_[idx]);
        }
        state.ResumeTiming();
        
        auto results = db_->multiGet(batch_keys);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * BATCH_SIZE);
    state.SetLabel("no_prefetch");
}
BENCHMARK_REGISTER_F(RandomAccessFixture, BatchRead_NoPrefetch)
    ->Arg(0)  // Prefetch disabled
    ->Unit(benchmark::kMicrosecond);

/**
 * Optimized: Batch reads with CPU prefetch hints
 * Target: 20-40% throughput improvement
 */
BENCHMARK_DEFINE_F(RandomAccessFixture, BatchRead_WithPrefetch)(benchmark::State& state) {
    std::vector<std::string> batch_keys;
    batch_keys.reserve(BATCH_SIZE);
    
    for (auto _ : state) {
        state.PauseTiming();
        batch_keys.clear();
        for (size_t idx : random_indices_) {
            batch_keys.push_back(all_keys_[idx]);
        }
        state.ResumeTiming();
        
        auto results = db_->multiGet(batch_keys);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * BATCH_SIZE);
    state.SetLabel("with_prefetch");
}
BENCHMARK_REGISTER_F(RandomAccessFixture, BatchRead_WithPrefetch)
    ->Arg(1)  // Prefetch enabled
    ->Unit(benchmark::kMicrosecond);

// ═══════════════════════════════════════════════════════════
// Prefix Scan Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * Baseline: Prefix scan without prefetch
 * Target: <500us for 100 entries
 */
BENCHMARK_DEFINE_F(RandomAccessFixture, PrefixScan_NoPrefetch)(benchmark::State& state) {
    for (auto _ : state) {
        size_t count = 0;
        db_->scanPrefix("bench_key_1", [&](std::string_view key, std::string_view value) {
            benchmark::DoNotOptimize(key);
            benchmark::DoNotOptimize(value);
            ++count;
            return count < 100;  // Limit to 100 entries
        });
        benchmark::DoNotOptimize(count);
    }
    
    state.SetLabel("no_prefetch");
}
BENCHMARK_REGISTER_F(RandomAccessFixture, PrefixScan_NoPrefetch)
    ->Arg(0)  // Prefetch disabled
    ->Unit(benchmark::kMicrosecond);

/**
 * Optimized: Prefix scan with CPU prefetch hints
 * Target: 10-20% latency reduction
 */
BENCHMARK_DEFINE_F(RandomAccessFixture, PrefixScan_WithPrefetch)(benchmark::State& state) {
    for (auto _ : state) {
        size_t count = 0;
        db_->scanPrefix("bench_key_1", [&](std::string_view key, std::string_view value) {
            benchmark::DoNotOptimize(key);
            benchmark::DoNotOptimize(value);
            ++count;
            return count < 100;  // Limit to 100 entries
        });
        benchmark::DoNotOptimize(count);
    }
    
    state.SetLabel("with_prefetch");
}
BENCHMARK_REGISTER_F(RandomAccessFixture, PrefixScan_WithPrefetch)
    ->Arg(1)  // Prefetch enabled
    ->Unit(benchmark::kMicrosecond);

// ═══════════════════════════════════════════════════════════
// Prefetch Utility Benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * Measure overhead of prefetch instruction itself
 * Target: <10ns per prefetch (should be negligible)
 */
static void BM_Prefetch_Overhead(benchmark::State& state) {
    std::vector<uint8_t> data(1024 * 1024, 0);  // 1MB buffer
    
    for (auto _ : state) {
        for (size_t i = 0; i < 1000; ++i) {
            prefetch(&data[i * 64], PrefetchHint::T0);
        }
        benchmark::ClobberMemory();
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
    state.SetLabel("prefetch_overhead");
}
BENCHMARK(BM_Prefetch_Overhead);

/**
 * Measure benefit of prefetch_range for large structures
 * Target: 20-30% latency reduction for large sequential access
 */
static void BM_Prefetch_Range_Benefit(benchmark::State& state) {
    constexpr size_t SIZE = 4 * 1024 * 1024;  // 4MB
    std::vector<uint8_t> data(SIZE, 0);
    bool use_prefetch = (state.range(0) == 1);
    
    for (auto _ : state) {
        uint64_t sum = 0;
        const uint64_t* ptr = reinterpret_cast<const uint64_t*>(data.data());
        size_t count = SIZE / sizeof(uint64_t);
        
        for (size_t i = 0; i < count; i += 8) {
            if (use_prefetch && i + 32 < count) {
                prefetch_range(&ptr[i + 32], 256, PrefetchHint::T1);
            }
            for (size_t j = 0; j < 8 && (i + j) < count; ++j) {
                sum += ptr[i + j];
            }
        }
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }
    
    state.SetBytesProcessed(state.iterations() * SIZE);
    state.SetLabel(use_prefetch ? "with_prefetch_range" : "no_prefetch");
}
BENCHMARK(BM_Prefetch_Range_Benefit)
    ->Arg(0)  // No prefetch
    ->Arg(1)  // With prefetch
    ->Unit(benchmark::kMicrosecond);

// ═══════════════════════════════════════════════════════════
// Complexity Analysis
// ═══════════════════════════════════════════════════════════

/**
 * Verify prefetch scales linearly with batch size
 */
BENCHMARK_DEFINE_F(RandomAccessFixture, BatchRead_Scaling)(benchmark::State& state) {
    const size_t batch_size = state.range(0);
    std::vector<std::string> batch_keys;
    batch_keys.reserve(batch_size);
    
    for (auto _ : state) {
        state.PauseTiming();
        batch_keys.clear();
        for (size_t i = 0; i < batch_size && i < all_keys_.size(); ++i) {
            batch_keys.push_back(all_keys_[i]);
        }
        state.ResumeTiming();
        
        auto results = db_->multiGet(batch_keys);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
    state.SetComplexityN(batch_size);
}
BENCHMARK_REGISTER_F(RandomAccessFixture, BatchRead_Scaling)
    ->Arg(1)  // Prefetch enabled
    ->RangeMultiplier(2)
    ->Range(8, 256)
    ->Complexity();

} // namespace

// ═══════════════════════════════════════════════════════════
// Main - Configure JSON output for CI
// ═══════════════════════════════════════════════════════════

BENCHMARK_MAIN();
