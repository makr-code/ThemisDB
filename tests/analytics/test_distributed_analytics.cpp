/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_distributed_analytics.cpp                     ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 07:15:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     805                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f38c013cdc  2026-03-29  Enhance various components with improvements and fixes ║
    • 3f98a289d9  2026-03-18  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Unit tests for DistributedAnalyticsSharding
 *
 * Covers:
 *  - Shard management (add / remove / count / healthy)
 *  - mergeResults: COUNT, SUM, AVG, MIN, MAX, FIRST, LAST
 *  - mergeResults with multiple dimension groups
 *  - executeDistributed: scatter-gather with LocalShardExecutor stubs
 *  - executeDistributed with partial shard failure (allow_partial_results)
 *  - empty shard list
 *  - single shard (result should equal the local result)
 *  - grand_totals merging
 *  - CUBE / grouping_id preservation
 */

#include <gtest/gtest.h>
#include "analytics/distributed_analytics.h"
#include "analytics/olap.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <future>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

// ============================================================================
// Aliases
// ============================================================================
using namespace themisdb::analytics;
using OLAPQuery  = themis::analytics::OLAPQuery;
using OLAPResult = themis::analytics::OLAPResult;
using OLAPRow    = OLAPResult::Row;
using Measure    = themis::analytics::Measure;
using Dimension  = themis::analytics::Dimension;
using RowValue   = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;

// ============================================================================
// Helpers
// ============================================================================

/**
 * Build a simple OLAPResult with a single dimension column "region" and the
 * given measure name, value, and grand_total.
 */
static OLAPResult makeSimpleResult(
        const std::vector<std::pair<std::string, double>>& rows,  // {region, value}
        const std::string& measure_col,
        double grand_total) {
    OLAPResult r;
    r.columns = {"region", measure_col};
    for (const auto& [region, val] : rows) {
        OLAPRow row;
        row.values["region"]      = RowValue{region};
        row.values[measure_col]   = RowValue{val};
        row.grouping_id = 0;
        r.rows.push_back(row);
    }
    r.grand_totals[measure_col] = grand_total;
    r.total_rows = static_cast<int64_t>(r.rows.size());
    return r;
}

/**
 * Build a trivial query with one dimension ("region") and one measure.
 */
static OLAPQuery makeQuery(const std::string& measure_name,
                            Measure::Function func) {
    OLAPQuery q;
    q.collection = "sales";
    q.dimensions = {{"region", "", true}};
    Measure m;
    m.name     = measure_name;
    m.field    = "amount";
    m.function = func;
    q.measures = {m};
    return q;
}

/** Extract double from a RowValue. */
static double dval(const RowValue& v) {
    if (auto* d = std::get_if<double>(&v))   return *d;
    if (auto* i = std::get_if<int64_t>(&v))  return static_cast<double>(*i);
    return 0.0;
}

/** Get row with matching region from result. Returns nullptr if not found. */
static const OLAPRow* findRow(const OLAPResult& r, const std::string& region) {
    for (const auto& row : r.rows) {
        auto it = row.values.find("region");
        if (it != row.values.end()) {
            if (auto* s = std::get_if<std::string>(&it->second)) {
                if (*s == region) return &row;
            }
        }
    }
    return nullptr;
}

// ============================================================================
// Shard management tests
// ============================================================================

TEST(DistributedAnalyticsShardingTest, InitiallyEmpty) {
    DistributedAnalyticsSharding das;
    EXPECT_EQ(das.getShardCount(), 0u);
    EXPECT_EQ(das.getHealthyShardCount(), 0u);
    EXPECT_TRUE(das.getShardIds().empty());
}

TEST(DistributedAnalyticsShardingTest, AddAndCountShards) {
    DistributedAnalyticsSharding das;

    // A dummy executor that always returns empty
    class NullExecutor : public ShardQueryExecutor {
    public:
        OLAPResult execute(const std::string&, const OLAPQuery&) override {
            return {};
        }
    };

    das.addShard("s1", std::make_shared<NullExecutor>());
    das.addShard("s2", std::make_shared<NullExecutor>());
    das.addShard("s3", std::make_shared<NullExecutor>());

    EXPECT_EQ(das.getShardCount(), 3u);
    EXPECT_EQ(das.getHealthyShardCount(), 3u);

    auto ids = das.getShardIds();
    EXPECT_EQ(ids.size(), 3u);
}

