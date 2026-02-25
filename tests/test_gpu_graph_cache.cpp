#include <gtest/gtest.h>
#include "themis/gpu/graph_cache.h"
#include "themis/gpu/query_accelerator.h"
#include <cstring>
#include <thread>
#include <vector>

using namespace themis::gpu;

// ============================================================================
// QueryShape equality and hashing
// ============================================================================

TEST(QueryShapeTest, EqualShapesCompareEqual) {
    QueryShape a{QueryShape::OpType::SCAN, 100, 0};
    QueryShape b{QueryShape::OpType::SCAN, 100, 0};
    EXPECT_TRUE(a == b);
}

TEST(QueryShapeTest, DifferentOpTypesNotEqual) {
    QueryShape a{QueryShape::OpType::SCAN, 100, 0};
    QueryShape b{QueryShape::OpType::SORT, 100, 0};
    EXPECT_FALSE(a == b);
}

TEST(QueryShapeTest, DifferentRowCountsNotEqual) {
    QueryShape a{QueryShape::OpType::SCAN, 100, 0};
    QueryShape b{QueryShape::OpType::SCAN, 200, 0};
    EXPECT_FALSE(a == b);
}

TEST(QueryShapeTest, DifferentParamHashesNotEqual) {
    QueryShape a{QueryShape::OpType::AGGREGATE, 50, 1};
    QueryShape b{QueryShape::OpType::AGGREGATE, 50, 2};
    EXPECT_FALSE(a == b);
}

TEST(QueryShapeTest, HashFunctionProducesDifferentValuesForDifferentShapes) {
    QueryShapeHash hasher;
    QueryShape a{QueryShape::OpType::SCAN, 100, 0};
    QueryShape b{QueryShape::OpType::SORT, 100, 0};
    QueryShape c{QueryShape::OpType::SCAN, 200, 0};
    // Hashes should differ (no strict guarantee, but these are well-separated).
    EXPECT_NE(hasher(a), hasher(b));
    EXPECT_NE(hasher(a), hasher(c));
}

TEST(QueryShapeTest, HashFunctionIsConsistentForEqualShapes) {
    QueryShapeHash hasher;
    QueryShape a{QueryShape::OpType::JOIN, 500, 42};
    QueryShape b{QueryShape::OpType::JOIN, 500, 42};
    EXPECT_EQ(hasher(a), hasher(b));
}

// ============================================================================
// GPUGraphCache — basic capture / lookup
// ============================================================================

TEST(GPUGraphCacheTest, InitiallyEmpty) {
    GPUGraphCache cache;
    EXPECT_EQ(cache.size(), 0u);
}

TEST(GPUGraphCacheTest, LookupOnEmptyCacheReturnsMiss) {
    GPUGraphCache cache;
    QueryShape shape{QueryShape::OpType::SCAN, 100, 0};
    EXPECT_EQ(cache.lookup(shape), nullptr);
}

TEST(GPUGraphCacheTest, CaptureAddsEntry) {
    GPUGraphCache cache;
    QueryShape shape{QueryShape::OpType::SCAN, 100, 0};
    cache.capture(shape);
    EXPECT_EQ(cache.size(), 1u);
}

TEST(GPUGraphCacheTest, LookupAfterCaptureReturnsEntry) {
    GPUGraphCache cache;
    QueryShape shape{QueryShape::OpType::SORT, 200, 1};
    cache.capture(shape);
    const GraphEntry* entry = cache.lookup(shape);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->replay_count, 1u);
}

TEST(GPUGraphCacheTest, LookupIncrementsReplayCount) {
    GPUGraphCache cache;
    QueryShape shape{QueryShape::OpType::AGGREGATE, 50, 3};
    cache.capture(shape);
    cache.lookup(shape);
    cache.lookup(shape);
    const GraphEntry* entry = cache.lookup(shape);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->replay_count, 3u);
}

TEST(GPUGraphCacheTest, CaptureExistingShapeIncrementsCapturCount) {
    GPUGraphCache cache;
    QueryShape shape{QueryShape::OpType::SCAN, 10, 0};
    cache.capture(shape);
    cache.capture(shape);  // idempotent — same shape
    EXPECT_EQ(cache.size(), 1u);

    const GraphEntry* entry = cache.lookup(shape);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->capture_count, 2u);
}

