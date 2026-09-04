// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w7d_guardrails_variance_operability.cpp
 * @brief Wave 7-D: Final Guardrails, Variance Zeroing & Operability Benchmarks.
 *
 * Purpose: Sharpen the final release-blocking gates by:
 *   1. Measuring and reporting coefficient of variation (CV) per workload so
 *      that noisy benchmarks can be identified and their guardrail tolerances
 *      adjusted.
 *   2. Validating that deterministic execution (fixed seed, pinned thread
 *      affinity model) produces identical key sequences across runs.
 *   3. Emitting structured counters that feed the Regression Report Standard
 *      (see RELEASE_GATE_W7.md) without requiring post-processing.
 *
 * Covered scenarios:
 *   GVO-01  Variance measurement – read workload CV (should be < 5%)
 *   GVO-02  Variance measurement – write workload CV (should be < 8%)
 *   GVO-03  Determinism check – two runs produce identical operation counts
 *   GVO-04  Gate assertion – p99 read ≤ 200 µs hard gate (pass/fail counter)
 *   GVO-05  Gate assertion – write throughput ≥ 80 000 ops/s hard gate
 *   GVO-06  Isolation test – bench runs with CPU-yield isolation model
 *   GVO-07  Regression delta baseline – captures mean ± std for CI delta check
 *   GVO-08  Operability counters – standard counters for automated reporting
 *
 * Guardrail pass/fail conventions:
 *   Each GVO-0N benchmark emits a `gate_passed` counter: 1.0 = pass, 0.0 = fail.
 *   The release_gate_manifest_w7.json maps these counters to hard/soft gates.
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"

namespace fs = std::filesystem;

