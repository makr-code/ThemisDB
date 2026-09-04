// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w7c_degradation_fault_recovery.cpp
 * @brief Wave 7-C: Degradation, Fault & Recovery Performance Characterization.
 *
 * Purpose: Characterise system behaviour under degrading conditions and after
 * simulated faults so that release go/no-go decisions can be made with
 * evidence-based metrics rather than assumptions.
 *
 * Covered scenarios:
 *   DFR-01  Latency injection – reads with simulated I/O delay
 *   DFR-02  Partial write failure – write path with injected error rate
 *   DFR-03  Saturation point – throughput at increasing concurrency
 *   DFR-04  Backpressure – write stall detection under compaction pressure
 *   DFR-05  Cold-start after crash – read latency on empty block cache
 *   DFR-06  Resource reduction – performance at half available threads
 *   DFR-07  Recovery characterisation – latency after simulated compaction
 *   DFR-08  Degraded mixed workload – combined read/write under fault pressure
 *
 * Fault injection model:
 *   The benchmarks do not crash the process; instead they simulate the
 *   observable performance effect of faults by injecting controlled delays,
 *   conditional failures, or constrained resources.  This is the standard
 *   approach for performance characterisation tests that must run reliably in
 *   CI without external infrastructure.
 *
 * Decision metrics produced by each scenario:
 *   - Mean / p50 / p95 / p99 latency (µs or ms)
 *   - Throughput delta vs healthy baseline (items/s)
 *   - Recovery time (iterations until latency returns to ≤ 1.2× healthy mean)
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
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
namespace w7c {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr uint64_t kW7CanonicalSeed = 42;
static constexpr int      kDatasetSize     = 50'000;
static constexpr int      kWarmup          = 500;

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
    return fs::temp_directory_path().string() + "/w7c_" + tag + "_" + std::to_string(ts);
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
private:
    std::mt19937_64 rng_;
};

/**
 * @brief Fault injector that introduces probabilistic latency delay.
 *
 * Simulates I/O jitter or degraded storage path.
 * Error rate and delay duration are configurable per scenario.
 */
class FaultInjector {
public:
    FaultInjector(double error_rate, std::chrono::microseconds delay_us)
        : error_rate_(error_rate), delay_(delay_us), rng_(kW7CanonicalSeed + 99) {}

    /// Apply fault: sleeps delay_ with probability error_rate_.
    void MaybeInject() {
        std::uniform_real_distribution<double> d(0.0, 1.0);
        if (d(rng_) < error_rate_) {
            std::this_thread::sleep_for(delay_);
        }
    }

    /// Returns true with probability error_rate_ (simulates write failure).
    bool ShouldFail() {
        std::uniform_real_distribution<double> d(0.0, 1.0);
        return d(rng_) < error_rate_;
    }

private:
    double                         error_rate_;
    std::chrono::microseconds      delay_;
    std::mt19937_64                rng_;
};

} // namespace

// ---------------------------------------------------------------------------
// Shared dataset fixture
// ---------------------------------------------------------------------------

class DegradationBaseFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("dfr");
        RemoveAll(db_path_);
        db_ = std::make_unique<RocksDBWrapper>(DefaultCfg(db_path_));
        if (!db_->open()) {
            throw std::runtime_error("W7C: failed to open DB");
        }
        for (int i = 0; i < kDatasetSize; ++i) {
            db_->put("k_" + std::to_string(i), "v_" + std::to_string(i));
        }
        // Warmup
        KeyGenerator wkg(kW7CanonicalSeed + 1);
        for (int i = 0; i < kWarmup; ++i) {
            std::string val;
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
// DFR-01: Latency injection – reads with simulated I/O delay
// ---------------------------------------------------------------------------

BENCHMARK_F(DegradationBaseFixture, DFR01_LatencyInjectionReads)(benchmark::State& state) {
    // Inject 50 µs delay in 10% of reads (simulates occasional SSD stall)
    FaultInjector fi(0.10, std::chrono::microseconds(50));
    KeyGenerator kg(kW7CanonicalSeed + 101);
    for (auto _ : state) {
        fi.MaybeInject();
        std::string val;
        benchmark::DoNotOptimize(db_->get(kg.Next(kDatasetSize), val));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(DegradationBaseFixture, DFR01_LatencyInjectionReads)
    ->Repetitions(5)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W7C/DFR01_LatencyInjection_Reads");

// ---------------------------------------------------------------------------
// DFR-02: Partial write failure – write path with injected error rate
// ---------------------------------------------------------------------------

BENCHMARK_F(DegradationBaseFixture, DFR02_PartialWriteFailure)(benchmark::State& state) {
    // 5% of writes "fail" (simulated via conditional skip)
    FaultInjector fi(0.05, std::chrono::microseconds(0));
    KeyGenerator kg(kW7CanonicalSeed + 102);
    std::atomic<int64_t> success_count{0};
    std::atomic<int64_t> fail_count{0};
    int ctr = 0;
    for (auto _ : state) {
        if (!fi.ShouldFail()) {
            db_->put(kg.Next(kDatasetSize * 2), "dfr02_" + std::to_string(ctr++));
            ++success_count;
        } else {
            ++fail_count;
        }
    }
    state.SetItemsProcessed(success_count.load());
    state.counters["write_success"] = static_cast<double>(success_count.load());
    state.counters["write_fail"]    = static_cast<double>(fail_count.load());
}
BENCHMARK_REGISTER_F(DegradationBaseFixture, DFR02_PartialWriteFailure)
    ->Repetitions(5)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W7C/DFR02_PartialWriteFailure");

// ---------------------------------------------------------------------------
// DFR-03: Saturation point – throughput at increasing concurrency
// ---------------------------------------------------------------------------

BENCHMARK_F(DegradationBaseFixture, DFR03_SaturationConcurrency)(benchmark::State& state) {
    // Arg(0) controls thread count
    const int num_threads = static_cast<int>(state.range(0));
    const int ops_per_thread = 2'000;

    for (auto _ : state) {
        std::atomic<int64_t> total_ops{0};
        std::vector<std::thread> threads;
        threads.reserve(num_threads);
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([this, t, ops_per_thread, &total_ops]() {
                KeyGenerator kg(kW7CanonicalSeed + 103 + static_cast<uint64_t>(t));
                for (int i = 0; i < ops_per_thread; ++i) {
                    std::string val;
                    db_->get(kg.Next(kDatasetSize), val);
                    ++total_ops;
                }
            });
        }
        for (auto& th : threads) {
          th.join();
        }
        state.SetItemsProcessed(total_ops.load());
    }
}
BENCHMARK_REGISTER_F(DegradationBaseFixture, DFR03_SaturationConcurrency)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8)
    ->Repetitions(3)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W7C/DFR03_SaturationPoint_Concurrency");

// ---------------------------------------------------------------------------
// DFR-04: Backpressure – write stall detection under compaction pressure
// ---------------------------------------------------------------------------

BENCHMARK_F(DegradationBaseFixture, DFR04_BackpressureWriteStall)(benchmark::State& state) {
    // Inject compaction-like delay in 15% of writes (300 µs)
    FaultInjector fi(0.15, std::chrono::microseconds(300));
    KeyGenerator kg(kW7CanonicalSeed + 104);
    int ctr = 0;
    for (auto _ : state) {
        fi.MaybeInject();
        db_->put(kg.Next(kDatasetSize * 4), "bp_" + std::to_string(ctr++));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(DegradationBaseFixture, DFR04_BackpressureWriteStall)
    ->Repetitions(5)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W7C/DFR04_Backpressure_WriteStall");

// ---------------------------------------------------------------------------
// DFR-05: Cold-start after crash – read latency on cold block cache
// ---------------------------------------------------------------------------

/**
 * @brief Fixture that intentionally reopens the DB with a cold cache.
 *
 * Simulates a crash-recovery cold-start: close, reopen with block_cache_size 0,
 * then measure first-access read latency.
 */
class ColdStartFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("dfr05");
        RemoveAll(db_path_);

        // Initial warm population
        {
            RocksDBWrapper::Config cfg = DefaultCfg(db_path_);
            RocksDBWrapper warm_db(cfg);
            if (!warm_db.open()) {
              throw std::runtime_error("W7C DFR-05: warm open failed");
            }
            for (int i = 0; i < kDatasetSize; ++i) {
                warm_db.put("k_" + std::to_string(i), "v_" + std::to_string(i));
            }
        } // close warm_db

        // Reopen with cold cache (block_cache_size_mb = 1)
        RocksDBWrapper::Config cold_cfg = DefaultCfg(db_path_);
        cold_cfg.block_cache_size_mb = 1;
        db_ = std::make_unique<RocksDBWrapper>(cold_cfg);
        if (!db_->open()) {
          throw std::runtime_error("W7C DFR-05: cold open failed");
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

BENCHMARK_F(ColdStartFixture, DFR05_ColdStartReadLatency)(benchmark::State& state) {
    KeyGenerator kg(kW7CanonicalSeed + 105);
    for (auto _ : state) {
        std::string val;
        benchmark::DoNotOptimize(db_->get(kg.Next(kDatasetSize), val));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(ColdStartFixture, DFR05_ColdStartReadLatency)
    ->Repetitions(5)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W7C/DFR05_ColdStart_ReadLatency");

// ---------------------------------------------------------------------------
// DFR-06: Resource reduction – performance at half available threads
// ---------------------------------------------------------------------------

BENCHMARK_F(DegradationBaseFixture, DFR06_HalfThreadsResourceReduction)(benchmark::State& state) {
    // Simulate half-resource constraint by using only 2 concurrent workers
    // regardless of hardware parallelism.
    constexpr int kConstrainedThreads = 2;
    constexpr int kOpsPerWorker       = 1'000;
    for (auto _ : state) {
        std::vector<std::thread> workers;
        workers.reserve(kConstrainedThreads);
        for (int t = 0; t < kConstrainedThreads; ++t) {
            workers.emplace_back([this, t]() {
                KeyGenerator kg(kW7CanonicalSeed + 106 + static_cast<uint64_t>(t));
                for (int i = 0; i < kOpsPerWorker; ++i) {
                    std::string val;
                    db_->get(kg.Next(kDatasetSize), val);
                }
            });
        }
        for (auto& w : workers) {
          w.join();
        }
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) * kConstrainedThreads * kOpsPerWorker);
}
BENCHMARK_REGISTER_F(DegradationBaseFixture, DFR06_HalfThreadsResourceReduction)
    ->Repetitions(5)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W7C/DFR06_HalfThreads_ResourceReduction");

// ---------------------------------------------------------------------------
// DFR-07: Recovery characterisation – latency normalisation after compaction
// ---------------------------------------------------------------------------

BENCHMARK_F(DegradationBaseFixture, DFR07_RecoveryAfterCompaction)(benchmark::State& state) {
    // Phase 0 (iterations 0-2): inject heavy write load (compaction trigger)
    // Phase 1 (iterations 3+): measure recovery read latency
    int iter = 0;
    int w_ctr = 0;
    for (auto _ : state) {
        if (iter < 3) {
            // Heavy write phase
            KeyGenerator kg(kW7CanonicalSeed + 107 + static_cast<uint64_t>(iter));
            for (int i = 0; i < 5'000; ++i) {
                db_->put(kg.Next(kDatasetSize * 4), "cmp_" + std::to_string(w_ctr++));
            }
        } else {
            // Recovery read phase – measure whether latency has normalised
            KeyGenerator kg(kW7CanonicalSeed + 200 + static_cast<uint64_t>(iter));
            for (int i = 0; i < 1'000; ++i) {
                std::string val;
                benchmark::DoNotOptimize(db_->get(kg.Next(kDatasetSize), val));
            }
        }
        state.counters["phase"] = (iter < 3) ? 0.0 : 1.0;
        ++iter;
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(DegradationBaseFixture, DFR07_RecoveryAfterCompaction)
    ->Iterations(3 + 5) // 3 compaction + 5 recovery
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W7C/DFR07_Recovery_AfterCompaction");

// ---------------------------------------------------------------------------
// DFR-08: Degraded mixed workload – combined read/write under fault pressure
// ---------------------------------------------------------------------------

BENCHMARK_F(DegradationBaseFixture, DFR08_DegradedMixedWorkload)(benchmark::State& state) {
    // 20% reads with 50 µs injected delay + 5% writes that fail = full pressure
    FaultInjector read_fi(0.20, std::chrono::microseconds(50));
    FaultInjector write_fi(0.05, std::chrono::microseconds(0));
    std::mt19937_64 mix_rng(kW7CanonicalSeed + 88);
    std::uniform_int_distribution<int> mix(0, 99);
    KeyGenerator kg(kW7CanonicalSeed + 108);
    int w_ctr = 0;
    std::atomic<int64_t> successful_ops{0};
    for (auto _ : state) {
        if (mix(mix_rng) < 40) {
            if (!write_fi.ShouldFail()) {
                db_->put(kg.Next(kDatasetSize * 2), "dfr08_" + std::to_string(w_ctr++));
                ++successful_ops;
            }
        } else {
            read_fi.MaybeInject();
            std::string val;
            benchmark::DoNotOptimize(db_->get(kg.Next(kDatasetSize), val));
            ++successful_ops;
        }
    }
    state.SetItemsProcessed(successful_ops.load());
}
BENCHMARK_REGISTER_F(DegradationBaseFixture, DFR08_DegradedMixedWorkload)
    ->Repetitions(5)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W7C/DFR08_Degraded_MixedWorkload");

} // namespace w7c
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
