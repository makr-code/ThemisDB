#include <gtest/gtest.h>
#include "analytics/olap.h"
#include <cmath>

using namespace themis::analytics;

class OLAPEngineTest : public ::testing::Test {
protected:
    OLAPEngine engine;
};

// ===== Basic Query Execution Tests =====

TEST_F(OLAPEngineTest, ExecuteEmptyQuery) {
    OLAPQuery query;
    query.collection = "nonexistent";
    query.dimensions.push_back({"category", "", true});
    query.measures.push_back({"total", "amount", Measure::Function::Sum});
    
    auto result = engine.execute(query);
    
    EXPECT_EQ(result.rows.size(), 0);
    EXPECT_GT(result.execution_time_ms, 0);
}

TEST_F(OLAPEngineTest, MeasureFunctionNames) {
    EXPECT_EQ(Measure::functionName(Measure::Function::Count), "COUNT");
    EXPECT_EQ(Measure::functionName(Measure::Function::Sum), "SUM");
    EXPECT_EQ(Measure::functionName(Measure::Function::Avg), "AVG");
    EXPECT_EQ(Measure::functionName(Measure::Function::Min), "MIN");
    EXPECT_EQ(Measure::functionName(Measure::Function::Max), "MAX");
    EXPECT_EQ(Measure::functionName(Measure::Function::StdDev), "STDDEV");
    EXPECT_EQ(Measure::functionName(Measure::Function::Variance), "VARIANCE");
    EXPECT_EQ(Measure::functionName(Measure::Function::Median), "MEDIAN");
    EXPECT_EQ(Measure::functionName(Measure::Function::Percentile), "PERCENTILE");
    EXPECT_EQ(Measure::functionName(Measure::Function::CountDistinct), "COUNT_DISTINCT");
    EXPECT_EQ(Measure::functionName(Measure::Function::First), "FIRST");
    EXPECT_EQ(Measure::functionName(Measure::Function::Last), "LAST");
}

// ===== Query Plan Tests =====

TEST_F(OLAPEngineTest, ExplainSimpleQuery) {
    OLAPQuery query;
    query.collection = "sales";
    query.dimensions.push_back({"region", "", true});
    query.measures.push_back({"total", "amount", Measure::Function::Sum});
    
    auto plan = engine.explain(query);
    
    EXPECT_GT(plan.estimated_cost, 0);
    EXPECT_FALSE(plan.optimization_notes.empty());
}

TEST_F(OLAPEngineTest, ExplainCubeQuery) {
    OLAPQuery query;
    query.collection = "sales";
    query.grouping_mode = OLAPQuery::GroupingMode::Cube;
    query.dimensions.push_back({"region", "", true});
    query.dimensions.push_back({"product", "", true});
    query.dimensions.push_back({"year", "", true});
    query.measures.push_back({"total", "amount", Measure::Function::Sum});
    
    auto plan = engine.explain(query);
    
    // CUBE with 3 dimensions should note 8 combinations
    bool foundCubeNote = false;
    for (const auto& note : plan.optimization_notes) {
        if (note.find("8 grouping combinations") != std::string::npos) {
            foundCubeNote = true;
            break;
        }
    }
    EXPECT_TRUE(foundCubeNote);
}

// ===== Columnar Store Tests =====

class ColumnarStoreTest : public ::testing::Test {
protected:
    ColumnarStore store;
    
    void SetUp() override {
        store.createColumn("id", "string");
        store.createColumn("name", "string");
        store.createColumn("amount", "double");
        store.createColumn("count", "int64");
    }
};

TEST_F(ColumnarStoreTest, CreateAndCheckColumn) {
    EXPECT_TRUE(store.hasColumn("id"));
    EXPECT_TRUE(store.hasColumn("name"));
    EXPECT_TRUE(store.hasColumn("amount"));
    EXPECT_TRUE(store.hasColumn("count"));
    EXPECT_FALSE(store.hasColumn("nonexistent"));
}

TEST_F(ColumnarStoreTest, DropColumn) {
    store.dropColumn("name");
    EXPECT_FALSE(store.hasColumn("name"));
    EXPECT_TRUE(store.hasColumn("id"));
}

TEST_F(ColumnarStoreTest, AppendRows) {
    using Value = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;
    
    std::vector<std::unordered_map<std::string, Value>> rows = {
        {{"id", std::string("1")}, {"name", std::string("Alice")}, {"amount", 100.0}, {"count", int64_t(5)}},
        {{"id", std::string("2")}, {"name", std::string("Bob")}, {"amount", 200.0}, {"count", int64_t(10)}},
        {{"id", std::string("3")}, {"name", std::string("Charlie")}, {"amount", 150.0}, {"count", int64_t(7)}}
    };
    
    store.appendRows(rows);
    
    EXPECT_EQ(store.rowCount(), 3);
}

