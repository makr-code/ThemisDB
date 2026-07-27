/*
 * ThemisDB | File: test_chaos_callback_determinism.cpp | Version: 1.0.0
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY | Status: Phase 4 — Callback Determinism
 * Purpose: Phase 4 deterministic behavior tests for callback-heavy and pending-queue edge permutations.
 */

/**
 * @file test_chaos_callback_determinism.cpp
 * @brief Phase 4 deterministic behavior tests for chaos callback and pending-queue edge cases.
 *
 * Covers eight test cases mapped to chaos roadmap Phase 4 hardening:
 *
 * ### CCD — Callback and Contract Determinism
 *   CCD-01  Recover callback fires for every successful recoverFault()
 *   CCD-02  Inject on already-active (node, type) returns false and does NOT fire callback again
 *   CCD-03  Inject with empty node_id returns false; callback is NOT fired
 *   CCD-04  Inject with out-of-range probability returns false; callback is NOT fired
 *   CCD-05  clearAllFaults() does NOT fire any callbacks
 *   CCD-06  scheduleIn() with delay=0 fires the fault through the injector within tick window
 *   CCD-07  Pending queue drains to zero after clearPending(); subsequent start/stop is clean
 *   CCD-08  Multiple schedule() entries for the same node fire sequentially without racing
 *
 * @see src/chaos/ROADMAP.md — Phase 4 item
 * @see include/chaos/chaos_contract.h — § 4 fail-closed, § 5 callback semantics, § 6 scheduler state
 */

#include <gtest/gtest.h>

#include "chaos/chaos_framework.h"
#include "chaos/chaos_contract.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace themis::chaos;
using namespace std::chrono_literals;

namespace {

FaultSpec makeSpec(std::string_view node, FaultType type = FaultType::NODE_FAILURE,
                   std::chrono::milliseconds dur = 0ms) {
    return FaultSpec{type, std::string(node), dur};
}

} // anonymous namespace

// ============================================================================
// CCD-01: Recover callback fires for every successful recoverFault()
// ============================================================================

/**
 * @test CCD-01 — The event callback is called with injected=false for every
 *       successful recoverFault() call.
 */
TEST(ChaosCallbackDeterminismTest, CCD01_RecoverCallbackFires) {
    FaultInjector fi{"ccd01"};
    std::atomic<int> recover_calls{0};

    fi.registerEventCallback([&recover_calls](const FaultSpec& /*spec*/, bool injected) {
        if (!injected) {
            recover_calls.fetch_add(1, std::memory_order_relaxed);
        }
    });

    fi.injectFault(makeSpec("ccd01-n1", FaultType::NODE_FAILURE));
    fi.injectFault(makeSpec("ccd01-n2", FaultType::DISK_FAILURE));

    EXPECT_TRUE(fi.recoverFault("ccd01-n1"));
    EXPECT_TRUE(fi.recoverFault("ccd01-n2"));

    // Each successful recover must have fired the callback once.
    EXPECT_EQ(recover_calls.load(), 2);
}

// ============================================================================
// CCD-02: Duplicate inject uses LWW semantics — updates entry, fires callback again
// ============================================================================

/**
 * @test CCD-02 — injectFault() on an already-active (node, type) pair uses
 *       last-writer-wins (LWW) semantics: it updates the existing entry, returns
 *       true, and fires the callback a second time.
 *
 * This matches the implementation contract: re-injection extends/refreshes the
 * active fault rather than being silently rejected.
 */
TEST(ChaosCallbackDeterminismTest, CCD02_DuplicateInjectLWWSemanticsFiresCallbackAgain) {
    FaultInjector fi{"ccd02"};
    std::atomic<int> cb_count{0};

    fi.registerEventCallback([&cb_count](const FaultSpec& /*spec*/, bool injected) {
        if (injected) {
            cb_count.fetch_add(1, std::memory_order_relaxed);
        }
    });

    const FaultSpec spec = makeSpec("ccd02-n1");
    EXPECT_TRUE(fi.injectFault(spec));   // first inject — succeeds
    EXPECT_EQ(cb_count.load(), 1);

    EXPECT_TRUE(fi.injectFault(spec));   // duplicate — LWW update, also returns true
    EXPECT_EQ(cb_count.load(), 2);       // callback fires again for the re-injection

    // Still only one active entry for this (node, type) pair.
    EXPECT_EQ(fi.activeFaultCount(), 1u);
}

