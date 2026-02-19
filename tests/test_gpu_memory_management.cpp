#include <gtest/gtest.h>
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
        GTEST_SKIP() << "GPU not available in this edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t one_mb = 1024ULL * 1024ULL;
    EXPECT_TRUE(mgr.TryAllocateGPU(one_mb, "test"));
    EXPECT_EQ(mgr.GetGPUMemoryUsed(), one_mb);
}

TEST_F(GPUMemoryManagerTest, AllocExactLimit_Succeeds) {
    const uint64_t limit = GPUMemoryManager::GetMaxGPUVRAMBytes();
    if (limit == 0) {
        GTEST_SKIP() << "GPU not available in this edition";
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
        GTEST_SKIP() << "GPU not available in this edition";
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
        GTEST_SKIP() << "GPU not available in this edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t size = 512ULL * 1024ULL * 1024ULL;  // 512 MB
    ASSERT_TRUE(mgr.TryAllocateGPU(size, "alloc"));
    mgr.DeallocateGPU(size);
    EXPECT_EQ(mgr.GetGPUMemoryUsed(), 0u);
}

TEST_F(GPUMemoryManagerTest, Dealloc_AllowsReallocAfterFree) {
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "GPU not available in this edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t size = 1024ULL * 1024ULL * 1024ULL;  // 1 GB
    if (size > GPUMemoryManager::GetMaxGPUVRAMBytes()) {
        GTEST_SKIP() << "Edition limit too small for this test";
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
    EXPECT_EQ(s.deallocation_count, 0u);
}

TEST_F(GPUMemoryManagerTest, Stats_CountsAllocsAndDeallocs) {
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "GPU not available in this edition";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t mb = 1024ULL * 1024ULL;

    ASSERT_TRUE(mgr.TryAllocateGPU(mb, "t1"));
    ASSERT_TRUE(mgr.TryAllocateGPU(mb, "t2"));
    mgr.DeallocateGPU(mb);

    auto s = mgr.GetStats();
    EXPECT_EQ(s.allocation_count, 2u);
    EXPECT_EQ(s.deallocation_count, 1u);
    EXPECT_EQ(s.allocated_bytes, mb);
}

TEST_F(GPUMemoryManagerTest, Stats_PeakTracksHighWaterMark) {
    if (GPUMemoryManager::GetMaxGPUVRAMBytes() == 0) {
        GTEST_SKIP() << "GPU not available in this edition";
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
    // Attempt an allocation that is guaranteed to fail.
    mgr.TryAllocateGPU(limit + 1, "fail");
    EXPECT_EQ(mgr.GetStats().allocation_count, 0u);
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
        GTEST_SKIP() << "GPU not available in this edition";
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
        GTEST_SKIP() << "GPU not available in this edition";
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
        GTEST_SKIP() << "GPU not available in this edition";
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
        GTEST_SKIP() << "GPU not available in this edition";
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
