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
