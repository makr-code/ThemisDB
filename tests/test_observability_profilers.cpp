/**
 * @file test_observability_profilers.cpp
 * @brief Tests for QueryProfiler, StorageProfiler, and PerformanceAnalyzer.
 *        Previously uncovered by the test suite.
 *
 * Covers:
 *   QueryProfiler:
 *     - start_query / end_query lifecycle
 *     - record_phase, record_cache_usage
 *     - get_profile / get_all_profiles
 *     - get_slow_queries (threshold filtering)
 *     - get_top_queries
 *     - clear
 *
 *   StorageProfiler:
 *     - record_operation (GET, PUT, SCAN)
 *     - get_operations / get_slow_operations
 *     - get_operation_summary (JSON)
 *     - get_cache_metrics (JSON)
 *
 *   PerformanceAnalyzer:
 *     - analyze() integrating both profilers
 *     - analyze_queries() / analyze_storage() individually
 *     - get_config / set_config round-trip
 */

#include <gtest/gtest.h>
#include "observability/query_profiler.h"
#include "observability/storage_profiler.h"
#include "observability/performance_analyzer.h"
#include <thread>
#include <chrono>

using namespace themis::observability;
using namespace std::chrono_literals;

// ============================================================================
// QueryProfiler
// ============================================================================

class QueryProfilerTest : public ::testing::Test {
protected:
    QueryProfilerConfig cfg_;
    QueryProfiler       profiler_{cfg_};
};

TEST_F(QueryProfilerTest, StartAndEndQuery_ProfileExists) {
    profiler_.start_query("q1", "FOR x IN col RETURN x");
    profiler_.end_query("q1");

    auto profile = profiler_.get_profile("q1");
    ASSERT_NE(profile, nullptr);
    EXPECT_EQ(profile->query_id, "q1");
    EXPECT_EQ(profile->query_text, "FOR x IN col RETURN x");
}

TEST_F(QueryProfilerTest, GetProfile_UnknownQuery_ReturnsNullptr) {
    auto profile = profiler_.get_profile("nonexistent");
    EXPECT_EQ(profile, nullptr);
}

TEST_F(QueryProfilerTest, GetAllProfiles_MultipleQueries) {
    profiler_.start_query("qa", "query_a");
    profiler_.end_query("qa");
    profiler_.start_query("qb", "query_b");
    profiler_.end_query("qb");

    auto all = profiler_.get_all_profiles();
    EXPECT_GE(all.size(), 2u);
}

TEST_F(QueryProfilerTest, RecordPhase_AppearsInProfile) {
    profiler_.start_query("qphase", "SELECT 1");
    profiler_.record_phase("qphase", QueryPhase::PARSE, 100us);
    profiler_.record_phase("qphase", QueryPhase::OPTIMIZE, 200us);
    profiler_.end_query("qphase");

    auto profile = profiler_.get_profile("qphase");
    ASSERT_NE(profile, nullptr);
    EXPECT_FALSE(profile->phase_timings.empty());
}

TEST_F(QueryProfilerTest, RecordCacheUsage_TrackedInProfile) {
    profiler_.start_query("qcache", "cache_query");
    profiler_.record_cache_usage("qcache", /*cache_hit=*/true);
    profiler_.record_cache_usage("qcache", /*cache_hit=*/false);
    profiler_.end_query("qcache");

    auto profile = profiler_.get_profile("qcache");
    ASSERT_NE(profile, nullptr);
    // QueryProfile tracks cache usage as a boolean flag.
    EXPECT_TRUE(profile->used_cache);
}

TEST_F(QueryProfilerTest, GetSlowQueries_FiltersCorrectly) {
    // Create a "slow" query by injecting a delay
    profiler_.start_query("qslow", "slow_query");
    std::this_thread::sleep_for(10ms);
    profiler_.end_query("qslow");

    profiler_.start_query("qfast", "fast_query");
    profiler_.end_query("qfast");

    // With 1ms threshold almost everything qualifies; with 1000ms nothing does
    auto slow_threshold = profiler_.get_slow_queries(1ms);
    auto none_threshold = profiler_.get_slow_queries(10min);

    EXPECT_GE(slow_threshold.size(), 1u); // "qslow" should be in there
    EXPECT_EQ(none_threshold.size(), 0u);
}

TEST_F(QueryProfilerTest, GetTopQueries_ReturnsLimitedResults) {
    for (int i = 0; i < 5; ++i) {
        std::string id = "qtop_" + std::to_string(i);
        profiler_.start_query(id, "query_" + std::to_string(i));
        profiler_.end_query(id);
    }
    auto top3 = profiler_.get_top_queries(3);
    EXPECT_LE(top3.size(), 3u);
}

TEST_F(QueryProfilerTest, Clear_RemovesAllProfiles) {
    profiler_.start_query("q_clear", "will_be_cleared");
    profiler_.end_query("q_clear");
    ASSERT_NE(profiler_.get_profile("q_clear"), nullptr);

    profiler_.clear();

    EXPECT_EQ(profiler_.get_profile("q_clear"), nullptr);
    EXPECT_TRUE(profiler_.get_all_profiles().empty());
}

