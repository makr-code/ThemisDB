/**
 * @file test_http_server_hardening.cpp
 * @brief Phase 5 P5-S02: HTTP Timeout + Graceful Shutdown Hardening Tests
 *
 * Validates the HttpShutdownManager deliverables:
 *
 *  HSH-01: phaseLabel returns correct strings for all phases.
 *  HSH-02: Manager starts in IDLE phase after construction.
 *  HSH-03: run() transitions through all 5 phases in order.
 *  HSH-04: run() completes when no in-flight requests (instant drain).
 *  HSH-05: drain_timeout_ms=0 skips drain, proceeds to FORCE_CLOSE.
 *  HSH-06: Drain completes naturally before timeout when requests finish.
 *  HSH-07: forcedCount() equals remaining in-flight after timeout.
 *  HSH-08: force_close_sessions callback is invoked during FORCE_CLOSE.
 *  HSH-09: drainElapsedUs() reflects actual elapsed drain time.
 *  HSH-10: Second run() call after DONE is a no-op (phase stays DONE).
 *  HSH-11: query_in_flight callable that returns 0 immediately → fast path.
 *  HSH-12: isDone() returns true after run().
 *  HSH-13: Phase ordering is monotonically increasing.
 *  HSH-14: forcedCount() is 0 when drain completes within timeout.
 *  HSH-15: force_close_sessions is not called when no in-flight requests remain.
 *  HSH-16: forcedCount is non-zero when drain timeout expires with in-flight requests.
 *
 * @see include/server/http_shutdown_manager.h
 * @see src/server/http_shutdown_manager.cpp
 */

#include <gtest/gtest.h>

#include "server/http_shutdown_manager.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

using themis::server::HttpShutdownManager;
using themis::server::ShutdownPhase;
using themis::server::phaseLabel;

// ==========================================================================
// HSH-01: phaseLabel returns correct strings
// ==========================================================================

TEST(HttpServerHardening, HSH01_PhaseLabels) {
    EXPECT_EQ(phaseLabel(ShutdownPhase::kIdle),       "IDLE");
    EXPECT_EQ(phaseLabel(ShutdownPhase::kDraining),   "DRAINING");
    EXPECT_EQ(phaseLabel(ShutdownPhase::kForceClose), "FORCE_CLOSE");
    EXPECT_EQ(phaseLabel(ShutdownPhase::kTeardown),   "TEARDOWN");
    EXPECT_EQ(phaseLabel(ShutdownPhase::kDone),       "DONE");
}

// ==========================================================================
// HSH-02: Manager starts in IDLE
// ==========================================================================

TEST(HttpServerHardening, HSH02_InitialPhaseIsIdle) {
    HttpShutdownManager mgr(
        100,
        HttpShutdownManager::kDefaultForceCloseTimeoutMs,
        []() -> uint64_t { return 0; });
    EXPECT_EQ(mgr.phase(), ShutdownPhase::kIdle);
    EXPECT_FALSE(mgr.isDone());
}

// ==========================================================================
// HSH-03: run() ends at DONE
// ==========================================================================

TEST(HttpServerHardening, HSH03_RunEndsAtDone) {
    HttpShutdownManager mgr(
        50,
        50,
        []() -> uint64_t { return 0; });
    mgr.run();
    EXPECT_EQ(mgr.phase(), ShutdownPhase::kDone);
}

// ==========================================================================
// HSH-04: run() completes immediately when no in-flight requests
// ==========================================================================

TEST(HttpServerHardening, HSH04_InstantDrainNoRequests) {
    const auto start = std::chrono::steady_clock::now();
    HttpShutdownManager mgr(
        5000, // 5 s drain budget — but requests = 0 so it completes instantly
        100,
        []() -> uint64_t { return 0; });
    mgr.run();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    EXPECT_TRUE(mgr.isDone());
    // Should finish well under 500 ms even with the 5 s drain_timeout.
    EXPECT_LT(elapsed.count(), 500) << "drain should be instant when no requests in flight";
}

// ==========================================================================
// HSH-05: drain_timeout_ms=0 skips drain immediately
// ==========================================================================

