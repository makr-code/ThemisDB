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
        if (!s.is_free && s.owner_tag == "index_loader") {
          found = true;
        }
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
    for (auto& th : threads) {
      th.join();
    }

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

// ===========================================================================
// Internal fragmentation tracking fix (request_size per slab)
// ===========================================================================

TEST(GPUMemoryPoolFragTest, Release_DecrementsWastedBytes) {
    // When a slab is freed, the internal-fragmentation (wasted) bytes it
    // contributed must be subtracted so fragmentation drops back to 0.
    const uint64_t slab = 256 * 1024;
    GPUMemoryPool pool(4 * slab, slab, 4);

    uint64_t offset = 0;
    // Allocate half the slab — wastes slab/2 bytes.
    ASSERT_TRUE(pool.tryAcquire(slab / 2, "frag_test", offset));
    EXPECT_GT(pool.fragmentation(), 0.0f);

    // After release the pool is empty; fragmentation must be 0.
    EXPECT_TRUE(pool.release(offset));
    EXPECT_FLOAT_EQ(pool.fragmentation(), 0.0f);
}

TEST(GPUMemoryPoolFragTest, ExactSizeRequest_ZeroFragmentation) {
    const uint64_t slab = 64 * 1024;
    GPUMemoryPool pool(4 * slab, slab, 4);

    uint64_t o = 0;
    ASSERT_TRUE(pool.tryAcquire(slab, "exact", o));
    EXPECT_FLOAT_EQ(pool.fragmentation(), 0.0f);
}

// ===========================================================================
// Defragmentation tests
// ===========================================================================

TEST(GPUMemoryPoolDefragTest, NoAction_BelowThreshold) {
    const uint64_t slab = 256 * 1024;
    GPUMemoryPool pool(4 * slab, slab, 4);

    // Allocate exactly a slab — internal fragmentation is zero.
    uint64_t o = 0;
    ASSERT_TRUE(pool.tryAcquire(slab, "full", o));

    // Fragmentation is 0, threshold is 0.05 → defrag should not run.
    auto result = pool.defragment(0.05f);
    EXPECT_FALSE(result.ran);
    EXPECT_EQ(result.slabs_moved, 0u);
}

TEST(GPUMemoryPoolDefragTest, Runs_WhenFragAboveThreshold) {
    const uint64_t slab = 256 * 1024;
    GPUMemoryPool pool(4 * slab, slab, 4);

    // Allocate slabs with half-size requests — each wastes slab/2 bytes.
    uint64_t o1 = 0, o2 = 0;
    ASSERT_TRUE(pool.tryAcquire(slab / 2, "a", o1));
    ASSERT_TRUE(pool.tryAcquire(slab / 2, "b", o2));

    const float frag_before = pool.fragmentation();
    EXPECT_GT(frag_before, 0.0f);
    // wasted = 2*(slab - slab/2) = slab; total = 4*slab → 0.25
    EXPECT_NEAR(frag_before, 0.25f, 1e-5f);

    // Threshold of 0.0f runs whenever fragmentation > 0 (true here since
    // requests are half the slab size).
    auto result = pool.defragment(0.0f);
    EXPECT_TRUE(result.ran);
    EXPECT_FLOAT_EQ(result.frag_before, frag_before);
    // After defrag with identical allocations, internal frag is unchanged in
    // value but wasted_bytes_ is freshly calculated from request_size fields.
    EXPECT_FLOAT_EQ(result.frag_after, frag_before);
}

TEST(GPUMemoryPoolDefragTest, CompactsOccupiedSlabsToFront) {
    const uint64_t slab = 64 * 1024;
    // 4 slabs: allocate 0,1,2 with half-slab requests (creates fragmentation),
    // free slot 1 → hole at offset slab, then defrag.
    GPUMemoryPool pool(4 * slab, slab, 4);

    uint64_t o0 = 0, o1 = 0, o2 = 0;
    ASSERT_TRUE(pool.tryAcquire(slab / 2, "s0", o0));
    ASSERT_TRUE(pool.tryAcquire(slab / 2, "s1", o1));
    ASSERT_TRUE(pool.tryAcquire(slab / 2, "s2", o2));
    ASSERT_TRUE(pool.release(o1));   // free the middle slab

    // Before defrag the occupied slabs are at offsets 0 and 2*slab with a gap.
    // Fragmentation > 0 because request_size < slab_size.
    auto result = pool.defragment(0.0f);
    EXPECT_TRUE(result.ran);

    // After defrag all occupied slabs must start at a contiguous range [0, 2*slab).
    const auto snap = pool.slabSnapshot();
    uint64_t max_occupied_offset = 0;
    size_t occupied_count = 0;
    for (const auto& s : snap) {
        if (!s.is_free) {
            max_occupied_offset = std::max(max_occupied_offset, s.offset);
            ++occupied_count;
        }
    }
    EXPECT_EQ(occupied_count, 2u);
    // Two occupied slabs must fit in the first 2*slab bytes.
    EXPECT_LT(max_occupied_offset, 2 * slab);
}

TEST(GPUMemoryPoolDefragTest, FreeSlabsRemainUsableAfterDefrag) {
    const uint64_t slab = 64 * 1024;
    GPUMemoryPool pool(4 * slab, slab, 4);

    uint64_t o0 = 0, o1 = 0;
    ASSERT_TRUE(pool.tryAcquire(slab, "a", o0));
    ASSERT_TRUE(pool.tryAcquire(slab, "b", o1));
    pool.release(o0);  // free first slab, creating a hole

    pool.defragment(0.0f);

    // Pool must still have 3 free slabs and accept new allocations.
    EXPECT_EQ(pool.freeSlabs(), 3u);
    uint64_t new_off = 0;
    EXPECT_TRUE(pool.tryAcquire(slab, "new", new_off));
}