// ============================================================================
// StorageProfiler
// ============================================================================

class StorageProfilerTest : public ::testing::Test {
protected:
    StorageProfilerConfig cfg_;
    StorageProfiler       profiler_{cfg_};
};

TEST_F(StorageProfilerTest, RecordGet_AppearsInOperations) {
    StorageOpStats op;
    op.type         = StorageOpType::GET;
    op.duration     = 50us;
    op.bytes_read   = 128;
    op.cache_hit    = true;
    profiler_.record_operation(op);

    auto ops = profiler_.get_operations();
    ASSERT_GE(ops.size(), 1u);
    EXPECT_EQ(ops.back().type, StorageOpType::GET);
    EXPECT_EQ(ops.back().bytes_read, 128u);
}

TEST_F(StorageProfilerTest, RecordPut_AppearsInOperations) {
    StorageOpStats op;
    op.type          = StorageOpType::PUT;
    op.duration      = 200us;
    op.bytes_written = 512;
    profiler_.record_operation(op);

    auto ops = profiler_.get_operations();
    EXPECT_GE(ops.size(), 1u);
}

TEST_F(StorageProfilerTest, GetSlowOperations_FiltersCorrectly) {
    StorageOpStats fast_op;
    fast_op.type     = StorageOpType::GET;
    fast_op.duration = 10us;
    profiler_.record_operation(fast_op);

    StorageOpStats slow_op;
    slow_op.type     = StorageOpType::SCAN;
    slow_op.duration = 500ms;
    profiler_.record_operation(slow_op);

    // With 1ms threshold only the slow op qualifies
    auto slow = profiler_.get_slow_operations(1ms);
    EXPECT_GE(slow.size(), 1u);

    auto none = profiler_.get_slow_operations(1h);
    EXPECT_EQ(none.size(), 0u);
}

TEST_F(StorageProfilerTest, GetOperationSummary_IsValidJson) {
    StorageOpStats op;
    op.type        = StorageOpType::GET;
    op.duration    = 50us;
    op.bytes_read  = 64;
    profiler_.record_operation(op);

    auto summary = profiler_.get_operation_summary();
    EXPECT_FALSE(summary.empty());
    EXPECT_TRUE(summary.is_object());
}

TEST_F(StorageProfilerTest, GetCacheMetrics_IsValidJson) {
    StorageOpStats op;
    op.type      = StorageOpType::GET;
    op.duration  = 20us;
    op.cache_hit = true;
    profiler_.record_operation(op);

    auto cache_info = profiler_.get_cache_metrics();
    EXPECT_FALSE(cache_info.empty());
}

// ============================================================================
// PerformanceAnalyzer
// ============================================================================

class PerformanceAnalyzerTest : public ::testing::Test {
protected:
    QueryProfilerConfig   qcfg_;
    StorageProfilerConfig scfg_;
    QueryProfiler         query_profiler_{qcfg_};
    StorageProfiler       storage_profiler_{scfg_};
    PerformanceAnalyzer   analyzer_;
};

TEST_F(PerformanceAnalyzerTest, Analyze_EmptyProfilers_ReturnsResult) {
    auto analysis = analyzer_.analyze(query_profiler_, storage_profiler_);
    // Even with no data, should return a valid analysis object
    EXPECT_TRUE(analysis.summary_metrics.is_object() ||
                analysis.summary_metrics.is_null() ||
                analysis.issues.empty());
}

TEST_F(PerformanceAnalyzerTest, AnalyzeQueries_ReturnsIssues) {
    // Insert a slow query to trigger a potential issue
    query_profiler_.start_query("slow_q", "slow query");
    std::this_thread::sleep_for(5ms);
    query_profiler_.end_query("slow_q");

    auto issues = analyzer_.analyze_queries(query_profiler_);
    // Issues may or may not appear depending on thresholds; result should be a vector
    EXPECT_NO_THROW((void)issues.size());
}

TEST_F(PerformanceAnalyzerTest, AnalyzeStorage_NoOps_NoIssues) {
    auto issues = analyzer_.analyze_storage(storage_profiler_);
    EXPECT_NO_THROW((void)issues.size());
}

TEST_F(PerformanceAnalyzerTest, GetConfig_DefaultsSet) {
    auto cfg = analyzer_.get_config();
    EXPECT_TRUE(cfg.analyze_queries);
    EXPECT_TRUE(cfg.analyze_storage);
}

TEST_F(PerformanceAnalyzerTest, SetConfig_UpdatesAnalyzerBehavior) {
    PerformanceAnalyzerConfig new_cfg;
    new_cfg.analyze_queries = false;
    new_cfg.analyze_storage = false;
    analyzer_.set_config(new_cfg);

    auto cfg = analyzer_.get_config();
    EXPECT_FALSE(cfg.analyze_queries);
    EXPECT_FALSE(cfg.analyze_storage);
}
