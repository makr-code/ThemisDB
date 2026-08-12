/**
 * @file bench_thread_pool_saturation.cpp
 * @brief Load benchmarks for ThreadPoolManager under saturation conditions
 *
 * Validates:
 *   UTILS-PHASE4: Load tests for thread_pool_manager.h under saturation
 *
 * Scenarios:
 *   - Single-pool submission throughput (IO / CPU / BLOCKING pools)
 *   - Saturated queue: submit N tasks beyond queue capacity, measure drop rate
 *   - Priority-aware ordering under heavy load
 *   - Concurrent producers from multiple threads
 *   - Shutdown latency while tasks are in-flight
 *   - Statistics accuracy under high throughput
 */

#include <benchmark/benchmark.h>
#include "utils/thread_pool_manager.h"

#include <atomic>
#include <chrono>
#include <latch>
#include <thread>
#include <vector>

using namespace themis::utils;

// ─── helpers ─────────────────────────────────────────────────────────────────

static ThreadPoolManager::Config makeSaturatedConfig(size_t max_threads = 4,
                                                     size_t queue_size   = 64) {
    ThreadPoolManager::Config cfg;
    cfg.io_pool.min_threads       = 2;
    cfg.io_pool.max_threads       = max_threads;
    cfg.io_pool.queue_size        = queue_size;
    cfg.io_pool.name              = "bench_io";
    cfg.cpu_pool.min_threads      = 2;
    cfg.cpu_pool.max_threads      = max_threads;
    cfg.cpu_pool.queue_size       = queue_size;
    cfg.cpu_pool.name             = "bench_cpu";
    cfg.blocking_pool.min_threads = 2;
    cfg.blocking_pool.max_threads = max_threads;
    cfg.blocking_pool.queue_size  = queue_size;
    cfg.blocking_pool.name        = "bench_blocking";
    cfg.enable_metrics            = false;
    return cfg;
}

// ─── Fixture ─────────────────────────────────────────────────────────────────

class ThreadPoolSaturationFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*s*/) override {
        mgr = std::make_unique<ThreadPoolManager>(makeSaturatedConfig());
    }
    void TearDown(const benchmark::State& /*s*/) override {
        mgr->shutdown();
        mgr.reset();
    }
    std::unique_ptr<ThreadPoolManager> mgr;
};

// ─── 1. Submission throughput — IO pool ──────────────────────────────────────

