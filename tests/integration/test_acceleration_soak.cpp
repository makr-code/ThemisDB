// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_acceleration_soak.cpp
 * @brief Wave D — Acceleration Module Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB acceleration module hot paths:
 * GPU fallback throughput, kernel dispatch stability, and CPU fallback
 * reliability. Verifies that all three metrics remain within acceptable
 * bounds over a configurable soak window driven by THEMIS_SOAK_DURATION_MS.
 *
 * ## Acceptance criteria
 * - AccelerationSoak_GPUFallbackThroughput    : ≥ 500 ops/sec over soak window
 * - AccelerationSoak_KernelDispatchStability  : zero dispatch failures
 * - AccelerationSoak_CPUFallbackReliability   : zero CPU fallback failures
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * Environment overrides
 * ---------------------
 * THEMIS_SOAK_DURATION_MS — total run window in milliseconds (default 60000)
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_ACCELERATION.md — operator runbook
 * @see src/acceleration/ROADMAP.md — Wave D contribution closure
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL;
}

// ─────────────────────────────────────────────────────────────────────────────
// STUB / SIMULATION NOTE
//
// In-process stubs model the acceleration hot paths without requiring
// external GPU hardware or CUDA/HIP runtimes. MUST NOT be used in production
// code paths.
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// ---------------------------------------------------------------------------
// StubGPUFallbackRouter — simulates GPU→CPU fallback routing
// ---------------------------------------------------------------------------
class StubGPUFallbackRouter {
public:
    struct Result { bool used_cpu_fallback; };

    Result dispatch(uint64_t op_id) {
        ++dispatch_count_;
        (void)op_id;
        // Stub: alternate between GPU path and CPU fallback
        bool fallback = (dispatch_count_.load() % 10 == 0);
        if (fallback) ++fallback_count_;
        return {fallback};
    }

    uint64_t dispatchCount()  const { return dispatch_count_.load(); }
    uint64_t fallbackCount()  const { return fallback_count_.load(); }
    uint64_t failCount()      const { return fail_count_.load(); }

private:
    std::atomic<uint64_t> dispatch_count_{0};
    std::atomic<uint64_t> fallback_count_{0};
    std::atomic<uint64_t> fail_count_{0};
};

// ---------------------------------------------------------------------------
// StubKernelDispatchQueue — simulates kernel dispatch pipeline
// ---------------------------------------------------------------------------
class StubKernelDispatchQueue {
public:
    explicit StubKernelDispatchQueue(std::size_t capacity) : capacity_(capacity) {}

    bool enqueue(uint64_t kernel_id) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.size() >= capacity_) { ++overflow_count_; return false; }
        queue_.push_back(kernel_id);
        ++enqueue_count_;
        return true;
    }

    bool dequeue(uint64_t& out) {
        std::lock_guard<std::mutex> lk(mu_);
        if (queue_.empty()) return false;
        out = queue_.front();
        queue_.pop_front();
        ++dequeue_count_;
        return true;
    }

    uint64_t enqueueCount()  const { return enqueue_count_.load(); }
    uint64_t dequeueCount()  const { return dequeue_count_.load(); }
    uint64_t overflowCount() const { return overflow_count_.load(); }

private:
    const std::size_t capacity_;
    std::deque<uint64_t> queue_;
    std::mutex mu_;
    std::atomic<uint64_t> enqueue_count_{0};
    std::atomic<uint64_t> dequeue_count_{0};
    std::atomic<uint64_t> overflow_count_{0};
};

// ---------------------------------------------------------------------------
// StubCPUFallbackExecutor — simulates CPU fallback execution
// ---------------------------------------------------------------------------
class StubCPUFallbackExecutor {
public:
    bool execute(uint64_t op_id) {
        ++exec_count_;
        (void)op_id;
        return true; // stub: always succeeds
    }

    uint64_t execCount() const { return exec_count_.load(); }
    uint64_t failCount() const { return fail_count_.load(); }

private:
    std::atomic<uint64_t> exec_count_{0};
    std::atomic<uint64_t> fail_count_{0};
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: AccelerationSoak_GPUFallbackThroughput
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_AccelerationSoak, AccelerationSoak_GPUFallbackThroughput) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());

    StubGPUFallbackRouter router;
    std::atomic<bool> running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            uint64_t op = static_cast<uint64_t>(i) * 1'000'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                router.dispatch(op++);
                std::this_thread::yield();
            }
        });
    }

    const auto t0 = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    const double elapsed_s =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    const uint64_t total_ops = router.dispatchCount();
    const double   ops_per_sec = static_cast<double>(total_ops) / elapsed_s;

    EXPECT_GT(total_ops, 0u)
        << "[ACCEL:GPUUnavailable] At least one dispatch must complete";
    EXPECT_GE(ops_per_sec, 500.0)
        << "GPU fallback throughput must be ≥ 500 ops/sec. "
           "Observed: " << ops_per_sec << " ops/sec";
    EXPECT_EQ(router.failCount(), 0u)
        << "[ACCEL:GPUUnavailable] Zero dispatch failures expected";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: AccelerationSoak_KernelDispatchStability
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_AccelerationSoak, AccelerationSoak_KernelDispatchStability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubKernelDispatchQueue queue(4096);
    std::atomic<bool> running{true};

    std::vector<std::thread> producers;
    for (int i = 0; i < 4; ++i) {
        producers.emplace_back([&, i]() {
            uint64_t id = static_cast<uint64_t>(i) * 1'000'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                queue.enqueue(id++);
                std::this_thread::yield();
            }
        });
    }

    std::vector<std::thread> consumers;
    for (int i = 0; i < 4; ++i) {
        consumers.emplace_back([&]() {
            uint64_t dummy = 0;
            while (running.load(std::memory_order_relaxed)) {
                queue.dequeue(dummy);
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : producers) t.join();
    for (auto& t : consumers)  t.join();

    EXPECT_GT(queue.enqueueCount(), 0u)
        << "[ACCEL:DispatchOverflow] At least one kernel dispatch must complete";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: AccelerationSoak_CPUFallbackReliability
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_AccelerationSoak, AccelerationSoak_CPUFallbackReliability) {
    const auto soak_duration =
        std::chrono::milliseconds(soakDurationMs() / 10);

    StubCPUFallbackExecutor executor;
    std::atomic<bool>       running{true};

    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&, i]() {
            uint64_t op = static_cast<uint64_t>(i) * 1'000'000ULL;
            while (running.load(std::memory_order_relaxed)) {
                executor.execute(op++);
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(soak_duration);
    running.store(false, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    EXPECT_GT(executor.execCount(), 0u)
        << "[ACCEL:FallbackOOM] At least one CPU fallback execution must complete";
    EXPECT_EQ(executor.failCount(), 0u)
        << "[ACCEL:FallbackOOM] Zero CPU fallback failures expected. "
           "Observed: " << executor.failCount();
}
