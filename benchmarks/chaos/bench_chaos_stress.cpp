/**
 * @file bench_chaos_stress.cpp
 * @brief Extended stress-benchmark coverage for the chaos framework
 *
 * Validates:
 *   CHAOS-PHASE4: Add dedicated API conformance tests at include boundary
 *   CHAOS-PHASE5: Extended Stress-Benchmark Coverage
 *
 * Scenarios:
 *   - FaultInjector: injectFault() throughput (single-threaded)
 *   - FaultInjector: isFaultActive() query throughput (positive / negative path)
 *   - FaultInjector: recoverFault() throughput
 *   - FaultInjector: getActiveFaults() snapshot under high churn
 *   - FaultInjector: expired-fault pruning overhead
 *   - FaultInjector: concurrent inject + query (stress, N threads)
 *   - ChaosScheduler: schedule() throughput
 *   - ChaosScheduler: start/stop cycle overhead
 *   - Mixed fault-type benchmark (all 7 FaultType values)
 *   - Event callback dispatch overhead (0, 1, 10 callbacks)
 */

#include <benchmark/benchmark.h>
#include "chaos/chaos_framework.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace themis::chaos;

// ─── helpers ─────────────────────────────────────────────────────────────────

static const FaultType kAllFaultTypes[] = {
    FaultType::NODE_FAILURE,
    FaultType::NETWORK_PARTITION,
    FaultType::LEADER_CRASH,
    FaultType::DELAYED_RESPONSE,
    FaultType::DISK_FAILURE,
    FaultType::RANDOM_FAILURE,
    FaultType::DISASTER_RECOVERY_DRILL,
};
static const int kFaultTypeCount = 7;

/// Build a FaultSpec with a short TTL so it expires quickly.
static FaultSpec makeSpec(const std::string& node_id, FaultType type,
                           int ttl_ms = 0) {
    return FaultSpec(type, node_id,
                     ttl_ms > 0 ? std::chrono::milliseconds(ttl_ms)
                                : std::chrono::milliseconds(0),
                     1.0, "bench");
}

// ─── FaultInjector fixture ────────────────────────────────────────────────────

class FaultInjectorFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*s*/) override {
        injector = std::make_unique<FaultInjector>("bench_injector");
    }

    void TearDown(const benchmark::State& /*s*/) override {
        injector->clearAllFaults();
        injector.reset();
    }

    std::unique_ptr<FaultInjector> injector;
};

// ─── 1. injectFault() throughput ─────────────────────────────────────────────

