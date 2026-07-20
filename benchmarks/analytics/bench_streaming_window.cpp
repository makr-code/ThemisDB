/**
 * @file bench_streaming_window.cpp
 * @brief Google Benchmark performance tests for streaming window runtime limit hardening.
 *
 * Covers the Q3 2026 roadmap item: "hardening of streaming and distributed runtime limits
 * under sustained load" (src/analytics/ROADMAP.md).
 *
 * Benchmarks:
 *   - BM_TumblingWindow_IngestThroughput      – baseline throughput, 1 M records
 *   - BM_TumblingWindow_SustainedLoad_Bounded – same with max_open_windows=10 (eviction overhead)
 *   - BM_TumblingWindow_FlushLatency          – flush() latency with N open windows
 *   - BM_SlidingWindow_IngestThroughput       – baseline throughput
 *   - BM_SlidingWindow_RecordLimitDrop        – throughput with max_records_per_window enforced
 *   - BM_SessionWindow_IngestThroughput       – baseline throughput, multi-key
 *   - BM_SessionWindow_BoundedSessions        – throughput with max_open_sessions=50 (eviction)
 *
 * Measurement hygiene:
 *   - Canonical seed: 42
 *   - No file I/O; all data in-memory
 *   - UseRealTime() for all benchmarks (wall-clock; scheduler jitter amortised over 1 M ops)
 */

#include <benchmark/benchmark.h>

#include <analytics/streaming_window.h>

#include <chrono>
#include <random>
#include <string>
#include <vector>

namespace {

// ═══════════════════════════════════════════════════════════
// Constants
// ═══════════════════════════════════════════════════════════

constexpr uint64_t kCanonicalSeed     = 42;
constexpr int64_t  kNumRecords        = 1'000'000;
constexpr int      kNumPartitionKeys  = 100;

// ═══════════════════════════════════════════════════════════
// Test Data Generation
// ═══════════════════════════════════════════════════════════

/**
 * @brief Generates a fixed set of @p n stream records spread over @p span_minutes minutes,
 *        using @p num_keys distinct partition keys.
 *
 * The canonical seed (42) guarantees deterministic, reproducible workloads across runs.
 *
 * @param n           Number of records to generate.
 * @param span_minutes Time span in minutes covered by the records.
 * @param num_keys    Number of distinct partition keys.
 * @return Vector of StreamRecord with monotonically advancing event times.
 */
std::vector<themis::analytics::StreamRecord> makeRecords(int64_t n,
                                                        int     span_minutes = 60,
                                                        int     num_keys     = kNumPartitionKeys) {
    using namespace std::chrono;
    std::mt19937_64 rng(kCanonicalSeed);

    const auto    base    = system_clock::time_point{};
    const int64_t span_us = static_cast<int64_t>(span_minutes) * 60LL * 1'000'000LL;

    std::vector<themis::analytics::StreamRecord> records;
    records.reserve(static_cast<size_t>(n));

    for (int64_t i = 0; i < n; ++i) {
        themis::analytics::StreamRecord r;
        const int64_t offset_us = (n > 0 && span_us > 0) ? ((i * span_us) / n) : 0;
        r.event_time     = base + microseconds(offset_us);
        r.partition_key  = "key_" + std::to_string(static_cast<int>(rng() % static_cast<uint64_t>(num_keys)));
        r.value          = static_cast<double>(rng() % 10000) / 100.0;
        records.push_back(std::move(r));
    }
    return records;
}

// ═══════════════════════════════════════════════════════════
// TumblingWindow benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * @brief Baseline throughput: 1 M records into a TumblingWindow without record/window limits.
 */
static void BM_TumblingWindow_IngestThroughput(benchmark::State &state) {
    using namespace themis::analytics;

    const auto records = makeRecords(kNumRecords);

    for (auto _ : state) {
        TumblingWindowConfig cfg;
        cfg.size = std::chrono::minutes(1);

        auto win = createTumblingWindow(cfg);
        for (const auto &r : records) {
            win->ingest(r);
        }
        benchmark::DoNotOptimize(win->getStats());
    }

    state.SetItemsProcessed(state.iterations() * kNumRecords);
}
BENCHMARK(BM_TumblingWindow_IngestThroughput)->UseRealTime()->Unit(benchmark::kMillisecond);

/**
 * @brief Sustained load with max_open_windows=10: measures overhead of window eviction
 *        when the open window count is bounded.
 */
static void BM_TumblingWindow_SustainedLoad_Bounded(benchmark::State &state) {
    using namespace themis::analytics;

    const auto records = makeRecords(kNumRecords);

    for (auto _ : state) {
        TumblingWindowConfig cfg;
        cfg.size             = std::chrono::minutes(1);
        cfg.max_open_windows = 10;

        auto win = createTumblingWindow(cfg);
        for (const auto &r : records) {
            win->ingest(r);
        }
        benchmark::DoNotOptimize(win->getStats());
    }

    state.SetItemsProcessed(state.iterations() * kNumRecords);
}
BENCHMARK(BM_TumblingWindow_SustainedLoad_Bounded)->UseRealTime()->Unit(benchmark::kMillisecond);

/**
 * @brief Flush latency with N open windows: measures the cost of a full flush()
 *        when there are many open windows pending.
 *
 * Parameterised over window count (8, 64, 512) to show latency scaling.
 */
static void BM_TumblingWindow_FlushLatency(benchmark::State &state) {
    using namespace themis::analytics;
    using namespace std::chrono;

    const int64_t window_count = state.range(0);
    // Use a 10-second window size; spread records so we open window_count distinct windows.
    const auto    window_size  = seconds(10);

    auto base = system_clock::now();

    for (auto _ : state) {
        state.PauseTiming();

        TumblingWindowConfig cfg;
        cfg.size = window_size;

        auto win = createTumblingWindow(cfg);

        // Ingest one record per window slot to open exactly window_count windows.
        for (int64_t i = 0; i < window_count; ++i) {
            themis::analytics::StreamRecord r;
            r.event_time    = base + seconds(i * 10);
            r.partition_key = "key_0";
            r.value         = static_cast<double>(i);
            win->ingest(r);
        }

        state.ResumeTiming();
        win->flush();
        benchmark::DoNotOptimize(win->getStats());
    }
}
BENCHMARK(BM_TumblingWindow_FlushLatency)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond)
    ->Arg(8)
    ->Arg(64)
    ->Arg(512);

