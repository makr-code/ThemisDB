/**
 * @file bench_latency_comprehensive.cpp
 * @brief Comprehensive latency benchmarks measuring P50, P95, P99, and P99.9 percentiles
 * 
 * Follows scientific standards for latency measurement with:
 * - Deterministic seeds for reproducibility
 * - Warmup phases to eliminate cold start effects
 * - Percentile calculations (P50, P95, P99, P99.9)
 * - Realistic data sizes and access patterns
 */

#include <benchmark/benchmark.h>
#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <random>
#include <algorithm>
#include <chrono>
#include <mutex>

#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"

using namespace themis;
using namespace std::chrono;

// ============================================================================
// Latency Tracker with Percentile Calculation
// ============================================================================

class LatencyTracker {
private:
    std::vector<double> latencies_us_; // Latencies in microseconds
    mutable std::mutex mutex_;

public:
    void record(double latency_us) {
        std::lock_guard<std::mutex> lock(mutex_);
        latencies_us_.push_back(latency_us);
    }

    struct LatencyStats {
        double min_us = 0;
        double max_us = 0;
        double mean_us = 0;
        double p50_us = 0;
        double p95_us = 0;
        double p99_us = 0;
        double p99_9_us = 0;
        size_t count = 0;
    };

    LatencyStats calculateStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (latencies_us_.empty()) return {};

        LatencyStats stats;
        stats.count = latencies_us_.size();

        // Sort for percentile calculation
        auto sorted = latencies_us_;
        std::sort(sorted.begin(), sorted.end());

        stats.min_us = sorted.front();
        stats.max_us = sorted.back();

        // Mean
        double sum = 0;
        for (double lat : sorted) sum += lat;
        stats.mean_us = sum / sorted.size();

        // Percentiles
        auto percentile = [&](double p) -> double {
            if (sorted.empty()) {
                return 0.0;
            }
            // Use p * (n - 1) with linear interpolation between neighboring points
            const double pos = p * static_cast<double>(sorted.size() - 1);
            const size_t idx_lower = static_cast<size_t>(pos);
            const size_t idx_upper = (idx_lower + 1 < sorted.size()) ? idx_lower + 1 : idx_lower;
            const double weight = pos - static_cast<double>(idx_lower);
            return sorted[idx_lower] * (1.0 - weight) + sorted[idx_upper] * weight;
        };

        stats.p50_us = percentile(0.50);
        stats.p95_us = percentile(0.95);
        stats.p99_us = percentile(0.99);
        stats.p99_9_us = percentile(0.999);

        return stats;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        latencies_us_.clear();
    }
};

// ============================================================================
// Deterministic RNG
// ============================================================================

class DeterministicRNG {
private:
    std::mt19937_64 gen_;

public:
    explicit DeterministicRNG(uint64_t seed = 42) : gen_(seed) {}

    uint64_t next() {
        return gen_();
    }

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
// Latency Benchmark Fixture
// ============================================================================

class LatencyBenchFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        db_path_ = std::filesystem::temp_directory_path() / ("bench_latency_" + std::to_string(reinterpret_cast<uintptr_t>(this)));
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_.string();
        cfg.memtable_size_mb = 256;
        cfg.block_cache_size_mb = 512;
        cfg.disable_wal_for_benchmark = true;
        cfg.allow_concurrent_memtable_write = true;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) {
            throw std::runtime_error("Failed to open database");
        }

        // Warmup: pre-populate database
        warmup();
    }

    void TearDown(const ::benchmark::State& state) override {
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::filesystem::path db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    LatencyTracker latency_tracker_;

    void warmup() {
        DeterministicRNG rng(42);
        for (int i = 0; i < 10000; ++i) {
            BaseEntity entity("warmup_" + std::to_string(i), BaseEntity::FieldMap{
                {"value", rng.generateInt(0, 1000000)},
                {"data", rng.generateString(100)}
            });
            db_->put("entity:" + entity.getPrimaryKey(), entity.serialize());
        }
    }

    void setLatencyCounters(benchmark::State& state) {
        auto stats = latency_tracker_.calculateStats();
        if (stats.count > 0) {
            state.counters["P50_us"] = stats.p50_us;
            state.counters["P95_us"] = stats.p95_us;
            state.counters["P99_us"] = stats.p99_us;
            state.counters["P99.9_us"] = stats.p99_9_us;
            state.counters["Min_us"] = stats.min_us;
            state.counters["Max_us"] = stats.max_us;
            state.counters["Mean_us"] = stats.mean_us;
        }
        latency_tracker_.reset();
    }
};

// ============================================================================
// Read Latency Benchmarks
// ============================================================================

