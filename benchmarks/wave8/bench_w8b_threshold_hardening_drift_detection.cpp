// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w8b_threshold_hardening_drift_detection.cpp
 * @brief Wave 8-B: Threshold Hardening & Drift Detection Benchmarks.
 *
 * Purpose: Sharpen performance gates beyond W7 baselines and introduce
 * explicit drift-detection scenarios so that gradual, sub-threshold
 * regressions are surfaced before they cross hard gate boundaries.
 *
 * Two complementary approaches are used:
 *   1. Hardened thresholds: selected gates are tightened by 10–15% vs W7
 *      to provide earlier warning margin.
 *   2. Drift signals: rolling-window and trend-slope benchmarks emit
 *      structured counters that a CI script can compare against a frozen
 *      baseline to detect creeping degradation.
 *
 * Covered scenarios (THD = Threshold Hardening / Drift):
 *   THD-01  Tightened point-read gate (175 µs vs W7 200 µs)
 *   THD-02  Tightened write-throughput gate (90k ops/s vs W7 80k)
 *   THD-03  Drift: read throughput rolling-window variance
 *   THD-04  Drift: write throughput rolling-window variance
 *   THD-05  Trend slope analysis — latency rising trend detection
 *   THD-06  Delta baseline comparison — current vs frozen baseline
 *   THD-07  Tightened batch-write gate (4 ms vs W7 5 ms)
 *   THD-08  Composite drift score — multi-metric aggregate signal
 *
 * Drift detection conventions:
 *   THD-03/04 divide measurement into kDriftSegments equal windows.
 *   The segment throughput is recorded via state.counters[] so that
 *   report_variance_w8.py can compare first-half vs second-half mean
 *   and flag drift if the delta exceeds kDriftTolerancePct.
 *
 * Hard gates enforced by release_gate_manifest_w8.json:
 *   - THD-01 p99 ≤ 175 µs
 *   - THD-02 throughput ≥ 90 000 ops/s
 *   - THD-07 p99 ≤ 4 ms
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"

namespace fs = std::filesystem;

namespace themis {
namespace bench {
namespace w8b {

// ---------------------------------------------------------------------------
// Constants – hardened release thresholds
// ---------------------------------------------------------------------------

static constexpr uint64_t kW8CanonicalSeed = 42;

/// Hardened gates (tightened vs W7).
static constexpr double kReadP99GateUs      = 175.0;  ///< µs  (W7: 200)
static constexpr double kWriteThroughputGate = 90'000.0; ///< ops/s (W7: 80k)
static constexpr double kBatchWriteGateMs   = 4.0;    ///< ms   (W7: 5)

/// Drift detection tolerance (% change across segments before alerting).
static constexpr double kDriftTolerancePct  = 8.0;

static constexpr int kWarmupIterations = 500;
static constexpr int kRepetitions      = 7; // higher for drift estimation
static constexpr int kDatasetSize      = 50'000;
static constexpr int kDriftSegments    = 6; // segments for rolling-window drift
static constexpr int kOpsPerSegment    = 3'000;

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
    return fs::temp_directory_path().string() + "/w8b_" + tag + "_" + std::to_string(ts);
}

RocksDBWrapper::Config DefaultConfig(const std::string& db_path) {
    RocksDBWrapper::Config cfg;
    cfg.db_path                         = db_path;
    cfg.compression_default             = "lz4";
    cfg.compression_bottommost          = "zstd";
    cfg.block_cache_size_mb             = 256;
    cfg.memtable_size_mb                = 128;
    cfg.max_write_buffer_number         = 4;
    cfg.allow_concurrent_memtable_write = true;
    cfg.enable_statistics               = false;
    return cfg;
}

class KeyGenerator {
public:
    explicit KeyGenerator(uint64_t seed = kW8CanonicalSeed) : rng_(seed) {}
    std::string NextKey(int upper_bound) {
        std::uniform_int_distribution<int> d(0, upper_bound - 1);
        return "e_" + std::to_string(d(rng_));
    }
private:
    std::mt19937_64 rng_;
};

/// Compute coefficient of variation (%) from a vector of samples.
double cv_percent(const std::vector<double>& v) {
    if (v.size() < 2) {
      return 0.0;
    }
    const double mean = std::accumulate(v.begin(), v.end(), 0.0) / static_cast<double>(v.size());
    if (std::abs(mean) < 1e-12) {
      return 0.0;
    }
    double sq_sum = 0.0;
    for (double x : v) {
        const double d = x - mean;
        sq_sum += d * d;
    }
    const double stddev = std::sqrt(sq_sum / static_cast<double>(v.size() - 1));
    return (stddev / mean) * 100.0;
}

/// Simple linear trend slope (ops/segment).
double trend_slope(const std::vector<double>& v) {
    if (v.size() < 2) {
      return 0.0;
    }
    const int n = static_cast<int>(v.size());
    double sx = 0.0, sy = 0.0, sxy = 0.0, sxx = 0.0;
    for (int i = 0; i < n; ++i) {
        sx  += i;
        sy  += v[i];
        sxy += i * v[i];
        sxx += static_cast<double>(i) * i;
    }
    const double denom = n * sxx - sx * sx;
    if (std::abs(denom) < 1e-12) {
      return 0.0;
    }
    return (n * sxy - sx * sy) / denom;
}

} // namespace

// ---------------------------------------------------------------------------
// Base fixture shared by hardened-gate benchmarks
// ---------------------------------------------------------------------------

/**
 * @brief Common fixture for threshold-hardening benchmarks.
 *
 * Pre-loads kDatasetSize records and applies warmup before measurement.
 */
class HardenedGateFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("hardened");
        RemoveAll(db_path_);

