// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w5d_governance.cpp
 * @brief Wave 5 / PR B5-D — Governance, Diagnostics & Maintainability
 *
 * Release-gate benchmarks that enforce hard thresholds on the four most
 * production-critical paths.  Each benchmark emits structured counters
 * consumed by report_variance_w5.py and release_gate_manifest_w5.json:
 *
 *   Gate W5D-1: CRUD read p50 ≤ 20 µs  /  p99 ≤ 200 µs
 *   Gate W5D-2: CRUD write throughput ≥ 80 000 ops/s
 *   Gate W5D-3: Secondary-index range scan ≤ 500 µs / scan
 *   Gate W5D-4: Batch commit 100-rec ≤ 5 ms / batch
 *
 * Diagnostic counters (available in JSON output):
 *   gate_pass   — 1.0 if the gate criterion was met, 0.0 otherwise
 *   p50_us      — observed p50 latency in microseconds
 *   p99_us      — observed p99 latency in microseconds
 *   throughput  — observed ops/s
 *
 * Repro steps (see RUNBOOK_W5.md §3 for full instructions):
 *   cmake --preset windows-release
 *   cmake --build --preset windows-release --target bench_w5d_governance
 *   .\bench_w5d_governance --benchmark_out=bench_w5d.json \
 *       --benchmark_out_format=json --benchmark_filter=BM_W5D
 *   python benchmarks/wave5/report_variance_w5.py \
 *       --input bench_w5d.json \
 *       --baseline benchmarks/baselines/wave5/bench_w5d_baseline.json
 *
 * Design principles (Wave 5 hygiene):
 *   - kW5CanonicalSeed = 42
 *   - All I/O paths use OS temp dir + steady_clock suffix
 *   - 3-phase warmup (cold/warm/hot) applied in every fixture SetUp
 *   - UseRealTime() for all I/O-bound benchmarks
 *
 * Baseline: benchmarks/baselines/wave5/bench_w5d_baseline.json
 */

#include <benchmark/benchmark.h>

#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace themis;

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr uint64_t kW5DSeed       = 42;
static constexpr int      kW5DCorpus     = 3'000;
static constexpr int      kW5DWarmupCold = 50;
static constexpr int      kW5DWarmupWarm = 100;
static constexpr int      kW5DWarmupHot  = 200;

// Release gate thresholds (µs / ops/s)
static constexpr double kGateReadP50MaxUs  =  20.0;
static constexpr double kGateReadP99MaxUs  = 200.0;
static constexpr double kGateWriteMinOpsS  = 80'000.0;
static constexpr double kGateScanMaxUs     = 500.0;
static constexpr double kGateBatchMaxMs    =   5.0;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace w5d {

static fs::path tempPath(std::string_view prefix) {
    auto ts = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return fs::temp_directory_path() / (std::string(prefix) + "_" + ts);
}

class Rng {
public:
    explicit Rng(uint64_t seed = kW5DSeed)
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
private:
    std::mt19937_64 eng_ = {};
};

struct Percentiles {
    double p50, p95, p99;
};

static Percentiles computePercentiles(std::vector<double>& v) {
    if (v.empty()) return {0, 0, 0};
    std::sort(v.begin(), v.end());
    const auto at = [&](double p) -> double {
        const std::size_t idx =
            std::min(static_cast<std::size_t>(p / 100.0 * v.size()), v.size() - 1);
        return v[idx];
    };
    return {at(50), at(95), at(99)};
}

} // namespace w5d

// ===========================================================================
// Shared warmed-DB fixture
// ===========================================================================

/**
 * @brief Shared base fixture for all W5D gate benchmarks.
 *
 * Applies the canonical Wave-5 three-phase warmup protocol in SetUp so
 * all gate benchmarks start from an identical thermal state:
 *   Phase 1 (cold): kW5DWarmupCold writes
 *   Phase 2 (warm): corpus load + kW5DWarmupWarm sequential reads
 *   Phase 3 (hot):  kW5DWarmupHot  random reads
 */
class W5dGateFixture : public benchmark::Fixture {
public:
    void SetUp(::benchmark::State& /*state*/) override {
        dbPath_ = w5d::tempPath("w5d_gate");
        fs::create_directories(dbPath_);

        RocksDBWrapper::Config cfg;
        cfg.db_path             = dbPath_.string();
        cfg.block_cache_size_mb = 128;
        cfg.compression_default = "lz4";
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open())
            throw std::runtime_error("W5dGateFixture: DB open failed");

        idx_ = std::make_unique<SecondaryIndexManager>(*db_);
        idx_->createIndex("gate", "cat", false);
        idx_->createRangeIndex("gate", "seq");

        w5d::Rng rng(kW5DSeed);

