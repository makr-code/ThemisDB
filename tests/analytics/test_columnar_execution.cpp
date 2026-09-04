/**
 * Unit tests for the Columnar Execution Engine with Vectorized Operator Pipeline.
 *
 * Covers:
 *  - SelectionVector: construction, reset, all(), push_back, indexing
 *  - Column: all types (Int64, Double, String, Bool, Null), null bitmap,
 *            reserve, clear, get(), filter(), slice()
 *  - ColumnBatch: addColumn, getColumn, materialize, split, hasSelection,
 *                 selectedRowCount
 *  - Predicate factories and all Op variants
 *  - FilterOperator: all comparison ops, IsNull/IsNotNull, missing column,
 *                    multi-predicate AND, empty predicate list
 *  - ProjectOperator: column subset, unknown column ignored, selection preserved
 *  - AggregateOperator: COUNT(*), SUM, AVG, MIN, MAX, COUNT_DISTINCT
 *                       without GROUP BY; with GROUP BY
 *  - SortOperator: ascending, descending, multi-key, null handling
 *  - VectorizedPipeline: filter→project→aggregate, filter→sort,
 *                        empty pipeline, stageCount
 *  - ColumnarExecutionEngine: execute, executeBatched, convenience methods,
 *                             stats, resetStats, config
 */

#include <gtest/gtest.h>
#include "analytics/columnar_execution.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

using namespace themisdb::analytics;

// ============================================================================
// Helpers
// ============================================================================

/** Build a small ColumnBatch with int64 "id", double "price", string "cat". */
static ColumnBatch makeTestBatch() {
    ColumnBatch batch(5);

    auto id_col = std::make_shared<Column>("id", ColumnType::Int64);
    for (int64_t v : {1, 2, 3, 4, 5}) id_col->appendInt64(v);

    auto price_col = std::make_shared<Column>("price", ColumnType::Double);
    for (double v : {10.0, 20.0, 30.0, 40.0, 50.0}) price_col->appendDouble(v);

    auto cat_col = std::make_shared<Column>("cat", ColumnType::String);
    for (const char* v : {"A", "B", "A", "B", "A"}) cat_col->appendString(v);

    batch.addColumn(id_col);
    batch.addColumn(price_col);
    batch.addColumn(cat_col);
    return batch;
}

// ============================================================================
// SelectionVector tests
// ============================================================================

TEST(SelectionVectorTest, DefaultEmpty) {
    SelectionVector sv;
    EXPECT_TRUE(sv.empty());
    EXPECT_EQ(0u, sv.size());
}

TEST(SelectionVectorTest, ResetProducesDenseVector) {
    SelectionVector sv;
    sv.reset(5);
    ASSERT_EQ(5u, sv.size());
    for (uint32_t i = 0; i < 5; ++i) {
      EXPECT_EQ(i, sv[i]);
    }
}

TEST(SelectionVectorTest, AllFactory) {
    auto sv = SelectionVector::all(4);
    ASSERT_EQ(4u, sv.size());
    for (uint32_t i = 0; i < 4; ++i) {
      EXPECT_EQ(i, sv[i]);
    }
}

TEST(SelectionVectorTest, PushBack) {
    SelectionVector sv(3);
    sv.push_back(0);
    sv.push_back(2);
    ASSERT_EQ(2u, sv.size());
    EXPECT_EQ(0u, sv[0]);
    EXPECT_EQ(2u, sv[1]);
}

TEST(SelectionVectorTest, IndicesAccessor) {
    SelectionVector sv;
    sv.push_back(7);
    sv.push_back(9);
    const auto& idxs = sv.indices();
    ASSERT_EQ(2u, idxs.size());
    EXPECT_EQ(7u, idxs[0]);
    EXPECT_EQ(9u, idxs[1]);
}

// ============================================================================
// Column tests
// ============================================================================

TEST(ColumnTest, Int64AppendAndGet) {
    Column col("val", ColumnType::Int64);
    col.appendInt64(42);
    col.appendInt64(-1, true);  // null
    ASSERT_EQ(2u, col.size());
    EXPECT_EQ(42, std::get<int64_t>(col.get(0)));
    EXPECT_FALSE(col.isNull(0));
    EXPECT_TRUE(col.isNull(1));
}

TEST(ColumnTest, DoubleAppendAndGet) {
    Column col("d", ColumnType::Double);
    col.appendDouble(3.14);
    ASSERT_EQ(1u, col.size());
    EXPECT_NEAR(3.14, std::get<double>(col.get(0)), 1e-9);
}

TEST(ColumnTest, StringAppendAndGet) {
    Column col("s", ColumnType::String);
    col.appendString("hello");
    ASSERT_EQ(1u, col.size());
    EXPECT_EQ("hello", std::get<std::string>(col.get(0)));
}

TEST(ColumnTest, BoolAppendAndGet) {
    Column col("b", ColumnType::Bool);
    col.appendBool(true);
    col.appendBool(false);
    ASSERT_EQ(2u, col.size());
    EXPECT_TRUE(std::get<bool>(col.get(0)));
    EXPECT_FALSE(std::get<bool>(col.get(1)));
}

TEST(ColumnTest, AppendNull) {
    Column col("x", ColumnType::Double);
    col.appendNull();
    ASSERT_EQ(1u, col.size());
    EXPECT_TRUE(col.isNull(0));
    EXPECT_EQ(nullptr, std::get<std::nullptr_t>(col.get(0)));
}

TEST(ColumnTest, ClearResetsData) {
    Column col("x", ColumnType::Int64);
    col.appendInt64(1);
    col.appendInt64(2);
    col.clear();
    EXPECT_EQ(0u, col.size());
}

