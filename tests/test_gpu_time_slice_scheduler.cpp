/*
 * Unit tests for GPUTimeSliceScheduler.
 *
 * All tests run on CI without GPU hardware.  The scheduler's dispatch loop
 * calls the supplied BackendFn (or a CPU no-op) synchronously so every
 * scheduling path is exercised without a real device.
 */

#include <gtest/gtest.h>
#include "themis/gpu/time_slice_scheduler.h"

#include <atomic>
#include <chrono>
#include <thread>

using namespace themis::gpu;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static GPULauncher::WorkItem makeItem(const std::string& kernel  = "k1",
                                       const std::string& tenant  = "t1") {
    GPULauncher::WorkItem w;
    w.kernel_id = kernel;
    w.tenant_id = tenant;
    w.tag       = "test";
    return w;
}

static GPUTimeSliceScheduler::TenantConfig makeCfg(const std::string& id,
                                                     uint32_t slice_ms = 100) {
    GPUTimeSliceScheduler::TenantConfig cfg;
    cfg.tenant_id = id;
    cfg.slice_ms  = slice_ms;
    return cfg;
}

// ---------------------------------------------------------------------------
// Fixture — fresh local instance per test (avoids singleton state bleed)
// ---------------------------------------------------------------------------
class GPUTimeSliceSchedulerTest : public ::testing::Test {
protected:
    GPUTimeSliceScheduler sched;
};

// ===========================================================================
// Tenant lifecycle
// ===========================================================================

TEST_F(GPUTimeSliceSchedulerTest, RegisterTenant_SucceedsForNewId) {
    EXPECT_TRUE(sched.registerTenant(makeCfg("t1")));
    EXPECT_TRUE(sched.hasTenant("t1"));
    EXPECT_EQ(sched.tenantCount(), 1u);
}

TEST_F(GPUTimeSliceSchedulerTest, RegisterTenant_FailsForDuplicateId) {
    ASSERT_TRUE(sched.registerTenant(makeCfg("dup")));
    EXPECT_FALSE(sched.registerTenant(makeCfg("dup")));
    EXPECT_EQ(sched.tenantCount(), 1u);
}

TEST_F(GPUTimeSliceSchedulerTest, RegisterTenant_FailsForEmptyId) {
    EXPECT_FALSE(sched.registerTenant(makeCfg("")));
}

TEST_F(GPUTimeSliceSchedulerTest, RegisterTenant_FailsForZeroSlice) {
    EXPECT_FALSE(sched.registerTenant(makeCfg("t_zero", 0)));
}

TEST_F(GPUTimeSliceSchedulerTest, UnregisterTenant_SucceedsForKnownId) {
    sched.registerTenant(makeCfg("t1"));
    EXPECT_TRUE(sched.unregisterTenant("t1"));
    EXPECT_FALSE(sched.hasTenant("t1"));
    EXPECT_EQ(sched.tenantCount(), 0u);
}

TEST_F(GPUTimeSliceSchedulerTest, UnregisterTenant_FailsForUnknownId) {
    EXPECT_FALSE(sched.unregisterTenant("ghost"));
}

TEST_F(GPUTimeSliceSchedulerTest, TenantIds_ReflectsRegistrationOrder) {
    sched.registerTenant(makeCfg("a"));
    sched.registerTenant(makeCfg("b"));
    sched.registerTenant(makeCfg("c"));
    const auto ids = sched.tenantIds();
    ASSERT_EQ(ids.size(), 3u);
    EXPECT_EQ(ids[0], "a");
    EXPECT_EQ(ids[1], "b");
    EXPECT_EQ(ids[2], "c");
}

TEST_F(GPUTimeSliceSchedulerTest, TenantIds_UpdatesAfterUnregister) {
    sched.registerTenant(makeCfg("a"));
    sched.registerTenant(makeCfg("b"));
    sched.unregisterTenant("a");
    const auto ids = sched.tenantIds();
    ASSERT_EQ(ids.size(), 1u);
    EXPECT_EQ(ids[0], "b");
}

// ===========================================================================
// Work submission
// ===========================================================================

TEST_F(GPUTimeSliceSchedulerTest, Submit_SucceedsForRegisteredTenant) {
    sched.registerTenant(makeCfg("t1"));
    EXPECT_TRUE(sched.submit("t1", makeItem()));
    EXPECT_EQ(sched.queueDepth("t1"), 1u);
}

TEST_F(GPUTimeSliceSchedulerTest, Submit_FailsForUnregisteredTenant) {
    EXPECT_FALSE(sched.submit("nobody", makeItem()));
}

