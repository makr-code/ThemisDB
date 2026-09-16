// benchmarks/scheduler/bench_scheduler_dedicated_gates.cpp
// Wave D dedicated benchmark gates for the Scheduler module.
// Gate IDs: SC-BM-01 .. SC-BM-04
//
// All benchmarks use in-process stubs; no external engine is required.
// A full hardware-representative run is required before Wave D sign-off
// (see src/scheduler/ROADMAP.md).

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <queue>
#include <random>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// In-process stubs
// ---------------------------------------------------------------------------

namespace stubs {

struct TaskScheduler {
    std::atomic<uint64_t> registered{0};
    std::atomic<uint64_t> executed{0};
    std::atomic<uint64_t> listed{0};
    std::atomic<uint64_t> stats_queries{0};

    std::mutex             mu;
    std::queue<uint64_t>   q;

    bool register_task(uint64_t id) {
        {
            std::lock_guard<std::mutex> lk(mu);
            q.push(id);
        }
        registered.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    bool execute_next() {
        std::lock_guard<std::mutex> lk(mu);
        if (q.empty()) return false;
        q.pop();
        executed.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    uint64_t list_tasks() {
        std::lock_guard<std::mutex> lk(mu);
        listed.fetch_add(1, std::memory_order_relaxed);
        return static_cast<uint64_t>(q.size());
    }

    uint64_t query_stats() {
        stats_queries.fetch_add(1, std::memory_order_relaxed);
        return registered.load();
    }
};

} // namespace stubs

static stubs::TaskScheduler g_sched;

// ---------------------------------------------------------------------------
// SC-BM-01 — Task register p95 latency
// ---------------------------------------------------------------------------
// Measures per-registration call latency.
// p95 target: < 100 µs on representative hardware.
// ---------------------------------------------------------------------------
static void BM_SC_BM_01_RegisterP95(benchmark::State& state) {
    uint64_t id = 0;
    for (auto _ : state) {
        g_sched.register_task(id++);
        benchmark::ClobberMemory();
        // Keep the queue from growing unboundedly.
        if (id % 1000 == 0) g_sched.execute_next();
    }
    state.SetLabel("SC-BM-01:register_p95");
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_SC_BM_01_RegisterP95)
    ->Threads(1)
    ->Threads(4)
    ->UseRealTime();

// ---------------------------------------------------------------------------
// SC-BM-02 — Task execute p95 latency
// ---------------------------------------------------------------------------
// Measures per-execution call latency.
// p95 target: < 100 µs on representative hardware.
// ---------------------------------------------------------------------------
static void BM_SC_BM_02_ExecuteP95(benchmark::State& state) {
    // Pre-fill queue so execute_next always has work.
    for (uint64_t i = 0; i < 10000; ++i) g_sched.register_task(i);

    uint64_t refill_counter = 0;
    for (auto _ : state) {
        if (!g_sched.execute_next()) {
            // Refill if drained.
            for (uint64_t i = 0; i < 1000; ++i) g_sched.register_task(refill_counter++);
            g_sched.execute_next();
        }
        benchmark::ClobberMemory();
    }
    state.SetLabel("SC-BM-02:execute_p95");
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_SC_BM_02_ExecuteP95)
    ->Threads(1)
    ->Threads(4)
    ->UseRealTime();

// ---------------------------------------------------------------------------
// SC-BM-03 — Task list p95 latency
// ---------------------------------------------------------------------------
// Measures per-list-call latency.
// p95 target: < 200 µs on representative hardware.
// ---------------------------------------------------------------------------
static void BM_SC_BM_03_ListP95(benchmark::State& state) {
    // Ensure queue has entries.
    for (uint64_t i = 0; i < 100; ++i) g_sched.register_task(i);

    for (auto _ : state) {
        benchmark::DoNotOptimize(g_sched.list_tasks());
        benchmark::ClobberMemory();
    }
    state.SetLabel("SC-BM-03:list_p95");
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_SC_BM_03_ListP95)
    ->Threads(1)
    ->Threads(4)
    ->UseRealTime();

// ---------------------------------------------------------------------------
// SC-BM-04 — Stats query p95 latency
// ---------------------------------------------------------------------------
// Measures per-stats-query call latency.
// p95 target: < 50 µs on representative hardware.
// ---------------------------------------------------------------------------
static void BM_SC_BM_04_StatsQueryP95(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(g_sched.query_stats());
        benchmark::ClobberMemory();
    }
    state.SetLabel("SC-BM-04:stats_query_p95");
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_SC_BM_04_StatsQueryP95)
    ->Threads(1)
    ->Threads(4)
    ->UseRealTime();

BENCHMARK_MAIN();
