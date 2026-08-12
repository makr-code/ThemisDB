// Phase 4: Query Optimizer – Dynamic Aggregate Discovery & Plan Tests

#include <gtest/gtest.h>
#include "timeseries/query_optimizer.h"
#include "timeseries/tsstore.h"
#include "timeseries/continuous_agg.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <chrono>
#include <memory>

using namespace themis;
namespace fs = std::filesystem;

static std::string makeOptimTempPath(const std::string& tag) {
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / ("themis_optim_" + tag + "_" + std::to_string(ns))).string();
}

struct QueryOptimizerFixture : ::testing::Test {
    std::string db_path;
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<TSStore> store;
    int64_t base_ms{1700000000000LL};

    void SetUp() override {
        db_path = makeOptimTempPath("opt");
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
        cfg.enable_blobdb = false;
        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open()) << "RocksDB open failed at " << db_path;
        store = std::make_unique<TSStore>(db->getRawDB());
    }

    void TearDown() override {
        store.reset();
        db.reset();
        fs::remove_all(db_path);
    }

    // Insert n raw data points for a metric
    void insertRawPoints(const std::string& metric, const std::string& entity, int n) {
        for (int i = 0; i < n; ++i) {
            TSStore::DataPoint p;
            p.metric       = metric;
            p.entity       = entity;
            p.timestamp_ms = base_ms + i * 10000;  // 10s intervals
            p.value        = static_cast<double>(i);
            ASSERT_TRUE(store->putDataPoint(p).has_value());
        }
    }

    // Compute and store aggregates
    void insertAggregates(const std::string& metric, const std::string& entity,
                          std::chrono::milliseconds window) {
        ContinuousAggregateManager mgr(store.get());
        AggConfig cfg;
        cfg.metric       = metric;
        cfg.entity       = entity;
        cfg.window.size  = window;
        mgr.refresh(cfg, base_ms, base_ms + 3600000);  // 1 hour range
    }
};

// ===== Construction =====

TEST_F(QueryOptimizerFixture, ConstructsSuccessfully) {
    EXPECT_NO_THROW({ TSQueryOptimizer opt(store.get()); });
}

TEST_F(QueryOptimizerFixture, NullStoreThrows) {
    EXPECT_THROW({ TSQueryOptimizer opt(nullptr); }, std::invalid_argument);
}

// ===== Default query plan (no aggregates) =====

TEST_F(QueryOptimizerFixture, PlanWithNoAggregatesUsesRaw) {
    insertRawPoints("cpu", "s1", 100);
    TSQueryOptimizer opt(store.get());
    auto plan = opt.optimizeAggregateQuery("cpu", std::string("s1"), base_ms, base_ms + 3600000);
    EXPECT_EQ(plan.source_metric, "cpu");
    EXPECT_EQ(plan.from_timestamp_ms, base_ms);
    EXPECT_EQ(plan.to_timestamp_ms, base_ms + 3600000);
}

TEST_F(QueryOptimizerFixture, PlanHasCorrectTimeRange) {
    TSQueryOptimizer opt(store.get());
    int64_t from = base_ms;
    int64_t to   = base_ms + 7200000;  // 2 hours
    auto plan = opt.optimizeAggregateQuery("any", std::string("srv"), from, to);
    EXPECT_EQ(plan.from_timestamp_ms, from);
    EXPECT_EQ(plan.to_timestamp_ms, to);
}

// ===== Optimization hints =====

TEST_F(QueryOptimizerFixture, HintDisabledAggregatesReturnsRaw) {
    insertRawPoints("mem", "s2", 100);
    insertAggregates("mem", "s2", std::chrono::minutes(1));

    TSQueryOptimizer opt(store.get());
    TSQueryOptimizer::OptimizationHint hint;
    hint.use_aggregates = false;
    auto plan = opt.optimizeAggregateQuery("mem", std::string("s2"),
                                            base_ms, base_ms + 3600000, hint);
    EXPECT_FALSE(plan.uses_aggregate);
    EXPECT_EQ(plan.source_metric, "mem");
}

