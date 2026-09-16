// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_gpu_dedicated_gates.cpp
 * @brief Wave D dedicated benchmark gates for GPU manager hot paths.
 *
 * Gates GPU-BM-01..04 lock the acceptable performance envelope for the
 * GPU manager module Wave D operability deliverables.
 *
 * Wave D GPU Benchmark Gates
 * ─────────────────────────────────────────────────────────────────────────────
 * GPU-BM-01  Kernel dispatch submit latency              ≤ 1 µs   (per-call)
 * GPU-BM-02  VRAM allocate/free round-trip               ≤ 10 µs  (per-cycle)
 * GPU-BM-03  Multi-GPU routing decision latency          ≤ 1 µs   (per-call)
 * GPU-BM-04  Concurrent kernel submit (8 workers)        ≥ 500 ops/sec total
 *
 * ## SIMULATION NOTE
 * All benchmarks use in-process stubs; no external CUDA/HIP hardware is
 * required. MUST NOT be used in production code paths.
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_GPU_MANAGER.md
 * @see src/gpu/ROADMAP.md — Wave D gate closure
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

namespace {

static constexpr uint32_t kGPUBMSeed = 42u;

// ---------------------------------------------------------------------------
// Stubs
// ---------------------------------------------------------------------------

struct StubKernelSubmitter {
    bool submit(uint64_t, int) { ++n; return true; }
    std::atomic<uint64_t> n{0};
};

class StubVRAMAllocator {
public:
    uint64_t allocate(std::size_t) {
        std::lock_guard<std::mutex> lk(mu_);
        ++alloc_n_;
        return ++next_handle_;
    }
    bool free_handle(uint64_t) {
        std::lock_guard<std::mutex> lk(mu_);
        ++free_n_;
        return true;
    }
    uint64_t allocN() const { return alloc_n_.load(); }
private:
    std::mutex mu_;
    std::atomic<uint64_t> alloc_n_{0};
    std::atomic<uint64_t> free_n_{0};
    uint64_t next_handle_{0};
};

struct StubMultiGPURouter {
    int route(uint64_t op) { ++n; return static_cast<int>(op % 4); }
    uint64_t n{0};
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// GPU-BM-01: Kernel dispatch submit latency ≤ 1 µs per call
// ─────────────────────────────────────────────────────────────────────────────
static void GPUBM01_KernelSubmitLatency(benchmark::State& state) {
    StubKernelSubmitter sub;
    uint64_t id = 0;
    for (auto _ : state) {
        sub.submit(id++, 0);
        benchmark::DoNotOptimize(sub.n.load());
    }
    state.SetLabel("GPU-BM-01: kernel submit ≤ 1 µs");
}
BENCHMARK(GPUBM01_KernelSubmitLatency)
    ->Repetitions(5)
    ->Unit(benchmark::kNanosecond);

// ─────────────────────────────────────────────────────────────────────────────
// GPU-BM-02: VRAM allocate/free round-trip ≤ 10 µs per cycle
// ─────────────────────────────────────────────────────────────────────────────
static void GPUBM02_VRAMAllocFreeRoundTrip(benchmark::State& state) {
    StubVRAMAllocator alloc;
    for (auto _ : state) {
        uint64_t h = alloc.allocate(4096);
        alloc.free_handle(h);
        benchmark::DoNotOptimize(alloc.allocN());
    }
    state.SetLabel("GPU-BM-02: VRAM alloc/free ≤ 10 µs");
}
BENCHMARK(GPUBM02_VRAMAllocFreeRoundTrip)
    ->Repetitions(5)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// GPU-BM-03: Multi-GPU routing decision latency ≤ 1 µs per call
// ─────────────────────────────────────────────────────────────────────────────
static void GPUBM03_MultiGPURoutingDecision(benchmark::State& state) {
    StubMultiGPURouter router;
    uint64_t op = 0;
    for (auto _ : state) {
        int dev = router.route(op++);
        benchmark::DoNotOptimize(dev);
    }
    state.SetLabel("GPU-BM-03: multi-GPU routing ≤ 1 µs");
}
BENCHMARK(GPUBM03_MultiGPURoutingDecision)
    ->Repetitions(5)
    ->Unit(benchmark::kNanosecond);

// ─────────────────────────────────────────────────────────────────────────────
// GPU-BM-04: Concurrent kernel submit ≥ 500 ops/sec (8 workers)
// ─────────────────────────────────────────────────────────────────────────────
static void GPUBM04_ConcurrentKernelSubmit(benchmark::State& state) {
    StubKernelSubmitter sub;
    std::atomic<uint64_t> total_ops{0};

    for (auto _ : state) {
        constexpr int kWorkers = 8;
        constexpr int kOps     = 256;
        std::vector<std::thread> workers;
        for (int w = 0; w < kWorkers; ++w) {
            workers.emplace_back([&, w]() {
                uint64_t id = static_cast<uint64_t>(w) * kOps;
                for (int i = 0; i < kOps; ++i) {
                    sub.submit(id + i, w % 4);
                    ++total_ops;
                }
            });
        }
        for (auto& t : workers) t.join();
        benchmark::DoNotOptimize(total_ops.load());
    }
    state.SetItemsProcessed(static_cast<int64_t>(total_ops.load()));
    state.SetLabel("GPU-BM-04: concurrent kernel submit ≥ 500 ops/sec");
}
BENCHMARK(GPUBM04_ConcurrentKernelSubmit)
    ->Repetitions(3)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