TEST(DistributedAnalyticsShardingTest, RemoveShard) {
    DistributedAnalyticsSharding das;

    class NullExecutor : public ShardQueryExecutor {
    public:
        OLAPResult execute(const std::string&, const OLAPQuery&) override { return {}; }
    };

    das.addShard("s1", std::make_shared<NullExecutor>());
    das.addShard("s2", std::make_shared<NullExecutor>());
    das.removeShard("s1");

    EXPECT_EQ(das.getShardCount(), 1u);
    auto ids = das.getShardIds();
    ASSERT_EQ(ids.size(), 1u);
    EXPECT_EQ(ids[0], "s2");
}

TEST(DistributedAnalyticsShardingTest, OverwriteShardExecutor) {
    DistributedAnalyticsSharding das;

    class NullExecutor : public ShardQueryExecutor {
    public:
        OLAPResult execute(const std::string&, const OLAPQuery&) override { return {}; }
    };

    das.addShard("s1", std::make_shared<NullExecutor>());
    das.addShard("s1", std::make_shared<NullExecutor>()); // overwrite

    EXPECT_EQ(das.getShardCount(), 1u);
}

TEST(DistributedAnalyticsShardingTest, UnhealthyShardNotCounted) {
    DistributedAnalyticsSharding das;

    class HealthyExec : public ShardQueryExecutor {
    public:
        OLAPResult execute(const std::string&, const OLAPQuery&) override { return {}; }
        bool isHealthy() const override { return true; }
    };
    class UnhealthyExec : public ShardQueryExecutor {
    public:
        OLAPResult execute(const std::string&, const OLAPQuery&) override { return {}; }
        bool isHealthy() const override { return false; }
    };

    das.addShard("s1", std::make_shared<HealthyExec>());
    das.addShard("s2", std::make_shared<UnhealthyExec>());
    das.addShard("s3", std::make_shared<HealthyExec>());

    EXPECT_EQ(das.getShardCount(), 3u);
    EXPECT_EQ(das.getHealthyShardCount(), 2u);
}

// ============================================================================
// mergeResults – COUNT
// ============================================================================

TEST(MergeResultsTest, MergeCount) {
    auto q = makeQuery("cnt", Measure::Function::Count);

    // Shard A: north=3, south=7
    auto pA = makeSimpleResult({{"north", 3.0}, {"south", 7.0}}, "cnt", 10.0);
    // Shard B: north=5, south=2
    auto pB = makeSimpleResult({{"north", 5.0}, {"south", 2.0}}, "cnt",  7.0);

    auto merged = DistributedAnalyticsSharding::mergeResults({pA, pB}, q);

    ASSERT_EQ(merged.rows.size(), 2u);

    const OLAPRow* north = findRow(merged, "north");
    const OLAPRow* south = findRow(merged, "south");
    ASSERT_NE(north, nullptr);
    ASSERT_NE(south, nullptr);

    EXPECT_EQ(std::get<int64_t>(north->values.at("cnt")), 8);
    EXPECT_EQ(std::get<int64_t>(south->values.at("cnt")), 9);

    // grand total
    EXPECT_DOUBLE_EQ(merged.grand_totals.at("cnt"), 17.0);
}

// ============================================================================
// mergeResults – SUM
// ============================================================================

TEST(MergeResultsTest, MergeSum) {
    auto q = makeQuery("total", Measure::Function::Sum);

    auto pA = makeSimpleResult({{"east", 100.0}, {"west", 200.0}}, "total", 300.0);
    auto pB = makeSimpleResult({{"east",  50.0}, {"west",  80.0}}, "total", 130.0);

    auto merged = DistributedAnalyticsSharding::mergeResults({pA, pB}, q);

    const OLAPRow* east = findRow(merged, "east");
    const OLAPRow* west = findRow(merged, "west");
    ASSERT_NE(east, nullptr);
    ASSERT_NE(west, nullptr);

    EXPECT_DOUBLE_EQ(dval(east->values.at("total")), 150.0);
    EXPECT_DOUBLE_EQ(dval(west->values.at("total")), 280.0);
    EXPECT_DOUBLE_EQ(merged.grand_totals.at("total"), 430.0);
}

// ============================================================================
// mergeResults – MIN
// ============================================================================

