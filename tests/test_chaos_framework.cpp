/**
 * @file test_chaos_framework.cpp
 * @brief Focused unit tests for the ChaosFramework (Phase 4.3)
 *
 * Covers:
 *  - FaultInjector lifecycle and ID
 *  - Single fault injection and active-check
 *  - Fault recovery (all types and typed)
 *  - Expired fault pruning
 *  - Multiple faults on the same node
 *  - Event callbacks on inject and recover
 *  - activeFaultCount / getActiveFaults snapshot
 *  - clearAllFaults
 *  - ChaosScheduler lifecycle (start/stop/isRunning)
 *  - scheduleIn fires fault via injector
 *  - pendingCount / clearPending
 *  - ChaosScheduler rejects null injector
 */

#include <gtest/gtest.h>

#include "chaos/chaos_framework.h"

#include <atomic>
#include <chrono>
#include <thread>

using namespace themis::chaos;
using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// FaultInjector — basic lifecycle
// ─────────────────────────────────────────────────────────────────────────────

TEST(FaultInjectorTest, DefaultIdIsDefault) {
    FaultInjector fi;
    EXPECT_EQ(fi.id(), "default");
}

TEST(FaultInjectorTest, CustomIdIsPreserved) {
    FaultInjector fi{"cluster-fi-1"};
    EXPECT_EQ(fi.id(), "cluster-fi-1");
}

