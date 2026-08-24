// Benchmark: Continuous Query Engine Performance
// Measures tuple ingestion throughput and window evaluation tick overhead
// for the ContinuousQueryEngine introduced in Phase 8.1–8.4.
//
// CQ-PERF-01: BM_ContinuousQuery_Throughput
//   Injects N tuples into a sliding time-window query and measures
//   the sustained throughput in tuples/second.  Target: ≥ 500 k tuples/s.
//
// CQ-PERF-02: BM_ContinuousQuery_WindowTick
//   Times a single evaluation tick on an otherwise empty window.
//   Target: ≤ 1 µs per tick.

#include "query/continuous_query_engine_impl.h"
#include "query/continuous_query_engine.h"
#include "query/window_spec.h"
#include "query/synopsis_store.h"
#include "query/incremental_agg.h"
#include "query/cq_watermark.h"

#include <benchmark/benchmark.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

namespace {

using namespace themis::query;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Build a minimal JSON-serialised sensor tuple.
/// Avoid heavy allocations in the hot path by pre-building representative payloads.
inline std::string makeTuple(int64_t ts_us, double value) {
    // Compact representation; real production tuples average ~100 B.
    char buf[128];
    std::snprintf(buf, sizeof(buf),
                  R"({"ts":%lld,"v":%.4f,"src":"bench"})",
                  static_cast<long long>(ts_us), value);
    return std::string(buf);
}

/// A self-contained sliding time-window query specification used by all benchmarks.
ContinuousQuerySpec makeSlidingSpec(const std::string& name) {
    ContinuousQuerySpec spec;
    spec.name               = name;
    spec.source_collection  = "bench_stream";
    spec.window.type        = WindowSpec::Type::TIME_SLIDING;
    spec.window.range_ms    = 10'000;  // 10 s
    spec.window.slide_ms    = 1'000;   // 1 s
    spec.aql_body           = "FOR t IN __window__ COLLECT AGGREGATE s = SUM(t.v) RETURN s";
    spec.result_mode        = ResultMode::DELTA;
    spec.allowed_lateness_ms = 200;
    return spec;
}

// ---------------------------------------------------------------------------
// Fixture: spins up a ContinuousQueryEngineImpl per benchmark
// ---------------------------------------------------------------------------
class CQBenchFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& /*state*/) override {
        engine_ = std::make_unique<ContinuousQueryEngineImpl>(
            std::chrono::milliseconds{100});
        auto res = engine_->registerQuery(makeSlidingSpec("bench_q"));
        if (!res) {
            throw std::runtime_error("Failed to register benchmark query: " +
                                     res.error().message());
        }
    }

    void TearDown(const ::benchmark::State& /*state*/) override {
        if (engine_) {
            (void)engine_->dropQuery("bench_q");
            engine_.reset();
        }
    }

protected:
    std::unique_ptr<ContinuousQueryEngineImpl> engine_;
};

}  // namespace

// ============================================================================
// CQ-PERF-01: Tuple ingestion throughput
//
// Measures sustained tuples/second for a single sliding time-window query.
// The benchmark injects batches of 1 000 tuples per iteration and reports
// the achieved items/second rate.
//
// Target: ≥ 500 000 tuples/s on a 4-core host.
// ============================================================================
BENCHMARK_F(CQBenchFixture, BM_ContinuousQuery_Throughput)(benchmark::State& state) {
    constexpr int kBatchSize = 1000;

    // Pre-build payloads to avoid allocation overhead in the hot path.
    std::vector<std::string> payloads;
    payloads.reserve(kBatchSize);
    for (int i = 0; i < kBatchSize; ++i) {
        payloads.push_back(makeTuple(static_cast<int64_t>(i) * 1000, static_cast<double>(i)));
    }

    int64_t base_ts = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    int64_t total_tuples = 0;

    for (auto _ : state) {
        for (int i = 0; i < kBatchSize; ++i) {
            engine_->injectTuple("bench_stream", payloads[i], base_ts + total_tuples * 1000LL);
            ++total_tuples;
        }
    }

    state.SetItemsProcessed(total_tuples);
    state.counters["tuples_per_sec"] = benchmark::Counter(
        static_cast<double>(total_tuples),
        benchmark::Counter::kIsRate);
}

// ============================================================================
// CQ-PERF-01 — variant with per-tuple latency measurement
//
// Injects 1 tuple at a time and measures mean/p99 latency using
// Google Benchmark's manual timing API.
//
// Target: p99 per-tuple latency ≤ 5 ms.
// ============================================================================
BENCHMARK_F(CQBenchFixture, BM_ContinuousQuery_TupleLatency)(benchmark::State& state) {
    int64_t base_ts = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    int64_t seq = 0;

    for (auto _ : state) {
        const std::string payload = makeTuple(base_ts + seq * 1000LL, static_cast<double>(seq));
        auto t0 = std::chrono::high_resolution_clock::now();
        engine_->injectTuple("bench_stream", payload, base_ts + seq * 1000LL);
        auto t1 = std::chrono::high_resolution_clock::now();
        state.SetIterationTime(
            std::chrono::duration<double>(t1 - t0).count());
        ++seq;
    }

    state.SetItemsProcessed(seq);
}

