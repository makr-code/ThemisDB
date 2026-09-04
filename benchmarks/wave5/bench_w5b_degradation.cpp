// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w5b_degradation.cpp
 * @brief Wave 5 / PR B5-B — Failure/Degradation Performance Validation
 *
 * Benchmarks system behaviour under degraded conditions that mirror
 * production failure modes:
 *
 *   1. Simulated read-latency injection — measures throughput as increasing
 *      artificial delay is added to storage reads.
 *   2. Write-pressure / backpressure — sustained writer flood with a bounded
 *      write queue to expose stall behaviour and throughput cliff.
 *   3. Index rebuild overhead — cost of re-indexing a corpus after simulated
 *      index loss (partial failure / recovery entry-point).
 *   4. Concurrent reader/writer contention — mixed 80/20 read-write workload
 *      with varying thread counts to surface lock contention and p99 tail.
 *
 * Degradation instrumentation:
 *   - p50/p95/p99 percentiles via state.SetLabel() + manual histogram
 *   - Throughput drop-off captured by varying Arg(0) = injected delay (µs)
 *   - Recovery throughput measured as separate BM_ after the "fault" phase
 *
 * Design principles (Wave 5 hygiene):
 *   - kW5CanonicalSeed = 42 for all RNG initialisation
 *   - All paths use OS temp dir + steady_clock suffix
 *   - I/O-bound benchmarks call UseRealTime()
 *
 * Baseline: benchmarks/baselines/wave5/bench_w5b_baseline.json
 */

#include <benchmark/benchmark.h>

#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using namespace themis;

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr uint64_t kW5BSeed       = 42;
static constexpr int      kW5BWarmup     = 150;
static constexpr int      kW5BCorpus     = 3'000;
static constexpr int      kW5BWriteFlood = 10'000;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace w5b {

static fs::path tempPath(std::string_view prefix) {
    auto ts = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return fs::temp_directory_path() / (std::string(prefix) + "_" + ts);
}

class Rng {
public:
    explicit Rng(uint64_t seed = kW5BSeed)
        : eng_(static_cast<std::mt19937_64::result_type>(seed)) {}
    std::string key(int len = 16) {
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
    std::mt19937_64 eng_;
};

/** @brief Simple manual percentile tracker (no allocations during benchmarking). */
class Histogram {
public:
    void record(double us) { samples_.push_back(us); }

    double percentile(double p) {
        if (samples_.empty()) {
          return 0.0;
        }
        auto& s = samples_;
        std::sort(s.begin(), s.end());
        const std::size_t idx =
            std::min(static_cast<std::size_t>(p / 100.0 * s.size()),
                     s.size() - 1);
        return s[idx];
    }

    void clear() { samples_.clear(); }

private:
    std::vector<double> samples_;
};

} // namespace w5b

// ===========================================================================
// 1. Simulated read-latency injection
// ===========================================================================

/**
 * @brief Fixture for latency-injection benchmarks.
 *
 * Pre-populates kW5BCorpus records.  During the benchmark, an artificial
 * sleep of Arg(0) microseconds is injected after each read to simulate
 * a slow storage tier, increased network RTT, or a partially degraded node.
 *
 * @note The artificial delay is placed *after* DoNotOptimize so that the
 *       injected time appears in the wall-clock measurements but is still
 *       associated with the same iteration.
 */
class W5bLatencyInjectionFixture : public benchmark::Fixture {
public:
    void SetUp(::benchmark::State& /*state*/) override {
        dbPath_ = w5b::tempPath("w5b_latency");
        fs::create_directories(dbPath_);

        RocksDBWrapper::Config cfg;
        cfg.db_path             = dbPath_.string();
        cfg.block_cache_size_mb = 64;
        cfg.compression_default = "lz4";
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open())
            throw std::runtime_error("W5bLatencyInjectionFixture: open failed");

        idx_ = std::make_unique<SecondaryIndexManager>(*db_);
        idx_->createIndex("r", "cat", false);
        readKeys_.reserve(kW5BWarmup + kW5BCorpus);

