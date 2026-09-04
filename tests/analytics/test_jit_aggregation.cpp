/**
 * Unit tests for JITAggregationCompiler (hot-path JIT for columnar aggregation).
 *
 * Covers:
 *  - Default construction and configuration
 *  - makeSpecKey() determinism and distinctness
 *  - Cold-path execution (call_count < hot_threshold)
 *  - Transition to hot path (call_count == hot_threshold → compiled)
 *  - Hot-path execution correctness for: COUNT(*), SUM, AVG, MIN, MAX,
 *    COUNT_DISTINCT, and GROUP-BY variants
 *  - Statistics: total_calls, jit_hits, jit_compilations, cache_size
 *  - resetStats() does not evict compiled code
 *  - invalidate() evicts one entry and resets its counter
 *  - invalidateAll() flushes the entire cache
 *  - enable_jit = false bypasses the specialisation layer entirely
 *  - max_cache_entries eviction (oldest entry dropped when limit reached)
 *  - Numerical correctness: hot-path results match cold-path results
 *  - Empty spec list returns empty batch
 *  - Batch with selection vector is materialised before aggregation
 */

#include <gtest/gtest.h>
#include "analytics/jit_aggregation.h"
#include "analytics/columnar_execution.h"

#include <cmath>
#include <string>
#include <vector>

using namespace themisdb::analytics;

// ============================================================================
// Test helpers
// ============================================================================

/** Build a small ColumnBatch: int64 "id", double "price", string "cat". */
static ColumnBatch makeTestBatch(size_t n = 6) {
    ColumnBatch batch(n);

    auto id_col = std::make_shared<Column>("id", ColumnType::Int64);
    for (size_t i = 1; i <= n; ++i) {
      id_col->appendInt64(static_cast<int64_t>(i));
    }

    auto price_col = std::make_shared<Column>("price", ColumnType::Double);
    for (size_t i = 1; i <= n; ++i)
        price_col->appendDouble(static_cast<double>(i) * 10.0);

    auto cat_col = std::make_shared<Column>("cat", ColumnType::String);
    const char* cats[] = {"A", "B", "A", "B", "A", "B"};
    for (size_t i = 0; i < n; ++i) {
      cat_col->appendString(cats[i % 2]);
    }

    batch.addColumn(id_col);
    batch.addColumn(price_col);
    batch.addColumn(cat_col);
    return batch;
}

static std::vector<AggregateSpec> sumSpec() {
    return {{
        .result_name  = "total",
        .input_column = "price",
        .function     = AggregateSpec::Function::Sum,
        .group_by     = {}
    }};
}

static std::vector<AggregateSpec> sumGroupBySpec() {
    return {{
        .result_name  = "total",
        .input_column = "price",
        .function     = AggregateSpec::Function::Sum,
        .group_by     = {"cat"}
    }};
}

// Warm up the compiler past the threshold.
static void warmUp(JITAggregationCompiler& jit,
                   const ColumnBatch& batch,
                   const std::vector<AggregateSpec>& specs,
                   size_t times)
{
    for (size_t i = 0; i < times; ++i) {
      jit.aggregate(batch, specs);
    }
}

// ============================================================================
// Construction / Config
// ============================================================================

TEST(JITAggregationTest, DefaultConstruction) {
    JITAggregationCompiler jit;
    EXPECT_EQ(jit.config().hot_threshold, 10u);
    EXPECT_TRUE(jit.config().enable_jit);
    EXPECT_EQ(jit.config().optimization_level, 2);
    EXPECT_EQ(jit.config().max_cache_entries, 256u);
}

TEST(JITAggregationTest, CustomConfig) {
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold     = 3;
    cfg.enable_jit        = false;
    cfg.optimization_level = 1;
    cfg.max_cache_entries  = 8;

    JITAggregationCompiler jit(cfg);
    EXPECT_EQ(jit.config().hot_threshold, 3u);
    EXPECT_FALSE(jit.config().enable_jit);
    EXPECT_EQ(jit.config().optimization_level, 1);
    EXPECT_EQ(jit.config().max_cache_entries, 8u);
}

// ============================================================================
// makeSpecKey
// ============================================================================

