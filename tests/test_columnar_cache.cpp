/**
 * @file test_columnar_cache.cpp
 * @brief Focused unit tests for storage::ColumnarCache and PinGuard.
 *
 * Test IDs: CC-01 … CC-12
 */

#include <gtest/gtest.h>
#include "storage/columnar_cache.h"

#include <atomic>
#include <string>
#include <thread>
#include <vector>

using namespace themis::storage;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static ColumnSegment makeInt64Segment(const std::string& table,
                                      const std::string& col,
                                      uint64_t seg_id,
                                      const std::vector<int64_t>& values) {
    ColumnSegment seg;
    seg.key         = {table, col, seg_id};
    seg.dtype       = SegmentDType::Int64;
    seg.int64_data  = values;
    seg.null_bitmap.assign(values.size(), false);
    seg.row_count   = values.size();
    return seg;
}

// ===========================================================================
// CC-01  put + contains
// ===========================================================================
TEST(ColumnarCacheTest, CC01_PutContains) {
    ColumnarCache cache;
    EXPECT_FALSE(cache.contains({"t", "c", 0}));
    cache.put(makeInt64Segment("t", "c", 0, {1, 2, 3}));
    EXPECT_TRUE(cache.contains({"t", "c", 0}));
    EXPECT_EQ(cache.size(), 1u);
}

// ===========================================================================
// CC-02  get() returns valid PinGuard on hit
// ===========================================================================
TEST(ColumnarCacheTest, CC02_GetHit) {
    ColumnarCache cache;
    cache.put(makeInt64Segment("t", "c", 0, {10, 20, 30}));
    auto guard = cache.get({"t", "c", 0});
    ASSERT_TRUE(guard);
    const auto& seg = guard.segment();
    EXPECT_EQ(seg.row_count, 3u);
    EXPECT_EQ(seg.int64_data[1], 20);
    EXPECT_EQ(cache.hitCount(), 1u);
}

// ===========================================================================
// CC-03  get() returns empty guard on miss
// ===========================================================================
TEST(ColumnarCacheTest, CC03_GetMiss) {
    ColumnarCache cache;
    auto guard = cache.get({"t", "c", 99});
    EXPECT_FALSE(guard);
    EXPECT_EQ(cache.missCount(), 1u);
}

// ===========================================================================
// CC-04  PinGuard: pinned segment is not evicted
// ===========================================================================
TEST(ColumnarCacheTest, CC04_PinnedNotEvicted) {
    ColumnarCache::Config cfg;
    cfg.max_bytes = 1; // tiny limit
    ColumnarCache cache(cfg);
    cache.put(makeInt64Segment("t", "c", 0, {42}));
    auto guard = cache.get({"t", "c", 0}); // pin it
    EXPECT_TRUE(guard);
    EXPECT_EQ(cache.pinnedCount(), 1u);

    // Putting another large segment triggers eviction of unpinned segments.
    // The pinned one must survive.
    cache.put(makeInt64Segment("t", "c", 1, {1, 2, 3, 4, 5, 6, 7, 8}));
    EXPECT_TRUE(cache.contains({"t", "c", 0})); // still present (pinned)
}

// ===========================================================================
// CC-05  PinGuard release decrements pin count
// ===========================================================================
TEST(ColumnarCacheTest, CC05_PinReleaseDecrements) {
    ColumnarCache cache;
    cache.put(makeInt64Segment("t", "c", 0, {1}));
    {
        auto guard = cache.get({"t", "c", 0});
        EXPECT_EQ(cache.pinnedCount(), 1u);
    } // guard destroyed here
    EXPECT_EQ(cache.pinnedCount(), 0u);
}

// ===========================================================================
// CC-06  LRU eviction: oldest unpinned segment is evicted first
// ===========================================================================
TEST(ColumnarCacheTest, CC06_LRUEviction) {
    // Each int64 segment: 1 row × 8 bytes + 1 null byte = 9 bytes.
    // max_bytes = 18 → fits exactly 2 segments.
    ColumnarCache::Config cfg;
    cfg.max_bytes = 18;
    ColumnarCache cache(cfg);

    cache.put(makeInt64Segment("t", "c", 0, {0})); // oldest
    cache.put(makeInt64Segment("t", "c", 1, {1}));
    // Accessing seg 0 makes it MRU.
    { auto g = cache.get({"t", "c", 0}); (void)g; }
    // Insert a third segment — seg 1 should be evicted (LRU).
    cache.put(makeInt64Segment("t", "c", 2, {2}));

    EXPECT_FALSE(cache.contains({"t", "c", 1})); // evicted (was LRU)
    EXPECT_TRUE(cache.contains({"t", "c", 0}));
    EXPECT_TRUE(cache.contains({"t", "c", 2}));
}