TEST_F(GPUTimeSliceSchedulerTest, QueueDepth_ReturnsZeroForUnknownTenant) {
    EXPECT_EQ(sched.queueDepth("ghost"), 0u);
}

TEST_F(GPUTimeSliceSchedulerTest, Submit_MultipleItems_AllQueued) {
    sched.registerTenant(makeCfg("t1"));
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(sched.submit("t1", makeItem("k" + std::to_string(i))));
    }
    EXPECT_EQ(sched.queueDepth("t1"), 5u);
}

// ===========================================================================
// Dispatch — basic execution
// ===========================================================================

TEST_F(GPUTimeSliceSchedulerTest, Dispatch_ExecutesQueuedItems) {
    sched.registerTenant(makeCfg("t1", 1000));
    sched.submit("t1", makeItem("k1"));
    sched.submit("t1", makeItem("k2"));

    std::atomic<int> executed{0};
    sched.dispatch([&](const GPULauncher::WorkItem&) {
        ++executed;
        return true;
    });

    EXPECT_EQ(executed.load(), 2);
    EXPECT_EQ(sched.queueDepth("t1"), 0u);
}

TEST_F(GPUTimeSliceSchedulerTest, Dispatch_WithNullBackend_ItemsComplete) {
    sched.registerTenant(makeCfg("t1", 1000));
    sched.submit("t1", makeItem("k1"));
    sched.dispatch(nullptr);  // no-op backend
    EXPECT_EQ(sched.queueDepth("t1"), 0u);
}

TEST_F(GPUTimeSliceSchedulerTest, Dispatch_MultipleTenants_AllServiced) {
    sched.registerTenant(makeCfg("a", 1000));
    sched.registerTenant(makeCfg("b", 1000));
    sched.submit("a", makeItem("ka1"));
    sched.submit("a", makeItem("ka2"));
    sched.submit("b", makeItem("kb1"));
    sched.submit("b", makeItem("kb2"));

    std::atomic<int> executed{0};
    sched.dispatch([&](const GPULauncher::WorkItem&) {
        ++executed;
        return true;
    });

    EXPECT_EQ(executed.load(), 4);
    EXPECT_EQ(sched.queueDepth("a"), 0u);
    EXPECT_EQ(sched.queueDepth("b"), 0u);
}

TEST_F(GPUTimeSliceSchedulerTest, Dispatch_EmptyQueues_IsNoOp) {
    sched.registerTenant(makeCfg("t1"));
    EXPECT_NO_FATAL_FAILURE(sched.dispatch());
    const auto stats = sched.getStats();
    EXPECT_EQ(stats.total_completed, 0u);
    EXPECT_EQ(stats.dispatch_rounds, 1u);
}

// ===========================================================================
// Time-slice preemption
// ===========================================================================

TEST_F(GPUTimeSliceSchedulerTest, Dispatch_PreemptsWhenSliceExpires) {
    // Use a 1 ms slice and a backend that sleeps for 10 ms per item.
    // With a 1 ms slice and items that each take 10 ms, the first item should
    // consume the full slice (completing before the slice check) and the
    // remaining items should be preempted.
    sched.registerTenant(makeCfg("slow", 1));

    const int kItems = 5;
    for (int i = 0; i < kItems; ++i) {
        sched.submit("slow", makeItem("k" + std::to_string(i)));
    }

    std::atomic<int> executed{0};
    sched.dispatch([&](const GPULauncher::WorkItem&) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ++executed;
        return true;
    });

    // At least one item should have been preempted (not all kItems executed).
    const auto stats = sched.getTenantStats("slow");
    EXPECT_LT(stats.completed, static_cast<size_t>(kItems));
    EXPECT_GT(stats.preempted, 0u);
    EXPECT_GT(sched.queueDepth("slow"), 0u);
}

TEST_F(GPUTimeSliceSchedulerTest, Dispatch_NoPreemption_WhenSliceLarge) {
    // With a very large slice all items complete in one round.
    sched.registerTenant(makeCfg("fast", 60000));

    const int kItems = 10;
    for (int i = 0; i < kItems; ++i) {
        sched.submit("fast", makeItem("k" + std::to_string(i)));
    }

    sched.dispatch(nullptr);

    const auto stats = sched.getTenantStats("fast");
    EXPECT_EQ(stats.completed, static_cast<size_t>(kItems));
    EXPECT_EQ(stats.preempted, 0u);
    EXPECT_EQ(sched.queueDepth("fast"), 0u);
}

// ===========================================================================
// drainAll
// ===========================================================================