TEST(GPUGraphCacheTest, MultipleDistinctShapesCached) {
    GPUGraphCache cache;
    cache.capture({QueryShape::OpType::SCAN,      100, 0});
    cache.capture({QueryShape::OpType::SORT,      200, 1});
    cache.capture({QueryShape::OpType::AGGREGATE, 300, 2});
    cache.capture({QueryShape::OpType::JOIN,      400, 0});
    EXPECT_EQ(cache.size(), 4u);
}

// ============================================================================
// GPUGraphCache — invalidate / clear
// ============================================================================

TEST(GPUGraphCacheTest, InvalidateRemovesEntry) {
    GPUGraphCache cache;
    QueryShape shape{QueryShape::OpType::SCAN, 10, 0};
    cache.capture(shape);
    ASSERT_EQ(cache.size(), 1u);
    cache.invalidate(shape);
    EXPECT_EQ(cache.size(), 0u);
    EXPECT_EQ(cache.lookup(shape), nullptr);
}

TEST(GPUGraphCacheTest, InvalidateUnknownShapeIsNoop) {
    GPUGraphCache cache;
    QueryShape shape{QueryShape::OpType::SCAN, 999, 0};
    EXPECT_NO_THROW(cache.invalidate(shape));
    EXPECT_EQ(cache.size(), 0u);
}

TEST(GPUGraphCacheTest, ClearRemovesAllEntries) {
    GPUGraphCache cache;
    cache.capture({QueryShape::OpType::SCAN, 1, 0});
    cache.capture({QueryShape::OpType::SORT, 2, 0});
    ASSERT_EQ(cache.size(), 2u);
    cache.clear();
    EXPECT_EQ(cache.size(), 0u);
}

// ============================================================================
// GPUGraphCache — LRU eviction
// ============================================================================

TEST(GPUGraphCacheTest, LRUEvictionWhenCacheFull) {
    GPUGraphCache cache;
    constexpr size_t kMax = GPUGraphCache::kMaxEntries;

    // Fill the cache to capacity.
    for (size_t i = 0; i < kMax; ++i) {
        cache.capture({QueryShape::OpType::SCAN, i, 0});
    }
    ASSERT_EQ(cache.size(), kMax);

    // Access shape 0 last so it becomes the MRU and survives eviction.
    cache.lookup({QueryShape::OpType::SCAN, 0, 0});

    // Insert one more entry — this should evict the LRU entry (one of 1..kMax-1).
    cache.capture({QueryShape::OpType::SCAN, kMax, 0});
    EXPECT_EQ(cache.size(), kMax);

    // Shape 0 (MRU) must still be present.
    EXPECT_NE(cache.lookup({QueryShape::OpType::SCAN, 0, 0}), nullptr);

    // The newly inserted shape must be present.
    EXPECT_NE(cache.lookup({QueryShape::OpType::SCAN, kMax, 0}), nullptr);
}

TEST(GPUGraphCacheTest, EvictionIncreasesEvictionCounter) {
    GPUGraphCache cache;
    constexpr size_t kMax = GPUGraphCache::kMaxEntries;
    for (size_t i = 0; i <= kMax; ++i) {
        cache.capture({QueryShape::OpType::SORT, i, 0});
    }
    EXPECT_GE(cache.getStats().evictions, 1u);
}

// ============================================================================
// GPUGraphCache — stats
// ============================================================================

TEST(GPUGraphCacheTest, StatsHitsAndMisses) {
    GPUGraphCache cache;
    QueryShape shape{QueryShape::OpType::AGGREGATE, 100, 5};

    // First lookup: miss
    cache.lookup(shape);
    // Capture
    cache.capture(shape);
    // Second lookup: hit
    cache.lookup(shape);
    // Third lookup: hit
    cache.lookup(shape);

    auto stats = cache.getStats();
    EXPECT_EQ(stats.misses, 1u);
    EXPECT_EQ(stats.hits,   2u);
    EXPECT_EQ(stats.entries, 1u);
}

