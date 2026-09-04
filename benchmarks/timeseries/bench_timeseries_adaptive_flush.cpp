/*
 * bench_timeseries_adaptive_flush.cpp
 *
 * Benchmark suite for TSAutoBuffer adaptive batch flush (PERF-D1).
 *
 * Acceptance criteria validated here:
 *   • Sustained ≥500k pts/s single- and multi-threaded write throughput
 *   • P99 write latency <100µs per add() call
 *   • FlushController adaptive batch-size convergence
 *   • Buffer statistics and backpressure counter exposure
 *
 * Run with:
 *   bench_timeseries_adaptive_flush --benchmark_filter=AdaptiveFlush
 *
 * Performance target: ≥500k pts/s (NVMe hardware, THEMIS_RUN_PERF_TESTS=1)
 */

#include "timeseries/ts_auto_buffer.h"
#include "timeseries/ts_auto_buffer_adaptive.h"
#include "timeseries/timeseries_metrics.h"
#include "timeseries/tsstore.h"
#include "storage/rocksdb_wrapper.h"

#include <benchmark/benchmark.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace themis;
namespace fs = std::filesystem;

// ============================================================================
// Fixture: opens a temporary RocksDB + TSStore + TSAutoBuffer
// ============================================================================

class AdaptiveFlushFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        // In multi-threaded Google Benchmark fixtures, SetUp is called once per
        // thread on the *same* fixture instance concurrently.  Only thread 0
        // initialises shared state; all other threads wait until it is ready.
        if (state.thread_index() != 0) {
            // Spin-wait until thread 0 has finished SetUp.
            while (!setup_done_.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            return;
        }
        auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        db_path_ = "/tmp/bench_adaptive_flush_" + std::to_string(ns);

        if (fs::exists(db_path_)) {
          fs::remove_all(db_path_);
        }

        RocksDBWrapper::Config db_cfg;
        db_cfg.db_path              = db_path_;
        db_cfg.memtable_size_mb     = 256;
        db_cfg.block_cache_size_mb  = 512;
        db_cfg.enable_blobdb        = false;

        db_ = std::make_unique<RocksDBWrapper>(db_cfg);
        if (!db_->open()) {
            throw std::runtime_error("AdaptiveFlushFixture: failed to open RocksDB at " + db_path_);
        }

        tsstore_ = std::make_unique<TSStore>(db_->getRawDB());

        TSAutoBufferConfig buf_cfg;
        buf_cfg.max_points_per_buffer      = 10000;
        buf_cfg.max_total_points           = 100000;
        buf_cfg.flush_interval             = std::chrono::milliseconds(50);
        buf_cfg.max_memory_bytes           = 256UL * 1024 * 1024; // 256 MB
        buf_cfg.async_flush                = true;
        buf_cfg.flush_batch_size           = 2000;
        buf_cfg.enable_adaptive_flush      = true;
        buf_cfg.backpressure_slo_ms        = 20.0;
        buf_cfg.ewma_alpha                 = 0.15;
        buf_cfg.adaptive_batch_min         = 500;
        buf_cfg.adaptive_batch_max         = 10000;
        buf_cfg.backpressure_high_watermark = 80000;
        buf_cfg.backpressure_low_watermark  = 20000;
        buf_cfg.overdue_flush_multiplier    = 3;

        buffer_ = std::make_unique<TSAutoBuffer>(tsstore_.get(), buf_cfg);
        buffer_->start();
        setup_done_.store(true, std::memory_order_release);
    }

    void TearDown(const ::benchmark::State& state) override {
        if (state.thread_index() != 0) {
          return;
        }
        if (buffer_) {
          buffer_->stop();
        }
        buffer_.reset();
        tsstore_.reset();
        if (db_) {
          db_->close();
        }
        db_.reset();
        std::error_code ec;
        if (fs::exists(db_path_)) {
          fs::remove_all(db_path_, ec);
        }
        setup_done_.store(false, std::memory_order_release);
    }

protected:
    static TSStore::DataPoint makePoint(const std::string& metric,
                                        const std::string& entity,
                                        int64_t ts_ms,
                                        double val) {
        TSStore::DataPoint p;
        p.metric       = metric;
        p.entity       = entity;
        p.timestamp_ms = ts_ms;
        p.value        = val;
        return p;
    }

    std::atomic<bool>               setup_done_{false};
    std::string                     db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<TSStore>        tsstore_;
    std::unique_ptr<TSAutoBuffer>   buffer_;
};

// ============================================================================
// BM_AdaptiveFlush_SingleThreaded
//
// Measures sustained single-threaded TSAutoBuffer::add() throughput with
// adaptive flush enabled.  Target: ≥500k pts/s.
// ============================================================================

