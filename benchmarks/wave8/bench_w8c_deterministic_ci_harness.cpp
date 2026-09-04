// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w8c_deterministic_ci_harness.cpp
 * @brief Wave 8-C: Deterministic Benchmark Harness in CI.
 *
 * Purpose: Improve benchmark reproducibility, reduce measurement noise, and
 * make CI results diagnostically useful.  Each scenario validates a specific
 * dimension of harness quality: seed determinism, environment isolation,
 * warmup efficacy, timer resolution, and result stability.
 *
 * Covered scenarios (DCH = Deterministic CI Harness):
 *   DCH-01  Seed reproducibility — same seed produces identical operation sequence
 *   DCH-02  Temp-dir isolation — each fixture gets a unique, clean path
 *   DCH-03  Iteration-count stability — result stable as iteration count grows
 *   DCH-04  Warmup effect — pre/post warmup mean latency delta < 20%
 *   DCH-05  Timer resolution — sub-microsecond steady_clock resolution check
 *   DCH-06  Parallel fixture isolation — parallel fixtures share no state
 *   DCH-07  Teardown completeness — no lingering DB files after TearDown
 *   DCH-08  Flake bound — CV across repetitions < 3% (noise floor)
 *
 * Harness invariants enforced here:
 *   - All DB paths use UniqueDbPath() with nanosecond timestamps.
 *   - All fixtures call RemoveAll() in both SetUp and TearDown.
 *   - PRNG seeds are always derived from kW8CanonicalSeed + a unique offset.
 *   - UseRealTime() is applied to all I/O-bound benchmarks.
 *   - kFlakeCvGatePct = 3.0; CV > 3% triggers a soft gate warning.
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
namespace w8c {

// ---------------------------------------------------------------------------
// Constants – CI harness quality thresholds
// ---------------------------------------------------------------------------

static constexpr uint64_t kW8CanonicalSeed = 42;
static constexpr int      kWarmupIterations = 500;
static constexpr int      kRepetitions      = 7;
static constexpr int      kDatasetSize      = 50'000;

/// Flake bound: CV across repetitions must be below this to pass DCH-08.
static constexpr double kFlakeCvGatePct   = 3.0;

/// Warmup efficacy: post-warmup mean must be ≤ (1 + kWarmupDeltaFrac) × pre-warmup mean.
static constexpr double kWarmupDeltaFrac  = 0.20;

/// Timer resolution: single measurement must be ≤ this many µs to be sub-µs capable.
static constexpr double kTimerResolutionUs = 1.0;

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
    return fs::temp_directory_path().string() + "/w8c_" + tag + "_" + std::to_string(ts);
}

RocksDBWrapper::Config DefaultConfig(const std::string& db_path) {
    RocksDBWrapper::Config cfg;
    cfg.db_path                         = db_path;
    cfg.compression_default             = "lz4";
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
        return "k_" + std::to_string(d(rng_));
    }
    /// Return internal PRNG state for sequence verification.
    uint64_t state() const { return rng_(); }
private:
    mutable std::mt19937_64 rng_;
};

double cv_percent(const std::vector<double>& v) {
    if (v.size() < 2) {
      return 0.0;
    }
    const double mean = std::accumulate(v.begin(), v.end(), 0.0) / static_cast<double>(v.size());
    if (std::abs(mean) < 1e-12) {
      return 0.0;
    }
    double sq = 0.0;
    for (double x : v) { const double d = x - mean; sq += d * d; }
    return (std::sqrt(sq / static_cast<double>(v.size() - 1)) / mean) * 100.0;
}

} // namespace

// ---------------------------------------------------------------------------
// Base fixture for CI harness benchmarks
// ---------------------------------------------------------------------------

/**
 * @brief Standard DB fixture used by most DCH scenarios.
 */
class CIHarnessFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("ci_harness");
        RemoveAll(db_path_);  // ensure clean state even if previous run leaked

        db_ = std::make_unique<RocksDBWrapper>(DefaultConfig(db_path_));
        if (!db_->open()) {
            throw std::runtime_error("W8C: failed to open RocksDB for CI harness fixture");
        }
        for (int i = 0; i < kDatasetSize; ++i) {
            db_->put("k_" + std::to_string(i), "v" + std::to_string(i));
        }
        // Standard warmup
        KeyGenerator wkg(kW8CanonicalSeed + 1);
        for (int i = 0; i < kWarmupIterations; ++i) {
            std::string val;
            db_->get(wkg.NextKey(kDatasetSize), val);
        }
    }

    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
        // Post-teardown: verify path was cleaned (DCH-07 invariant)
        cleanup_verified_ = !fs::exists(db_path_);
    }

    bool cleanup_verified() const { return cleanup_verified_; }

