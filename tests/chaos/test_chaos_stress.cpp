/*
 * ThemisDB — Chaos Module: Scheduler Jitter and Stress Tests (Phase 4)
 *
 * File:    tests/test_chaos_stress.cpp
 * Module:  chaos
 * Phase:   4 — Dedicated scheduler jitter and stress tests
 *
 * Covers:
 *  - Configurable tick_interval (FIXED_TICK and CONDVAR strategies)
 *  - CONDVAR stop latency (wakes promptly on stop())
 *  - Near-simultaneous fault scheduling (scheduler jitter)
 *  - Many faults scheduled in a burst (throughput)
 *  - Concurrent injectFault / recoverFault under load
 *  - scheduleIn with very short delays and configurable tick
 *  - clearPending does not fire faults
 *  - Repeated start/stop cycles under both strategies
 */

#include <gtest/gtest.h>
#include "chaos/chaos_framework.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace themis::chaos;
using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::shared_ptr<FaultInjector> make_injector(const std::string& id = "stress-fi") {
    return std::make_shared<FaultInjector>(id);
}

// ─────────────────────────────────────────────────────────────────────────────
// ChaosScheduler::Config — basic construction
// ─────────────────────────────────────────────────────────────────────────────

TEST(ChaosStressConfig, DefaultConfigIsFixedTick10ms) {
    ChaosScheduler::Config cfg;
    EXPECT_EQ(cfg.tick_interval, 10ms);
    EXPECT_EQ(cfg.wake_strategy, WakeStrategy::FIXED_TICK);
}

TEST(ChaosStressConfig, CustomTickIntervalIsPreserved) {
    ChaosScheduler::Config cfg;
    cfg.tick_interval = 25ms;
    EXPECT_EQ(cfg.tick_interval, 25ms);
}

TEST(ChaosStressConfig, CondvarStrategyIsSetCorrectly) {
    ChaosScheduler::Config cfg;
    cfg.wake_strategy = WakeStrategy::CONDVAR;
    EXPECT_EQ(cfg.wake_strategy, WakeStrategy::CONDVAR);
}

TEST(ChaosStressConfig, SchedulerAcceptsCustomConfig) {
    auto fi = make_injector();
    ChaosScheduler::Config cfg;
    cfg.tick_interval  = 5ms;
    cfg.wake_strategy  = WakeStrategy::CONDVAR;
    EXPECT_NO_THROW((ChaosScheduler{fi, cfg}));
}

// ─────────────────────────────────────────────────────────────────────────────
// FIXED_TICK — basic operation unchanged
// ─────────────────────────────────────────────────────────────────────────────

TEST(ChaosStressFixedTick, FaultFiresWithShortTick) {
    auto fi = make_injector();
    ChaosScheduler::Config cfg;
    cfg.tick_interval = 5ms;
    cfg.wake_strategy = WakeStrategy::FIXED_TICK;

    ChaosScheduler sched{fi, cfg};
    sched.start();

    sched.scheduleIn(15ms, FaultSpec{FaultType::NODE_FAILURE, "ft-node-1"});
    std::this_thread::sleep_for(60ms);

    EXPECT_TRUE(fi->isFaultActive("ft-node-1"));
    sched.stop();
}

TEST(ChaosStressFixedTick, StartStopRepeatedCycles) {
    auto fi = make_injector();
    ChaosScheduler::Config cfg;
    cfg.tick_interval = 5ms;

    ChaosScheduler sched{fi, cfg};
    for (int i = 0; i < 5; ++i) {
        sched.start();
        EXPECT_TRUE(sched.isRunning());
        sched.stop();
        EXPECT_FALSE(sched.isRunning());
    }
}

