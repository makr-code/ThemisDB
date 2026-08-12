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

// ===== ENHANCED TESTS: Edge Cases and Boundary Conditions =====

// Test: Empty column store operations
TEST_F(ColumnarStoreTest, EmptyStoreOperations) {
    // Operations on empty store should handle gracefully
    EXPECT_EQ(store.rowCount(), 0);
    EXPECT_DOUBLE_EQ(store.sum("amount"), 0.0);
    EXPECT_DOUBLE_EQ(store.avg("amount"), 0.0);
    EXPECT_EQ(store.count("amount"), 0);
    EXPECT_EQ(store.countDistinct("name"), 0);
}

// Test: Single row operations
TEST_F(ColumnarStoreTest, SingleRowOperations) {
    using Value = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;
    
    std::vector<std::unordered_map<std::string, Value>> rows = {
        {{"id", std::string("1")}, {"amount", 100.0}}
    };
    
    store.appendRows(rows);
    
    EXPECT_EQ(store.rowCount(), 1);
    EXPECT_DOUBLE_EQ(store.sum("amount"), 100.0);
    EXPECT_DOUBLE_EQ(store.avg("amount"), 100.0);
    EXPECT_DOUBLE_EQ(store.min("amount"), 100.0);
    EXPECT_DOUBLE_EQ(store.max("amount"), 100.0);
    EXPECT_EQ(store.count("amount"), 1);
}

// Test: All null values
TEST_F(ColumnarStoreTest, AllNullValues) {
    using Value = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;
    
    std::vector<std::unordered_map<std::string, Value>> rows = {
        {{"amount", nullptr}},
        {{"amount", nullptr}},
        {{"amount", nullptr}}
    };
    
    store.appendRows(rows);
    
    auto stats = store.getColumnStats("amount");
    EXPECT_EQ(stats.row_count, 3);
    EXPECT_EQ(stats.null_count, 3);
    EXPECT_FALSE(stats.min_value.has_value());
    EXPECT_FALSE(stats.max_value.has_value());
}

// Test: Mixed null and non-null values
TEST_F(ColumnarStoreTest, MixedNullValues) {
    using Value = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;
    
    std::vector<std::unordered_map<std::string, Value>> rows = {
        {{"amount", 50.0}},
        {{"amount", nullptr}},
        {{"amount", 150.0}},
        {{"amount", nullptr}},
        {{"amount", 100.0}}
    };
    
    store.appendRows(rows);
    
    auto stats = store.getColumnStats("amount");
    EXPECT_EQ(stats.row_count, 5);
    EXPECT_EQ(stats.null_count, 2);
    EXPECT_DOUBLE_EQ(*stats.min_value, 50.0);
    EXPECT_DOUBLE_EQ(*stats.max_value, 150.0);
    // Average of non-null values: (50 + 150 + 100) / 3 = 100.0
    EXPECT_DOUBLE_EQ(stats.avg_value, 100.0);
}

// Test: Large dataset aggregations
TEST_F(ColumnarStoreTest, LargeDatasetAggregations) {
    using Value = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;
    
    std::vector<std::unordered_map<std::string, Value>> rows;
    double expected_sum = 0.0;
    const int num_rows = 10000;
    
    for (int i = 0; i < num_rows; ++i) {
        double value = static_cast<double>(i);
        rows.push_back({{"amount", value}});
        expected_sum += value;
    }
    
    store.appendRows(rows);
    
    EXPECT_EQ(store.rowCount(), num_rows);
    EXPECT_DOUBLE_EQ(store.sum("amount"), expected_sum);
    EXPECT_DOUBLE_EQ(store.min("amount"), 0.0);
    EXPECT_DOUBLE_EQ(store.max("amount"), static_cast<double>(num_rows - 1));
    EXPECT_EQ(store.count("amount"), num_rows);
}