        // Phase 1: cold writes
        for (int i = 0; i < kW5DWarmupCold; ++i) {
            BaseEntity e("cold_" + std::to_string(i));
            e.setField("cat", rng.key(4));
            e.setField("seq", static_cast<int64_t>(i));
            idx_->put("gate", e);
        }

        // Phase 2: corpus + sequential warm reads
        keys_.reserve(kW5DCorpus);
        for (int i = 0; i < kW5DCorpus; ++i) {
            const std::string k = "gate_" + std::to_string(i);
            keys_.push_back(k);
            BaseEntity e(k);
            e.setField("cat", rng.key(4));
            e.setField("seq", static_cast<int64_t>(1'000'000 + i));
            idx_->put("gate", e);
        }
        for (int i = 0; i < kW5DWarmupWarm; ++i) {
            auto blob = db_->get(KeySchema::makeRelationalKey("gate", keys_[i % keys_.size()]));
            (void)blob;
        }

        // Phase 3: random hot reads
        w5d::Rng hotRng(kW5DSeed + 77);
        for (int i = 0; i < kW5DWarmupHot; ++i) {
            int idx_i = static_cast<int>(hotRng.integer(0, keys_.size() - 1));
            auto blob = db_->get(KeySchema::makeRelationalKey("gate", keys_[idx_i]));
            (void)blob;
        }

        writeSeq_ = kW5DWarmupCold + kW5DCorpus;
    }

    void TearDown(::benchmark::State& /*state*/) override {
        idx_.reset();
        db_->close();
        db_.reset();
        std::error_code ec = {};
        fs::remove_all(dbPath_, ec);
    }

protected:
    fs::path                               dbPath_;
    std::unique_ptr<RocksDBWrapper>        db_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::vector<std::string>               keys_;
    int                                    writeSeq_{0};
    w5d::Rng                               rng_{kW5DSeed + 1};
};

// ===========================================================================
// Gate W5D-1: CRUD read latency (p50 / p99)
// ===========================================================================

/**
 * @brief BM_W5D_Gate1_ReadLatency
 *
 * Enforces release gate W5D-1:
 *   p50 ≤ 20 µs  AND  p99 ≤ 200 µs
 *
 * Iterates round-robin across all corpus keys to avoid cache-hit bias.
 * Emits gate_pass=1.0 when both percentile thresholds are satisfied;
 * gate_pass=0.0 triggers a CI regression signal.
 */