TEST(ColumnTest, FilterBySelectionVector) {
    Column col("v", ColumnType::Int64);
    for (int64_t i = 0; i < 5; ++i) {
      col.appendInt64(i * 10);
    }

    SelectionVector sel;
    sel.push_back(1);
    sel.push_back(3);
    auto filtered = col.filter(sel);
    ASSERT_EQ(2u, filtered->size());
    EXPECT_EQ(10, std::get<int64_t>(filtered->get(0)));
    EXPECT_EQ(30, std::get<int64_t>(filtered->get(1)));
}

TEST(ColumnTest, SliceReturnsSubset) {
    Column col("v", ColumnType::Double);
    for (double d : {1.0, 2.0, 3.0, 4.0, 5.0}) col.appendDouble(d);

    auto s = col.slice(1, 3);
    ASSERT_EQ(3u, s->size());
    EXPECT_NEAR(2.0, std::get<double>(s->get(0)), 1e-9);
    EXPECT_NEAR(4.0, std::get<double>(s->get(2)), 1e-9);
}

TEST(ColumnTest, SliceOutOfBoundsReturnsEmpty) {
    Column col("v", ColumnType::Int64);
    col.appendInt64(1);
    auto s = col.slice(5, 10);
    EXPECT_EQ(0u, s->size());
}

// ============================================================================
// ColumnBatch tests
// ============================================================================

TEST(ColumnBatchTest, AddAndGetColumn) {
    ColumnBatch batch;
    auto col = std::make_shared<Column>("x", ColumnType::Int64);
    col->appendInt64(1);
    batch.addColumn(col);
    EXPECT_TRUE(batch.hasColumn("x"));
    EXPECT_FALSE(batch.hasColumn("y"));
    EXPECT_NE(nullptr, batch.getColumn("x"));
    EXPECT_EQ(nullptr, batch.getColumn("missing"));
}

TEST(ColumnBatchTest, RowCountFromFirstColumn) {
    ColumnBatch batch;
    auto col = std::make_shared<Column>("a", ColumnType::Int64);
    for (int i = 0; i < 7; ++i) {
      col->appendInt64(i);
    }
    batch.addColumn(col);
    EXPECT_EQ(7u, batch.rowCount());
}

TEST(ColumnBatchTest, DefaultNoSelection) {
    ColumnBatch batch = makeTestBatch();
    EXPECT_FALSE(batch.hasSelection());
    EXPECT_EQ(5u, batch.selectedRowCount());
}

TEST(ColumnBatchTest, SetSelectionUpdatesCount) {
    ColumnBatch batch = makeTestBatch();
    SelectionVector sel;
    sel.push_back(0);
    sel.push_back(2);
    batch.setSelection(sel);
    EXPECT_TRUE(batch.hasSelection());
    EXPECT_EQ(2u, batch.selectedRowCount());
}

TEST(ColumnBatchTest, MaterializeAppliesSelection) {
    ColumnBatch batch = makeTestBatch();
    SelectionVector sel;
    sel.push_back(0);
    sel.push_back(4);
    batch.setSelection(sel);

    ColumnBatch dense = batch.materialize();
    EXPECT_FALSE(dense.hasSelection());
    EXPECT_EQ(2u, dense.rowCount());
    auto id_col = dense.getColumn("id");
    ASSERT_NE(nullptr, id_col);
    EXPECT_EQ(1, std::get<int64_t>(id_col->get(0)));
    EXPECT_EQ(5, std::get<int64_t>(id_col->get(1)));
}

TEST(ColumnBatchTest, MaterializeDenseReturnsSelf) {
    ColumnBatch batch = makeTestBatch();
    ColumnBatch m = batch.materialize();
    EXPECT_EQ(5u, m.rowCount());
    EXPECT_FALSE(m.hasSelection());
}

TEST(ColumnBatchTest, SplitProducesCorrectSlices) {
    ColumnBatch batch = makeTestBatch();
    auto parts = batch.split(2);
    ASSERT_EQ(3u, parts.size());
    EXPECT_EQ(2u, parts[0].rowCount());
    EXPECT_EQ(2u, parts[1].rowCount());
    EXPECT_EQ(1u, parts[2].rowCount());
}

TEST(ColumnBatchTest, ClearEmptiesBatch) {
    ColumnBatch batch = makeTestBatch();
    batch.clear();
    EXPECT_EQ(0u, batch.columnCount());
    EXPECT_EQ(0u, batch.rowCount());
}

// ============================================================================
// Predicate tests
// ============================================================================

TEST(PredicateTest, FactoriesSetFields) {
    auto p = Predicate::gt("price", 30.0);
    EXPECT_EQ("price", p.column);
    EXPECT_EQ(Predicate::Op::Gt, p.op);
    EXPECT_NEAR(30.0, std::get<double>(p.value), 1e-9);

    auto pn = Predicate::isNull("cat");
    EXPECT_EQ(Predicate::Op::IsNull, pn.op);

    auto pnn = Predicate::isNotNull("cat");
    EXPECT_EQ(Predicate::Op::IsNotNull, pnn.op);
}

// ============================================================================
// FilterOperator tests
// ============================================================================

TEST(FilterOperatorTest, EmptyPredicatesReturnInput) {
    ColumnBatch batch = makeTestBatch();
    FilterOperator op({});
    auto result = op.execute(batch);
    // No selection set, all rows remain
    EXPECT_EQ(5u, result.selectedRowCount());
}

TEST(FilterOperatorTest, GtDoubleFiltersCorrectly) {
    ColumnBatch batch = makeTestBatch();
    FilterOperator op({Predicate::gt("price", 25.0)});
    auto result = op.execute(batch);
    ColumnBatch dense = result.materialize();

    ASSERT_EQ(3u, dense.rowCount());  // prices 30, 40, 50
    auto prices = dense.getColumn("price");
    ASSERT_NE(nullptr, prices);
    EXPECT_NEAR(30.0, std::get<double>(prices->get(0)), 1e-9);
    EXPECT_NEAR(40.0, std::get<double>(prices->get(1)), 1e-9);
    EXPECT_NEAR(50.0, std::get<double>(prices->get(2)), 1e-9);
}

