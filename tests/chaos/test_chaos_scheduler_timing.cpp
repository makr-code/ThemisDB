/**
 * @file test_chaos_scheduler_timing.cpp
 * @brief Phase 4 resilience regression tests for ChaosScheduler timing race conditions.
 *
 * Covers eight test cases mapped to chaos roadmap Phase 4 hardening:
 *
 * ### CTI — Chaos Timing and Interface Regressions
 *   CTI-01  stop() called immediately after start() — no deadlock, isRunning() returns false
 *   CTI-02  Restart (stop + start) cycle repeated N times — stable, no resource leak
 *   CTI-03  scheduleIn() called after stop() — entry is queued but not fired
 *   CTI-04  start() called while scheduler already running — idempotent, single worker thread
 *   CTI-05  stop() on a STOPPED scheduler is idempotent — no exception, no crash
 *   CTI-06  CONDVAR wake strategy: stop() latency is bounded within kSchedulerStopTimeout
 *   CTI-07  FIXED_TICK wake strategy: fault fires within expected tick window
 *   CTI-08  Mixed wake strategies: two independent schedulers run concurrently without interference
 *
 * @see src/chaos/ROADMAP.md — Phase 4 item (expand resilience regressions for timing races)
 * @see include/chaos/chaos_contract.h — § 6 scheduler state contract, kSchedulerStopTimeout
 */

#include <gtest/gtest.h>

#include "chaos/chaos_framework.h"
#include "chaos/chaos_contract.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>

using namespace themis::chaos;
using namespace std::chrono_literals;

namespace {

FaultSpec makeSpec(std::string_view node, FaultType type = FaultType::NODE_FAILURE) {
    return FaultSpec{type, std::string(node)};
}

} // anonymous namespace

// ============================================================================
// CTI-01: stop() immediately after start() — no deadlock
// ============================================================================

/**
 * @test CTI-01 — stop() called immediately after start() must return within
 *       kSchedulerStopTimeout and leave isRunning() == false.
 *
 * @see chaos_contract.h § 6 — kSchedulerStopTimeout
 */
TEST(ChaosSchedulerTimingTest, CTI01_ImmediateStopAfterStart) {
    auto fi = std::make_shared<FaultInjector>("cti01");
    ChaosScheduler sched{fi, ChaosSchedulerConfig{/*.tick_interval=*/10ms,
                                                   /*.wake_strategy=*/WakeStrategy::CONDVAR}};

    sched.start();
    EXPECT_TRUE(sched.isRunning());

    const auto t0 = std::chrono::steady_clock::now();
    sched.stop();
    const auto elapsed = std::chrono::steady_clock::now() - t0;

    EXPECT_FALSE(sched.isRunning());
    EXPECT_LT(elapsed, themis::chaos::kSchedulerStopTimeout)
        << "stop() took longer than kSchedulerStopTimeout";
}

// ============================================================================
// CTI-02: Restart cycle N times — stable, no resource leak
// ============================================================================

/**
 * @test CTI-02 — Repeated stop + start cycles are stable; the scheduler can
 *       be restarted after every stop without crashing or leaking resources.
 */
TEST(ChaosSchedulerTimingTest, CTI02_RepeatRestartCycle) {
    auto fi = std::make_shared<FaultInjector>("cti02");
    ChaosScheduler sched{fi, ChaosSchedulerConfig{/*.tick_interval=*/5ms}};

    constexpr int kCycles = 8;
    for (int i = 0; i < kCycles; ++i) {
        sched.start();
        EXPECT_TRUE(sched.isRunning()) << "iteration " << i;
        sched.stop();
        EXPECT_FALSE(sched.isRunning()) << "iteration " << i;
    }
}

// ============================================================================
// CTI-03: scheduleIn() after stop() — queued but not fired
// ============================================================================

/**
 * @test CTI-03 — Entries added with scheduleIn() when the scheduler is STOPPED
 *       are added to the pending queue but MUST NOT fire until start() is called.
 *
 * @see chaos_contract.h § 6 — scheduleIn() on STOPPED scheduler may queue entry
 */
TEST(ChaosSchedulerTimingTest, CTI03_ScheduleAfterStopNotFired) {
    auto fi = std::make_shared<FaultInjector>("cti03");
    ChaosScheduler sched{fi, ChaosSchedulerConfig{/*.tick_interval=*/5ms}};

    // Scheduler is not started. Schedule a fault with immediate delay.
    sched.scheduleIn(0ms, makeSpec("cti03-n1"));
    EXPECT_GE(sched.pendingCount(), 1u);

    // Wait longer than one tick — the fault must NOT fire because we never called start().
    std::this_thread::sleep_for(30ms);
    EXPECT_FALSE(fi->isFaultActive("cti03-n1"));
}

// ============================================================================
// CTI-04: start() while already running — idempotent
// ============================================================================

/**
 * @test CTI-04 — Calling start() while the scheduler is already running is
 *       idempotent: it must not launch a second worker thread or crash.
 *
 * @see chaos_contract.h § 6 — start() on RUNNING is idempotent
 */
TEST(ChaosSchedulerTimingTest, CTI04_DoubleStartIdempotent) {
    auto fi = std::make_shared<FaultInjector>("cti04");
    ChaosScheduler sched{fi, ChaosSchedulerConfig{/*.tick_interval=*/10ms}};

    sched.start();
    sched.start();  // must be idempotent
    EXPECT_TRUE(sched.isRunning());

    sched.stop();
    EXPECT_FALSE(sched.isRunning());
}