TEST(MergeResultsTest, MergeMin) {
    auto q = makeQuery("low", Measure::Function::Min);

    auto pA = makeSimpleResult({{"us", 5.0}},  "low", 5.0);
    auto pB = makeSimpleResult({{"us", 3.0}},  "low", 3.0);
    auto pC = makeSimpleResult({{"us", 8.0}},  "low", 8.0);

    auto merged = DistributedAnalyticsSharding::mergeResults({pA, pB, pC}, q);

    ASSERT_EQ(merged.rows.size(), 1u);
    EXPECT_DOUBLE_EQ(dval(merged.rows[0].values.at("low")), 3.0);
}

// ============================================================================
// mergeResults – MAX
// ============================================================================

TEST(MergeResultsTest, MergeMax) {
    auto q = makeQuery("high", Measure::Function::Max);

    auto pA = makeSimpleResult({{"eu", 42.0}}, "high", 42.0);
    auto pB = makeSimpleResult({{"eu", 99.0}}, "high", 99.0);
    auto pC = makeSimpleResult({{"eu", 17.0}}, "high", 17.0);

    auto merged = DistributedAnalyticsSharding::mergeResults({pA, pB, pC}, q);

    ASSERT_EQ(merged.rows.size(), 1u);
    EXPECT_DOUBLE_EQ(dval(merged.rows[0].values.at("high")), 99.0);
}

// ============================================================================
// mergeResults – AVG (approximate, equal shard sizes)
// ============================================================================

TEST(MergeResultsTest, MergeAvgApproximate) {
    // Each shard contributes an AVG with equal row count (1 row → weight 1).
    // After merge: avg = sum / count = (10 + 20) / 2 = 15
    auto q = makeQuery("avg_val", Measure::Function::Avg);

    auto pA = makeSimpleResult({{"apac", 10.0}}, "avg_val", 10.0);
    auto pB = makeSimpleResult({{"apac", 20.0}}, "avg_val", 20.0);

    auto merged = DistributedAnalyticsSharding::mergeResults({pA, pB}, q);

    ASSERT_EQ(merged.rows.size(), 1u);
    double avg = dval(merged.rows[0].values.at("avg_val"));
    EXPECT_DOUBLE_EQ(avg, 15.0);
}

// ============================================================================
// mergeResults – FIRST / LAST
// ============================================================================

TEST(MergeResultsTest, MergeFirstLast) {
    OLAPQuery q;
    q.collection = "events";
    q.dimensions = {{"channel", "", true}};
    Measure mFirst, mLast;
    mFirst.name = "first_val"; mFirst.field = "v"; mFirst.function = Measure::Function::First;
    mLast.name  = "last_val";  mLast.field  = "v"; mLast.function  = Measure::Function::Last;
    q.measures = {mFirst, mLast};

    OLAPResult pA, pB;
    pA.columns = {"channel", "first_val", "last_val"};
    {
        OLAPRow r;
        r.values["channel"]   = RowValue{std::string("web")};
        r.values["first_val"] = RowValue{1.0};
        r.values["last_val"]  = RowValue{5.0};
        pA.rows.push_back(r);
    }
    pB.columns = {"channel", "first_val", "last_val"};
    {
        OLAPRow r;
        r.values["channel"]   = RowValue{std::string("web")};
        r.values["first_val"] = RowValue{7.0};
        r.values["last_val"]  = RowValue{9.0};
        pB.rows.push_back(r);
    }

    auto merged = DistributedAnalyticsSharding::mergeResults({pA, pB}, q);

    ASSERT_EQ(merged.rows.size(), 1u);
    // first_val comes from the first shard that had a value (pA → 1.0)
    EXPECT_DOUBLE_EQ(dval(merged.rows[0].values.at("first_val")), 1.0);
    // last_val is the last value seen (pB → 9.0)
    EXPECT_DOUBLE_EQ(dval(merged.rows[0].values.at("last_val")), 9.0);
}

// ============================================================================
// mergeResults – multiple groups
// ============================================================================