// ===========================================================================
// CC-07  explicit evict()
// ===========================================================================
TEST(ColumnarCacheTest, CC07_ExplicitEvict) {
    ColumnarCache cache;
    cache.put(makeInt64Segment("t", "c", 0, {1, 2}));
    EXPECT_TRUE(cache.evict({"t", "c", 0}));
    EXPECT_FALSE(cache.contains({"t", "c", 0}));
    EXPECT_EQ(cache.size(), 0u);
    // Evict non-existent key.
    EXPECT_FALSE(cache.evict({"t", "c", 99}));
}

// ===========================================================================
// CC-08  evict() fails when segment is pinned
// ===========================================================================
TEST(ColumnarCacheTest, CC08_EvictPinnedFails) {
    ColumnarCache cache;
    cache.put(makeInt64Segment("t", "c", 0, {7}));
    auto guard = cache.get({"t", "c", 0});
    EXPECT_FALSE(cache.evict({"t", "c", 0})); // pinned → cannot evict
    EXPECT_TRUE(cache.contains({"t", "c", 0}));
}

// ===========================================================================
// CC-09  clear() removes all unpinned segments
// ===========================================================================
TEST(ColumnarCacheTest, CC09_ClearUnpinned) {
    ColumnarCache cache;
    cache.put(makeInt64Segment("t", "a", 0, {1}));
    cache.put(makeInt64Segment("t", "b", 0, {2}));
    auto guard = cache.get({"t", "a", 0}); // pin "a"
    cache.clear();
    EXPECT_TRUE(cache.contains({"t", "a", 0}));  // pinned → not cleared
    EXPECT_FALSE(cache.contains({"t", "b", 0})); // unpinned → cleared
}

// ===========================================================================
// CC-10  on_evict callback is invoked
// ===========================================================================
TEST(ColumnarCacheTest, CC10_OnEvictCallback) {
    std::vector<SegmentKey> evicted;
    ColumnarCache::Config cfg;
    cfg.max_bytes = 9; // fits 1 int64 row
    cfg.on_evict = [&evicted](const SegmentKey& k) { evicted.push_back(k); };
    ColumnarCache cache(cfg);

    cache.put(makeInt64Segment("t", "c", 0, {1}));
    cache.put(makeInt64Segment("t", "c", 1, {2})); // triggers eviction of seg 0

    EXPECT_EQ(evicted.size(), 1u);
    EXPECT_EQ(evicted[0].segment_id, 0u);
}

// ===========================================================================
// CC-11  bytesUsed() tracks correctly after put/evict
// ===========================================================================
TEST(ColumnarCacheTest, CC11_BytesUsedTracking) {
    ColumnarCache cache;
    EXPECT_EQ(cache.bytesUsed(), 0u);

    auto seg = makeInt64Segment("t", "c", 0, {1, 2, 3});
    size_t expected = seg.byteSize();
    cache.put(seg);
    EXPECT_EQ(cache.bytesUsed(), expected);

    cache.evict({"t", "c", 0});
    EXPECT_EQ(cache.bytesUsed(), 0u);
}

// ===========================================================================
// CC-12  thread-safe concurrent put + get from multiple threads
// ===========================================================================
TEST(ColumnarCacheTest, CC12_ConcurrentAccess) {
    constexpr int N = 8;
    constexpr int OPS = 200;
    ColumnarCache::Config cfg;
    cfg.max_bytes = 1024 * 1024;
    ColumnarCache cache(cfg);
    std::atomic<int> errors{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < N; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < OPS; ++i) {
                uint64_t seg_id = static_cast<uint64_t>(t * OPS + i);
                cache.put(makeInt64Segment("tbl", "col", seg_id,
                                          {static_cast<int64_t>(seg_id)}));
                auto guard = cache.get({"tbl", "col", seg_id});
                if (guard) {
                    if (guard.segment().int64_data[0] != static_cast<int64_t>(seg_id)) {
                        errors.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            }
        });
    }
    for (auto& th : threads) th.join();
    EXPECT_EQ(errors.load(), 0);
}