TEST(JITAggregationTest, MakeSpecKeyDeterministic) {
    auto specs = sumSpec();
    EXPECT_EQ(JITAggregationCompiler::makeSpecKey(specs),
              JITAggregationCompiler::makeSpecKey(specs));
}

TEST(JITAggregationTest, MakeSpecKeyDistinct) {
    auto specs1 = sumSpec();
    auto specs2 = std::vector<AggregateSpec>{{
        .result_name  = "cnt",
        .input_column = "",
        .function     = AggregateSpec::Function::Count,
        .group_by     = {}
    }};
    EXPECT_NE(JITAggregationCompiler::makeSpecKey(specs1),
              JITAggregationCompiler::makeSpecKey(specs2));
}

TEST(JITAggregationTest, MakeSpecKeyGroupByDiffers) {
    auto specs_no_grp = sumSpec();
    auto specs_grp    = sumGroupBySpec();
    EXPECT_NE(JITAggregationCompiler::makeSpecKey(specs_no_grp),
              JITAggregationCompiler::makeSpecKey(specs_grp));
}

TEST(JITAggregationTest, MakeSpecKeyEmptySpecs) {
    std::vector<AggregateSpec> empty = {};

    EXPECT_FALSE(JITAggregationCompiler::makeSpecKey(empty).empty());
}

// ============================================================================
// Cold path
// ============================================================================

TEST(JITAggregationTest, ColdPathProducesCorrectResult) {
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold = 5;
    JITAggregationCompiler jit(cfg);
    auto batch = makeTestBatch(6);
    auto specs = sumSpec();

    // Only 2 calls → still cold.
    warmUp(jit, batch, specs, 2);

    const std::string key = JITAggregationCompiler::makeSpecKey(specs);
    EXPECT_FALSE(jit.isCompiled(key));
    EXPECT_EQ(jit.callCount(key), 2u);

    ColumnBatch result = jit.aggregate(batch, specs);
    ASSERT_EQ(result.rowCount(), 1u);
    auto col = result.getColumn("total");
    ASSERT_NE(col, nullptr);
    // sum(10+20+30+40+50+60) = 210
    EXPECT_DOUBLE_EQ(col->doubleData()[0], 210.0);
}

// ============================================================================
// Hot-path transition
// ============================================================================

TEST(JITAggregationTest, CompilationTriggeredAtThreshold) {
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold = 3;
    JITAggregationCompiler jit(cfg);
    auto batch = makeTestBatch();
    auto specs = sumSpec();
    const std::string key = JITAggregationCompiler::makeSpecKey(specs);

    // Two cold calls.
    warmUp(jit, batch, specs, 2);
    EXPECT_FALSE(jit.isCompiled(key));

    // Third call → triggers compilation.
    jit.aggregate(batch, specs);
    EXPECT_TRUE(jit.isCompiled(key));
    EXPECT_EQ(jit.stats().jit_compilations, 1u);
}

TEST(JITAggregationTest, HotPathHitsTracked) {
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold = 2;
    JITAggregationCompiler jit(cfg);
    auto batch = makeTestBatch();
    auto specs = sumSpec();

    warmUp(jit, batch, specs, 4);  // 1 cold + compile + 2 hits = 4 total

    EXPECT_GE(jit.stats().jit_hits, 2u);
    EXPECT_EQ(jit.stats().total_calls, 4u);
}

// ============================================================================
// Numerical correctness (hot path == cold path)
// ============================================================================

TEST(JITAggregationTest, SumCorrectnessHotPath) {
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold = 2;
    JITAggregationCompiler jit(cfg);
    auto batch = makeTestBatch(6);
    auto specs = sumSpec();

    warmUp(jit, batch, specs, 3);  // ensure compiled

    ColumnBatch result = jit.aggregate(batch, specs);
    ASSERT_EQ(result.rowCount(), 1u);
    auto col = result.getColumn("total");
    ASSERT_NE(col, nullptr);
    EXPECT_DOUBLE_EQ(col->doubleData()[0], 210.0);
}