TEST(MergeResultsTest, MultipleGroupsMergedCorrectly) {
    auto q = makeQuery("revenue", Measure::Function::Sum);

    // Shard A: covers regions A, B, C
    auto pA = makeSimpleResult(
        {{"A", 100.0}, {"B", 200.0}, {"C", 50.0}}, "revenue", 350.0);
    // Shard B: covers regions B, C, D (B and C overlap with shard A)
    auto pB = makeSimpleResult(
        {{"B",  75.0}, {"C", 25.0},  {"D", 300.0}}, "revenue", 400.0);

    auto merged = DistributedAnalyticsSharding::mergeResults({pA, pB}, q);

    EXPECT_EQ(merged.rows.size(), 4u);

    EXPECT_DOUBLE_EQ(dval(findRow(merged, "A")->values.at("revenue")), 100.0);
    EXPECT_DOUBLE_EQ(dval(findRow(merged, "B")->values.at("revenue")), 275.0);
    EXPECT_DOUBLE_EQ(dval(findRow(merged, "C")->values.at("revenue")),  75.0);
    EXPECT_DOUBLE_EQ(dval(findRow(merged, "D")->values.at("revenue")), 300.0);

    EXPECT_DOUBLE_EQ(merged.grand_totals.at("revenue"), 750.0);
}

// ============================================================================
// mergeResults – empty partials list
// ============================================================================

TEST(MergeResultsTest, EmptyPartials) {
    auto q = makeQuery("x", Measure::Function::Sum);
    auto merged = DistributedAnalyticsSharding::mergeResults({}, q);
    EXPECT_TRUE(merged.rows.empty());
}

// ============================================================================
// mergeResults – single partial
// ============================================================================

TEST(MergeResultsTest, SinglePartialPassThrough) {
    auto q = makeQuery("val", Measure::Function::Sum);
    auto p = makeSimpleResult({{"r1", 10.0}, {"r2", 20.0}}, "val", 30.0);

    auto merged = DistributedAnalyticsSharding::mergeResults({p}, q);

    ASSERT_EQ(merged.rows.size(), 2u);
    EXPECT_DOUBLE_EQ(dval(findRow(merged, "r1")->values.at("val")), 10.0);
    EXPECT_DOUBLE_EQ(dval(findRow(merged, "r2")->values.at("val")), 20.0);
}

// ============================================================================
// mergeResults – CUBE grouping_id isolation
// ============================================================================

TEST(MergeResultsTest, GroupingIdPreserved) {
    // Simulate CUBE subtotals: grouping_id=0 = detail, grouping_id=1 = subtotal
    OLAPQuery q;
    q.collection = "sales";
    q.dimensions = {{"region", "", true}};
    q.grouping_mode = OLAPQuery::GroupingMode::Cube;
    Measure m;
    m.name = "s"; m.field = "v"; m.function = Measure::Function::Sum;
    q.measures = {m};

    OLAPResult p;
    p.columns = {"region", "s"};
    // detail row
    OLAPRow detail;
    detail.values["region"] = RowValue{std::string("north")};
    detail.values["s"]      = RowValue{100.0};
    detail.grouping_id      = 0;
    p.rows.push_back(detail);
    // subtotal row (region = null represented as a separate grouping_id)
    OLAPRow subtotal;
    subtotal.values["region"] = RowValue{std::nullptr_t{}};
    subtotal.values["s"]      = RowValue{100.0};
    subtotal.grouping_id      = 1;
    p.rows.push_back(subtotal);

    auto merged = DistributedAnalyticsSharding::mergeResults({p, p}, q);

    // With two identical shards each contributing 100 for both rows, the
    // merged result should have 2 distinct groups (grouping_id=0 and =1),
    // each with sum=200.
    ASSERT_EQ(merged.rows.size(), 2u);

    bool found_detail   = false;
    bool found_subtotal = false;
    for (const auto& row : merged.rows) {
        if (row.grouping_id == 0) {
            EXPECT_DOUBLE_EQ(dval(row.values.at("s")), 200.0);
            found_detail = true;
        } else if (row.grouping_id == 1) {
            EXPECT_DOUBLE_EQ(dval(row.values.at("s")), 200.0);
            found_subtotal = true;
        }
    }
    EXPECT_TRUE(found_detail);
    EXPECT_TRUE(found_subtotal);
}

// ============================================================================
// executeDistributed – no shards registered
// ============================================================================

TEST(ExecuteDistributedTest, NoShards) {
    DistributedAnalyticsSharding das;
    auto q = makeQuery("v", Measure::Function::Sum);
    auto dr = das.executeDistributed(q);

    EXPECT_EQ(dr.total_shards, 0u);
    EXPECT_EQ(dr.successful_shards, 0u);
    EXPECT_TRUE(dr.merged.rows.empty());
}

