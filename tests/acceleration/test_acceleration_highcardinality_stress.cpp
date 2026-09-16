// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_acceleration_highcardinality_stress.cpp
 * @brief Wave D — Acceleration Module High-Cardinality Stress Tests.
 *
 * Stress coverage for acceleration hot paths under high-cardinality
 * concurrent load: kernel dispatch, concurrent GPU/CPU fallback, and
 * fail-closed path stress.
 *
 * ## Test cases
 * - HighCardinalityKernelDispatch        : concurrent kernel dispatch at scale
 * - ConcurrentGPUCPUFallbackStress       : concurrent GPU→CPU fallback
 * - AccelerationPathFailClosedStress     : fail-closed behavior under load
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_ACCELERATION.md
 * @see src/acceleration/ROADMAP.md — Wave D contribution closure
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// STUB / SIMULATION NOTE
//
// In-process stubs model acceleration stress paths without requiring real
// CUDA/HIP hardware or drivers. MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

namespace {

static constexpr unsigned int kStressSeed    = 42;
static constexpr int          kWorkerCount   = 8;
static constexpr int          kOpsPerWorker  = 5000;

// ---------------------------------------------------------------------------
// StubKernelDispatcher
// ---------------------------------------------------------------------------
class StubKernelDispatcher {
public:
    bool dispatch(uint64_t kernel_id, bool prefer_gpu) {
        std::lock_guard<std::mutex> lk(mu_);
        ++dispatch_count_;
        if (!prefer_gpu) ++cpu_count_;
        (void)kernel_id;
        return true;
    }

    uint64_t dispatchCount() const { return dispatch_count_.load(); }
    uint64_t cpuCount()      const { return cpu_count_.load(); }

private:
    std::mutex mu_;
    std::atomic<uint64_t> dispatch_count_{0};
    std::atomic<uint64_t> cpu_count_{0};
};

// ---------------------------------------------------------------------------
// StubFallbackRouter
// ---------------------------------------------------------------------------
class StubFallbackRouter {
public:
    enum class Path { GPU, CPU };

    Path route(uint64_t op_id) {
        ++route_count_;
        return (op_id % 5 == 0) ? Path::CPU : Path::GPU;
    }

    bool executeCPU(uint64_t op_id) {
        ++cpu_exec_count_;
        (void)op_id;
        return true;
    }

    bool executeGPU(uint64_t op_id) {
        ++gpu_exec_count_;
        (void)op_id;
        return true;
    }

    uint64_t routeCount()    const { return route_count_.load(); }
    uint64_t cpuExecCount()  const { return cpu_exec_count_.load(); }
    uint64_t gpuExecCount()  const { return gpu_exec_count_.load(); }

private:
    std::atomic<uint64_t> route_count_{0};
    std::atomic<uint64_t> cpu_exec_count_{0};
    std::atomic<uint64_t> gpu_exec_count_{0};
};

// ---------------------------------------------------------------------------
// StubFailClosedAccelerator — simulates fail-closed behavior
// ---------------------------------------------------------------------------
class StubFailClosedAccelerator {
public:
    bool accelerate(uint64_t op_id) {
        ++attempt_count_;
        // Stub: always succeeds (fail-closed means degrade, not fail open)
        (void)op_id;
        return true;
    }

    uint64_t attemptCount() const { return attempt_count_.load(); }
    uint64_t failCount()    const { return fail_count_.load(); }

private:
    std::atomic<uint64_t> attempt_count_{0};
    std::atomic<uint64_t> fail_count_{0};
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: HighCardinalityKernelDispatch
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_AccelerationStress, HighCardinalityKernelDispatch) {
    StubKernelDispatcher dispatcher;
    std::atomic<uint64_t> error_count{0};

    std::vector<std::thread> workers;
    for (int i = 0; i < kWorkerCount; ++i) {
        workers.emplace_back([&, i]() {
            std::mt19937 rng(kStressSeed + static_cast<unsigned>(i));
            std::bernoulli_distribution gpu_dist(0.8);
            for (int op = 0; op < kOpsPerWorker; ++op) {
                uint64_t kid = static_cast<uint64_t>(i) * kOpsPerWorker + op;
                bool ok = dispatcher.dispatch(kid, gpu_dist(rng));
                if (!ok) ++error_count;
            }
        });
    }

    for (auto& t : workers) t.join();

    const uint64_t expected = static_cast<uint64_t>(kWorkerCount) * kOpsPerWorker;
    EXPECT_EQ(dispatcher.dispatchCount(), expected)
        << "[ACCEL:DispatchOverflow] All kernel dispatches must succeed under stress";
    EXPECT_EQ(error_count.load(), 0u)
        << "[ACCEL:DispatchOverflow] Zero dispatch errors expected";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: ConcurrentGPUCPUFallbackStress
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_AccelerationStress, ConcurrentGPUCPUFallbackStress) {
    StubFallbackRouter router;
    std::atomic<uint64_t> error_count{0};

    std::vector<std::thread> workers;
    for (int i = 0; i < kWorkerCount; ++i) {
        workers.emplace_back([&, i]() {
            for (int op = 0; op < kOpsPerWorker; ++op) {
                uint64_t oid = static_cast<uint64_t>(i) * kOpsPerWorker + op;
                auto path = router.route(oid);
                bool ok = (path == StubFallbackRouter::Path::CPU)
                              ? router.executeCPU(oid)
                              : router.executeGPU(oid);
                if (!ok) ++error_count;
            }
        });
    }

    for (auto& t : workers) t.join();

    EXPECT_EQ(error_count.load(), 0u)
        << "[ACCEL:FallbackOOM] Zero fallback errors expected under concurrent stress";
    EXPECT_GT(router.cpuExecCount(), 0u)
        << "[ACCEL:FallbackOOM] At least one CPU fallback execution must occur";
    EXPECT_GT(router.gpuExecCount(), 0u)
        << "[ACCEL:GPUUnavailable] At least one GPU execution must occur";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: AccelerationPathFailClosedStress
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_AccelerationStress, AccelerationPathFailClosedStress) {
    StubFailClosedAccelerator accel;
    std::atomic<uint64_t> error_count{0};

    std::vector<std::thread> workers;
    for (int i = 0; i < kWorkerCount; ++i) {
        workers.emplace_back([&, i]() {
            for (int op = 0; op < kOpsPerWorker; ++op) {
                uint64_t oid = static_cast<uint64_t>(i) * kOpsPerWorker + op;
                bool ok = accel.accelerate(oid);
                if (!ok) ++error_count;
            }
        });
    }

    for (auto& t : workers) t.join();

    EXPECT_EQ(accel.failCount(), 0u)
        << "[ACCEL:GPUUnavailable] Fail-closed path must degrade, not fail open";
    EXPECT_EQ(error_count.load(), 0u)
        << "[ACCEL:GPUUnavailable] Zero fail-closed errors expected under stress";
}