TEST(JITAggregationTest, AvgCorrectnessHotPath) {
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold = 2;
    JITAggregationCompiler jit(cfg);
    auto batch = makeTestBatch(4);

    std::vector<AggregateSpec> specs = {{
        .result_name  = "avg_price",
        .input_column = "price",
        .function     = AggregateSpec::Function::Avg,
        .group_by     = {}
    }};

    warmUp(jit, batch, specs, 3);
    ColumnBatch result = jit.aggregate(batch, specs);
    ASSERT_EQ(result.rowCount(), 1u);
    auto col = result.getColumn("avg_price");
    ASSERT_NE(col, nullptr);
    // avg(10+20+30+40) = 25
    EXPECT_DOUBLE_EQ(col->doubleData()[0], 25.0);
}

TEST(JITAggregationTest, MinMaxCorrectnessHotPath) {
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold = 2;
    JITAggregationCompiler jit(cfg);
    auto batch = makeTestBatch(5);

    std::vector<AggregateSpec> specs = {
        {.result_name  = "min_p", .input_column = "price",
         .function     = AggregateSpec::Function::Min, .group_by = {}},
        {.result_name  = "max_p", .input_column = "price",
         .function     = AggregateSpec::Function::Max, .group_by = {}}
    };

    warmUp(jit, batch, specs, 3);
    ColumnBatch result = jit.aggregate(batch, specs);
    ASSERT_EQ(result.rowCount(), 1u);
    EXPECT_DOUBLE_EQ(result.getColumn("min_p")->doubleData()[0], 10.0);
    EXPECT_DOUBLE_EQ(result.getColumn("max_p")->doubleData()[0], 50.0);
}

TEST(JITAggregationTest, CountStarHotPath) {
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold = 2;
    JITAggregationCompiler jit(cfg);
    auto batch = makeTestBatch(6);

    std::vector<AggregateSpec> specs = {{
        .result_name  = "n",
        .input_column = "",
        .function     = AggregateSpec::Function::Count,
        .group_by     = {}
    }};

    warmUp(jit, batch, specs, 3);
    ColumnBatch result = jit.aggregate(batch, specs);
    ASSERT_EQ(result.rowCount(), 1u);
    EXPECT_DOUBLE_EQ(result.getColumn("n")->doubleData()[0], 6.0);
}

TEST(JITAggregationTest, CountDistinctHotPath) {
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold = 2;
    JITAggregationCompiler jit(cfg);
    auto batch = makeTestBatch(6);  // cats: A,B,A,B,A,B → 2 distinct

    std::vector<AggregateSpec> specs = {{
        .result_name  = "n_cats",
        .input_column = "cat",
        .function     = AggregateSpec::Function::CountDistinct,
        .group_by     = {}
    }};

    warmUp(jit, batch, specs, 3);
    ColumnBatch result = jit.aggregate(batch, specs);
    ASSERT_EQ(result.rowCount(), 1u);
    EXPECT_DOUBLE_EQ(result.getColumn("n_cats")->doubleData()[0], 2.0);
}

TEST(JITAggregationTest, GroupByCorrectnessHotPath) {
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold = 2;
    JITAggregationCompiler jit(cfg);
    auto batch = makeTestBatch(6);   // A:10,30,50  B:20,40,60
    auto specs = sumGroupBySpec();

    warmUp(jit, batch, specs, 3);
    ColumnBatch result = jit.aggregate(batch, specs);
    ASSERT_EQ(result.rowCount(), 2u);

    // Collect group→total map.
    auto cat_col   = result.getColumn("cat");
    auto total_col = result.getColumn("total");
    ASSERT_NE(cat_col, nullptr);
    ASSERT_NE(total_col, nullptr);

    std::unordered_map<std::string, double> sums = {};

    for (size_t i = 0; i < result.rowCount(); ++i) {
        sums[cat_col->stringData()[i]] = total_col->doubleData()[i];
    }
    EXPECT_DOUBLE_EQ(sums["A"], 90.0);   // 10+30+50
    EXPECT_DOUBLE_EQ(sums["B"], 120.0);  // 20+40+60
}

// ============================================================================
// Statistics
// ============================================================================

