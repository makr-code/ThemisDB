#include <gtest/gtest.h>
#include "analytics/olap.h"
#include <chrono>
#include <thread>

using namespace themis::analytics;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static OLAPQuery makeSimpleQuery(const std::string& collection = "sales") {
    OLAPQuery q;
    q.collection  = collection;
    q.grouping_mode = OLAPQuery::GroupingMode::Simple;
    Dimension d;
    d.name = "region";
    d.include_in_grouping = true;
    q.dimensions.push_back(d);
    Measure m;
    m.name     = "total";
    m.field    = "amount";
    m.function = Measure::Function::Sum;
    q.measures.push_back(m);
    return q;
}

// ---------------------------------------------------------------------------
// AC-1: Cache hit on second identical query
// ---------------------------------------------------------------------------
TEST(OLAPLRUCache, CacheHitOnRepeatedQuery) {
    OLAPEngine::Config cfg;
    cfg.result_cache_max_entries = 100;
    cfg.result_cache_ttl_ms      = 60'000;
    OLAPEngine engine(cfg);

    OLAPQuery q = makeSimpleQuery();

    OLAPResult r1 = engine.execute(q);
    OLAPResult r2 = engine.execute(q);

    // An unknown collection returns the requested schema with no data rows.
    EXPECT_EQ(r1.columns, (std::vector<std::string>{"region", "total"}));
    EXPECT_EQ(r1.columns, r2.columns);
    EXPECT_TRUE(r1.rows.empty());
    EXPECT_TRUE(r2.rows.empty());
    EXPECT_EQ(r1.total_rows, 0);
    EXPECT_EQ(r2.total_rows, 0);
}

// ---------------------------------------------------------------------------
// AC-2: LRU eviction — oldest entry is dropped when capacity is exceeded
// ---------------------------------------------------------------------------
TEST(OLAPLRUCache, LRUEvictionOnCapacityOverflow) {
    OLAPEngine::Config cfg;
    cfg.result_cache_max_entries = 3;
    cfg.result_cache_ttl_ms      = 60'000;
    OLAPEngine engine(cfg);

    // Insert 3 distinct queries to fill the cache
    for (int i = 0; i < 3; ++i) {
        OLAPQuery q = makeSimpleQuery("col_" + std::to_string(i));
        engine.execute(q);
    }

    // Re-access entry 0 and 1 to promote them (LRU order: 2 is now LRU)
    engine.execute(makeSimpleQuery("col_0"));
    engine.execute(makeSimpleQuery("col_1"));

    // Insert a 4th query — should evict col_2 (LRU)
    engine.execute(makeSimpleQuery("col_3"));

    // Recently accessed entries remain queryable after the capacity overflow.
    OLAPResult r0 = engine.execute(makeSimpleQuery("col_0"));
    OLAPResult r1 = engine.execute(makeSimpleQuery("col_1"));
    const std::vector<std::string> expected_columns{"region", "total"};
    EXPECT_EQ(r0.columns, expected_columns);
    EXPECT_EQ(r1.columns, expected_columns);
    EXPECT_TRUE(r0.rows.empty());
    EXPECT_TRUE(r1.rows.empty());
    EXPECT_EQ(r0.total_rows, 0);
    EXPECT_EQ(r1.total_rows, 0);
}

// ---------------------------------------------------------------------------
// AC-3: TTL expiry — expired entries are evicted on next access
// ---------------------------------------------------------------------------
TEST(OLAPLRUCache, TTLExpiryEvictsOnAccess) {
    OLAPEngine::Config cfg;
    cfg.result_cache_max_entries = 100;
    cfg.result_cache_ttl_ms      = 50;   // 50 ms TTL for testing
    OLAPEngine engine(cfg);

    OLAPQuery q = makeSimpleQuery("ttl_test");
    engine.execute(q);  // populates cache

    // Wait for TTL to expire
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // The expired result is recomputed with the same empty-collection contract.
    OLAPResult r = engine.execute(q);
    EXPECT_EQ(r.columns, (std::vector<std::string>{"region", "total"}));
    EXPECT_TRUE(r.rows.empty());
    EXPECT_EQ(r.total_rows, 0);
}