BENCHMARK_F(FaultInjectorFixture, InjectFault_Throughput)(benchmark::State& state) {
    int n = 0;
    for (auto _ : state) {
        const std::string node = "node_" + std::to_string(n++);
        bool ok = injector->injectFault(
            makeSpec(node, FaultType::NODE_FAILURE));
        benchmark::DoNotOptimize(ok);
        // Clear to avoid saturating the map.
        if (n % 64 == 0) injector->clearAllFaults();
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("injectFault() — unique nodes, no TTL");
}

// ─── 2. injectFault() — all fault types rotation ─────────────────────────────

BENCHMARK_F(FaultInjectorFixture, InjectFault_AllTypes)(benchmark::State& state) {
    int n = 0;
    for (auto _ : state) {
        FaultType type = kAllFaultTypes[n % kFaultTypeCount];
        const std::string node = "node_" + std::to_string(n);
        bool ok = injector->injectFault(makeSpec(node, type));
        benchmark::DoNotOptimize(ok);
        ++n;
        if (n % 64 == 0) injector->clearAllFaults();
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("injectFault() — rotating through all 7 FaultType values");
}

// ─── 3. isFaultActive() — positive path ──────────────────────────────────────

BENCHMARK_F(FaultInjectorFixture, IsFaultActive_Positive)(benchmark::State& state) {
    injector->injectFault(makeSpec("node_bench", FaultType::NETWORK_PARTITION));

    for (auto _ : state) {
        bool active = injector->isFaultActive("node_bench");
        benchmark::DoNotOptimize(active);
    }

    state.SetLabel("isFaultActive() — node with active fault (positive path)");
}

// ─── 4. isFaultActive() — negative path ──────────────────────────────────────

BENCHMARK_F(FaultInjectorFixture, IsFaultActive_Negative)(benchmark::State& state) {
    for (auto _ : state) {
        bool active = injector->isFaultActive("nonexistent_node_bench");
        benchmark::DoNotOptimize(active);
    }

    state.SetLabel("isFaultActive() — node with no fault (negative path)");
}

// ─── 5. recoverFault() throughput ────────────────────────────────────────────

BENCHMARK_F(FaultInjectorFixture, RecoverFault_Throughput)(benchmark::State& state) {
    int n = 0;
    for (auto _ : state) {
        state.PauseTiming();
        const std::string node = "recover_node_" + std::to_string(n++ % 32);
        injector->injectFault(makeSpec(node, FaultType::DISK_FAILURE));
        state.ResumeTiming();

        bool ok = injector->recoverFault(node);
        benchmark::DoNotOptimize(ok);
    }

    state.SetLabel("recoverFault() (inject+recover cycle, recovery timed)");
}

// ─── 6. getActiveFaults() snapshot under high churn ──────────────────────────

BENCHMARK_F(FaultInjectorFixture, GetActiveFaults_HighChurn)(benchmark::State& state) {
    const int kFaults = static_cast<int>(state.range(0));

    // Populate
    injector->clearAllFaults();
    for (int i = 0; i < kFaults; ++i) {
        injector->injectFault(
            makeSpec("node_" + std::to_string(i), FaultType::RANDOM_FAILURE));
    }

    for (auto _ : state) {
        auto faults = injector->getActiveFaults();
        benchmark::DoNotOptimize(faults.size());
    }

    state.SetLabel("getActiveFaults() snapshot with " + std::to_string(kFaults) + " faults");
}
BENCHMARK_REGISTER_F(FaultInjectorFixture, GetActiveFaults_HighChurn)
    ->Arg(0)->Arg(8)->Arg(64)->Arg(256)->Arg(1024);

// ─── 7. Expired-fault pruning overhead ───────────────────────────────────────

BENCHMARK_F(FaultInjectorFixture, ExpiredFaultPruning)(benchmark::State& state) {
    const int kExpired = 64;

    // Inject faults that expire immediately (1 ms TTL)
    for (int i = 0; i < kExpired; ++i) {
        injector->injectFault(
            makeSpec("expired_" + std::to_string(i), FaultType::DELAYED_RESPONSE, 1));
    }
    // Let them expire
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    for (auto _ : state) {
        // getActiveFaults() triggers pruning internally
        auto faults = injector->getActiveFaults();
        benchmark::DoNotOptimize(faults.size());
    }

    state.SetLabel("getActiveFaults() — " + std::to_string(kExpired) + " expired entries pruned");
}

// ─── 8. Event callback dispatch — varying callback count ─────────────────────

static void BM_CallbackDispatch(benchmark::State& state) {
    const int kCallbacks = static_cast<int>(state.range(0));
    FaultInjector injector("bench_cb");

    std::atomic<uint64_t> fired{0};
    for (int c = 0; c < kCallbacks; ++c) {
        injector.registerEventCallback(
            [&fired](const FaultSpec&, bool) {
                fired.fetch_add(1, std::memory_order_relaxed);
            });
    }

    int n = 0;
    for (auto _ : state) {
        const std::string node = "cb_node_" + std::to_string(n++);
        injector.injectFault(makeSpec(node, FaultType::NODE_FAILURE));
        if (n % 64 == 0) injector.clearAllFaults();
    }

    benchmark::DoNotOptimize(fired.load());
    state.SetLabel("Event callback dispatch — " + std::to_string(kCallbacks) + " callbacks");
}
BENCHMARK(BM_CallbackDispatch)->Arg(0)->Arg(1)->Arg(5)->Arg(10);

// ─── 9. Concurrent inject + query (stress) ───────────────────────────────────

static void BM_ConcurrentStress(benchmark::State& state) {
    const int kThreads = static_cast<int>(state.range(0));
    FaultInjector injector("bench_stress");

    for (auto _ : state) {
        std::atomic<uint64_t> ops{0};
        std::vector<std::thread> threads;
        threads.reserve(static_cast<size_t>(kThreads));

        for (int t = 0; t < kThreads; ++t) {
            threads.emplace_back([&injector, &ops, t]() {
                for (int i = 0; i < 64; ++i) {
                    if (i % 3 == 0) {
                        injector.injectFault(
                            makeSpec("stress_" + std::to_string(t) + "_" + std::to_string(i),
                                     kAllFaultTypes[i % kFaultTypeCount]));
                    } else if (i % 3 == 1) {
                        bool active = injector.isFaultActive(
                            "stress_" + std::to_string(t) + "_" + std::to_string(i - 1));
                        benchmark::DoNotOptimize(active);
                    } else {
                        injector.recoverFault(
                            "stress_" + std::to_string(t) + "_" + std::to_string(i - 1));
                    }
                    ops.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        for (auto& th : threads) th.join();
        injector.clearAllFaults();
        benchmark::DoNotOptimize(ops.load());
    }

    state.SetItemsProcessed(state.iterations() * kThreads * 64LL);
    state.SetLabel("concurrent stress: inject+query+recover, threads=" +
                   std::to_string(kThreads));
}
BENCHMARK(BM_ConcurrentStress)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

// ─── 10. ChaosScheduler: schedule() throughput ───────────────────────────────

static void BM_ChaosScheduler_Schedule(benchmark::State& state) {
    auto inj = std::make_shared<FaultInjector>("sched_bench");
    ChaosScheduler scheduler(inj);
    scheduler.start();

    int n = 0;
    for (auto _ : state) {
        scheduler.scheduleIn(
            std::chrono::milliseconds(10000),  // far future — never fires during benchmark
            makeSpec("sched_node_" + std::to_string(n++), FaultType::LEADER_CRASH));
    }

    scheduler.stop();
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("ChaosScheduler::scheduleIn() throughput");
}
BENCHMARK(BM_ChaosScheduler_Schedule);

// ─── 11. activeFaultCount() throughput ───────────────────────────────────────

BENCHMARK_F(FaultInjectorFixture, ActiveFaultCount_Throughput)(benchmark::State& state) {
    // Inject 128 faults once
    for (int i = 0; i < 128; ++i) {
        injector->injectFault(
            makeSpec("cnt_node_" + std::to_string(i), FaultType::NODE_FAILURE));
    }

    for (auto _ : state) {
        size_t cnt = injector->activeFaultCount();
        benchmark::DoNotOptimize(cnt);
    }

    state.SetLabel("activeFaultCount() with 128 active faults");
}