// ============================================================================
// GPUQueryAccelerator — graph cache integration
// ============================================================================

static std::vector<GPUQueryAccelerator::Row> makeAccRows(size_t n) {
    std::vector<GPUQueryAccelerator::Row> rows(n);
    for (size_t i = 0; i < n; ++i) {
        rows[i].id = static_cast<uint64_t>(i);
        float v = static_cast<float>(i);
        rows[i].data.resize(sizeof(float));
        std::memcpy(rows[i].data.data(), &v, sizeof(float));
    }
    return rows;
}

static double extractVal(const GPUQueryAccelerator::Row& r) {
    if (r.data.size() < sizeof(float)) return 0.0;
    float v;
    std::memcpy(&v, r.data.data(), sizeof(float));
    return static_cast<double>(v);
}

TEST(GPUQueryAcceleratorGraphTest, DisabledByDefaultNoStats) {
    GPUQueryAccelerator acc;
    acc.scan(makeAccRows(5));
    auto s = acc.getStats();
    EXPECT_EQ(s.graph_cache_hits,   0u);
    EXPECT_EQ(s.graph_cache_misses, 0u);
}

TEST(GPUQueryAcceleratorGraphTest, EnableViaConfig) {
    GPUQueryAccelerator::Config cfg;
    cfg.force_cpu         = true;
    cfg.enable_graph_cache = true;
    GPUQueryAccelerator acc(cfg);

    auto rows = makeAccRows(10);
    acc.scan(rows);  // first call → miss

    auto s = acc.getStats();
    EXPECT_EQ(s.graph_cache_misses, 1u);
    EXPECT_EQ(s.graph_cache_hits,   0u);
}

TEST(GPUQueryAcceleratorGraphTest, SecondIdenticalScanIsHit) {
    GPUQueryAccelerator::Config cfg;
    cfg.force_cpu         = true;
    cfg.enable_graph_cache = true;
    GPUQueryAccelerator acc(cfg);

    auto rows = makeAccRows(8);
    acc.scan(rows);  // miss
    acc.scan(rows);  // hit

    auto s = acc.getStats();
    EXPECT_EQ(s.graph_cache_misses, 1u);
    EXPECT_EQ(s.graph_cache_hits,   1u);
}

TEST(GPUQueryAcceleratorGraphTest, DifferentRowCountsAreDifferentShapes) {
    GPUQueryAccelerator::Config cfg;
    cfg.force_cpu         = true;
    cfg.enable_graph_cache = true;
    GPUQueryAccelerator acc(cfg);

    acc.scan(makeAccRows(5));   // miss
    acc.scan(makeAccRows(10));  // miss (different shape)

    auto s = acc.getStats();
    EXPECT_EQ(s.graph_cache_misses, 2u);
    EXPECT_EQ(s.graph_cache_hits,   0u);
}

TEST(GPUQueryAcceleratorGraphTest, SortCacheHit) {
    GPUQueryAccelerator::Config cfg;
    cfg.force_cpu         = true;
    cfg.enable_graph_cache = true;
    GPUQueryAccelerator acc(cfg);

    auto rows = makeAccRows(6);
    acc.sort(rows, extractVal, GPUQueryAccelerator::SortOrder::ASC);  // miss
    acc.sort(rows, extractVal, GPUQueryAccelerator::SortOrder::ASC);  // hit

    auto s = acc.getStats();
    EXPECT_EQ(s.graph_cache_misses, 1u);
    EXPECT_EQ(s.graph_cache_hits,   1u);
}

TEST(GPUQueryAcceleratorGraphTest, SortOrderDifferentiatesShapes) {
    GPUQueryAccelerator::Config cfg;
    cfg.force_cpu         = true;
    cfg.enable_graph_cache = true;
    GPUQueryAccelerator acc(cfg);

    auto rows = makeAccRows(4);
    acc.sort(rows, extractVal, GPUQueryAccelerator::SortOrder::ASC);   // miss
    acc.sort(rows, extractVal, GPUQueryAccelerator::SortOrder::DESC);  // miss (different param)

    auto s = acc.getStats();
    EXPECT_EQ(s.graph_cache_misses, 2u);
    EXPECT_EQ(s.graph_cache_hits,   0u);
}

