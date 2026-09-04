// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w5c_determinism.cpp
 * @brief Wave 5 / PR B5-C — Determinism, Variance Control & Reproducibility
 *
 * Validates that benchmark results are stable across repeated runs by
 * quantifying measurement variance and enforcing a coefficient-of-variation
 * (CV) threshold:
 *
 *   1. Repeated-run variance test — executes the same micro-op N times in
 *      separate benchmark instances and records min/mean/max/CV.
 *   2. Warmup convergence — shows how many warmup iterations are required
 *      before throughput stabilises (identifies the warmup minimum).
 *   3. RNG determinism check — proves that seeded runs produce identical
 *      data sequences (critical for baseline comparability).
 *   4. Canonical warmup protocol validator — the standard 3-phase warmup
 *      (cold, warm, hot) used by all Wave 5 benchmarks.
 *
 * Acceptance criterion:
 *   CV ≤ 5% for all critical-path benchmarks across 5 repeated runs.
 *
 * Design principles (Wave 5 hygiene):
 *   - kW5CanonicalSeed = 42 for all RNG initialisation
 *   - All I/O paths use OS temp dir + steady_clock suffix
 *   - UseRealTime() for I/O-bound benchmarks
 *   - Warmup: explicit 3-phase (cold / warm / hot) before SetIterations
 *
 * Baseline: benchmarks/baselines/wave5/bench_w5c_baseline.json
 */

#include <benchmark/benchmark.h>

#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <memory>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace themis;

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr uint64_t kW5CSeed       = 42;
static constexpr int      kW5CCorpus     = 2'000;
static constexpr int      kW5CWarmupCold = 50;
static constexpr int      kW5CWarmupWarm = 100;
static constexpr int      kW5CWarmupHot  = 200;
static constexpr double   kW5CMaxCV      = 0.05; ///< 5% coefficient-of-variation

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace w5c {

static fs::path tempPath(std::string_view prefix) {
    auto ts = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return fs::temp_directory_path() / (std::string(prefix) + "_" + ts);
}

class Rng {
public:
    explicit Rng(uint64_t seed = kW5CSeed)
        : eng_(static_cast<std::mt19937_64::result_type>(seed)) {}

    std::string key(int len = 12) {
        static constexpr std::string_view kC =
            "abcdefghijklmnopqrstuvwxyz0123456789";
        std::uniform_int_distribution<std::size_t> d(0, kC.size() - 1);
        std::string s(len, ' ');
        for (auto& c : s) {
          c = kC[d(eng_)];
        }
        return s;
    }
    int64_t integer(int64_t lo, int64_t hi) {
        std::uniform_int_distribution<int64_t> d(lo, hi);
        return d(eng_);
    }
    /** @brief Deterministic sequence: first N keys generated from this seed. */
    std::vector<std::string> sequence(int n, int len = 12) {
        std::vector<std::string> v;
        v.reserve(n);
        for (int i = 0; i < n; ++i) {
          v.push_back(key(len));
        }
        return v;
    }

private:
    std::mt19937_64 eng_;
};

/**
 * @brief Computes mean, stddev, and coefficient of variation for a sample.
 * @param samples Non-empty vector of doubles (latency or throughput values).
 * @returns {mean, stddev, cv}
 */
struct Stats { double mean, stddev, cv; };

static Stats computeStats(const std::vector<double>& samples) {
    if (samples.empty()) return {0, 0, 0};
    const double n = static_cast<double>(samples.size());
    const double mean = std::accumulate(samples.begin(), samples.end(), 0.0) / n;
    double var = 0.0;
    for (double x : samples) {
      var += (x - mean) * (x - mean);
    }
    var /= n;
    const double stddev = std::sqrt(var);
    const double cv = mean > 0.0 ? stddev / mean : 0.0;
    return {mean, stddev, cv};
}

static std::string fmtStats(const Stats& s) {
    std::ostringstream oss;
    oss << "mean=" << static_cast<int>(s.mean)
        << " sd="  << static_cast<int>(s.stddev)
        << " cv="  << static_cast<int>(s.cv * 100) << "%";
    return oss.str();
}

} // namespace w5c

// ===========================================================================
// 1. Repeated-run variance measurement (point-lookup)
// ===========================================================================