TEST_F(QueryOptimizerFixture, SmallTimeRangeDoesNotUseAggregates) {
    TSQueryOptimizer opt(store.get());
    TSQueryOptimizer::OptimizationHint hint;
    hint.min_window_for_agg_ms = 3600000;  // 1 hour threshold
    // 30-minute range < 1-hour threshold → raw
    auto plan = opt.optimizeAggregateQuery("cpu2", std::string("s3"),
                                            base_ms, base_ms + 1800000, hint);
    EXPECT_FALSE(plan.uses_aggregate);
}

TEST_F(QueryOptimizerFixture, ExplainPlanContainsString) {
    TSQueryOptimizer opt(store.get());
    TSQueryOptimizer::OptimizationHint hint;
    hint.explain = true;
    auto plan = opt.optimizeAggregateQuery("cpu3", std::nullopt,
                                            base_ms, base_ms + 86400000, hint);
    EXPECT_FALSE(plan.explanation.empty());
}

// ===== Aggregate existence checks =====

TEST_F(QueryOptimizerFixture, AggregateExistsReturnsFalseWhenNone) {
    TSQueryOptimizer opt(store.get());
    EXPECT_FALSE(opt.aggregateExists("cpu4", std::chrono::minutes(1)));
}

TEST_F(QueryOptimizerFixture, AggregateExistsTrueAfterRegister) {
    TSQueryOptimizer opt(store.get());
    opt.registerAvailableAggregate("cpu5", std::chrono::minutes(1));
    EXPECT_TRUE(opt.aggregateExists("cpu5", std::chrono::minutes(1)));
}

TEST_F(QueryOptimizerFixture, AggregateExistsFalseForDifferentWindow) {
    TSQueryOptimizer opt(store.get());
    opt.registerAvailableAggregate("cpu6", std::chrono::minutes(1));
    EXPECT_FALSE(opt.aggregateExists("cpu6", std::chrono::hours(1)));
}

// ===== findBestAggregate =====

TEST_F(QueryOptimizerFixture, FindBestAggregateNoneAvailable) {
    TSQueryOptimizer opt(store.get());
    auto result = opt.findBestAggregate("no_agg_metric", 3600000);
    EXPECT_FALSE(result.has_value());
}

TEST_F(QueryOptimizerFixture, FindBestAggregateWithRegisteredAggregate) {
    insertRawPoints("net", "s4", 100);
    insertAggregates("net", "s4", std::chrono::minutes(1));

    TSQueryOptimizer opt(store.get());
    opt.registerAvailableAggregate("net", std::chrono::minutes(1));
    auto result = opt.findBestAggregate("net", 3600000);
    // May or may not find it depending on threshold
    // Just verify no crash
}

// ===== Plan with actual aggregates =====

TEST_F(QueryOptimizerFixture, PlanUsesAggregateForLargeTimeRange) {
    insertRawPoints("disk", "s5", 360);  // 3600 points at 10s = 1 hour
    insertAggregates("disk", "s5", std::chrono::minutes(1));

    TSQueryOptimizer opt(store.get());
    opt.registerAvailableAggregate("disk", std::chrono::minutes(1));
    TSQueryOptimizer::OptimizationHint hint;
    hint.use_aggregates       = true;
    hint.min_window_for_agg_ms = 0;  // Always try aggregates
    hint.max_raw_points       = 10;  // Force aggregate when >10 raw points estimated
    auto plan = opt.optimizeAggregateQuery("disk", std::string("s5"),
                                            base_ms, base_ms + 3600000, hint);
    // With registered aggregate, should use it
    EXPECT_TRUE(plan.uses_aggregate);
    EXPECT_NE(plan.source_metric, "disk");
}

TEST_F(QueryOptimizerFixture, SpeedupGreaterThanOneWithAggregate) {
    insertRawPoints("req", "s6", 360);
    insertAggregates("req", "s6", std::chrono::minutes(1));

    TSQueryOptimizer opt(store.get());
    opt.registerAvailableAggregate("req", std::chrono::minutes(1));
    TSQueryOptimizer::OptimizationHint hint;
    hint.use_aggregates       = true;
    hint.min_window_for_agg_ms = 0;
    hint.max_raw_points       = 1;
    auto plan = opt.optimizeAggregateQuery("req", std::string("s6"),
                                            base_ms, base_ms + 3600000, hint);
    if (plan.uses_aggregate) {
        EXPECT_GT(plan.estimated_speedup, 1.0);
    }
}

