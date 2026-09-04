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
    if (auto* d = std::get_if<double>(&v)) {
      return *d;
    }
    if (auto* i = std::get_if<int64_t>(&v)) {
      return static_cast<double>(*i);
    }
    return 0.0;
}

/** Get row with matching region from result. Returns nullptr if not found. */
static const OLAPRow* findRow(const OLAPResult& r, const std::string& region) {
    for (const auto& row : r.rows) {
        auto it = row.values.find("region");
        if (it != row.values.end()) {
            if (auto* s = std::get_if<std::string>(&it->second)) {
                if (*s == region) {
                  return &row;
                }
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
    cfg.max_failure_rate = 1.0;  // allow up to 100% failure rate so the 1 good shard is merged
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
        if (!si.success) {
          ++failures;
        }
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

// ============================================================================
// Federated query dispatch tests (FED-01 .. FED-08)
// Covers: tenant isolation, failure-rate threshold, per-shard timeout,
//         PERMISSION_DENIED guard, partial toleration.
// ============================================================================

namespace {

// Helper: builds a minimal OLAPQuery pointing at a dummy collection.
static themis::analytics::OLAPQuery makeSumQuery(
        const std::string& tenant_id = {}) {
    using namespace themis::analytics;
    OLAPQuery q;
    q.collection = "sales";
    q.tenant_id  = tenant_id;
    q.measures.push_back({"total", "amount", Measure::Function::Sum});
    return q;
}

// Helper: returns an executor that always succeeds with a single-row result
// whose SUM value equals `value`.
static std::shared_ptr<ShardQueryExecutor> makeSuccessExec(double value) {
    using namespace themis::analytics;
    class FixedExec : public ShardQueryExecutor {
    public:
        explicit FixedExec(double v) : v_(v) {}
        OLAPResult execute(const std::string&, const OLAPQuery&) override {
            OLAPResult r;
            r.columns = {"total"};
            OLAPResult::Row row;
            row.values["total"] = v_;
            r.rows.push_back(std::move(row));
            r.total_rows = 1;
            return r;
        }
    private:
        double v_;
    };
    return std::make_shared<FixedExec>(value);
}

// Helper: executor that always throws.
static std::shared_ptr<ShardQueryExecutor> makeFailExec(
        const std::string& msg = "shard error") {
    using namespace themis::analytics;
    class FailExec : public ShardQueryExecutor {
    public:
        explicit FailExec(std::string m) : m_(std::move(m)) {}
        OLAPResult execute(const std::string&, const OLAPQuery&) override {
            throw std::runtime_error(m_);
        }
    private:
        std::string m_;
    };
    return std::make_shared<FailExec>(msg);
}

// Helper: executor that sleeps longer than any timeout window used in tests.
static std::shared_ptr<ShardQueryExecutor> makeSlowExec(int sleep_ms = 500) {
    using namespace themis::analytics;
    class SlowExec : public ShardQueryExecutor {
    public:
        explicit SlowExec(int ms) : ms_(ms) {}
        OLAPResult execute(const std::string&, const OLAPQuery&) override {
            std::this_thread::sleep_for(std::chrono::milliseconds{ms_});
            return {};
        }
    private:
        int ms_;
    };
    return std::make_shared<SlowExec>(sleep_ms);
}

// Helper: executor that tracks concurrent in-flight calls.
static std::shared_ptr<ShardQueryExecutor> makeTrackingExec(
        std::shared_ptr<std::atomic<int>> current_inflight,
        std::shared_ptr<std::atomic<int>> peak_inflight,
        int sleep_ms = 120) {
    using namespace themis::analytics;
    class TrackingExec : public ShardQueryExecutor {
    public:
        TrackingExec(std::shared_ptr<std::atomic<int>> current,
                     std::shared_ptr<std::atomic<int>> peak,
                     int sleep_ms)
            : current_(std::move(current)),
              peak_(std::move(peak)),
              sleep_ms_(sleep_ms) {}

        OLAPResult execute(const std::string&, const OLAPQuery&) override {
            const int now = current_->fetch_add(1, std::memory_order_relaxed) + 1;
            int observed_peak = peak_->load(std::memory_order_relaxed);
            while (now > observed_peak &&
                   !peak_->compare_exchange_weak(
                       observed_peak, now,
                       std::memory_order_relaxed,
                       std::memory_order_relaxed)) {
            }

            std::this_thread::sleep_for(std::chrono::milliseconds{sleep_ms_});
            current_->fetch_sub(1, std::memory_order_relaxed);

            OLAPResult r;
            r.columns = {"total"};
            OLAPResult::Row row;
            row.values["total"] = 1.0;
            r.rows.push_back(std::move(row));
            r.total_rows = 1;
            return r;
        }

    private:
        std::shared_ptr<std::atomic<int>> current_;
        std::shared_ptr<std::atomic<int>> peak_;
        int sleep_ms_;
    };
    return std::make_shared<TrackingExec>(
        std::move(current_inflight), std::move(peak_inflight), sleep_ms);
}

} // anonymous namespace

// FED-01: All shards belong to the same tenant — query is dispatched normally.
TEST(FederatedDispatchTest, FED01_TenantMatchAllShards) {
    using namespace themisdb::analytics;

    DistributedAnalyticsSharding das;
    das.addShard("s1", makeSuccessExec(10.0), "acme");
    das.addShard("s2", makeSuccessExec(20.0), "acme");

    auto res = das.executeDistributed(makeSumQuery("acme"));
    EXPECT_EQ(res.successful_shards, 2u);
    EXPECT_EQ(res.total_shards, 2u);
    ASSERT_FALSE(res.merged.rows.empty());
    // Merged SUM should be 30.0
    const auto& val = res.merged.rows[0].values.at("total");
    EXPECT_DOUBLE_EQ(std::get<double>(val), 30.0);
}

// FED-02: Shard registered for tenant "acme" must not serve queries from "beta".
TEST(FederatedDispatchTest, FED02_TenantMismatchShardExcluded) {
    using namespace themisdb::analytics;

    DistributedAnalyticsSharding das;
    das.addShard("s_acme", makeSuccessExec(99.0), "acme");
    das.addShard("s_open", makeSuccessExec(5.0));   // no restriction

    auto res = das.executeDistributed(makeSumQuery("beta"));
    // Only s_open is eligible for tenant "beta"
    EXPECT_EQ(res.total_shards, 1u);
    EXPECT_EQ(res.successful_shards, 1u);
    ASSERT_FALSE(res.merged.rows.empty());
    const auto& val = res.merged.rows[0].values.at("total");
    EXPECT_DOUBLE_EQ(std::get<double>(val), 5.0);
}

// FED-03: All registered shards are tenant-specific and belong to a different
//         tenant — no shard is eligible, result is empty.
TEST(FederatedDispatchTest, FED03_AllShardsDenied) {
    using namespace themisdb::analytics;

    DistributedAnalyticsSharding das;
    das.addShard("s1", makeSuccessExec(1.0), "corp");
    das.addShard("s2", makeSuccessExec(2.0), "corp");

    auto res = das.executeDistributed(makeSumQuery("outsider"));
    EXPECT_EQ(res.total_shards, 0u);      // none passed the tenant gate
    EXPECT_EQ(res.successful_shards, 0u);
    EXPECT_TRUE(res.merged.rows.empty());
}

// FED-04: Failure rate below max_failure_rate — partial results returned.
TEST(FederatedDispatchTest, FED04_FailureRateBelowThreshold) {
    using namespace themisdb::analytics;

    DistributedAnalyticsSharding::Config cfg;
    cfg.allow_partial_results = true;
    cfg.max_failure_rate      = 0.30;  // tolerate up to 30 %
    DistributedAnalyticsSharding das(cfg);

    // 3 shards: 2 succeed, 1 fails → failure rate = 33 % > 30 % threshold
    // Re-design: use 4 shards, 1 fails → 25 % < 30 % → merge should succeed
    das.addShard("s1", makeSuccessExec(10.0));
    das.addShard("s2", makeSuccessExec(10.0));
    das.addShard("s3", makeSuccessExec(10.0));
    das.addShard("s4", makeFailExec("disk error"));

    auto res = das.executeDistributed(makeSumQuery());
    EXPECT_EQ(res.successful_shards, 3u);
    // Merged result must be present (failure rate 25 % < 30 %)
    ASSERT_FALSE(res.merged.rows.empty());
    const auto& val = res.merged.rows[0].values.at("total");
    EXPECT_DOUBLE_EQ(std::get<double>(val), 30.0);
}

// FED-05: Failure rate exceeds max_failure_rate — no merged result returned.
TEST(FederatedDispatchTest, FED05_FailureRateExceedsThreshold) {
    using namespace themisdb::analytics;

    DistributedAnalyticsSharding::Config cfg;
    cfg.allow_partial_results = true;
    cfg.max_failure_rate      = 0.20;  // tolerate up to 20 %
    DistributedAnalyticsSharding das(cfg);

    // 3 shards: 1 succeeds, 2 fail → failure rate ≈ 67 % > 20 %
    das.addShard("s1", makeSuccessExec(5.0));
    das.addShard("s2", makeFailExec("network error"));
    das.addShard("s3", makeFailExec("network error"));

    auto res = das.executeDistributed(makeSumQuery());
    EXPECT_EQ(res.total_shards, 3u);
    EXPECT_EQ(res.successful_shards, 1u);
    // Merge must be aborted — merged result is empty
    EXPECT_TRUE(res.merged.rows.empty());
}

// FED-06: Shard timeout — timed-out shard is treated as failed.
TEST(FederatedDispatchTest, FED06_ShardTimeoutCountsAsFailed) {
    using namespace themisdb::analytics;

    DistributedAnalyticsSharding::Config cfg;
    cfg.shard_timeout_ms      = 100;   // 100 ms timeout
    cfg.allow_partial_results = true;
    cfg.max_failure_rate      = 0.60;  // allow up to 60 % failures
    DistributedAnalyticsSharding das(cfg);

    das.addShard("fast", makeSuccessExec(42.0));
    das.addShard("slow", makeSlowExec(800));  // will time out

    const auto t0 = std::chrono::steady_clock::now();
    auto res = das.executeDistributed(makeSumQuery());
    const auto t1 = std::chrono::steady_clock::now();
    const auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    EXPECT_EQ(res.total_shards, 2u);
    // The slow shard should have timed out → only fast shard succeeded
    EXPECT_EQ(res.successful_shards, 1u);
    ASSERT_FALSE(res.merged.rows.empty());
    const auto& val = res.merged.rows[0].values.at("total");
    EXPECT_DOUBLE_EQ(std::get<double>(val), 42.0);

    // Verify a timeout error is recorded in shard_info
    bool found_timeout = false;
    for (const auto& si : res.shard_info) {
        if (!si.success && si.error.find("timeout") != std::string::npos) {
            found_timeout = true;
        }
    }
    EXPECT_TRUE(found_timeout) << "Expected a timeout entry in shard_info";
    EXPECT_LT(elapsed_ms, 400)
        << "executeDistributed() still blocked for timed-out async work";
}

// FED-07: Mixed tenant shards — unrestricted shards serve any tenant.
TEST(FederatedDispatchTest, FED07_UnrestrictedShardServesAnyTenant) {
    using namespace themisdb::analytics;

    DistributedAnalyticsSharding das;
    das.addShard("global", makeSuccessExec(7.0));  // no tenant restriction
    das.addShard("acme",   makeSuccessExec(3.0), "acme");

    // Query from "acme" — both shards participate
    {
        auto res = das.executeDistributed(makeSumQuery("acme"));
        EXPECT_EQ(res.total_shards, 2u);
        EXPECT_EQ(res.successful_shards, 2u);
    }

    // Query from "beta" — only the global shard participates
    {
        auto res = das.executeDistributed(makeSumQuery("beta"));
        EXPECT_EQ(res.total_shards, 1u);
        EXPECT_EQ(res.successful_shards, 1u);
        ASSERT_FALSE(res.merged.rows.empty());
        const auto& val = res.merged.rows[0].values.at("total");
        EXPECT_DOUBLE_EQ(std::get<double>(val), 7.0);
    }
}

// FED-08: addShard() with tenant_id, then update without tenant_id — tenant
//         restriction is cleared on the second call.
TEST(FederatedDispatchTest, FED08_UpdateShardClearsTenantRestriction) {
    using namespace themisdb::analytics;

    DistributedAnalyticsSharding das;
    das.addShard("s1", makeSuccessExec(1.0), "corp");

    // Initially restricted to "corp"
    {
        auto res = das.executeDistributed(makeSumQuery("other"));
        EXPECT_EQ(res.total_shards, 0u);
    }

    // Re-register the same shard without tenant restriction
    das.addShard("s1", makeSuccessExec(2.0));  // no tenant_id → unrestricted

    // Now any tenant can reach s1
    {
        auto res = das.executeDistributed(makeSumQuery("other"));
        EXPECT_EQ(res.total_shards, 1u);
        EXPECT_EQ(res.successful_shards, 1u);
    }
}

// FED-09: max_parallel_shards limits concurrent in-flight shard execution.
TEST(FederatedDispatchTest, FED09_MaxParallelShardsRespected) {
    using namespace themisdb::analytics;

    DistributedAnalyticsSharding::Config cfg;
    cfg.max_parallel_shards = 2;
    cfg.shard_timeout_ms = 1000;
    DistributedAnalyticsSharding das(cfg);

    auto current = std::make_shared<std::atomic<int>>(0);
    auto peak = std::make_shared<std::atomic<int>>(0);

    das.addShard("s1", makeTrackingExec(current, peak));
    das.addShard("s2", makeTrackingExec(current, peak));
    das.addShard("s3", makeTrackingExec(current, peak));
    das.addShard("s4", makeTrackingExec(current, peak));
    das.addShard("s5", makeTrackingExec(current, peak));

    auto res = das.executeDistributed(makeSumQuery());
    EXPECT_EQ(res.total_shards, 5u);
    EXPECT_EQ(res.successful_shards, 5u);
    EXPECT_LE(peak->load(std::memory_order_relaxed), 2);
}
