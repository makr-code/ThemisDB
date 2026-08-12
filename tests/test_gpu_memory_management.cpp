#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>
#include "themis/gpu/memory_manager.h"

using namespace themis::gpu;

// ---------------------------------------------------------------------------
// Helper: reset the singleton between tests by deallocating everything.
// The singleton persists across tests, so each test must start from a clean
// state by draining whatever was left by previous allocations.
// ---------------------------------------------------------------------------
static void DrainManager() {
    auto& mgr = GPUMemoryManager::GetInstance();
    mgr.DeallocateGPU(mgr.GetGPUMemoryUsed());
    // Clean up any tenant state that tests may have registered.
    for (const auto& ts : mgr.GetAllTenantStats()) {
        mgr.RemoveTenantQuota(ts.tenant_id);
    }
}

class GPUMemoryManagerTest : public ::testing::Test {
protected:
    void SetUp() override { DrainManager(); }
    void TearDown() override { DrainManager(); }
};

// ---------------------------------------------------------------------------
// Edition-limit enforcement
// ---------------------------------------------------------------------------

TEST_F(GPUMemoryManagerTest, GetMaxVRAM_MatchesEditionConstant) {
    const uint64_t expected_bytes =
        static_cast<uint64_t>(GPUMemoryManager::GetMaxGPUVRAMGB()) *
        1024ULL * 1024ULL * 1024ULL;
    EXPECT_EQ(GPUMemoryManager::GetMaxGPUVRAMBytes(), expected_bytes);
}

TEST_F(GPUMemoryManagerTest, IsGPUAccelerationEnabled_MatchesVRAMLimit) {
    auto& mgr = GPUMemoryManager::GetInstance();
    EXPECT_EQ(mgr.IsGPUAccelerationEnabled(),
              GPUMemoryManager::GetMaxGPUVRAMGB() > 0);
}

// ---------------------------------------------------------------------------
// TryAllocateGPU — success path
// ---------------------------------------------------------------------------

TEST_F(GPUMemoryManagerTest, AllocSmallAmount_Succeeds) {
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t one_mb = 1024ULL * 1024ULL;
    EXPECT_TRUE(mgr.TryAllocateGPU(one_mb, "test"));
    EXPECT_EQ(mgr.GetGPUMemoryUsed(), one_mb);
}

