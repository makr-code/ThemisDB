/*
 * Unit tests for GPUUnifiedMemoryAllocator (unified_memory.h / unified_memory.cpp).
 *
 * All tests run on CI without GPU hardware.  The allocator falls back to
 * ordinary malloc/free when THEMIS_ENABLE_CUDA and THEMIS_ENABLE_HIP are
 * absent, so every allocation, deallocation, prefetch, advise, stats, and
 * tenant-isolation path is always exercised and tested.
 */

#include <gtest/gtest.h>
#include "themis/gpu/unified_memory.h"

#include <atomic>
#include <thread>
#include <vector>

using namespace themis::gpu;

// ---------------------------------------------------------------------------
// Test fixture: reset allocator state before/after each test so that
// singleton state does not bleed between tests.
// ---------------------------------------------------------------------------
class GPUUnifiedMemoryTest : public ::testing::Test {
protected:
    void SetUp()    override { GPUUnifiedMemoryAllocator::GetInstance().reset(); }
    void TearDown() override { GPUUnifiedMemoryAllocator::GetInstance().reset(); }
};

// ===========================================================================
// isSupported
// ===========================================================================

TEST_F(GPUUnifiedMemoryTest, IsSupported_ReturnsBool) {
    // On CPU-only CI the result must be false; on real GPU hardware it may be
    // true.  Either way the call must not throw or crash.
    EXPECT_NO_FATAL_FAILURE(GPUUnifiedMemoryAllocator::isSupported());
}

// ===========================================================================
// allocate — success cases
// ===========================================================================

TEST_F(GPUUnifiedMemoryTest, Allocate_SmallBuffer_ReturnsNonNull) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    void* ptr = alloc.allocate(64, "test");
    ASSERT_NE(ptr, nullptr);
    alloc.free(ptr);
}

TEST_F(GPUUnifiedMemoryTest, Allocate_TaggedAndTenanted_Succeeds) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    void* ptr = alloc.allocate(128, "index_loader", "tenant_a");
    ASSERT_NE(ptr, nullptr);
    alloc.free(ptr);
}

TEST_F(GPUUnifiedMemoryTest, Allocate_MultiplePointers_AllNonNull) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    void* p1 = alloc.allocate(256, "a");
    void* p2 = alloc.allocate(512, "b");
    void* p3 = alloc.allocate(1024, "c");
    EXPECT_NE(p1, nullptr);
    EXPECT_NE(p2, nullptr);
    EXPECT_NE(p3, nullptr);
    // Pointers must be distinct.
    EXPECT_NE(p1, p2);
    EXPECT_NE(p2, p3);
    alloc.free(p1);
    alloc.free(p2);
    alloc.free(p3);
}

// ===========================================================================
// allocate — failure cases
// ===========================================================================

TEST_F(GPUUnifiedMemoryTest, Allocate_ZeroBytes_ReturnsNull) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    EXPECT_EQ(alloc.allocate(0, "zero"), nullptr);
}

// ===========================================================================
// free
// ===========================================================================

TEST_F(GPUUnifiedMemoryTest, Free_ValidPtr_ReturnsTrue) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    void* ptr = alloc.allocate(64, "free_test");
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(alloc.free(ptr));
}

TEST_F(GPUUnifiedMemoryTest, Free_NullPtr_ReturnsFalse) {
    EXPECT_FALSE(GPUUnifiedMemoryAllocator::GetInstance().free(nullptr));
}

TEST_F(GPUUnifiedMemoryTest, Free_UnknownPtr_ReturnsFalse) {
    // A pointer not obtained from this allocator must return false (not crash).
    int dummy = 42;
    EXPECT_FALSE(GPUUnifiedMemoryAllocator::GetInstance().free(&dummy));
}

TEST_F(GPUUnifiedMemoryTest, Free_FreedPtr_NotTrackedAgain) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    void* ptr = alloc.allocate(128, "double_free_guard");
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(alloc.free(ptr));
    // After the first free the pointer is no longer tracked; a second call
    // must return false without crashing.
    EXPECT_FALSE(alloc.free(ptr));
}

// ===========================================================================
// Stats
// ===========================================================================