TEST_F(ColumnarStoreTest, AggregationFunctions) {
    using Value = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;
    
    std::vector<std::unordered_map<std::string, Value>> rows = {
        {{"amount", 100.0}},
        {{"amount", 200.0}},
        {{"amount", 300.0}}
    };
    
    store.appendRows(rows);
    
    EXPECT_DOUBLE_EQ(store.sum("amount"), 600.0);
    EXPECT_DOUBLE_EQ(store.avg("amount"), 200.0);
    EXPECT_DOUBLE_EQ(store.min("amount"), 100.0);
    EXPECT_DOUBLE_EQ(store.max("amount"), 300.0);
    EXPECT_EQ(store.count("amount"), 3);
}

TEST_F(ColumnarStoreTest, SumWhere) {
    using Value = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;
    
    std::vector<std::unordered_map<std::string, Value>> rows = {
        {{"amount", 100.0}},
        {{"amount", 200.0}},
        {{"amount", 300.0}},
        {{"amount", 400.0}}
    };
    
    store.appendRows(rows);
    
    std::vector<bool> mask = {true, false, true, false};  // Include rows 0 and 2
    
    EXPECT_DOUBLE_EQ(store.sumWhere("amount", mask), 400.0);  // 100 + 300
}

TEST_F(ColumnarStoreTest, ColumnStats) {
    using Value = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;
    
    std::vector<std::unordered_map<std::string, Value>> rows = {
        {{"amount", 100.0}},
        {{"amount", 200.0}},
        {{"amount", nullptr}},  // NULL value
        {{"amount", 300.0}}
    };
    
    store.appendRows(rows);
    
    auto stats = store.getColumnStats("amount");
    
    EXPECT_EQ(stats.name, "amount");
    EXPECT_EQ(stats.row_count, 4);
    EXPECT_EQ(stats.null_count, 1);
    EXPECT_EQ(stats.distinct_count, 3);  // 100, 200, 300
    EXPECT_DOUBLE_EQ(*stats.min_value, 100.0);
    EXPECT_DOUBLE_EQ(*stats.max_value, 300.0);
    EXPECT_DOUBLE_EQ(stats.avg_value, 200.0);  // (100+200+300)/3
}

TEST_F(ColumnarStoreTest, CountDistinct) {
    using Value = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;
    
    std::vector<std::unordered_map<std::string, Value>> rows = {
        {{"name", std::string("Alice")}},
        {{"name", std::string("Bob")}},
        {{"name", std::string("Alice")}},  // Duplicate
        {{"name", std::string("Charlie")}},
        {{"name", std::string("Bob")}}     // Duplicate
    };
    
    store.appendRows(rows);
    
    EXPECT_EQ(store.countDistinct("name"), 3);  // Alice, Bob, Charlie
}

TEST_F(ColumnarStoreTest, ClearStore) {
    using Value = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;
    
    std::vector<std::unordered_map<std::string, Value>> rows = {
        {{"amount", 100.0}},
        {{"amount", 200.0}}
    };
    
    store.appendRows(rows);
    EXPECT_EQ(store.rowCount(), 2);
    
    store.clear();
    EXPECT_EQ(store.rowCount(), 0);
}

// ===== Materialized View Tests =====

class MaterializedViewTest : public ::testing::Test {};

TEST_F(MaterializedViewTest, CreateView) {
    MaterializedView::Definition def;
    def.name = "sales_by_region";
    def.source_collection = "sales";
    def.dimensions.push_back({"region", "", true});
    def.measures.push_back({"total_sales", "amount", Measure::Function::Sum});
    def.refresh_mode = MaterializedView::Definition::RefreshMode::Manual;
    
    MaterializedView view(def);
    
    EXPECT_EQ(view.definition().name, "sales_by_region");
    EXPECT_EQ(view.definition().source_collection, "sales");
    EXPECT_EQ(view.definition().dimensions.size(), 1);
    EXPECT_EQ(view.definition().measures.size(), 1);
}

TEST_F(MaterializedViewTest, ViewStaleCheck) {
    MaterializedView::Definition def;
    def.name = "test_view";
    def.source_collection = "test";
    def.refresh_mode = MaterializedView::Definition::RefreshMode::Periodic;
    def.refresh_interval_seconds = 1;  // 1 second for testing
    
    MaterializedView view(def);
    
    // View is stale before first refresh
    EXPECT_TRUE(view.isStale());
    
    view.refresh();
    
    // View should not be stale immediately after refresh
    EXPECT_FALSE(view.isStale());
}

TEST_F(MaterializedViewTest, ManualRefreshNotStale) {
    MaterializedView::Definition def;
    def.name = "manual_view";
    def.source_collection = "test";
    def.refresh_mode = MaterializedView::Definition::RefreshMode::Manual;
    
    MaterializedView view(def);
    view.refresh();
    
    // Manual views are never considered "stale"
    EXPECT_FALSE(view.isStale());
}