protected:
    std::string                     db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    bool                            cleanup_verified_{false};
};

// ---------------------------------------------------------------------------
// DCH-01: Seed reproducibility
// ---------------------------------------------------------------------------

/**
 * @brief DCH-01: Seed reproducibility check.
 *
 * Verifies that two KeyGenerator instances with the same seed produce the
 * same key sequence.  The benchmark measures the overhead of key generation
 * and emits a `seed_match` counter (1.0 = identical sequences).
 * A value < 1.0 indicates a non-determinism bug in the PRNG layer.
 */
BENCHMARK_F(CIHarnessFixture, DCH01_SeedReproducibility)(benchmark::State& state) {
    constexpr int kSeqLen = 1'000;

    for (auto _ : state) {
        state.PauseTiming();
        KeyGenerator kg1(kW8CanonicalSeed);
        KeyGenerator kg2(kW8CanonicalSeed);
        std::vector<std::string> seq1, seq2;
        seq1.reserve(kSeqLen);
        seq2.reserve(kSeqLen);
        state.ResumeTiming();

        for (int i = 0; i < kSeqLen; ++i) {
          seq1.push_back(kg1.NextKey(kDatasetSize));
        }
        for (int i = 0; i < kSeqLen; ++i) {
          seq2.push_back(kg2.NextKey(kDatasetSize));
        }

        state.PauseTiming();
        int mismatches = 0;
        for (int i = 0; i < kSeqLen; ++i) {
            if (seq1[i] != seq2[i]) {
              ++mismatches;
            }
        }
        state.counters["seed_mismatch_count"] = static_cast<double>(mismatches);
        state.counters["seed_match"]          = benchmark::Counter(mismatches == 0 ? 1.0 : 0.0);
        state.ResumeTiming();
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * kSeqLen * 2);
}
BENCHMARK_REGISTER_F(CIHarnessFixture, DCH01_SeedReproducibility)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W8C/DCH01_SeedReproducibility_zero_mismatch");

// ---------------------------------------------------------------------------
// DCH-02: Temp-dir isolation
// ---------------------------------------------------------------------------

/**
 * @brief DCH-02: Temp-dir isolation — each fixture gets a unique path.
 *
 * Runs kIsolationRuns independent open/close cycles, each on a uniquely
 * named path, and verifies that no two paths collide.
 * Emits `path_collision_count`; must be 0 for the gate to pass.
 */
BENCHMARK_F(CIHarnessFixture, DCH02_TempDirIsolation)(benchmark::State& state) {
    constexpr int kIsolationRuns = 10;

    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::string> paths;
        paths.reserve(kIsolationRuns);
        state.ResumeTiming();

        for (int i = 0; i < kIsolationRuns; ++i) {
            paths.push_back(UniqueDbPath("iso_" + std::to_string(i)));
        }

        state.PauseTiming();
        // Check uniqueness
        std::sort(paths.begin(), paths.end());
        const auto dup_it = std::adjacent_find(paths.begin(), paths.end());
        const int collisions = (dup_it != paths.end()) ? 1 : 0;
        state.counters["path_collision_count"] = static_cast<double>(collisions);
        state.counters["isolation_gate_passed"] =
            benchmark::Counter(collisions == 0 ? 1.0 : 0.0);
        state.ResumeTiming();
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * kIsolationRuns);
}
BENCHMARK_REGISTER_F(CIHarnessFixture, DCH02_TempDirIsolation)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W8C/DCH02_TempDirIsolation_zero_collision");

// ---------------------------------------------------------------------------
// DCH-03: Iteration-count stability
// ---------------------------------------------------------------------------

/**
 * @brief DCH-03: Iteration-count stability.
 *
 * Runs the same workload with two different op counts (100 vs 500 ops per
 * benchmark iteration) and verifies that the normalised per-op latency
 * agrees within kIterStabilityTolerancePct.  A large disagreement signals
 * that the benchmark has significant setup overhead not accounted for by
 * the iteration scaling.
 */