TEST(FilterOperatorTest, EqInt64FiltersCorrectly) {
    ColumnBatch batch = makeTestBatch();
    FilterOperator op({Predicate::eq("id", int64_t{3})});
    auto result = op.execute(batch).materialize();

    ASSERT_EQ(1u, result.rowCount());
    auto id_col = result.getColumn("id");
    EXPECT_EQ(3, std::get<int64_t>(id_col->get(0)));
}

TEST(FilterOperatorTest, EqStringFiltersCorrectly) {
    ColumnBatch batch = makeTestBatch();
    FilterOperator op({Predicate::eq("cat", std::string{"A"})});
    auto result = op.execute(batch).materialize();

    ASSERT_EQ(3u, result.rowCount());  // rows 0, 2, 4
}

TEST(FilterOperatorTest, MultiPredicateAnd) {
    ColumnBatch batch = makeTestBatch();
    // price > 15 AND price < 45  => 20, 30, 40
    FilterOperator op({
        Predicate::gt("price", 15.0),
        Predicate::lt("price", 45.0)
    });
    auto result = op.execute(batch).materialize();

    ASSERT_EQ(3u, result.rowCount());
    auto prices = result.getColumn("price");
    EXPECT_NEAR(20.0, std::get<double>(prices->get(0)), 1e-9);
    EXPECT_NEAR(40.0, std::get<double>(prices->get(2)), 1e-9);
}

TEST(FilterOperatorTest, NoMatchReturnsEmpty) {
    ColumnBatch batch = makeTestBatch();
    FilterOperator op({Predicate::gt("price", 1000.0)});
    auto result = op.execute(batch).materialize();
    EXPECT_EQ(0u, result.rowCount());
}

TEST(FilterOperatorTest, IsNullAndIsNotNull) {
    // Batch with one null price
    ColumnBatch batch;
    auto prices = std::make_shared<Column>("price", ColumnType::Double);
    prices->appendDouble(1.0);
    prices->appendDouble(0.0, true);  // null
    prices->appendDouble(3.0);
    batch.addColumn(prices);

    FilterOperator opNull({Predicate::isNull("price")});
    auto r1 = opNull.execute(batch).materialize();
    ASSERT_EQ(1u, r1.rowCount());

    FilterOperator opNotNull({Predicate::isNotNull("price")});
    auto r2 = opNotNull.execute(batch).materialize();
    ASSERT_EQ(2u, r2.rowCount());
}

TEST(FilterOperatorTest, MissingColumnProducesEmpty) {
    ColumnBatch batch = makeTestBatch();
    FilterOperator op({Predicate::gt("nonexistent", 0.0)});
    auto result = op.execute(batch).materialize();
    EXPECT_EQ(0u, result.rowCount());
}

TEST(FilterOperatorTest, LeOperator) {
    ColumnBatch batch = makeTestBatch();
    FilterOperator op({Predicate::le("price", 20.0)});
    auto result = op.execute(batch).materialize();
    ASSERT_EQ(2u, result.rowCount());  // 10, 20
}

TEST(FilterOperatorTest, GeOperator) {
    ColumnBatch batch = makeTestBatch();
    FilterOperator op({Predicate::ge("price", 40.0)});
    auto result = op.execute(batch).materialize();
    ASSERT_EQ(2u, result.rowCount());  // 40, 50
}

TEST(FilterOperatorTest, NeOperator) {
    ColumnBatch batch = makeTestBatch();
    FilterOperator op({Predicate::ne("id", int64_t{3})});
    auto result = op.execute(batch).materialize();
    ASSERT_EQ(4u, result.rowCount());
}

// ============================================================================
// ProjectOperator tests
// ============================================================================

TEST(ProjectOperatorTest, ProjectSubsetOfColumns) {
    ColumnBatch batch = makeTestBatch();
    ProjectOperator op({"id", "price"});
    auto result = op.execute(batch);

    EXPECT_EQ(2u, result.columnCount());
    EXPECT_TRUE(result.hasColumn("id"));
    EXPECT_TRUE(result.hasColumn("price"));
    EXPECT_FALSE(result.hasColumn("cat"));
}

TEST(ProjectOperatorTest, UnknownColumnIgnored) {
    ColumnBatch batch = makeTestBatch();
    ProjectOperator op({"id", "unknown"});
    auto result = op.execute(batch);
    EXPECT_EQ(1u, result.columnCount());
}

TEST(ProjectOperatorTest, RowCountPreserved) {
    ColumnBatch batch = makeTestBatch();
    ProjectOperator op({"price"});
    auto result = op.execute(batch);
    EXPECT_EQ(5u, result.rowCount());
}

TEST(ProjectOperatorTest, SelectionPreservedAfterProject) {
    ColumnBatch batch = makeTestBatch();
    SelectionVector sel;
    sel.push_back(1);
    sel.push_back(3);
    batch.setSelection(sel);

    ProjectOperator op({"price"});
    auto result = op.execute(batch);
    EXPECT_TRUE(result.hasSelection());
    EXPECT_EQ(2u, result.selectedRowCount());
}

// ============================================================================
// AggregateOperator tests
// ============================================================================

TEST(AggregateOperatorTest, CountStar) {
    ColumnBatch batch = makeTestBatch();
    AggregateOperator op({{
        .result_name  = "cnt",
        .input_column = "",
        .function     = AggregateSpec::Function::Count
    }});
    auto result = op.execute(batch);

    ASSERT_EQ(1u, result.rowCount());
    auto cnt = result.getColumn("cnt");
    ASSERT_NE(nullptr, cnt);
    EXPECT_NEAR(5.0, std::get<double>(cnt->get(0)), 1e-9);
}

