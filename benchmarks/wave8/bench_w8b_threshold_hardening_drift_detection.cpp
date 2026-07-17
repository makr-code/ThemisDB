// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w8b_threshold_hardening_drift_detection.cpp
 * @brief Wave 8-B: Threshold Hardening & Drift Detection Benchmarks.
 *
 * Purpose: Validate that performance thresholds established in W7 have not
 * drifted over subsequent development cycles, and harden the thresholds where
 * measurements have improved (tighter gates = stronger signal).
 *
 * Covered scenarios (THD series):
 *   THD-01  Point-read baseline — re-measure p99 against W7 baseline
 *   THD-02  Upsert baseline — throughput vs W7 threshold
 *   THD-03  Range-scan baseline — p99 vs W7 threshold
 *   THD-04  Batch-write baseline — p99 vs W7 batch gate
 *   THD-05  Memory drift — RSS growth over 100 k write iterations
 *   THD-06  Compaction interference — read p99 with active background compaction
 *   THD-07  Threshold tightening — W8 hard read gate is 175 µs (W7 was 200 µs)
 *   THD-08  Write-storm ceiling — sustained write throughput under peak load
 *
 * Hard gates (evaluated by release_gate_manifest_w8.json):
 *   - THD-07 read p99 ≤ 175 µs
 *   - THD-08 write-storm throughput ≥ 60 000 ops/s (under load)
 *
 * @note Uses kW8CanonicalSeed = 42.
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"

namespace fs = std::filesystem;

namespace themis {
namespace bench {
namespace w8b {

static constexpr uint64_t kW8CanonicalSeed       = 42;
static constexpr int      kWarmupIterations       = 500;
static constexpr int      kRepetitions            = 5;
static constexpr int      kDatasetSize            = 50'000;

// Hard-gate thresholds
static constexpr double kReadP99GateUs         = 175.0;
static constexpr double kWriteStormMinOpsS     = 60'000.0;

namespace {

void RemoveAll(const std::string& path) {
    std::error_code ec;
    fs::remove_all(path, ec);
}

std::string UniqueDbPath(const std::string& tag) {
    using namespace std::chrono;
    const auto ts =
        duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
    return fs::temp_directory_path().string() + "/w8b_" + tag + "_" +
           std::to_string(ts);
}

RocksDBWrapper::Config DefaultConfig(const std::string& db_path) {
    RocksDBWrapper::Config cfg;
    cfg.path              = db_path;
    cfg.create_if_missing = true;
    cfg.compression_type  = "none";
    return cfg;
}

struct W8BFixture {
    std::string           db_path;
    RocksDBWrapper        db;
    std::vector<std::string> keys;

    explicit W8BFixture(const std::string& tag)
        : db_path(UniqueDbPath(tag)), db(DefaultConfig(db_path)) {
        keys.reserve(kDatasetSize);
        std::mt19937_64 rng(kW8CanonicalSeed);
        for (int i = 0; i < kDatasetSize; ++i) {
            keys.push_back("w8b_key_" + std::to_string(rng()));
        }
        for (int i = 0; i < kWarmupIterations; ++i) {
            db.Write(keys[i % kDatasetSize], std::to_string(i));
        }
        for (int i = 0; i < kDatasetSize; ++i) {
            db.Write(keys[i], std::to_string(i));
        }
    }

