/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_gpu_memory_pool.cpp                           ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 11:03:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     279                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f6d2a1ab9  2026-02-20  GPU module: production-ready implementation — memory mana... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "themis/gpu/memory_pool.h"
#include <thread>
#include <vector>
#include <atomic>

using namespace themis::gpu;

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST(GPUMemoryPoolTest, Construct_SlabsAllocated) {
    GPUMemoryPool pool(8ULL * 1024 * 1024 * 1024,  // 8 GB
                       256ULL * 1024 * 1024,         // 256 MB slabs
                       0);
    EXPECT_EQ(pool.numSlabs(), 32u);  // 8 GB / 256 MB = 32
    EXPECT_EQ(pool.freeSlabs(), 32u);
}

TEST(GPUMemoryPoolTest, Construct_ZeroSlabThrows) {
    EXPECT_THROW(
        (GPUMemoryPool(1024, 0)),
        std::invalid_argument);
}

TEST(GPUMemoryPoolTest, Construct_ExplicitNumSlabs) {
    GPUMemoryPool pool(1024 * 1024 * 1024ULL, 64 * 1024 * 1024ULL, 4);
    EXPECT_EQ(pool.numSlabs(), 4u);
}

// ---------------------------------------------------------------------------
// tryAcquire — success
// ---------------------------------------------------------------------------

TEST(GPUMemoryPoolTest, TryAcquire_SmallRequest_Succeeds) {
    GPUMemoryPool pool(4 * 256ULL * 1024 * 1024,  // 4 slabs
                       256ULL * 1024 * 1024, 4);
    uint64_t offset = 0;
    EXPECT_TRUE(pool.tryAcquire(64 * 1024 * 1024ULL, "test", offset));
    EXPECT_EQ(pool.freeSlabs(), 3u);
}

TEST(GPUMemoryPoolTest, TryAcquire_ExactSlabSize_Succeeds) {
    const uint64_t slab_sz = 256ULL * 1024 * 1024;
    GPUMemoryPool pool(slab_sz * 2, slab_sz, 2);
    uint64_t offset = 0;
    EXPECT_TRUE(pool.tryAcquire(slab_sz, "exact", offset));
}

TEST(GPUMemoryPoolTest, TryAcquire_OffsetMonotonicallyIncreases) {
    const uint64_t slab_sz = 256ULL * 1024 * 1024;
    GPUMemoryPool pool(slab_sz * 4, slab_sz, 4);
    uint64_t o1 = 0, o2 = 0, o3 = 0;
    ASSERT_TRUE(pool.tryAcquire(slab_sz, "a", o1));
    ASSERT_TRUE(pool.tryAcquire(slab_sz, "b", o2));
    ASSERT_TRUE(pool.tryAcquire(slab_sz, "c", o3));
    EXPECT_LT(o1, o2);
    EXPECT_LT(o2, o3);
}

// ---------------------------------------------------------------------------
// tryAcquire — failure
// ---------------------------------------------------------------------------

TEST(GPUMemoryPoolTest, TryAcquire_RequestTooLarge_Miss) {
    const uint64_t slab_sz = 256ULL * 1024 * 1024;
    GPUMemoryPool pool(slab_sz * 2, slab_sz, 2);
    uint64_t offset = 0;
    // Request larger than slab_size → pool miss.
    EXPECT_FALSE(pool.tryAcquire(slab_sz + 1, "big", offset));
    EXPECT_EQ(pool.getStats().alloc_misses, 1u);
}

TEST(GPUMemoryPoolTest, TryAcquire_PoolFull_Miss) {
    const uint64_t slab_sz = 64ULL * 1024 * 1024;
    GPUMemoryPool pool(slab_sz * 2, slab_sz, 2);
    uint64_t o = 0;
    ASSERT_TRUE(pool.tryAcquire(slab_sz, "a", o));
    ASSERT_TRUE(pool.tryAcquire(slab_sz, "b", o));
    EXPECT_FALSE(pool.tryAcquire(slab_sz, "c", o));
    EXPECT_EQ(pool.getStats().alloc_misses, 1u);
}

// ---------------------------------------------------------------------------
// release
// ---------------------------------------------------------------------------

TEST(GPUMemoryPoolTest, Release_FreesSlabForReuse) {
    const uint64_t slab_sz = 64ULL * 1024 * 1024;
    GPUMemoryPool pool(slab_sz, slab_sz, 1);
    uint64_t offset = 0;
    ASSERT_TRUE(pool.tryAcquire(slab_sz, "work", offset));
    EXPECT_EQ(pool.freeSlabs(), 0u);

    EXPECT_TRUE(pool.release(offset));
    EXPECT_EQ(pool.freeSlabs(), 1u);

    // Should be reusable.
    EXPECT_TRUE(pool.tryAcquire(slab_sz, "work2", offset));
}

TEST(GPUMemoryPoolTest, Release_WrongOffset_ReturnsFalse) {
    const uint64_t slab_sz = 64ULL * 1024 * 1024;
    GPUMemoryPool pool(slab_sz, slab_sz, 1);
    uint64_t o = 0;
    pool.tryAcquire(slab_sz, "x", o);
    EXPECT_FALSE(pool.release(o + 1));  // wrong offset
}

TEST(GPUMemoryPoolTest, Release_AlreadyFree_ReturnsFalse) {
    const uint64_t slab_sz = 64ULL * 1024 * 1024;
    GPUMemoryPool pool(slab_sz, slab_sz, 1);
    // Never acquired — releasing offset 0 (free slab) must return false.
    EXPECT_FALSE(pool.release(0));
}

// ---------------------------------------------------------------------------
// Stats
// ---------------------------------------------------------------------------