BENCHMARK_DEFINE_F(AdaptiveFlushFixture, SingleThreaded)(benchmark::State& state) {
    const std::string metric = "cpu_usage";
    const std::string entity = "server_0";

    std::mt19937                          rng(42);
    std::uniform_real_distribution<double> val_dist(0.0, 100.0);

    int64_t ts = 1700000000000LL;

    for (auto _ : state) {
        auto pt = makePoint(metric, entity, ts++, val_dist(rng));
        auto res = buffer_->add(pt);
        benchmark::DoNotOptimize(res);
        if (!res) {
            state.SkipWithError("TSAutoBuffer::add failed");
            return;
        }
    }

    // Flush remainder
    state.PauseTiming();
    buffer_->flush();
    state.ResumeTiming();

    auto stats = buffer_->getStats();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.counters["points_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()), benchmark::Counter::kIsRate);
    state.counters["flush_count"]    = static_cast<double>(stats.flush_count.load());
    state.counters["adaptive_batch"] = static_cast<double>(stats.current_adaptive_batch_size);
}

BENCHMARK_REGISTER_F(AdaptiveFlushFixture, SingleThreaded)
    ->Threads(1)
    ->MinTime(2.0)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// BM_AdaptiveFlush_MultiThreaded
//
// Measures multi-threaded write throughput via TSAutoBuffer::add().
// Each thread writes to a distinct metric:entity to avoid hot-key contention.
// Target: ≥500k pts/s aggregate across all threads.
// ============================================================================

BENCHMARK_DEFINE_F(AdaptiveFlushFixture, MultiThreaded)(benchmark::State& state) {
    const int    tid    = state.thread_index();
    const std::string metric = "sensor";
    const std::string entity = "node_" + std::to_string(tid);

    std::mt19937                          rng(static_cast<uint32_t>(42 + tid));
    std::uniform_real_distribution<double> val_dist(0.0, 100.0);

    int64_t ts = 1700000000000LL + (tid * 1'000'000LL);

    for (auto _ : state) {
        auto pt  = makePoint(metric, entity, ts++, val_dist(rng));
        auto res = buffer_->add(pt);
        benchmark::DoNotOptimize(res);
        if (!res) {
            state.SkipWithError("TSAutoBuffer::add failed (multi-thread)");
            return;
        }
    }

    // Only the first thread flushes to avoid double-counting
    if (tid == 0) {
        state.PauseTiming();
        buffer_->flush();
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.counters["points_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()),
        benchmark::Counter::kIsRate | benchmark::Counter::kAvgThreads);
}

BENCHMARK_REGISTER_F(AdaptiveFlushFixture, MultiThreaded)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->MinTime(2.0)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// BM_AdaptiveFlush_P99Latency
//
// Records individual add() call durations, then reports P50/P95/P99 latency.
// Target: P99 <100µs.
// ============================================================================

BENCHMARK_DEFINE_F(AdaptiveFlushFixture, P99Latency)(benchmark::State& state) {
    const std::string metric = "latency_probe";
    const std::string entity = "probe_0";

    std::mt19937                          rng(99);
    std::uniform_real_distribution<double> val_dist(0.0, 1000.0);

    // Pre-allocate for up to 4M samples to avoid reallocations during measurement.
    static constexpr size_t kMaxLatencySamples = 4 * 1024 * 1024;
    std::vector<double> latencies_us;
    latencies_us.reserve(kMaxLatencySamples);

    int64_t ts = 1700000000000LL;

    for (auto _ : state) {
        auto pt = makePoint(metric, entity, ts++, val_dist(rng));

        auto t0  = std::chrono::steady_clock::now();
        auto res = buffer_->add(pt);
        auto t1  = std::chrono::steady_clock::now();

        benchmark::DoNotOptimize(res);

        double lat_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        latencies_us.push_back(lat_us);
    }

    // Compute percentiles
    state.PauseTiming();
    std::sort(latencies_us.begin(), latencies_us.end());
    state.ResumeTiming();

    auto pct = [&](double p) -> double {
        if (latencies_us.empty()) {
          return 0.0;
        }
        size_t idx = static_cast<size_t>(std::ceil(p / 100.0 * latencies_us.size())) - 1;
        idx = std::min(idx, latencies_us.size() - 1);
        return latencies_us[idx];
    };

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.counters["p50_us"]  = pct(50.0);
    state.counters["p95_us"]  = pct(95.0);
    state.counters["p99_us"]  = pct(99.0);
    state.counters["p999_us"] = pct(99.9);
    state.counters["points_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()), benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(AdaptiveFlushFixture, P99Latency)
    ->Threads(1)
    ->MinTime(2.0)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// BM_AdaptiveFlush_BatchWatermark
//
// Drives high-volume ingestion until the watermark is triggered, then
// measures flush throughput at different batch sizes.
// ============================================================================

BENCHMARK_DEFINE_F(AdaptiveFlushFixture, BatchWatermark)(benchmark::State& state) {
    const int    batch_sz = static_cast<int>(state.range(0));
    const std::string metric  = "bulk";
    const std::string entity  = "loader";

    std::mt19937                          rng(7);
    std::uniform_real_distribution<double> val_dist(0.0, 100.0);

    int64_t ts = 1700000000000LL;
    size_t  total_pts = 0;

    for (auto _ : state) {
        state.PauseTiming();
        std::vector<TSStore::DataPoint> batch;
        batch.reserve(static_cast<size_t>(batch_sz));
        for (int i = 0; i < batch_sz; ++i) {
            batch.push_back(makePoint(metric, entity, ts++, val_dist(rng)));
        }
        state.ResumeTiming();

        for (auto& pt : batch) {
            auto res = buffer_->add(pt);
            benchmark::DoNotOptimize(res);
        }
        total_pts += static_cast<size_t>(batch_sz);
    }

    state.PauseTiming();
    buffer_->flush();
    state.ResumeTiming();

    state.SetItemsProcessed(static_cast<int64_t>(total_pts));
    state.counters["batch_size"] = static_cast<double>(batch_sz);
    state.counters["points_per_sec"] = benchmark::Counter(
        static_cast<double>(total_pts), benchmark::Counter::kIsRate);

    auto stats = buffer_->getStats();
    state.counters["adaptive_batch"] = static_cast<double>(stats.current_adaptive_batch_size);
    state.counters["ewma_latency_ms"] = stats.current_ewma_latency_ms;
}

BENCHMARK_REGISTER_F(AdaptiveFlushFixture, BatchWatermark)
    ->Arg(100)
    ->Arg(500)
    ->Arg(1000)
    ->Arg(5000)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// BM_FlushController_Standalone
//
// Benchmarks the FlushController's reportFlushLatency + recommendedBatchSize
// hot path in isolation (no I/O).
// ============================================================================

static void BM_FlushController_Standalone(benchmark::State& state) {
    FlushControllerConfig cfg;
    cfg.slo_threshold_ms   = 10.0;
    cfg.ewma_alpha         = 0.2;
    cfg.min_batch_size     = 100;
    cfg.max_batch_size     = 10000;
    cfg.initial_batch_size = 1000;
    cfg.warmup_samples     = 0; // disable warmup for benchmark

    FlushController ctrl(cfg);

    std::mt19937                          rng(42);
    std::uniform_real_distribution<double> lat_dist(1.0, 20.0);

    for (auto _ : state) {
        double lat_ms = lat_dist(rng);
        ctrl.reportFlushLatency(lat_ms);
        size_t batch = ctrl.recommendedBatchSize();
        benchmark::DoNotOptimize(batch);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.counters["updates_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()), benchmark::Counter::kIsRate);

    auto s = ctrl.stats();
    state.counters["final_batch_sz"]  = static_cast<double>(s.current_batch_sz);
    state.counters["ewma_latency_ms"] = s.ewma_latency_ms;
}

BENCHMARK(BM_FlushController_Standalone)
    ->Threads(1)
    ->Threads(4)
    ->MinTime(1.0)
    ->Unit(benchmark::kNanosecond);

// ============================================================================
// BM_AdaptiveFlush_StatsExposure
//
// Verifies that buffer statistics and overdue-flush counters are accessible
// (regression guard for acceptance criterion AC-4).
// ============================================================================

BENCHMARK_DEFINE_F(AdaptiveFlushFixture, StatsExposure)(benchmark::State& state) {
    const std::string metric = "stats_probe";
    const std::string entity = "probe";

    TimeSeriesMetrics tsmetrics;

    // Wire metrics into buffer config
    TSAutoBufferConfig updated_cfg = buffer_->getConfig();
    updated_cfg.metrics            = &tsmetrics;
    buffer_->setConfig(updated_cfg);

    std::mt19937                          rng(11);
    std::uniform_real_distribution<double> val_dist(0.0, 100.0);

    int64_t ts = 1700000000000LL;

    for (auto _ : state) {
        auto pt = makePoint(metric, entity, ts++, val_dist(rng));
        auto res = buffer_->add(pt);
        benchmark::DoNotOptimize(res);
    }

    state.PauseTiming();
    buffer_->flush();
    auto buf_stats = buffer_->getStats();
    state.ResumeTiming();

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.counters["points_buffered"]   = static_cast<double>(buf_stats.points_buffered.load());
    state.counters["points_flushed"]    = static_cast<double>(buf_stats.points_flushed.load());
    state.counters["backpressure_evts"] = static_cast<double>(buf_stats.backpressure_events.load());
    state.counters["overdue_flush_evts"]= static_cast<double>(
        tsmetrics.getTotalOverdueFlushEvents());
    state.counters["ts_backpressure"]   = static_cast<double>(
        tsmetrics.getTotalBackpressureEvents());
    state.counters["points_per_sec"]    = benchmark::Counter(
        static_cast<double>(state.iterations()), benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(AdaptiveFlushFixture, StatsExposure)
    ->Threads(1)
    ->MinTime(1.0)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