BENCHMARK_F(LatencyBenchFixture, ReadLatency_SmallValues)(benchmark::State& state) {
    DeterministicRNG rng(42);

    for (auto _ : state) {
        int id = rng.generateInt(0, 9999);
        std::string key = "entity:warmup_" + std::to_string(id);

        auto start = high_resolution_clock::now();
        auto value = db_->get(key);
        auto end = high_resolution_clock::now();

        double latency_us = duration_cast<nanoseconds>(end - start).count() / 1000.0;
        latency_tracker_.record(latency_us);

        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(state.iterations());
    setLatencyCounters(state);
}

BENCHMARK_F(LatencyBenchFixture, ReadLatency_LargeValues)(benchmark::State& state) {
    // Pre-populate with large values
    DeterministicRNG setup_rng(42);
    for (int i = 0; i < 1000; ++i) {
        BaseEntity entity("large_" + std::to_string(i), BaseEntity::FieldMap{
            {"data", setup_rng.generateString(10000)} // 10KB values
        });
        db_->put("entity:" + entity.getPrimaryKey(), entity.serialize());
    }

    DeterministicRNG rng(42);
    for (auto _ : state) {
        int id = rng.generateInt(0, 999);
        std::string key = "entity:large_" + std::to_string(id);

        auto start = high_resolution_clock::now();
        auto value = db_->get(key);
        auto end = high_resolution_clock::now();

        double latency_us = duration_cast<nanoseconds>(end - start).count() / 1000.0;
        latency_tracker_.record(latency_us);

        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(state.iterations());
    setLatencyCounters(state);
}

// ============================================================================
// Write Latency Benchmarks
// ============================================================================

BENCHMARK_F(LatencyBenchFixture, WriteLatency_SmallValues)(benchmark::State& state) {
    DeterministicRNG rng(42);
    int counter = 0;

    for (auto _ : state) {
        std::string key = "write_small_" + std::to_string(counter++);
        BaseEntity entity(key, BaseEntity::FieldMap{
            {"value", rng.generateInt(0, 1000000)}
        });

        auto start = high_resolution_clock::now();
        db_->put("entity:" + key, entity.serialize());
        auto end = high_resolution_clock::now();

        double latency_us = duration_cast<nanoseconds>(end - start).count() / 1000.0;
        latency_tracker_.record(latency_us);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(LatencyBenchFixture, WriteLatency_LargeValues)(benchmark::State& state) {
    DeterministicRNG rng(42);
    int counter = 0;

    for (auto _ : state) {
        std::string key = "write_large_" + std::to_string(counter++);
        BaseEntity entity(key, BaseEntity::FieldMap{
            {"data", rng.generateString(10000)} // 10KB values
        });

        auto start = high_resolution_clock::now();
        db_->put("entity:" + key, entity.serialize());
        auto end = high_resolution_clock::now();

        double latency_us = duration_cast<nanoseconds>(end - start).count() / 1000.0;
        latency_tracker_.record(latency_us);
    }

    state.SetItemsProcessed(state.iterations());
    setLatencyCounters(state);
}

// ============================================================================
// Mixed Workload Latency
// ============================================================================

BENCHMARK_F(LatencyBenchFixture, MixedLatency_70Read30Write)(benchmark::State& state) {
    DeterministicRNG rng(42);
    int write_counter = 0;

    for (auto _ : state) {
        double op = static_cast<double>(rng.next() % 100) / 100.0;

        auto start = high_resolution_clock::now();
        if (op < 0.7) {
            // Read operation
            int id = rng.generateInt(0, 9999);
            std::string key = "entity:warmup_" + std::to_string(id);
            auto value = db_->get(key);
            benchmark::DoNotOptimize(value);
        } else {
            // Write operation
            std::string key = "mixed_" + std::to_string(write_counter++);
            BaseEntity entity(key, BaseEntity::FieldMap{
                {"value", rng.generateInt(0, 1000000)}
            });
            db_->put("entity:" + key, entity.serialize());
        }
        auto end = high_resolution_clock::now();

        double latency_us = duration_cast<nanoseconds>(end - start).count() / 1000.0;
        latency_tracker_.record(latency_us);
    }

    state.SetItemsProcessed(state.iterations());
    setLatencyCounters(state);
}

// ============================================================================
// Sequential vs Random Access Latency
// ============================================================================

BENCHMARK_F(LatencyBenchFixture, SequentialReadLatency)(benchmark::State& state) {
    int counter = 0;

    for (auto _ : state) {
        int id = counter++ % 10000;
        std::string key = "entity:warmup_" + std::to_string(id);

        auto start = high_resolution_clock::now();
        auto value = db_->get(key);
        auto end = high_resolution_clock::now();

        double latency_us = duration_cast<nanoseconds>(end - start).count() / 1000.0;
        latency_tracker_.record(latency_us);

        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(state.iterations());
    setLatencyCounters(state);
}

BENCHMARK_F(LatencyBenchFixture, RandomReadLatency)(benchmark::State& state) {
    DeterministicRNG rng(42);

    for (auto _ : state) {
        int id = rng.generateInt(0, 9999);
        std::string key = "entity:warmup_" + std::to_string(id);

        auto start = high_resolution_clock::now();
        auto value = db_->get(key);
        auto end = high_resolution_clock::now();

        double latency_us = duration_cast<nanoseconds>(end - start).count() / 1000.0;
        latency_tracker_.record(latency_us);

        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(state.iterations());
    setLatencyCounters(state);
}

// ============================================================================
// Batch Operation Latency
// ============================================================================

BENCHMARK_F(LatencyBenchFixture, BatchWriteLatency)(benchmark::State& state) {
    const int batch_size = state.range(0);
    int counter = 0;

    for (auto _ : state) {
        DeterministicRNG rng(42 + counter);

        auto start = high_resolution_clock::now();
        for (int i = 0; i < batch_size; ++i) {
            std::string key = "batch_" + std::to_string(counter++) + "_" + std::to_string(i);
            BaseEntity entity(key, BaseEntity::FieldMap{
                {"value", rng.generateInt(0, 1000000)}
            });
            db_->put("entity:" + key, entity.serialize());
        }
        auto end = high_resolution_clock::now();

        double latency_us = duration_cast<nanoseconds>(end - start).count() / 1000.0;
        latency_tracker_.record(latency_us);
    }

    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK_REGISTER_F(LatencyBenchFixture, BatchWriteLatency)
    ->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// Cache Hit/Miss Latency
// ============================================================================

BENCHMARK_F(LatencyBenchFixture, CacheHitLatency)(benchmark::State& state) {
    // Read same key repeatedly (should be cached)
    const std::string hot_key = "entity:warmup_5000";

    // Prime the cache
    for (int i = 0; i < 100; ++i) {
        auto value = db_->get(hot_key);
        benchmark::DoNotOptimize(value);
    }

    for (auto _ : state) {
        auto start = high_resolution_clock::now();
        auto value = db_->get(hot_key);
        auto end = high_resolution_clock::now();

        double latency_us = duration_cast<nanoseconds>(end - start).count() / 1000.0;
        latency_tracker_.record(latency_us);

        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(state.iterations());
    setLatencyCounters(state);
}

BENCHMARK_F(LatencyBenchFixture, CacheMissLatency)(benchmark::State& state) {
    DeterministicRNG rng(42);

    // Pre-create keys but don't access them (cold)
    for (int i = 10000; i < 20000; ++i) {
        BaseEntity entity("cold_" + std::to_string(i), BaseEntity::FieldMap{
            {"value", rng.generateInt(0, 1000000)}
        });
        db_->put("entity:" + entity.getPrimaryKey(), entity.serialize());
    }

    int counter = 10000;
    for (auto _ : state) {
        std::string key = "entity:cold_" + std::to_string(counter++);
        if (counter >= 20000) counter = 10000;

        auto start = high_resolution_clock::now();
        auto value = db_->get(key);
        auto end = high_resolution_clock::now();

        double latency_us = duration_cast<nanoseconds>(end - start).count() / 1000.0;
        latency_tracker_.record(latency_us);

        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(state.iterations());
    setLatencyCounters(state);
}

// ============================================================================
// Extreme Value Latency
// ============================================================================

BENCHMARK_F(LatencyBenchFixture, TinyValueLatency)(benchmark::State& state) {
    DeterministicRNG rng(42);
    int counter = 0;

    for (auto _ : state) {
        std::string key = "tiny_" + std::to_string(counter++);
        BaseEntity entity(key, BaseEntity::FieldMap{
            {"v", static_cast<int64_t>(1)} // Minimal value
        });

        auto start = high_resolution_clock::now();
        db_->put("entity:" + key, entity.serialize());
        auto end = high_resolution_clock::now();

        double latency_us = duration_cast<nanoseconds>(end - start).count() / 1000.0;
        latency_tracker_.record(latency_us);
    }

    state.SetItemsProcessed(state.iterations());
    setLatencyCounters(state);
}

BENCHMARK_F(LatencyBenchFixture, HugeValueLatency)(benchmark::State& state) {
    DeterministicRNG rng(42);
    int counter = 0;

    for (auto _ : state) {
        std::string key = "huge_" + std::to_string(counter++);
        BaseEntity entity(key, BaseEntity::FieldMap{
            {"data", rng.generateString(1000000)} // 1MB value
        });

        auto start = high_resolution_clock::now();
        db_->put("entity:" + key, entity.serialize());
        auto end = high_resolution_clock::now();

        double latency_us = duration_cast<nanoseconds>(end - start).count() / 1000.0;
        latency_tracker_.record(latency_us);
    }

    state.SetItemsProcessed(state.iterations());
    setLatencyCounters(state);
}
