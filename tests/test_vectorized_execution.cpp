/**
 * Unit tests for the Vectorized Execution Engine (query module facade).
 *
 * Covers:
 *  - VectorizedPredicate factory methods and all Op variants
 *  - VectorizedQueryPlan stage composition
 *  - VectorizedExecutionEngine::execute (filter, project, aggregate, sort)
 *  - Convenience methods: filter(), aggregate(), project(), sort()
 *  - Stats tracking and resetStats()
 *  - Empty input, limit, mixed-type rows, null handling
 */

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "query/vectorized_execution.h"

using namespace themis::query;
using json = nlohmann::json;

// ============================================================================
// Helpers
// ============================================================================

/** Build a small collection of product-like JSON rows for reuse across tests. */
static std::vector<json> makeProductRows() {
    return {
        {{"id", 1},  {"category", "A"}, {"price", 10.0}, {"active", true}},
        {{"id", 2},  {"category", "B"}, {"price", 20.0}, {"active", false}},
        {{"id", 3},  {"category", "A"}, {"price", 30.0}, {"active", true}},
        {{"id", 4},  {"category", "B"}, {"price", 40.0}, {"active", false}},
        {{"id", 5},  {"category", "A"}, {"price", 50.0}, {"active", true}},
    };
}

// ============================================================================
// VectorizedPredicate factory tests
// ============================================================================

TEST(VectorizedPredicateTest, EqFactory) {
    auto p = VectorizedPredicate::eq("price", 30.0);
    EXPECT_EQ(p.field, "price");
    EXPECT_EQ(p.op, VectorizedPredicate::Op::Eq);
    EXPECT_NEAR(p.value.get<double>(), 30.0, 1e-9);
}

TEST(VectorizedPredicateTest, NeFactory) {
    auto p = VectorizedPredicate::ne("cat", "A");
    EXPECT_EQ(p.op, VectorizedPredicate::Op::Ne);
}

TEST(VectorizedPredicateTest, LtFactory) {
    auto p = VectorizedPredicate::lt("id", 3);
    EXPECT_EQ(p.op, VectorizedPredicate::Op::Lt);
}

TEST(VectorizedPredicateTest, LeFactory) {
    auto p = VectorizedPredicate::le("id", 3);
    EXPECT_EQ(p.op, VectorizedPredicate::Op::Le);
}

TEST(VectorizedPredicateTest, GtFactory) {
    auto p = VectorizedPredicate::gt("price", 25.0);
    EXPECT_EQ(p.op, VectorizedPredicate::Op::Gt);
}

TEST(VectorizedPredicateTest, GeFactory) {
    auto p = VectorizedPredicate::ge("price", 30.0);
    EXPECT_EQ(p.op, VectorizedPredicate::Op::Ge);
}

TEST(VectorizedPredicateTest, IsNullFactory) {
    auto p = VectorizedPredicate::isNull("score");
    EXPECT_EQ(p.op, VectorizedPredicate::Op::IsNull);
    EXPECT_EQ(p.field, "score");
}

TEST(VectorizedPredicateTest, IsNotNullFactory) {
    auto p = VectorizedPredicate::isNotNull("score");
    EXPECT_EQ(p.op, VectorizedPredicate::Op::IsNotNull);
}

// ============================================================================
// VectorizedQueryPlan composition tests
// ============================================================================

TEST(VectorizedQueryPlanTest, EmptyPlanHasZeroStages) {
    VectorizedQueryPlan plan;
    EXPECT_EQ(0u, plan.stageCount());
    EXPECT_FALSE(plan.limit().has_value());
}

TEST(VectorizedQueryPlanTest, AddFilterStage) {
    VectorizedQueryPlan plan;
    plan.addFilter({VectorizedPredicate::gt("price", 20.0)});
    EXPECT_EQ(1u, plan.stageCount());
    EXPECT_EQ(VectorizedQueryPlan::StageType::Filter,
              plan.stages()[0].type);
}