// ===== OLAP Query Structure Tests =====

TEST(OLAPQueryTest, CreateSimpleQuery) {
    OLAPQuery query;
    query.collection = "sales";
    query.grouping_mode = OLAPQuery::GroupingMode::Simple;
    
    query.dimensions.push_back({"region", "", true});
    query.dimensions.push_back({"product", "", true});
    
    query.measures.push_back({"total_amount", "amount", Measure::Function::Sum});
    query.measures.push_back({"avg_amount", "amount", Measure::Function::Avg});
    query.measures.push_back({"order_count", "id", Measure::Function::Count});
    
    query.limit = 100;
    query.offset = 0;
    
    EXPECT_EQ(query.dimensions.size(), 2);
    EXPECT_EQ(query.measures.size(), 3);
    EXPECT_EQ(*query.limit, 100);
}

TEST(OLAPQueryTest, CreateCubeQuery) {
    OLAPQuery query;
    query.collection = "sales";
    query.grouping_mode = OLAPQuery::GroupingMode::Cube;
    
    query.dimensions.push_back({"year", "", true});
    query.dimensions.push_back({"quarter", "", true});
    query.dimensions.push_back({"region", "", true});
    
    EXPECT_EQ(query.grouping_mode, OLAPQuery::GroupingMode::Cube);
}

TEST(OLAPQueryTest, CreateRollupQuery) {
    OLAPQuery query;
    query.collection = "sales";
    query.grouping_mode = OLAPQuery::GroupingMode::Rollup;
    
    // Rollup on time hierarchy: year > quarter > month
    query.dimensions.push_back({"year", "", true});
    query.dimensions.push_back({"quarter", "", true});
    query.dimensions.push_back({"month", "", true});
    
    EXPECT_EQ(query.grouping_mode, OLAPQuery::GroupingMode::Rollup);
}

TEST(OLAPQueryTest, CreateGroupingSetsQuery) {
    OLAPQuery query;
    query.collection = "sales";
    query.grouping_mode = OLAPQuery::GroupingMode::GroupingSets;
    
    query.dimensions.push_back({"region", "", true});
    query.dimensions.push_back({"product", "", true});
    query.dimensions.push_back({"year", "", true});
    
    // Custom grouping sets
    query.grouping_sets.push_back({{"region", "product"}});
    query.grouping_sets.push_back({{"region", "year"}});
    query.grouping_sets.push_back({{"product"}});
    
    EXPECT_EQ(query.grouping_sets.size(), 3);
}

TEST(OLAPQueryTest, CreateQueryWithFilters) {
    OLAPQuery query;
    query.collection = "sales";
    
    Filter filter1;
    filter1.field = "year";
    filter1.op = Filter::Operator::Eq;
    filter1.value = int64_t(2024);
    query.filters.push_back(filter1);
    
    Filter filter2;
    filter2.field = "amount";
    filter2.op = Filter::Operator::Gt;
    filter2.value = 100.0;
    query.filters.push_back(filter2);
    
    Filter filter3;
    filter3.field = "region";
    filter3.op = Filter::Operator::In;
    filter3.value = std::vector<std::string>{"North", "South", "East"};
    query.filters.push_back(filter3);
    
    EXPECT_EQ(query.filters.size(), 3);
}

TEST(OLAPQueryTest, CreateQueryWithSorts) {
    OLAPQuery query;
    query.collection = "sales";
    
    query.sorts.push_back({"total_amount", false, false});  // DESC
    query.sorts.push_back({"region", true, false});          // ASC
    
    EXPECT_EQ(query.sorts.size(), 2);
    EXPECT_FALSE(query.sorts[0].ascending);
    EXPECT_TRUE(query.sorts[1].ascending);
}

TEST(OLAPQueryTest, CreateQueryWithWindow) {
    OLAPQuery query;
    query.collection = "sales";
    
    OLAPQuery::WindowSpec window;
    window.name = "rolling_avg";
    window.partition_by = {"region"};
    window.order_by.push_back({"date", true, false});
    window.rows_preceding = 2;
    window.rows_following = 0;
    
    query.windows.push_back(window);
    
    EXPECT_EQ(query.windows.size(), 1);
    EXPECT_EQ(query.windows[0].name, "rolling_avg");
    EXPECT_EQ(*query.windows[0].rows_preceding, 2);
}

// ===== Filter Tests =====