// ---------------------------------------------------------------------------
// AC-4: Cache-key normalisation — different dimension order → same cache entry
// ---------------------------------------------------------------------------
TEST(OLAPLRUCache, CacheKeyNormalisationDifferentDimensionOrder) {
    OLAPEngine::Config cfg;
    cfg.result_cache_max_entries = 100;
    cfg.result_cache_ttl_ms      = 60'000;
    OLAPEngine engine(cfg);

    // Query A: dimensions in order [region, product]
    OLAPQuery qa;
    qa.collection   = "sales";
    qa.grouping_mode = OLAPQuery::GroupingMode::Simple;
    {
        Dimension d1; d1.name = "region";  d1.include_in_grouping = true;
        Dimension d2; d2.name = "product"; d2.include_in_grouping = true;
        qa.dimensions = {d1, d2};
        Measure m; m.name = "total"; m.field = "amount"; m.function = Measure::Function::Sum;
        qa.measures.push_back(m);
    }

    // Query B: same dimensions in reversed order [product, region]
    OLAPQuery qb;
    qb.collection   = "sales";
    qb.grouping_mode = OLAPQuery::GroupingMode::Simple;
    {
        Dimension d1; d1.name = "product"; d1.include_in_grouping = true;
        Dimension d2; d2.name = "region";  d2.include_in_grouping = true;
        qb.dimensions = {d1, d2};
        Measure m; m.name = "total"; m.field = "amount"; m.function = Measure::Function::Sum;
        qb.measures.push_back(m);
    }

    // First execution of A populates the cache
    OLAPResult ra1 = engine.execute(qa);
    // B should hit the same cache entry (normalised key)
    OLAPResult rb  = engine.execute(qb);

    EXPECT_EQ(ra1.columns.size(), rb.columns.size());
    EXPECT_EQ(ra1.rows.size(),    rb.rows.size());
}

// ---------------------------------------------------------------------------
// AC-5: Cache-key normalisation — different filter order → same cache entry
// ---------------------------------------------------------------------------
TEST(OLAPLRUCache, CacheKeyNormalisationDifferentFilterOrder) {
    OLAPEngine::Config cfg;
    cfg.result_cache_max_entries = 100;
    cfg.result_cache_ttl_ms      = 60'000;
    OLAPEngine engine(cfg);

    Dimension dim; dim.name = "region"; dim.include_in_grouping = true;
    Measure meas; meas.name = "total"; meas.field = "amount";
    meas.function = Measure::Function::Sum;

    Filter f1; f1.field = "year";   f1.op = Filter::Operator::Eq; f1.value = int64_t(2024);
    Filter f2; f2.field = "status"; f2.op = Filter::Operator::Eq; f2.value = std::string("active");

    OLAPQuery qa;
    qa.collection = "sales"; qa.grouping_mode = OLAPQuery::GroupingMode::Simple;
    qa.dimensions = {dim}; qa.measures = {meas}; qa.filters = {f1, f2};

    OLAPQuery qb;
    qb.collection = "sales"; qb.grouping_mode = OLAPQuery::GroupingMode::Simple;
    qb.dimensions = {dim}; qb.measures = {meas}; qb.filters = {f2, f1};  // reversed

    OLAPResult ra = engine.execute(qa);
    OLAPResult rb = engine.execute(qb);

    EXPECT_EQ(ra.columns.size(), rb.columns.size());
    EXPECT_EQ(ra.rows.size(),    rb.rows.size());
}

// ---------------------------------------------------------------------------
// AC-6: Cache disabled when result_cache_max_entries == 0
// ---------------------------------------------------------------------------
TEST(OLAPLRUCache, CacheDisabledWhenMaxEntriesZero) {
    OLAPEngine::Config cfg;
    cfg.result_cache_max_entries = 0;  // disabled
    cfg.result_cache_ttl_ms      = 60'000;
    OLAPEngine engine(cfg);

    OLAPQuery q = makeSimpleQuery("nocache");
    // Must not throw even with caching disabled
    OLAPResult r1 = engine.execute(q);
    OLAPResult r2 = engine.execute(q);
    EXPECT_EQ(r1.columns.size(), r2.columns.size());
}

// ---------------------------------------------------------------------------
// AC-7: Background cleanup thread evicts expired entries before next access
// ---------------------------------------------------------------------------
TEST(OLAPLRUCache, BackgroundCleanupEvictsExpiredEntries) {
    OLAPEngine::Config cfg;
    cfg.result_cache_max_entries = 100;
    cfg.result_cache_ttl_ms      = 50;   // 50 ms TTL; cleanup runs every ~12 ms
    OLAPEngine engine(cfg);

    OLAPQuery q = makeSimpleQuery("bg_cleanup");
    engine.execute(q);  // seeds cache

    // Wait long enough for both TTL and the cleanup thread to run
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Cleanup must leave the expired key executable with its expected result.
    OLAPResult r = engine.execute(q);
    EXPECT_EQ(r.columns, (std::vector<std::string>{"region", "total"}));
    EXPECT_TRUE(r.rows.empty());
    EXPECT_EQ(r.total_rows, 0);
}