BENCHMARK_DEFINE_F(W5dGateFixture, Gate1_ReadLatency)(benchmark::State& state) {
    std::vector<double> latencies;
    latencies.reserve(state.max_iterations);
    std::size_t ki = 0;

    for (auto _ : state) {
        const auto& k = keys_[ki % keys_.size()];
        const auto t0 = std::chrono::steady_clock::now();
        auto ent = db_->get(KeySchema::makeRelationalKey("gate", k));
        const auto t1 = std::chrono::steady_clock::now();
        benchmark::DoNotOptimize(ent);

        latencies.push_back(
            std::chrono::duration<double, std::micro>(t1 - t0).count());
        ++ki;
    }

    const auto pct = w5d::computePercentiles(latencies);
    const bool pass = (pct.p50 <= kGateReadP50MaxUs) &&
                      (pct.p99 <= kGateReadP99MaxUs);

    state.SetItemsProcessed(state.iterations());
    state.counters["p50_us"]   = pct.p50;
    state.counters["p99_us"]   = pct.p99;
    state.counters["gate_pass"] = pass ? 1.0 : 0.0;
    state.SetLabel(std::string(pass ? "PASS" : "FAIL")
                   + " p50=" + std::to_string(static_cast<int>(pct.p50))
                   + " p99=" + std::to_string(static_cast<int>(pct.p99)));
}
BENCHMARK_REGISTER_F(W5dGateFixture, Gate1_ReadLatency)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Iterations(5'000);

// ===========================================================================
// Gate W5D-2: CRUD write throughput
// ===========================================================================

/**
 * @brief BM_W5D_Gate2_WriteThroughput
 *
 * Enforces release gate W5D-2:
 *   write throughput ≥ 80 000 ops/s
 *
 * Uses unique keys in every iteration to prevent merge-path shortcuts.
 * Emits gate_pass, throughput counters.
 */
BENCHMARK_DEFINE_F(W5dGateFixture, Gate2_WriteThroughput)(benchmark::State& state) {
    for (auto _ : state) {
        BaseEntity e("gw_" + std::to_string(writeSeq_));
        e.setField("cat", rng_.key(4));
        e.setField("seq", static_cast<int64_t>(writeSeq_));
        idx_->put("gate", e);
        ++writeSeq_;
    }

    state.SetItemsProcessed(state.iterations());

    const double elapsed_s = state.elapsed_real_time() > 0
        ? state.elapsed_real_time()
        : 1.0;
    const double throughput = static_cast<double>(state.iterations()) / elapsed_s;
    const bool pass = throughput >= kGateWriteMinOpsS;

    state.counters["throughput"] = throughput;
    state.counters["gate_pass"]  = pass ? 1.0 : 0.0;
    state.SetLabel(std::string(pass ? "PASS" : "FAIL")
                   + " ops/s=" + std::to_string(static_cast<int>(throughput)));
}
BENCHMARK_REGISTER_F(W5dGateFixture, Gate2_WriteThroughput)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Iterations(30'000);

// ===========================================================================
// Gate W5D-3: Secondary-index range scan
// ===========================================================================

/**
 * @brief BM_W5D_Gate3_RangeScanLatency
 *
 * Enforces release gate W5D-3:
 *   range scan latency ≤ 500 µs / scan (100-record window)
 *
 * Performs a range predicate scan (sequential key space).  The 100-record
 * scan window is representative of paginated list API calls.
 */
BENCHMARK_DEFINE_F(W5dGateFixture, Gate3_RangeScanLatency)(benchmark::State& state) {
    const int scanWindow = 100;
    std::vector<double> latencies;
    latencies.reserve(state.max_iterations);
    int lo = 0;

    for (auto _ : state) {
        const int64_t rangeLo = static_cast<int64_t>(1'000'000 + lo);
        const int64_t rangeHi = rangeLo + scanWindow - 1;
        const auto lower = std::optional<std::string>(std::to_string(rangeLo));
        const auto upper = std::optional<std::string>(std::to_string(rangeHi));

        const auto t0 = std::chrono::steady_clock::now();
        auto [status, results] = idx_->scanKeysRange("gate", "seq", lower, upper, true, true, scanWindow, false);
        const auto t1 = std::chrono::steady_clock::now();
        benchmark::DoNotOptimize(status.ok);
        benchmark::DoNotOptimize(results);

        latencies.push_back(
            std::chrono::duration<double, std::micro>(t1 - t0).count());

        lo = (lo + scanWindow) % (kW5DCorpus - scanWindow);
    }

    const auto pct = w5d::computePercentiles(latencies);
    const bool pass = pct.p99 <= kGateScanMaxUs;

    state.SetItemsProcessed(state.iterations());
    state.counters["p50_us"]   = pct.p50;
    state.counters["p99_us"]   = pct.p99;
    state.counters["gate_pass"] = pass ? 1.0 : 0.0;
    state.SetLabel(std::string(pass ? "PASS" : "FAIL")
                   + " p99=" + std::to_string(static_cast<int>(pct.p99))
                   + "µs scan_window=100");
}
BENCHMARK_REGISTER_F(W5dGateFixture, Gate3_RangeScanLatency)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Iterations(2'000);

// ===========================================================================
// Gate W5D-4: Batch commit latency
// ===========================================================================

/**
 * @brief BM_W5D_Gate4_BatchCommitLatency
 *
 * Enforces release gate W5D-4:
 *   100-record batch commit ≤ 5 ms / batch
 *
 * Each iteration generates and commits a batch of 100 unique records.
 * The batch preparation is excluded from timing (PauseTiming / ResumeTiming).
 */
BENCHMARK_DEFINE_F(W5dGateFixture, Gate4_BatchCommitLatency)(benchmark::State& state) {
    const int kBatch = 100;
    std::vector<double> latencies;
    latencies.reserve(state.max_iterations);

    for (auto _ : state) {
        state.PauseTiming();
        std::vector<BaseEntity> batch;
        batch.reserve(kBatch);
        for (int i = 0; i < kBatch; ++i) {
            BaseEntity e("bc_" + std::to_string(writeSeq_));
            e.setField("cat", rng_.key(4));
            e.setField("seq", static_cast<int64_t>(writeSeq_));
            batch.push_back(std::move(e));
            ++writeSeq_;
        }
        state.ResumeTiming();

        const auto t0 = std::chrono::steady_clock::now();
        for (auto& entity : batch)
            idx_->put("gate", entity);
        const auto t1 = std::chrono::steady_clock::now();

        latencies.push_back(
            std::chrono::duration<double, std::milli>(t1 - t0).count());
    }

    const auto pct = w5d::computePercentiles(latencies);
    const bool pass = pct.p99 <= kGateBatchMaxMs;

    state.SetItemsProcessed(state.iterations() * kBatch);
    state.counters["p50_ms"]   = pct.p50;
    state.counters["p99_ms"]   = pct.p99;
    state.counters["gate_pass"] = pass ? 1.0 : 0.0;
    state.SetLabel(std::string(pass ? "PASS" : "FAIL")
                   + " p99=" + std::to_string(static_cast<int>(pct.p99 * 1000))
                   + "µs batch=100");
}
BENCHMARK_REGISTER_F(W5dGateFixture, Gate4_BatchCommitLatency)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Iterations(1'000);