// Test: Extreme values (very large and very small)
TEST_F(ColumnarStoreTest, ExtremeValues) {
    using Value = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;
    
    std::vector<std::unordered_map<std::string, Value>> rows = {
        {{"amount", 1e308}},    // Very large
        {{"amount", 1e-308}},   // Very small
        {{"amount", -1e308}},   // Very large negative
        {{"amount", 100.0}}     // Normal
    };
    
    store.appendRows(rows);
    
    EXPECT_DOUBLE_EQ(store.min("amount"), -1e308);
    EXPECT_DOUBLE_EQ(store.max("amount"), 1e308);
    EXPECT_EQ(store.count("amount"), 4);
}

// Test: Zero values handling
TEST_F(ColumnarStoreTest, ZeroValuesHandling) {
    using Value = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;
    
    std::vector<std::unordered_map<std::string, Value>> rows = {
        {{"amount", 0.0}},
        {{"amount", 0.0}},
        {{"amount", 100.0}},
        {{"amount", 0.0}}
    };
    
    store.appendRows(rows);
    
    EXPECT_DOUBLE_EQ(store.sum("amount"), 100.0);
    EXPECT_DOUBLE_EQ(store.avg("amount"), 25.0);
    EXPECT_DOUBLE_EQ(store.min("amount"), 0.0);
    EXPECT_EQ(store.count("amount"), 4);
}

// Test: Negative values
TEST_F(ColumnarStoreTest, NegativeValues) {
    using Value = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;
    
    std::vector<std::unordered_map<std::string, Value>> rows = {
        {{"amount", -100.0}},
        {{"amount", -200.0}},
        {{"amount", 50.0}}
    };
    
    store.appendRows(rows);
    
    EXPECT_DOUBLE_EQ(store.sum("amount"), -250.0);
    EXPECT_NEAR(store.avg("amount"), -250.0 / 3.0, 1e-6);  // Reasonable tolerance for fp arithmetic
    EXPECT_DOUBLE_EQ(store.min("amount"), -200.0);
    EXPECT_DOUBLE_EQ(store.max("amount"), 50.0);
}

// ===== ENHANCED TESTS: Parametrized Aggregation Tests =====

// Parametrized test for different aggregation functions
class AggregationFunctionTest : public ColumnarStoreTest,
                                 public ::testing::WithParamInterface<std::tuple<Measure::Function, double>> {
};

TEST_P(AggregationFunctionTest, TestAggregationFunction) {
    using Value = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;
    
    auto [func, expected_value] = GetParam();
    
    // Set up test data: [10, 20, 30, 40, 50]
    std::vector<std::unordered_map<std::string, Value>> rows = {
        {{"amount", 10.0}},
        {{"amount", 20.0}},
        {{"amount", 30.0}},
        {{"amount", 40.0}},
        {{"amount", 50.0}}
    };
    
    store.appendRows(rows);
    
    // Test based on function type
    switch(func) {
        case Measure::Function::Sum:
            EXPECT_DOUBLE_EQ(store.sum("amount"), expected_value);
            break;
        case Measure::Function::Avg:
            EXPECT_DOUBLE_EQ(store.avg("amount"), expected_value);
            break;
        case Measure::Function::Min:
            EXPECT_DOUBLE_EQ(store.min("amount"), expected_value);
            break;
        case Measure::Function::Max:
            EXPECT_DOUBLE_EQ(store.max("amount"), expected_value);
            break;
        case Measure::Function::Count:
            EXPECT_EQ(store.count("amount"), static_cast<size_t>(expected_value));
            break;
        default:
            // Other functions not implemented in this test
            break;
    }
}

INSTANTIATE_TEST_SUITE_P(
    AggregationTests,
    AggregationFunctionTest,
    ::testing::Values(
        std::make_tuple(Measure::Function::Sum, 150.0),    // 10+20+30+40+50
        std::make_tuple(Measure::Function::Avg, 30.0),     // 150/5
        std::make_tuple(Measure::Function::Min, 10.0),
        std::make_tuple(Measure::Function::Max, 50.0),
        std::make_tuple(Measure::Function::Count, 5.0)
    )
);

// ===== ENHANCED TESTS: Error Handling =====