TEST(FaultInjectorTest, StartsWithZeroFaults) {
    FaultInjector fi;
    EXPECT_EQ(fi.activeFaultCount(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// FaultInjector — inject + query
// ─────────────────────────────────────────────────────────────────────────────

TEST(FaultInjectorTest, InjectNodeFailureIsActive) {
    FaultInjector fi;
    FaultSpec spec{FaultType::NODE_FAILURE, "node-1"};
    EXPECT_TRUE(fi.injectFault(spec));
    EXPECT_TRUE(fi.isFaultActive("node-1"));
    EXPECT_TRUE(fi.isFaultActive("node-1", FaultType::NODE_FAILURE));
}

TEST(FaultInjectorTest, InjectedFaultCountIsOne) {
    FaultInjector fi;
    fi.injectFault(FaultSpec{FaultType::NETWORK_PARTITION, "node-2"});
    EXPECT_EQ(fi.activeFaultCount(), 1u);
}

TEST(FaultInjectorTest, TwoDifferentNodesAreBothActive) {
    FaultInjector fi;
    fi.injectFault(FaultSpec{FaultType::NODE_FAILURE, "n1"});
    fi.injectFault(FaultSpec{FaultType::DISK_FAILURE, "n2"});
    EXPECT_TRUE(fi.isFaultActive("n1"));
    EXPECT_TRUE(fi.isFaultActive("n2"));
    EXPECT_EQ(fi.activeFaultCount(), 2u);
}

TEST(FaultInjectorTest, SameNodeTwoDifferentTypesAreBothActive) {
    FaultInjector fi;
    fi.injectFault(FaultSpec{FaultType::NODE_FAILURE, "n1"});
    fi.injectFault(FaultSpec{FaultType::DISK_FAILURE, "n1"});
    EXPECT_TRUE(fi.isFaultActive("n1", FaultType::NODE_FAILURE));
    EXPECT_TRUE(fi.isFaultActive("n1", FaultType::DISK_FAILURE));
    EXPECT_EQ(fi.activeFaultCount(), 2u);
}

TEST(FaultInjectorTest, UnaffectedNodeIsNotActive) {
    FaultInjector fi;
    fi.injectFault(FaultSpec{FaultType::NODE_FAILURE, "n1"});
    EXPECT_FALSE(fi.isFaultActive("n2"));
}

TEST(FaultInjectorTest, EmptyNodeIdIsRejected) {
    FaultInjector fi;
    EXPECT_FALSE(fi.injectFault(FaultSpec{FaultType::NODE_FAILURE, ""}));
    EXPECT_EQ(fi.activeFaultCount(), 0u);
}

TEST(FaultInjectorTest, InvalidProbabilityIsRejected) {
    FaultInjector fi;
    FaultSpec spec{FaultType::RANDOM_FAILURE, "n1", 0ms, 1.5};
    EXPECT_FALSE(fi.injectFault(spec));
}

// ─────────────────────────────────────────────────────────────────────────────
// FaultInjector — expiry
// ─────────────────────────────────────────────────────────────────────────────

TEST(FaultInjectorTest, ExpiredFaultIsNotActive) {
    FaultInjector fi;
    // Inject with 1ms duration — will expire immediately
    fi.injectFault(FaultSpec{FaultType::NODE_FAILURE, "n-expire", 1ms});
    std::this_thread::sleep_for(10ms);
    EXPECT_FALSE(fi.isFaultActive("n-expire"));
}

TEST(FaultInjectorTest, PermanentFaultRemainsActive) {
    FaultInjector fi;
    fi.injectFault(FaultSpec{FaultType::NODE_FAILURE, "n-perm"});  // duration=0 → permanent
    std::this_thread::sleep_for(5ms);
    EXPECT_TRUE(fi.isFaultActive("n-perm"));
}

// ─────────────────────────────────────────────────────────────────────────────
// FaultInjector — recovery
// ─────────────────────────────────────────────────────────────────────────────

TEST(FaultInjectorTest, RecoverAllFaultsOnNode) {
    FaultInjector fi;
    fi.injectFault(FaultSpec{FaultType::NODE_FAILURE, "n1"});
    fi.injectFault(FaultSpec{FaultType::DISK_FAILURE, "n1"});
    EXPECT_TRUE(fi.recoverFault("n1"));
    EXPECT_FALSE(fi.isFaultActive("n1"));
    EXPECT_EQ(fi.activeFaultCount(), 0u);
}

TEST(FaultInjectorTest, RecoverSpecificFaultType) {
    FaultInjector fi;
    fi.injectFault(FaultSpec{FaultType::NODE_FAILURE, "n1"});
    fi.injectFault(FaultSpec{FaultType::DISK_FAILURE, "n1"});
    EXPECT_TRUE(fi.recoverFault("n1", FaultType::DISK_FAILURE));
    EXPECT_TRUE(fi.isFaultActive("n1", FaultType::NODE_FAILURE));
    EXPECT_FALSE(fi.isFaultActive("n1", FaultType::DISK_FAILURE));
}

TEST(FaultInjectorTest, RecoverNonexistentFaultReturnsFalse) {
    FaultInjector fi;
    EXPECT_FALSE(fi.recoverFault("nonexistent"));
    EXPECT_FALSE(fi.recoverFault("nonexistent", FaultType::NODE_FAILURE));
}

// ─────────────────────────────────────────────────────────────────────────────
// FaultInjector — snapshot + clear
// ─────────────────────────────────────────────────────────────────────────────

TEST(FaultInjectorTest, GetActiveFaultsSnapshotSize) {
    FaultInjector fi;
    fi.injectFault(FaultSpec{FaultType::NODE_FAILURE, "n1"});
    fi.injectFault(FaultSpec{FaultType::NETWORK_PARTITION, "n2"});
    const auto faults = fi.getActiveFaults();
    EXPECT_EQ(faults.size(), 2u);
}

TEST(FaultInjectorTest, ClearAllFaultsEmptiesRegistry) {
    FaultInjector fi;
    fi.injectFault(FaultSpec{FaultType::NODE_FAILURE, "n1"});
    fi.injectFault(FaultSpec{FaultType::DISK_FAILURE, "n2"});
    fi.clearAllFaults();
    EXPECT_EQ(fi.activeFaultCount(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// FaultInjector — event callbacks
// ─────────────────────────────────────────────────────────────────────────────

TEST(FaultInjectorTest, CallbackFiredOnInject) {
    FaultInjector fi;
    std::atomic<int> inject_count{0};
    fi.registerEventCallback([&](const FaultSpec&, bool injected) {
        if (injected) inject_count.fetch_add(1, std::memory_order_relaxed);
    });
    fi.injectFault(FaultSpec{FaultType::NODE_FAILURE, "n1"});
    fi.injectFault(FaultSpec{FaultType::DISK_FAILURE, "n2"});
    EXPECT_EQ(inject_count.load(), 2);
}

TEST(FaultInjectorTest, CallbackFiredOnRecover) {
    FaultInjector fi;
    std::atomic<int> recover_count{0};
    fi.registerEventCallback([&](const FaultSpec&, bool injected) {
        if (!injected) recover_count.fetch_add(1, std::memory_order_relaxed);
    });
    fi.injectFault(FaultSpec{FaultType::NODE_FAILURE, "n1"});
    fi.recoverFault("n1");
    EXPECT_GE(recover_count.load(), 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// ChaosScheduler — lifecycle
// ─────────────────────────────────────────────────────────────────────────────

TEST(ChaosSchedulerTest, NullInjectorThrows) {
    EXPECT_THROW({ ChaosScheduler{nullptr}; }, std::invalid_argument);
}

TEST(ChaosSchedulerTest, StartsNotRunning) {
    auto fi = std::make_shared<FaultInjector>();
    ChaosScheduler sched{fi};
    EXPECT_FALSE(sched.isRunning());
}

TEST(ChaosSchedulerTest, StartAndStopLifecycle) {
    auto fi = std::make_shared<FaultInjector>();
    ChaosScheduler sched{fi};
    sched.start();
    EXPECT_TRUE(sched.isRunning());
    sched.stop();
    EXPECT_FALSE(sched.isRunning());
}

TEST(ChaosSchedulerTest, DoubleStartIsIdempotent) {
    auto fi = std::make_shared<FaultInjector>();
    ChaosScheduler sched{fi};
    sched.start();
    sched.start();  // second start — must not deadlock or crash
    EXPECT_TRUE(sched.isRunning());
    sched.stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// ChaosScheduler — scheduling
// ─────────────────────────────────────────────────────────────────────────────

TEST(ChaosSchedulerTest, ScheduledFaultFiresViaInjector) {
    auto fi = std::make_shared<FaultInjector>();
    ChaosScheduler sched{fi};
    sched.start();

    sched.scheduleIn(20ms, FaultSpec{FaultType::NODE_FAILURE, "n-sched"});
    // Not yet fired
    std::this_thread::sleep_for(5ms);
    EXPECT_FALSE(fi->isFaultActive("n-sched"));

    // Now wait for scheduler to pick it up
    std::this_thread::sleep_for(50ms);
    EXPECT_TRUE(fi->isFaultActive("n-sched"));

    sched.stop();
}

TEST(ChaosSchedulerTest, PendingCountReflectsScheduledEntries) {
    auto fi = std::make_shared<FaultInjector>();
    ChaosScheduler sched{fi};

    // Schedule two far-future faults (won't fire during test)
    sched.schedule({std::chrono::steady_clock::now() + 60s,
                    FaultSpec{FaultType::NODE_FAILURE, "n1"}});
    sched.schedule({std::chrono::steady_clock::now() + 60s,
                    FaultSpec{FaultType::DISK_FAILURE, "n2"}});
    EXPECT_EQ(sched.pendingCount(), 2u);
}

TEST(ChaosSchedulerTest, ClearPendingRemovesAllEntries) {
    auto fi = std::make_shared<FaultInjector>();
    ChaosScheduler sched{fi};
    sched.schedule({std::chrono::steady_clock::now() + 60s,
                    FaultSpec{FaultType::NODE_FAILURE, "n1"}});
    sched.clearPending();
    EXPECT_EQ(sched.pendingCount(), 0u);
}

TEST(ChaosSchedulerTest, StopDoesNotFirePendingFaults) {
    auto fi = std::make_shared<FaultInjector>();
    ChaosScheduler sched{fi};
    sched.start();

    // Schedule far-future fault
    sched.schedule({std::chrono::steady_clock::now() + 60s,
                    FaultSpec{FaultType::NODE_FAILURE, "n-pending"}});
    sched.stop();

    EXPECT_FALSE(fi->isFaultActive("n-pending"));
}