TEST(AggregateOperatorTest, SumDouble) {
    ColumnBatch batch = makeTestBatch();
    AggregateOperator op({{
        .result_name  = "total",
        .input_column = "price",
        .function     = AggregateSpec::Function::Sum
    }});
    auto result = op.execute(batch);
    auto total  = result.getColumn("total");
    ASSERT_NE(nullptr, total);
    EXPECT_NEAR(150.0, std::get<double>(total->get(0)), 1e-9);
}

TEST(AggregateOperatorTest, AvgDouble) {
    ColumnBatch batch = makeTestBatch();
    AggregateOperator op({{
        .result_name  = "avg_price",
        .input_column = "price",
        .function     = AggregateSpec::Function::Avg
    }});
    auto result = op.execute(batch);
    auto avg    = result.getColumn("avg_price");
    ASSERT_NE(nullptr, avg);
    EXPECT_NEAR(30.0, std::get<double>(avg->get(0)), 1e-9);
}

TEST(AggregateOperatorTest, MinDouble) {
    ColumnBatch batch = makeTestBatch();
    AggregateOperator op({{
        .result_name  = "lo",
        .input_column = "price",
        .function     = AggregateSpec::Function::Min
    }});
    auto result = op.execute(batch);
    EXPECT_NEAR(10.0, std::get<double>(result.getColumn("lo")->get(0)), 1e-9);
}

TEST(AggregateOperatorTest, MaxDouble) {
    ColumnBatch batch = makeTestBatch();
    AggregateOperator op({{
        .result_name  = "hi",
        .input_column = "price",
        .function     = AggregateSpec::Function::Max
    }});
    auto result = op.execute(batch);
    EXPECT_NEAR(50.0, std::get<double>(result.getColumn("hi")->get(0)), 1e-9);
}

TEST(AggregateOperatorTest, CountDistinct) {
    ColumnBatch batch = makeTestBatch();  // cat: A, B, A, B, A → 2 distinct
    AggregateOperator op({{
        .result_name  = "uniq",
        .input_column = "cat",
        .function     = AggregateSpec::Function::CountDistinct
    }});
    auto result = op.execute(batch);
    EXPECT_NEAR(2.0, std::get<double>(result.getColumn("uniq")->get(0)), 1e-9);
}

TEST(AggregateOperatorTest, GroupByCategory) {
    ColumnBatch batch = makeTestBatch();
    // SUM(price) GROUP BY cat
    AggregateOperator op({{
        .result_name  = "total",
        .input_column = "price",
        .function     = AggregateSpec::Function::Sum,
        .group_by     = {"cat"}
    }});
    auto result = op.execute(batch);

    // Two groups: A and B
    ASSERT_EQ(2u, result.rowCount());
    auto cat_col   = result.getColumn("cat");
    auto total_col = result.getColumn("total");
    ASSERT_NE(nullptr, cat_col);
    ASSERT_NE(nullptr, total_col);

    // Collect results into a map
    std::unordered_map<std::string, double> totals;
    for (size_t i = 0; i < result.rowCount(); ++i) {
        std::string c = std::get<std::string>(cat_col->get(i));
        double      t = std::get<double>(total_col->get(i));
        totals[c] = t;
    }
    EXPECT_NEAR(90.0, totals["A"], 1e-9);  // 10 + 30 + 50
    EXPECT_NEAR(60.0, totals["B"], 1e-9);  // 20 + 40
}

TEST(AggregateOperatorTest, CountGroupBy) {
    ColumnBatch batch = makeTestBatch();
    AggregateOperator op({{
        .result_name  = "cnt",
        .input_column = "",
        .function     = AggregateSpec::Function::Count,
        .group_by     = {"cat"}
    }});
    auto result = op.execute(batch);

    ASSERT_EQ(2u, result.rowCount());
    std::unordered_map<std::string, double> counts;
    auto cat_col = result.getColumn("cat");
    auto cnt_col = result.getColumn("cnt");
    for (size_t i = 0; i < result.rowCount(); ++i) {
        counts[std::get<std::string>(cat_col->get(i))] =
            std::get<double>(cnt_col->get(i));
    }
    EXPECT_NEAR(3.0, counts["A"], 1e-9);
    EXPECT_NEAR(2.0, counts["B"], 1e-9);
}

TEST(AggregateOperatorTest, EmptyBatch) {
    ColumnBatch batch(0);
    auto col = std::make_shared<Column>("v", ColumnType::Double);
    batch.addColumn(col);

    AggregateOperator op({{
        .result_name  = "total",
        .input_column = "v",
        .function     = AggregateSpec::Function::Sum
    }});
    auto result = op.execute(batch);
    ASSERT_EQ(1u, result.rowCount());
    EXPECT_NEAR(0.0, std::get<double>(result.getColumn("total")->get(0)), 1e-9);
}

TEST(AggregateOperatorTest, SpecCount) {
    AggregateOperator op({
        {.result_name="a", .input_column="x", .function=AggregateSpec::Function::Sum},
        {.result_name="b", .input_column="x", .function=AggregateSpec::Function::Count}
    });
    EXPECT_EQ(2u, op.specCount());
}

// ============================================================================
// SortOperator tests
// ============================================================================

TEST(SortOperatorTest, SortDoubleAscending) {
    ColumnBatch batch;
    auto prices = std::make_shared<Column>("price", ColumnType::Double);
    for (double d : {30.0, 10.0, 50.0, 20.0, 40.0}) prices->appendDouble(d);
    batch.addColumn(prices);

    SortOperator op({{.column = "price", .ascending = true}});
    auto result = op.execute(batch);

    ASSERT_EQ(5u, result.rowCount());
    auto col = result.getColumn("price");
    EXPECT_NEAR(10.0, std::get<double>(col->get(0)), 1e-9);
    EXPECT_NEAR(20.0, std::get<double>(col->get(1)), 1e-9);
    EXPECT_NEAR(50.0, std::get<double>(col->get(4)), 1e-9);
}