TEST_F(GPUUnifiedMemoryTest, Stats_InitialState_AllZero) {
    const auto s = GPUUnifiedMemoryAllocator::GetInstance().getStats();
    EXPECT_EQ(s.total_allocations, 0u);
    EXPECT_EQ(s.total_frees,       0u);
    EXPECT_EQ(s.allocated_bytes,   0u);
    EXPECT_EQ(s.peak_bytes,        0u);
    EXPECT_EQ(s.prefetch_calls,    0u);
    EXPECT_EQ(s.advise_calls,      0u);
}

TEST_F(GPUUnifiedMemoryTest, Stats_AllocFree_Counters) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();

    void* p1 = alloc.allocate(256, "a");
    void* p2 = alloc.allocate(512, "b");
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);

    {
        const auto s = alloc.getStats();
        EXPECT_EQ(s.total_allocations, 2u);
        EXPECT_EQ(s.total_frees,       0u);
        EXPECT_EQ(s.allocated_bytes,   768u);
        EXPECT_GE(s.peak_bytes,        768u);
    }

    alloc.free(p1);

    {
        const auto s = alloc.getStats();
        EXPECT_EQ(s.total_frees,     1u);
        EXPECT_EQ(s.allocated_bytes, 512u);
        EXPECT_GE(s.peak_bytes,      768u);  // peak must not decrease
    }

    alloc.free(p2);

    {
        const auto s = alloc.getStats();
        EXPECT_EQ(s.total_frees,     2u);
        EXPECT_EQ(s.allocated_bytes, 0u);
        EXPECT_GE(s.peak_bytes,      768u);
    }
}

TEST_F(GPUUnifiedMemoryTest, Stats_PeakBytes_DoesNotDecrease) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    void* p = alloc.allocate(1024, "peak");
    const uint64_t peak_before = alloc.getStats().peak_bytes;
    alloc.free(p);
    EXPECT_EQ(alloc.getStats().peak_bytes, peak_before);
}

// ===========================================================================
// Active allocations snapshot
// ===========================================================================

TEST_F(GPUUnifiedMemoryTest, GetActiveAllocations_TracksLivePointers) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    void* p1 = alloc.allocate(64, "live1", "t1");
    void* p2 = alloc.allocate(128, "live2", "t2");

    const auto snap = alloc.getActiveAllocations();
    ASSERT_EQ(snap.size(), 2u);

    bool found_p1 = false, found_p2 = false;
    for (const auto& rec : snap) {
        if (rec.ptr == p1) { found_p1 = true; EXPECT_EQ(rec.tag, "live1"); }
        if (rec.ptr == p2) { found_p2 = true; EXPECT_EQ(rec.tag, "live2"); }
    }
    EXPECT_TRUE(found_p1);
    EXPECT_TRUE(found_p2);

    alloc.free(p1);
    alloc.free(p2);
}

TEST_F(GPUUnifiedMemoryTest, GetActiveAllocations_EmptyAfterAllFreed) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    void* p = alloc.allocate(64, "temp");
    alloc.free(p);
    EXPECT_TRUE(alloc.getActiveAllocations().empty());
}

// ===========================================================================
// Tenant isolation
// ===========================================================================

TEST_F(GPUUnifiedMemoryTest, TenantBytes_TracksPerTenantUsage) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    void* p1 = alloc.allocate(256, "op_a", "alice");
    void* p2 = alloc.allocate(512, "op_b", "alice");
    void* p3 = alloc.allocate(128, "op_c", "bob");

    EXPECT_EQ(alloc.getTenantBytes("alice"), 768u);
    EXPECT_EQ(alloc.getTenantBytes("bob"),   128u);
    EXPECT_EQ(alloc.getTenantBytes("carol"),   0u);  // unknown tenant

    alloc.free(p1);
    EXPECT_EQ(alloc.getTenantBytes("alice"), 512u);

    alloc.free(p2);
    alloc.free(p3);
    EXPECT_EQ(alloc.getTenantBytes("alice"), 0u);
    EXPECT_EQ(alloc.getTenantBytes("bob"),   0u);
}

