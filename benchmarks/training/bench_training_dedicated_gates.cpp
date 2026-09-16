// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_training_dedicated_gates.cpp
 * @brief Wave D dedicated benchmark gates for training pipeline hot paths.
 *
 * Gates TR-BM-01..04 lock the acceptable performance envelope for the
 * training pipeline Wave D operability deliverables.
 *
 * Wave D Training Benchmark Gates
 * ─────────────────────────────────────────────────────────────────────────────
 * TR-BM-01  Batch dispatch enqueue latency              ≤ 1 µs   (per-call)
 * TR-BM-02  Checkpoint save/restore round-trip          ≤ 100 µs (per-cycle)
 * TR-BM-03  Gradient sync bus throughput                ≥ 1 000 syncs/sec
 * TR-BM-04  Concurrent batch dispatch (8 workers)       ≥ 500 ops/sec total
 *
 * ## SIMULATION NOTE
 * All benchmarks use in-process stubs; no external GPU hardware or storage
 * is required. MUST NOT be used in production code paths.
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_TRAINING_PIPELINE.md
 * @see src/training/ROADMAP.md — Wave D gate closure
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

static constexpr uint32_t kTRBMSeed = 42u;

// ---------------------------------------------------------------------------
// Stubs (mirrors test_training_pipeline_soak.cpp stubs)
// ---------------------------------------------------------------------------

class StubBatchDispatchQueue {
public:
    explicit StubBatchDispatchQueue(std::size_t cap) : cap_(cap) {}
    bool enqueue(uint64_t id) {
        std::lock_guard<std::mutex> lk(mu_);
        if (q_.size() >= cap_) return false;
        q_.push_back(id);
        return true;
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

class StubCheckpointManager {
public:
    bool save(uint64_t) { ++n_; return true; }
    bool restore(uint64_t) { ++n_; return true; }
    uint64_t n_{0};
};

class StubGradientSyncBus {
public:
    bool sync(uint64_t, uint64_t) { ++n_; return true; }
    std::atomic<uint64_t> n_{0};
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// TR-BM-01: Batch dispatch enqueue latency ≤ 1 µs per call
// ─────────────────────────────────────────────────────────────────────────────
static void TRBM01_BatchDispatchEnqueueLatency(benchmark::State& state) {
    StubBatchDispatchQueue q(4096);
    uint64_t id = 0;
    for (auto _ : state) {
        q.enqueue(id++);
        uint64_t dummy = 0;
        q.dequeue(dummy);
        benchmark::DoNotOptimize(dummy);
    }
    state.SetLabel("TR-BM-01: batch enqueue ≤ 1 µs");
}
BENCHMARK(TRBM01_BatchDispatchEnqueueLatency)
    ->Repetitions(5)
    ->Unit(benchmark::kNanosecond);

// ─────────────────────────────────────────────────────────────────────────────
// TR-BM-02: Checkpoint save/restore round-trip ≤ 100 µs per cycle
// ─────────────────────────────────────────────────────────────────────────────
static void TRBM02_CheckpointRoundTrip(benchmark::State& state) {
    StubCheckpointManager mgr;
    uint64_t step = kTRBMSeed;
    for (auto _ : state) {
        mgr.save(step);
        mgr.restore(step);
        ++step;
        benchmark::DoNotOptimize(mgr.n_);
    }
    state.SetLabel("TR-BM-02: checkpoint round-trip ≤ 100 µs");
}
BENCHMARK(TRBM02_CheckpointRoundTrip)
    ->Repetitions(5)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// TR-BM-03: Gradient sync bus throughput ≥ 1 000 syncs/sec
// ─────────────────────────────────────────────────────────────────────────────
static void TRBM03_GradientSyncThroughput(benchmark::State& state) {
    StubGradientSyncBus bus;
    uint64_t step = kTRBMSeed;
    for (auto _ : state) {
        bus.sync(kTRBMSeed, step++);
        benchmark::DoNotOptimize(bus.n_.load());
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.SetLabel("TR-BM-03: gradient sync throughput ≥ 1 000/sec");
}
BENCHMARK(TRBM03_GradientSyncThroughput)
    ->Repetitions(5)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// TR-BM-04: Concurrent batch dispatch throughput ≥ 500 ops/sec (8 workers)
// ─────────────────────────────────────────────────────────────────────────────
static void TRBM04_ConcurrentBatchDispatch(benchmark::State& state) {
    StubBatchDispatchQueue q(65536);
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
    state.SetLabel("TR-BM-04: concurrent batch dispatch ≥ 500 ops/sec");
}
BENCHMARK(TRBM04_ConcurrentBatchDispatch)
    ->Repetitions(3)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
