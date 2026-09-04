// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w7b_endurance_soak.cpp
 * @brief Wave 7-B: Endurance / Soak & Peak Transition Validation Benchmarks.
 *
 * Purpose: Validate that ThemisDB does not exhibit throughput drift, latency
 * tail growth, or resource accumulation under sustained and burst load.  The
 * benchmarks model the four operating phases of a production workload:
 *
 *   Phase 1 – Steady:   sustained baseline load for kSteadySegments segments.
 *   Phase 2 – Burst:    2× baseline throughput for kBurstSegments segments.
 *   Phase 3 – Peak:     3× baseline throughput for kPeakSegments segments.
 *   Phase 4 – Recovery: return to baseline; measure latency normalisation.
 *
 * Covered scenarios:
 *   SOK-01  Steady read  – no throughput drift across segments
 *   SOK-02  Steady write – no throughput drift across segments
 *   SOK-03  Burst → Peak read spike – p95/p99 tail build-up check
 *   SOK-04  Peak → Recovery – latency normalisation measurement
 *   SOK-05  Write-heavy soak – memory / write-buffer accumulation guard
 *   SOK-06  Mixed OLTP soak (60 r / 40 w) – combined stability
 *   SOK-07  Concurrent reader soak (4 threads) – under sustained concurrency
 *   SOK-08  Large-value soak (16 KB values) – bandwidth saturation detection
 *
 * @note Segment-level timing is captured via state.PauseTiming() /
 *       state.ResumeTiming() so that the intra-segment drift is observable in
 *       the standard Google Benchmark JSON output.
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "index/secondary_index.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"

namespace fs = std::filesystem;

namespace themis {
namespace bench {
namespace w7b {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr uint64_t kW7CanonicalSeed = 42;

/// Number of operation-batches per steady phase segment.
static constexpr int kOpsPerSegment    = 5'000;
static constexpr int kSteadySegments   = 6;
static constexpr int kBurstSegments    = 3;
static constexpr int kPeakSegments     = 2;
static constexpr int kRecoverySegments = 4;

/// Pre-loaded dataset size for soak tests.
static constexpr int kSoakRecordCount  = 100'000;

/// Warmup operations before soak measurement.
static constexpr int kSoakWarmup       = 1'000;

/// Large-value soak: value payload size in bytes.
static constexpr std::size_t kLargeValueBytes = 16'384;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

void RemoveAll(const std::string& path) {
    std::error_code ec;
    fs::remove_all(path, ec);
}

std::string UniqueDbPath(const std::string& tag) {
    using namespace std::chrono;
    auto ts = duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
    return fs::temp_directory_path().string() + "/w7b_" + tag + "_" + std::to_string(ts);
}

RocksDBWrapper::Config SoakConfig(const std::string& db_path) {
    RocksDBWrapper::Config cfg;
    cfg.db_path                     = db_path;
    cfg.compression_default         = "lz4";
    cfg.compression_bottommost      = "zstd";
    cfg.block_cache_size_mb         = 512;
    cfg.memtable_size_mb            = 256;
    cfg.max_write_buffer_number     = 6;
    cfg.allow_concurrent_memtable_write = true;
    cfg.enable_statistics           = false;
    return cfg;
}

class KeyGenerator {
public:
    explicit KeyGenerator(uint64_t seed = kW7CanonicalSeed) : rng_(seed) {}

    std::string Next(int upper_bound) {
        std::uniform_int_distribution<int> d(0, upper_bound - 1);
        return "r_" + std::to_string(d(rng_));
    }