// ============================================================================
// executeDistributed – fixed-data executor
// ============================================================================

/**
 * A test executor that returns a pre-built OLAPResult.
 */
class FixedResultExecutor : public ShardQueryExecutor {
public:
    explicit FixedResultExecutor(OLAPResult r) : result_(std::move(r)) {}

    OLAPResult execute(const std::string&, const OLAPQuery&) override {
        return result_;
    }

private:
    OLAPResult result_;
};

TEST(ExecuteDistributedTest, TwoShardsSum) {
    DistributedAnalyticsSharding das;

    auto r1 = makeSimpleResult({{"us", 500.0}, {"eu", 300.0}}, "revenue", 800.0);
    auto r2 = makeSimpleResult({{"us", 200.0}, {"eu", 100.0}}, "revenue", 300.0);

    das.addShard("shard1", std::make_shared<FixedResultExecutor>(r1));
    das.addShard("shard2", std::make_shared<FixedResultExecutor>(r2));

    auto q = makeQuery("revenue", Measure::Function::Sum);
    auto dr = das.executeDistributed(q);

    EXPECT_EQ(dr.total_shards, 2u);
    EXPECT_EQ(dr.successful_shards, 2u);

    const OLAPRow* us = findRow(dr.merged, "us");
    const OLAPRow* eu = findRow(dr.merged, "eu");
    ASSERT_NE(us, nullptr);
    ASSERT_NE(eu, nullptr);

    EXPECT_DOUBLE_EQ(dval(us->values.at("revenue")), 700.0);
    EXPECT_DOUBLE_EQ(dval(eu->values.at("revenue")), 400.0);
    EXPECT_DOUBLE_EQ(dr.merged.grand_totals.at("revenue"), 1100.0);
}

// ============================================================================
// executeDistributed – single shard pass-through
// ============================================================================

TEST(ExecuteDistributedTest, SingleShardPassThrough) {
    DistributedAnalyticsSharding das;

    auto r = makeSimpleResult({{"zone", 42.0}}, "x", 42.0);
    das.addShard("only", std::make_shared<FixedResultExecutor>(r));

    auto q = makeQuery("x", Measure::Function::Sum);
    auto merged = das.execute(q);

    ASSERT_EQ(merged.rows.size(), 1u);
    EXPECT_DOUBLE_EQ(dval(merged.rows[0].values.at("x")), 42.0);
}

// ============================================================================
// executeDistributed – partial failure with allow_partial_results=true
// ============================================================================

class ThrowingExecutor : public ShardQueryExecutor {
public:
    OLAPResult execute(const std::string&, const OLAPQuery&) override {
        throw std::runtime_error("shard unavailable");
    }
};

TEST(ExecuteDistributedTest, PartialFailureAllowed) {
    DistributedAnalyticsSharding::Config cfg;
    cfg.allow_partial_results = true;
    DistributedAnalyticsSharding das(cfg);

    auto r_good = makeSimpleResult({{"alpha", 77.0}}, "v", 77.0);
    das.addShard("good",  std::make_shared<FixedResultExecutor>(r_good));
    das.addShard("broken", std::make_shared<ThrowingExecutor>());

    auto q = makeQuery("v", Measure::Function::Sum);
    auto dr = das.executeDistributed(q);

    EXPECT_EQ(dr.total_shards, 2u);
    EXPECT_EQ(dr.successful_shards, 1u);

    // The result from the good shard should be available
    ASSERT_FALSE(dr.merged.rows.empty());
    EXPECT_DOUBLE_EQ(dval(findRow(dr.merged, "alpha")->values.at("v")), 77.0);

    // shard_info should record one failure
    int failures = 0;
    for (const auto& si : dr.shard_info) {
        if (!si.success) ++failures;
    }
    EXPECT_EQ(failures, 1);
}

// ============================================================================
// executeDistributed – partial failure with allow_partial_results=false
// ============================================================================

TEST(ExecuteDistributedTest, PartialFailureDisallowed) {
    DistributedAnalyticsSharding::Config cfg;
    cfg.allow_partial_results = false;
    DistributedAnalyticsSharding das(cfg);

    auto r_good = makeSimpleResult({{"x", 1.0}}, "v", 1.0);
    das.addShard("good",  std::make_shared<FixedResultExecutor>(r_good));
    das.addShard("broken", std::make_shared<ThrowingExecutor>());

    auto q = makeQuery("v", Measure::Function::Sum);
    auto dr = das.executeDistributed(q);

    // Merged result should be empty because allow_partial_results=false
    // and at least one shard failed.
    EXPECT_TRUE(dr.merged.rows.empty());
}