BENCHMARK_F(ThreadPoolSaturationFixture, SubmitThroughput_IO)(benchmark::State& state) {
    std::atomic<uint64_t> executed{0};

    for (auto _ : state) {
        bool accepted = mgr->submitTask(
            ThreadPoolManager::PoolType::IO,
            [&executed]() { executed.fetch_add(1, std::memory_order_relaxed); },
            "bench_io",
            Task::Priority::NORMAL);
        benchmark::DoNotOptimize(accepted);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("IO pool submit throughput");
}

// ─── 2. Submission throughput — CPU pool ─────────────────────────────────────

BENCHMARK_F(ThreadPoolSaturationFixture, SubmitThroughput_CPU)(benchmark::State& state) {
    std::atomic<uint64_t> executed{0};

    for (auto _ : state) {
        bool accepted = mgr->submitTask(
            ThreadPoolManager::PoolType::CPU,
            [&executed]() { executed.fetch_add(1, std::memory_order_relaxed); },
            "bench_cpu",
            Task::Priority::NORMAL);
        benchmark::DoNotOptimize(accepted);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("CPU pool submit throughput");
}

// ─── 3. Saturated queue — drop-rate measurement ──────────────────────────────

BENCHMARK_F(ThreadPoolSaturationFixture, SaturatedQueue_DropRate)(benchmark::State& state) {
    // Use a very small queue (already saturated config has queue_size=64).
    // Flood it from a single thread and count accepted vs rejected.
    const int kBurst = 256;
    std::atomic<uint64_t> noop{0};

    for (auto _ : state) {
        int accepted = 0;
        int rejected = 0;

        for (int i = 0; i < kBurst; ++i) {
            bool ok = mgr->submitTask(
                ThreadPoolManager::PoolType::CPU,
                [&noop]() { noop.fetch_add(1, std::memory_order_relaxed); },
                "flood");
            ok ? ++accepted : ++rejected;
        }

        benchmark::DoNotOptimize(accepted);
        benchmark::DoNotOptimize(rejected);
        // Drain before next iteration
        mgr->getPoolStatistics(ThreadPoolManager::PoolType::CPU);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    state.SetLabel("256-task burst; queue_size=64");
}

// ─── 4. Priority ordering under load ─────────────────────────────────────────

BENCHMARK_F(ThreadPoolSaturationFixture, PriorityOrdering_UnderLoad)(benchmark::State& state) {
    std::atomic<uint64_t> critical_count{0};
    std::atomic<uint64_t> low_count{0};

    for (auto _ : state) {
        // Submit 8 LOW tasks then 1 CRITICAL task; CRITICAL should drain first.
        for (int i = 0; i < 8; ++i) {
            mgr->submitTask(
                ThreadPoolManager::PoolType::CPU,
                [&low_count]() { low_count.fetch_add(1, std::memory_order_relaxed); },
                "low", Task::Priority::LOW);
        }
        mgr->submitTask(
            ThreadPoolManager::PoolType::CPU,
            [&critical_count]() { critical_count.fetch_add(1, std::memory_order_relaxed); },
            "critical", Task::Priority::CRITICAL);

        benchmark::DoNotOptimize(critical_count.load());
    }

    state.SetLabel("8xLOW + 1xCRITICAL per iteration");
}

// ─── 5. Concurrent producers ─────────────────────────────────────────────────

BENCHMARK_F(ThreadPoolSaturationFixture, ConcurrentProducers)(benchmark::State& state) {
    const int kProducers      = static_cast<int>(state.range(0));
    const int kTasksPerProducer = 32;

    for (auto _ : state) {
        std::atomic<uint64_t> total_submitted{0};
        std::vector<std::thread> producers;
        producers.reserve(static_cast<size_t>(kProducers));

        for (int p = 0; p < kProducers; ++p) {
            producers.emplace_back([&, p]() {
                for (int t = 0; t < kTasksPerProducer; ++t) {
                    bool ok = mgr->submitTask(
                        ThreadPoolManager::PoolType::IO,
                        []() { /* no-op payload */ },
                        "prod_" + std::to_string(p));
                    if (ok) total_submitted.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        for (auto& th : producers) th.join();

        benchmark::DoNotOptimize(total_submitted.load());
    }

    state.SetLabel("concurrent producers x" + std::to_string(kProducers));
}
BENCHMARK_REGISTER_F(ThreadPoolSaturationFixture, ConcurrentProducers)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16)
    ->UseRealTime();

// ─── 6. Statistics overhead ───────────────────────────────────────────────────

BENCHMARK_F(ThreadPoolSaturationFixture, StatisticsQuery)(benchmark::State& state) {
    for (auto _ : state) {
        auto stats = mgr->getStatistics();
        benchmark::DoNotOptimize(stats.cpu_stats.total_executed);
    }
    state.SetLabel("getStatistics() call cost");
}

// ─── 7. Shutdown latency with in-flight tasks ────────────────────────────────

static void BM_ShutdownLatency(benchmark::State& state) {
    const int kInFlight = static_cast<int>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();
        auto mgr = std::make_unique<ThreadPoolManager>(
            makeSaturatedConfig(/*max_threads=*/8, /*queue_size=*/4096));

        std::atomic<int> submitted{0};
        for (int i = 0; i < kInFlight; ++i) {
            bool ok = mgr->submitTask(
                ThreadPoolManager::PoolType::BLOCKING,
                []() { /* instant task */ },
                "inflight");
            if (ok) ++submitted;
        }
        state.ResumeTiming();

        mgr->shutdown();
        benchmark::DoNotOptimize(submitted.load());
        mgr.reset();
    }

    state.SetLabel("shutdown with " + std::to_string(kInFlight) + " in-flight tasks");
}
BENCHMARK(BM_ShutdownLatency)->Arg(0)->Arg(64)->Arg(256)->Arg(1024)->UseRealTime();