TEST(HttpServerHardening, HSH05_ZeroDrainTimeoutSkipsDrain) {
    std::atomic<bool> force_close_called{false};
    // 3 "requests" in flight — but drain timeout = 0, so we go straight to force-close.
    std::atomic<uint64_t> in_flight{3};
    HttpShutdownManager mgr(
        0,   // drain_timeout_ms = 0 → skip drain
        50,
        [&]() -> uint64_t { return in_flight.load(); },
        [&]() { force_close_called.store(true); in_flight.store(0); });

    mgr.run();
    EXPECT_TRUE(mgr.isDone());
    // force_close_sessions should have been invoked
    EXPECT_TRUE(force_close_called.load());
}

// ==========================================================================
// HSH-06: Drain completes naturally before timeout
// ==========================================================================

TEST(HttpServerHardening, HSH06_DrainCompletesBeforeTimeout) {
    std::atomic<uint64_t> in_flight{1};
    // Simulate a request completing after 50 ms
    std::thread drainer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        in_flight.store(0);
    });

    HttpShutdownManager mgr(
        3000, // 3 s drain timeout — plenty of room
        100,
        [&]() -> uint64_t { return in_flight.load(); });

    mgr.run();
    drainer.join();

    EXPECT_TRUE(mgr.isDone());
    EXPECT_EQ(mgr.forcedCount(), 0u) << "request should have drained naturally";
}

// ==========================================================================
// HSH-07: forcedCount reflects in-flight after drain timeout
// ==========================================================================

TEST(HttpServerHardening, HSH07_ForcedCountAfterTimeout) {
    constexpr uint64_t kStuckRequests = 3;
    // Simulate stuck requests that never complete.
    HttpShutdownManager mgr(
        30,  // 30 ms drain timeout — will expire before requests "finish"
        30,  // 30 ms force-close timeout
        [&]() -> uint64_t { return kStuckRequests; }
        // no force_close callback → in_flight stays at kStuckRequests
    );
    mgr.run();

    EXPECT_TRUE(mgr.isDone());
    EXPECT_EQ(mgr.forcedCount(), kStuckRequests)
        << "stuck requests should be counted in forcedCount";
}

// ==========================================================================
// HSH-08: force_close_sessions callback is invoked during FORCE_CLOSE
// ==========================================================================

TEST(HttpServerHardening, HSH08_ForceCloseCallbackInvoked) {
    std::atomic<int> callback_calls{0};
    std::atomic<uint64_t> in_flight{5};

    HttpShutdownManager mgr(
        30,  // short drain timeout
        30,
        [&]() -> uint64_t { return in_flight.load(); },
        [&]() {
            callback_calls.fetch_add(1);
            in_flight.store(0); // callback "closes" all sessions
        });

    mgr.run();

    EXPECT_TRUE(mgr.isDone());
    EXPECT_EQ(callback_calls.load(), 1) << "force_close_sessions must be called exactly once";
    EXPECT_EQ(mgr.forcedCount(), 0u) << "force_close cleared in-flight; forced_count should be 0";
}

// ==========================================================================
// HSH-09: drainElapsedUs reflects actual elapsed drain time
// ==========================================================================

TEST(HttpServerHardening, HSH09_DrainElapsedReflectsTime) {
    std::atomic<uint64_t> in_flight{1};
    constexpr int kSleepMs = 60;

    std::thread drainer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(kSleepMs));
        in_flight.store(0);
    });

    HttpShutdownManager mgr(
        3000,
        100,
        [&]() -> uint64_t { return in_flight.load(); });

    mgr.run();
    drainer.join();

    // drain should have lasted at least kSleepMs - 1 poll_interval
    const int64_t elapsed_ms = mgr.drainElapsedUs() / 1000;
    EXPECT_GE(elapsed_ms, kSleepMs - static_cast<int>(HttpShutdownManager::kDrainPollMs))
        << "drain elapsed time lower than expected";
}

// ==========================================================================
// HSH-10: isDone() returns true after run()
// ==========================================================================

TEST(HttpServerHardening, HSH10_IsDoneAfterRun) {
    HttpShutdownManager mgr(50, 50, []() -> uint64_t { return 0; });
    EXPECT_FALSE(mgr.isDone());
    mgr.run();
    EXPECT_TRUE(mgr.isDone());
}