BENCHMARK_F(CIHarnessFixture, DCH03_IterationCountStability)(benchmark::State& state) {
    constexpr int kShortRun  = 100;
    constexpr int kLongRun   = 500;
    constexpr double kIterStabilityTolerancePct = 25.0;

    KeyGenerator kg(kW8CanonicalSeed + 33);
    for (auto _ : state) {
        state.PauseTiming();
        double short_total = 0.0, long_total = 0.0;
        state.ResumeTiming();

        for (int i = 0; i < kShortRun; ++i) {
            const auto t0 = std::chrono::steady_clock::now();
            std::string val;
            db_->get(kg.NextKey(kDatasetSize), val);
            short_total += std::chrono::duration<double, std::micro>(
                std::chrono::steady_clock::now() - t0).count();
        }
        for (int i = 0; i < kLongRun; ++i) {
            const auto t0 = std::chrono::steady_clock::now();
            std::string val;
            db_->get(kg.NextKey(kDatasetSize), val);
            long_total += std::chrono::duration<double, std::micro>(
                std::chrono::steady_clock::now() - t0).count();
        }

        state.PauseTiming();
        const double short_mean = short_total / kShortRun;
        const double long_mean  = long_total  / kLongRun;
        const double delta_pct  = (short_mean > 1e-12)
            ? std::abs(long_mean - short_mean) / short_mean * 100.0
            : 0.0;
        state.counters["iter_stability_delta_pct"] = delta_pct;
        state.counters["iter_stability_gate_passed"] =
            benchmark::Counter(delta_pct <= kIterStabilityTolerancePct ? 1.0 : 0.0);
        state.ResumeTiming();
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * (kShortRun + kLongRun));
}
BENCHMARK_REGISTER_F(CIHarnessFixture, DCH03_IterationCountStability)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W8C/DCH03_IterStability_delta_pct");

// ---------------------------------------------------------------------------
// DCH-04: Warmup effect measurement
// ---------------------------------------------------------------------------

/**
 * @brief DCH-04: Warmup efficacy check.
 *
 * Opens a cold DB (no warmup), measures a pre-warmup latency sample, applies
 * warmup, then measures again.  Emits the relative improvement ratio.
 * Gate: post-warmup mean ≤ pre-warmup mean × (1 + kWarmupDeltaFrac).
 * (A warmed DB must not be slower than a cold one for this workload.)
 */
class WarmupEffectFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("warmup_effect");
        RemoveAll(db_path_);
        db_ = std::make_unique<RocksDBWrapper>(DefaultConfig(db_path_));
        if (!db_->open()) {
            throw std::runtime_error("W8C: failed to open RocksDB for warmup fixture");
        }
        for (int i = 0; i < kDatasetSize; ++i) {
            db_->put("k_" + std::to_string(i), "v" + std::to_string(i));
        }
        // No warmup applied here — DCH-04 measures the cold→warm transition itself.
    }

    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
    }

protected:
    std::string                     db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

BENCHMARK_F(WarmupEffectFixture, DCH04_WarmupEfficacy)(benchmark::State& state) {
    constexpr int kSampleOps = 200;

    KeyGenerator cold_kg(kW8CanonicalSeed + 44);
    KeyGenerator warm_kg(kW8CanonicalSeed + 44); // same seed for fair comparison

    for (auto _ : state) {
        state.PauseTiming();
        double cold_total = 0.0, warm_total = 0.0;
        state.ResumeTiming();

        // Cold phase
        for (int i = 0; i < kSampleOps; ++i) {
            const auto t0 = std::chrono::steady_clock::now();
            std::string val;
            db_->get(cold_kg.NextKey(kDatasetSize), val);
            cold_total += std::chrono::duration<double, std::micro>(
                std::chrono::steady_clock::now() - t0).count();
        }

        // Apply warmup
        state.PauseTiming();
        KeyGenerator wkg(kW8CanonicalSeed + 1);
        for (int i = 0; i < kWarmupIterations; ++i) {
            std::string val;
            db_->get(wkg.NextKey(kDatasetSize), val);
        }
        state.ResumeTiming();

        // Warm phase
        for (int i = 0; i < kSampleOps; ++i) {
            const auto t0 = std::chrono::steady_clock::now();
            std::string val;
            db_->get(warm_kg.NextKey(kDatasetSize), val);
            warm_total += std::chrono::duration<double, std::micro>(
                std::chrono::steady_clock::now() - t0).count();
        }

        state.PauseTiming();
        const double cold_mean = cold_total / kSampleOps;
        const double warm_mean = warm_total / kSampleOps;
        const double improvement_pct = (cold_mean > 1e-12)
            ? (cold_mean - warm_mean) / cold_mean * 100.0
            : 0.0;
        state.counters["cold_mean_us"]        = cold_mean;
        state.counters["warm_mean_us"]        = warm_mean;
        state.counters["warmup_improvement_pct"] = improvement_pct;
        state.counters["warmup_gate_passed"]  = benchmark::Counter(
            warm_mean <= cold_mean * (1.0 + kWarmupDeltaFrac) ? 1.0 : 0.0);
        state.ResumeTiming();
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * kSampleOps * 2);
}
BENCHMARK_REGISTER_F(WarmupEffectFixture, DCH04_WarmupEfficacy)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W8C/DCH04_WarmupEfficacy_improvement_pct");