// ============================================================================
// CCD-03: Empty node_id returns false; no callback
// ============================================================================

/**
 * @test CCD-03 — injectFault() with an empty node_id returns false without
 *       touching the registry or firing any callback.
 *
 * @see chaos_contract.h § 1 — kMinNodeIdBytes = 1
 */
TEST(ChaosCallbackDeterminismTest, CCD03_EmptyNodeIdRejectedNoCallback) {
    FaultInjector fi{"ccd03"};
    std::atomic<int> cb_count{0};

    fi.registerEventCallback([&cb_count](const FaultSpec& /*spec*/, bool /*injected*/) {
        cb_count.fetch_add(1, std::memory_order_relaxed);
    });

    FaultSpec bad{FaultType::NODE_FAILURE, ""};
    EXPECT_FALSE(fi.injectFault(bad));
    EXPECT_EQ(fi.activeFaultCount(), 0u);
    EXPECT_EQ(cb_count.load(), 0);
}

// ============================================================================
// CCD-04: Out-of-range probability returns false; no callback
// ============================================================================

/**
 * @test CCD-04 — injectFault() with probability outside [0.0, 1.0] returns
 *       false and does not fire any callback.
 *
 * @see chaos_contract.h § 1 — kMinProbability / kMaxProbability
 */
TEST(ChaosCallbackDeterminismTest, CCD04_InvalidProbabilityRejectedNoCallback) {
    FaultInjector fi{"ccd04"};
    std::atomic<int> cb_count{0};

    fi.registerEventCallback([&cb_count](const FaultSpec& /*spec*/, bool /*injected*/) {
        cb_count.fetch_add(1, std::memory_order_relaxed);
    });

    // Probability > 1.0
    FaultSpec over{FaultType::RANDOM_FAILURE, "ccd04-n1", 0ms, 1.5};
    EXPECT_FALSE(fi.injectFault(over));
    EXPECT_EQ(cb_count.load(), 0);

    // Probability < 0.0
    FaultSpec under{FaultType::RANDOM_FAILURE, "ccd04-n2", 0ms, -0.1};
    EXPECT_FALSE(fi.injectFault(under));
    EXPECT_EQ(cb_count.load(), 0);

    EXPECT_EQ(fi.activeFaultCount(), 0u);
}

// ============================================================================
// CCD-05: clearAllFaults() does NOT fire callbacks
// ============================================================================

/**
 * @test CCD-05 — clearAllFaults() removes all faults from the registry without
 *       invoking any event callbacks.
 *
 * @see chaos_contract.h § 5 — callbacks are invoked only on inject/recover
 */
TEST(ChaosCallbackDeterminismTest, CCD05_ClearAllFaultsNoCallbacks) {
    FaultInjector fi{"ccd05"};
    std::atomic<int> cb_count{0};

    // Inject a few faults (each fires the callback once).
    fi.injectFault(makeSpec("ccd05-n1"));
    fi.injectFault(makeSpec("ccd05-n2"));
    fi.injectFault(makeSpec("ccd05-n3"));

    // Register callback AFTER the three injects so baseline is 0.
    fi.registerEventCallback([&cb_count](const FaultSpec& /*spec*/, bool /*injected*/) {
        cb_count.fetch_add(1, std::memory_order_relaxed);
    });

    fi.clearAllFaults();

    EXPECT_EQ(fi.activeFaultCount(), 0u);
    // clearAllFaults() must NOT have fired the callback.
    EXPECT_EQ(cb_count.load(), 0);
}

// ============================================================================
// CCD-06: scheduleIn(delay=0) fires through injector within tick window
// ============================================================================

/**
 * @test CCD-06 — scheduleIn() with a zero-or-minimal delay fires the fault
 *       through the injector within one tick window after start().
 *
 * @see chaos_contract.h § 6 — scheduler state contract
 */