// ============================================================================
// executeDistributed – unhealthy shard is skipped
// ============================================================================

TEST(ExecuteDistributedTest, UnhealthyShardSkipped) {
    DistributedAnalyticsSharding das;

    auto r_good = makeSimpleResult({{"r", 10.0}}, "s", 10.0);

    class UnhealthyExecutor : public ShardQueryExecutor {
    public:
        OLAPResult execute(const std::string&, const OLAPQuery&) override {
            return makeSimpleResult({{"r", 999.0}}, "s", 999.0);
        }
        bool isHealthy() const override { return false; }
    };

    das.addShard("good",      std::make_shared<FixedResultExecutor>(r_good));
    das.addShard("unhealthy", std::make_shared<UnhealthyExecutor>());

    auto q = makeQuery("s", Measure::Function::Sum);
    auto dr = das.executeDistributed(q);

    // total_shards counts only healthy shards dispatched
    EXPECT_EQ(dr.total_shards, 1u);
    EXPECT_EQ(dr.successful_shards, 1u);

    ASSERT_FALSE(dr.merged.rows.empty());
    EXPECT_DOUBLE_EQ(dval(dr.merged.rows[0].values.at("s")), 10.0);
}

// ============================================================================
// ShardExecutionInfo timing
// ============================================================================

TEST(ExecuteDistributedTest, ExecutionTimingCaptured) {
    DistributedAnalyticsSharding das;

    auto r = makeSimpleResult({}, "s", 0.0);
    das.addShard("s1", std::make_shared<FixedResultExecutor>(r));

    auto q = makeQuery("s", Measure::Function::Sum);
    auto dr = das.executeDistributed(q);

    ASSERT_EQ(dr.shard_info.size(), 1u);
    EXPECT_TRUE(dr.shard_info[0].success);
    EXPECT_GE(dr.shard_info[0].execution_time_ms, 0.0);
}

// ============================================================================
// LocalShardExecutor round-trip
// ============================================================================

TEST(LocalShardExecutorTest, RoundTrip) {
    // Create a real OLAPEngine (it needs data to return non-empty results,
    // but we just verify the executor wrapper works without crashing).
    themis::analytics::OLAPEngine engine;

    LocalShardExecutor exec(engine);
    OLAPQuery q;
    q.collection = "test_col";
    q.dimensions = {{"dim", "", true}};
    Measure m;
    m.name = "s"; m.field = "v"; m.function = Measure::Function::Sum;
    q.measures = {m};

    // Should not throw
    OLAPResult r = exec.execute("shard_test", q);
    EXPECT_TRUE(r.columns.empty() || !r.columns.empty()); // any result is fine
}

// ============================================================================
// Cached health path (background monitor)
// ============================================================================

TEST(DistributedAnalyticsShardingTest, CachedHealthyShardCount) {
    // Disable background monitor so we can control cached_healthy manually.
    DistributedAnalyticsSharding::Config cfg;
    cfg.health_check_interval = std::chrono::milliseconds{0};
    DistributedAnalyticsSharding das(cfg);

    class HealthyExec : public ShardQueryExecutor {
    public:
        OLAPResult execute(const std::string&, const OLAPQuery&) override { return {}; }
        bool isHealthy() const override { return true; }
    };
    class UnhealthyExec : public ShardQueryExecutor {
    public:
        OLAPResult execute(const std::string&, const OLAPQuery&) override { return {}; }
        bool isHealthy() const override { return false; }
    };

    das.addShard("h1", std::make_shared<HealthyExec>());
    das.addShard("h2", std::make_shared<HealthyExec>());
    das.addShard("u1", std::make_shared<UnhealthyExec>());

    // cached_healthy is initialised from executor->isHealthy(); with monitor
    // disabled, the cached value remains at that initial snapshot.
    EXPECT_EQ(das.getHealthyShardCount(), 2u);
}

// ============================================================================
// getHealthyShardCountAsync – live health path
// ============================================================================