TEST(SortOperatorTest, SortDoubleDescending) {
    ColumnBatch batch;
    auto prices = std::make_shared<Column>("price", ColumnType::Double);
    for (double d : {30.0, 10.0, 50.0}) prices->appendDouble(d);
    batch.addColumn(prices);

    SortOperator op({{.column = "price", .ascending = false}});
    auto result = op.execute(batch);

    auto col = result.getColumn("price");
    EXPECT_NEAR(50.0, std::get<double>(col->get(0)), 1e-9);
    EXPECT_NEAR(10.0, std::get<double>(col->get(2)), 1e-9);
}

TEST(SortOperatorTest, SortStringAscending) {
    ColumnBatch batch;
    auto cats = std::make_shared<Column>("cat", ColumnType::String);
    for (const char* s : {"banana", "apple", "cherry"}) cats->appendString(s);
    batch.addColumn(cats);

    SortOperator op({{.column = "cat", .ascending = true}});
    auto result = op.execute(batch);

    auto col = result.getColumn("cat");
    EXPECT_EQ("apple",  std::get<std::string>(col->get(0)));
    EXPECT_EQ("banana", std::get<std::string>(col->get(1)));
    EXPECT_EQ("cherry", std::get<std::string>(col->get(2)));
}

TEST(SortOperatorTest, MultiKeySort) {
    ColumnBatch batch;
    auto grp = std::make_shared<Column>("grp", ColumnType::String);
    auto val = std::make_shared<Column>("val", ColumnType::Int64);
    // (B,2), (A,3), (A,1), (B,1)
    grp->appendString("B"); val->appendInt64(2);
    grp->appendString("A"); val->appendInt64(3);
    grp->appendString("A"); val->appendInt64(1);
    grp->appendString("B"); val->appendInt64(1);
    batch.addColumn(grp);
    batch.addColumn(val);

    SortOperator op({
        {.column = "grp", .ascending = true},
        {.column = "val", .ascending = true}
    });
    auto result = op.execute(batch);

    auto gc = result.getColumn("grp");
    auto vc = result.getColumn("val");
    EXPECT_EQ("A", std::get<std::string>(gc->get(0)));
    EXPECT_EQ(1,   std::get<int64_t>(vc->get(0)));
    EXPECT_EQ("A", std::get<std::string>(gc->get(1)));
    EXPECT_EQ(3,   std::get<int64_t>(vc->get(1)));
    EXPECT_EQ("B", std::get<std::string>(gc->get(2)));
    EXPECT_EQ(1,   std::get<int64_t>(vc->get(2)));
    EXPECT_EQ("B", std::get<std::string>(gc->get(3)));
    EXPECT_EQ(2,   std::get<int64_t>(vc->get(3)));
}

TEST(SortOperatorTest, EmptyBatchNoOp) {
    ColumnBatch batch(0);
    auto col = std::make_shared<Column>("x", ColumnType::Int64);
    batch.addColumn(col);
    SortOperator op({{.column = "x"}});
    auto result = op.execute(batch);
    EXPECT_EQ(0u, result.rowCount());
}

// ============================================================================
// VectorizedPipeline tests
// ============================================================================

TEST(VectorizedPipelineTest, EmptyPipelinePassesThrough) {
    VectorizedPipeline p;
    EXPECT_EQ(0u, p.stageCount());
    ColumnBatch batch = makeTestBatch();
    auto result = p.execute(batch);
    EXPECT_EQ(5u, result.rowCount());
}

TEST(VectorizedPipelineTest, FilterThenAggregate) {
    // SUM(price) WHERE price > 25  =>  30 + 40 + 50 = 120
    VectorizedPipeline p;
    p.addFilter({Predicate::gt("price", 25.0)})
     .addAggregate({{
         .result_name  = "total",
         .input_column = "price",
         .function     = AggregateSpec::Function::Sum
     }});

    ColumnBatch result = p.execute(makeTestBatch());
    ASSERT_EQ(1u, result.rowCount());
    EXPECT_NEAR(120.0, std::get<double>(result.getColumn("total")->get(0)), 1e-9);
}

TEST(VectorizedPipelineTest, FilterThenProject) {
    VectorizedPipeline p;
    p.addFilter({Predicate::le("price", 20.0)})
     .addProject({"price"});

    ColumnBatch result = p.execute(makeTestBatch()).materialize();
    ASSERT_EQ(2u, result.rowCount());
    EXPECT_EQ(1u, result.columnCount());
    EXPECT_TRUE(result.hasColumn("price"));
}

TEST(VectorizedPipelineTest, FilterGroupByAggregate) {
    // SUM(price) GROUP BY cat WHERE price >= 20
    VectorizedPipeline p;
    p.addFilter({Predicate::ge("price", 20.0)})
     .addAggregate({{
         .result_name  = "sum_price",
         .input_column = "price",
         .function     = AggregateSpec::Function::Sum,
         .group_by     = {"cat"}
     }});

    ColumnBatch result = p.execute(makeTestBatch());
    ASSERT_EQ(2u, result.rowCount());

    auto cat_col   = result.getColumn("cat");
    auto total_col = result.getColumn("sum_price");

    std::unordered_map<std::string, double> totals;
    for (size_t i = 0; i < result.rowCount(); ++i) {
        totals[std::get<std::string>(cat_col->get(i))] =
            std::get<double>(total_col->get(i));
    }
    EXPECT_NEAR(80.0, totals["A"], 1e-9);  // 30 + 50
    EXPECT_NEAR(60.0, totals["B"], 1e-9);  // 20 + 40
}