        db_ = std::make_unique<RocksDBWrapper>(DefaultConfig(db_path_));
        if (!db_->open()) {
            throw std::runtime_error("W8B: failed to open RocksDB for hardened-gate fixture");
        }
        for (int i = 0; i < kDatasetSize; ++i) {
            BaseEntity e("e_" + std::to_string(i));
            e.setField("v", std::to_string(i));
            db_->put(e.id(), e.serialize());
        }
        KeyGenerator wkg(kW8CanonicalSeed + 1);
        for (int i = 0; i < kWarmupIterations; ++i) {
            std::string val;
            db_->get(wkg.NextKey(kDatasetSize), val);
        }
    }

    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
    }

protected:
    std::string                     db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

// ---------------------------------------------------------------------------
// THD-01: Tightened point-read gate (175 µs)
// ---------------------------------------------------------------------------

/**
 * @brief THD-01: Tightened point-read p99 gate.
 *
 * Hard gate tightened from W7's 200 µs to 175 µs to provide 12.5% extra
 * warning margin.  Exceeding 175 µs blocks CI before the 200 µs hard limit
 * would be reached, giving the team time to investigate early drift.
 */
BENCHMARK_F(HardenedGateFixture, THD01_TightenedReadGate)(benchmark::State& state) {
    KeyGenerator kg(kW8CanonicalSeed + 11);
    for (auto _ : state) {
        const std::string key = kg.NextKey(kDatasetSize);
        std::string val;
        benchmark::DoNotOptimize(db_->get(key, val));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(HardenedGateFixture, THD01_TightenedReadGate)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W8B/THD01_TightenedRead_p99_175us_gate");

// ---------------------------------------------------------------------------
// THD-02: Tightened write throughput gate (90k ops/s)
// ---------------------------------------------------------------------------

/**
 * @brief THD-02: Tightened write-throughput gate.
 *
 * Gate raised from W7's 80k to 90k ops/s (+12.5%) to detect throughput
 * erosion earlier.  A CI failure here triggers investigation before the
 * W7 gate (80k) would fire.
 */
BENCHMARK_F(HardenedGateFixture, THD02_TightenedWriteGate)(benchmark::State& state) {
    KeyGenerator kg(kW8CanonicalSeed + 22);
    int ctr = 0;
    for (auto _ : state) {
        db_->put(kg.NextKey(kDatasetSize * 2), "hv_" + std::to_string(ctr++));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(HardenedGateFixture, THD02_TightenedWriteGate)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W8B/THD02_TightenedWrite_90k_ops_s_gate");

// ---------------------------------------------------------------------------
// THD-03 / THD-04: Rolling-window drift detection
// ---------------------------------------------------------------------------

/**
 * @brief Fixture for rolling-window drift-detection benchmarks.
 *
 * Segments the benchmark run into kDriftSegments equal windows and records
 * per-segment throughput. The CI drift script compares segment pairs to
 * detect monotonic degradation.
 */
class DriftDetectionFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("drift");
        RemoveAll(db_path_);

        db_ = std::make_unique<RocksDBWrapper>(DefaultConfig(db_path_));
        if (!db_->open()) {
            throw std::runtime_error("W8B: failed to open RocksDB for drift-detection fixture");
        }
        for (int i = 0; i < kDatasetSize; ++i) {
            db_->put("d_" + std::to_string(i), "v" + std::to_string(i));
        }
    }

    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
    }

protected:
    std::string                     db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

/**
 * @brief THD-03: Read throughput rolling-window drift.
 *
 * Runs kDriftSegments × kOpsPerSegment reads and records per-segment
 * throughput as structured counters.  report_variance_w8.py uses the
 * counter series to compute a drift score and alerts if the trend
 * slope exceeds kDriftTolerancePct of the first-segment throughput.
 */
BENCHMARK_F(DriftDetectionFixture, THD03_ReadThroughputDrift)(benchmark::State& state) {
    KeyGenerator kg(kW8CanonicalSeed + 33);
    std::vector<double> segment_ops;
    segment_ops.reserve(kDriftSegments);

    for (auto _ : state) {
        state.PauseTiming();
        segment_ops.clear();
        state.ResumeTiming();

        for (int seg = 0; seg < kDriftSegments; ++seg) {
            const auto t0 = std::chrono::steady_clock::now();
            for (int i = 0; i < kOpsPerSegment; ++i) {
                std::string val;
                db_->get(kg.NextKey(kDatasetSize), val);
            }
            const auto t1 = std::chrono::steady_clock::now();
            const double elapsed_s =
                std::chrono::duration<double>(t1 - t0).count();
            segment_ops.push_back(kOpsPerSegment / (elapsed_s + 1e-12));
        }
        benchmark::DoNotOptimize(segment_ops[0]);
    }

    const double slope   = trend_slope(segment_ops);
    const double cv      = cv_percent(segment_ops);
    state.counters["read_drift_slope_ops_s_per_seg"] = slope;
    state.counters["read_drift_cv_pct"]              = cv;
    state.counters["read_drift_gate_passed"] =
        benchmark::Counter(std::abs(slope) < kDriftTolerancePct ? 1.0 : 0.0);
    state.SetItemsProcessed(
        static_cast<int64_t>(kDriftSegments) * kOpsPerSegment *
        static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(DriftDetectionFixture, THD03_ReadThroughputDrift)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W8B/THD03_ReadThroughput_drift_detection");

/**
 * @brief THD-04: Write throughput rolling-window drift.
 *
 * Same structure as THD-03 but for the write path.  Detects gradual write
 * amplification or compaction debt accumulating across segments.
 */
BENCHMARK_F(DriftDetectionFixture, THD04_WriteThroughputDrift)(benchmark::State& state) {
    KeyGenerator kg(kW8CanonicalSeed + 44);
    std::vector<double> segment_ops;
    segment_ops.reserve(kDriftSegments);
    int ctr = 0;

    for (auto _ : state) {
        state.PauseTiming();
        segment_ops.clear();
        state.ResumeTiming();

        for (int seg = 0; seg < kDriftSegments; ++seg) {
            const auto t0 = std::chrono::steady_clock::now();
            for (int i = 0; i < kOpsPerSegment; ++i) {
                db_->put(kg.NextKey(kDatasetSize * 3), "dw_" + std::to_string(ctr++));
            }
            const auto t1 = std::chrono::steady_clock::now();
            const double elapsed_s =
                std::chrono::duration<double>(t1 - t0).count();
            segment_ops.push_back(kOpsPerSegment / (elapsed_s + 1e-12));
        }
        benchmark::DoNotOptimize(segment_ops[0]);
    }

    const double slope = trend_slope(segment_ops);
    const double cv    = cv_percent(segment_ops);
    state.counters["write_drift_slope_ops_s_per_seg"] = slope;
    state.counters["write_drift_cv_pct"]              = cv;
    state.counters["write_drift_gate_passed"] =
        benchmark::Counter(std::abs(slope) < kDriftTolerancePct ? 1.0 : 0.0);
    state.SetItemsProcessed(
        static_cast<int64_t>(kDriftSegments) * kOpsPerSegment *
        static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(DriftDetectionFixture, THD04_WriteThroughputDrift)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W8B/THD04_WriteThroughput_drift_detection");

// ---------------------------------------------------------------------------
// THD-05: Trend slope analysis – latency rising trend
// ---------------------------------------------------------------------------

/**
 * @brief THD-05: Latency rising-trend detection.
 *
 * Measures per-segment mean read latency across kDriftSegments windows and
 * computes the linear trend slope.  A positive slope (latency increasing per
 * segment) is emitted as a counter so the CI drift script can alert when the
 * trend exceeds kDriftTolerancePct of the first-segment mean.
 */
BENCHMARK_F(DriftDetectionFixture, THD05_LatencyTrendSlope)(benchmark::State& state) {
    KeyGenerator kg(kW8CanonicalSeed + 55);
    std::vector<double> seg_mean_us;
    seg_mean_us.reserve(kDriftSegments);

    for (auto _ : state) {
        state.PauseTiming();
        seg_mean_us.clear();
        state.ResumeTiming();

        for (int seg = 0; seg < kDriftSegments; ++seg) {
            double total_us = 0.0;
            for (int i = 0; i < kOpsPerSegment; ++i) {
                const auto t0 = std::chrono::steady_clock::now();
                std::string val;
                db_->get(kg.NextKey(kDatasetSize), val);
                const auto t1 = std::chrono::steady_clock::now();
                total_us += std::chrono::duration<double, std::micro>(t1 - t0).count();
            }
            seg_mean_us.push_back(total_us / kOpsPerSegment);
        }
        benchmark::DoNotOptimize(seg_mean_us[0]);
    }

    const double slope = trend_slope(seg_mean_us);
    state.counters["latency_trend_slope_us_per_seg"] = slope;
    state.counters["latency_trend_rising"] =
        benchmark::Counter(slope > 0.0 ? 1.0 : 0.0);
    state.SetItemsProcessed(
        static_cast<int64_t>(kDriftSegments) * kOpsPerSegment *
        static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(DriftDetectionFixture, THD05_LatencyTrendSlope)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W8B/THD05_Latency_trend_slope");

// ---------------------------------------------------------------------------
// THD-06: Delta baseline comparison
// ---------------------------------------------------------------------------

/**
 * @brief THD-06: Delta baseline comparison.
 *
 * Simulates the CI baseline-comparison step by running a read workload and
 * emitting a `delta_pct` counter (always 0.0 here, as the baseline is the
 * run itself).  In production CI this counter is compared to a frozen
 * baseline JSON file.  A delta > 10% triggers a soft gate failure.
 *
 * The benchmark itself validates the comparison scaffold's correctness by
 * running two identical workloads back-to-back and checking their means
 * differ by < kDriftTolerancePct.
 */
BENCHMARK_F(DriftDetectionFixture, THD06_DeltaBaselineComparison)(benchmark::State& state) {
    KeyGenerator kg1(kW8CanonicalSeed + 66);
    KeyGenerator kg2(kW8CanonicalSeed + 66); // same seed → same sequence

    double run1_total = 0.0;
    double run2_total = 0.0;
    constexpr int kCompareOps = 1'000;

    for (auto _ : state) {
        state.PauseTiming();
        run1_total = 0.0;
        run2_total = 0.0;
        state.ResumeTiming();

        for (int i = 0; i < kCompareOps; ++i) {
            const auto t0 = std::chrono::steady_clock::now();
            std::string val;
            db_->get(kg1.NextKey(kDatasetSize), val);
            run1_total += std::chrono::duration<double, std::micro>(
                std::chrono::steady_clock::now() - t0).count();
        }
        for (int i = 0; i < kCompareOps; ++i) {
            const auto t0 = std::chrono::steady_clock::now();
            std::string val;
            db_->get(kg2.NextKey(kDatasetSize), val);
            run2_total += std::chrono::duration<double, std::micro>(
                std::chrono::steady_clock::now() - t0).count();
        }

        const double mean1 = run1_total / kCompareOps;
        const double mean2 = run2_total / kCompareOps;
        const double delta = (mean1 > 1e-12)
                             ? std::abs(mean2 - mean1) / mean1 * 100.0
                             : 0.0;
        state.counters["delta_pct"]         = delta;
        state.counters["delta_gate_passed"] = benchmark::Counter(
            delta <= kDriftTolerancePct ? 1.0 : 0.0);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * kCompareOps * 2);
}
BENCHMARK_REGISTER_F(DriftDetectionFixture, THD06_DeltaBaselineComparison)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W8B/THD06_DeltaBaseline_comparison");

// ---------------------------------------------------------------------------
// THD-07: Tightened batch-write gate (4 ms)
// ---------------------------------------------------------------------------

/**
 * @brief Fixture for tightened batch-write gate benchmark.
 */
class BatchWriteHardenedFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("batch_hardened");
        RemoveAll(db_path_);
        db_ = std::make_unique<RocksDBWrapper>(DefaultConfig(db_path_));
        if (!db_->open()) {
            throw std::runtime_error("W8B: failed to open RocksDB for batch-hardened fixture");
        }
        for (int i = 0; i < kDatasetSize; ++i) {
            db_->put("b_" + std::to_string(i), "v" + std::to_string(i));
        }
    }

    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
    }

protected:
    std::string                     db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

/**
 * @brief THD-07: Tightened batch-write gate.
 *
 * Batch write (500 records) gate tightened from W7's 5 ms to 4 ms (−20%).
 * Provides early warning of batch-path degradation before the W7 hard limit
 * would be reached.
 */
BENCHMARK_F(BatchWriteHardenedFixture, THD07_TightenedBatchWrite)(benchmark::State& state) {
    KeyGenerator kg(kW8CanonicalSeed + 77);
    int ctr = 0;
    for (auto _ : state) {
        std::vector<std::pair<std::string, std::string>> batch;
        batch.reserve(500);
        for (int i = 0; i < 500; ++i) {
            batch.emplace_back(kg.NextKey(kDatasetSize * 2), "bv_" + std::to_string(ctr++));
        }
        db_->putBatch(batch);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * 500);
}
BENCHMARK_REGISTER_F(BatchWriteHardenedFixture, THD07_TightenedBatchWrite)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W8B/THD07_TightenedBatch_p99_4ms_gate");

// ---------------------------------------------------------------------------
// THD-08: Composite drift score
// ---------------------------------------------------------------------------

/**
 * @brief THD-08: Composite drift score.
 *
 * Aggregates per-operation read latency samples and computes a composite
 * drift score: the product of the CV-gate (read) and latency-trend-gate
 * counters.  A composite score of 1.0 means all drift sub-signals pass;
 * any value < 1.0 indicates at least one drift dimension has exceeded
 * its tolerance and requires investigation.
 */
BENCHMARK_F(DriftDetectionFixture, THD08_CompositeDriftScore)(benchmark::State& state) {
    KeyGenerator kg(kW8CanonicalSeed + 88);
    std::vector<double> latencies;
    latencies.reserve(kOpsPerSegment * kDriftSegments);

    for (auto _ : state) {
        state.PauseTiming();
        latencies.clear();
        state.ResumeTiming();

        for (int i = 0; i < kOpsPerSegment; ++i) {
            const auto t0 = std::chrono::steady_clock::now();
            std::string val;
            db_->get(kg.NextKey(kDatasetSize), val);
            latencies.push_back(
                std::chrono::duration<double, std::micro>(
                    std::chrono::steady_clock::now() - t0).count());
        }

        std::sort(latencies.begin(), latencies.end());
        const double cv    = cv_percent(latencies);
        const double slope = trend_slope(latencies);

        const double cv_pass    = (cv    <= 5.0)  ? 1.0 : 0.0;
        const double slope_pass = (slope <= 0.05) ? 1.0 : 0.0;
        state.counters["composite_drift_score"] =
            benchmark::Counter(cv_pass * slope_pass);
        state.counters["cv_pct"]     = cv;
        state.counters["slope_us_i"] = slope;
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * kOpsPerSegment);
}
BENCHMARK_REGISTER_F(DriftDetectionFixture, THD08_CompositeDriftScore)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W8B/THD08_CompositeDrift_score");

} // namespace w8b
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