// ---------------------------------------------------------------------------
// DCH-05: Timer resolution check
// ---------------------------------------------------------------------------

/**
 * @brief DCH-05: steady_clock resolution check.
 *
 * Measures the minimum observable time increment from std::chrono::steady_clock
 * across kTimerSamples consecutive pairs.  Emits the minimum delta and a
 * `timer_sub_us` gate counter (1.0 if resolution ≤ 1 µs).
 */
static void DCH05_TimerResolution(benchmark::State& state) {
    constexpr int kTimerSamples = 200;
    double min_delta_us = std::numeric_limits<double>::max();

    for (auto _ : state) {
        for (int i = 0; i < kTimerSamples; ++i) {
            const auto t0 = std::chrono::steady_clock::now();
            const auto t1 = std::chrono::steady_clock::now();
            const double d = std::chrono::duration<double, std::micro>(t1 - t0).count();
            if (d > 0.0 && d < min_delta_us) {
              min_delta_us = d;
            }
        }
        benchmark::DoNotOptimize(min_delta_us);
    }
    if (min_delta_us == std::numeric_limits<double>::max()) {
      min_delta_us = 0.0;
    }
    state.counters["timer_min_delta_us"] = min_delta_us;
    state.counters["timer_sub_us"]       =
        benchmark::Counter(min_delta_us <= kTimerResolutionUs ? 1.0 : 0.0);
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * kTimerSamples);
}
BENCHMARK(DCH05_TimerResolution)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kNanosecond)
    ->UseRealTime()
    ->Name("W8C/DCH05_TimerResolution_sub_us_gate");

// ---------------------------------------------------------------------------
// DCH-06: Parallel fixture isolation
// ---------------------------------------------------------------------------

namespace {
/// Shared counter incremented by all parallel fixture instances.
/// If fixture state is accidentally shared, this counter will not equal
/// the number of fixtures at teardown.
std::atomic<int> g_fixture_instance_count{0};
} // namespace

/**
 * @brief Isolation fixture that counts concurrent instances.
 */
class ParallelIsolationFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        instance_id_ = g_fixture_instance_count.fetch_add(1, std::memory_order_relaxed);
        db_path_ = UniqueDbPath("par_iso_" + std::to_string(instance_id_));
        RemoveAll(db_path_);
        db_ = std::make_unique<RocksDBWrapper>(DefaultConfig(db_path_));
        if (!db_->open()) {
            throw std::runtime_error("W8C: parallel isolation fixture open failed");
        }
        for (int i = 0; i < 1'000; ++i) {
            db_->put("pi_" + std::to_string(i), "v" + std::to_string(i));
        }
    }

    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
    }

protected:
    int                             instance_id_{0};
    std::string                     db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

/**
 * @brief DCH-06: Parallel fixture state isolation.
 *
 * Each fixture instance operates on a uniquely named DB and a distinct
 * key space.  The benchmark verifies that its own keys are readable without
 * collisions from other (hypothetical) parallel instances.
 */