TEST(VectorizedQueryPlanTest, AddMultipleStages) {
    VectorizedQueryPlan plan;
    plan.addFilter({VectorizedPredicate::gt("price", 10.0)})
        .addProject({"id", "price"})
        .addAggregate({{
            .result_field = "total",
            .input_field  = "price",
            .function     = VectorizedAggregation::Function::Sum,
        }})
        .addSort({{"price", false}});
    EXPECT_EQ(4u, plan.stageCount());
}

TEST(VectorizedQueryPlanTest, SetLimit) {
    VectorizedQueryPlan plan;
    plan.setLimit(3u);
    ASSERT_TRUE(plan.limit().has_value());
    EXPECT_EQ(3u, *plan.limit());
}

// ============================================================================
// VectorizedExecutionEngine – filter tests
// ============================================================================

class VectorizedEngineTest : public ::testing::Test {
protected:
    VectorizedExecutionEngine engine;
    std::vector<json> rows = makeProductRows();
};

TEST_F(VectorizedEngineTest, FilterGtPrice30) {
    auto result = engine.filter(rows, {VectorizedPredicate::gt("price", 30.0)});
    ASSERT_TRUE(result.has_value());
    // price > 30 → ids 4 and 5
    ASSERT_EQ(2u, result->size());
}

TEST_F(VectorizedEngineTest, FilterEqCategory) {
    auto result = engine.filter(rows, {VectorizedPredicate::eq("category", "A")});
    ASSERT_TRUE(result.has_value());
    // category == "A" → ids 1, 3, 5
    ASSERT_EQ(3u, result->size());
}

TEST_F(VectorizedEngineTest, FilterLePrice20) {
    auto result = engine.filter(rows, {VectorizedPredicate::le("price", 20.0)});
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(2u, result->size());
}

TEST_F(VectorizedEngineTest, FilterLtPrice20) {
    auto result = engine.filter(rows, {VectorizedPredicate::lt("price", 20.0)});
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(1u, result->size());
}

TEST_F(VectorizedEngineTest, FilterNeCategory) {
    auto result = engine.filter(rows, {VectorizedPredicate::ne("category", "A")});
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(2u, result->size());
}

TEST_F(VectorizedEngineTest, FilterGePrice50) {
    auto result = engine.filter(rows, {VectorizedPredicate::ge("price", 50.0)});
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(1u, result->size());
}

TEST_F(VectorizedEngineTest, FilterNullField) {
    std::vector<json> rows_with_null = {
        {{"id", 1}, {"score", nullptr}},
        {{"id", 2}, {"score", 42}},
        {{"id", 3}},  // missing score
    };
    auto result = engine.filter(rows_with_null,
                                {VectorizedPredicate::isNull("score")});
    ASSERT_TRUE(result.has_value());
    // rows 1 (explicit null) and 3 (missing field → null)
    EXPECT_EQ(2u, result->size());
}

TEST_F(VectorizedEngineTest, FilterNotNullField) {
    std::vector<json> rows_with_null = {
        {{"id", 1}, {"score", nullptr}},
        {{"id", 2}, {"score", 42}},
        {{"id", 3}, {"score", 7}},
    };
    auto result = engine.filter(rows_with_null,
                                {VectorizedPredicate::isNotNull("score")});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(2u, result->size());
}

// ============================================================================
// VectorizedExecutionEngine – project tests
// ============================================================================

TEST_F(VectorizedEngineTest, ProjectSubsetOfFields) {
    auto result = engine.project(rows, {"id", "price"});
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(rows.size(), result->size());
    // Each row should only contain the projected fields
    for (const auto& row : *result) {
        EXPECT_TRUE(row.contains("id"));
        EXPECT_TRUE(row.contains("price"));
        EXPECT_FALSE(row.contains("category"));
        EXPECT_FALSE(row.contains("active"));
    }
}

// ============================================================================
// VectorizedExecutionEngine – aggregate tests
// ============================================================================

TEST_F(VectorizedEngineTest, AggregateSumPrice) {
    auto result = engine.aggregate(rows, {{
        .result_field = "total_price",
        .input_field  = "price",
        .function     = VectorizedAggregation::Function::Sum,
    }});
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(1u, result->size());
    EXPECT_NEAR(150.0, (*result)[0]["total_price"].get<double>(), 1e-6);
}