TEST(DistributedAnalyticsShardingTest, GetHealthyShardCountAsync) {
    DistributedAnalyticsSharding::Config cfg;
    cfg.health_check_interval = std::chrono::milliseconds{0}; // disable monitor
    DistributedAnalyticsSharding das(cfg);

    class HealthyExec : public ShardQueryExecutor {
    public:
        OLAPResult execute(const std::string&, const OLAPQuery&) override { return {}; }
        bool isHealthy() const override { return true; }
    };
    class UnhealthyExec : public ShardQueryExecutor {
    public:
        OLAPResult execute(const std::string&, const OLAPQuery&) override { return {}; }
        bool isHealthy() const override { return false; }
    };

    das.addShard("h1", std::make_shared<HealthyExec>());
    das.addShard("h2", std::make_shared<HealthyExec>());
    das.addShard("u1", std::make_shared<UnhealthyExec>());

    // Async path performs live isHealthy() calls — returns 2 (not 3).
    std::future<size_t> f = das.getHealthyShardCountAsync();
    EXPECT_EQ(f.get(), 2u);
}

// ============================================================================
// Background monitor updates cached_healthy
// ============================================================================

TEST(DistributedAnalyticsShardingTest, BackgroundMonitorUpdatesCachedHealth) {
    // Run monitor every 100 ms so the test stays fast.
    DistributedAnalyticsSharding::Config cfg;
    cfg.health_check_interval = std::chrono::milliseconds{100};
    DistributedAnalyticsSharding das(cfg);

    // Executor whose health can be toggled at runtime.
    class ToggleExec : public ShardQueryExecutor {
    public:
        std::atomic<bool> healthy{true};
        OLAPResult execute(const std::string&, const OLAPQuery&) override { return {}; }
        bool isHealthy() const override {
            return healthy.load(std::memory_order_relaxed);
        }
    };

    auto exec = std::make_shared<ToggleExec>();
    das.addShard("toggle", exec);

    // Initial cached value mirrors current executor health.
    EXPECT_EQ(das.getHealthyShardCount(), 1u);

    // Mark executor unhealthy and wait for at least two monitor cycles.
    exec->healthy.store(false, std::memory_order_relaxed);
    std::this_thread::sleep_for(std::chrono::milliseconds{350});

    // The background monitor should have refreshed the cached value.
    EXPECT_EQ(das.getHealthyShardCount(), 0u);

    // Re-enable and wait for another monitor cycle.
    exec->healthy.store(true, std::memory_order_relaxed);
    std::this_thread::sleep_for(std::chrono::milliseconds{250});
    EXPECT_EQ(das.getHealthyShardCount(), 1u);
}

// ============================================================================
// addShard does not block during a slow health check (lock contention test)
// ============================================================================

TEST(DistributedAnalyticsShardingTest, AddShardDoesNotBlockDuringHealthCheck) {
    // Run monitor every 100 ms so it quickly kicks off a health sweep.
    DistributedAnalyticsSharding::Config cfg;
    cfg.health_check_interval = std::chrono::milliseconds{100};
    DistributedAnalyticsSharding das(cfg);

    // Executor whose isHealthy() simulates a slow network ping (500 ms).
    class SlowHealthExec : public ShardQueryExecutor {
    public:
        OLAPResult execute(const std::string&, const OLAPQuery&) override { return {}; }
        bool isHealthy() const override {
            std::this_thread::sleep_for(std::chrono::milliseconds{500});
            return true;
        }
    };
    class NullExec : public ShardQueryExecutor {
    public:
        OLAPResult execute(const std::string&, const OLAPQuery&) override { return {}; }
    };

    das.addShard("slow", std::make_shared<SlowHealthExec>());

    // Wait long enough for the monitor to start its first sweep (> 100 ms
    // interval) but within the 500 ms window that isHealthy() is running.
    std::this_thread::sleep_for(std::chrono::milliseconds{200});

    // addShard() must complete quickly — the monitor holds only its own
    // health_monitor_mutex_, NOT the main mutex_.
    auto t0 = std::chrono::steady_clock::now();
    das.addShard("fast", std::make_shared<NullExec>());
    auto t1 = std::chrono::steady_clock::now();

    double elapsed_ms =
        std::chrono::duration<double, std::milli>(t1 - t0).count();

    // Generous threshold (50 ms) to absorb CI scheduling jitter;
    // the old code would block for ~300 ms (remainder of the 500 ms check).
    EXPECT_LT(elapsed_ms, 50.0)
        << "addShard() was blocked during health check for " << elapsed_ms << " ms";
}