TEST(JITAggregationTest, StatisticsAccurate) {
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold = 3;
    JITAggregationCompiler jit(cfg);
    auto batch = makeTestBatch();
    auto specs = sumSpec();

    EXPECT_EQ(jit.stats().total_calls, 0u);
    EXPECT_EQ(jit.stats().jit_hits, 0u);
    EXPECT_EQ(jit.stats().jit_compilations, 0u);
    EXPECT_EQ(jit.stats().cache_size, 0u);

    warmUp(jit, batch, specs, 5);

    EXPECT_EQ(jit.stats().total_calls, 5u);
    EXPECT_EQ(jit.stats().jit_compilations, 1u);
    EXPECT_GE(jit.stats().jit_hits, 2u);
    EXPECT_EQ(jit.stats().cache_size, 1u);
}

TEST(JITAggregationTest, ResetStatsPreservesCache) {
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold = 2;
    JITAggregationCompiler jit(cfg);
    auto batch = makeTestBatch();
    auto specs = sumSpec();
    const std::string key = JITAggregationCompiler::makeSpecKey(specs);

    warmUp(jit, batch, specs, 4);
    EXPECT_TRUE(jit.isCompiled(key));

    jit.resetStats();

    EXPECT_EQ(jit.stats().total_calls, 0u);
    EXPECT_EQ(jit.stats().jit_hits, 0u);
    // Compiled code should still be present.
    EXPECT_TRUE(jit.isCompiled(key));
    EXPECT_EQ(jit.stats().cache_size, 1u);
}

// ============================================================================
// Invalidation
// ============================================================================

TEST(JITAggregationTest, InvalidateOneEntry) {
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold = 2;
    JITAggregationCompiler jit(cfg);
    auto batch = makeTestBatch();
    auto specs = sumSpec();
    const std::string key = JITAggregationCompiler::makeSpecKey(specs);

    warmUp(jit, batch, specs, 4);
    EXPECT_TRUE(jit.isCompiled(key));

    jit.invalidate(key);
    EXPECT_FALSE(jit.isCompiled(key));
    EXPECT_EQ(jit.callCount(key), 0u);
}

TEST(JITAggregationTest, InvalidateAllClearsCache) {
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold = 2;
    JITAggregationCompiler jit(cfg);
    auto batch = makeTestBatch();
    auto specs1 = sumSpec();
    auto specs2 = sumGroupBySpec();

    warmUp(jit, batch, specs1, 4);
    warmUp(jit, batch, specs2, 4);
    EXPECT_EQ(jit.stats().cache_size, 2u);

    jit.invalidateAll();
    EXPECT_EQ(jit.stats().cache_size, 0u);
    EXPECT_FALSE(jit.isCompiled(JITAggregationCompiler::makeSpecKey(specs1)));
    EXPECT_FALSE(jit.isCompiled(JITAggregationCompiler::makeSpecKey(specs2)));
}

TEST(JITAggregationTest, AfterInvalidateRecompilesOnReuse) {
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold = 2;
    JITAggregationCompiler jit(cfg);
    auto batch = makeTestBatch(6);
    auto specs = sumSpec();
    const std::string key = JITAggregationCompiler::makeSpecKey(specs);

    warmUp(jit, batch, specs, 4);
    jit.invalidate(key);
    EXPECT_FALSE(jit.isCompiled(key));

    // Warm up again – must recompile.
    warmUp(jit, batch, specs, 4);
    EXPECT_TRUE(jit.isCompiled(key));
    EXPECT_EQ(jit.stats().jit_compilations, 2u);
}

// ============================================================================
// enable_jit = false
// ============================================================================

TEST(JITAggregationTest, DisabledJitNeverCompiles) {
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold = 2;
    cfg.enable_jit    = false;
    JITAggregationCompiler jit(cfg);
    auto batch = makeTestBatch(6);
    auto specs = sumSpec();
    const std::string key = JITAggregationCompiler::makeSpecKey(specs);

    warmUp(jit, batch, specs, 10);
    EXPECT_FALSE(jit.isCompiled(key));
    EXPECT_EQ(jit.stats().jit_compilations, 0u);
    EXPECT_EQ(jit.stats().jit_hits, 0u);
}

TEST(JITAggregationTest, DisabledJitResultStillCorrect) {
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold = 2;
    cfg.enable_jit    = false;
    JITAggregationCompiler jit(cfg);
    auto batch = makeTestBatch(6);
    auto specs = sumSpec();

    ColumnBatch result = jit.aggregate(batch, specs);
    ASSERT_EQ(result.rowCount(), 1u);
    auto col = result.getColumn("total");
    ASSERT_NE(col, nullptr);
    EXPECT_DOUBLE_EQ(col->doubleData()[0], 210.0);
}