// ============================================================================
// CTI-05: stop() on STOPPED scheduler — idempotent
// ============================================================================

/**
 * @test CTI-05 — Calling stop() when the scheduler has never been started (or
 *       has already been stopped) must be a no-op without throwing.
 *
 * @see chaos_contract.h § 6 — stop() on STOPPED is idempotent
 */
TEST(ChaosSchedulerTimingTest, CTI05_StopOnStoppedIdempotent) {
    auto fi = std::make_shared<FaultInjector>("cti05");
    ChaosScheduler sched{fi};

    // Never started — stop() must be safe.
    EXPECT_NO_THROW(sched.stop());
    EXPECT_FALSE(sched.isRunning());

    // Stop again after a regular lifecycle.
    sched.start();
    sched.stop();
    EXPECT_NO_THROW(sched.stop());
    EXPECT_FALSE(sched.isRunning());
}

// ============================================================================
// CTI-06: CONDVAR stop() latency bounded by kSchedulerStopTimeout
// ============================================================================

/**
 * @test CTI-06 — With WakeStrategy::CONDVAR, stop() completes within
 *       themis::chaos::kSchedulerStopTimeout regardless of tick_interval.
 *
 * @see chaos_contract.h § 6 — kSchedulerStopTimeout = 500 ms
 */
TEST(ChaosSchedulerTimingTest, CTI06_CondvarStopLatencyBounded) {
    auto fi = std::make_shared<FaultInjector>("cti06");
    // Use a long tick_interval to exercise the condvar early-wake path.
    ChaosScheduler sched{fi,
                         ChaosSchedulerConfig{/*.tick_interval=*/200ms,
                                              /*.wake_strategy=*/WakeStrategy::CONDVAR}};
    sched.start();
    EXPECT_TRUE(sched.isRunning());

    const auto t0 = std::chrono::steady_clock::now();
    sched.stop();
    const auto elapsed = std::chrono::steady_clock::now() - t0;

    EXPECT_FALSE(sched.isRunning());
    // With CONDVAR, stop() should wake the thread immediately; actual stop
    // must complete well within the 500 ms contract bound.
    EXPECT_LT(elapsed, kSchedulerStopTimeout)
        << "CONDVAR stop() exceeded kSchedulerStopTimeout";
}

// ============================================================================
// CTI-07: FIXED_TICK fault fires within expected tick window
// ============================================================================

/**
 * @test CTI-07 — With WakeStrategy::FIXED_TICK, a scheduled fault fires within
 *       the expected deadline (delay + 3 × tick_interval as headroom).
 */
TEST(ChaosSchedulerTimingTest, CTI07_FixedTickFaultFiresInWindow) {
    auto fi = std::make_shared<FaultInjector>("cti07");
    constexpr auto kTick  = 10ms;
    constexpr auto kDelay = 20ms;

    ChaosScheduler sched{fi,
                         ChaosSchedulerConfig{/*.tick_interval=*/kTick,
                                              /*.wake_strategy=*/WakeStrategy::FIXED_TICK}};
    sched.start();
    sched.scheduleIn(kDelay, makeSpec("cti07-n1"));

    const auto deadline = std::chrono::steady_clock::now() + kDelay + 3 * kTick + 50ms;
    while (!fi->isFaultActive("cti07-n1") &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(kTick / 2);
    }

    EXPECT_TRUE(fi->isFaultActive("cti07-n1"));
    sched.stop();
}

// ============================================================================
// CTI-08: Mixed wake strategies run concurrently without interference
// ============================================================================

/**
 * @test CTI-08 — Two independent ChaosScheduler instances (one FIXED_TICK, one
 *       CONDVAR) running concurrently inject into separate FaultInjectors without
 *       interfering with each other.
 */
TEST(ChaosSchedulerTimingTest, CTI08_TwoSchedulersConcurrentNoInterference) {
    auto fi_a = std::make_shared<FaultInjector>("cti08-a");
    auto fi_b = std::make_shared<FaultInjector>("cti08-b");

    ChaosScheduler sched_a{fi_a,
                            ChaosSchedulerConfig{/*.tick_interval=*/10ms,
                                                 /*.wake_strategy=*/WakeStrategy::FIXED_TICK}};
    ChaosScheduler sched_b{fi_b,
                            ChaosSchedulerConfig{/*.tick_interval=*/10ms,
                                                 /*.wake_strategy=*/WakeStrategy::CONDVAR}};

    sched_a.start();
    sched_b.start();

    sched_a.scheduleIn(15ms, makeSpec("cti08-a-n1"));
    sched_b.scheduleIn(15ms, makeSpec("cti08-b-n1"));

    const auto deadline = std::chrono::steady_clock::now() + 200ms;
    while ((!fi_a->isFaultActive("cti08-a-n1") || !fi_b->isFaultActive("cti08-b-n1")) &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(5ms);
    }

    sched_a.stop();
    sched_b.stop();

    // Each scheduler must have fired into its own injector only.
    EXPECT_TRUE(fi_a->isFaultActive("cti08-a-n1"));
    EXPECT_TRUE(fi_b->isFaultActive("cti08-b-n1"));

    // Cross-contamination check: wrong injector must NOT have the other's fault.
    EXPECT_FALSE(fi_a->isFaultActive("cti08-b-n1"));
    EXPECT_FALSE(fi_b->isFaultActive("cti08-a-n1"));
}
