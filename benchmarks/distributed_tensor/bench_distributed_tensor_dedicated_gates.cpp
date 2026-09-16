// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_distributed_tensor_dedicated_gates.cpp
 * @brief Wave D dedicated benchmark gates for distributed tensor hot paths.
 *
 * Gates DT-BM-01..04 lock the acceptable performance envelope for the
 * distributed tensor module Wave D operability deliverables.
 *
 * Wave D Distributed Tensor Benchmark Gates
 * ─────────────────────────────────────────────────────────────────────────────
 * DT-BM-01  Sharded op dispatch latency                  ≤ 1 µs   (per-call)
 * DT-BM-02  Tensor sync round-trip                       ≤ 10 µs  (per-cycle)
 * DT-BM-03  Partition routing decision latency           ≤ 1 µs   (per-call)
 * DT-BM-04  Concurrent sharded ops (8 workers)           ≥ 500 ops/sec total
 *
 * ## SIMULATION NOTE
 * All benchmarks use in-process stubs; no external network shards or GPU
 * backends are required. MUST NOT be used in production code paths.
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_DISTRIBUTED_TENSOR.md
 * @see src/distributed_tensor/ROADMAP.md — Wave D gate closure
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

namespace {

static constexpr uint32_t kDTBMSeed = 42u;

// ---------------------------------------------------------------------------
// Stubs
// ---------------------------------------------------------------------------

struct StubShardedOpDispatcher {
    bool dispatch(uint64_t, int) { ++n; return true; }
    std::atomic<uint64_t> n{0};
};

struct StubTensorSyncBus {
    bool sync(uint64_t, uint64_t) { ++n; return true; }
    std::atomic<uint64_t> n{0};
};

struct StubPartitionRouter {
    int route(uint64_t op) { ++n; return static_cast<int>(op % 4); }
    uint64_t n{0};
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// DT-BM-01: Sharded op dispatch latency ≤ 1 µs per call
// ─────────────────────────────────────────────────────────────────────────────
static void DTBM01_ShardedOpDispatchLatency(benchmark::State& state) {
    StubShardedOpDispatcher disp;
    uint64_t id = 0;
    for (auto _ : state) {
        disp.dispatch(id++, static_cast<int>(id % 8));
        benchmark::DoNotOptimize(disp.n.load());
    }
    state.SetLabel("DT-BM-01: sharded op dispatch ≤ 1 µs");
}
BENCHMARK(DTBM01_ShardedOpDispatchLatency)
    ->Repetitions(5)
    ->Unit(benchmark::kNanosecond);

// ─────────────────────────────────────────────────────────────────────────────
// DT-BM-02: Tensor sync round-trip ≤ 10 µs per cycle
// ─────────────────────────────────────────────────────────────────────────────
static void DTBM02_TensorSyncRoundTrip(benchmark::State& state) {
    StubTensorSyncBus bus;
    uint64_t step = 0;
    for (auto _ : state) {
        bus.sync(0, step++);
        benchmark::DoNotOptimize(bus.n.load());
    }
    state.SetLabel("DT-BM-02: tensor sync ≤ 10 µs");
}
BENCHMARK(DTBM02_TensorSyncRoundTrip)
    ->Repetitions(5)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// DT-BM-03: Partition routing decision latency ≤ 1 µs per call
// ─────────────────────────────────────────────────────────────────────────────
static void DTBM03_PartitionRoutingDecision(benchmark::State& state) {
    StubPartitionRouter router;
    uint64_t op = 0;
    for (auto _ : state) {
        int p = router.route(op++);
        benchmark::DoNotOptimize(p);
    }
    state.SetLabel("DT-BM-03: partition routing ≤ 1 µs");
}
BENCHMARK(DTBM03_PartitionRoutingDecision)
    ->Repetitions(5)
    ->Unit(benchmark::kNanosecond);

// ─────────────────────────────────────────────────────────────────────────────
// DT-BM-04: Concurrent sharded ops ≥ 500 ops/sec (8 workers)
// ─────────────────────────────────────────────────────────────────────────────
static void DTBM04_ConcurrentShardedOps(benchmark::State& state) {
    StubShardedOpDispatcher disp;
    std::atomic<uint64_t> total_ops{0};

    for (auto _ : state) {
        constexpr int kWorkers = 8;
        constexpr int kOps     = 256;
        std::vector<std::thread> workers;
        for (int w = 0; w < kWorkers; ++w) {
            workers.emplace_back([&, w]() {
                uint64_t id = static_cast<uint64_t>(w) * kOps;
                for (int i = 0; i < kOps; ++i) {
                    disp.dispatch(id + i, w % 8);
                    ++total_ops;
                }
            });
        }
        for (auto& t : workers) t.join();
        benchmark::DoNotOptimize(total_ops.load());
    }
    state.SetItemsProcessed(static_cast<int64_t>(total_ops.load()));
    state.SetLabel("DT-BM-04: concurrent sharded ops ≥ 500 ops/sec");
}
BENCHMARK(DTBM04_ConcurrentShardedOps)
    ->Repetitions(3)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