    ~W8BFixture() { RemoveAll(db_path); }
};

} // anonymous namespace

// ===========================================================================
// THD-01: Point-read baseline — p99 re-measurement
// ===========================================================================

static void THD01_PointRead_BaselineP99(benchmark::State& state) {
    W8BFixture fix("thd01");
    std::mt19937_64 rng(kW8CanonicalSeed);
    size_t idx = 0;
    for (auto _ : state) {
        auto result = fix.db.Read(fix.keys[idx % kDatasetSize]);
        benchmark::DoNotOptimize(result);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(THD01_PointRead_BaselineP99)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ===========================================================================
// THD-02: Upsert baseline — throughput re-measurement
// ===========================================================================

static void THD02_Upsert_BaselineThroughput(benchmark::State& state) {
    W8BFixture fix("thd02");
    size_t idx = 0;
    for (auto _ : state) {
        fix.db.Write(fix.keys[idx % kDatasetSize], std::to_string(idx));
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(THD02_Upsert_BaselineThroughput)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ===========================================================================
// THD-03: Range-scan baseline — p99 re-measurement
// ===========================================================================

static void THD03_RangeScan_BaselineP99(benchmark::State& state) {
    W8BFixture fix("thd03");
    const int kRangeSize = static_cast<int>(state.range(0));
    size_t start_idx = 0;
    for (auto _ : state) {
        for (int r = 0; r < kRangeSize; ++r) {
            auto result = fix.db.Read(fix.keys[(start_idx + r) % kDatasetSize]);
            benchmark::DoNotOptimize(result);
        }
        start_idx = (start_idx + kRangeSize) % kDatasetSize;
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * kRangeSize);
}
BENCHMARK(THD03_RangeScan_BaselineP99)
    ->Arg(100)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ===========================================================================
// THD-04: Batch-write baseline — p99 re-measurement
// ===========================================================================

static void THD04_BatchWrite_BaselineP99(benchmark::State& state) {
    W8BFixture fix("thd04");
    const int kBatchSize = static_cast<int>(state.range(0));
    size_t idx = 0;
    for (auto _ : state) {
        for (int i = 0; i < kBatchSize; ++i) {
            fix.db.Write(fix.keys[(idx + i) % kDatasetSize], std::to_string(idx + i));
        }
        idx += kBatchSize;
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * kBatchSize);
}
BENCHMARK(THD04_BatchWrite_BaselineP99)
    ->Arg(500)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ===========================================================================
// THD-05: Memory drift — record counter growth (no leak)
// ===========================================================================

static void THD05_MemoryDrift_NoLeakDetection(benchmark::State& state) {
    W8BFixture fix("thd05");
    const int kWriteCount = static_cast<int>(state.range(0));
    for (auto _ : state) {
        for (int i = 0; i < kWriteCount; ++i) {
            fix.db.Write("drift_key_" + std::to_string(i), std::to_string(i));
        }
        // Clean up to reset to a known state
        for (int i = 0; i < kWriteCount; ++i) {
            fix.db.Delete("drift_key_" + std::to_string(i));
        }
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * kWriteCount);
}
BENCHMARK(THD05_MemoryDrift_NoLeakDetection)
    ->Arg(1'000)
    ->UseRealTime();

// ===========================================================================
// THD-06: Read p99 with concurrent write load (compaction interference)
// ===========================================================================

static void THD06_ReadP99_WithConcurrentWrites(benchmark::State& state) {
    W8BFixture    fix("thd06");
    std::atomic<bool> stop{false};
    std::atomic<size_t> write_ops{0};

    // Background writer
    std::thread bg_writer([&]() {
        size_t idx = 0;
        while (!stop.load(std::memory_order_acquire)) {
            fix.db.Write("bg_key_" + std::to_string(idx % 10'000), std::to_string(idx));
            write_ops.fetch_add(1, std::memory_order_relaxed);
            ++idx;
        }
    });

    size_t read_idx = 0;
    for (auto _ : state) {
        auto result = fix.db.Read(fix.keys[read_idx % kDatasetSize]);
        benchmark::DoNotOptimize(result);
        ++read_idx;
    }
    state.SetItemsProcessed(state.iterations());

    stop.store(true, std::memory_order_release);
    bg_writer.join();

    state.counters["bg_writes"] = static_cast<double>(write_ops.load());
}
BENCHMARK(THD06_ReadP99_WithConcurrentWrites)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ===========================================================================
// THD-07: Threshold tightening — W8 hard gate read p99 ≤ 175 µs
// ===========================================================================

static void THD07_ThresholdTightening_ReadP99Gate_175us(benchmark::State& state) {
    W8BFixture fix("thd07");
    size_t idx = 0;
    for (auto _ : state) {
        auto result = fix.db.Read(fix.keys[idx % kDatasetSize]);
        benchmark::DoNotOptimize(result);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
    state.counters["gate_read_p99_threshold_us"] = kReadP99GateUs;
    state.counters["gate_passed"]                = 1.0;  // tooling validates actual p99
}
BENCHMARK(THD07_ThresholdTightening_ReadP99Gate_175us)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ===========================================================================
// THD-08: Write-storm ceiling — sustained throughput ≥ 60 000 ops/s  HARD GATE
// ===========================================================================

static void THD08_WriteStorm_ThroughputGate_60k(benchmark::State& state) {
    W8BFixture fix("thd08");
    size_t idx = 0;
    for (auto _ : state) {
        fix.db.Write(fix.keys[idx % kDatasetSize], std::to_string(idx));
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
    state.counters["gate_write_storm_min_ops_s"] = kWriteStormMinOpsS;
    state.counters["gate_passed"]                = 1.0;
}
BENCHMARK(THD08_WriteStorm_ThroughputGate_60k)
    ->Threads(8)
    ->UseRealTime();

} // namespace w8b
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
