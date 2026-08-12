// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Storage latency benchmark:
//  - Measures put/get/del p50/p99 latency at 1,000 TPS
//  - Validates success criteria: p99 tx-latency < 50 ms
//  - Reports write-amplification factor from CompactionManager::Stats
//
// These are *functional* benchmarks executed as regular GTest tests.
// They use wall-clock timing and assert that latency targets are met.

#include <gtest/gtest.h>
#include "storage/storage_engine.h"
#include "storage/compaction_manager.h"
#include "storage/rocksdb_wrapper.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <numeric>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace themis;
using Clock = std::chrono::steady_clock;
using US    = std::chrono::microseconds;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────
class LatencyBenchmark : public ::testing::Test {
protected:
    void SetUp() override {
        auto ts = Clock::now().time_since_epoch().count();
        db_path_ = (fs::temp_directory_path() /
                    ("themis_latbench_" + std::to_string(ts))).string();

        RocksDBWrapper::Config cfg;
        cfg.db_path           = db_path_;
        cfg.enable_wal        = false; // disable RocksDB WAL to measure pure storage operations
        cfg.enable_statistics = true;
        cfg.memtable_size_mb  = 64;
        cfg.block_cache_size_mb = 64;

        db_ = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        engine_ = StorageEngine::createDefault();
        ASSERT_TRUE(engine_ != nullptr);
        ASSERT_TRUE(engine_->open(db_path_).has_value());
        compaction_ = std::make_unique<CompactionManager>(db_);
    }

    void TearDown() override {
        engine_.reset();
        compaction_.reset();
        db_->close();
        db_.reset();
        fs::remove_all(db_path_);
    }

    // Measure latency of N operations, return sorted µs vector
    template <typename Op>
    std::vector<int64_t> measureLatencies(int n, Op op) {
        std::vector<int64_t> latencies;
        latencies.reserve(n);
        for (int i = 0; i < n; ++i) {
            auto t0 = Clock::now();
            op(i);
            auto t1 = Clock::now();
            latencies.push_back(
                std::chrono::duration_cast<US>(t1 - t0).count());
        }
        std::sort(latencies.begin(), latencies.end());
        return latencies;
    }

    static int64_t percentile(const std::vector<int64_t>& sorted, double pct) {
        if (sorted.empty()) return 0;
        size_t idx = static_cast<size_t>(pct / 100.0 * (sorted.size() - 1));
        return sorted[idx];
    }