TEST(GPUMemoryPoolTest, Stats_AllocHits_Counted) {
    const uint64_t slab_sz = 64ULL * 1024 * 1024;
    GPUMemoryPool pool(slab_sz * 4, slab_sz, 4);
    uint64_t o = 0;
    pool.tryAcquire(slab_sz, "a", o);
    pool.tryAcquire(slab_sz, "b", o);
    EXPECT_EQ(pool.getStats().alloc_hits, 2u);
}

TEST(GPUMemoryPoolTest, Stats_PeakTrack) {
    const uint64_t slab_sz = 64ULL * 1024 * 1024;
    GPUMemoryPool pool(slab_sz * 4, slab_sz, 4);
    uint64_t o = 0;
    pool.tryAcquire(slab_sz, "a", o);
    pool.tryAcquire(slab_sz, "b", o);
    const uint64_t peak = pool.getStats().peak_bytes;
    pool.release(o);  // free one
    EXPECT_EQ(pool.getStats().peak_bytes, peak);  // peak must not decrease
}

TEST(GPUMemoryPoolTest, Stats_FreeBytes_ConsistentAfterRelease) {
    const uint64_t slab_sz = 64ULL * 1024 * 1024;
    GPUMemoryPool pool(slab_sz * 2, slab_sz, 2);
    uint64_t o1 = 0, o2 = 0;
    pool.tryAcquire(slab_sz, "a", o1);
    pool.tryAcquire(slab_sz, "b", o2);
    EXPECT_EQ(pool.getStats().free_bytes, 0u);

    pool.release(o1);
    EXPECT_EQ(pool.getStats().free_bytes, slab_sz);
}

// ---------------------------------------------------------------------------
// Slab snapshot
// ---------------------------------------------------------------------------

TEST(GPUMemoryPoolTest, SlabSnapshot_ShowsOccupiedTags) {
    const uint64_t slab_sz = 64ULL * 1024 * 1024;
    GPUMemoryPool pool(slab_sz * 3, slab_sz, 3);
    uint64_t o = 0;
    pool.tryAcquire(slab_sz, "index_loader", o);

    const auto snap = pool.slabSnapshot();
    ASSERT_EQ(snap.size(), 3u);
    bool found = false;
    for (const auto& s : snap) {
        if (!s.is_free && s.owner_tag == "index_loader") found = true;
    }
    EXPECT_TRUE(found);
}

// ---------------------------------------------------------------------------
// Concurrent safety
// ---------------------------------------------------------------------------

TEST(GPUMemoryPoolTest, Concurrent_AllocRelease_NoCounterDrift) {
    const uint64_t slab_sz = 64ULL * 1024 * 1024;
    GPUMemoryPool pool(slab_sz * 64, slab_sz, 64);
    constexpr int THREADS = 8, OPS = 8;

    std::atomic<int> misses{0};
    std::vector<std::thread> threads;
    threads.reserve(THREADS);

    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < OPS; ++i) {
                uint64_t offset = 0;
                if (pool.tryAcquire(slab_sz, "concurrent", offset)) {
                    pool.release(offset);
                } else {
                    misses.fetch_add(1);
                }
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(pool.freeSlabs(), pool.numSlabs());
}

// ===========================================================================
// Zero-on-free tests
// ===========================================================================

TEST(GPUMemoryPoolZeroOnFreeTest, DefaultIsDisabled) {
    GPUMemoryPool pool(1024 * 1024, 256 * 1024, 4);
    EXPECT_FALSE(pool.getZeroOnFree());
    EXPECT_EQ(pool.getStats().zeroed_slabs, 0u);
}

TEST(GPUMemoryPoolZeroOnFreeTest, EnableZeroOnFree_SetsFlag) {
    GPUMemoryPool pool(1024 * 1024, 256 * 1024, 4);
    pool.setZeroOnFree(true);
    EXPECT_TRUE(pool.getZeroOnFree());
}

TEST(GPUMemoryPoolZeroOnFreeTest, Release_IncrementsZeroedSlabsCount) {
    const uint64_t slab = 256 * 1024;
    GPUMemoryPool pool(4 * slab, slab, 4);
    pool.setZeroOnFree(true);

    uint64_t offset = 0;
    ASSERT_TRUE(pool.tryAcquire(slab, "tenant_a", offset));
    EXPECT_EQ(pool.getStats().zeroed_slabs, 0u);

    EXPECT_TRUE(pool.release(offset));
    EXPECT_EQ(pool.getStats().zeroed_slabs, 1u);
}

TEST(GPUMemoryPoolZeroOnFreeTest, Release_NoZeroWhenDisabled) {
    const uint64_t slab = 256 * 1024;
    GPUMemoryPool pool(4 * slab, slab, 4);
    // zero_on_free defaults to false

    uint64_t offset = 0;
    ASSERT_TRUE(pool.tryAcquire(slab, "tenant_b", offset));
    EXPECT_TRUE(pool.release(offset));
    EXPECT_EQ(pool.getStats().zeroed_slabs, 0u);
}

TEST(GPUMemoryPoolZeroOnFreeTest, MultipleReleases_AccumulatesCount) {
    const uint64_t slab = 128 * 1024;
    GPUMemoryPool pool(4 * slab, slab, 4);
    pool.setZeroOnFree(true);

    uint64_t o1 = 0, o2 = 0;
    ASSERT_TRUE(pool.tryAcquire(slab, "t1", o1));
    ASSERT_TRUE(pool.tryAcquire(slab, "t2", o2));
    pool.release(o1);
    pool.release(o2);
    EXPECT_EQ(pool.getStats().zeroed_slabs, 2u);
}