/**
 * @brief Fixture for variance benchmarks.
 *
 * Opens a pre-warmed database using the canonical 3-phase warmup protocol:
 *   Phase 1 (cold): kW5CWarmupCold writes — fills write buffer, cold I/O
 *   Phase 2 (warm): kW5CWarmupWarm reads — warms OS page cache
 *   Phase 3 (hot):  kW5CWarmupHot reads  — stabilises CPU caches
 *
 * After warmup the fixture enters the steady-state measurement window.
 */
class W5cVarianceFixture : public benchmark::Fixture {
public:
    void SetUp(::benchmark::State& /*state*/) override {
        dbPath_ = w5c::tempPath("w5c_var");
        fs::create_directories(dbPath_);

        RocksDBWrapper::Config cfg;
        cfg.db_path             = dbPath_.string();
        cfg.block_cache_size_mb = 64;
        cfg.compression_default = "lz4";
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open())
            throw std::runtime_error("W5cVarianceFixture: open failed");

        idx_ = std::make_unique<SecondaryIndexManager>(*db_);
        idx_->createIndex("d", "tag", false);

        w5c::Rng rng(kW5CSeed);

        // Phase 1: cold writes
        for (int i = 0; i < kW5CWarmupCold; ++i) {
            BaseEntity e("cold_" + std::to_string(i));
            e.setField("tag", rng.key(6));
            idx_->put("d", e);
        }

        // Phase 2: corpus + warm reads
        keys_.reserve(kW5CCorpus);
        for (int i = 0; i < kW5CCorpus; ++i) {
            const std::string k = "corpus_" + std::to_string(i);
            keys_.push_back(k);
            BaseEntity e(k);
            e.setField("tag", rng.key(6));
            idx_->put("d", e);
        }
        for (int i = 0; i < kW5CWarmupWarm; ++i) {
            auto blob = db_->get(KeySchema::makeRelationalKey("d", keys_[i % keys_.size()]));
            (void)blob;
        }

        // Phase 3: hot reads
        for (int i = 0; i < kW5CWarmupHot; ++i) {
            auto blob = db_->get(KeySchema::makeRelationalKey("d", keys_[i % keys_.size()]));
            (void)blob;
        }
    }

    void TearDown(::benchmark::State& /*state*/) override {
        idx_.reset();
        db_->close();
        db_.reset();
        std::error_code ec;
        fs::remove_all(dbPath_, ec);
    }

protected:
    fs::path                               dbPath_;
    std::unique_ptr<RocksDBWrapper>        db_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::vector<std::string>               keys_;
};

/**
 * @brief BM_W5C_ReadVariance
 *
 * Performs repeated point-lookups over the warmed corpus and tracks per-sample
 * latency.  At the end of the run reports CV in the benchmark label.
 * Acceptance: CV ≤ 5% (enforced by release_gate_manifest_w5.json).
 */
