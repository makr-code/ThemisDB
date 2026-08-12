/**
 * @file bench_chaos_release_gates.cpp
 * @brief Release gate benchmarks for the chaos framework (Phase 5 hardening delivery).
 *
 * Validates six hard performance gates for the chaos runtime hot paths:
 *
 *   GATE-CHS-01  injectFault() single-threaded throughput ≥ 500 000 ops/s
 *   GATE-CHS-02  isFaultActive() query latency ≤ 1 µs average per call (positive path)
 *   GATE-CHS-03  recoverFault() single-threaded throughput ≥ 200 000 ops/s
 *   GATE-CHS-04  concurrent inject+query (8 threads) ≥ 200 000 ops/s combined
 *   GATE-CHS-05  ChaosScheduler schedule() throughput ≥ 100 000 entries/s
 *   GATE-CHS-06  event callback dispatch overhead ≤ 2× baseline (10 callbacks vs 0)
 *
 * Gate thresholds are emitted as benchmark counters so CI tooling can validate
 * results from the JSON output:
 *   bench_chaos_release_gates --benchmark_format=json --benchmark_out=gates.json
 *
 * @see src/chaos/ROADMAP.md — Phase 5 item (lock benchmark-backed release gates)
 * @see include/chaos/chaos_contract.h — § 7 process-local blast-radius
 * @see benchmarks/MEASUREMENT_HYGIENE.md — seeded RNG, steady_clock, real-time mode
 */

#include <benchmark/benchmark.h>

#include "chaos/chaos_framework.h"
#include "chaos/chaos_contract.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace themis::chaos;
using namespace std::chrono_literals;

// ─── gate thresholds (emitted as counters for CI tooling) ────────────────────

static constexpr double kGateCHS01_InjectOpsPerSec    = 500'000.0;
static constexpr double kGateCHS03_RecoverOpsPerSec   = 200'000.0;
static constexpr double kGateCHS04_ConcurrentOpsPerSec = 200'000.0;
static constexpr double kGateCHS05_ScheduleOpsPerSec  = 100'000.0;

// ─── helpers ─────────────────────────────────────────────────────────────────

static FaultSpec makeGateSpec(const std::string& node,
                               FaultType type = FaultType::NODE_FAILURE) {
    return FaultSpec(type, node, std::chrono::milliseconds(0), 1.0, "gate");
}

// ─── GATE-CHS-01: injectFault() single-threaded throughput ───────────────────

/**
 * @brief GATE-CHS-01 — injectFault() throughput (single-threaded).
 *
 * Injects one fault per unique node per iteration.  Measures pure registry
 * insertion throughput on the fast path (no expiry, no callbacks).
 *
 * Gate: ≥ 500 000 ops/s.  Emitted as counter "gate_threshold_ops_per_sec".
 */
