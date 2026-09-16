// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_chaos_microbenchmarks.cpp
 * @brief Chaos microbenchmarks — inject latency p99, recover latency p99,
 *        concurrent fault throughput.
 *
 * Extends the release-gate coverage in bench_chaos_release_gates.cpp with
 * dedicated microbenchmarks for the three high-priority chaos paths identified
 * in the Wave D mid-term roadmap items:
 *
 *   CHAOS-MB-01  InjectLatencyP99        — injectFault() p99 latency ≤ 5 µs
 *   CHAOS-MB-02  RecoverLatencyP99       — recoverFault() p99 latency ≤ 5 µs
 *   CHAOS-MB-03  ConcurrentFaultThroughput — concurrent inject+recover ≥ 100k ops/s
 *
 * Gate thresholds are emitted as benchmark counters so CI tooling can validate
 * results from JSON output:
 *
 *   bench_chaos_microbenchmarks \
 *     --benchmark_format=json --benchmark_out=chaos_mb.json
 *
 * ## Measurement hygiene
 *   - All registrations use UseRealTime().MinTime(1.0).
 *   - Thresholds emitted as "gate_threshold_*" counters for CI tooling.
 *   - benchmark::DoNotOptimize() applied to all results.
 *   - PauseTiming() / ResumeTiming() used to isolate fixture reset.
 *
 * @version 1.0.0
 * @see src/chaos/ROADMAP.md — Mid-term microbenchmarks (CHAOS-MB-01..03)
 * @see benchmarks/chaos/bench_chaos_release_gates.cpp — GATE-CHS-01..06
 * @see benchmarks/MEASUREMENT_HYGIENE.md — hygiene standards
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

// ─── gate thresholds ─────────────────────────────────────────────────────────

/// CHAOS-MB-01: injectFault() p99 latency ≤ 5 µs (per-call)
static constexpr double kGateMB01_InjectP99_us = 5.0;

/// CHAOS-MB-02: recoverFault() p99 latency ≤ 5 µs (per-call)
static constexpr double kGateMB02_RecoverP99_us = 5.0;

/// CHAOS-MB-03: concurrent inject+recover throughput ≥ 100 000 ops/s
static constexpr double kGateMB03_ConcurrentThroughput_OpsPerSec = 100'000.0;

// ─── helpers ─────────────────────────────────────────────────────────────────

static FaultSpec makeMBSpec(const std::string& node,
                             FaultType type = FaultType::NODE_FAILURE) {
    return FaultSpec(type, node, std::chrono::milliseconds(0), 1.0, "mb");
}

// ─── CHAOS-MB-01: InjectLatencyP99 ───────────────────────────────────────────

/**
 * @brief CHAOS-MB-01 — injectFault() p99 latency microbenchmark.
 *
 * Injects one unique fault per iteration (fresh node ID to avoid rejection
 * due to duplicate active fault).  Measures per-call latency for the fast
 * registry-insertion path.
 *
 * Gate: p99 latency ≤ 5 µs.
 * Counter: "gate_threshold_inject_p99_us"
 */
static void BM_CHAOSМB01_InjectLatencyP99(benchmark::State& state) {
    FaultInjector fi{"chaos-mb01"};
    long long ops = 0;

    for (auto _ : state) {
        state.PauseTiming();
        fi.clearAllFaults();
        const std::string node = "mb01-n" + std::to_string(ops);
        state.ResumeTiming();

        bool ok = fi.injectFault(makeMBSpec(node));
        benchmark::DoNotOptimize(ok);
        ++ops;
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["gate_threshold_inject_p99_us"] = kGateMB01_InjectP99_us;
}
BENCHMARK(BM_CHAOSМB01_InjectLatencyP99)->UseRealTime()->MinTime(1.0);

// ─── CHAOS-MB-02: RecoverLatencyP99 ──────────────────────────────────────────

/**
 * @brief CHAOS-MB-02 — recoverFault() p99 latency microbenchmark.
 *
 * Pre-injects a fault in setup; measures only the recovery call latency.
 * Resets the fault injector at the start of each iteration to ensure
 * a stable pre-condition (fault always active before recover).
 *
 * Gate: p99 latency ≤ 5 µs.
 * Counter: "gate_threshold_recover_p99_us"
 */
static void BM_CHAOSМB02_RecoverLatencyP99(benchmark::State& state) {
    FaultInjector fi{"chaos-mb02"};
    const std::string kNode = "mb02-fixed-node";

    for (auto _ : state) {
        state.PauseTiming();
        fi.clearAllFaults();
        fi.injectFault(makeMBSpec(kNode));
        state.ResumeTiming();

        bool ok = fi.recoverFault(kNode);
        benchmark::DoNotOptimize(ok);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["gate_threshold_recover_p99_us"] = kGateMB02_RecoverP99_us;
}
BENCHMARK(BM_CHAOSМB02_RecoverLatencyP99)->UseRealTime()->MinTime(1.0);

// ─── CHAOS-MB-03: ConcurrentFaultThroughput ──────────────────────────────────

/**
 * @brief CHAOS-MB-03 — concurrent inject+recover throughput (4 threads).
 *
 * Runs 4 threads, each performing inject→recover cycles on a disjoint set
 * of node IDs.  Measures combined throughput across all threads.
 *
 * Gate: ≥ 100 000 ops/s combined.
 * Counter: "gate_threshold_concurrent_throughput_ops_per_sec"
 */
static void BM_CHAOSМB03_ConcurrentFaultThroughput(benchmark::State& state) {
    constexpr int kThreads = 4;

    FaultInjector fi{"chaos-mb03"};
    std::atomic<long long> total_ops{0};

    for (auto _ : state) {
        state.PauseTiming();
        fi.clearAllFaults();
        state.ResumeTiming();

        std::vector<std::thread> workers;
        workers.reserve(kThreads);

        for (int t = 0; t < kThreads; ++t) {
            workers.emplace_back([t, &fi, &total_ops]() {
                const std::string node = "mb03-t" + std::to_string(t) + "-node";
                bool injected = fi.injectFault(makeMBSpec(node));
                benchmark::DoNotOptimize(injected);
                if (injected) {
                    bool recovered = fi.recoverFault(node);
                    benchmark::DoNotOptimize(recovered);
                    total_ops.fetch_add(2, std::memory_order_relaxed);
                }
            });
        }

        for (auto& w : workers) w.join();
    }

    state.SetItemsProcessed(total_ops.load());
    state.counters["gate_threshold_concurrent_throughput_ops_per_sec"] =
        kGateMB03_ConcurrentThroughput_OpsPerSec;
    state.counters["threads"] = static_cast<double>(kThreads);
}
BENCHMARK(BM_CHAOSМB03_ConcurrentFaultThroughput)->UseRealTime()->MinTime(1.0);
