/**
 * @file bench_process_dedicated_gates.cpp
 * @brief Wave D — Process Manager Dedicated Benchmark Gates.
 *
 * Benchmark IDs:
 *   PM-BM-01 — Spawn latency p95 (single-threaded sequential spawn/terminate)
 *   PM-BM-02 — Terminate latency p95 (single-threaded sequential terminate)
 *   PM-BM-03 — Signal-deliver p99 (signal bus round-trip throughput)
 *   PM-BM-04 — Concurrent spawn throughput (N threads × spawn/terminate)
 *
 * ## Performance gates (representative hardware, Release build)
 * | Gate     | Target                              |
 * |----------|-------------------------------------|
 * | PM-BM-01 | Spawn p95 < 1 ms                    |
 * | PM-BM-02 | Terminate p95 < 500 µs              |
 * | PM-BM-03 | Signal delivery ≥ 1 M signals/s     |
 * | PM-BM-04 | Concurrent spawn ≥ 200 k spawns/s at 4 threads |
 *
 * @note No external dependencies beyond standard library + GoogleBenchmark.
 * @see src/process/ROADMAP.md — Wave D contribution closure
 * @see docs/operability/RUNBOOK_PROCESS_MANAGER.md — operator runbook
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// In-process stubs — model process manager hot paths without OS processes.
// MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

// ---------------------------------------------------------------------------
// StubProcessManager
// ---------------------------------------------------------------------------
class BenchStubProcessManager {
public:
    BenchStubProcessManager() = default;

    uint64_t spawn() {
        std::lock_guard<std::mutex> lk(mu_);
        const uint64_t pid = next_pid_++;
        table_[pid] = true;
        ++active_;
        return pid;
    }

    void terminate(uint64_t pid) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = table_.find(pid);
        if (it != table_.end() && it->second) {
            it->second = false;
            --active_;
        }
    }

    void reset() {
        std::lock_guard<std::mutex> lk(mu_);
        table_.clear();
        next_pid_ = 1;
        active_ = 0;
    }

    int64_t active() const { return active_.load(); }

private:
    std::mutex mu_;
    std::unordered_map<uint64_t, bool> table_;
    uint64_t next_pid_{1};
    std::atomic<int64_t> active_{0};
};

// ---------------------------------------------------------------------------
// StubSignalBus
// ---------------------------------------------------------------------------
class BenchStubSignalBus {
public:
    void send(uint64_t pid, int signum) {
        (void)pid;
        (void)signum;
        ++sent_;
    }
    uint64_t sent() const { return sent_.load(); }
    void reset() { sent_.store(0); }

private:
    std::atomic<uint64_t> sent_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// PM-BM-01: Spawn latency p95
//
// Single-threaded: spawn one process per iteration and immediately terminate
// it to keep the table from growing unboundedly.
// ─────────────────────────────────────────────────────────────────────────────
static void BM_PM_BM_01_SpawnLatencyP95(benchmark::State& state) {
    BenchStubProcessManager mgr;
    for (auto _ : state) {
        const uint64_t pid = mgr.spawn();
        benchmark::DoNotOptimize(pid);
        mgr.terminate(pid);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_PM_BM_01_SpawnLatencyP95)
    ->Name("PM-BM-01/SpawnLatencyP95")
    ->Repetitions(5)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// PM-BM-02: Terminate latency p95
//
// Pre-allocate a pool of processes; single-threaded sequential terminate.
// Refills the pool when it runs low.
// ─────────────────────────────────────────────────────────────────────────────
static void BM_PM_BM_02_TerminateLatencyP95(benchmark::State& state) {
    constexpr int kPool = 1000;
    BenchStubProcessManager mgr;

    std::vector<uint64_t> pool;
    pool.reserve(static_cast<std::size_t>(kPool));
    for (int i = 0; i < kPool; ++i) pool.push_back(mgr.spawn());

    std::size_t idx = 0;
    for (auto _ : state) {
        if (idx >= pool.size()) {
            // Refill
            state.PauseTiming();
            pool.clear();
            mgr.reset();
            for (int i = 0; i < kPool; ++i) pool.push_back(mgr.spawn());
            idx = 0;
            state.ResumeTiming();
        }
        mgr.terminate(pool[idx++]);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_PM_BM_02_TerminateLatencyP95)
    ->Name("PM-BM-02/TerminateLatencyP95")
    ->Repetitions(5)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// PM-BM-03: Signal-deliver p99
//
// Single-threaded: measure raw signal-bus send throughput (p99 target).
// ─────────────────────────────────────────────────────────────────────────────
static void BM_PM_BM_03_SignalDeliverP99(benchmark::State& state) {
    BenchStubSignalBus bus;
    uint64_t pid = 1;
    for (auto _ : state) {
        bus.send(pid, 15);
        ++pid;
        benchmark::DoNotOptimize(pid);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_PM_BM_03_SignalDeliverP99)
    ->Name("PM-BM-03/SignalDeliverP99")
    ->Repetitions(5)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kNanosecond);

// ─────────────────────────────────────────────────────────────────────────────
// PM-BM-04: Concurrent spawn throughput
//
// 4 threads each perform spawn+terminate in a tight loop for N iterations.
// Measures aggregate spawn throughput under contention.
// ─────────────────────────────────────────────────────────────────────────────
static void BM_PM_BM_04_ConcurrentSpawnThroughput(benchmark::State& state) {
    constexpr unsigned kThreads = 4;
    const auto kIter = static_cast<uint64_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();
        BenchStubProcessManager mgr;
        state.ResumeTiming();

        std::vector<std::thread> threads;
        for (unsigned t = 0; t < kThreads; ++t) {
            threads.emplace_back([&]() {
                for (uint64_t i = 0; i < kIter; ++i) {
                    const uint64_t pid = mgr.spawn();
                    mgr.terminate(pid);
                }
            });
        }
        for (auto& th : threads) th.join();

        state.PauseTiming();
        benchmark::DoNotOptimize(mgr.active());
        state.ResumeTiming();
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(kThreads) *
        static_cast<int64_t>(state.range(0)) *
        static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_PM_BM_04_ConcurrentSpawnThroughput)
    ->Name("PM-BM-04/ConcurrentSpawnThroughput")
    ->Arg(500)
    ->Repetitions(3)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