// ═══════════════════════════════════════════════════════════
// SlidingWindow benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * @brief Baseline throughput for SlidingWindow without limits.
 */
static void BM_SlidingWindow_IngestThroughput(benchmark::State &state) {
    using namespace themis::analytics;

    const auto records = makeRecords(kNumRecords);

    for (auto _ : state) {
        SlidingWindowConfig cfg;
        cfg.size  = std::chrono::minutes(5);
        cfg.slide = std::chrono::minutes(1);

        auto win = createSlidingWindow(cfg);
        for (const auto &r : records) {
            win->ingest(r);
        }
        benchmark::DoNotOptimize(win->getStats());
    }

    state.SetItemsProcessed(state.iterations() * kNumRecords);
}
BENCHMARK(BM_SlidingWindow_IngestThroughput)->UseRealTime()->Unit(benchmark::kMillisecond);

/**
 * @brief Throughput with max_records_per_window enforced:
 *        measures drop-path overhead when every window is already full.
 */
static void BM_SlidingWindow_RecordLimitDrop(benchmark::State &state) {
    using namespace themis::analytics;

    const auto records = makeRecords(kNumRecords);

    for (auto _ : state) {
        SlidingWindowConfig cfg;
        cfg.size                   = std::chrono::minutes(5);
        cfg.slide                  = std::chrono::minutes(1);
        cfg.max_records_per_window = 100; // Small cap — most records will be dropped after initial fill.

        auto win = createSlidingWindow(cfg);
        for (const auto &r : records) {
            win->ingest(r);
        }
        benchmark::DoNotOptimize(win->getStats());
    }

    state.SetItemsProcessed(state.iterations() * kNumRecords);
}
BENCHMARK(BM_SlidingWindow_RecordLimitDrop)->UseRealTime()->Unit(benchmark::kMillisecond);

// ═══════════════════════════════════════════════════════════
// SessionWindow benchmarks
// ═══════════════════════════════════════════════════════════

/**
 * @brief Baseline throughput for SessionWindow with many partition keys.
 */
static void BM_SessionWindow_IngestThroughput(benchmark::State &state) {
    using namespace themis::analytics;

    const auto records = makeRecords(kNumRecords, 60, kNumPartitionKeys);

    for (auto _ : state) {
        SessionWindowConfig cfg;
        cfg.gap = std::chrono::seconds(30);

        auto win = createSessionWindow(cfg);
        for (const auto &r : records) {
            win->ingest(r);
        }
        benchmark::DoNotOptimize(win->getStats());
    }

    state.SetItemsProcessed(state.iterations() * kNumRecords);
}
BENCHMARK(BM_SessionWindow_IngestThroughput)->UseRealTime()->Unit(benchmark::kMillisecond);

/**
 * @brief Bounded sessions throughput: max_open_sessions=50 forces eviction of the oldest
 *        session when the 101st key arrives.  Measures eviction path overhead.
 */
static void BM_SessionWindow_BoundedSessions(benchmark::State &state) {
    using namespace themis::analytics;

    const auto records = makeRecords(kNumRecords, 60, kNumPartitionKeys);

    for (auto _ : state) {
        SessionWindowConfig cfg;
        cfg.gap              = std::chrono::seconds(30);
        cfg.max_open_sessions = 50; // Half the distinct keys — frequent eviction.

        auto win = createSessionWindow(cfg);
        for (const auto &r : records) {
            win->ingest(r);
        }
        benchmark::DoNotOptimize(win->getStats());
    }

    state.SetItemsProcessed(state.iterations() * kNumRecords);
}
BENCHMARK(BM_SessionWindow_BoundedSessions)->UseRealTime()->Unit(benchmark::kMillisecond);

} // namespace

BENCHMARK_MAIN();
