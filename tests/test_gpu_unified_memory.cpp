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
    for (auto& th : threads) {
      th.join();
    }

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
    for (auto& th : threads) {
      th.join();
    }

    // All threads must observe the same value (CPU-only → 0 / false).
    for (int t = 1; t < THREADS; ++t) {
        EXPECT_EQ(results[t], results[0]);
    }
}