TEST(GPUMemoryPoolDefragTest, FragmentationDropsToZero_WhenAllExact) {
    const uint64_t slab = 64 * 1024;
    // Use half-slab requests to create internal fragmentation, then free
    // all but one and verify defrag recomputes wasted_bytes_ correctly.
    GPUMemoryPool pool(4 * slab, slab, 4);

    uint64_t o0 = 0, o1 = 0, o2 = 0;
    ASSERT_TRUE(pool.tryAcquire(slab / 2, "x0", o0));
    ASSERT_TRUE(pool.tryAcquire(slab / 2, "x1", o1));
    ASSERT_TRUE(pool.tryAcquire(slab,     "x2", o2));  // exact — no waste

    // Release the two half-slab allocations.
    pool.release(o0);
    pool.release(o1);

    // Only x2 (exact size) remains; wasted_bytes_ must be 0.
    pool.defragment(0.0f);
    EXPECT_FLOAT_EQ(pool.fragmentation(), 0.0f);
}

// ===========================================================================
// offset_map and device_base_ptr tests
// ===========================================================================

TEST(GPUMemoryPoolDefragTest, OffsetMap_EmptyWhenDefragNotRun) {
    const uint64_t slab = 64 * 1024;
    GPUMemoryPool pool(4 * slab, slab, 4);

    // Allocate exactly a slab — zero internal fragmentation → threshold not met.
    uint64_t o = 0;
    ASSERT_TRUE(pool.tryAcquire(slab, "full", o));

    auto result = pool.defragment(0.05f);
    EXPECT_FALSE(result.ran);
    EXPECT_TRUE(result.offset_map.empty());
}

TEST(GPUMemoryPoolDefragTest, OffsetMap_PopulatedForMovedSlabs) {
    const uint64_t slab = 64 * 1024;
    // Allocate 3 slabs with half-slab requests (creates fragmentation),
    // free the middle one, then defrag so the third slab moves forward.
    GPUMemoryPool pool(4 * slab, slab, 4);

    uint64_t o0 = 0, o1 = 0, o2 = 0;
    ASSERT_TRUE(pool.tryAcquire(slab / 2, "a", o0));
    ASSERT_TRUE(pool.tryAcquire(slab / 2, "b", o1));
    ASSERT_TRUE(pool.tryAcquire(slab / 2, "c", o2));
    pool.release(o1);   // free the middle slab, creating a gap

    auto result = pool.defragment(0.0f);
    EXPECT_TRUE(result.ran);

    // The slab that was at o2 (offset 2*slab) must appear in the map with a
    // smaller new offset.
    auto it = result.offset_map.find(o2);
    ASSERT_NE(it, result.offset_map.end());
    EXPECT_LT(it->second, o2);  // moved forward
}

TEST(GPUMemoryPoolDefragTest, OffsetMap_SlabsNotMovedAbsent) {
    const uint64_t slab = 64 * 1024;
    GPUMemoryPool pool(4 * slab, slab, 4);

    // Allocate all 4 slabs with half-size requests (fragmented, but no gaps).
    uint64_t o0 = 0, o1 = 0, o2 = 0, o3 = 0;
    ASSERT_TRUE(pool.tryAcquire(slab / 2, "s0", o0));
    ASSERT_TRUE(pool.tryAcquire(slab / 2, "s1", o1));
    ASSERT_TRUE(pool.tryAcquire(slab / 2, "s2", o2));
    ASSERT_TRUE(pool.tryAcquire(slab / 2, "s3", o3));

    // All slabs already contiguous from offset 0 — none should be moved.
    auto result = pool.defragment(0.0f);
    EXPECT_TRUE(result.ran);
    EXPECT_EQ(result.slabs_moved, 0u);
    EXPECT_TRUE(result.offset_map.empty());
}

TEST(GPUMemoryPoolDeviceBasePtrTest, DefaultIsZero) {
    GPUMemoryPool pool(4 * 64 * 1024ULL, 64 * 1024ULL, 4);
    EXPECT_EQ(pool.getDeviceBasePtr(), 0u);
}

TEST(GPUMemoryPoolDeviceBasePtrTest, SetAndGetRoundTrips) {
    GPUMemoryPool pool(4 * 64 * 1024ULL, 64 * 1024ULL, 4);
    const uintptr_t mock_device_ptr = 0xDEADBEEFULL;
    pool.setDeviceBasePtr(mock_device_ptr);
    EXPECT_EQ(pool.getDeviceBasePtr(), mock_device_ptr);
    pool.setDeviceBasePtr(0);
    EXPECT_EQ(pool.getDeviceBasePtr(), 0u);
}

TEST(GPUMemoryPoolDefragTest, DataMoveErrors_ZeroWhenNoCpuSimulation) {
    // Without a real device_base_ptr (== 0), no CUDA/HIP copy is attempted,
    // so data_move_errors must always be 0 in the CPU simulation path.
    const uint64_t slab = 64 * 1024;
    GPUMemoryPool pool(4 * slab, slab, 4);

    uint64_t o0 = 0, o1 = 0, o2 = 0;
    ASSERT_TRUE(pool.tryAcquire(slab / 2, "a", o0));
    ASSERT_TRUE(pool.tryAcquire(slab / 2, "b", o1));
    ASSERT_TRUE(pool.tryAcquire(slab / 2, "c", o2));
    pool.release(o1);  // create a gap to trigger actual moves

    auto result = pool.defragment(0.0f);
    EXPECT_TRUE(result.ran);
    EXPECT_EQ(result.data_move_errors, 0u);
}