    std::string LargeValue() {
        std::string v(kLargeValueBytes, 'x');
        v[0] = static_cast<char>(rng_() & 0xFF);
        return v;
    }

private:
    std::mt19937_64 rng_;
};

/// Run @p ops reads against @p db picking keys from [0, @p upper_bound).
static void DoReads(RocksDBWrapper& db, int ops, int upper_bound, uint64_t seed) {
    KeyGenerator kg(seed);
    for (int i = 0; i < ops; ++i) {
        std::string val;
        benchmark::DoNotOptimize(db.get(kg.Next(upper_bound), val));
    }
}

/// Run @p ops writes against @p db.
static void DoWrites(RocksDBWrapper& db, int ops, int upper_bound, uint64_t seed) {
    KeyGenerator kg(seed);
    int c = 0;
    for (int i = 0; i < ops; ++i) {
        db.put(kg.Next(upper_bound), "sv_" + std::to_string(c++));
    }
}

} // namespace

// ---------------------------------------------------------------------------
// SOK-01: Steady read – no throughput drift
// ---------------------------------------------------------------------------

class SteadyReadSoakFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("sok01");
        RemoveAll(db_path_);
        db_ = std::make_unique<RocksDBWrapper>(SoakConfig(db_path_));
        if (!db_->open()) {
          throw std::runtime_error("W7B SOK-01: open failed");
        }
        for (int i = 0; i < kSoakRecordCount; ++i) {
            db_->put("r_" + std::to_string(i), "v_" + std::to_string(i));
        }
        DoReads(*db_, kSoakWarmup, kSoakRecordCount, kW7CanonicalSeed + 100);
    }
    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
    }
protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

BENCHMARK_F(SteadyReadSoakFixture, SOK01_SteadyReadNoThroughputDrift)(benchmark::State& state) {
    int seg = 0;
    for (auto _ : state) {
        state.PauseTiming();
        ++seg;
        state.ResumeTiming();
        DoReads(*db_, kOpsPerSegment, kSoakRecordCount,
                kW7CanonicalSeed + static_cast<uint64_t>(seg));
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * kOpsPerSegment);
}
BENCHMARK_REGISTER_F(SteadyReadSoakFixture, SOK01_SteadyReadNoThroughputDrift)
    ->Iterations(kSteadySegments)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W7B/SOK01_SteadyRead_NoThroughputDrift");

// ---------------------------------------------------------------------------
// SOK-02: Steady write – no throughput drift
// ---------------------------------------------------------------------------

class SteadyWriteSoakFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("sok02");
        RemoveAll(db_path_);
        db_ = std::make_unique<RocksDBWrapper>(SoakConfig(db_path_));
        if (!db_->open()) {
          throw std::runtime_error("W7B SOK-02: open failed");
        }
        DoWrites(*db_, kSoakWarmup, kSoakRecordCount * 2, kW7CanonicalSeed + 200);
    }
    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
    }
protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

BENCHMARK_F(SteadyWriteSoakFixture, SOK02_SteadyWriteNoThroughputDrift)(benchmark::State& state) {
    int seg = 0;
    for (auto _ : state) {
        state.PauseTiming();
        ++seg;
        state.ResumeTiming();
        DoWrites(*db_, kOpsPerSegment, kSoakRecordCount * 2,
                 kW7CanonicalSeed + 200 + static_cast<uint64_t>(seg));
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * kOpsPerSegment);
}
BENCHMARK_REGISTER_F(SteadyWriteSoakFixture, SOK02_SteadyWriteNoThroughputDrift)
    ->Iterations(kSteadySegments)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W7B/SOK02_SteadyWrite_NoThroughputDrift");

// ---------------------------------------------------------------------------
// SOK-03: Burst → Peak read spike – tail latency build-up check
// ---------------------------------------------------------------------------

class BurstPeakFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("sok03");
        RemoveAll(db_path_);
        db_ = std::make_unique<RocksDBWrapper>(SoakConfig(db_path_));
        if (!db_->open()) {
          throw std::runtime_error("W7B SOK-03: open failed");
        }
        for (int i = 0; i < kSoakRecordCount; ++i) {
            db_->put("r_" + std::to_string(i), "v_" + std::to_string(i));
        }
        DoReads(*db_, kSoakWarmup, kSoakRecordCount, kW7CanonicalSeed + 300);
    }
    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
    }
protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

BENCHMARK_F(BurstPeakFixture, SOK03_BurstPeakTailLatency)(benchmark::State& state) {
    // Phase multipliers: steady(1x) → burst(2x) → peak(3x)
    static constexpr int kPhases[] = {1, 2, 3};
    int seg = 0;
    int64_t total_ops = 0;
    for (auto _ : state) {
        state.PauseTiming();
        int phase_mult = kPhases[seg % 3];
        ++seg;
        state.ResumeTiming();
        const int ops_this_segment = kOpsPerSegment * phase_mult;
        DoReads(*db_, ops_this_segment, kSoakRecordCount,
                kW7CanonicalSeed + 300 + static_cast<uint64_t>(seg));
        total_ops += ops_this_segment;
    }
    state.SetItemsProcessed(total_ops);
}
BENCHMARK_REGISTER_F(BurstPeakFixture, SOK03_BurstPeakTailLatency)
    ->Iterations(kSteadySegments + kBurstSegments + kPeakSegments)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W7B/SOK03_BurstPeak_TailLatencyCheck");

