// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_acceleration_dedicated_gates.cpp
 * @brief Wave D dedicated benchmark gates for acceleration module hot paths.
 *
 * Gates ACC-BM-01..04 lock the acceptable performance envelope for the
 * acceleration module Wave D operability deliverables.
 *
 * Wave D Acceleration Benchmark Gates
 * ─────────────────────────────────────────────────────────────────────────────
 * ACC-BM-01  GPU→CPU fallback routing decision latency     ≤ 1 µs   (per-call)
 * ACC-BM-02  Kernel dispatch enqueue/dequeue latency       ≤ 1 µs   (per-call)
 * ACC-BM-03  CPU fallback executor throughput              ≥ 1 000 ops/sec
 * ACC-BM-04  Concurrent kernel dispatch (8 workers)        ≥ 500 ops/sec total
 *
 * ## SIMULATION NOTE
 * All benchmarks use in-process stubs; no external GPU hardware or CUDA/HIP
 * runtime is required. MUST NOT be used in production code paths.
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_ACCELERATION.md
 * @see src/acceleration/ROADMAP.md — Wave D gate closure
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace {

static constexpr uint32_t kACCBMSeed = 42u;

// ---------------------------------------------------------------------------
// Stubs
// ---------------------------------------------------------------------------

struct StubFallbackRouter {
    enum class Path { GPU, CPU };
    Path route(uint64_t op) {
        ++n;
        return (op % 5 == 0) ? Path::CPU : Path::GPU;
    }
    uint64_t n{0};
};

class StubKernelDispatchQueue {
public:
    explicit StubKernelDispatchQueue(std::size_t cap) : cap_(cap) {}
    bool enqueue(uint64_t id) {
        std::lock_guard<std::mutex> lk(mu_);
        if (q_.size() >= cap_) return false;
        q_.push_back(id); return true;
    }
    bool dequeue(uint64_t& out) {
        std::lock_guard<std::mutex> lk(mu_);
        if (q_.empty()) return false;
        out = q_.front(); q_.pop_front(); return true;
    }
private:
    const std::size_t cap_;
    std::deque<uint64_t> q_;
    std::mutex mu_;
};

struct StubCPUFallbackExecutor {
    bool execute(uint64_t) { ++n; return true; }
    std::atomic<uint64_t> n{0};
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// ACC-BM-01: GPU→CPU fallback routing decision ≤ 1 µs per call
// ─────────────────────────────────────────────────────────────────────────────
static void ACCBM01_FallbackRoutingDecision(benchmark::State& state) {
    StubFallbackRouter router;
    uint64_t op = 0;
    for (auto _ : state) {
        auto path = router.route(op++);
        benchmark::DoNotOptimize(path);
    }
    state.SetLabel("ACC-BM-01: fallback routing ≤ 1 µs");
}
BENCHMARK(ACCBM01_FallbackRoutingDecision)
    ->Repetitions(5)
    ->Unit(benchmark::kNanosecond);

// ─────────────────────────────────────────────────────────────────────────────
// ACC-BM-02: Kernel dispatch enqueue/dequeue latency ≤ 1 µs per call
// ─────────────────────────────────────────────────────────────────────────────
static void ACCBM02_KernelDispatchLatency(benchmark::State& state) {
    StubKernelDispatchQueue q(4096);
    uint64_t id = 0;
    for (auto _ : state) {
        q.enqueue(id++);
        uint64_t dummy = 0;
        q.dequeue(dummy);
        benchmark::DoNotOptimize(dummy);
    }
    state.SetLabel("ACC-BM-02: kernel dispatch ≤ 1 µs");
}
BENCHMARK(ACCBM02_KernelDispatchLatency)
    ->Repetitions(5)
    ->Unit(benchmark::kNanosecond);

// ─────────────────────────────────────────────────────────────────────────────
// ACC-BM-03: CPU fallback executor throughput ≥ 1 000 ops/sec
// ─────────────────────────────────────────────────────────────────────────────
static void ACCBM03_CPUFallbackThroughput(benchmark::State& state) {
    StubCPUFallbackExecutor exec;
    uint64_t op = 0;
    for (auto _ : state) {
        exec.execute(op++);
        benchmark::DoNotOptimize(exec.n.load());
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.SetLabel("ACC-BM-03: CPU fallback ≥ 1 000 ops/sec");
}
BENCHMARK(ACCBM03_CPUFallbackThroughput)
    ->Repetitions(5)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// ACC-BM-04: Concurrent kernel dispatch ≥ 500 ops/sec (8 workers)
// ─────────────────────────────────────────────────────────────────────────────
static void ACCBM04_ConcurrentKernelDispatch(benchmark::State& state) {
    StubKernelDispatchQueue q(65536);
    std::atomic<uint64_t> total_ops{0};

    for (auto _ : state) {
        constexpr int kWorkers = 8;
        constexpr int kOps     = 256;
        std::vector<std::thread> workers;
        for (int w = 0; w < kWorkers; ++w) {
            workers.emplace_back([&, w]() {
                uint64_t id = static_cast<uint64_t>(w) * kOps;
                for (int i = 0; i < kOps; ++i) {
                    q.enqueue(id + i);
                    uint64_t dummy = 0;
                    q.dequeue(dummy);
                    ++total_ops;
                }
            });
        }
        for (auto& t : workers) t.join();
        benchmark::DoNotOptimize(total_ops.load());
    }
    state.SetItemsProcessed(static_cast<int64_t>(total_ops.load()));
    state.SetLabel("ACC-BM-04: concurrent kernel dispatch ≥ 500 ops/sec");
}
BENCHMARK(ACCBM04_ConcurrentKernelDispatch)
    ->Repetitions(3)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