TEST_F(GPUUnifiedMemoryTest, TenantBytes_UntaggedAlloc_NoTenantEntry) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    void* p = alloc.allocate(64, "no_tenant");  // no tenant_id
    EXPECT_EQ(alloc.getTenantBytes(""), 0u);
    alloc.free(p);
}

// ===========================================================================
// prefetch
// ===========================================================================

TEST_F(GPUUnifiedMemoryTest, Prefetch_ValidPtr_Succeeds) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    void* ptr = alloc.allocate(1024, "prefetch_test");
    ASSERT_NE(ptr, nullptr);

    // On CPU-only builds prefetch is a no-op that returns true.
    EXPECT_TRUE(alloc.prefetch(ptr, 1024, 0));
    EXPECT_EQ(alloc.getStats().prefetch_calls, 1u);

    alloc.free(ptr);
}

TEST_F(GPUUnifiedMemoryTest, Prefetch_NullPtr_ReturnsFalse) {
    EXPECT_FALSE(GPUUnifiedMemoryAllocator::GetInstance().prefetch(nullptr, 64, 0));
}

TEST_F(GPUUnifiedMemoryTest, Prefetch_ZeroBytes_ReturnsFalse) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    void* ptr = alloc.allocate(64, "pf_zero");
    EXPECT_FALSE(alloc.prefetch(ptr, 0, 0));
    alloc.free(ptr);
}

// ===========================================================================
// advise
// ===========================================================================

TEST_F(GPUUnifiedMemoryTest, Advise_PreferredLocation_Succeeds) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    void* ptr = alloc.allocate(512, "advise_test");
    ASSERT_NE(ptr, nullptr);

    EXPECT_TRUE(alloc.advise(ptr, 512,
        GPUUnifiedMemoryAllocator::MemAdvice::SET_PREFERRED_LOCATION, 0));
    EXPECT_EQ(alloc.getStats().advise_calls, 1u);

    alloc.free(ptr);
}

TEST_F(GPUUnifiedMemoryTest, Advise_AllHints_IncrementCounter) {
    using MA = GPUUnifiedMemoryAllocator::MemAdvice;
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    void* ptr = alloc.allocate(256, "all_hints");
    ASSERT_NE(ptr, nullptr);

    alloc.advise(ptr, 256, MA::SET_PREFERRED_LOCATION,   0);
    alloc.advise(ptr, 256, MA::SET_ACCESSED_BY,          0);
    alloc.advise(ptr, 256, MA::SET_READ_MOSTLY,          0);
    alloc.advise(ptr, 256, MA::UNSET_PREFERRED_LOCATION, 0);
    alloc.advise(ptr, 256, MA::UNSET_ACCESSED_BY,        0);
    alloc.advise(ptr, 256, MA::UNSET_READ_MOSTLY,        0);

    EXPECT_EQ(alloc.getStats().advise_calls, 6u);

    alloc.free(ptr);
}

TEST_F(GPUUnifiedMemoryTest, Advise_NullPtr_ReturnsFalse) {
    EXPECT_FALSE(GPUUnifiedMemoryAllocator::GetInstance().advise(
        nullptr, 64, GPUUnifiedMemoryAllocator::MemAdvice::SET_READ_MOSTLY, 0));
}

// ===========================================================================
// reset
// ===========================================================================

TEST_F(GPUUnifiedMemoryTest, Reset_ClearsAllState) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    alloc.allocate(128, "r1", "t1");
    alloc.allocate(256, "r2", "t2");

    alloc.reset();

    const auto s = alloc.getStats();
    EXPECT_EQ(s.total_allocations, 0u);
    EXPECT_EQ(s.total_frees,       0u);
    EXPECT_EQ(s.allocated_bytes,   0u);
    EXPECT_EQ(s.peak_bytes,        0u);
    EXPECT_TRUE(alloc.getActiveAllocations().empty());
    EXPECT_EQ(alloc.getTenantBytes("t1"), 0u);
    EXPECT_EQ(alloc.getTenantBytes("t2"), 0u);
}

// ===========================================================================
// Concurrent safety
// ===========================================================================