TEST_F(GPUTimeSliceSchedulerTest, DrainAll_EmptiesAllQueues) {
    sched.registerTenant(makeCfg("a", 1000));
    sched.registerTenant(makeCfg("b", 1000));
    for (int i = 0; i < 20; ++i) {
        sched.submit("a", makeItem("ka" + std::to_string(i)));
        sched.submit("b", makeItem("kb" + std::to_string(i)));
    }

    sched.drainAll(nullptr);

    EXPECT_TRUE(sched.allQueuesEmpty());
    const auto s = sched.getStats();
    EXPECT_EQ(s.total_completed, 40u);
}

TEST_F(GPUTimeSliceSchedulerTest, DrainAll_WithTinySlice_StillDrains) {
    // Even with a 1 ms slice and 5 ms items, drainAll must eventually
    // empty all queues (may require multiple dispatch rounds).
    sched.registerTenant(makeCfg("t", 1));

    const int kItems = 4;
    for (int i = 0; i < kItems; ++i) {
        sched.submit("t", makeItem("k" + std::to_string(i)));
    }

    sched.drainAll([](const GPULauncher::WorkItem&) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        return true;
    });

    EXPECT_TRUE(sched.allQueuesEmpty());
    const auto stats = sched.getTenantStats("t");
    EXPECT_EQ(stats.completed, static_cast<size_t>(kItems));
}

// ===========================================================================
// allQueuesEmpty
// ===========================================================================

TEST_F(GPUTimeSliceSchedulerTest, AllQueuesEmpty_TrueWhenNothingQueued) {
    sched.registerTenant(makeCfg("t1"));
    EXPECT_TRUE(sched.allQueuesEmpty());
}

TEST_F(GPUTimeSliceSchedulerTest, AllQueuesEmpty_FalseWhenItemsPending) {
    sched.registerTenant(makeCfg("t1"));
    sched.submit("t1", makeItem());
    EXPECT_FALSE(sched.allQueuesEmpty());
}

// ===========================================================================
// Statistics
// ===========================================================================

TEST_F(GPUTimeSliceSchedulerTest, TenantStats_TracksSubmittedAndCompleted) {
    sched.registerTenant(makeCfg("t1", 1000));
    for (int i = 0; i < 3; ++i) {
        sched.submit("t1", makeItem("k" + std::to_string(i)));
    }
    sched.dispatch(nullptr);

    const auto st = sched.getTenantStats("t1");
    EXPECT_EQ(st.submitted, 3u);
    EXPECT_EQ(st.completed, 3u);
    EXPECT_EQ(st.queue_depth, 0u);
    EXPECT_EQ(st.slice_ms, 1000u);
}

TEST_F(GPUTimeSliceSchedulerTest, TenantStats_UnknownTenant_ReturnsZeros) {
    const auto st = sched.getTenantStats("ghost");
    EXPECT_EQ(st.submitted, 0u);
    EXPECT_EQ(st.completed, 0u);
    EXPECT_EQ(st.preempted, 0u);
}

TEST_F(GPUTimeSliceSchedulerTest, GetAllTenantStats_ReturnsOneEntryPerTenant) {
    sched.registerTenant(makeCfg("a"));
    sched.registerTenant(makeCfg("b"));
    const auto all = sched.getAllTenantStats();
    EXPECT_EQ(all.size(), 2u);
}

TEST_F(GPUTimeSliceSchedulerTest, GetAllTenantStats_RespectRegistrationOrder) {
    sched.registerTenant(makeCfg("first"));
    sched.registerTenant(makeCfg("second"));
    const auto all = sched.getAllTenantStats();
    ASSERT_EQ(all.size(), 2u);
    EXPECT_EQ(all[0].tenant_id, "first");
    EXPECT_EQ(all[1].tenant_id, "second");
}

TEST_F(GPUTimeSliceSchedulerTest, GetStats_AggregateCountsAreCorrect) {
    sched.registerTenant(makeCfg("a", 1000));
    sched.registerTenant(makeCfg("b", 1000));
    sched.submit("a", makeItem());
    sched.submit("b", makeItem());
    sched.dispatch(nullptr);

    const auto s = sched.getStats();
    EXPECT_EQ(s.total_submitted,   2u);
    EXPECT_EQ(s.total_completed,   2u);
    EXPECT_EQ(s.dispatch_rounds,   1u);
    EXPECT_EQ(s.registered_tenants, 2u);
}

TEST_F(GPUTimeSliceSchedulerTest, ResetStats_ClearsCountersAndQueues) {
    sched.registerTenant(makeCfg("t1", 1000));
    sched.submit("t1", makeItem());
    sched.dispatch(nullptr);

    sched.resetStats();

    const auto s  = sched.getStats();
    const auto ts = sched.getTenantStats("t1");
    EXPECT_EQ(s.total_submitted, 0u);
    EXPECT_EQ(s.total_completed, 0u);
    EXPECT_EQ(s.dispatch_rounds, 0u);
    EXPECT_EQ(ts.submitted,      0u);
    EXPECT_EQ(ts.completed,      0u);
    EXPECT_TRUE(sched.allQueuesEmpty());
}