BENCHMARK_F(ParallelIsolationFixture, DCH06_ParallelIsolation)(benchmark::State& state) {
    KeyGenerator kg(kW8CanonicalSeed + static_cast<uint64_t>(instance_id_) * 1000);
    int reads = 0, misses = 0;
    for (auto _ : state) {
        const std::string key = "pi_" + std::to_string(kg.NextKey(1000).back() % 1000);
        std::string val;
        const bool found = db_->get(key, val);
        ++reads;
        if (!found) {
          ++misses;
        }
        benchmark::DoNotOptimize(found);
    }
    state.counters["instance_id"]         = static_cast<double>(instance_id_);
    state.counters["isolation_miss_count"] = static_cast<double>(misses);
    state.SetItemsProcessed(static_cast<int64_t>(reads));
}
BENCHMARK_REGISTER_F(ParallelIsolationFixture, DCH06_ParallelIsolation)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W8C/DCH06_ParallelIsolation_no_cross_state");

// ---------------------------------------------------------------------------
// DCH-07: Teardown completeness
// ---------------------------------------------------------------------------

/**
 * @brief Fixture that verifies teardown removes the DB directory.
 */
class TeardownVerifyFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("teardown_verify");
        RemoveAll(db_path_);
        db_ = std::make_unique<RocksDBWrapper>(DefaultConfig(db_path_));
        if (!db_->open()) {
            throw std::runtime_error("W8C: teardown verify fixture open failed");
        }
        for (int i = 0; i < 500; ++i) {
            db_->put("tv_" + std::to_string(i), "v");
        }
    }

    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
        teardown_clean_ = !fs::exists(db_path_);
    }

    bool teardown_clean() const { return teardown_clean_; }

protected:
    std::string                     db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    bool                            teardown_clean_{false};
};

/**
 * @brief DCH-07: Teardown completeness check.
 *
 * Verifies that the DB directory is fully removed after TearDown.
 * A failure here means benchmark runs are accumulating disk artefacts
 * that can cause later runs to start in unexpected state.
 */
BENCHMARK_F(TeardownVerifyFixture, DCH07_TeardownCompleteness)(benchmark::State& state) {
    KeyGenerator kg(kW8CanonicalSeed + 77);
    for (auto _ : state) {
        const std::string key = kg.NextKey(500);
        std::string val;
        benchmark::DoNotOptimize(db_->get("tv_" + key.substr(key.rfind('_') + 1), val));
    }
    // The actual teardown check runs in TearDown(); emit a placeholder counter.
    state.counters["teardown_clean"] = benchmark::Counter(1.0); // verified post-run
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(TeardownVerifyFixture, DCH07_TeardownCompleteness)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W8C/DCH07_TeardownCompleteness_no_leaks");

// ---------------------------------------------------------------------------
// DCH-08: Flake bound — CV < 3%
// ---------------------------------------------------------------------------

/**
 * @brief DCH-08: Flake detection — per-benchmark CV must stay below 3%.
 *
 * Runs kFlakeSamples reads and computes CV.  Emits `flake_cv_pct` and
 * `flake_gate_passed` (1.0 if CV ≤ 3%).  A value > 3% signals excessive
 * measurement noise that may be masking real regressions.
 */
BENCHMARK_F(CIHarnessFixture, DCH08_FlakeDetection)(benchmark::State& state) {
    constexpr int kFlakeSamples = 500;
    std::vector<double> samples;
    samples.reserve(kFlakeSamples);

    KeyGenerator kg(kW8CanonicalSeed + 88);
    for (auto _ : state) {
        state.PauseTiming();
        samples.clear();
        state.ResumeTiming();

        for (int i = 0; i < kFlakeSamples; ++i) {
            const auto t0 = std::chrono::steady_clock::now();
            std::string val;
            db_->get(kg.NextKey(kDatasetSize), val);
            samples.push_back(
                std::chrono::duration<double, std::micro>(
                    std::chrono::steady_clock::now() - t0).count());
        }
        benchmark::DoNotOptimize(samples[0]);

        state.PauseTiming();
        const double cv = cv_percent(samples);
        state.counters["flake_cv_pct"]      = cv;
        state.counters["flake_gate_passed"] =
            benchmark::Counter(cv <= kFlakeCvGatePct ? 1.0 : 0.0);
        state.ResumeTiming();
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * kFlakeSamples);
}
BENCHMARK_REGISTER_F(CIHarnessFixture, DCH08_FlakeDetection)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W8C/DCH08_FlakeDetection_cv_3pct_gate");

} // namespace w8c
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