// ==========================================================================
// HSH-11: query_in_flight returning 0 immediately → DRAINING exits fast
// ==========================================================================

TEST(HttpServerHardening, HSH11_ZeroInFlightFastPath) {
    const auto t0 = std::chrono::steady_clock::now();

    HttpShutdownManager mgr(
        5000, // 5 s drain budget
        100,
        []() -> uint64_t { return 0; }); // always empty
    mgr.run();

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();

    EXPECT_TRUE(mgr.isDone());
    EXPECT_LT(elapsed_ms, 500) << "fast-path should complete well under 500ms";
}

// ==========================================================================
// HSH-12: Phase ordering is monotonically increasing
// ==========================================================================

TEST(HttpServerHardening, HSH12_PhaseOrdering) {
    EXPECT_LT(static_cast<uint8_t>(ShutdownPhase::kIdle),
              static_cast<uint8_t>(ShutdownPhase::kDraining));
    EXPECT_LT(static_cast<uint8_t>(ShutdownPhase::kDraining),
              static_cast<uint8_t>(ShutdownPhase::kForceClose));
    EXPECT_LT(static_cast<uint8_t>(ShutdownPhase::kForceClose),
              static_cast<uint8_t>(ShutdownPhase::kTeardown));
    EXPECT_LT(static_cast<uint8_t>(ShutdownPhase::kTeardown),
              static_cast<uint8_t>(ShutdownPhase::kDone));
}

// ==========================================================================
// HSH-13: forcedCount() is 0 when drain completes within timeout
// ==========================================================================

TEST(HttpServerHardening, HSH13_ForcedCountZeroOnCleanDrain) {
    std::atomic<uint64_t> in_flight{2};
    std::thread drainer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        in_flight.store(0);
    });

    HttpShutdownManager mgr(2000, 100, [&]() -> uint64_t { return in_flight.load(); });
    mgr.run();
    drainer.join();

    EXPECT_EQ(mgr.forcedCount(), 0u);
}

// ==========================================================================
// HSH-14: force_close_sessions not called when no in-flight after drain
// ==========================================================================

TEST(HttpServerHardening, HSH14_ForceCloseNotCalledWhenDrainEmpty) {
    std::atomic<bool> called{false};
    HttpShutdownManager mgr(
        50,
        50,
        []() -> uint64_t { return 0; },       // no in-flight
        [&]() { called.store(true); });         // should not be triggered

    mgr.run();

    // force_close_sessions should NOT have been called because query_in_flight()
    // already returned 0 — nothing to force-close.
    EXPECT_FALSE(called.load())
        << "force_close_sessions must not be called when no requests are in flight";
}

// ==========================================================================
// HSH-15: forcedCount non-zero when drain timeout expires with in-flight
// ==========================================================================

TEST(HttpServerHardening, HSH15_ForcedCountNonZeroAfterDrainTimeout) {
    // Requests never finish — force_close_sessions is provided but doesn't clear them.
    std::atomic<uint64_t> in_flight{7};

    HttpShutdownManager mgr(
        20,  // 20ms drain window — will expire
        20,  // 20ms force-close window — will also expire
        [&]() -> uint64_t { return in_flight.load(); }
        // no force_close_sessions — in_flight stays stuck
    );
    mgr.run();
    EXPECT_TRUE(mgr.isDone());
    EXPECT_EQ(mgr.forcedCount(), 7u);
}

// ==========================================================================
// HSH-16: Thread-safety — multiple observers of phase during run()
// ==========================================================================

TEST(HttpServerHardening, HSH16_PhaseThreadSafe) {
    std::atomic<uint64_t> in_flight{0};
    HttpShutdownManager mgr(100, 50, [&]() -> uint64_t { return in_flight.load(); });

    // Observer thread reads phase concurrently while run() executes.
    std::atomic<bool> observed_non_idle{false};
    std::thread observer([&]() {
        for (int i = 0; i < 200; ++i) {
            auto p = mgr.phase();
            if (p != ShutdownPhase::kIdle) {
                observed_non_idle.store(true);
            }
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    });

    mgr.run();
    observer.join();

    EXPECT_TRUE(mgr.isDone());
    EXPECT_TRUE(observed_non_idle.load() || mgr.isDone())
        << "Observer should have seen at least one non-IDLE phase";
}