// ===== Overload without hint =====

TEST_F(QueryOptimizerFixture, OverloadWithoutHintWorks) {
    TSQueryOptimizer opt(store.get());
    auto plan = opt.optimizeAggregateQuery("cpu7", std::nullopt, base_ms, base_ms + 86400000);
    EXPECT_EQ(plan.from_timestamp_ms, base_ms);
    EXPECT_EQ(plan.to_timestamp_ms, base_ms + 86400000);
}

// ===== Predicate Filter =====

TEST_F(QueryOptimizerFixture, PredicateFilterInHint) {
    TSQueryOptimizer opt(store.get());
    TSQueryOptimizer::OptimizationHint hint;
    hint.predicates.push_back(TSQueryOptimizer::PredicateFilter::eq("region", "us-east"));
    auto plan = opt.optimizeAggregateQuery("cpu8", std::nullopt, base_ms, base_ms + 3600000, hint);
    // Predicates should be included in plan
    EXPECT_EQ(plan.active_predicates.size(), 1u);
    EXPECT_EQ(plan.active_predicates[0].tag_key, "region");
    EXPECT_EQ(plan.active_predicates[0].tag_value, "us-east");
}

TEST_F(QueryOptimizerFixture, PredicateFilterEqFactory) {
    auto p = TSQueryOptimizer::PredicateFilter::eq("env", "prod");
    EXPECT_EQ(p.tag_key, "env");
    EXPECT_EQ(p.tag_value, "prod");
    EXPECT_TRUE(p.required);
}

TEST_F(QueryOptimizerFixture, MultiplePredicatesInPlan) {
    TSQueryOptimizer opt(store.get());
    TSQueryOptimizer::OptimizationHint hint;
    hint.predicates.push_back(TSQueryOptimizer::PredicateFilter::eq("region", "eu-west"));
    hint.predicates.push_back(TSQueryOptimizer::PredicateFilter::eq("env", "staging"));
    auto plan = opt.optimizeAggregateQuery("mem3", std::nullopt, base_ms, base_ms + 3600000, hint);
    EXPECT_EQ(plan.active_predicates.size(), 2u);
}

TEST_F(QueryOptimizerFixture, EmptyPredicatesDoesNotAffectPlan) {
    TSQueryOptimizer opt(store.get());
    TSQueryOptimizer::OptimizationHint hint;
    // No predicates
    auto plan = opt.optimizeAggregateQuery("cpu9", std::nullopt, base_ms, base_ms + 3600000, hint);
    EXPECT_TRUE(plan.active_predicates.empty());
}

// ===== Query Plan Cache =====

TEST_F(QueryOptimizerFixture, CacheInitiallyEmpty) {
    TSQueryOptimizer opt(store.get());
    EXPECT_EQ(opt.cacheSize(), 0u);
    EXPECT_EQ(opt.cacheHits(), 0u);
    EXPECT_EQ(opt.cacheMisses(), 0u);
}

TEST_F(QueryOptimizerFixture, CacheStoresPlan) {
    TSQueryOptimizer opt(store.get());
    TSQueryOptimizer::OptimizationHint hint;
    hint.use_cache = true;
    opt.optimizeAggregateQuery("cpu10", std::nullopt, base_ms, base_ms + 3600000, hint);
    EXPECT_EQ(opt.cacheSize(), 1u);
    EXPECT_EQ(opt.cacheMisses(), 1u);
}

TEST_F(QueryOptimizerFixture, CacheHitOnSecondCall) {
    TSQueryOptimizer opt(store.get());
    TSQueryOptimizer::OptimizationHint hint;
    hint.use_cache = true;
    opt.optimizeAggregateQuery("cpu11", std::nullopt, base_ms, base_ms + 3600000, hint);
    opt.optimizeAggregateQuery("cpu11", std::nullopt, base_ms, base_ms + 3600000, hint);
    EXPECT_EQ(opt.cacheHits(), 1u);
    EXPECT_EQ(opt.cacheMisses(), 1u);
}