TEST_F(GPUMemoryManagerTest, AllocExactLimit_Succeeds) {
    const uint64_t limit = GPUMemoryManager::GetMaxGPUVRAMBytes();
    if (limit == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    EXPECT_TRUE(mgr.TryAllocateGPU(limit, "exact_limit"));
    EXPECT_EQ(mgr.GetGPUMemoryUsed(), limit);
}

// ---------------------------------------------------------------------------
// TryAllocateGPU — failure path
// ---------------------------------------------------------------------------

TEST_F(GPUMemoryManagerTest, AllocBeyondLimit_ReturnsFalse) {
    const uint64_t limit = GPUMemoryManager::GetMaxGPUVRAMBytes();
    auto& mgr = GPUMemoryManager::GetInstance();
    // Requesting one byte more than the limit must fail.
    EXPECT_FALSE(mgr.TryAllocateGPU(limit + 1, "over_limit"));
    // Usage must remain zero — the failed attempt must not modify state.
    EXPECT_EQ(mgr.GetGPUMemoryUsed(), 0u);
}

TEST_F(GPUMemoryManagerTest, AllocAfterLimitReached_ReturnsFalse) {
    const uint64_t limit = GPUMemoryManager::GetMaxGPUVRAMBytes();
    if (limit == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    ASSERT_TRUE(mgr.TryAllocateGPU(limit, "fill"));
    const uint64_t one_mb = 1024ULL * 1024ULL;
    EXPECT_FALSE(mgr.TryAllocateGPU(one_mb, "overflow"));
    EXPECT_EQ(mgr.GetGPUMemoryUsed(), limit);
}

// ---------------------------------------------------------------------------
// DeallocateGPU
// ---------------------------------------------------------------------------

TEST_F(GPUMemoryManagerTest, Dealloc_ReducesUsage) {
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t size = 512ULL * 1024ULL * 1024ULL;  // 512 MB
    ASSERT_TRUE(mgr.TryAllocateGPU(size, "alloc"));
    mgr.DeallocateGPU(size);
    EXPECT_EQ(mgr.GetGPUMemoryUsed(), 0u);
}

TEST_F(GPUMemoryManagerTest, Dealloc_AllowsReallocAfterFree) {
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t size = 1024ULL * 1024ULL * 1024ULL;  // 1 GB
    if (size > GPUMemoryManager::GetMaxGPUVRAMBytes()) {
        GTEST_SKIP() << "capability:edition_limit_sufficient=false;reason=edition_limit_too_small_for_test";
    }
    ASSERT_TRUE(mgr.TryAllocateGPU(size, "a1"));
    mgr.DeallocateGPU(size);
    EXPECT_EQ(mgr.GetGPUMemoryUsed(), 0u);
    EXPECT_TRUE(mgr.TryAllocateGPU(size, "a2"));
}

TEST_F(GPUMemoryManagerTest, Dealloc_DoubleFreeGuard_DoesNotUnderflow) {
    auto& mgr = GPUMemoryManager::GetInstance();
    // Deallocating more than is allocated must clamp to zero, not underflow.
    mgr.DeallocateGPU(1024ULL * 1024ULL * 1024ULL);
    EXPECT_EQ(mgr.GetGPUMemoryUsed(), 0u);
}

// ---------------------------------------------------------------------------
// Stats
// ---------------------------------------------------------------------------

TEST_F(GPUMemoryManagerTest, Stats_InitiallyZero) {
    auto& mgr = GPUMemoryManager::GetInstance();
    auto s = mgr.GetStats();
    EXPECT_EQ(s.allocated_bytes, 0u);
    EXPECT_EQ(s.allocation_count, 0u);
    // SetUp() drains the singleton and may increment deallocation_count.
    EXPECT_GE(s.deallocation_count, 1u);
}

TEST_F(GPUMemoryManagerTest, Stats_CountsAllocsAndDeallocs) {
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t mb = 1024ULL * 1024ULL;
    const auto before = mgr.GetStats();

    ASSERT_TRUE(mgr.TryAllocateGPU(mb, "t1"));
    ASSERT_TRUE(mgr.TryAllocateGPU(mb, "t2"));
    mgr.DeallocateGPU(mb);

    auto s = mgr.GetStats();
    EXPECT_EQ(s.allocation_count, before.allocation_count + 2u);
    EXPECT_EQ(s.deallocation_count, before.deallocation_count + 1u);
    EXPECT_EQ(s.allocated_bytes, mb);
}

TEST_F(GPUMemoryManagerTest, Stats_PeakTracksHighWaterMark) {
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t mb = 1024ULL * 1024ULL;

    ASSERT_TRUE(mgr.TryAllocateGPU(4 * mb, "fill"));
    const uint64_t peak_after_alloc = mgr.GetStats().peak_bytes;

    mgr.DeallocateGPU(4 * mb);
    const uint64_t peak_after_free = mgr.GetStats().peak_bytes;

    // Peak must not decrease after deallocation.
    EXPECT_EQ(peak_after_alloc, 4 * mb);
    EXPECT_EQ(peak_after_free, 4 * mb);
    EXPECT_EQ(mgr.GetStats().allocated_bytes, 0u);
}

TEST_F(GPUMemoryManagerTest, Stats_FailedAllocDoesNotIncrementCount) {
    const uint64_t limit = GPUMemoryManager::GetMaxGPUVRAMBytes();
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t before = mgr.GetStats().allocation_count;
    // Attempt an allocation that is guaranteed to fail.
    mgr.TryAllocateGPU(limit + 1, "fail");
    EXPECT_EQ(mgr.GetStats().allocation_count, before);
}

// ---------------------------------------------------------------------------
// UsagePercent
// ---------------------------------------------------------------------------

TEST_F(GPUMemoryManagerTest, UsagePercent_ZeroWhenNotAllocated) {
    auto& mgr = GPUMemoryManager::GetInstance();
    EXPECT_FLOAT_EQ(mgr.GetGPUMemoryUsagePercent(), 0.0f);
}

TEST_F(GPUMemoryManagerTest, UsagePercent_100WhenFull) {
    const uint64_t limit = GPUMemoryManager::GetMaxGPUVRAMBytes();
    if (limit == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    ASSERT_TRUE(mgr.TryAllocateGPU(limit, "fill"));
    EXPECT_FLOAT_EQ(mgr.GetGPUMemoryUsagePercent(), 100.0f);
}

// ---------------------------------------------------------------------------
// ValidateAllocation — throws on rejection
// ---------------------------------------------------------------------------

TEST_F(GPUMemoryManagerTest, ValidateAllocation_ThrowsWhenOverLimit) {
    const uint64_t limit = GPUMemoryManager::GetMaxGPUVRAMBytes();
    auto& mgr = GPUMemoryManager::GetInstance();
    EXPECT_THROW(mgr.ValidateAllocation(limit + 1), std::runtime_error);
}

TEST_F(GPUMemoryManagerTest, ValidateAllocation_DoesNotThrowWhenFits) {
    const uint64_t limit = GPUMemoryManager::GetMaxGPUVRAMBytes();
    if (limit == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    EXPECT_NO_THROW(mgr.ValidateAllocation(limit / 2));
}

// ---------------------------------------------------------------------------
// GetEditionInfo / GetGPUFallbackStrategy
// ---------------------------------------------------------------------------

TEST_F(GPUMemoryManagerTest, GetEditionInfo_ContainsEditionName) {
    auto& mgr = GPUMemoryManager::GetInstance();
    const std::string info = mgr.GetEditionInfo();
    EXPECT_FALSE(info.empty());
    EXPECT_NE(info.find("Edition:"), std::string::npos);
    EXPECT_NE(info.find("GPU VRAM:"), std::string::npos);
}

TEST_F(GPUMemoryManagerTest, GetGPUFallbackStrategy_NonEmpty) {
    const std::string strat = GetGPUFallbackStrategy();
    EXPECT_FALSE(strat.empty());
    EXPECT_NE(strat.find("Falling back to CPU"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Tag / owner tracking (AllocationRecord)
// ---------------------------------------------------------------------------

TEST_F(GPUMemoryManagerTest, ActiveAllocations_TagsAreTracked) {
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t mb = 1024ULL * 1024ULL;

    ASSERT_TRUE(mgr.TryAllocateGPU(mb, "vector_index"));
    ASSERT_TRUE(mgr.TryAllocateGPU(2 * mb, "embedding_cache"));

    const auto records = mgr.GetActiveAllocations();
    ASSERT_EQ(records.size(), 2u);
    EXPECT_EQ(records[0].tag, "vector_index");
    EXPECT_EQ(records[0].size_bytes, mb);
    EXPECT_EQ(records[1].tag, "embedding_cache");
    EXPECT_EQ(records[1].size_bytes, 2 * mb);
}

TEST_F(GPUMemoryManagerTest, ActiveAllocations_RecordRemovedOnDealloc) {
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t mb = 1024ULL * 1024ULL;

    ASSERT_TRUE(mgr.TryAllocateGPU(mb, "a"));
    ASSERT_TRUE(mgr.TryAllocateGPU(mb, "b"));
    EXPECT_EQ(mgr.GetActiveAllocations().size(), 2u);

    mgr.DeallocateGPU(mb);
    EXPECT_EQ(mgr.GetActiveAllocations().size(), 1u);

    mgr.DeallocateGPU(mb);
    EXPECT_EQ(mgr.GetActiveAllocations().size(), 0u);
}

TEST_F(GPUMemoryManagerTest, ActiveAllocations_EmptyAfterDrainManager) {
    auto& mgr = GPUMemoryManager::GetInstance();
    // DrainManager() is called in TearDown, but verify it also leaves
    // the active allocations empty.
    EXPECT_EQ(mgr.GetActiveAllocations().size(), 0u);
}

// ---------------------------------------------------------------------------
// Tenant / domain isolation
// ---------------------------------------------------------------------------

TEST_F(GPUMemoryManagerTest, Tenant_QuotaSetAndRetrieved) {
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t quota = 4ULL * 1024ULL * 1024ULL * 1024ULL;  // 4 GB
    mgr.SetTenantQuota("tenant_a", quota);

    const auto ts = mgr.GetTenantStats("tenant_a");
    EXPECT_EQ(ts.tenant_id, "tenant_a");
    EXPECT_EQ(ts.quota_bytes, quota);
    EXPECT_EQ(ts.allocated_bytes, 0u);
}

TEST_F(GPUMemoryManagerTest, Tenant_AllocWithinQuota_Succeeds) {
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t quota = 2ULL * 1024ULL * 1024ULL * 1024ULL;  // 2 GB
    const uint64_t alloc = 1ULL * 1024ULL * 1024ULL * 1024ULL;  // 1 GB
    if (alloc > GPUMemoryManager::GetMaxGPUVRAMBytes()) {
        GTEST_SKIP() << "capability:edition_limit_sufficient=false;reason=edition_limit_too_small_for_test";
    }
    mgr.SetTenantQuota("tenant_b", quota);

    EXPECT_TRUE(mgr.TryAllocateGPU(alloc, "work", "tenant_b"));
    EXPECT_EQ(mgr.GetTenantStats("tenant_b").allocated_bytes, alloc);

    mgr.DeallocateGPU(alloc, "tenant_b");
}

TEST_F(GPUMemoryManagerTest, Tenant_AllocExceedsQuota_Rejected) {
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t mb    = 1024ULL * 1024ULL;
    const uint64_t quota = 10 * mb;   // 10 MB quota
    const uint64_t alloc = 20 * mb;   // trying to allocate 20 MB
    mgr.SetTenantQuota("tenant_c", quota);

    EXPECT_FALSE(mgr.TryAllocateGPU(alloc, "overflow", "tenant_c"));
    // Global usage must remain zero — rejected alloc must not modify state.
    EXPECT_EQ(mgr.GetGPUMemoryUsed(), 0u);
    EXPECT_EQ(mgr.GetTenantStats("tenant_c").allocated_bytes, 0u);
}

TEST_F(GPUMemoryManagerTest, Tenant_QuotaFillThenReject) {
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t mb    = 1024ULL * 1024ULL;
    const uint64_t quota = 4 * mb;
    mgr.SetTenantQuota("tenant_d", quota);

    ASSERT_TRUE(mgr.TryAllocateGPU(4 * mb, "fill", "tenant_d"));
    EXPECT_FALSE(mgr.TryAllocateGPU(mb, "overflow", "tenant_d"));  // quota full

    mgr.DeallocateGPU(4 * mb, "tenant_d");
}

TEST_F(GPUMemoryManagerTest, Tenant_TenantsAreIsolated) {
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() < 20ULL * 1024ULL * 1024ULL) {
        GTEST_SKIP() << "capability:edition_limit_sufficient=false;reason=edition_limit_too_small_for_test";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t mb = 1024ULL * 1024ULL;
    mgr.SetTenantQuota("tenant_x", 5 * mb);
    mgr.SetTenantQuota("tenant_y", 5 * mb);

    // Fill tenant_x quota.
    ASSERT_TRUE(mgr.TryAllocateGPU(5 * mb, "x_work", "tenant_x"));

    // tenant_y must still be able to allocate (separate quota).
    EXPECT_TRUE(mgr.TryAllocateGPU(5 * mb, "y_work", "tenant_y"));

    EXPECT_EQ(mgr.GetTenantStats("tenant_x").allocated_bytes, 5 * mb);
    EXPECT_EQ(mgr.GetTenantStats("tenant_y").allocated_bytes, 5 * mb);

    mgr.DeallocateGPU(5 * mb, "tenant_x");
    mgr.DeallocateGPU(5 * mb, "tenant_y");
}

TEST_F(GPUMemoryManagerTest, Tenant_DeallocByTenantDecrementsTenantUsage) {
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t mb = 1024ULL * 1024ULL;
    mgr.SetTenantQuota("tenant_e", 10 * mb);

    ASSERT_TRUE(mgr.TryAllocateGPU(4 * mb, "work", "tenant_e"));
    EXPECT_EQ(mgr.GetTenantStats("tenant_e").allocated_bytes, 4 * mb);

    mgr.DeallocateGPU(4 * mb, "tenant_e");
    EXPECT_EQ(mgr.GetTenantStats("tenant_e").allocated_bytes, 0u);
    EXPECT_EQ(mgr.GetGPUMemoryUsed(), 0u);
}

TEST_F(GPUMemoryManagerTest, Tenant_PeakTrackedPerTenant) {
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t mb = 1024ULL * 1024ULL;
    mgr.SetTenantQuota("tenant_f", 8 * mb);

    ASSERT_TRUE(mgr.TryAllocateGPU(6 * mb, "peak_work", "tenant_f"));
    const uint64_t peak_before_free = mgr.GetTenantStats("tenant_f").peak_bytes;

    mgr.DeallocateGPU(6 * mb, "tenant_f");
    const uint64_t peak_after_free = mgr.GetTenantStats("tenant_f").peak_bytes;

    EXPECT_EQ(peak_before_free, 6 * mb);
    EXPECT_EQ(peak_after_free, 6 * mb);  // peak doesn't decrease
}

TEST_F(GPUMemoryManagerTest, Tenant_GetAllTenantStats_IncludesAllRegistered) {
    auto& mgr = GPUMemoryManager::GetInstance();
    mgr.SetTenantQuota("t1", 1024ULL * 1024ULL);
    mgr.SetTenantQuota("t2", 2048ULL * 1024ULL);

    const auto all = mgr.GetAllTenantStats();
    EXPECT_GE(all.size(), 2u);
    // Both tenant_ids must appear in the results.
    bool found_t1 = false, found_t2 = false;
    for (const auto& ts : all) {
        if (ts.tenant_id == "t1") found_t1 = true;
        if (ts.tenant_id == "t2") found_t2 = true;
    }
    EXPECT_TRUE(found_t1);
    EXPECT_TRUE(found_t2);
}

TEST_F(GPUMemoryManagerTest, Tenant_GetTenantStats_UnknownTenant_ReturnsZero) {
    auto& mgr = GPUMemoryManager::GetInstance();
    const auto ts = mgr.GetTenantStats("does_not_exist");
    EXPECT_EQ(ts.allocated_bytes, 0u);
    EXPECT_EQ(ts.quota_bytes, 0u);
    EXPECT_EQ(ts.peak_bytes, 0u);
}

TEST_F(GPUMemoryManagerTest, Tenant_Headroom_WithQuota) {
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t mb = 1024ULL * 1024ULL;
    mgr.SetTenantQuota("tenant_g", 8 * mb);

    ASSERT_TRUE(mgr.TryAllocateGPU(3 * mb, "work", "tenant_g"));
    const uint64_t headroom = mgr.GetTenantHeadroom("tenant_g");
    // tenant headroom = 8 - 3 = 5 MB (assuming global still has plenty)
    EXPECT_EQ(headroom, 5 * mb);

    mgr.DeallocateGPU(3 * mb, "tenant_g");
}

TEST_F(GPUMemoryManagerTest, Tenant_Headroom_NoQuota_ReturnsGlobalRemaining) {
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t mb = 1024ULL * 1024ULL;

    // No quota registered for this tenant.
    ASSERT_TRUE(mgr.TryAllocateGPU(mb, "global_work"));
    const uint64_t headroom = mgr.GetTenantHeadroom("no_quota_tenant");
    EXPECT_EQ(headroom, GPUMemoryManager::GetMaxGPUVRAMBytes() - mb);

    mgr.DeallocateGPU(mb);
}

TEST_F(GPUMemoryManagerTest, Tenant_RemoveQuota_AllowsUnlimitedUse) {
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t mb = 1024ULL * 1024ULL;
    mgr.SetTenantQuota("tenant_h", 2 * mb);

    // With quota, 3 MB is rejected.
    EXPECT_FALSE(mgr.TryAllocateGPU(3 * mb, "over", "tenant_h"));

    // After removing the quota, 3 MB should succeed (limited only by edition).
    mgr.RemoveTenantQuota("tenant_h");
    EXPECT_TRUE(mgr.TryAllocateGPU(3 * mb, "now_ok", "tenant_h"));
    // Verify allocation tracking still works after quota removal.
    EXPECT_EQ(mgr.GetTenantStats("tenant_h").allocated_bytes, 3 * mb);
    mgr.DeallocateGPU(3 * mb, "tenant_h");
}

// ---------------------------------------------------------------------------
// Concurrent stress tests — verify thread safety of GPUMemoryManager
// ---------------------------------------------------------------------------

TEST_F(GPUMemoryManagerTest, Concurrent_AllocDealloc_NoCounterDrift) {
    const uint64_t limit = GPUMemoryManager::GetMaxGPUVRAMBytes();
    if (limit == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t chunk = 1024ULL * 1024ULL;  // 1 MB per operation
    // Use at most 25% of limit so we don't hit the ceiling.
    const int max_live = static_cast<int>((limit / 4) / chunk);
    if (max_live < 4) {
        GTEST_SKIP() << "capability:edition_limit_sufficient=false;reason=edition_limit_too_small_for_concurrent_test";
    }

    constexpr int THREADS = 4;
    // Each thread allocates then frees a single chunk; the 25% budget means
    // at least one thread always succeeds.
    std::atomic<int> unexpected_failures{0};

    auto worker = [&]() {
        std::vector<bool> allocated(max_live, false);
        // Allocate all.
        for (int i = 0; i < max_live; ++i) {
            allocated[i] = mgr.TryAllocateGPU(chunk, "stress");
        }
        // Deallocate what was allocated.
        for (int i = 0; i < max_live; ++i) {
            if (allocated[i]) {
                mgr.DeallocateGPU(chunk);
            }
        }
        // After each thread fully drains its own allocations, used bytes must
        // only contain what *other* threads still hold — never negative.
        if (mgr.GetGPUMemoryUsed() > limit) {
            unexpected_failures.fetch_add(1);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(THREADS);
    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back(worker);
    }
    for (auto& th : threads) th.join();

    // After all threads finish, allocated bytes must be exactly 0.
    EXPECT_EQ(mgr.GetGPUMemoryUsed(), 0u) << "Counter drift detected";
    EXPECT_EQ(unexpected_failures.load(), 0);
}

TEST_F(GPUMemoryManagerTest, Concurrent_Stats_NeverNegative) {
    const uint64_t limit = GPUMemoryManager::GetMaxGPUVRAMBytes();
    if (limit == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t chunk = 512ULL * 1024ULL;  // 512 KB
    constexpr int THREADS = 8;
    constexpr int OPS     = 20;

    std::atomic<bool> saw_negative{false};

    auto worker = [&]() {
        for (int i = 0; i < OPS; ++i) {
            mgr.TryAllocateGPU(chunk, "stats_stress");
            const auto s = mgr.GetStats();
            // allocated_bytes is uint64_t — underflow would wrap to very large value
            if (s.allocated_bytes > limit) {
                saw_negative.store(true);
            }
            mgr.DeallocateGPU(chunk);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(THREADS);
    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back(worker);
    }
    for (auto& th : threads) th.join();

    EXPECT_FALSE(saw_negative.load()) << "Underflow/overflow detected in concurrent stats";
    EXPECT_EQ(mgr.GetGPUMemoryUsed(), 0u);
}

TEST_F(GPUMemoryManagerTest, Concurrent_TenantIsolation_NoLeakage) {
    const uint64_t limit = GPUMemoryManager::GetMaxGPUVRAMBytes();
    if (limit < 32ULL * 1024ULL * 1024ULL) {
        GTEST_SKIP() << "capability:edition_limit_sufficient=false;reason=edition_limit_too_small_for_concurrent_tenant_test";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t mb = 1024ULL * 1024ULL;

    // Each tenant gets 4 MB quota; 4 tenants run in parallel.
    constexpr int N_TENANTS = 4;
    constexpr int OPS_PER   = 10;
    for (int i = 0; i < N_TENANTS; ++i) {
        mgr.SetTenantQuota("stress_t" + std::to_string(i), 4 * mb);
    }

    std::atomic<int> quota_violations{0};

    auto tenant_worker = [&](int tid) {
        const std::string name = "stress_t" + std::to_string(tid);
        for (int op = 0; op < OPS_PER; ++op) {
            bool ok = mgr.TryAllocateGPU(mb, "work", name);
            if (ok) {
                // Check tenant usage never exceeds quota.
                auto ts = mgr.GetTenantStats(name);
                if (ts.allocated_bytes > 4 * mb) {
                    quota_violations.fetch_add(1);
                }
                mgr.DeallocateGPU(mb, name);
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(N_TENANTS);
    for (int t = 0; t < N_TENANTS; ++t) {
        threads.emplace_back(tenant_worker, t);
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(quota_violations.load(), 0)
        << "Tenant quota exceeded during concurrent access";
    EXPECT_EQ(mgr.GetGPUMemoryUsed(), 0u);
}

// ---------------------------------------------------------------------------
// Fuzz-style tests — arbitrary sizes fed to TryAllocateGPU / ValidateAllocation
// ---------------------------------------------------------------------------

TEST_F(GPUMemoryManagerTest, Fuzz_ArbitrarySizes_NeverCrash) {
    auto& mgr = GPUMemoryManager::GetInstance();
    const std::vector<uint64_t> sizes = {
        0,
        1,
        1023,
        1024,
        1ULL * 1024 * 1024,            // 1 MB
        512ULL * 1024 * 1024,          // 512 MB
        1ULL * 1024 * 1024 * 1024,     // 1 GB
        16ULL * 1024 * 1024 * 1024,    // 16 GB
        100ULL * 1024 * 1024 * 1024,   // 100 GB (exceeds any edition)
        UINT64_MAX / 2,
        UINT64_MAX - 1,
        UINT64_MAX,
    };
    for (uint64_t sz : sizes) {
        // Must not crash or throw — only return true/false.
        bool result = mgr.TryAllocateGPU(sz, "fuzz");
        // Cleanup any successful allocation.
        if (result) mgr.DeallocateGPU(sz);
        // UINT64_MAX should never be allocated.
        if (sz == UINT64_MAX) EXPECT_FALSE(result);
    }
}

TEST_F(GPUMemoryManagerTest, Fuzz_ValidateAllocation_LargeRequest_ThrowsNotCrash) {
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t big = UINT64_MAX;
    EXPECT_THROW(mgr.ValidateAllocation(big), std::runtime_error);
}

TEST_F(GPUMemoryManagerTest, Fuzz_DeallocLargerThanAllocated_ClampsToZero) {
    auto& mgr = GPUMemoryManager::GetInstance();
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    const uint64_t mb = 1024ULL * 1024ULL;
    ASSERT_TRUE(mgr.TryAllocateGPU(mb, "small"));
    // Deallocate 1000x the allocated amount — must not underflow.
    mgr.DeallocateGPU(mb * 1000);
    EXPECT_EQ(mgr.GetGPUMemoryUsed(), 0u);
}

TEST_F(GPUMemoryManagerTest, Fuzz_EmptyTag_Handled) {
    auto& mgr = GPUMemoryManager::GetInstance();
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    const uint64_t mb = 1024ULL * 1024ULL;
    EXPECT_TRUE(mgr.TryAllocateGPU(mb, ""));  // empty tag must not crash
    mgr.DeallocateGPU(mb);
}

TEST_F(GPUMemoryManagerTest, Fuzz_LongTag_Handled) {
    auto& mgr = GPUMemoryManager::GetInstance();
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    const uint64_t mb = 1024ULL * 1024ULL;
    const std::string long_tag(10000, 'x');
    EXPECT_TRUE(mgr.TryAllocateGPU(mb, long_tag));
    mgr.DeallocateGPU(mb);
}

// ---------------------------------------------------------------------------
// Chaos tests — simulate device loss mid-operation
// ---------------------------------------------------------------------------

TEST_F(GPUMemoryManagerTest, Chaos_SimulateDeviceLoss_FallsBackGracefully) {
    // Simulate: some allocations succeed, then "device is lost" → all
    // subsequent TryAllocateGPU return false.  After recovery (DeallocateGPU
    // drains), allocations succeed again.
    auto& mgr = GPUMemoryManager::GetInstance();
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=gpu_not_available_in_edition";
    }
    const uint64_t mb = 1024ULL * 1024ULL;

    // Phase 1: normal ops.
    ASSERT_TRUE(mgr.TryAllocateGPU(mb, "phase1"));

    // Phase 2: "device lost" — forcibly fill VRAM to simulate total OOM.
    const uint64_t remaining =
        GPUMemoryManager::GetMaxGPUVRAMBytes() - mgr.GetGPUMemoryUsed();
    if (remaining > 0) {
        mgr.TryAllocateGPU(remaining, "fill_for_device_loss");
    }
    // Now VRAM is at limit — further alloc must fail.
    EXPECT_FALSE(mgr.TryAllocateGPU(mb, "after_loss"));

    // Phase 3: recovery — drain all and verify allocations work again.
    mgr.DeallocateGPU(mgr.GetGPUMemoryUsed());
    EXPECT_EQ(mgr.GetGPUMemoryUsed(), 0u);
    EXPECT_TRUE(mgr.TryAllocateGPU(mb, "recovered"));
    mgr.DeallocateGPU(mb);
}

TEST_F(GPUMemoryManagerTest, Chaos_SimulateMultipleTenantOOMAndRecovery) {
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t mb = 1024ULL * 1024ULL;
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() < 10 * mb) {
        GTEST_SKIP() << "capability:edition_limit_sufficient=false;reason=edition_limit_too_small_for_chaos_test";
    }
    mgr.SetTenantQuota("chaos_t1", 3 * mb);
    mgr.SetTenantQuota("chaos_t2", 3 * mb);

    // Fill both tenants.
    ASSERT_TRUE(mgr.TryAllocateGPU(3 * mb, "fill", "chaos_t1"));
    ASSERT_TRUE(mgr.TryAllocateGPU(3 * mb, "fill", "chaos_t2"));

    // Both at quota — no further allocs succeed for either.
    EXPECT_FALSE(mgr.TryAllocateGPU(mb, "over", "chaos_t1"));
    EXPECT_FALSE(mgr.TryAllocateGPU(mb, "over", "chaos_t2"));

    // Recovery: release t1.
    mgr.DeallocateGPU(3 * mb, "chaos_t1");
    EXPECT_TRUE(mgr.TryAllocateGPU(mb, "post_recovery", "chaos_t1"));

    // Cleanup.
    mgr.DeallocateGPU(3 * mb, "chaos_t2");
    mgr.DeallocateGPU(mb, "chaos_t1");
}

// ===========================================================================
// Pre-allocation hint tests
// ===========================================================================

class GPUMemoryHintTest : public ::testing::Test {
protected:
    void SetUp() override    { DrainManager(); }
    void TearDown() override { DrainManager(); }
};

TEST_F(GPUMemoryHintTest, ReserveHint_ValidRequest_ReturnsNonZeroId) {
    auto& mgr = GPUMemoryManager::GetInstance();
    auto h = mgr.ReserveHint(1024, "hint_tag");
    EXPECT_GT(h.id, 0u);
    EXPECT_EQ(h.bytes, 1024u);
    mgr.CancelHint(h.id);
}

TEST_F(GPUMemoryHintTest, ReserveHint_SetsHintReservedBytes) {
    auto& mgr = GPUMemoryManager::GetInstance();
    auto h = mgr.ReserveHint(512, "h");
    ASSERT_GT(h.id, 0u);
    EXPECT_EQ(mgr.GetHintReservedBytes(), 512u);
    mgr.CancelHint(h.id);
    EXPECT_EQ(mgr.GetHintReservedBytes(), 0u);
}

TEST_F(GPUMemoryHintTest, ReserveHint_BlocksOtherAllocationsWhenFull) {
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t max = GPUMemoryManager::GetMaxGPUVRAMBytes();
    // Reserve all available VRAM as a hint.
    auto h = mgr.ReserveHint(max, "full_hint");
    ASSERT_GT(h.id, 0u);
    // Any additional allocation or hint should now fail.
    EXPECT_FALSE(mgr.TryAllocateGPU(1, "over_hint"));
    auto h2 = mgr.ReserveHint(1, "over_hint_2");
    EXPECT_EQ(h2.id, 0u);
    mgr.CancelHint(h.id);
}

TEST_F(GPUMemoryHintTest, CancelHint_InvalidId_IsNoOp) {
    auto& mgr = GPUMemoryManager::GetInstance();
    EXPECT_NO_THROW(mgr.CancelHint(0));
    EXPECT_NO_THROW(mgr.CancelHint(999999));
}

TEST_F(GPUMemoryHintTest, ConsumeHint_ConvertsHintToRealAllocation) {
    auto& mgr = GPUMemoryManager::GetInstance();
    auto h = mgr.ReserveHint(256, "consume_tag");
    ASSERT_GT(h.id, 0u);
    const uint64_t before_alloc = mgr.GetStats().allocation_count;

    EXPECT_TRUE(mgr.ConsumeHint(h.id));

    // Hint bytes are no longer in the hint pool.
    EXPECT_EQ(mgr.GetHintReservedBytes(), 0u);
    // But they are now tracked as a real allocation.
    EXPECT_EQ(mgr.GetGPUMemoryUsed(), 256u);
    EXPECT_EQ(mgr.GetStats().allocation_count, before_alloc + 1);

    mgr.DeallocateGPU(256u);
}

TEST_F(GPUMemoryHintTest, ConsumeHint_DoubleConsume_ReturnsFalse) {
    auto& mgr = GPUMemoryManager::GetInstance();
    auto h = mgr.ReserveHint(128, "dbl");
    ASSERT_GT(h.id, 0u);
    EXPECT_TRUE(mgr.ConsumeHint(h.id));
    EXPECT_FALSE(mgr.ConsumeHint(h.id));
    mgr.DeallocateGPU(128u);
}

TEST_F(GPUMemoryHintTest, ConsumeHint_InvalidId_ReturnsFalse) {
    auto& mgr = GPUMemoryManager::GetInstance();
    EXPECT_FALSE(mgr.ConsumeHint(0));
    EXPECT_FALSE(mgr.ConsumeHint(999999));
}

TEST_F(GPUMemoryHintTest, ReserveHint_TenantQuota_EnforcedForHints) {
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t quota = 1024 * 1024;  // 1 MB
    mgr.SetTenantQuota("hint_tenant", quota);

    auto h1 = mgr.ReserveHint(quota, "h1", "hint_tenant");
    EXPECT_GT(h1.id, 0u);
    // Tenant quota should now be exhausted.
    auto h2 = mgr.ReserveHint(1, "h2", "hint_tenant");
    EXPECT_EQ(h2.id, 0u);

    mgr.CancelHint(h1.id);
    mgr.RemoveTenantQuota("hint_tenant");
}

TEST_F(GPUMemoryHintTest, ReserveHint_MultipleHints_AggregatedAgainstTenantQuota) {
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t quota = 1024 * 1024;  // 1 MB
    mgr.SetTenantQuota("mh_tenant", quota);

    // Reserve half the quota with first hint.
    auto h1 = mgr.ReserveHint(quota / 2, "h1", "mh_tenant");
    EXPECT_GT(h1.id, 0u);
    // Reserve remaining half.
    auto h2 = mgr.ReserveHint(quota / 2, "h2", "mh_tenant");
    EXPECT_GT(h2.id, 0u);
    // Any further reservation should fail — quota fully used by hints.
    auto h3 = mgr.ReserveHint(1, "h3", "mh_tenant");
    EXPECT_EQ(h3.id, 0u);

    mgr.CancelHint(h1.id);
    mgr.CancelHint(h2.id);
    mgr.RemoveTenantQuota("mh_tenant");
}