TEST_F(VectorizedEngineTest, AggregateCount) {
    auto result = engine.aggregate(rows, {{
        .result_field = "n",
        .input_field  = "",
        .function     = VectorizedAggregation::Function::Count,
    }});
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(1u, result->size());
    // Aggregation result columns are stored as doubles by the analytics engine
    EXPECT_NEAR(5.0, (*result)[0]["n"].get<double>(), 1e-6);
}

TEST_F(VectorizedEngineTest, AggregateAvgPrice) {
    auto result = engine.aggregate(rows, {{
        .result_field = "avg_price",
        .input_field  = "price",
        .function     = VectorizedAggregation::Function::Avg,
    }});
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(1u, result->size());
    EXPECT_NEAR(30.0, (*result)[0]["avg_price"].get<double>(), 1e-6);
}

TEST_F(VectorizedEngineTest, AggregateMinPrice) {
    auto result = engine.aggregate(rows, {{
        .result_field = "min_price",
        .input_field  = "price",
        .function     = VectorizedAggregation::Function::Min,
    }});
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(1u, result->size());
    EXPECT_NEAR(10.0, (*result)[0]["min_price"].get<double>(), 1e-6);
}

TEST_F(VectorizedEngineTest, AggregateMaxPrice) {
    auto result = engine.aggregate(rows, {{
        .result_field = "max_price",
        .input_field  = "price",
        .function     = VectorizedAggregation::Function::Max,
    }});
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(1u, result->size());
    EXPECT_NEAR(50.0, (*result)[0]["max_price"].get<double>(), 1e-6);
}

TEST_F(VectorizedEngineTest, AggregateGroupByCategory) {
    auto result = engine.aggregate(rows, {{
        .result_field = "total",
        .input_field  = "price",
        .function     = VectorizedAggregation::Function::Sum,
        .group_by     = {"category"},
    }});
    ASSERT_TRUE(result.has_value());
    // Two distinct category groups (A and B)
    ASSERT_EQ(2u, result->size());

    double sum_A = 0.0, sum_B = 0.0;
    for (const auto& row : *result) {
        const std::string cat = row["category"].get<std::string>();
        const double total    = row["total"].get<double>();
        if (cat == "A") {
          sum_A = total;
        }
        if (cat == "B") {
          sum_B = total;
        }
    }
    // A: 10+30+50=90, B: 20+40=60
    EXPECT_NEAR(90.0, sum_A, 1e-6);
    EXPECT_NEAR(60.0, sum_B, 1e-6);
}

// ============================================================================
// VectorizedExecutionEngine – sort tests
// ============================================================================

TEST_F(VectorizedEngineTest, SortByPriceDescending) {
    auto result = engine.sort(rows, {{"price", false}});
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(rows.size(), result->size());
    EXPECT_NEAR(50.0, (*result)[0]["price"].get<double>(), 1e-9);
    EXPECT_NEAR(10.0, (*result)[4]["price"].get<double>(), 1e-9);
}

TEST_F(VectorizedEngineTest, SortByPriceAscending) {
    // Reverse the rows first
    auto reversed = rows;
    std::reverse(reversed.begin(), reversed.end());

    auto result = engine.sort(reversed, {{"price", true}});
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(rows.size(), result->size());
    EXPECT_NEAR(10.0, (*result)[0]["price"].get<double>(), 1e-9);
    EXPECT_NEAR(50.0, (*result)[4]["price"].get<double>(), 1e-9);
}

// ============================================================================
// VectorizedExecutionEngine – combined plan tests
// ============================================================================

TEST_F(VectorizedEngineTest, FilterThenProject) {
    VectorizedQueryPlan plan;
    plan.addFilter({VectorizedPredicate::gt("price", 20.0)})
        .addProject({"id", "price"});

    auto result = engine.execute(rows, plan);
    ASSERT_TRUE(result.has_value());
    // price > 20 → 3 rows
    ASSERT_EQ(3u, result->size());
    for (const auto& row : *result) {
        EXPECT_TRUE(row.contains("id"));
        EXPECT_TRUE(row.contains("price"));
        EXPECT_FALSE(row.contains("category"));
    }
}