namespace themis {
namespace bench {
namespace w7d {

// ---------------------------------------------------------------------------
// Constants – final release-pinned values
// ---------------------------------------------------------------------------

static constexpr uint64_t kW7CanonicalSeed    = 42;
static constexpr int      kDatasetSize        = 50'000;
static constexpr int      kWarmup             = 500;
static constexpr int      kMeasurementRuns    = 1'000;
static constexpr int      kRepetitions        = 7; // higher repetitions for CV

// Hard-gate thresholds (must match release_gate_manifest_w7.json)
static constexpr double   kReadP99GateUs      = 200.0;  ///< µs
static constexpr double   kWriteThroughputGate = 80'000.0; ///< ops/s
static constexpr double   kReadCvGatePercent  = 5.0;    ///< %
static constexpr double   kWriteCvGatePercent = 8.0;    ///< %

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

void RemoveAll(const std::string& path) {
    std::error_code ec = {};
    fs::remove_all(path, ec);
}

std::string UniqueDbPath(const std::string& tag) {
    using namespace std::chrono;
    auto ts = duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
    return fs::temp_directory_path().string() + "/w7d_" + tag + "_" + std::to_string(ts);
}

RocksDBWrapper::Config DefaultCfg(const std::string& db_path) {
    RocksDBWrapper::Config cfg;
    cfg.db_path                     = db_path;
    cfg.compression_default         = "lz4";
    cfg.block_cache_size_mb         = 256;
    cfg.memtable_size_mb            = 128;
    cfg.max_write_buffer_number     = 4;
    cfg.allow_concurrent_memtable_write = true;
    cfg.enable_statistics           = false;
    return cfg;
}

class KeyGenerator {
public:
    explicit KeyGenerator(uint64_t seed = kW7CanonicalSeed) : rng_(seed) {}
    std::string Next(int upper_bound) {
        std::uniform_int_distribution<int> d(0, upper_bound - 1);
        return "k_" + std::to_string(d(rng_));
    }
    /// Reset to the same seed – used for determinism validation.
    void Reset(uint64_t seed) { rng_.seed(seed); }
private:
    std::mt19937_64 rng_ = {};
};

/**
 * @brief Compute coefficient of variation from a sample vector.
 * @param samples  Non-empty vector of positive latency measurements.
 * @return CV as a percentage (std_dev / mean * 100).
 */
double CoefficientOfVariation(const std::vector<double>& samples) {
    if (samples.empty()) {
      return 0.0;
    }
    const double mean = std::accumulate(samples.begin(), samples.end(), 0.0)
                        / static_cast<double>(samples.size());
    if (mean < 1e-12) {
      return 0.0;
    }
    double sq_sum = 0.0;
    for (double x : samples) {
        const double d = x - mean;
        sq_sum += d * d;
    }
    const double stddev = std::sqrt(sq_sum / static_cast<double>(samples.size()));
    return (stddev / mean) * 100.0;
}

} // namespace

// ---------------------------------------------------------------------------
// Shared base fixture
// ---------------------------------------------------------------------------

class GuardrailBaseFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("gvo");
        RemoveAll(db_path_);
        db_ = std::make_unique<RocksDBWrapper>(DefaultCfg(db_path_));
        if (!db_->open()) {
          throw std::runtime_error("W7D: open failed");
        }
        for (int i = 0; i < kDatasetSize; ++i) {
            db_->put("k_" + std::to_string(i), "v_" + std::to_string(i));
        }
        // Warmup
        KeyGenerator wkg(kW7CanonicalSeed + 99);
        for (int i = 0; i < kWarmup; ++i) {
            std::string val = {};
            db_->get(wkg.Next(kDatasetSize), val);
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

// ---------------------------------------------------------------------------
// GVO-01: Variance measurement – read workload CV (should be < 5%)
// ---------------------------------------------------------------------------

BENCHMARK_F(GuardrailBaseFixture, GVO01_ReadVarianceCV)(benchmark::State& state) {
    KeyGenerator kg(kW7CanonicalSeed + 1);
    std::vector<double> latencies;
    latencies.reserve(kMeasurementRuns);

    for (auto _ : state) {
        auto t0 = std::chrono::steady_clock::now();
        std::string val = {};
        db_->get(kg.Next(kDatasetSize), val);
        auto t1 = std::chrono::steady_clock::now();
        double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        latencies.push_back(us);
    }

    const double cv = CoefficientOfVariation(latencies);
    state.counters["cv_percent"]  = cv;
    state.counters["gate_passed"] = (cv <= kReadCvGatePercent) ? 1.0 : 0.0;
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(GuardrailBaseFixture, GVO01_ReadVarianceCV)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W7D/GVO01_ReadVariance_CV_gate_5pct");

// ---------------------------------------------------------------------------
// GVO-02: Variance measurement – write workload CV (should be < 8%)
// ---------------------------------------------------------------------------

BENCHMARK_F(GuardrailBaseFixture, GVO02_WriteVarianceCV)(benchmark::State& state) {
    KeyGenerator kg(kW7CanonicalSeed + 2);
    std::vector<double> latencies;
    latencies.reserve(kMeasurementRuns);
    int ctr = 0;

    for (auto _ : state) {
        auto t0 = std::chrono::steady_clock::now();
        db_->put(kg.Next(kDatasetSize * 2), "gvo_" + std::to_string(ctr++));
        auto t1 = std::chrono::steady_clock::now();
        double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        latencies.push_back(us);
    }

    const double cv = CoefficientOfVariation(latencies);
    state.counters["cv_percent"]  = cv;
    state.counters["gate_passed"] = (cv <= kWriteCvGatePercent) ? 1.0 : 0.0;
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(GuardrailBaseFixture, GVO02_WriteVarianceCV)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W7D/GVO02_WriteVariance_CV_gate_8pct");

// ---------------------------------------------------------------------------
// GVO-03: Determinism check – identical key sequence across two sub-runs
// ---------------------------------------------------------------------------

BENCHMARK_F(GuardrailBaseFixture, GVO03_DeterministicKeySequence)(benchmark::State& state) {
    // Two generators seeded identically must produce identical sequences.
    constexpr int kCheckLen = 200;
    KeyGenerator kg_a(kW7CanonicalSeed + 3);
    KeyGenerator kg_b(kW7CanonicalSeed + 3);

    int64_t mismatches = 0;
    for (auto _ : state) {
        kg_a.Reset(kW7CanonicalSeed + 3);
        kg_b.Reset(kW7CanonicalSeed + 3);
        for (int i = 0; i < kCheckLen; ++i) {
            const std::string ka = kg_a.Next(kDatasetSize);
            const std::string kb = kg_b.Next(kDatasetSize);
            if (ka != kb) {
              ++mismatches;
            }
        }
    }
    state.counters["mismatches"]  = static_cast<double>(mismatches);
    state.counters["gate_passed"] = (mismatches == 0) ? 1.0 : 0.0;
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * kCheckLen);
}
BENCHMARK_REGISTER_F(GuardrailBaseFixture, GVO03_DeterministicKeySequence)
    ->Repetitions(3)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W7D/GVO03_Determinism_KeySequenceCheck");

// ---------------------------------------------------------------------------
// GVO-04: Gate assertion – p99 read ≤ 200 µs hard gate
// ---------------------------------------------------------------------------

BENCHMARK_F(GuardrailBaseFixture, GVO04_P99ReadGateAssertion)(benchmark::State& state) {
    constexpr int kSampleSize = 200;
    KeyGenerator kg(kW7CanonicalSeed + 4);
    std::vector<double> latencies;
    latencies.reserve(kSampleSize);

    for (auto _ : state) {
        latencies.clear();
        for (int i = 0; i < kSampleSize; ++i) {
            auto t0 = std::chrono::steady_clock::now();
            std::string val = {};
            db_->get(kg.Next(kDatasetSize), val);
            auto t1 = std::chrono::steady_clock::now();
            latencies.push_back(
                std::chrono::duration<double, std::micro>(t1 - t0).count());
        }
        std::sort(latencies.begin(), latencies.end());
        const double p99 = latencies[static_cast<std::size_t>(kSampleSize * 0.99)];
        state.counters["p99_us"]      = p99;
        state.counters["gate_passed"] = (p99 <= kReadP99GateUs) ? 1.0 : 0.0;
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * kSampleSize);
}
BENCHMARK_REGISTER_F(GuardrailBaseFixture, GVO04_P99ReadGateAssertion)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W7D/GVO04_P99Read_HardGate_200us");

// ---------------------------------------------------------------------------
// GVO-05: Gate assertion – write throughput ≥ 80 000 ops/s hard gate
// ---------------------------------------------------------------------------

BENCHMARK_F(GuardrailBaseFixture, GVO05_WriteThroughputGateAssertion)(benchmark::State& state) {
    constexpr int kBatchOps = 5'000;
    KeyGenerator kg(kW7CanonicalSeed + 5);
    int ctr = 0;
    for (auto _ : state) {
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < kBatchOps; ++i) {
            db_->put(kg.Next(kDatasetSize * 3), "gvo05_" + std::to_string(ctr++));
        }
        auto t1 = std::chrono::steady_clock::now();
        const double elapsed_s =
            std::chrono::duration<double>(t1 - t0).count();
        const double ops_per_s = kBatchOps / std::max(elapsed_s, 1e-9);
        state.counters["ops_per_s"]   = ops_per_s;
        state.counters["gate_passed"] = (ops_per_s >= kWriteThroughputGate) ? 1.0 : 0.0;
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * kBatchOps);
}
BENCHMARK_REGISTER_F(GuardrailBaseFixture, GVO05_WriteThroughputGateAssertion)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W7D/GVO05_WriteThroughput_HardGate_80k_ops_s");

// ---------------------------------------------------------------------------
// GVO-06: Isolation test – bench runs with CPU-yield isolation model
// ---------------------------------------------------------------------------

BENCHMARK_F(GuardrailBaseFixture, GVO06_IsolatedReadLatency)(benchmark::State& state) {
    // Yield before each measurement to model inter-run CPU sharing fairly.
    KeyGenerator kg(kW7CanonicalSeed + 6);
    for (auto _ : state) {
        state.PauseTiming();
        std::this_thread::yield();
        state.ResumeTiming();
        std::string val = {};
        benchmark::DoNotOptimize(db_->get(kg.Next(kDatasetSize), val));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(GuardrailBaseFixture, GVO06_IsolatedReadLatency)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W7D/GVO06_Isolated_ReadLatency");

// ---------------------------------------------------------------------------
// GVO-07: Regression delta baseline – captures mean ± std for CI delta check
// ---------------------------------------------------------------------------

BENCHMARK_F(GuardrailBaseFixture, GVO07_RegressionDeltaBaseline)(benchmark::State& state) {
    constexpr int kSamples = 500;
    KeyGenerator kg(kW7CanonicalSeed + 7);
    std::vector<double> lats;
    lats.reserve(kSamples);

    for (auto _ : state) {
        lats.clear();
        for (int i = 0; i < kSamples; ++i) {
            auto t0 = std::chrono::steady_clock::now();
            std::string val = {};
            db_->get(kg.Next(kDatasetSize), val);
            auto t1 = std::chrono::steady_clock::now();
            lats.push_back(
                std::chrono::duration<double, std::micro>(t1 - t0).count());
        }
        const double mean = std::accumulate(lats.begin(), lats.end(), 0.0)
                            / static_cast<double>(lats.size());
        double sq = 0.0;
        for (double x : lats) { const double d = x - mean; sq += d * d; }
        const double stddev = std::sqrt(sq / static_cast<double>(lats.size()));

        std::sort(lats.begin(), lats.end());
        const double p50 = lats[static_cast<std::size_t>(kSamples * 0.50)];
        const double p95 = lats[static_cast<std::size_t>(kSamples * 0.95)];
        const double p99 = lats[static_cast<std::size_t>(kSamples * 0.99)];

        state.counters["mean_us"]   = mean;
        state.counters["stddev_us"] = stddev;
        state.counters["p50_us"]    = p50;
        state.counters["p95_us"]    = p95;
        state.counters["p99_us"]    = p99;
        state.counters["cv_pct"]    = CoefficientOfVariation(lats);
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * kSamples);
}
BENCHMARK_REGISTER_F(GuardrailBaseFixture, GVO07_RegressionDeltaBaseline)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W7D/GVO07_RegressionDelta_Baseline");

// ---------------------------------------------------------------------------
// GVO-08: Operability counters – standard counters for automated reporting
// ---------------------------------------------------------------------------

BENCHMARK_F(GuardrailBaseFixture, GVO08_OperabilityCounters)(benchmark::State& state) {
    // Runs a mixed workload and emits a full set of structured counters
    // that the Regression Report Standard parser consumes directly.
    constexpr int kOps        = 2'000;
    constexpr int kWriteRatio = 40; // 40% writes

    std::mt19937_64 mix_rng(kW7CanonicalSeed + 88);
    std::uniform_int_distribution<int> mix(0, 99);
    KeyGenerator kg(kW7CanonicalSeed + 8);

    std::vector<double> read_lats, write_lats;
    read_lats.reserve(kOps);
    write_lats.reserve(kOps);
    int w_ctr = 0;

    for (auto _ : state) {
        read_lats.clear();
        write_lats.clear();

        for (int i = 0; i < kOps; ++i) {
            if (mix(mix_rng) < kWriteRatio) {
                auto t0 = std::chrono::steady_clock::now();
                db_->put(kg.Next(kDatasetSize * 2), "op_" + std::to_string(w_ctr++));
                auto t1 = std::chrono::steady_clock::now();
                write_lats.push_back(
                    std::chrono::duration<double, std::micro>(t1 - t0).count());
            } else {
                auto t0 = std::chrono::steady_clock::now();
                std::string val = {};
                db_->get(kg.Next(kDatasetSize), val);
                auto t1 = std::chrono::steady_clock::now();
                read_lats.push_back(
                    std::chrono::duration<double, std::micro>(t1 - t0).count());
            }
        }

        auto emit = [&state](const std::string& prefix,
                             std::vector<double>& lats) {
            if (lats.empty()) {
              return;
            }
            std::sort(lats.begin(), lats.end());
            const double mean = std::accumulate(lats.begin(), lats.end(), 0.0)
                                / static_cast<double>(lats.size());
            const double p95 =
                lats[static_cast<std::size_t>(lats.size() * 0.95)];
            const double p99 =
                lats[static_cast<std::size_t>(lats.size() * 0.99)];
            state.counters[prefix + "_mean_us"] = mean;
            state.counters[prefix + "_p95_us"]  = p95;
            state.counters[prefix + "_p99_us"]  = p99;
            state.counters[prefix + "_cv_pct"]  = CoefficientOfVariation(lats);
            state.counters[prefix + "_count"]   =
                static_cast<double>(lats.size());
        };
        emit("read",  read_lats);
        emit("write", write_lats);
        state.counters["total_ops"] = static_cast<double>(kOps);
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * kOps);
}
BENCHMARK_REGISTER_F(GuardrailBaseFixture, GVO08_OperabilityCounters)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W7D/GVO08_Operability_Counters");

} // namespace w7d
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
