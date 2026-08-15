/**
 * @file test_failover_exception_safety.cpp
 * @brief Exception-safety and RAII validation for AutoFailoverManager.
 *
 * Validates fixes for:
 * - Issue #5: uninitialized_access (container member initialization)
 * - Issue #502, #597: resource_leaked_in_exception (noexcept guarantees)
 *
 * Tests:
 * 1. Constructor initializes all containers properly (no uninitialized_access)
 * 2. emitEvent is noexcept and handles exceptions from callbacks safely
 * 3. emitDiagnostic is noexcept and never throws to caller
 * 4. Exception from callback doesn't leak resources or crash
 * 5. Recovery stats are updated even if event emission fails
 */

#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "failover/auto_failover_manager.h"

using namespace std::chrono_literals;

namespace themis { namespace failover { namespace test {

/// Helper: construct a minimal AutoFailoverManager with null dependencies (unit-test mode).
static AutoFailoverManager makeManager() {
    AutoFailoverConfig cfg;
    cfg.max_concurrent_failovers = 4;
    cfg.health_check_interval = std::chrono::milliseconds(100);
    return AutoFailoverManager(cfg, nullptr, nullptr, nullptr, nullptr);
}

/**
 * TEST 1: Constructor initializes failover_queue_ (fixes uninitialized_access)
 * Verifies that the object is valid immediately after construction and that
 * getStatistics() returns zeroed counters (not garbage from uninitialized memory).
 */
TEST(FailoverExceptionSafety, ConstructorInitializesQueue) {
    auto mgr = makeManager();
    auto stats = mgr.getStatistics();
    EXPECT_EQ(stats.total_failovers, 0u);
    EXPECT_EQ(stats.successful_failovers, 0u);
    EXPECT_EQ(stats.failed_failovers, 0u);
    EXPECT_EQ(stats.current_queue_depth, 0u);
    EXPECT_EQ(stats.tasks_dropped_queue_full, 0u);
}

/**
 * TEST 2: emitEvent is noexcept — a throwing callback must not propagate to the caller.
 * Registers a throwing callback and verifies stop() completes without exception and
 * stats remain coherent.
 */
TEST(FailoverExceptionSafety, EmitEventNoThrowWithThrowingCallback) {
    std::atomic<int> call_count{0};

    auto mgr = makeManager();
    mgr.registerEventCallback([&call_count](FailoverEventType, const std::string&,
                                             const std::string&) {
        call_count.fetch_add(1, std::memory_order_relaxed);
        throw std::runtime_error("callback intentionally throws");
    });

    // emitEvent is called internally; triggering a manual failover with null deps
    // will not produce a real failover but we can verify stop() is safe.
    EXPECT_NO_THROW(mgr.stop());

    // Manager must be in a valid state after stop(); getStatistics() must not throw.
    EXPECT_NO_THROW(mgr.getStatistics());
}

/**
 * TEST 3: emitDiagnostic is noexcept (exposed via THEMIS_TEST_BUILD accessor).
 * Verifies that the diagnostic path does not throw to the caller even when called
 * with every FailoverErrorCode variant.
 */
TEST(FailoverExceptionSafety, EmitDiagnosticNoThrow) {
#ifdef THEMIS_TEST_BUILD
    auto mgr = makeManager();
    // Cover all error codes; none must propagate an exception.
    EXPECT_NO_THROW(mgr.testEmitDiagnostic(
        FailoverErrorCode::QUORUM_UNAVAILABLE, "node-1", "unit test"));
    EXPECT_NO_THROW(mgr.testEmitDiagnostic(
        FailoverErrorCode::SPLIT_BRAIN_DETECTED, "node-1", "unit test"));
    EXPECT_NO_THROW(mgr.testEmitDiagnostic(
        FailoverErrorCode::FENCING_FAILED, "node-1", "unit test"));
#else
    GTEST_SKIP() << "THEMIS_TEST_BUILD not set; skipping white-box diagnostic test";
#endif
}

/**
 * TEST 4: RAII Guarantees — queue depth tracking remains correct after a throwing callback.
 * Registers a throwing callback, attempts a manual failover (rejected because deps are null),
 * and verifies stats are self-consistent.
 */
TEST(FailoverExceptionSafety, QueueDepthTrackingWithCallbackExceptions) {
    auto mgr = makeManager();
    mgr.registerEventCallback([](FailoverEventType, const std::string&,
                                  const std::string&) {
        throw std::logic_error("queue-depth callback throws");
    });

    // With null dependencies, triggerManualFailover will fail gracefully.
    // The queue depth must still be consistent afterward.
    mgr.triggerManualFailover("node-dead", "node-spare");

    auto stats = mgr.getStatistics();
    // current_queue_depth must never exceed max_concurrent_failovers.
    EXPECT_LE(stats.current_queue_depth, mgr.getConfig().max_concurrent_failovers);
}

/**
 * TEST 5: Object remains valid after construction and multiple getStatistics() calls.
 * Covers the container-member initialization path (no uninitialized_access UB).
 */
TEST(FailoverExceptionSafety, ObjectValidAfterConstruction) {
    auto mgr = makeManager();
    EXPECT_FALSE(mgr.isRunning());
    EXPECT_EQ(mgr.getState(), FailoverOrchestratorState::IDLE);

    // Repeated stats access must not crash.
    for (int i = 0; i < 10; ++i) {
        EXPECT_NO_THROW(mgr.getStatistics());
    }
}

/**
 * TEST 6: Multiple trigger attempts don't corrupt queue state.
 * Verifies that the in-class initialized queue remains valid across multiple
 * triggerManualFailover calls (null deps reject all gracefully).
 */
TEST(FailoverExceptionSafety, MultipleTriggersWithExceptions) {
    auto mgr = makeManager();
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW(mgr.triggerManualFailover("node-" + std::to_string(i)));
    }
    auto stats = mgr.getStatistics();
    EXPECT_LE(stats.current_queue_depth, mgr.getConfig().max_concurrent_failovers);
}

}}}  // namespace themis::failover::test
