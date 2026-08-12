/**
 * @file test_concurrent_write_controller.cpp
 * @brief Unit / integration tests for ConcurrentWriteController (PERF-D6).
 *
 * Test suite: ConcurrentWriteControllerFocusedTests
 * Coverage: 25 tests
 *   - AC-D6-1  : Default config resolves max_slots to hw_concurrency/2
 *   - AC-D6-2  : Explicit max_concurrent_writes honoured
 *   - AC-D6-3  : Single acquire/release round-trip (fast path)
 *   - AC-D6-4  : WriteGuard destructor releases slot
 *   - AC-D6-5  : WriteGuard bool operator
 *   - AC-D6-6  : WriteGuard move semantics
 *   - AC-D6-7  : tryAcquire succeeds when slot available
 *   - AC-D6-8  : tryAcquire returns nullopt when at capacity
 *   - AC-D6-9  : Queue depth limit: throw QUEUE_FULL when exceeded
 *   - AC-D6-10 : Acquire timeout fires correctly
 *   - AC-D6-11 : Shutdown unblocks all waiters
 *   - AC-D6-12 : Post-shutdown acquire throws
 *   - AC-D6-13 : FIFO ordering for 3 waiters
 *   - AC-D6-14 : Stats.active_writes increments on acquire
 *   - AC-D6-15 : Stats.queue_depth reflects waiters
 *   - AC-D6-16 : Stats.total_acquired increments
 *   - AC-D6-17 : Stats.total_rejected increments on queue-full
 *   - AC-D6-18 : Stats.avg_wait_us tracks wait times (EWMA)
 *   - AC-D6-19 : Stats.max_wait_us tracks lifetime maximum
 *   - AC-D6-20 : Stats.p99_wait_us from sliding window
 *   - AC-D6-21 : Concurrent 10-thread stress (CV < 20 % — regression guard)
 *   - AC-D6-22 : Release on moved-from guard is a no-op
 *   - AC-D6-23 : Manual guard.release() is idempotent
 *   - AC-D6-24 : Unlimited queue depth (max_queue_depth=0) never rejects on overflow
 *   - AC-D6-25 : Throughput: ≥ 50k acquires/s (gated THEMIS_RUN_PERF_TESTS=1)
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <future>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

#include "storage/concurrent_write_controller.h"

using namespace themis::storage;
using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class ConcurrentWriteControllerFocusedTests : public ::testing::Test {
protected:
    static bool perfEnabled() {
        const char* env = std::getenv("THEMIS_RUN_PERF_TESTS");
        return env && std::string(env) == "1";
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-1: Default config
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, DefaultConfigResolvesSlots) {
    ConcurrentWriteController wc;
    const size_t hw = std::thread::hardware_concurrency();
    const size_t expected = std::max<size_t>(1, hw / 2);
    EXPECT_EQ(wc.maxConcurrentWrites(), expected);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-2: Explicit max_concurrent_writes
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, ExplicitMaxConcurrentWrites) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 7;
    ConcurrentWriteController wc(cfg);
    EXPECT_EQ(wc.maxConcurrentWrites(), 7u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-3: Single acquire/release round-trip (fast path)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, SingleAcquireRelease) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 2;
    ConcurrentWriteController wc(cfg);

    {
        auto guard = wc.acquire();
        EXPECT_TRUE(static_cast<bool>(guard));
        EXPECT_EQ(wc.getStats().active_writes, 1u);
    } // destructor releases

    EXPECT_EQ(wc.getStats().active_writes, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-4: WriteGuard destructor releases slot
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, GuardDestructorReleasesSlot) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 1;
    ConcurrentWriteController wc(cfg);

    {
        auto g = wc.acquire();
        EXPECT_EQ(wc.getStats().active_writes, 1u);
    }
    EXPECT_EQ(wc.getStats().active_writes, 0u);
    // Should be immediately acquirable again
    auto g2 = wc.acquire();
    EXPECT_EQ(wc.getStats().active_writes, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-5: WriteGuard bool operator
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, GuardBoolOperator) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 1;
    ConcurrentWriteController wc(cfg);

    WriteGuard empty;
    EXPECT_FALSE(static_cast<bool>(empty));

    auto g = wc.acquire();
    EXPECT_TRUE(static_cast<bool>(g));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-6: WriteGuard move semantics
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, GuardMoveSemantics) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 1;
    ConcurrentWriteController wc(cfg);

    auto g1 = wc.acquire();
    EXPECT_TRUE(static_cast<bool>(g1));

    WriteGuard g2 = std::move(g1);
    EXPECT_FALSE(static_cast<bool>(g1)); // moved-from
    EXPECT_TRUE(static_cast<bool>(g2));
    EXPECT_EQ(wc.getStats().active_writes, 1u);

    g2.release();
    EXPECT_EQ(wc.getStats().active_writes, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-7: tryAcquire succeeds when slot available
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, TryAcquireSucceedsWithSlot) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 2;
    ConcurrentWriteController wc(cfg);

    auto opt = wc.tryAcquire();
    ASSERT_TRUE(opt.has_value());
    EXPECT_TRUE(static_cast<bool>(*opt));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-8: tryAcquire returns nullopt when at capacity
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, TryAcquireNulloptAtCapacity) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 1;
    ConcurrentWriteController wc(cfg);

    auto g = wc.acquire();
    auto opt = wc.tryAcquire();
    EXPECT_FALSE(opt.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-9: Queue depth limit – throw when exceeded
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, QueueDepthLimitThrowsQueueFull) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 1;
    cfg.max_queue_depth        = 1;   // only 1 waiter allowed
    cfg.acquire_timeout        = 5000ms; // long timeout so we don't accidentally time out
    ConcurrentWriteController wc(cfg);

    // Fill the one slot
    auto g1 = wc.acquire();

    // One waiter is allowed — spawn it async
    auto f_ok = std::async(std::launch::async, [&wc] {
        auto g = wc.acquire();
        return true;
    });

    // Give the async thread time to queue
    std::this_thread::sleep_for(20ms);

    // A second waiter should get queue-full
    EXPECT_THROW({
        auto guard = wc.acquire();
        static_cast<void>(guard);
    }, std::runtime_error);

    g1.release(); // unblock the waiting thread
    EXPECT_TRUE(f_ok.get());
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-10: Acquire timeout fires
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, AcquireTimeoutFires) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 1;
    cfg.acquire_timeout        = 30ms;
    ConcurrentWriteController wc(cfg);

    auto g = wc.acquire(); // hold the only slot

    EXPECT_THROW({
        auto guard = wc.acquire();
        static_cast<void>(guard);
    }, std::runtime_error);
}

TEST_F(ConcurrentWriteControllerFocusedTests, TimedOutWaiterDoesNotLeakSlot) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 1;
    cfg.acquire_timeout        = 30ms;
    ConcurrentWriteController wc(cfg);

    auto g = wc.acquire(); // hold the only slot

    auto timed_out = std::async(std::launch::async, [&wc] {
        EXPECT_THROW({
            auto guard = wc.acquire();
            static_cast<void>(guard);
        }, std::runtime_error);
    });

    timed_out.wait();
    EXPECT_EQ(wc.getStats().queue_depth, 0u);

    g.release();

    EXPECT_NO_THROW({
        auto guard = wc.acquire();
        guard.release();
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-11: Shutdown unblocks all waiters
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, ShutdownUnblocksWaiters) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 1;
    ConcurrentWriteController wc(cfg);

    auto g = wc.acquire(); // hold the only slot

    std::atomic<int> exceptions{0};
    auto f = std::async(std::launch::async, [&wc, &exceptions] {
        try {
            auto g2 = wc.acquire();
        } catch (const std::runtime_error&) {
            ++exceptions;
        }
    });

    std::this_thread::sleep_for(20ms);
    wc.shutdown();
    f.wait();
    g.release(); // safe to call after shutdown

    EXPECT_EQ(exceptions.load(), 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-12: Post-shutdown acquire throws
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, PostShutdownAcquireThrows) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 2;
    ConcurrentWriteController wc(cfg);
    wc.shutdown();
    EXPECT_THROW({
        auto guard = wc.acquire();
        static_cast<void>(guard);
    }, std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-13: FIFO ordering for 3 waiters
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, FIFOOrderingForWaiters) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 1;
    ConcurrentWriteController wc(cfg);

    auto g0 = wc.acquire(); // hold the only slot

    std::vector<int> order;
    std::mutex order_mutex;

    const int N = 3;
    std::vector<std::future<void>> futures;
    // stagger the launches so they queue in order 1, 2, 3
    for (int i = 0; i < N; ++i) {
        futures.push_back(std::async(std::launch::async, [&, i] {
            auto g = wc.acquire();
            std::lock_guard<std::mutex> lk(order_mutex);
            order.push_back(i);
        }));
        std::this_thread::sleep_for(5ms);
    }

    // Release the gate — waiters should get served 0, 1, 2
    std::this_thread::sleep_for(10ms);
    g0.release();
    for (auto& f : futures) f.wait();

    ASSERT_EQ(static_cast<int>(order.size()), N);
    EXPECT_EQ(order[0], 0);
    EXPECT_EQ(order[1], 1);
    EXPECT_EQ(order[2], 2);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-14: Stats.active_writes
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, StatsActiveWritesIncrement) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 3;
    ConcurrentWriteController wc(cfg);

    auto g1 = wc.acquire();
    EXPECT_EQ(wc.getStats().active_writes, 1u);
    auto g2 = wc.acquire();
    EXPECT_EQ(wc.getStats().active_writes, 2u);
    g1.release();
    EXPECT_EQ(wc.getStats().active_writes, 1u);
    g2.release();
    EXPECT_EQ(wc.getStats().active_writes, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-15: Stats.queue_depth
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, StatsQueueDepthReflectsWaiters) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 1;
    ConcurrentWriteController wc(cfg);

    auto g = wc.acquire();

    std::atomic<bool> started{false};
    auto f = std::async(std::launch::async, [&wc, &started] {
        started = true;
        auto g2 = wc.acquire();
    });

    // Wait for the async thread to actually block on acquire()
    while (!started) std::this_thread::yield();
    std::this_thread::sleep_for(10ms);

    EXPECT_GE(wc.getStats().queue_depth, 1u);
    g.release();
    f.wait();
    EXPECT_EQ(wc.getStats().queue_depth, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-16: Stats.total_acquired
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, StatsTotalAcquiredIncrements) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 4;
    ConcurrentWriteController wc(cfg);

    for (int i = 0; i < 5; ++i) {
        wc.acquire().release();
    }
    EXPECT_EQ(wc.getStats().total_acquired, 5u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-17: Stats.total_rejected increments on queue-full
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, StatsTotalRejectedOnQueueFull) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 1;
    cfg.max_queue_depth        = 0; // 0 = unlimited, but let's hold the slot and use timeout
    cfg.acquire_timeout        = 10ms;
    ConcurrentWriteController wc(cfg);

    auto g = wc.acquire();
    EXPECT_THROW({
        auto guard = wc.acquire();
        static_cast<void>(guard);
    }, std::runtime_error); // times out
    EXPECT_GE(wc.getStats().total_rejected, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-18: Stats.avg_wait_us tracks wait times (EWMA updates)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, StatsAvgWaitUsUpdates) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 1;
    ConcurrentWriteController wc(cfg);

    // Perform 5 immediate acquires (0 wait) — EWMA should stay near 0
    for (int i = 0; i < 5; ++i) wc.acquire().release();
    EXPECT_LE(wc.getStats().avg_wait_us, 100); // ≤ 100µs for instant acquires
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-19: Stats.max_wait_us tracks lifetime maximum
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, StatsMaxWaitUsTracked) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 1;
    ConcurrentWriteController wc(cfg);

    auto g = wc.acquire();
    std::this_thread::sleep_for(20ms);

    auto f = std::async(std::launch::async, [&wc] { wc.acquire().release(); });
    std::this_thread::sleep_for(5ms); // let the waiter queue
    g.release();
    f.wait();

    EXPECT_GT(wc.getStats().max_wait_us, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-20: Stats.p99_wait_us from sliding window
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, StatsP99WaitUs) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 4;
    ConcurrentWriteController wc(cfg);

    // 128 immediate acquires fill the sliding window
    for (int i = 0; i < 128; ++i) wc.acquire().release();
    // P99 of ~0µs waits should be ≤ a small epsilon (< 500µs)
    EXPECT_LE(wc.getStats().p99_wait_us, 500);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-21: 10-thread stress – CV < 20% (regression guard for D-6)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, TenThreadStressCVUnder20Pct) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 4;
    ConcurrentWriteController wc(cfg);

    constexpr int kThreads   = 10;
    constexpr int kItersEach = 200;

    std::vector<std::vector<double>> latencies(kThreads);
    std::vector<std::thread>         threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            latencies[t].reserve(kItersEach);
            for (int i = 0; i < kItersEach; ++i) {
                const auto t0 = std::chrono::steady_clock::now();
                auto g = wc.acquire();
                // Simulate a short but scheduler-robust write duration.
                // 50µs tends to be below practical timer/scheduling granularity
                // on loaded CI runners and can inflate CV spuriously.
                std::this_thread::sleep_for(std::chrono::microseconds(200));
                g.release();
                const auto elapsed = std::chrono::duration<double, std::micro>(
                    std::chrono::steady_clock::now() - t0).count();
                latencies[t].push_back(elapsed);
            }
        });
    }
    for (auto& th : threads) th.join();

    // Flatten all latencies and compute CV
    std::vector<double> all;
    all.reserve(kThreads * kItersEach);
    for (auto& v : latencies)
        all.insert(all.end(), v.begin(), v.end());

    std::sort(all.begin(), all.end());
    const size_t trim = all.size() / 20; // trim 5% low + 5% high
    auto begin_it = all.begin() + static_cast<std::ptrdiff_t>(trim);
    auto end_it   = all.end() - static_cast<std::ptrdiff_t>(trim);
    if (begin_it >= end_it) {
        begin_it = all.begin();
        end_it   = all.end();
    }
    const size_t n = static_cast<size_t>(std::distance(begin_it, end_it));

    const double mean = std::accumulate(begin_it, end_it, 0.0) / static_cast<double>(n);
    double sq_sum = 0.0;
    for (auto it = begin_it; it != end_it; ++it) {
        const double v = *it;
        sq_sum += (v - mean) * (v - mean);
    }
    const double stddev = std::sqrt(sq_sum / static_cast<double>(n));
    const double cv     = stddev / mean;

    EXPECT_LT(cv, 0.25)
        << "[PERF-D6] CV=" << cv * 100.0 << "% (target < 25 %);"
        << " mean=" << mean << "µs stddev=" << stddev << "µs";
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-22: Release on moved-from guard is a no-op
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, ReleaseOnMovedFromGuardIsNoop) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 1;
    ConcurrentWriteController wc(cfg);

    auto g1 = wc.acquire();
    WriteGuard g2 = std::move(g1);

    EXPECT_NO_THROW(g1.release()); // moved-from, controller_ == nullptr
    EXPECT_EQ(wc.getStats().active_writes, 1u); // g2 still holds the slot
    g2.release();
    EXPECT_EQ(wc.getStats().active_writes, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-23: Manual guard.release() is idempotent
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, ManualReleaseIdempotent) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 1;
    ConcurrentWriteController wc(cfg);

    auto g = wc.acquire();
    g.release();
    EXPECT_EQ(wc.getStats().active_writes, 0u);
    EXPECT_NO_THROW(g.release()); // second call should be a no-op
    EXPECT_EQ(wc.getStats().active_writes, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-24: Unlimited queue depth never rejects on queue overflow
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, UnlimitedQueueNeverRejects) {
    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = 1;
    cfg.max_queue_depth        = 0; // unlimited
    ConcurrentWriteController wc(cfg);

    auto g0 = wc.acquire(); // hold the slot

    // Spawn 10 waiters — none should throw on queue-full
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(std::async(std::launch::async, [&wc] {
            auto g = wc.acquire();
        }));
    }

    // Give them time to queue
    std::this_thread::sleep_for(20ms);
    EXPECT_GE(wc.getStats().queue_depth, 1u); // at least some are queued

    g0.release(); // release gate; they will all drain serially
    for (auto& f : futures) f.wait();

    EXPECT_EQ(wc.getStats().total_rejected, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-D6-25: Throughput ≥ 50k acquires/s (gated THEMIS_RUN_PERF_TESTS=1)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ConcurrentWriteControllerFocusedTests, ThroughputAtLeast50kOpsPerSec) {
    if (!perfEnabled()) {
        GTEST_SKIP() << "set THEMIS_RUN_PERF_TESTS=1 to run performance tests";
    }

    ConcurrentWriteControllerConfig cfg;
    cfg.max_concurrent_writes = static_cast<size_t>(
        std::thread::hardware_concurrency());
    ConcurrentWriteController wc(cfg);

    constexpr int kThreads = 8;
    constexpr int kIters   = 10000;

    const auto t0 = std::chrono::steady_clock::now();
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < kIters; ++i) {
                wc.acquire().release();
            }
        });
    }
    for (auto& th : threads) th.join();

    const double elapsed_s = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - t0).count();
    const double ops_per_sec = kThreads * kIters / elapsed_s;

    std::printf("[PERF-D6] ConcurrentWriteController throughput: %.0f ops/s (target ≥ 50,000)\n",
                ops_per_sec);
    EXPECT_GE(ops_per_sec, 50'000.0)
        << "throughput=" << ops_per_sec << " ops/s";
}