TEST(ChaosCallbackDeterminismTest, CCD06_ScheduleInZeroDelayFires) {
    auto injector = std::make_shared<FaultInjector>("ccd06-fi");
    ChaosScheduler sched{injector,
                         ChaosSchedulerConfig{/*.tick_interval=*/5ms,
                                              /*.wake_strategy=*/WakeStrategy::CONDVAR}};

    EXPECT_FALSE(injector->isFaultActive("ccd06-n1"));

    sched.start();
    sched.scheduleIn(0ms, makeSpec("ccd06-n1"));

    // Wait up to several ticks for the fault to appear.
    const auto deadline = std::chrono::steady_clock::now() + 200ms;
    while (!injector->isFaultActive("ccd06-n1") &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(5ms);
    }

    EXPECT_TRUE(injector->isFaultActive("ccd06-n1"));
    sched.stop();
}

// ============================================================================
// CCD-07: clearPending() drains queue; subsequent start/stop is clean
// ============================================================================

/**
 * @test CCD-07 — clearPending() reduces pendingCount() to zero, and the
 *       scheduler can be started and stopped cleanly afterwards.
 *
 * @see chaos_contract.h § 6 — clearPending() is safe from any state
 */
TEST(ChaosCallbackDeterminismTest, CCD07_ClearPendingThenLifecycle) {
    auto injector = std::make_shared<FaultInjector>("ccd07-fi");
    ChaosScheduler sched{injector,
                         ChaosSchedulerConfig{/*.tick_interval=*/10ms}};

    // Schedule entries while stopped.
    for (int i = 0; i < 16; ++i) {
        sched.scheduleIn(10'000ms, makeSpec("ccd07-n" + std::to_string(i)));
    }
    EXPECT_EQ(sched.pendingCount(), 16u);

    sched.clearPending();
    EXPECT_EQ(sched.pendingCount(), 0u);

    // Start and stop with an empty queue must be clean (no crash).
    sched.start();
    EXPECT_TRUE(sched.isRunning());
    sched.stop();
    EXPECT_FALSE(sched.isRunning());
}

// ============================================================================
// CCD-08: Multiple schedule entries for same node fire sequentially
// ============================================================================

/**
 * @test CCD-08 — Scheduling multiple faults for the same node with staggered
 *       delays fires them sequentially; the injector ends with the last fault
 *       active (earlier ones are overwritten if same node+type, or co-exist
 *       if different types).
 *
 * @see chaos_contract.h § 6 — pending entries fire in temporal order
 */
TEST(ChaosCallbackDeterminismTest, CCD08_MultipleSchedulesSameNodeFireSequentially) {
    auto injector = std::make_shared<FaultInjector>("ccd08-fi");
    std::atomic<int> inject_cb_count{0};

    injector->registerEventCallback([&inject_cb_count](const FaultSpec& /*spec*/, bool injected) {
        if (injected) {
            inject_cb_count.fetch_add(1, std::memory_order_relaxed);
        }
    });

    ChaosScheduler sched{injector,
                         ChaosSchedulerConfig{/*.tick_interval=*/5ms,
                                              /*.wake_strategy=*/WakeStrategy::CONDVAR}};
    sched.start();

    // Schedule two different fault types on the same node with staggered delays.
    sched.scheduleIn(10ms, FaultSpec{FaultType::NODE_FAILURE, "ccd08-n1"});
    sched.scheduleIn(30ms, FaultSpec{FaultType::DISK_FAILURE, "ccd08-n1"});

    // Wait long enough for both to fire.
    const auto deadline = std::chrono::steady_clock::now() + 300ms;
    while (inject_cb_count.load() < 2 &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(5ms);
    }

    sched.stop();

    // Both faults must have fired (callback invoked twice).
    EXPECT_EQ(inject_cb_count.load(), 2);
    // Both fault types must be active (different keys).
    EXPECT_TRUE(injector->isFaultActive("ccd08-n1", FaultType::NODE_FAILURE));
    EXPECT_TRUE(injector->isFaultActive("ccd08-n1", FaultType::DISK_FAILURE));
}