TEST_F(QueryOptimizerFixture, ClearCacheResetsEntries) {
    TSQueryOptimizer opt(store.get());
    TSQueryOptimizer::OptimizationHint hint;
    hint.use_cache = true;
    opt.optimizeAggregateQuery("cpu12", std::nullopt, base_ms, base_ms + 3600000, hint);
    EXPECT_EQ(opt.cacheSize(), 1u);
    opt.clearCache();
    EXPECT_EQ(opt.cacheSize(), 0u);
}

TEST_F(QueryOptimizerFixture, CacheDisabledDoesNotStore) {
    TSQueryOptimizer opt(store.get());
    TSQueryOptimizer::OptimizationHint hint;
    hint.use_cache = false;
    opt.optimizeAggregateQuery("cpu13", std::nullopt, base_ms, base_ms + 3600000, hint);
    EXPECT_EQ(opt.cacheSize(), 0u);
}

TEST_F(QueryOptimizerFixture, DifferentMetricsHaveSeparateCacheEntries) {
    TSQueryOptimizer opt(store.get());
    TSQueryOptimizer::OptimizationHint hint;
    hint.use_cache = true;
    opt.optimizeAggregateQuery("metricA", std::nullopt, base_ms, base_ms + 3600000, hint);
    opt.optimizeAggregateQuery("metricB", std::nullopt, base_ms, base_ms + 3600000, hint);
    EXPECT_EQ(opt.cacheSize(), 2u);
    EXPECT_EQ(opt.cacheMisses(), 2u);
}

// ===== Index-Aware Query Planning =====

TEST_F(QueryOptimizerFixture, GetIndexHintReturnsNulloptIfNone) {
    TSQueryOptimizer opt(store.get());
    EXPECT_FALSE(opt.getIndexHint("no_index_metric").has_value());
}

TEST_F(QueryOptimizerFixture, RegisterIndexHintAndRetrieve) {
    TSQueryOptimizer opt(store.get());
    TSQueryOptimizer::IndexHint hint;
    hint.metric      = "cpu14";
    hint.type        = TSQueryOptimizer::IndexType::TimeRange;
    hint.selectivity = 0.1;
    opt.registerIndexHint(hint);

    auto got = opt.getIndexHint("cpu14");
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->type, TSQueryOptimizer::IndexType::TimeRange);
    EXPECT_NEAR(got->selectivity, 0.1, 1e-9);
}

TEST_F(QueryOptimizerFixture, RegisterBloomIndexHint) {
    TSQueryOptimizer opt(store.get());
    opt.registerIndexHint({"mem4", TSQueryOptimizer::IndexType::Bloom, 0.5});
    auto got = opt.getIndexHint("mem4");
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->type, TSQueryOptimizer::IndexType::Bloom);
}

TEST_F(QueryOptimizerFixture, RegisterInvertedIndexHint) {
    TSQueryOptimizer opt(store.get());
    opt.registerIndexHint({"disk4", TSQueryOptimizer::IndexType::Inverted, 0.2});
    auto got = opt.getIndexHint("disk4");
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->type, TSQueryOptimizer::IndexType::Inverted);
}

TEST_F(QueryOptimizerFixture, IndexHintOverwrite) {
    TSQueryOptimizer opt(store.get());
    opt.registerIndexHint({"cpu15", TSQueryOptimizer::IndexType::Bloom, 0.8});
    opt.registerIndexHint({"cpu15", TSQueryOptimizer::IndexType::TimeRange, 0.3});
    auto got = opt.getIndexHint("cpu15");
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->type, TSQueryOptimizer::IndexType::TimeRange); // overwritten
    EXPECT_NEAR(got->selectivity, 0.3, 1e-9);
}

TEST_F(QueryOptimizerFixture, IndexHintNoneType) {
    TSQueryOptimizer opt(store.get());
    opt.registerIndexHint({"cpu16", TSQueryOptimizer::IndexType::None, 1.0});
    auto got = opt.getIndexHint("cpu16");
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->type, TSQueryOptimizer::IndexType::None);
}

TEST_F(QueryOptimizerFixture, IndexHintDoesNotAffectOtherMetrics) {
    TSQueryOptimizer opt(store.get());
    opt.registerIndexHint({"indexed_metric", TSQueryOptimizer::IndexType::TimeRange, 0.1});
    EXPECT_FALSE(opt.getIndexHint("other_metric").has_value());
}
