// benchmarks/timeseries/bench_timeseries_dedicated_gates.cpp
// Wave D dedicated benchmark gates for the Timeseries module.
// Gate IDs: TS-BM-01 .. TS-BM-04
//
// All benchmarks use in-process stubs; no external engine is required.
// A full hardware-representative run is required before Wave D sign-off
// (see src/timeseries/ROADMAP.md).

#include <benchmark/benchmark.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <random>
#include <vector>

// ---------------------------------------------------------------------------
// In-process stubs
// ---------------------------------------------------------------------------

namespace stubs {

struct TSIngest {
    std::atomic<uint64_t> ingested{0};
    bool ingest(uint64_t series, double value, int64_t ts_ns) {
        (void)series; (void)value; (void)ts_ns;
        ingested.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
};

struct TSQuery {
    std::atomic<uint64_t> queries{0};
    bool range_query(uint64_t series, int64_t start, int64_t end) {
        (void)series; (void)start; (void)end;
        queries.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
};

struct TSFlush {
    std::atomic<uint64_t> flushes{0};
    bool flush() {
        flushes.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
};

struct TSRemoteWrite {
    std::atomic<uint64_t> sent{0};
    bool write(uint64_t batch_id, int batch_size) {
        (void)batch_id; (void)batch_size;
        sent.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
};

} // namespace stubs

static stubs::TSIngest     g_ingest;
static stubs::TSQuery      g_query;
static stubs::TSFlush      g_flush;
static stubs::TSRemoteWrite g_rw;

// ---------------------------------------------------------------------------
// TS-BM-01 — Ingest throughput (samples/sec)
// ---------------------------------------------------------------------------
// Measures single-sample ingest throughput.
// Target: >= 500 000 samples/sec on representative hardware.
// ---------------------------------------------------------------------------
static void BM_TS_BM_01_IngestThroughput(benchmark::State& state) {
    std::mt19937_64 rng(1);
    std::uniform_real_distribution<double> vdist(0.0, 1e6);
    uint64_t series = 0;
    int64_t  ts     = 0;
    for (auto _ : state) {
        g_ingest.ingest(series++ % 10000, vdist(rng), ts++);
        benchmark::ClobberMemory();
    }
    state.SetLabel("TS-BM-01:ingest_throughput");
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_TS_BM_01_IngestThroughput)
    ->Threads(1)
    ->Threads(8)
    ->UseRealTime();

// ---------------------------------------------------------------------------
// TS-BM-02 — Range query p95 latency
// ---------------------------------------------------------------------------
// Measures per-query latency for a fixed time-window range scan.
// p95 target: < 5 ms on representative hardware.
// ---------------------------------------------------------------------------
static void BM_TS_BM_02_RangeQueryP95(benchmark::State& state) {
    uint64_t series = 0;
    for (auto _ : state) {
        g_query.range_query(series++ % 10000, 0LL, 3600LL * 1'000'000'000LL);
        benchmark::ClobberMemory();
    }
    state.SetLabel("TS-BM-02:range_query_p95");
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_TS_BM_02_RangeQueryP95)
    ->Threads(1)
    ->Threads(4)
    ->UseRealTime();

// ---------------------------------------------------------------------------
// TS-BM-03 — Flush latency
// ---------------------------------------------------------------------------
// Measures per-flush call latency.
// Target: p95 < 50 ms on representative hardware.
// ---------------------------------------------------------------------------
static void BM_TS_BM_03_FlushLatency(benchmark::State& state) {
    for (auto _ : state) {
        g_flush.flush();
        benchmark::ClobberMemory();
    }
    state.SetLabel("TS-BM-03:flush_latency");
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_TS_BM_03_FlushLatency)
    ->Threads(1)
    ->UseRealTime();

// ---------------------------------------------------------------------------
// TS-BM-04 — Remote-write throughput (batches/sec)
// ---------------------------------------------------------------------------
// Measures remote-write batch send throughput.
// Target: >= 10 000 batches/sec on representative hardware.
// ---------------------------------------------------------------------------
static void BM_TS_BM_04_RemoteWriteThroughput(benchmark::State& state) {
    uint64_t batch_id = 0;
    for (auto _ : state) {
        g_rw.write(batch_id++, 100 /*samples per batch*/);
        benchmark::ClobberMemory();
    }
    state.SetLabel("TS-BM-04:remote_write_throughput");
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_TS_BM_04_RemoteWriteThroughput)
    ->Threads(1)
    ->Threads(4)
    ->UseRealTime();

BENCHMARK_MAIN();