// ============================================================================
// Edge cases
// ============================================================================

TEST(JITAggregationTest, EmptySpecsReturnsEmptyBatch) {
    JITAggregationCompiler jit;
    auto batch = makeTestBatch(6);
    std::vector<AggregateSpec> empty;
    ColumnBatch result = jit.aggregate(batch, empty);
    EXPECT_EQ(result.columnCount(), 0u);
}

TEST(JITAggregationTest, BatchWithSelectionMaterialisedBeforeAgg) {
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold = 2;
    JITAggregationCompiler jit(cfg);

    auto batch = makeTestBatch(6);
    // Attach selection vector selecting rows 0,2,4 (prices 10,30,50 → sum=90).
    SelectionVector sel(3);
    sel.push_back(0); sel.push_back(2); sel.push_back(4);
    batch.setSelection(sel);

    auto specs = sumSpec();
    warmUp(jit, batch, specs, 3);  // ensure compiled

    ColumnBatch result = jit.aggregate(batch, specs);
    ASSERT_EQ(result.rowCount(), 1u);
    auto col = result.getColumn("total");
    ASSERT_NE(col, nullptr);
    EXPECT_DOUBLE_EQ(col->doubleData()[0], 90.0);
}

TEST(JITAggregationTest, CacheEvictionOnMaxEntries) {
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold     = 1;
    cfg.max_cache_entries = 2;
    JITAggregationCompiler jit(cfg);
    auto batch = makeTestBatch(4);

    // Produce 3 different spec-sets to trigger eviction.
    std::vector<AggregateSpec> s1 = {{
        .result_name = "r1", .input_column = "price",
        .function    = AggregateSpec::Function::Sum, .group_by = {}}};
    std::vector<AggregateSpec> s2 = {{
        .result_name = "r2", .input_column = "price",
        .function    = AggregateSpec::Function::Avg, .group_by = {}}};
    std::vector<AggregateSpec> s3 = {{
        .result_name = "r3", .input_column = "price",
        .function    = AggregateSpec::Function::Min, .group_by = {}}};

    warmUp(jit, batch, s1, 2);
    warmUp(jit, batch, s2, 2);
    warmUp(jit, batch, s3, 2);

    // After third compilation the cache size must not exceed max_cache_entries.
    EXPECT_LE(jit.stats().cache_size, cfg.max_cache_entries);
    EXPECT_EQ(jit.stats().jit_compilations, 3u);
}

TEST(JITAggregationTest, HotPathResultMatchesColdPath) {
    // Execute both paths on the same batch and compare results.
    auto batch = makeTestBatch(8);
    auto specs = sumGroupBySpec();

    // Cold reference via AggregateOperator directly.
    AggregateOperator ref_op(specs);
    ColumnBatch ref = ref_op.execute(batch);

    // Hot path via JIT compiler.
    JITAggregationCompiler::Config cfg;
    cfg.hot_threshold = 3;
    JITAggregationCompiler jit(cfg);
    warmUp(jit, batch, specs, 4);  // compile + hits
    ColumnBatch hot = jit.aggregate(batch, specs);

    ASSERT_EQ(ref.rowCount(), hot.rowCount());

    // Collect group→total maps from both results and compare.
    auto collect = [](const ColumnBatch& b) {
        std::unordered_map<std::string, double> m;
        auto cat_col   = b.getColumn("cat");
        auto total_col = b.getColumn("total");
        if (!cat_col || !total_col) {
          return m;
        }
        for (size_t i = 0; i < b.rowCount(); ++i) {
            m[cat_col->stringData()[i]] = total_col->doubleData()[i];
        }
        return m;
    };

    auto ref_m = collect(ref);
    auto hot_m = collect(hot);
    EXPECT_EQ(ref_m.size(), hot_m.size());
    for (const auto& [k, v] : ref_m) {
        ASSERT_TRUE(hot_m.count(k));
        EXPECT_DOUBLE_EQ(hot_m.at(k), v);
    }
}