BENCHMARK_DEFINE_F(W5cVarianceFixture, ReadVariance)(benchmark::State& state) {
    std::size_t ki = 0;
    std::vector<double> latencies;
    latencies.reserve(state.max_iterations);

    for (auto _ : state) {
        const auto& k = keys_[ki % keys_.size()];
        const auto t0 = std::chrono::steady_clock::now();
        auto ent = db_->get(KeySchema::makeRelationalKey("d", k));
        const auto t1 = std::chrono::steady_clock::now();
        benchmark::DoNotOptimize(ent);

        latencies.push_back(
            std::chrono::duration<double, std::micro>(t1 - t0).count());
        ++ki;
    }

    const auto stats = w5c::computeStats(latencies);
    state.SetItemsProcessed(state.iterations());
    state.SetLabel(w5c::fmtStats(stats));

    // Surface CV violation as a counter so CI tooling can detect it.
    state.counters["CV_pct"] = stats.cv * 100.0;
    state.counters["CV_ok"]  = (stats.cv <= kW5CMaxCV) ? 1.0 : 0.0;
}
BENCHMARK_REGISTER_F(W5cVarianceFixture, ReadVariance)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Iterations(5'000);

/**
 * @brief BM_W5C_WriteVariance
 *
 * Tracks per-iteration write latency CV to detect compaction-induced stalls
 * that inflate the tail and violate the CV gate.
 */
BENCHMARK_DEFINE_F(W5cVarianceFixture, WriteVariance)(benchmark::State& state) {
    int seq = static_cast<int>(keys_.size());
    std::vector<double> latencies;
    latencies.reserve(state.max_iterations);
    w5c::Rng rng(kW5CSeed + 3);

    for (auto _ : state) {
        BaseEntity e("var_w_" + std::to_string(seq));
        e.setField("tag", rng.key(6));
        const auto t0 = std::chrono::steady_clock::now();
        idx_->put("d", e);
        const auto t1 = std::chrono::steady_clock::now();

        latencies.push_back(
            std::chrono::duration<double, std::micro>(t1 - t0).count());
        ++seq;
    }

    const auto stats = w5c::computeStats(latencies);
    state.SetItemsProcessed(state.iterations());
    state.SetLabel(w5c::fmtStats(stats));

    state.counters["CV_pct"] = stats.cv * 100.0;
    state.counters["CV_ok"]  = (stats.cv <= kW5CMaxCV) ? 1.0 : 0.0;
}
BENCHMARK_REGISTER_F(W5cVarianceFixture, WriteVariance)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Iterations(3'000);

// ===========================================================================
// 2. Warmup convergence probe
// ===========================================================================

/**
 * @brief BM_W5C_WarmupConvergence
 *
 * Measures read throughput at three warmup levels (cold / warm / hot) to
 * determine the minimum warmup required for stable measurements.
 * Arg(0) = warmup phase: 0 = cold, 1 = warm, 2 = hot.
 *
 * Expected output: hot > warm > cold; hot/warm ratio ≤ 1.1 indicates
 * warmup is complete and further iterations are not needed.
 */
static void BM_W5C_WarmupConvergence(benchmark::State& state) {
    const int phase = static_cast<int>(state.range(0));

    auto dbPath = w5c::tempPath("w5c_conv");
    fs::create_directories(dbPath);

    RocksDBWrapper::Config cfg;
    cfg.db_path             = dbPath.string();
    cfg.block_cache_size_mb = 64;
    cfg.compression_default = "lz4";
    auto db = std::make_unique<RocksDBWrapper>(cfg);
    if (!db->open())
        throw std::runtime_error("BM_W5C_WarmupConvergence: open failed");

    SecondaryIndexManager idx(*db);
    idx.createIndex("wc", "t", false);

    w5c::Rng rng(kW5CSeed + 7);
    std::vector<std::string> keys = {};

    for (int i = 0; i < kW5CCorpus; ++i) {
        const std::string k = "wc_" + std::to_string(i);
        keys.push_back(k);
        BaseEntity e(k);
        e.setField("t", rng.key(4));
        idx.put("wc", e);
    }

    // Apply warmup according to phase
    const int warmupCount = (phase == 0) ? 0
                          : (phase == 1) ? kW5CWarmupCold + kW5CWarmupWarm
                                         : kW5CWarmupCold + kW5CWarmupWarm + kW5CWarmupHot;
    for (int i = 0; i < warmupCount; ++i) {
        auto blob = db->get(KeySchema::makeRelationalKey("wc", keys[i % keys.size()]));
        (void)blob;
    }

    std::size_t ki = 0;
    for (auto _ : state) {
        auto blob = db->get(KeySchema::makeRelationalKey("wc", keys[ki % keys.size()]));
        benchmark::DoNotOptimize(blob);
        ++ki;
    }

    state.SetItemsProcessed(state.iterations());
    const char* phaseName[] = {"cold", "warm", "hot"};
    state.SetLabel(phaseName[phase < 3 ? phase : 2]);

    db->close();
    std::error_code ec;
    fs::remove_all(dbPath, ec);
}
BENCHMARK(BM_W5C_WarmupConvergence)
    ->Arg(0)   // cold — no warmup
    ->Arg(1)   // warm — partial warmup
    ->Arg(2)   // hot  — full 3-phase warmup
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Iterations(3'000);

// ===========================================================================
// 3. RNG determinism check
// ===========================================================================

/**
 * @brief BM_W5C_RngDeterminism
 *
 * Verifies that the canonical RNG seed produces an identical key sequence
 * across independent instantiations.  Two Rng objects seeded with
 * kW5CSeed must yield the same first-N keys; a mismatch would indicate
 * a non-deterministic dependency and must be treated as a CI failure.
 *
 * The benchmark body is intentionally minimal — this is a correctness probe
 * that happens to be expressed as a benchmark.
 */
static void BM_W5C_RngDeterminism(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    bool allMatch = true;

    for (auto _ : state) {
        state.PauseTiming();
        w5c::Rng rng1(kW5CSeed);
        w5c::Rng rng2(kW5CSeed);
        state.ResumeTiming();

        for (int i = 0; i < n; ++i) {
            const auto k1 = rng1.key();
            const auto k2 = rng2.key();
            if (k1 != k2) {
                allMatch = false;
            }
            benchmark::DoNotOptimize(k1);
            benchmark::DoNotOptimize(k2);
        }
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.counters["deterministic"] = allMatch ? 1.0 : 0.0;
    state.SetLabel(allMatch ? "PASS: RNG is deterministic"
                            : "FAIL: RNG divergence detected");
}
BENCHMARK(BM_W5C_RngDeterminism)
    ->Arg(100)
    ->Arg(1'000)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(10);

// ===========================================================================
// 4. Canonical warmup protocol validator
// ===========================================================================

/**
 * @brief BM_W5C_CanonicalWarmup_Throughput
 *
 * Reference benchmark that applies the standard Wave 5 three-phase warmup
 * protocol and then measures steady-state read throughput.  Used as the
 * reference point for cross-PR comparisons.
 *
 * Protocol:
 *   - Phase 1 (cold, 50 ops):  initial writes; exercises write path + OS I/O
 *   - Phase 2 (warm, 100 ops): sequential reads; warms page cache
 *   - Phase 3 (hot,  200 ops): random reads; stabilises CPU branch predictor
 *   - Measurement window: 5000 iterations
 *
 * Output format: JSON via --benchmark_out (consumed by report_variance_w5.py).
 */
static void BM_W5C_CanonicalWarmup_Throughput(benchmark::State& state) {
    auto dbPath = w5c::tempPath("w5c_canonical");
    fs::create_directories(dbPath);

    RocksDBWrapper::Config cfg;
    cfg.db_path             = dbPath.string();
    cfg.block_cache_size_mb = 64;
    cfg.compression_default = "lz4";
    auto db = std::make_unique<RocksDBWrapper>(cfg);
    if (!db->open())
        throw std::runtime_error("BM_W5C_CanonicalWarmup: open failed");
    SecondaryIndexManager idx(*db);
    idx.createIndex("cw", "tag", false);

    w5c::Rng rng(kW5CSeed + 13);

    // Phase 1: cold writes
    for (int i = 0; i < kW5CWarmupCold; ++i) {
        BaseEntity e("cw_" + std::to_string(i));
        e.setField("tag", rng.key(6));
        idx.put("cw", e);
    }

    // Phase 2: sequential warm reads
    std::vector<std::string> keys;
    keys.reserve(kW5CCorpus);
    for (int i = 0; i < kW5CCorpus; ++i) {
        const std::string k = "cw_c_" + std::to_string(i);
        keys.push_back(k);
        BaseEntity e(k);
        e.setField("tag", rng.key(6));
        idx.put("cw", e);
    }
    for (int i = 0; i < kW5CWarmupWarm; ++i) {
        auto blob = db->get(KeySchema::makeRelationalKey("cw", keys[i % keys.size()]));
        (void)blob;
    }

    // Phase 3: random hot reads
    w5c::Rng hotRng(kW5CSeed + 99);
    for (int i = 0; i < kW5CWarmupHot; ++i) {
        int idx_i = static_cast<int>(hotRng.integer(0, keys.size() - 1));
        auto blob = db->get(KeySchema::makeRelationalKey("cw", keys[idx_i]));
        (void)blob;
    }

    // Measurement window
    std::size_t ki = 0;
    for (auto _ : state) {
        auto blob = db->get(KeySchema::makeRelationalKey("cw", keys[ki % keys.size()]));
        benchmark::DoNotOptimize(blob);
        ++ki;
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("canonical 3-phase warmup; steady-state read");

    db->close();
    std::error_code ec;
    fs::remove_all(dbPath, ec);
}
BENCHMARK(BM_W5C_CanonicalWarmup_Throughput)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Iterations(5'000);