TEST(FilterTest, AllOperators) {
    EXPECT_EQ(static_cast<int>(Filter::Operator::Eq), 0);
    EXPECT_EQ(static_cast<int>(Filter::Operator::Ne), 1);
    EXPECT_EQ(static_cast<int>(Filter::Operator::Lt), 2);
    EXPECT_EQ(static_cast<int>(Filter::Operator::Le), 3);
    EXPECT_EQ(static_cast<int>(Filter::Operator::Gt), 4);
    EXPECT_EQ(static_cast<int>(Filter::Operator::Ge), 5);
    EXPECT_EQ(static_cast<int>(Filter::Operator::In), 6);
    EXPECT_EQ(static_cast<int>(Filter::Operator::NotIn), 7);
    EXPECT_EQ(static_cast<int>(Filter::Operator::Contains), 8);
    EXPECT_EQ(static_cast<int>(Filter::Operator::StartsWith), 9);
    EXPECT_EQ(static_cast<int>(Filter::Operator::EndsWith), 10);
    EXPECT_EQ(static_cast<int>(Filter::Operator::IsNull), 11);
    EXPECT_EQ(static_cast<int>(Filter::Operator::IsNotNull), 12);
    EXPECT_EQ(static_cast<int>(Filter::Operator::Between), 13);
}

TEST(FilterTest, BetweenFilter) {
    Filter filter;
    filter.field = "amount";
    filter.op = Filter::Operator::Between;
    filter.value = 100.0;
    filter.value2 = 500.0;
    
    EXPECT_EQ(filter.op, Filter::Operator::Between);
    EXPECT_TRUE(filter.value2.has_value());
}

// ===== Dimension and Measure Tests =====

TEST(DimensionTest, CreateDimension) {
    Dimension dim;
    dim.name = "region";
    dim.expression = "";
    dim.include_in_grouping = true;
    
    EXPECT_EQ(dim.name, "region");
    EXPECT_TRUE(dim.include_in_grouping);
}

TEST(DimensionTest, ComputedDimension) {
    Dimension dim;
    dim.name = "quarter";
    dim.expression = "EXTRACT(QUARTER FROM date)";
    dim.include_in_grouping = true;
    
    EXPECT_FALSE(dim.expression.empty());
}

TEST(MeasureTest, CreateMeasure) {
    Measure m;
    m.name = "total_sales";
    m.field = "amount";
    m.function = Measure::Function::Sum;
    
    EXPECT_EQ(m.name, "total_sales");
    EXPECT_EQ(m.field, "amount");
    EXPECT_EQ(m.function, Measure::Function::Sum);
}

TEST(MeasureTest, PercentileMeasure) {
    Measure m;
    m.name = "p95_latency";
    m.field = "latency";
    m.function = Measure::Function::Percentile;
    m.percentile_value = 95.0;
    
    EXPECT_EQ(m.function, Measure::Function::Percentile);
    EXPECT_DOUBLE_EQ(m.percentile_value, 95.0);
}

// ===== Result Structure Tests =====

TEST(OLAPResultTest, ResultStructure) {
    OLAPResult result;
    result.columns = {"region", "total_amount", "count"};
    result.total_rows = 100;
    result.execution_time_ms = 15.5;
    result.has_more = true;
    
    OLAPResult::Row row;
    row.values["region"] = std::string("North");
    row.values["total_amount"] = 50000.0;
    row.values["count"] = int64_t(150);
    row.grouping_id = 0;
    
    result.rows.push_back(row);
    
    EXPECT_EQ(result.columns.size(), 3);
    EXPECT_EQ(result.rows.size(), 1);
    EXPECT_TRUE(result.has_more);
}

TEST(OLAPResultTest, GrandTotals) {
    OLAPResult result;
    result.grand_totals["total_amount"] = 1000000.0;
    result.grand_totals["order_count"] = 5000.0;
    
    EXPECT_EQ(result.grand_totals.size(), 2);
    EXPECT_DOUBLE_EQ(result.grand_totals["total_amount"], 1000000.0);
}

// ===== Cube and Rollup Result Tests =====

TEST(CubeCellTest, CubeCellStructure) {
    CubeCell cell;
    cell.dimensions["region"] = "North";
    cell.dimensions["product"] = std::nullopt;  // Aggregated
    cell.measures["total"] = 50000.0;
    cell.grouping_id = 2;  // Bitmask: product is aggregated
    
    EXPECT_TRUE(cell.dimensions["region"].has_value());
    EXPECT_FALSE(cell.dimensions["product"].has_value());
    EXPECT_EQ(cell.grouping_id, 2);
}

TEST(RollupRowTest, RollupRowStructure) {
    RollupRow row;
    row.dimension_values = {"2024", "Q1", std::nullopt};  // month is rolled up
    row.measures["total"] = 25000.0;
    row.level = 1;  // First level of rollup
    
    EXPECT_EQ(row.dimension_values.size(), 3);
    EXPECT_TRUE(row.dimension_values[0].has_value());
    EXPECT_TRUE(row.dimension_values[1].has_value());
    EXPECT_FALSE(row.dimension_values[2].has_value());
    EXPECT_EQ(row.level, 1);
}