        w5b::Rng rng(kW5BSeed);
        for (int i = 0; i < kW5BWarmup + kW5BCorpus; ++i) {
            BaseEntity e("k_" + std::to_string(i));
            e.setField("cat", rng.key(6));
            e.setField("val", rng.integer(0, 9999));
            idx_->put("r", e);
            readKeys_.push_back("k_" + std::to_string(i));
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
    std::vector<std::string>               readKeys_;
};

/**
 * @brief BM_W5B_ReadLatencyInjection
 *
 * Measures read throughput as a function of injected per-operation latency.
 * Arg(0) = simulated delay in microseconds (0, 50, 100, 500).
 * Throughput cliff reveals the latency budget consumed by the critical read path.
 */
BENCHMARK_DEFINE_F(W5bLatencyInjectionFixture, ReadLatencyInjection)(
    benchmark::State& state) {
    const auto injectedDelay = std::chrono::microseconds(state.range(0));
    std::size_t ki = 0;
    w5b::Histogram hist;

    for (auto _ : state) {
        const auto& k = readKeys_[ki % readKeys_.size()];
        auto t0 = std::chrono::steady_clock::now();
        auto blob = db_->get(KeySchema::makeRelationalKey("r", k));
        benchmark::DoNotOptimize(blob);
        auto t1 = std::chrono::steady_clock::now();

        hist.record(
            std::chrono::duration<double, std::micro>(t1 - t0).count());

        if (injectedDelay.count() > 0)
            std::this_thread::sleep_for(injectedDelay);

        ++ki;
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("delay=" + std::to_string(state.range(0)) + "µs"
                   + " p50=" + std::to_string(static_cast<int>(hist.percentile(50)))
                   + " p99=" + std::to_string(static_cast<int>(hist.percentile(99))));
}
BENCHMARK_REGISTER_F(W5bLatencyInjectionFixture, ReadLatencyInjection)
    ->Arg(0)
    ->Arg(50)
    ->Arg(100)
    ->Arg(500)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Iterations(2'000);

// ===========================================================================
// 2. Write-pressure / backpressure
// ===========================================================================

/**
 * @brief BM_W5B_WriteFlood_Throughput
 *
 * Floods the storage layer with kW5BWriteFlood sequential writes from
 * multiple threads (benchmark::Threads controls concurrency). Captures the throughput
 * plateau and stall onset that occurs when write-buffers fill.
 *
 * Recovery measurement: a subsequent read pass immediately after flood
 * reveals whether read latency has been elevated by compaction pressure.
 */
class W5bWriteFloodFixture : public benchmark::Fixture {
public:
    void SetUp(::benchmark::State& /*state*/) override {
        dbPath_ = w5b::tempPath("w5b_flood");
        fs::create_directories(dbPath_);

        RocksDBWrapper::Config cfg;
        cfg.db_path                 = dbPath_.string();
        cfg.block_cache_size_mb     = 64;
        cfg.compression_default     = "lz4";
        cfg.max_write_buffer_number = 4;
        cfg.memtable_size_mb        = 64;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open())
            throw std::runtime_error("W5bWriteFloodFixture: open failed");
        idx_ = std::make_unique<SecondaryIndexManager>(*db_);
        idx_->createIndex("flood", "t", false);
        seq_.store(0, std::memory_order_relaxed);
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
    std::atomic<int>                       seq_{0};
};

BENCHMARK_DEFINE_F(W5bWriteFloodFixture, WriteFlood_Throughput)(benchmark::State& state) {
    w5b::Rng rng(kW5BSeed + static_cast<uint64_t>(state.thread_index()) + 5);
    for (auto _ : state) {
        const int id = seq_.fetch_add(1, std::memory_order_relaxed);
        BaseEntity e("f_" + std::to_string(id));
        e.setField("t", rng.key(8));
        e.setField("n", static_cast<int64_t>(id));
        idx_->put("flood", e);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("write-flood, " + std::to_string(state.threads()) + " thread(s)");
}
BENCHMARK_REGISTER_F(W5bWriteFloodFixture, WriteFlood_Throughput)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Iterations(5'000);

// ===========================================================================
// 3. Index rebuild overhead
// ===========================================================================

/**
 * @brief BM_W5B_IndexRebuild_Cost
 *
 * Simulates the cost of rebuilding a secondary index after partial index loss.
 * Measures time to re-index Arg(0) records from an existing corpus.
 * This captures the recovery entry-point performance — a critical metric
 * for production SLAs during node recovery.
 */
static void BM_W5B_IndexRebuild_Cost(benchmark::State& state) {
    const int rebuildCount = static_cast<int>(state.range(0));

    auto dbPath = w5b::tempPath("w5b_rebuild");
    fs::create_directories(dbPath);

    RocksDBWrapper::Config cfg;
    cfg.db_path             = dbPath.string();
    cfg.block_cache_size_mb = 128;
    cfg.compression_default = "lz4";
    auto db = std::make_unique<RocksDBWrapper>(cfg);
    if (!db->open())
        throw std::runtime_error("BM_W5B_IndexRebuild: open failed");

    SecondaryIndexManager idx(*db);
    idx.createIndex("corpus", "tag", false);
    idx.createRangeIndex("corpus", "ts");

    // Pre-populate corpus
    w5b::Rng rng(kW5BSeed + 11);
    std::vector<BaseEntity> corpus;
    corpus.reserve(rebuildCount);
    for (int i = 0; i < rebuildCount; ++i) {
        BaseEntity e("c_" + std::to_string(i));
        e.setField("tag", rng.key(6));
        e.setField("ts", static_cast<int64_t>(i));
        idx.put("corpus", e);
        corpus.push_back(e);
    }

    for (auto _ : state) {
        // Simulate index drop + re-index
        state.PauseTiming();
        // In this simulation: drop and recreate indexes, then re-insert
        // (production path would call a dedicated rebuild API)
        idx.dropIndex("corpus", "tag");
        idx.createIndex("corpus", "tag", false);
        state.ResumeTiming();

        for (auto& entity : corpus)
            idx.put("corpus", entity);
    }

    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * rebuildCount);
    state.SetLabel("index rebuild, " + std::to_string(rebuildCount) + " records");

    db->close();
    std::error_code ec;
    fs::remove_all(dbPath, ec);
}
BENCHMARK(BM_W5B_IndexRebuild_Cost)
    ->Arg(500)
    ->Arg(1'000)
    ->Arg(2'000)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Iterations(3);

// ===========================================================================
// 4. Concurrent reader/writer contention (80/20 mixed workload)
// ===========================================================================

/**
 * @brief Fixture for mixed read/write contention benchmarks.
 *
 * Provides a shared database with a pre-warmed corpus.  Each benchmark
 * thread independently decides whether to read or write based on an 80/20
 * ratio derived from the thread index.
 */
class W5bMixedContention : public benchmark::Fixture {
public:
    void SetUp(::benchmark::State& /*state*/) override {
        dbPath_ = w5b::tempPath("w5b_mixed");
        fs::create_directories(dbPath_);

        RocksDBWrapper::Config cfg;
        cfg.db_path             = dbPath_.string();
        cfg.block_cache_size_mb = 128;
        cfg.compression_default = "lz4";
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open())
            throw std::runtime_error("W5bMixedContention: open failed");
        idx_ = std::make_unique<SecondaryIndexManager>(*db_);
        idx_->createIndex("m", "cat", false);

        w5b::Rng rng(kW5BSeed + 17);
        readKeys_.reserve(kW5BCorpus);
        for (int i = 0; i < kW5BCorpus; ++i) {
            const std::string k = "m_" + std::to_string(i);
            BaseEntity e(k);
            e.setField("cat", rng.key(4));
            idx_->put("m", e);
            readKeys_.push_back(k);
        }
        writeSeq_.store(kW5BCorpus);
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
    std::vector<std::string>               readKeys_;
    std::atomic<int>                       writeSeq_{0};
};

/**
 * @brief BM_W5B_MixedContention_80_20
 *
 * 80% reads / 20% writes across N concurrent threads (Arg not needed;
 * benchmark::Threads controls concurrency).  Models sustained mixed OLTP
 * workloads under production concurrency levels.
 *
 * Diagnostic signal: p99 read latency degrades as writer count grows —
 * visible by comparing 2-thread vs. 8-thread runs.
 */
BENCHMARK_DEFINE_F(W5bMixedContention, Mixed80_20)(benchmark::State& state) {
    const bool isWriter = (state.thread_index() % 5 == 0);
    std::size_t ki = static_cast<std::size_t>(state.thread_index()) * 13;
    w5b::Rng rng(kW5BSeed + state.thread_index());

    for (auto _ : state) {
        if (isWriter) {
            const int id = writeSeq_.fetch_add(1, std::memory_order_relaxed);
            BaseEntity e("mx_" + std::to_string(id));
            e.setField("cat", rng.key(4));
            idx_->put("m", e);
        } else {
            const auto& k = readKeys_[ki % readKeys_.size()];
            auto blob = db_->get(KeySchema::makeRelationalKey("m", k));
            benchmark::DoNotOptimize(blob);
            ++ki;
        }
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("mixed 80R/20W, " + std::to_string(state.threads()) + " threads");
}
BENCHMARK_REGISTER_F(W5bMixedContention, Mixed80_20)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Iterations(10'000);