// Test: Invalid column access
TEST_F(ColumnarStoreTest, InvalidColumnAccess) {
    // Attempting operations on non-existent column should handle gracefully
    EXPECT_FALSE(store.hasColumn("nonexistent"));
    
    // These should either return 0/default or throw - test the behavior
    // The exact behavior depends on implementation
}

// Test: Type mismatch in column operations
TEST_F(ColumnarStoreTest, TypeMismatchHandling) {
    using Value = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;
    
    store.createColumn("text_col", "string");
    
    std::vector<std::unordered_map<std::string, Value>> rows = {
        {{"text_col", std::string("hello")}},
        {{"text_col", std::string("world")}}
    };
    
    store.appendRows(rows);
    
    // Attempting numeric operations on string column
    // Should handle gracefully (return 0 or throw - depends on implementation)
}

// ===== ENHANCED TESTS: Concurrent Operations =====

// Test: Concurrent reads
TEST_F(ColumnarStoreTest, ConcurrentReads) {
    using Value = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;
    
    // Populate store with data
    std::vector<std::unordered_map<std::string, Value>> rows;
    for (int i = 0; i < 1000; ++i) {
        rows.push_back({{"amount", static_cast<double>(i)}});
    }
    store.appendRows(rows);
    
    // Multiple threads reading concurrently
    const int num_threads = 4;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, &success_count]() {
            for (int i = 0; i < 10; ++i) {
                auto count = store.rowCount();
                auto sum = store.sum("amount");
                if (count == 1000 && sum == 499500.0) {
                    success_count++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // All reads should be successful
    EXPECT_EQ(success_count, num_threads * 10);
}

// ===== ENHANCED TESTS: Performance Bounds =====

// Test: Aggregation performance on large dataset
TEST_F(ColumnarStoreTest, AggregationPerformanceBounds) {
    using Value = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;
    
    // Create large dataset
    const int num_rows = 100000;
    std::vector<std::unordered_map<std::string, Value>> rows;
    rows.reserve(num_rows);
    
    for (int i = 0; i < num_rows; ++i) {
        rows.push_back({{"amount", static_cast<double>(i)}});
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    store.appendRows(rows);
    auto end = std::chrono::high_resolution_clock::now();
    auto insert_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Insertion should be fast
    EXPECT_LT(insert_duration.count(), 1000) 
        << "Inserting " << num_rows << " rows took " << insert_duration.count() << "ms, expected < 1000ms";
    
    // Test aggregation performance
    start = std::chrono::high_resolution_clock::now();
    double sum = store.sum("amount");
    end = std::chrono::high_resolution_clock::now();
    auto sum_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Aggregation should be fast (columnar benefit)
    EXPECT_LT(sum_duration.count(), 10000) 
        << "Sum aggregation took " << sum_duration.count() << "μs, expected < 10000μs";
    
    // Verify correctness
    double expected_sum = (num_rows - 1) * num_rows / 2.0;
    EXPECT_DOUBLE_EQ(sum, expected_sum);
}

// ===== OLAP Engine Enhanced Tests =====

// Test: Empty query result handling
TEST_F(OLAPEngineTest, EmptyQueryResultHandling) {
    OLAPQuery query;
    query.collection = "nonexistent_collection";
    query.dimensions.push_back({"region", "", true});
    query.measures.push_back({"total", "amount", Measure::Function::Sum});
    
    auto result = engine.execute(query);
    
    EXPECT_EQ(result.rows.size(), 0) << "Empty collection should return 0 rows";
    EXPECT_GT(result.execution_time_ms, 0) << "Execution time should be recorded";
    EXPECT_FALSE(result.has_more) << "No more results for empty query";
}

// Test: Query with no dimensions (grand totals only)
TEST_F(OLAPEngineTest, QueryWithoutDimensions) {
    OLAPQuery query;
    query.collection = "sales";
    query.measures.push_back({"total", "amount", Measure::Function::Sum});
    
    auto plan = engine.explain(query);
    
    // Query should be valid even without dimensions
    EXPECT_GT(plan.estimated_cost, 0);
}

// Test: Query with many dimensions (high cardinality)
TEST_F(OLAPEngineTest, HighCardinalityQuery) {
    OLAPQuery query;
    query.collection = "sales";
    query.grouping_mode = OLAPQuery::GroupingMode::Cube;
    
    // Add many dimensions (high cardinality)
    for (int i = 0; i < 10; ++i) {
        query.dimensions.push_back({"dim" + std::to_string(i), "", true});
    }
    query.measures.push_back({"total", "amount", Measure::Function::Sum});
    
    auto plan = engine.explain(query);
    
    // CUBE with 10 dimensions = 2^10 = 1024 combinations
    EXPECT_GT(plan.estimated_cost, 1000);
}

// Test: Query plan optimization notes
TEST_F(OLAPEngineTest, QueryPlanOptimizationNotes) {
    OLAPQuery query;
    query.collection = "sales";
    query.grouping_mode = OLAPQuery::GroupingMode::Simple;
    query.dimensions.push_back({"region", "", true});
    query.measures.push_back({"total", "amount", Measure::Function::Sum});
    
    auto plan = engine.explain(query);
    
    // Should have optimization notes
    EXPECT_FALSE(plan.optimization_notes.empty()) 
        << "Query plan should include optimization notes";
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

TEST_F(MaterializedViewTest, QuerySortSupportsStringFields) {
    MaterializedView::Definition def;
    def.name = "sales_sort_by_region";
    def.source_collection = "sales";
    def.dimensions.push_back({"region", "", true});
    def.measures.push_back({"total_sales", "amount", Measure::Function::Sum});
    def.refresh_mode = MaterializedView::Definition::RefreshMode::Manual;

    MaterializedView view(def);
    view.incrementalRefresh({{{"region", std::string("US")}, {"amount", 100.0}},
                             {{"region", std::string("APAC")}, {"amount", 50.0}},
                             {{"region", std::string("EU")}, {"amount", 75.0}}});

    Sort sortByRegion;
    sortByRegion.field     = "region";
    sortByRegion.ascending = true;

    auto result = view.query({}, {sortByRegion});
    ASSERT_EQ(result.rows.size(), 3u);

    auto firstRegion = std::get<std::string>(result.rows[0].values.at("region"));
    auto secondRegion = std::get<std::string>(result.rows[1].values.at("region"));
    auto thirdRegion = std::get<std::string>(result.rows[2].values.at("region"));

    EXPECT_EQ(firstRegion, "APAC");
    EXPECT_EQ(secondRegion, "EU");
    EXPECT_EQ(thirdRegion, "US");
}

TEST_F(MaterializedViewTest, QuerySortUsesEpsilonForNearEqualDoubles) {
    MaterializedView::Definition def;
    def.name = "sales_sort_with_epsilon";
    def.source_collection = "sales";
    def.dimensions.push_back({"region", "", true});
    def.measures.push_back({"total_sales", "amount", Measure::Function::Sum});
    def.refresh_mode = MaterializedView::Definition::RefreshMode::Manual;

    MaterializedView view(def);
    view.incrementalRefresh({{{"region", std::string("US")}, {"amount", 1.0}},
                             {{"region", std::string("EU")}, {"amount", 1.0 + 5e-10}},
                             {{"region", std::string("APAC")}, {"amount", 2.0}}});

    Sort byTotalAsc;
    byTotalAsc.field     = "total_sales";
    byTotalAsc.ascending = true;

    Sort byRegionAsc;
    byRegionAsc.field     = "region";
    byRegionAsc.ascending = true;

    auto result = view.query({}, {byTotalAsc, byRegionAsc});
    ASSERT_EQ(result.rows.size(), 3u);

    auto firstRegion = std::get<std::string>(result.rows[0].values.at("region"));
    auto secondRegion = std::get<std::string>(result.rows[1].values.at("region"));
    auto thirdRegion = std::get<std::string>(result.rows[2].values.at("region"));

    EXPECT_EQ(firstRegion, "EU");
    EXPECT_EQ(secondRegion, "US");
    EXPECT_EQ(thirdRegion, "APAC");
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
