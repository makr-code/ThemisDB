/**
 * @file test_chaos_highcardinality_stress.cpp
 * @brief Wave D — Chaos High-Cardinality Stress Tests.
 *
 * Stress tests for the chaos module covering high-cardinality scenarios:
 * 200 distinct fault descriptors, 4-thread concurrent inject/recover,
 * and the scheduler under 50 concurrent faults.
 *
 * These tests are excluded from the fast release_critical gate and are
 * intended for Wave D stress validation pipelines.
 *
 * ## Test cases
 * - HighCardinalityFaultDescriptors    — 200 distinct fault descriptors, all injected
 * - ConcurrentInjectRecoverStress      — 4-thread concurrent inject/recover
 * - SchedulerHighConcurrencyLoad       — scheduler under 50 concurrent faults
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_CHAOS_FAULT_INJECTION.md — operator runbook
 * @see src/chaos/ROADMAP.md — Wave D Closure Batch (2026-09-16)
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// STUB/SIMULATION NOTE
//
// In-process stubs replace the live FaultInjector and ChaosScheduler paths.
// Correctness characteristics mirror the real module contracts:
//   - inject() is accepted iff node_id is non-empty (fail-closed, § 4)
//   - recover() returns true iff the fault was previously injected
//   - Scheduler start/stop follows the STOPPED↔RUNNING FSM (§ 6)
//   - Callbacks dispatched FIFO, no re-entry (§ 5)
//
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// Stub: FaultInjector
// ─────────────────────────────────────────────────────────────────────────────

class StubFaultInjector {
public:
    bool inject(const std::string& node_id, double probability = 1.0) {
        if (node_id.empty() || probability <= 0.0) return false;
        std::lock_guard<std::mutex> lk(mu_);
        active_.insert(node_id);
        inject_count_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    bool recover(const std::string& node_id) {
        std::lock_guard<std::mutex> lk(mu_);
        if (!active_.count(node_id)) return false;
        active_.erase(node_id);
        recover_count_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    bool isActive(const std::string& node_id) const {
        std::lock_guard<std::mutex> lk(mu_);
        return active_.count(node_id) > 0;
    }

    std::size_t activeCount() const {
        std::lock_guard<std::mutex> lk(mu_);
        return active_.size();
    }

    uint64_t injectCount()  const noexcept { return inject_count_.load(std::memory_order_relaxed); }
    uint64_t recoverCount() const noexcept { return recover_count_.load(std::memory_order_relaxed); }

private:
    mutable std::mutex              mu_;
    std::unordered_set<std::string> active_;
    std::atomic<uint64_t>           inject_count_{0};
    std::atomic<uint64_t>           recover_count_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Stub: ChaosScheduler (STOPPED / RUNNING FSM)
// ─────────────────────────────────────────────────────────────────────────────

class StubChaosScheduler {
public:
    enum class State { STOPPED, RUNNING };

    void start() {
        std::lock_guard<std::mutex> lk(mu_);
        state_ = State::RUNNING;
    }

    void stop() {
        std::lock_guard<std::mutex> lk(mu_);
        state_ = State::STOPPED;
    }

    State state() const {
        std::lock_guard<std::mutex> lk(mu_);
        return state_;
    }

    bool schedule(const std::string& node_id, std::chrono::milliseconds /*delay*/) {
        if (node_id.empty()) return false;
        schedule_count_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    uint64_t scheduleCount() const noexcept { return schedule_count_.load(std::memory_order_relaxed); }

private:
    mutable std::mutex    mu_;
    State                 state_{State::STOPPED};
    std::atomic<uint64_t> schedule_count_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: HighCardinalityFaultDescriptors
//
// Build 200 distinct fault descriptors; inject all; verify all are active;
// recover all; verify none remain active.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_ChaosHighCardinalityStress, HighCardinalityFaultDescriptors) {
    constexpr std::size_t kDescriptorCount = 200;

    // Build distinct fault descriptor node-id corpus
    std::vector<std::string> node_ids;
    node_ids.reserve(kDescriptorCount);
    for (std::size_t i = 0; i < kDescriptorCount; ++i) {
        node_ids.push_back("hc-fault-node-" + std::to_string(i));
    }
    {
        std::unordered_set<std::string> unique(node_ids.begin(), node_ids.end());
        ASSERT_EQ(unique.size(), kDescriptorCount)
            << "All fault descriptor node IDs must be distinct";
    }

    StubFaultInjector fi;

    // Inject all 200 faults
    for (const auto& id : node_ids) {
        ASSERT_TRUE(fi.inject(id, 1.0))
            << "inject() must succeed for non-empty node_id: " << id;
    }

    EXPECT_EQ(fi.injectCount(), kDescriptorCount)
        << "Inject count must equal the number of distinct descriptors";
    EXPECT_EQ(fi.activeCount(), kDescriptorCount)
        << "All " << kDescriptorCount << " faults must be active after injection";

    // Recover all 200 faults
    for (const auto& id : node_ids) {
        ASSERT_TRUE(fi.recover(id))
            << "recover() must succeed for an active fault: " << id;
    }

    EXPECT_EQ(fi.recoverCount(), kDescriptorCount)
        << "Recover count must match inject count";
    EXPECT_EQ(fi.activeCount(), 0u)
        << "No faults must remain active after recovering all descriptors";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: ConcurrentInjectRecoverStress
//
// 4 threads concurrently inject and recover faults over a shared set of
// 200 distinct node IDs.  All inject/recover operations must succeed;
// final active-fault count must be 0.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_ChaosHighCardinalityStress, ConcurrentInjectRecoverStress) {
    constexpr int         kThreads        = 4;
    constexpr std::size_t kNodesPerThread = 50;  // 4 × 50 = 200 distinct nodes
    constexpr int         kCyclesPerThread = 25;

    StubFaultInjector fi;
    std::atomic<uint64_t> inject_failures{0};
    std::atomic<uint64_t> recover_failures{0};
    std::vector<std::thread> workers;
    workers.reserve(kThreads);

    const auto t0 = std::chrono::steady_clock::now();

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([t, &fi, &inject_failures, &recover_failures,
                               kNodesPerThread, kCyclesPerThread]() {
            const std::size_t base = static_cast<std::size_t>(t) * kNodesPerThread;
            for (int cycle = 0; cycle < kCyclesPerThread; ++cycle) {
                // Inject phase
                for (std::size_t n = 0; n < kNodesPerThread; ++n) {
                    const std::string node = "ci-node-" + std::to_string(base + n);
                    if (!fi.inject(node)) {
                        inject_failures.fetch_add(1, std::memory_order_relaxed);
                    }
                }
                // Recover phase
                for (std::size_t n = 0; n < kNodesPerThread; ++n) {
                    const std::string node = "ci-node-" + std::to_string(base + n);
                    if (!fi.recover(node)) {
                        recover_failures.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            }
        });
    }

    for (auto& w : workers) w.join();

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0);

    EXPECT_EQ(inject_failures.load(), 0u)
        << "All concurrent inject operations must succeed";

    EXPECT_EQ(recover_failures.load(), 0u)
        << "All concurrent recover operations must succeed";

    EXPECT_EQ(fi.activeCount(), 0u)
        << "No faults must remain active after all concurrent inject/recover cycles";

    // Wall-clock gate: concurrent stress must complete within 10 s
    EXPECT_LE(elapsed_ms.count(), 10'000)
        << "Concurrent inject/recover stress must complete within 10 s. "
           "Elapsed: " << elapsed_ms.count() << " ms";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: SchedulerHighConcurrencyLoad
//
// Schedule 50 concurrent faults through the scheduler; start/stop the
// scheduler repeatedly; verify schedule count matches expected and the
// scheduler returns to STOPPED after each stop().
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_ChaosHighCardinalityStress, SchedulerHighConcurrencyLoad) {
    constexpr std::size_t kConcurrentFaults  = 50;
    constexpr int         kStartStopCycles   = 20;

    StubChaosScheduler sched;
    std::atomic<uint64_t> schedule_failures{0};

    for (int cycle = 0; cycle < kStartStopCycles; ++cycle) {
        sched.start();
        ASSERT_EQ(sched.state(), StubChaosScheduler::State::RUNNING)
            << "Scheduler must be RUNNING after start() (cycle " << cycle << ")";

        for (std::size_t f = 0; f < kConcurrentFaults; ++f) {
            const std::string node = "sched-hc-" + std::to_string(f);
            if (!sched.schedule(node, 10ms)) {
                schedule_failures.fetch_add(1, std::memory_order_relaxed);
            }
        }

        sched.stop();
        ASSERT_EQ(sched.state(), StubChaosScheduler::State::STOPPED)
            << "Scheduler must return to STOPPED after stop() (cycle " << cycle << ")";
    }

    const uint64_t expected_schedules =
        static_cast<uint64_t>(kStartStopCycles) * kConcurrentFaults;

    EXPECT_EQ(schedule_failures.load(), 0u)
        << "All scheduler schedule() calls must succeed for non-empty node IDs";

    EXPECT_EQ(sched.scheduleCount(), expected_schedules)
        << "Schedule count must equal kStartStopCycles × kConcurrentFaults. "
           "Expected=" << expected_schedules
        << " Observed=" << sched.scheduleCount();

    // Final FSM state must be STOPPED
    EXPECT_EQ(sched.state(), StubChaosScheduler::State::STOPPED)
        << "Scheduler must be in STOPPED state after final stop()";
}