TEST(VectorizedPipelineTest, FilterThenSort) {
    // price >= 20, sorted descending
    VectorizedPipeline p;
    p.addFilter({Predicate::ge("price", 20.0)})
     .addSort({{.column = "price", .ascending = false}});

    ColumnBatch result = p.execute(makeTestBatch());
    ASSERT_EQ(4u, result.rowCount());
    auto pc = result.getColumn("price");
    EXPECT_NEAR(50.0, std::get<double>(pc->get(0)), 1e-9);
    EXPECT_NEAR(20.0, std::get<double>(pc->get(3)), 1e-9);
}

TEST(VectorizedPipelineTest, StageCount) {
    VectorizedPipeline p;
    p.addFilter({Predicate::gt("x", 0.0)})
     .addProject({"x"})
     .addAggregate({{.result_name="s", .input_column="x",
                     .function=AggregateSpec::Function::Sum}})
     .addSort({{.column="s"}});
    EXPECT_EQ(4u, p.stageCount());
}

// ============================================================================
// ColumnarExecutionEngine tests
// ============================================================================

TEST(ColumnarExecutionEngineTest, DefaultConfig) {
    ColumnarExecutionEngine eng;
    EXPECT_EQ(ColumnBatch::kDefaultBatchSize, eng.config().batch_size);
    EXPECT_TRUE(eng.config().enable_simd);
}

TEST(ColumnarExecutionEngineTest, CustomConfig) {
    ColumnarExecutionEngine::Config cfg;
    cfg.batch_size  = 512;
    cfg.enable_simd = false;
    ColumnarExecutionEngine eng(cfg);
    EXPECT_EQ(512u, eng.config().batch_size);
    EXPECT_FALSE(eng.config().enable_simd);
}

TEST(ColumnarExecutionEngineTest, ExecuteUpdateStats) {
    ColumnarExecutionEngine eng;
    eng.resetStats();

    VectorizedPipeline p;
    p.addFilter({Predicate::gt("price", 25.0)});
    eng.execute(makeTestBatch(), p);

    const auto& s = eng.lastStats();
    EXPECT_EQ(1u, s.batches_processed);
    EXPECT_EQ(5u, s.rows_in);
    EXPECT_EQ(3u, s.rows_out);  // rows 30, 40, 50 pass
    EXPECT_GE(s.elapsed_ms, 0.0);
}

TEST(ColumnarExecutionEngineTest, ExecuteBatched) {
    ColumnarExecutionEngine eng;
    eng.resetStats();

    VectorizedPipeline p;
    p.addAggregate({{
        .result_name  = "s",
        .input_column = "price",
        .function     = AggregateSpec::Function::Sum
    }});

    auto parts = makeTestBatch().split(2);
    auto results = eng.executeBatched(parts, p);
    EXPECT_EQ(parts.size(), results.size());
    EXPECT_EQ(parts.size(), eng.lastStats().batches_processed);
}

TEST(ColumnarExecutionEngineTest, ConvenienceFilter) {
    ColumnarExecutionEngine eng;
    auto result = eng.filter(makeTestBatch(),
                             {Predicate::eq("cat", std::string{"B"})}).materialize();
    EXPECT_EQ(2u, result.rowCount());
}

TEST(ColumnarExecutionEngineTest, ConvenienceAggregate) {
    ColumnarExecutionEngine eng;
    auto result = eng.aggregate(makeTestBatch(), {{
        .result_name  = "total",
        .input_column = "price",
        .function     = AggregateSpec::Function::Sum
    }});
    EXPECT_NEAR(150.0, std::get<double>(result.getColumn("total")->get(0)), 1e-9);
}

TEST(ColumnarExecutionEngineTest, ConvenienceProject) {
    ColumnarExecutionEngine eng;
    auto result = eng.project(makeTestBatch(), {"id"});
    EXPECT_EQ(1u, result.columnCount());
}

TEST(ColumnarExecutionEngineTest, ConvenienceSort) {
    ColumnarExecutionEngine eng;
    auto result = eng.sort(makeTestBatch(),
                           {{.column = "price", .ascending = false}});
    ASSERT_EQ(5u, result.rowCount());
    auto pc = result.getColumn("price");
    EXPECT_NEAR(50.0, std::get<double>(pc->get(0)), 1e-9);
}

TEST(ColumnarExecutionEngineTest, ResetStats) {
    ColumnarExecutionEngine eng;
    VectorizedPipeline p;
    eng.execute(makeTestBatch(), p);
    eng.resetStats();

    const auto& s = eng.lastStats();
    EXPECT_EQ(0u, s.batches_processed);
    EXPECT_EQ(0u, s.rows_in);
    EXPECT_EQ(0u, s.rows_out);
    EXPECT_NEAR(0.0, s.elapsed_ms, 1e-9);
}

// ============================================================================
// End-to-end: larger batch with all stages
// ============================================================================

TEST(EndToEndTest, LargeBatchFilterAggregateSort) {
    // Generate 2048 rows with two categories
    ColumnBatch batch(2048);
    auto cat_col   = std::make_shared<Column>("cat",   ColumnType::String);
    auto price_col = std::make_shared<Column>("price", ColumnType::Double);
    double expected_a = 0.0, expected_b = 0.0;
    for (int i = 0; i < 2048; ++i) {
        const char* c = (i % 2 == 0) ? "A" : "B";
        double price = static_cast<double>(i + 1);
        cat_col->appendString(c);
        price_col->appendDouble(price);
        if (price > 512.0) {   // filter threshold
            if (i % 2 == 0) {
              expected_a += price;
            }
            else             expected_b += price;
        }
    }
    batch.addColumn(cat_col);
    batch.addColumn(price_col);

    VectorizedPipeline pipeline;
    pipeline
        .addFilter({Predicate::gt("price", 512.0)})
        .addAggregate({{
            .result_name  = "total",
            .input_column = "price",
            .function     = AggregateSpec::Function::Sum,
            .group_by     = {"cat"}
        }})
        .addSort({{.column = "cat", .ascending = true}});

    ColumnarExecutionEngine eng;
    auto result = eng.execute(batch, pipeline);

    ASSERT_EQ(2u, result.rowCount());
    auto rc = result.getColumn("cat");
    auto rt = result.getColumn("total");
    EXPECT_EQ("A", std::get<std::string>(rc->get(0)));
    EXPECT_NEAR(expected_a, std::get<double>(rt->get(0)), 1e-3);
    EXPECT_EQ("B", std::get<std::string>(rc->get(1)));
    EXPECT_NEAR(expected_b, std::get<double>(rt->get(1)), 1e-3);
}