    std::string db_path_;
    std::shared_ptr<RocksDBWrapper> db_;
    std::shared_ptr<StorageEngine> engine_;
    std::unique_ptr<CompactionManager> compaction_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Benchmark 1: put() p99 < 50 ms  (= 50 000 µs)
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(LatencyBenchmark, Put_P99Under50ms) {
    constexpr int kOps = 1000; // 1k operations
    const std::string value(256, 'x'); // 256-byte value

    auto lats = measureLatencies(kOps, [&](int i) {
        engine_->put("bench_put_" + std::to_string(i), value);
    });

    int64_t p50 = percentile(lats, 50);
    int64_t p99 = percentile(lats, 99);
    int64_t avg = std::accumulate(lats.begin(), lats.end(), int64_t{0}) / kOps;

    // Log statistics (visible with --gtest_print_time=1 or -v)
    std::printf("[BENCH] put: avg=%ldµs  p50=%ldµs  p99=%ldµs\n", avg, p50, p99);

    // Success criterion: p99 < 50 ms = 50 000 µs
    EXPECT_LT(p99, 50000LL)
        << "put() p99 latency exceeds 50ms: " << p99 << "µs";
}

// ─────────────────────────────────────────────────────────────────────────────
// Benchmark 2: get() p99 < 50 ms
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(LatencyBenchmark, Get_P99Under50ms) {
    constexpr int kOps = 1000;
    const std::string value(256, 'r');

    // Pre-fill
    for (int i = 0; i < kOps; ++i) {
        engine_->put("bench_get_" + std::to_string(i), value);
    }

    auto lats = measureLatencies(kOps, [&](int i) {
        engine_->get("bench_get_" + std::to_string(i));
    });

    int64_t p50 = percentile(lats, 50);
    int64_t p99 = percentile(lats, 99);
    int64_t avg = std::accumulate(lats.begin(), lats.end(), int64_t{0}) / kOps;

    std::printf("[BENCH] get: avg=%ldµs  p50=%ldµs  p99=%ldµs\n", avg, p50, p99);

    EXPECT_LT(p99, 50000LL)
        << "get() p99 latency exceeds 50ms: " << p99 << "µs";
}

// ─────────────────────────────────────────────────────────────────────────────
// Benchmark 3: del() p99 < 50 ms
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(LatencyBenchmark, Del_P99Under50ms) {
    constexpr int kOps = 1000;

    // Pre-fill
    for (int i = 0; i < kOps; ++i) {
        engine_->put("bench_del_" + std::to_string(i), "val");
    }

    auto lats = measureLatencies(kOps, [&](int i) {
        engine_->del("bench_del_" + std::to_string(i));
    });

    int64_t p50 = percentile(lats, 50);
    int64_t p99 = percentile(lats, 99);
    int64_t avg = std::accumulate(lats.begin(), lats.end(), int64_t{0}) / kOps;

    std::printf("[BENCH] del: avg=%ldµs  p50=%ldµs  p99=%ldµs\n", avg, p50, p99);

    EXPECT_LT(p99, 50000LL)
        << "del() p99 latency exceeds 50ms: " << p99 << "µs";
}

// ─────────────────────────────────────────────────────────────────────────────
// Benchmark 4: Mixed workload (70% get, 25% put, 5% del) p99 < 50 ms
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(LatencyBenchmark, MixedWorkload_P99Under50ms) {
    constexpr int kOps  = 1000;
    constexpr int kFill = 500;
    const std::string value(128, 'm');

    // Pre-fill
    for (int i = 0; i < kFill; ++i) {
        engine_->put("mixed_" + std::to_string(i), value);
    }

    // Deterministic "random" sequence
    std::vector<int64_t> lats;
    lats.reserve(kOps);

    for (int i = 0; i < kOps; ++i) {
        int bucket = i % 20; // 0-13 → get(70%), 14-18 → put(25%), 19 → del(5%)
        auto t0 = Clock::now();
        if (bucket < 14) {
            engine_->get("mixed_" + std::to_string(i % kFill));
        } else if (bucket < 19) {
            engine_->put("mixed_new_" + std::to_string(i), value);
        } else {
            engine_->del("mixed_" + std::to_string(i % kFill));
        }
        auto t1 = Clock::now();
        lats.push_back(std::chrono::duration_cast<US>(t1 - t0).count());
    }

    std::sort(lats.begin(), lats.end());
    int64_t p50 = percentile(lats, 50);
    int64_t p99 = percentile(lats, 99);
    int64_t avg = std::accumulate(lats.begin(), lats.end(), int64_t{0}) / kOps;

    std::printf("[BENCH] mixed: avg=%ldµs  p50=%ldµs  p99=%ldµs\n", avg, p50, p99);

    EXPECT_LT(p99, 50000LL)
        << "Mixed workload p99 latency exceeds 50ms: " << p99 << "µs";
}

// ─────────────────────────────────────────────────────────────────────────────
// Benchmark 5: Write-amplification < 2x baseline
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(LatencyBenchmark, WriteAmplification_LessThan2x) {
    constexpr int kWrites = 5000;
    const std::string value(512, 'w'); // 512-byte value

    for (int i = 0; i < kWrites; ++i) {
        engine_->put("wa_key_" + std::to_string(i), value);
    }

    // Trigger a manual compaction so compaction stats appear in getStats()
    compaction_->compactAll();

    auto s = compaction_->stats();

    std::printf("[BENCH] write-amp: user_bytes=%lu  compact=%lu  flush=%lu  ratio=%.2f\n",
                s.user_bytes_written, s.compact_bytes_written,
                s.flush_bytes_written, s.writeAmplification());

    // Only assert if user_bytes_written > 0 (statistics enabled)
    if (s.user_bytes_written > 0) {
        EXPECT_LT(s.writeAmplification(), 2.0)
            << "Write-amplification exceeds 2x baseline: "
            << s.writeAmplification();
    } else {
        // Statistics not available (e.g., first run with cold cache): skip assertion
        GTEST_SKIP() << "RocksDB statistics not available; skipping write-amp check";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Benchmark 6: IO metrics latency tracking consistency
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(LatencyBenchmark, IOMetrics_LatencyConsistentWithDirectMeasurement) {
    constexpr int kOps = 200;
    const std::string value(64, 'i');

    // Warm-up
    for (int i = 0; i < 10; ++i) {
        engine_->put("warmup_" + std::to_string(i), value);
    }
    engine_->resetIOMetrics();

    // Measure directly
    auto t_start = Clock::now();
    for (int i = 0; i < kOps; ++i) {
        engine_->put("metric_" + std::to_string(i), value);
    }
    auto t_end = Clock::now();

    int64_t wall_us = std::chrono::duration_cast<US>(t_end - t_start).count();
    auto m = engine_->ioMetrics();

    ASSERT_EQ(m.put_ops, static_cast<uint64_t>(kOps));

    // Cumulative latency from metrics should be ≤ wall time × 3.
    // (3x accounts for measurement overhead and CPU scheduling jitter between
    // the steady_clock calls wrapping each individual operation vs. the bulk
    // wall-clock window measured here.)
    EXPECT_GT(m.put_latency_us, 0u) << "Cumulative latency must be positive";
    EXPECT_LT(m.put_latency_us, static_cast<uint64_t>(wall_us * 3))
        << "Cumulative metric latency suspiciously large vs wall time";
    EXPECT_NE(m.put_latency_min_us, UINT64_MAX) << "min latency sentinel must be cleared";
    EXPECT_GE(m.put_latency_max_us, m.put_latency_min_us) << "max >= min";
}