static void BM_GATE_CHS01_InjectThroughput(benchmark::State& state) {
    FaultInjector fi{"gate-chs01"};
    long long ops = 0;

    for (auto _ : state) {
        state.PauseTiming();
        fi.clearAllFaults();
        state.ResumeTiming();

        const std::string node = "gate-chs01-n" + std::to_string(ops);
        bool ok = fi.injectFault(makeGateSpec(node));
        benchmark::DoNotOptimize(ok);
        ++ops;
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["gate_threshold_ops_per_sec"] = kGateCHS01_InjectOpsPerSec;
}
BENCHMARK(BM_GATE_CHS01_InjectThroughput)->UseRealTime()->MinTime(1.0);

// ─── GATE-CHS-02: isFaultActive() query latency ──────────────────────────────

/**
 * @brief GATE-CHS-02 — isFaultActive() positive-path latency.
 *
 * Queries a single permanently-active fault.  Measures the per-call cost of
 * the mutex-protected hash-map lookup on the hot path.
 *
 * Gate: average latency ≤ 1 µs.  Gate threshold emitted as counter.
 */
static void BM_GATE_CHS02_QueryLatency(benchmark::State& state) {
    FaultInjector fi{"gate-chs02"};
    fi.injectFault(makeGateSpec("gate-chs02-target"));

    for (auto _ : state) {
        bool active = fi.isFaultActive("gate-chs02-target");
        benchmark::DoNotOptimize(active);
    }

    state.SetItemsProcessed(state.iterations());
    // Emit gate threshold in ns so CI can compare against reported time/op.
    state.counters["gate_max_ns_per_op"] = 1000.0;  // 1 µs = 1000 ns
}
BENCHMARK(BM_GATE_CHS02_QueryLatency)->UseRealTime()->MinTime(1.0);

// ─── GATE-CHS-03: recoverFault() throughput ──────────────────────────────────

/**
 * @brief GATE-CHS-03 — recoverFault() single-threaded throughput.
 *
 * Injects a fault then immediately recovers it; measures the inject+recover
 * round-trip cost as a proxy for recover throughput.
 *
 * Gate: ≥ 200 000 recover ops/s.
 */
static void BM_GATE_CHS03_RecoverThroughput(benchmark::State& state) {
    FaultInjector fi{"gate-chs03"};
    const std::string node = "gate-chs03-n";

    for (auto _ : state) {
        state.PauseTiming();
        fi.injectFault(makeGateSpec(node));
        state.ResumeTiming();

        bool ok = fi.recoverFault(node);
        benchmark::DoNotOptimize(ok);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["gate_threshold_ops_per_sec"] = kGateCHS03_RecoverOpsPerSec;
}
BENCHMARK(BM_GATE_CHS03_RecoverThroughput)->UseRealTime()->MinTime(1.0);

// ─── GATE-CHS-04: concurrent inject+query throughput ─────────────────────────

/**
 * @brief GATE-CHS-04 — Concurrent inject + query throughput (8 threads).
 *
 * Four inject threads and four query threads run concurrently against the same
 * FaultInjector.  Measures combined ops/s.
 *
 * Gate: ≥ 200 000 combined ops/s.
 */
static void BM_GATE_CHS04_ConcurrentInjectQuery(benchmark::State& state) {
    FaultInjector fi{"gate-chs04"};
    constexpr int kThreads = 4;
    std::atomic<long long> total_ops{0};

    for (auto _ : state) {
        state.PauseTiming();
        fi.clearAllFaults();
        std::atomic<bool> go{false};
        std::vector<std::thread> workers;
        workers.reserve(kThreads * 2);
        state.ResumeTiming();

        // Inject threads.
        for (int t = 0; t < kThreads; ++t) {
            workers.emplace_back([&fi, &go, &total_ops, t]() {
                while (!go.load(std::memory_order_acquire)) {}
                for (int i = 0; i < 64; ++i) {
                    const std::string n =
                        "g04-t" + std::to_string(t) + "-" + std::to_string(i);
                    fi.injectFault(makeGateSpec(n));
                    total_ops.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        // Query threads.
        for (int t = 0; t < kThreads; ++t) {
            workers.emplace_back([&fi, &go, &total_ops, t]() {
                while (!go.load(std::memory_order_acquire)) {}
                for (int i = 0; i < 64; ++i) {
                    const std::string n =
                        "g04-t" + std::to_string(t) + "-" + std::to_string(i);
                    bool a = fi.isFaultActive(n);
                    benchmark::DoNotOptimize(a);
                    total_ops.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }

        go.store(true, std::memory_order_release);
        for (auto& w : workers) {
            w.join();
        }
    }

    state.SetItemsProcessed(total_ops.load());
    state.counters["gate_threshold_ops_per_sec"] = kGateCHS04_ConcurrentOpsPerSec;
}
BENCHMARK(BM_GATE_CHS04_ConcurrentInjectQuery)->UseRealTime()->Iterations(8);

// ─── GATE-CHS-05: ChaosScheduler schedule() throughput ───────────────────────

/**
 * @brief GATE-CHS-05 — ChaosScheduler schedule() throughput.
 *
 * Enqueues entries with a far-future trigger_at while the scheduler is stopped
 * to measure pure queue-insertion throughput without firing overhead.
 *
 * Gate: ≥ 100 000 schedule() ops/s.
 */
static void BM_GATE_CHS05_ScheduleThroughput(benchmark::State& state) {
    auto fi = std::make_shared<FaultInjector>("gate-chs05");
    ChaosSchedulerConfig cfg;
    cfg.tick_interval = 10ms;
    ChaosScheduler sched{fi, cfg};
    // Keep scheduler stopped to measure pure queue insert.

    long long ops = 0;
    for (auto _ : state) {
        state.PauseTiming();
        sched.clearPending();
        state.ResumeTiming();

        ChaosScheduleEntry entry{
            std::chrono::steady_clock::now() + std::chrono::hours(1),
            makeGateSpec("gate-chs05-n" + std::to_string(ops))
        };
        sched.schedule(entry);
        ++ops;
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["gate_threshold_ops_per_sec"] = kGateCHS05_ScheduleOpsPerSec;
}
BENCHMARK(BM_GATE_CHS05_ScheduleThroughput)->UseRealTime()->MinTime(1.0);

// ─── GATE-CHS-06: callback dispatch overhead ─────────────────────────────────

/**
 * @brief GATE-CHS-06 — Event callback dispatch overhead.
 *
 * Compares injectFault() throughput with 0 callbacks (baseline) vs. 10
 * callbacks registered.  Gate: ratio ≤ 2× (measured externally from JSON output).
 *
 * Parameterised via state.range(0) = number of callbacks.
 */
static void BM_GATE_CHS06_CallbackOverhead(benchmark::State& state) {
    const int kCallbacks = static_cast<int>(state.range(0));
    FaultInjector fi{"gate-chs06"};
    std::atomic<long long> cb_total{0};

    for (int c = 0; c < kCallbacks; ++c) {
        fi.registerEventCallback(
            [&cb_total](const FaultSpec& /*spec*/, bool /*injected*/) {
                cb_total.fetch_add(1, std::memory_order_relaxed);
            });
    }

    long long ops = 0;
    for (auto _ : state) {
        state.PauseTiming();
        fi.clearAllFaults();
        state.ResumeTiming();

        const std::string node = "gate-chs06-n" + std::to_string(ops++);
        bool ok = fi.injectFault(makeGateSpec(node));
        benchmark::DoNotOptimize(ok);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["callbacks"]  = static_cast<double>(kCallbacks);
    state.counters["cb_total"]   = static_cast<double>(cb_total.load());
    // Gate: overhead with 10 callbacks must be ≤ 2× the 0-callback baseline.
    // Evaluation is done externally by comparing items_per_second for Arg(0) vs Arg(10).
    state.counters["gate_max_overhead_ratio"] = 2.0;
}
// Register: 0-callback baseline and 10-callback loaded variant.
BENCHMARK(BM_GATE_CHS06_CallbackOverhead)
    ->Arg(0)
    ->Arg(10)
    ->UseRealTime()
    ->MinTime(1.0);