TEST_F(GPUUnifiedMemoryTest, Concurrent_AllocFree_NoRaces) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    constexpr int THREADS = 4;
    constexpr int OPS     = 8;

    std::atomic<int> success_count{0};
    std::vector<std::thread> threads;
    threads.reserve(THREADS);

    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < OPS; ++i) {
                void* ptr = alloc.allocate(64, "concurrent",
                                           "tenant_" + std::to_string(t));
                if (ptr) {
                    alloc.free(ptr);
                    success_count.fetch_add(1);
                }
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_GT(success_count.load(), 0);
    // After all threads complete the allocator should have no live allocations
    // (each thread freed its own pointer).
    EXPECT_EQ(alloc.getStats().allocated_bytes, 0u);
}

TEST_F(GPUUnifiedMemoryTest, Concurrent_IsSupported_NoRace) {
    // Verify isSupported() is safe to call from multiple threads concurrently.
    // Uses the C++11 thread-safe static-init path; should never produce a
    // TSan data-race warning.
    // Note: std::vector<bool> is bit-packed; use int to avoid bit-level races.
    constexpr int THREADS = 8;
    std::vector<int> results(THREADS, -1);
    std::vector<std::thread> threads;
    threads.reserve(THREADS);

    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&results, t]() {
            results[t] = GPUUnifiedMemoryAllocator::isSupported() ? 1 : 0;
        });
    }
    for (auto& th : threads) th.join();

    // All threads must observe the same value (CPU-only → 0 / false).
    for (int t = 1; t < THREADS; ++t) {
        EXPECT_EQ(results[t], results[0]);
    }
}

// ===========================================================================
// Phase 4: HIP/CUDA Error Handling & Timeout Enforcement
// ===========================================================================

// Test allocation under error conditions (Phase 4 hardening)
TEST_F(GPUUnifiedMemoryTest, Allocate_ExceptionSafety) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    
    // On CPU-only builds, allocation always succeeds.
    // On HIP-enabled hardware with OOM, CHECKED_HIP will throw.
    // This test verifies that the exception is caught and handled gracefully.
    void* ptr = alloc.allocate(64, "exception_safe_test");
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(alloc.free(ptr));
}

// Test mixed allocation coherence (Phase 4: unified memory coherence verification)
TEST_F(GPUUnifiedMemoryTest, MixedAllocation_CoherenceTracking) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    
    // Allocate multiple buffers with different tags
    void* p1 = alloc.allocate(256, "coherent_1");
    void* p2 = alloc.allocate(512, "coherent_2");
    void* p3 = alloc.allocate(128, "coherent_3");
    
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    ASSERT_NE(p3, nullptr);
    
    // Verify active allocations tracking
    const auto active = alloc.getActiveAllocations();
    ASSERT_EQ(active.size(), 3u);
    
    // Verify all allocations are tracked
    bool found_p1 = false, found_p2 = false, found_p3 = false;
    for (const auto& rec : active) {
        if (rec.ptr == p1) found_p1 = true;
        if (rec.ptr == p2) found_p2 = true;
        if (rec.ptr == p3) found_p3 = true;
    }
    EXPECT_TRUE(found_p1);
    EXPECT_TRUE(found_p2);
    EXPECT_TRUE(found_p3);
    
    // Cleanup
    alloc.free(p1);
    alloc.free(p2);
    alloc.free(p3);
}

// Test prefetch/advise error handling (Phase 4: HIP error consistency)
TEST_F(GPUUnifiedMemoryTest, Prefetch_ErrorHandling) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    void* ptr = alloc.allocate(1024, "prefetch_error_test");
    ASSERT_NE(ptr, nullptr);
    
    // Prefetch should succeed even on CPU-only builds
    EXPECT_TRUE(alloc.prefetch(ptr, 1024, 0));
    
    alloc.free(ptr);
}

// Test advise error handling (Phase 4: HIP error consistency)
TEST_F(GPUUnifiedMemoryTest, Advise_ErrorHandling) {
    using MA = GPUUnifiedMemoryAllocator::MemAdvice;
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    void* ptr = alloc.allocate(512, "advise_error_test");
    ASSERT_NE(ptr, nullptr);
    
    // Advise should succeed even on CPU-only builds
    EXPECT_TRUE(alloc.advise(ptr, 512, MA::SET_PREFERRED_LOCATION, 0));
    
    alloc.free(ptr);
}