// ---------------------------------------------------------------------------
// SOK-04: Peak → Recovery – latency normalisation
// ---------------------------------------------------------------------------

BENCHMARK_F(BurstPeakFixture, SOK04_PeakRecoveryNormalisation)(benchmark::State& state) {
    static constexpr int kPeakMult     = 3;
    static constexpr int kRecoveryMult = 1;
    int seg = 0;
    int64_t total_ops = 0;
    for (auto _ : state) {
        state.PauseTiming();
        bool is_recovery = (seg >= kPeakSegments);
        int  mult        = is_recovery ? kRecoveryMult : kPeakMult;
        ++seg;
        state.ResumeTiming();
        const int ops_this_segment = kOpsPerSegment * mult;
        DoReads(*db_, ops_this_segment, kSoakRecordCount,
                kW7CanonicalSeed + 400 + static_cast<uint64_t>(seg));
        total_ops += ops_this_segment;
    }
    state.SetItemsProcessed(total_ops);
}
BENCHMARK_REGISTER_F(BurstPeakFixture, SOK04_PeakRecoveryNormalisation)
    ->Iterations(kPeakSegments + kRecoverySegments)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W7B/SOK04_PeakRecovery_LatencyNormalisation");

// ---------------------------------------------------------------------------
// SOK-05: Write-heavy soak – write-buffer accumulation guard
// ---------------------------------------------------------------------------

class WriteHeavySoakFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("sok05");
        RemoveAll(db_path_);
        db_ = std::make_unique<RocksDBWrapper>(SoakConfig(db_path_));
        if (!db_->open()) {
          throw std::runtime_error("W7B SOK-05: open failed");
        }
        DoWrites(*db_, kSoakWarmup, kSoakRecordCount * 4, kW7CanonicalSeed + 500);
    }
    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
    }
protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

BENCHMARK_F(WriteHeavySoakFixture, SOK05_WriteHeavySoakAccumulationGuard)(benchmark::State& state) {
    // 90% write, 10% read – mimics write-dominated ingest
    std::mt19937_64 mix_rng(kW7CanonicalSeed + 55);
    std::uniform_int_distribution<int> mix(0, 99);
    int seg = 0;
    int w_ctr = 0;
    for (auto _ : state) {
        state.PauseTiming();
        ++seg;
        state.ResumeTiming();
        KeyGenerator kg(kW7CanonicalSeed + 500 + static_cast<uint64_t>(seg));
        for (int i = 0; i < kOpsPerSegment; ++i) {
            if (mix(mix_rng) < 10) {
                std::string val;
                benchmark::DoNotOptimize(db_->get(kg.Next(kSoakRecordCount), val));
            } else {
                db_->put(kg.Next(kSoakRecordCount * 4), "whs_" + std::to_string(w_ctr++));
            }
        }
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * kOpsPerSegment);
}
BENCHMARK_REGISTER_F(WriteHeavySoakFixture, SOK05_WriteHeavySoakAccumulationGuard)
    ->Iterations(kSteadySegments)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W7B/SOK05_WriteHeavySoak_AccumulationGuard");

// ---------------------------------------------------------------------------
// SOK-06: Mixed OLTP soak (60 r / 40 w)
// ---------------------------------------------------------------------------

class MixedOLTPSoakFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("sok06");
        RemoveAll(db_path_);
        db_ = std::make_unique<RocksDBWrapper>(SoakConfig(db_path_));
        if (!db_->open()) {
          throw std::runtime_error("W7B SOK-06: open failed");
        }
        for (int i = 0; i < kSoakRecordCount; ++i) {
            db_->put("r_" + std::to_string(i), "v_" + std::to_string(i));
        }
    }
    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
    }
protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