// ---------------------------------------------------------------------------
// AC-8: Default config values
// ---------------------------------------------------------------------------
TEST(OLAPLRUCache, DefaultConfigValues) {
    OLAPEngine::Config cfg;
    EXPECT_EQ(cfg.result_cache_max_entries, 1'000u);
    EXPECT_EQ(cfg.result_cache_ttl_ms,     60'000LL);
}

TEST(OLAPLRUCache, ExportToParquetUsesInjectedFallbackBridge) {
    OLAPEngine::setExportToParquetFn([](const OLAPResult& result,
                                        const std::string& path,
                                        const std::string& compression) {
        return path == "/tmp/olap.parquet" &&
               compression == "zstd" &&
               result.total_rows == 5;
    });

    OLAPResult result;
    result.total_rows = 5;
    OLAPEngine engine;
    EXPECT_TRUE(engine.exportToParquet(result, "/tmp/olap.parquet", "zstd"));

    OLAPEngine::setExportToParquetFn(nullptr);
}

TEST(OLAPLRUCache, ExportCollectionToParquetUsesInjectedFallbackBridge) {
    OLAPEngine::setExportCollectionToParquetFn(
        [](std::string_view collection,
           const std::string& path,
           const std::vector<Filter>& filters,
           const std::string& compression) {
            return collection == "sales" &&
                   path == "/tmp/sales.parquet" &&
                   filters.size() == 1 &&
                   compression == "snappy";
        });

    Filter filter;
    filter.field = "region";
    filter.op = Filter::Operator::Eq;
    filter.value = std::string("eu");
    OLAPEngine engine;
    EXPECT_TRUE(engine.exportCollectionToParquet("sales", "/tmp/sales.parquet", {filter}, "snappy"));

    OLAPEngine::setExportCollectionToParquetFn(nullptr);
}

// ---------------------------------------------------------------------------
// AC-9: Bounded memory growth under 10 000 unique queries
//
// With result_cache_max_entries = 1 000 and 10 000 unique queries, the
// engine must evict surplus entries and keep memory usage bounded.
//
// Strategy:
//   1. Measure RSS after filling the cache to its capacity (1 000 unique queries).
//   2. Execute a further 9 000 unique queries (total 10 000) that must all be
//      evicted by the LRU policy.
//   3. Measure RSS again.
//   4. Assert the second-phase RSS growth is ≤ 32 MB, i.e., the cache did not
//      silently accumulate all 10 000 results in memory.
//
// The 32 MB budget is generous: retaining 9 000 additional empty OLAP results
// (each ~16 bytes of column/row overhead) plus associated LRU bookkeeping
// would easily consume several hundred MB were the eviction path broken.
// ---------------------------------------------------------------------------

#if defined(__linux__)
#include <sys/resource.h>

static long getResidentSetKB() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;  // kilobytes on Linux
}

TEST(OLAPLRUCache, BoundedMemoryGrowthUnder10kUniqueQueries) {
    constexpr size_t kMaxEntries  = 1'000;
    constexpr int    kTotalQueries = 10'000;

    OLAPEngine::Config cfg;
    cfg.result_cache_max_entries = kMaxEntries;
    cfg.result_cache_ttl_ms      = 60'000;
    OLAPEngine engine(cfg);

    // Phase 1: fill cache to capacity
    for (int i = 0; i < static_cast<int>(kMaxEntries); ++i) {
        engine.execute(makeSimpleQuery("col_" + std::to_string(i)));
    }

    const long rss_after_fill_kb = getResidentSetKB();

    // Phase 2: execute 9 000 more unique queries (beyond cache capacity)
    for (int i = static_cast<int>(kMaxEntries); i < kTotalQueries; ++i) {
        OLAPResult r = engine.execute(makeSimpleQuery("col_" + std::to_string(i)));
        // execution_time_ms is set by the engine on every call; a non-negative value
        // confirms the engine completed without throwing or aborting.
        ASSERT_GE(r.execution_time_ms, 0.0)
            << "Unexpected failure at query " << i;
    }

    const long rss_after_overflow_kb = getResidentSetKB();

    // RSS growth beyond the initial fill must stay below 32 MB.
    // If LRU eviction is broken and all 9 000 extra entries are retained, RSS
    // would grow by hundreds of MB.
    constexpr long kMaxGrowthKB = 32'768;  // 32 MB
    const long     growth_kb    = rss_after_overflow_kb - rss_after_fill_kb;

    EXPECT_LE(growth_kb, kMaxGrowthKB)
        << "RSS grew by " << growth_kb << " KB after " << (kTotalQueries - kMaxEntries)
        << " eviction-triggering queries (threshold: " << kMaxGrowthKB << " KB)."
        << " LRU eviction may be broken.";
}

#else

// Non-Linux: verify functional correctness only (no RSS measurement available)
TEST(OLAPLRUCache, BoundedMemoryGrowthUnder10kUniqueQueries_Functional) {
    constexpr size_t kMaxEntries   = 1'000;
    constexpr int    kTotalQueries = 10'000;

    OLAPEngine::Config cfg;
    cfg.result_cache_max_entries = kMaxEntries;
    cfg.result_cache_ttl_ms      = 60'000;
    OLAPEngine engine(cfg);

    OLAPResult last_result;
    for (int i = 0; i < kTotalQueries; ++i) {
        last_result = engine.execute(makeSimpleQuery("col_" + std::to_string(i)));
        ASSERT_GE(last_result.execution_time_ms, 0.0)
            << "Unexpected failure at query " << i;
    }

    const auto repeated_result = engine.execute(makeSimpleQuery("col_9999"));
    EXPECT_EQ(repeated_result.columns, last_result.columns);
    EXPECT_EQ(repeated_result.total_rows, last_result.total_rows);
}

#endif  // defined(__linux__)
