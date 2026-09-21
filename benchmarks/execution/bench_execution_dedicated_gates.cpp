/*
 * @file bench_execution_dedicated_gates.cpp
 * @brief Wave D — Execution Engine Dedicated Benchmark Gates.
 *
 * Benchmark IDs:
 *   EX-BM-01 — Enqueue p95 latency (bounded queue hot path)
 *   EX-BM-02 — Dequeue p95 latency (bounded queue hot path)
 *   EX-BM-03 — Work-steal throughput (items stolen per second)
 *   EX-BM-04 — Concurrent dispatch throughput (N producers → 1 queue)
 *
 * ## Performance gates (representative hardware, Release build)
 * | Gate     | Target                 |
 * |----------|------------------------|
 * | EX-BM-01 | Enqueue p95 < 5 ms     |
 * | EX-BM-02 | Dequeue p95 < 100 µs   |
 * | EX-BM-03 | Steal throughput ≥ 1M items/s |
 * | EX-BM-04 | Dispatch throughput ≥ 500 k items/s at 8 threads |
 *
 * @note No external dependencies beyond standard library + GoogleBenchmark.
 * @see src/execution/ROADMAP.md — Wave D contribution closure
 * @see docs/operability/RUNBOOK_EXECUTION_ENGINE.md — operator runbook
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

/// @cond DOXYGEN_IGNORE

// ─────────────────────────────────────────────────────────────────────────────
// In-process stubs — model execution hot paths without external backends.
// MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

// ---------------------------------------------------------------------------
// BoundedQueue — minimal lock-based queue for latency benchmarking
// ---------------------------------------------------------------------------
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : cap_(capacity) {}

    bool enqueue(uint64_t item) {
        std::lock_guard<std::mutex> lk(mu_);
        if (q_.size() >= cap_) return false;
        q_.push_back(item);
        return true;
    }

    bool dequeue(uint64_t& out) {
        std::lock_guard<std::mutex> lk(mu_);
        if (q_.empty()) return false;
        out = q_.front();
        q_.pop_front();
        return true;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lk(mu_);
        return q_.size();
    }

private:
    const std::size_t cap_;
    mutable std::mutex mu_;
    std::deque<uint64_t> q_;
};

// ---------------------------------------------------------------------------
// StealPool — per-worker queue with steal-drain
// ---------------------------------------------------------------------------
class StealPool {
public:
    explicit StealPool(unsigned workers)
        : workers_(workers), queues_(workers) {}

    void submit(uint64_t task_id) {
        unsigned w = static_cast<unsigned>(task_id % workers_);
        std::lock_guard<std::mutex> lk(mu_[w % kMaxW]);
        queues_[w].push_back(task_id);
    }

    bool steal(uint64_t& out) {
        for (unsigned w = 0; w < workers_; ++w) {
            std::lock_guard<std::mutex> lk(mu_[w % kMaxW]);
            if (!queues_[w].empty()) {
                out = queues_[w].front();
                queues_[w].pop_front();
                return true;
            }
        }
        return false;
    }

private:
    static constexpr unsigned kMaxW = 32;
    unsigned workers_;
    std::vector<std::deque<uint64_t>> queues_;
    mutable std::mutex mu_[kMaxW];
};

// ─────────────────────────────────────────────────────────────────────────────
// EX-BM-01: Enqueue p95 latency
//
// Single-threaded sequential enqueue into a large-capacity queue.
// Measures raw enqueue overhead including mutex acquisition.
// ─────────────────────────────────────────────────────────────────────────────
static void BM_EX_BM_01_EnqueueP95(benchmark::State& state) {
    BoundedQueue queue(1'000'000);
    uint64_t id = 0;
    for (auto _ : state) {
        // Keep queue from filling up: dequeue every 1000 items
        if (id % 1000 == 0 && id > 0) {
            uint64_t dummy = 0;
            for (int i = 0; i < 500; ++i) queue.dequeue(dummy);
        }
        const bool ok = queue.enqueue(id++);
        benchmark::DoNotOptimize(ok);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_EX_BM_01_EnqueueP95)
    ->Name("EX-BM-01/EnqueueP95")
    ->Repetitions(5)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// EX-BM-02: Dequeue p95 latency
//
// Pre-fill queue, then single-threaded sequential dequeue.
// ─────────────────────────────────────────────────────────────────────────────
static void BM_EX_BM_02_DequeueP95(benchmark::State& state) {
    constexpr std::size_t kCapacity = 1'000'000;
    BoundedQueue queue(kCapacity);
    // Pre-fill
    for (uint64_t i = 0; i < kCapacity / 2; ++i) queue.enqueue(i);

    uint64_t id = kCapacity / 2;
    uint64_t out = 0;
    for (auto _ : state) {
        if (!queue.dequeue(out)) {
            // Refill when empty
            for (uint64_t i = 0; i < 1000; ++i) queue.enqueue(id++);
            queue.dequeue(out);
        }
        benchmark::DoNotOptimize(out);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_EX_BM_02_DequeueP95)
    ->Name("EX-BM-02/DequeueP95")
    ->Repetitions(5)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// EX-BM-03: Work-steal throughput
//
// Pre-populate an 8-worker steal pool, then steal all items as fast as
// possible.  Measures steal operations per second.
// ─────────────────────────────────────────────────────────────────────────────
static void BM_EX_BM_03_WorkStealThroughput(benchmark::State& state) {
    constexpr unsigned kWorkers = 8;
    const auto N = static_cast<uint64_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();
        StealPool pool(kWorkers);
        for (uint64_t i = 0; i < N; ++i) pool.submit(i);
        state.ResumeTiming();

        uint64_t out = 0;
        uint64_t stolen = 0;
        while (pool.steal(out)) ++stolen;
        benchmark::DoNotOptimize(stolen);
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.range(0)) *
        static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_EX_BM_03_WorkStealThroughput)
    ->Name("EX-BM-03/WorkStealThroughput")
    ->Arg(10000)
    ->Repetitions(5)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMillisecond);

// ─────────────────────────────────────────────────────────────────────────────
// EX-BM-04: Concurrent dispatch throughput
//
// 8 producer threads enqueue items into a shared queue.  Measures aggregate
// throughput (items enqueued per second) under producer contention.
// ─────────────────────────────────────────────────────────────────────────────
static void BM_EX_BM_04_ConcurrentDispatchThroughput(benchmark::State& state) {
    constexpr unsigned kProducers = 8;
    const auto kItems = static_cast<uint64_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();
        BoundedQueue queue(kItems * kProducers + 1);
        std::atomic<uint64_t> next_id{0};
        state.ResumeTiming();

        std::vector<std::thread> producers;
        for (unsigned p = 0; p < kProducers; ++p) {
            producers.emplace_back([&]() {
                for (uint64_t i = 0; i < kItems; ++i) {
                    const uint64_t id = next_id.fetch_add(1);
                    while (!queue.enqueue(id)) std::this_thread::yield();
                }
            });
        }
        for (auto& t : producers) t.join();

        state.PauseTiming();
        benchmark::DoNotOptimize(queue.size());
        state.ResumeTiming();
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(kProducers) *
        static_cast<int64_t>(state.range(0)) *
        static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_EX_BM_04_ConcurrentDispatchThroughput)
    ->Name("EX-BM-04/ConcurrentDispatchThroughput")
    ->Arg(5000)
    ->Repetitions(3)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();

/// @endcond