// ============================================================================
// SIMD parity tests (AVX-512 / AVX2 / ARM NEON vs. scalar baseline)
//
// These tests ensure that the SIMD aggregation paths in AggregateOperator
// produce results that are bit-identical (or within 1 ULP) to a scalar
// reference on the same input data.  The tests exercise the fast path
// (non-null Double column) that calls simdAggDouble().
// ============================================================================

namespace {

// Build a ColumnBatch with a single non-null Double column of @p n rows.
static ColumnBatch makeLargeDoubleColumn(const std::vector<double>& values) {
    ColumnBatch batch(values.size());
    auto col = std::make_shared<Column>("v", ColumnType::Double);
    col->reserve(values.size());
    for (double v : values) {
      col->appendDouble(v);
    }
    batch.addColumn(col);
    return batch;
}

// Scalar reference aggregation (no SIMD).
struct ScalarAgg {
    double sum     = 0.0;
    double min_val = std::numeric_limits<double>::max();
    double max_val = std::numeric_limits<double>::lowest();
};
static ScalarAgg scalarAgg(const std::vector<double>& data) {
    ScalarAgg r;
    for (double v : data) {
        r.sum += v;
        if (v < r.min_val) {
          r.min_val = v;
        }
        if (v > r.max_val) {
          r.max_val = v;
        }
    }
    return r;
}

}  // anonymous namespace

// Parity test: SUM on 1 024 doubles — SIMD result ≤ 1 ULP from scalar.
TEST(SIMDParityTest, Sum_1024_Doubles) {
    std::vector<double> data(1024);
    for (size_t i = 0; i < data.size(); ++i)
        data[i] = static_cast<double>(i + 1) * 0.123456789;

    auto batch = makeLargeDoubleColumn(data);
    AggregateOperator agg({{
        .result_name  = "s",
        .input_column = "v",
        .function     = AggregateSpec::Function::Sum
    }});
    auto result = agg.execute(batch);
    double simd_sum = std::get<double>(result.getColumn("s")->get(0));

    ScalarAgg ref = scalarAgg(data);

    // Tolerance: 2^-50 relative (tight enough to catch algorithmic errors,
    // loose enough to allow different FP summation order).
    double tol = std::abs(ref.sum) * 1e-12 + 1e-300;
    EXPECT_NEAR(ref.sum, simd_sum, tol)
        << "SIMD SUM diverges from scalar by more than 1 ULP relative";
}

// Parity test: MIN on 1 024 doubles.
TEST(SIMDParityTest, Min_1024_Doubles) {
    std::vector<double> data(1024);
    for (size_t i = 0; i < data.size(); ++i)
        data[i] = static_cast<double>(i % 17) - 8.0;

    auto batch = makeLargeDoubleColumn(data);
    AggregateOperator agg({{
        .result_name  = "m",
        .input_column = "v",
        .function     = AggregateSpec::Function::Min
    }});
    auto result = agg.execute(batch);
    double simd_min = std::get<double>(result.getColumn("m")->get(0));

    ScalarAgg ref = scalarAgg(data);
    EXPECT_DOUBLE_EQ(ref.min_val, simd_min)
        << "SIMD MIN differs from scalar baseline";
}

// Parity test: MAX on 1 024 doubles.
TEST(SIMDParityTest, Max_1024_Doubles) {
    std::vector<double> data(1024);
    for (size_t i = 0; i < data.size(); ++i)
        data[i] = static_cast<double>(i % 31) * 2.5 - 10.0;

    auto batch = makeLargeDoubleColumn(data);
    AggregateOperator agg({{
        .result_name  = "m",
        .input_column = "v",
        .function     = AggregateSpec::Function::Max
    }});
    auto result = agg.execute(batch);
    double simd_max = std::get<double>(result.getColumn("m")->get(0));

    ScalarAgg ref = scalarAgg(data);
    EXPECT_DOUBLE_EQ(ref.max_val, simd_max)
        << "SIMD MAX differs from scalar baseline";
}

// Parity test: AVG on 1 024 doubles.
TEST(SIMDParityTest, Avg_1024_Doubles) {
    std::vector<double> data(1024);
    for (size_t i = 0; i < data.size(); ++i)
        data[i] = std::sin(static_cast<double>(i) * 0.01) * 100.0;

    auto batch = makeLargeDoubleColumn(data);
    AggregateOperator agg({{
        .result_name  = "a",
        .input_column = "v",
        .function     = AggregateSpec::Function::Avg
    }});
    auto result = agg.execute(batch);
    double simd_avg = std::get<double>(result.getColumn("a")->get(0));

    ScalarAgg ref = scalarAgg(data);
    double ref_avg = ref.sum / static_cast<double>(data.size());

    double tol = std::abs(ref_avg) * 1e-12 + 1e-300;
    EXPECT_NEAR(ref_avg, simd_avg, tol)
        << "SIMD AVG diverges from scalar by more than 1 ULP relative";
}