// ============================================================================
// CQ-PERF-02: Empty-window tick overhead
//
// Times a single evaluation tick on a query whose window contains no tuples.
// Uses SynopsisStore and IncrementalAgg directly to isolate tick cost from
// ingestion overhead.
//
// Target: ≤ 1 µs per tick.
// ============================================================================
static void BM_ContinuousQuery_WindowTick(benchmark::State& state) {
    // Empty synopsis (no tuples — simulates an idle window interval).
    SynopsisStore store(10'000'000, 1ULL << 30);

    IncrementalAgg agg_sum(AggOp::SUM);
    IncrementalAgg agg_count(AggOp::COUNT);
    IncrementalAgg agg_avg(AggOp::AVG);

    // Window tick: expire old tuples + query current aggregate value.
    // Represents the work done every slide_ms when no new data has arrived.
    const int64_t window_start_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    for (auto _ : state) {
        // Expire (empty for empty store — but still exercises the code path).
        auto expired = store.expire(window_start_us);
        for (const auto& t : expired) {
            // Extract value — in a real pipeline this is a JSON field lookup.
            agg_sum.remove(0.0);
            agg_count.remove(0.0);
            agg_avg.remove(0.0);
        }

        // Read current aggregate.
        benchmark::DoNotOptimize(agg_sum.result());
        benchmark::DoNotOptimize(agg_count.result());
        benchmark::DoNotOptimize(agg_avg.result());
        benchmark::DoNotOptimize(store.size());
    }
}
BENCHMARK(BM_ContinuousQuery_WindowTick)->MinTime(1.0);

// ============================================================================
// CQ-PERF-02 — variant: tick with 1 000 active tuples in the window
//
// Times the tick cost when the window contains 1 000 entries and none expire.
// This exercises the aggregate read path without eviction.
// ============================================================================
static void BM_ContinuousQuery_WindowTick_1k(benchmark::State& state) {
    SynopsisStore store(10'000'000, 1ULL << 30);
    IncrementalAgg agg_sum(AggOp::SUM);

    // Pre-fill the window with 1 000 tuples.
    const int64_t now_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    for (int i = 0; i < 1000; ++i) {
        SynopsisTuple t;
        t.event_ts_us = now_us + static_cast<int64_t>(i) * 1000LL;
        t.payload     = makeTuple(t.event_ts_us, static_cast<double>(i));
        (void)store.insert(std::move(t));
        agg_sum.add(static_cast<double>(i));
    }

    // Tick with window_start far in the past → no expiry, all tuples remain.
    const int64_t old_window_start = now_us - 1'000'000'000LL;

    for (auto _ : state) {
        auto expired = store.expire(old_window_start);
        // No expired tuples — just read the aggregate.
        benchmark::DoNotOptimize(agg_sum.result());
        benchmark::DoNotOptimize(store.size());
    }
}
BENCHMARK(BM_ContinuousQuery_WindowTick_1k)->MinTime(1.0);

// ============================================================================
// CQ-PERF-02 — variant: full window expiry of 10 000 tuples
//
// Times the cost of expiring all 10 000 tuples in a window (worst-case eviction).
// ============================================================================
static void BM_ContinuousQuery_WindowExpiry_10k(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        SynopsisStore store(10'000'000, 1ULL << 30);
        IncrementalAgg agg_sum(AggOp::SUM);
        const int64_t base_us = 1'000'000'000LL;
        for (int i = 0; i < 10000; ++i) {
            SynopsisTuple t;
            t.event_ts_us = base_us + static_cast<int64_t>(i) * 1000LL;
            t.payload     = makeTuple(t.event_ts_us, static_cast<double>(i));
            (void)store.insert(std::move(t));
            agg_sum.add(static_cast<double>(i));
        }
        // Expire all tuples by advancing window past the last entry.
        const int64_t far_future = base_us + 100'000'000LL;
        state.ResumeTiming();

        auto expired = store.expire(far_future);
        for (const auto& t : expired) {
            agg_sum.remove(0.0);  // simplified; real pipeline parses t.payload
        }
        benchmark::DoNotOptimize(agg_sum.result());
        benchmark::DoNotOptimize(expired.size());
    }
}
BENCHMARK(BM_ContinuousQuery_WindowExpiry_10k)->MinTime(1.0);

BENCHMARK_MAIN();