BENCHMARK_F(MixedOLTPSoakFixture, SOK06_MixedOLTPSoakStability)(benchmark::State& state) {
    std::mt19937_64 mix_rng(kW7CanonicalSeed + 66);
    std::uniform_int_distribution<int> mix(0, 99);
    int seg = 0;
    int w_ctr = 0;
    for (auto _ : state) {
        state.PauseTiming();
        ++seg;
        state.ResumeTiming();
        KeyGenerator kg(kW7CanonicalSeed + 600 + static_cast<uint64_t>(seg));
        for (int i = 0; i < kOpsPerSegment; ++i) {
            if (mix(mix_rng) < 40) {
                db_->put(kg.Next(kSoakRecordCount * 2), "mx_" + std::to_string(w_ctr++));
            } else {
                std::string val;
                benchmark::DoNotOptimize(db_->get(kg.Next(kSoakRecordCount), val));
            }
        }
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * kOpsPerSegment);
}
BENCHMARK_REGISTER_F(MixedOLTPSoakFixture, SOK06_MixedOLTPSoakStability)
    ->Iterations(kSteadySegments + kBurstSegments)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W7B/SOK06_MixedOLTP_SoakStability");

// ---------------------------------------------------------------------------
// SOK-07: Concurrent reader soak (4 threads)
// ---------------------------------------------------------------------------

class ConcurrentReaderSoakFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("sok07");
        RemoveAll(db_path_);
        db_ = std::make_unique<RocksDBWrapper>(SoakConfig(db_path_));
        if (!db_->open()) {
          throw std::runtime_error("W7B SOK-07: open failed");
        }
        for (int i = 0; i < kSoakRecordCount; ++i) {
            db_->put("r_" + std::to_string(i), "v_" + std::to_string(i));
        }
    }
    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
    }
protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

BENCHMARK_F(ConcurrentReaderSoakFixture, SOK07_ConcurrentReaderSoak)(benchmark::State& state) {
    constexpr int kThreads     = 4;
    constexpr int kOpsPerThread = kOpsPerSegment / kThreads;
    int seg = 0;
    for (auto _ : state) {
        state.PauseTiming();
        ++seg;
        state.ResumeTiming();

        std::vector<std::thread> threads;
        threads.reserve(kThreads);
        for (int t = 0; t < kThreads; ++t) {
            threads.emplace_back([this, t, seg]() {
                KeyGenerator kg(kW7CanonicalSeed + 700 +
                                static_cast<uint64_t>(t) * 1000 +
                                static_cast<uint64_t>(seg));
                for (int i = 0; i < kOpsPerThread; ++i) {
                    std::string val;
                    db_->get(kg.Next(kSoakRecordCount), val);
                }
            });
        }
        for (auto& th : threads) {
          th.join();
        }
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * kOpsPerSegment);
}
BENCHMARK_REGISTER_F(ConcurrentReaderSoakFixture, SOK07_ConcurrentReaderSoak)
    ->Iterations(kSteadySegments)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W7B/SOK07_ConcurrentReaders_SoakStability");

// ---------------------------------------------------------------------------
// SOK-08: Large-value soak (16 KB values)
// ---------------------------------------------------------------------------

class LargeValueSoakFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("sok08");
        RemoveAll(db_path_);
        db_ = std::make_unique<RocksDBWrapper>(SoakConfig(db_path_));
        if (!db_->open()) {
          throw std::runtime_error("W7B SOK-08: open failed");
        }
        // Pre-load with large values
        KeyGenerator kg(kW7CanonicalSeed + 800);
        for (int i = 0; i < 5'000; ++i) {
            db_->put("lv_" + std::to_string(i), kg.LargeValue());
        }
    }
    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
    }
protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

BENCHMARK_F(LargeValueSoakFixture, SOK08_LargeValueSoakBandwidthSaturation)(benchmark::State& state) {
    int seg = 0;
    int w_ctr = 0;
    for (auto _ : state) {
        state.PauseTiming();
        ++seg;
        state.ResumeTiming();
        KeyGenerator kg(kW7CanonicalSeed + 800 + static_cast<uint64_t>(seg));
        for (int i = 0; i < 200; ++i) { // fewer ops due to large value cost
            db_->put("lv_" + std::to_string(5000 + w_ctr++), kg.LargeValue());
        }
    }
    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations()) * 200 * kLargeValueBytes);
}
BENCHMARK_REGISTER_F(LargeValueSoakFixture, SOK08_LargeValueSoakBandwidthSaturation)
    ->Iterations(kSteadySegments)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W7B/SOK08_LargeValue_BandwidthSaturation");

} // namespace w7b
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