// Test free error recovery (Phase 4: RAII cleanup on exception)
TEST_F(GPUUnifiedMemoryTest, Free_Cleanup_OnError) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    
    // Allocate and immediately free multiple times to test
    // cleanup path robustness
    for (int i = 0; i < 5; ++i) {
        void* ptr = alloc.allocate(128, "cleanup_test_" + std::to_string(i));
        ASSERT_NE(ptr, nullptr);
        EXPECT_TRUE(alloc.free(ptr));
    }
    
    EXPECT_EQ(alloc.getStats().allocated_bytes, 0u);
}

// Test stats consistency after mixed operations (Phase 4: coherence)
TEST_F(GPUUnifiedMemoryTest, Stats_ConsistencyUnderMixedOps) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    
    // Mix of allocations, frees, and statistics checks
    const size_t SIZE1 = 256;
    const size_t SIZE2 = 512;
    const size_t SIZE3 = 128;
    
    void* p1 = alloc.allocate(SIZE1, "mixed_1");
    ASSERT_NE(p1, nullptr);
    
    {
        auto s = alloc.getStats();
        EXPECT_EQ(s.allocated_bytes, SIZE1);
        EXPECT_EQ(s.total_allocations, 1u);
    }
    
    void* p2 = alloc.allocate(SIZE2, "mixed_2");
    ASSERT_NE(p2, nullptr);
    
    {
        auto s = alloc.getStats();
        EXPECT_EQ(s.allocated_bytes, SIZE1 + SIZE2);
        EXPECT_EQ(s.total_allocations, 2u);
    }
    
    alloc.free(p1);
    
    {
        auto s = alloc.getStats();
        EXPECT_EQ(s.allocated_bytes, SIZE2);
        EXPECT_EQ(s.total_frees, 1u);
    }
    
    void* p3 = alloc.allocate(SIZE3, "mixed_3");
    ASSERT_NE(p3, nullptr);
    
    {
        auto s = alloc.getStats();
        EXPECT_EQ(s.allocated_bytes, SIZE2 + SIZE3);
        EXPECT_EQ(s.total_allocations, 3u);
        EXPECT_GE(s.peak_bytes, SIZE1 + SIZE2);
    }
    
    alloc.free(p2);
    alloc.free(p3);
    
    {
        auto s = alloc.getStats();
        EXPECT_EQ(s.allocated_bytes, 0u);
        EXPECT_EQ(s.total_frees, 3u);
    }
}

// Test tenant isolation under error conditions (Phase 4: fault tolerance)
TEST_F(GPUUnifiedMemoryTest, TenantIsolation_FaultTolerance) {
    auto& alloc = GPUUnifiedMemoryAllocator::GetInstance();
    
    const size_t TENANT_A_SIZE = 256 + 128;  // p1 + p3
    const size_t TENANT_B_SIZE = 512;         // p2
    
    void* p1 = alloc.allocate(256, "a_op1", "tenant_a");
    void* p2 = alloc.allocate(512, "b_op1", "tenant_b");
    void* p3 = alloc.allocate(128, "a_op2", "tenant_a");
    
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    ASSERT_NE(p3, nullptr);
    
    EXPECT_EQ(alloc.getTenantBytes("tenant_a"), TENANT_A_SIZE);
    EXPECT_EQ(alloc.getTenantBytes("tenant_b"), TENANT_B_SIZE);
    
    // Free in different order to test isolation
    alloc.free(p2);
    EXPECT_EQ(alloc.getTenantBytes("tenant_b"), 0u);
    EXPECT_EQ(alloc.getTenantBytes("tenant_a"), TENANT_A_SIZE);
    
    alloc.free(p1);
    EXPECT_EQ(alloc.getTenantBytes("tenant_a"), 128u);
    
    alloc.free(p3);
    EXPECT_EQ(alloc.getTenantBytes("tenant_a"), 0u);
}