TEST_F(VectorizedEngineTest, FilterThenAggregate) {
    VectorizedQueryPlan plan;
    plan.addFilter({VectorizedPredicate::eq("category", "A")})
        .addAggregate({{
            .result_field = "total",
            .input_field  = "price",
            .function     = VectorizedAggregation::Function::Sum,
        }});

    auto result = engine.execute(rows, plan);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(1u, result->size());
    // A: 10+30+50=90
    EXPECT_NEAR(90.0, (*result)[0]["total"].get<double>(), 1e-6);
}

TEST_F(VectorizedEngineTest, PlanWithLimit) {
    VectorizedQueryPlan plan;
    plan.addFilter({VectorizedPredicate::ge("price", 10.0)})
        .setLimit(2u);

    auto result = engine.execute(rows, plan);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(2u, result->size());
}

// ============================================================================
// VectorizedExecutionEngine – edge cases
// ============================================================================

TEST_F(VectorizedEngineTest, EmptyInputReturnsEmpty) {
    std::vector<json> empty;
    VectorizedQueryPlan plan;
    plan.addFilter({VectorizedPredicate::gt("price", 0.0)});

    auto result = engine.execute(empty, plan);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

TEST_F(VectorizedEngineTest, EmptyPlanReturnsAllRows) {
    VectorizedQueryPlan plan; // no stages
    auto result = engine.execute(rows, plan);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(rows.size(), result->size());
}

TEST_F(VectorizedEngineTest, FilterMatchesNone) {
    auto result = engine.filter(rows, {VectorizedPredicate::gt("price", 999.0)});
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

TEST_F(VectorizedEngineTest, FilterMatchesAll) {
    auto result = engine.filter(rows, {VectorizedPredicate::gt("price", 0.0)});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(rows.size(), result->size());
}

// ============================================================================
// VectorizedExecutionEngine – stats tests
// ============================================================================

TEST_F(VectorizedEngineTest, StatsAreUpdatedAfterExecution) {
    engine.resetStats();
    auto result = engine.filter(rows, {VectorizedPredicate::gt("price", 20.0)});
    ASSERT_TRUE(result.has_value());

    const auto& stats = engine.lastStats();
    EXPECT_GE(stats.batches_processed, 1u);
    EXPECT_EQ(stats.rows_in, rows.size());
    EXPECT_GT(stats.elapsed_ms, 0.0);
}

TEST_F(VectorizedEngineTest, ResetStatsClearsCounters) {
    engine.filter(rows, {VectorizedPredicate::gt("price", 0.0)});
    engine.resetStats();

    const auto& stats = engine.lastStats();
    EXPECT_EQ(0u, stats.batches_processed);
    EXPECT_EQ(0u, stats.rows_in);
    EXPECT_EQ(0u, stats.rows_out);
    EXPECT_NEAR(0.0, stats.elapsed_ms, 1e-9);
}

// ============================================================================
// VectorizedExecutionEngine – custom batch size
// ============================================================================

TEST(VectorizedEngineConfigTest, SmallBatchSize) {
    VectorizedExecutionEngine::Config cfg;
    cfg.batch_size = 2;  // force multiple batches over 5 rows

    VectorizedExecutionEngine engine(cfg);
    auto rows = makeProductRows();

    auto result = engine.filter(rows, {VectorizedPredicate::ge("price", 10.0)});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(rows.size(), result->size());

    // With batch_size=2 and 5 rows → 3 batches
    EXPECT_EQ(3u, engine.lastStats().batches_processed);
}

TEST(VectorizedEngineConfigTest, DefaultConfig) {
    VectorizedExecutionEngine engine;
    EXPECT_EQ(1024u, engine.config().batch_size);
    EXPECT_TRUE(engine.config().enable_simd);
}