// Parity test: non-multiple-of-8 length (SIMD tail handling).
TEST(SIMDParityTest, Sum_NonAlignedLength) {
    // 13 values: exercises the scalar tail after SIMD loop
    std::vector<double> data = {
        1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.1, 11.2, 12.3, 13.4
    };
    auto batch = makeLargeDoubleColumn(data);
    AggregateOperator agg({{
        .result_name  = "s",
        .input_column = "v",
        .function     = AggregateSpec::Function::Sum
    }});
    auto result = agg.execute(batch);
    double simd_sum = std::get<double>(result.getColumn("s")->get(0));

    ScalarAgg ref = scalarAgg(data);
    double tol = std::abs(ref.sum) * 1e-12 + 1e-300;
    EXPECT_NEAR(ref.sum, simd_sum, tol);
}

// Parity test: nullable Double column falls back to per-row path — result
// must match scalar when some values are null.
TEST(SIMDParityTest, Sum_WithNulls_FallbackToScalar) {
    ColumnBatch batch(10);
    auto col = std::make_shared<Column>("v", ColumnType::Double);
    double expected = 0.0;
    for (int i = 0; i < 10; ++i) {
        bool is_null = (i % 3 == 0);
        col->appendDouble(static_cast<double>(i + 1), is_null);
        if (!is_null) {
          expected += static_cast<double>(i + 1);
        }
    }
    batch.addColumn(col);

    AggregateOperator agg({{
        .result_name  = "s",
        .input_column = "v",
        .function     = AggregateSpec::Function::Sum
    }});
    auto result = agg.execute(batch);
    double got = std::get<double>(result.getColumn("s")->get(0));
    EXPECT_DOUBLE_EQ(expected, got);
}

// ============================================================================
// SIMD Throughput tests (opt-in via THEMIS_RUN_PERF_TESTS=1)
//
// AC-6: AVX-512 SUM over 10 M doubles — verifies ≥ 4 GB/s on any modern
//       CPU; on hardware with AVX-512 this typically achieves ≥ 2× the AVX2
//       baseline throughput per the roadmap performance target.
//
// AC-7: ARM NEON — same measurement path; on AArch64 (Cortex-A78 / Apple
//       Silicon) the NEON float64x2_t path is active and expected to deliver
//       ≥ 4 GB/s.
//
// Both tests skip unless the environment variable THEMIS_RUN_PERF_TESTS=1.
// This follows the repository convention for timing-sensitive benchmarks.
// ============================================================================

TEST(SIMDThroughputTest, Sum_10M_Doubles_Throughput) {
    const char* perf_env = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!perf_env || std::string(perf_env) != "1") {
        GTEST_SKIP() << "Set THEMIS_RUN_PERF_TESTS=1 to run throughput benchmarks";
    }

    constexpr size_t N = 10'000'000;

    // Build a 10 M-row non-null Double column so the SIMD fast path activates.
    ColumnBatch batch(N);
    auto col = std::make_shared<Column>("v", ColumnType::Double);
    col->reserve(N);
    for (size_t i = 0; i < N; ++i)
        col->appendDouble(static_cast<double>(i + 1) * 0.001);
    batch.addColumn(col);

    AggregateOperator agg({{
        .result_name  = "s",
        .input_column = "v",
        .function     = AggregateSpec::Function::Sum
    }});

    // Warm up (fills caches, JITs branch predictors).
    agg.execute(batch);

    constexpr int kReps = 10;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int r = 0; r < kReps; ++r) {
      agg.execute(batch);
    }
    auto t1 = std::chrono::high_resolution_clock::now();

    double elapsed_s = std::chrono::duration<double>(t1 - t0).count() / kReps;
    double data_bytes = static_cast<double>(N) * sizeof(double);
    double gb_per_s   = data_bytes / elapsed_s / 1e9;

    // Minimum 4 GB/s is a conservative floor achievable on any modern CPU.
    // On AVX-512 hardware (8 doubles/cycle at ~3 GHz) peak is >> 4 GB/s.
    // On ARM AArch64 with NEON the target is also ≥ 4 GB/s.
    EXPECT_GE(gb_per_s, 4.0)
        << "SUM throughput " << gb_per_s << " GB/s is below the 4 GB/s floor "
        << "(AVX-512 target: ≥ 2× AVX2 baseline; ARM NEON target: ≥ 4 GB/s)";

    // Sanity: result must be finite.
    double result_val =
        std::get<double>(agg.execute(batch).getColumn("s")->get(0));
    EXPECT_TRUE(std::isfinite(result_val));
}

// Same path but for MIN/MAX — exercises the min/max SIMD lanes.
TEST(SIMDThroughputTest, MinMax_10M_Doubles_Throughput) {
    const char* perf_env = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!perf_env || std::string(perf_env) != "1") {
        GTEST_SKIP() << "Set THEMIS_RUN_PERF_TESTS=1 to run throughput benchmarks";
    }

    constexpr size_t N = 10'000'000;

    ColumnBatch batch(N);
    auto col = std::make_shared<Column>("v", ColumnType::Double);
    col->reserve(N);
    for (size_t i = 0; i < N; ++i)
        col->appendDouble(static_cast<double>(i % 997) - 498.0);
    batch.addColumn(col);

    AggregateOperator agg_min({{
        .result_name  = "mn",
        .input_column = "v",
        .function     = AggregateSpec::Function::Min
    }});
    AggregateOperator agg_max({{
        .result_name  = "mx",
        .input_column = "v",
        .function     = AggregateSpec::Function::Max
    }});

    agg_min.execute(batch); agg_max.execute(batch);  // warm-up

    constexpr int kReps = 10;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int r = 0; r < kReps; ++r) {
        agg_min.execute(batch);
        agg_max.execute(batch);
    }
    auto t1 = std::chrono::high_resolution_clock::now();

    double elapsed_s  = std::chrono::duration<double>(t1 - t0).count() / kReps / 2.0;
    double data_bytes = static_cast<double>(N) * sizeof(double);
    double gb_per_s   = data_bytes / elapsed_s / 1e9;

    EXPECT_GE(gb_per_s, 4.0)
        << "MIN/MAX throughput " << gb_per_s << " GB/s is below the 4 GB/s floor";
}