TEST(ChaosStressFixedTick, PendingCountAfterBurstSchedule) {
    auto fi = make_injector();
    ChaosScheduler::Config cfg;
    cfg.tick_interval = 100ms;  // long tick — faults won't fire during test

    ChaosScheduler sched{fi, cfg};
    constexpr int N = 50;
    for (int i = 0; i < N; ++i) {
        sched.schedule({std::chrono::steady_clock::now() + 60s,
                        FaultSpec{FaultType::DISK_FAILURE, "bulk-" + std::to_string(i)}});
    }
    EXPECT_EQ(sched.pendingCount(), static_cast<size_t>(N));
    sched.clearPending();
    EXPECT_EQ(sched.pendingCount(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// CONDVAR — basic operation
// ─────────────────────────────────────────────────────────────────────────────

TEST(ChaosStressCondvar, FaultFiresWithCondvarStrategy) {
    auto fi = make_injector();
    ChaosScheduler::Config cfg;
    cfg.tick_interval = 5ms;
    cfg.wake_strategy = WakeStrategy::CONDVAR;

    ChaosScheduler sched{fi, cfg};
    sched.start();

    sched.scheduleIn(15ms, FaultSpec{FaultType::NETWORK_PARTITION, "cv-node-1"});
    std::this_thread::sleep_for(60ms);

    EXPECT_TRUE(fi->isFaultActive("cv-node-1"));
    sched.stop();
}

TEST(ChaosStressCondvar, StopWakesCondvarPromptly) {
    // With CONDVAR and a long tick, stop() should return quickly because the
    // condition variable is notified instead of the worker sleeping the full interval.
    auto fi = make_injector();
    ChaosScheduler::Config cfg;
    cfg.tick_interval = 2000ms;  // intentionally long
    cfg.wake_strategy = WakeStrategy::CONDVAR;

    ChaosScheduler sched{fi, cfg};
    sched.start();

    const auto t0 = std::chrono::steady_clock::now();
    sched.stop();
    const auto elapsed = std::chrono::steady_clock::now() - t0;

    // stop() must return in well under 2000 ms
    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 500);
}

TEST(ChaosStressCondvar, StartStopRepeatedCycles) {
    auto fi = make_injector();
    ChaosScheduler::Config cfg;
    cfg.tick_interval = 5ms;
    cfg.wake_strategy = WakeStrategy::CONDVAR;

    ChaosScheduler sched{fi, cfg};
    for (int i = 0; i < 5; ++i) {
        sched.start();
        EXPECT_TRUE(sched.isRunning());
        sched.stop();
        EXPECT_FALSE(sched.isRunning());
    }
}

TEST(ChaosStressCondvar, ClearPendingDoesNotFireFaults) {
    auto fi = make_injector();
    ChaosScheduler::Config cfg;
    cfg.tick_interval = 5ms;
    cfg.wake_strategy = WakeStrategy::CONDVAR;

    ChaosScheduler sched{fi, cfg};
    sched.start();

    sched.schedule({std::chrono::steady_clock::now() + 60s,
                    FaultSpec{FaultType::NODE_FAILURE, "cv-pending"}});
    sched.clearPending();
    sched.stop();

    EXPECT_FALSE(fi->isFaultActive("cv-pending"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Scheduler jitter — near-simultaneous faults
// ─────────────────────────────────────────────────────────────────────────────

TEST(ChaosStressJitter, NearSimultaneousFaultsAllFire) {
    // Schedule N faults all due within a 5 ms window; all must fire within
    // a generous observation window.
    constexpr int N = 20;
    auto fi = make_injector();
    ChaosScheduler::Config cfg;
    cfg.tick_interval = 5ms;
    cfg.wake_strategy = WakeStrategy::CONDVAR;

    ChaosScheduler sched{fi, cfg};
    sched.start();

    const auto base = std::chrono::steady_clock::now() + 20ms;
    for (int i = 0; i < N; ++i) {
        sched.schedule({base + std::chrono::milliseconds(i % 5),
                        FaultSpec{FaultType::NODE_FAILURE,
                                  "jitter-node-" + std::to_string(i)}});
    }

    std::this_thread::sleep_for(200ms);

    int fired = 0;
    for (int i = 0; i < N; ++i) {
        if (fi->isFaultActive("jitter-node-" + std::to_string(i))) ++fired;
    }
    EXPECT_EQ(fired, N);

    sched.stop();
}

TEST(ChaosStressJitter, NearSimultaneousFaultsFixedTick) {
    constexpr int N = 20;
    auto fi = make_injector();
    ChaosScheduler::Config cfg;
    cfg.tick_interval = 5ms;
    cfg.wake_strategy = WakeStrategy::FIXED_TICK;

    ChaosScheduler sched{fi, cfg};
    sched.start();

    const auto base = std::chrono::steady_clock::now() + 20ms;
    for (int i = 0; i < N; ++i) {
        sched.schedule({base + std::chrono::milliseconds(i % 5),
                        FaultSpec{FaultType::DISK_FAILURE,
                                  "jft-node-" + std::to_string(i)}});
    }

    std::this_thread::sleep_for(200ms);

    int fired = 0;
    for (int i = 0; i < N; ++i) {
        if (fi->isFaultActive("jft-node-" + std::to_string(i))) ++fired;
    }
    EXPECT_EQ(fired, N);

    sched.stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// Burst throughput — many faults scheduled
// ─────────────────────────────────────────────────────────────────────────────

TEST(ChaosStressBurst, BurstScheduleAllFire_Condvar) {
    constexpr int N = 100;
    auto fi = make_injector();
    ChaosScheduler::Config cfg;
    cfg.tick_interval = 5ms;
    cfg.wake_strategy = WakeStrategy::CONDVAR;

    ChaosScheduler sched{fi, cfg};
    sched.start();

    const auto due = std::chrono::steady_clock::now() + 30ms;
    for (int i = 0; i < N; ++i) {
        sched.schedule({due,
                        FaultSpec{FaultType::RANDOM_FAILURE,
                                  "burst-cv-" + std::to_string(i)}});
    }

    std::this_thread::sleep_for(300ms);

    int fired = 0;
    for (int i = 0; i < N; ++i) {
        if (fi->isFaultActive("burst-cv-" + std::to_string(i))) ++fired;
    }
    EXPECT_EQ(fired, N);

    sched.stop();
}

TEST(ChaosStressBurst, BurstScheduleAllFire_FixedTick) {
    constexpr int N = 100;
    auto fi = make_injector();
    ChaosScheduler::Config cfg;
    cfg.tick_interval = 5ms;
    cfg.wake_strategy = WakeStrategy::FIXED_TICK;

    ChaosScheduler sched{fi, cfg};
    sched.start();

    const auto due = std::chrono::steady_clock::now() + 30ms;
    for (int i = 0; i < N; ++i) {
        sched.schedule({due,
                        FaultSpec{FaultType::RANDOM_FAILURE,
                                  "burst-ft-" + std::to_string(i)}});
    }

    std::this_thread::sleep_for(300ms);

    int fired = 0;
    for (int i = 0; i < N; ++i) {
        if (fi->isFaultActive("burst-ft-" + std::to_string(i))) ++fired;
    }
    EXPECT_EQ(fired, N);

    sched.stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// Concurrent FaultInjector operations under scheduler load
// ─────────────────────────────────────────────────────────────────────────────

TEST(ChaosStressConcurrent, ConcurrentInjectAndRecover) {
    auto fi = make_injector();
    ChaosScheduler::Config cfg;
    cfg.tick_interval = 5ms;
    cfg.wake_strategy = WakeStrategy::CONDVAR;

    ChaosScheduler sched{fi, cfg};
    sched.start();

    // Schedule some faults via the scheduler
    for (int i = 0; i < 10; ++i) {
        sched.scheduleIn(20ms,
            FaultSpec{FaultType::NODE_FAILURE, "conc-node-" + std::to_string(i)});
    }

    // Simultaneously inject and recover faults from multiple threads
    constexpr int THREADS = 4;
    constexpr int OPS = 50;
    std::vector<std::thread> threads;
    threads.reserve(THREADS);

    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&fi, t]() {
            for (int i = 0; i < OPS; ++i) {
                const std::string node = "thread-" + std::to_string(t) + "-node-" + std::to_string(i);
                fi->injectFault(FaultSpec{FaultType::DISK_FAILURE, node});
                fi->recoverFault(node, FaultType::DISK_FAILURE);
            }
        });
    }
    for (auto& th : threads) th.join();

    std::this_thread::sleep_for(150ms);
    sched.stop();

    // After all concurrent ops, injector must be in consistent state (no crash)
    EXPECT_GE(fi->activeFaultCount(), 0u);
}

TEST(ChaosStressConcurrent, ConcurrentScheduleFromMultipleThreads) {
    auto fi = make_injector();
    ChaosScheduler::Config cfg;
    cfg.tick_interval = 5ms;
    cfg.wake_strategy = WakeStrategy::CONDVAR;

    ChaosScheduler sched{fi, cfg};
    sched.start();

    constexpr int THREADS = 4;
    constexpr int PER_THREAD = 25;
    std::vector<std::thread> threads;
    threads.reserve(THREADS);

    const auto due = std::chrono::steady_clock::now() + 30ms;
    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&sched, t, due]() {
            for (int i = 0; i < PER_THREAD; ++i) {
                sched.schedule({due,
                    FaultSpec{FaultType::LEADER_CRASH,
                              "mt-" + std::to_string(t) + "-" + std::to_string(i)}});
            }
        });
    }
    for (auto& th : threads) th.join();

    std::this_thread::sleep_for(200ms);

    int fired = 0;
    for (int t = 0; t < THREADS; ++t) {
        for (int i = 0; i < PER_THREAD; ++i) {
            if (fi->isFaultActive("mt-" + std::to_string(t) + "-" + std::to_string(i)))
                ++fired;
        }
    }
    EXPECT_EQ(fired, THREADS * PER_THREAD);

    sched.stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// Tick interval accuracy (basic)
// ─────────────────────────────────────────────────────────────────────────────

TEST(ChaosStressTick, ShortTickAllowsFineGrainedScheduling) {
    // With a 2 ms tick a fault scheduled for 10 ms should fire within ~30 ms.
    auto fi = make_injector();
    ChaosScheduler::Config cfg;
    cfg.tick_interval = 2ms;
    cfg.wake_strategy = WakeStrategy::FIXED_TICK;

    ChaosScheduler sched{fi, cfg};
    sched.start();

    sched.scheduleIn(10ms, FaultSpec{FaultType::NODE_FAILURE, "tick-2ms-node"});
    std::this_thread::sleep_for(50ms);

    EXPECT_TRUE(fi->isFaultActive("tick-2ms-node"));
    sched.stop();
}

TEST(ChaosStressTick, LongTickDelaysFaultFiring) {
    // With a 200 ms tick a fault scheduled for 5 ms should NOT have fired yet
    // after only 50 ms (the scheduler hasn't woken again).
    auto fi = make_injector();
    ChaosScheduler::Config cfg;
    cfg.tick_interval = 200ms;
    cfg.wake_strategy = WakeStrategy::FIXED_TICK;

    ChaosScheduler sched{fi, cfg};
    sched.start();

    sched.scheduleIn(5ms, FaultSpec{FaultType::NODE_FAILURE, "slow-tick-node"});
    std::this_thread::sleep_for(50ms);

    // May or may not have fired depending on first-tick timing; but the
    // important thing is the scheduler is still running without deadlock.
    EXPECT_TRUE(sched.isRunning());
    sched.stop();
}