// ===========================================================================
// Round-robin fairness
// ===========================================================================

TEST_F(GPUTimeSliceSchedulerTest, RoundRobin_TenantsDontStarve) {
    // Register two tenants with equal slices and submit items to both.
    // After one dispatch round both tenants must have had at least one item
    // executed, confirming neither was starved.
    sched.registerTenant(makeCfg("x", 1000));
    sched.registerTenant(makeCfg("y", 1000));
    for (int i = 0; i < 5; ++i) {
        sched.submit("x", makeItem("kx" + std::to_string(i)));
        sched.submit("y", makeItem("ky" + std::to_string(i)));
    }

    sched.dispatch(nullptr);

    const auto sx = sched.getTenantStats("x");
    const auto sy = sched.getTenantStats("y");
    EXPECT_GE(sx.completed, 1u);
    EXPECT_GE(sy.completed, 1u);
}

// ===========================================================================
// Singleton accessor
// ===========================================================================

TEST(GPUTimeSliceSchedulerSingletonTest, GetInstance_ReturnsSameObject) {
    auto& a = GPUTimeSliceScheduler::GetInstance();
    auto& b = GPUTimeSliceScheduler::GetInstance();
    EXPECT_EQ(&a, &b);
}

// ===========================================================================
// Regression: double-counting of total_preempted_ after unregisterTenant
// ===========================================================================

TEST_F(GPUTimeSliceSchedulerTest, Regression_TotalPreemptedNotDoubleCountedOnUnregister) {
    // Bug: unregisterTenant() previously added tenant.stats.preempted to
    // total_preempted_ even though dispatch() already incremented it.
    // After the fix, total_preempted must equal the per-tenant preempted count,
    // not double that value.
    sched.registerTenant(makeCfg("t1", 1));

    for (int i = 0; i < 5; ++i) {
        sched.submit("t1", makeItem("k" + std::to_string(i)));
    }

    // Use a slow backend so the 1ms slice expires with items remaining.
    sched.dispatch([](const GPULauncher::WorkItem&) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return true;
    });

    // Record aggregate total_preempted_ before unregistering.
    const size_t preempted_before_unreg = sched.getStats().total_preempted;
    ASSERT_GT(preempted_before_unreg, 0u) << "preemption must have occurred";

    sched.unregisterTenant("t1");

    // After unregistering, total_preempted must be unchanged.
    EXPECT_EQ(sched.getStats().total_preempted, preempted_before_unreg)
        << "unregisterTenant must not add tenant preemptions to total_preempted again";
}

// ===========================================================================
// Thread safety: concurrent submit during dispatch (no deadlock / data race)
// ===========================================================================

TEST_F(GPUTimeSliceSchedulerTest, ConcurrentSubmit_DuringDispatch_NoDeadlock) {
    // Verifies that the mutex is NOT held during fn(item) execution:
    // a separate thread must be able to call submit() while dispatch() runs.
    sched.registerTenant(makeCfg("t1", 5000));

    // Submit one initial item so dispatch() has something to work on.
    sched.submit("t1", makeItem("initial"));

    std::atomic<bool> submit_succeeded{false};
    std::atomic<bool> dispatch_entered{false};

    // Backend: signal that dispatch has started, then sleep briefly so the
    // concurrent thread has time to call submit().
    auto backend = [&](const GPULauncher::WorkItem&) -> bool {
        dispatch_entered.store(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        return true;
    };

    // Concurrent thread: wait until dispatch enters the backend, then submit.
    std::thread submitter([&]() {
        // Spin until dispatch is inside fn(item).
        while (!dispatch_entered.load()) {
            std::this_thread::yield();
        }
        // dispatch() unlocks the mutex before calling fn(item), which sets
        // dispatch_entered.  A brief sleep ensures the unlock has propagated
        // before we attempt submit().
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        // This call must not deadlock (mutex must not be held during fn()).
        submit_succeeded.store(sched.submit("t1", makeItem("concurrent")));
    });

    sched.dispatch(backend);
    submitter.join();

    EXPECT_TRUE(submit_succeeded.load())
        << "submit() must succeed while dispatch() is executing a work item";
    // The concurrently submitted item is still in the queue (dispatch() already
    // completed its round), so drain it.
    sched.drainAll(nullptr);
    EXPECT_TRUE(sched.allQueuesEmpty());
}