TEST(GPUQueryAcceleratorGraphTest, AggregateCacheHit) {
    GPUQueryAccelerator::Config cfg;
    cfg.force_cpu         = true;
    cfg.enable_graph_cache = true;
    GPUQueryAccelerator acc(cfg);

    auto rows = makeAccRows(7);
    acc.aggregate(rows, GPUQueryAccelerator::AggFunc::SUM, extractVal);  // miss
    acc.aggregate(rows, GPUQueryAccelerator::AggFunc::SUM, extractVal);  // hit

    auto s = acc.getStats();
    EXPECT_EQ(s.graph_cache_misses, 1u);
    EXPECT_EQ(s.graph_cache_hits,   1u);
}

TEST(GPUQueryAcceleratorGraphTest, AggregateFuncDifferentiatesShapes) {
    GPUQueryAccelerator::Config cfg;
    cfg.force_cpu         = true;
    cfg.enable_graph_cache = true;
    GPUQueryAccelerator acc(cfg);

    auto rows = makeAccRows(7);
    acc.aggregate(rows, GPUQueryAccelerator::AggFunc::SUM, extractVal);   // miss
    acc.aggregate(rows, GPUQueryAccelerator::AggFunc::COUNT, extractVal); // miss

    auto s = acc.getStats();
    EXPECT_EQ(s.graph_cache_misses, 2u);
    EXPECT_EQ(s.graph_cache_hits,   0u);
}

TEST(GPUQueryAcceleratorGraphTest, EnableDisableToggle) {
    GPUQueryAccelerator acc;
    auto rows = makeAccRows(5);

    // Disabled by default.
    acc.scan(rows);
    EXPECT_EQ(acc.getStats().graph_cache_misses, 0u);

    // Enable.
    acc.enableGraphCache();
    acc.scan(rows);  // miss
    EXPECT_EQ(acc.getStats().graph_cache_misses, 1u);

    // Disable — subsequent scans should not update cache counters.
    acc.disableGraphCache();
    acc.resetStats();
    acc.scan(rows);
    EXPECT_EQ(acc.getStats().graph_cache_misses, 0u);
    EXPECT_EQ(acc.getStats().graph_cache_hits,   0u);
}

TEST(GPUQueryAcceleratorGraphTest, GetGraphCacheStatsReflectsHitsAndMisses) {
    GPUQueryAccelerator::Config cfg;
    cfg.force_cpu         = true;
    cfg.enable_graph_cache = true;
    GPUQueryAccelerator acc(cfg);

    auto rows = makeAccRows(9);
    acc.scan(rows);  // miss
    acc.scan(rows);  // hit
    acc.scan(rows);  // hit

    auto cs = acc.getGraphCacheStats();
    EXPECT_EQ(cs.misses, 1u);
    EXPECT_EQ(cs.hits,   2u);
}

TEST(GPUQueryAcceleratorGraphTest, ExistingBehaviourUnchangedWhenCacheDisabled) {
    GPUQueryAccelerator::Config cfg;
    cfg.force_cpu         = true;
    cfg.enable_graph_cache = false;
    GPUQueryAccelerator acc(cfg);

    auto rows = makeAccRows(5);
    auto res  = acc.scan(rows);
    EXPECT_EQ(res.rows_scanned, 5u);
    EXPECT_EQ(res.rows_passed,  5u);
    EXPECT_EQ(acc.getStats().total_scans, 1u);
    EXPECT_EQ(acc.getStats().graph_cache_hits, 0u);
}

// ============================================================================
// Thread safety
// ============================================================================

TEST(GPUGraphCacheTest, ConcurrentCaptureAndLookupIsSafe) {
    GPUGraphCache cache;
    QueryShape shape{QueryShape::OpType::SCAN, 16, 0};

    constexpr int kThreads = 4;
    std::vector<std::thread> threads;
    threads.reserve(kThreads * 2);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&]() { cache.capture(shape); });
        threads.emplace_back([&]() { cache.lookup(shape); });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(cache.size(), 1u);
}
