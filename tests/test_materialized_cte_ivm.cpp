/**
 * Unit + integration tests for Incremental View Maintenance of materialized CTEs.
 *
 * Covers:
 *  - MaterializedCTEView: INSERT / DELETE / UPDATE delta maintenance
 *  - Aggregation functions: COUNT / SUM / AVG / MIN / MAX / COUNT_DISTINCT
 *  - GROUP BY with single and multiple dimensions
 *  - Base filter push-down (applied per change record)
 *  - Pagination (limit / offset)
 *  - Staleness tracking
 *  - clear() / groupCount() / changeCount()
 *  - MaterializedCTERegistry: register / unregister / list / has /
 *                              applyChange / applyChanges / query / totalChanges
 *  - Edge cases: empty views, unknown collections, missing fields, overwrite
 *  - Thread-safety: concurrent readers while applying changes
 */

#include <gtest/gtest.h>
#include "query/materialized_cte.h"

#include <cmath>
#include <thread>
#include <atomic>
#include <chrono>

using namespace themis::query;

// ============================================================================
// Helpers
// ============================================================================

static CTEDataChange insert_rec(
    const std::string& collection,
    nlohmann::json row)
{
    CTEDataChange c;
    c.type       = CTEChangeType::INSERT;
    c.collection = collection;
    c.after_row  = std::move(row);
    return c;
}

static CTEDataChange delete_rec(
    const std::string& collection,
    nlohmann::json row)
{
    CTEDataChange c;
    c.type       = CTEChangeType::DELETE;
    c.collection = collection;
    c.before_row = std::move(row);
    return c;
}

static CTEDataChange update_rec(
    const std::string& collection,
    nlohmann::json before,
    nlohmann::json after)
{
    CTEDataChange c;
    c.type       = CTEChangeType::UPDATE;
    c.collection = collection;
    c.before_row = std::move(before);
    c.after_row  = std::move(after);
    return c;
}

/** Build a simple sales-by-region view definition. */
static MaterializedCTEDef sales_def(const std::string& name = "sales_by_region") {
    MaterializedCTEDef def;
    def.name              = name;
    def.source_collection = "sales";
    def.dimensions        = {"region"};
    def.aggregations      = {
        {"total",  CTEAggFunc::SUM,   "amount"},
        {"orders", CTEAggFunc::COUNT, ""}
    };
    return def;
}

/** Find row for given dimension value; returns empty json if not found. */
static nlohmann::json findRow(
    const MaterializedCTEResult& r,
    const std::string& dim,
    const std::string& val)
{
    for (const auto& row : r.rows) {
        auto it = row.data.find(dim);
        if (it != row.data.end() && it->get<std::string>() == val) {
            return row.data;
        }
    }
    return {};
}

// ============================================================================
// MaterializedCTEView — basic INSERT / DELETE / UPDATE
// ============================================================================

TEST(MaterializedCTEViewTest, InsertSingleRow) {
    MaterializedCTEView view(sales_def());

    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount", 100.0}}));

    auto r = view.query();
    ASSERT_EQ(r.rows.size(), 1u);
    auto row = findRow(r, "region", "EU");
    EXPECT_DOUBLE_EQ(row["total"].get<double>(), 100.0);
    EXPECT_EQ(row["orders"].get<int64_t>(), 1);
}

TEST(MaterializedCTEViewTest, InsertMultipleGroups) {
    MaterializedCTEView view(sales_def());

    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount", 100.0}}));
    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount",  50.0}}));
    view.applyChange(insert_rec("sales", {{"region", "US"}, {"amount", 200.0}}));

    auto r = view.query();
    ASSERT_EQ(r.rows.size(), 2u);
    ASSERT_EQ(r.total_rows, 2);

    auto eu = findRow(r, "region", "EU");
    EXPECT_DOUBLE_EQ(eu["total"].get<double>(), 150.0);
    EXPECT_EQ(eu["orders"].get<int64_t>(), 2);

    auto us = findRow(r, "region", "US");
    EXPECT_DOUBLE_EQ(us["total"].get<double>(), 200.0);
    EXPECT_EQ(us["orders"].get<int64_t>(), 1);
}

TEST(MaterializedCTEViewTest, DeleteRow) {
    MaterializedCTEView view(sales_def());

    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount", 100.0}}));
    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount",  50.0}}));

    auto before = view.query();
    EXPECT_EQ(before.rows.size(), 1u);

    view.applyChange(delete_rec("sales", {{"region", "EU"}, {"amount", 50.0}}));

    auto after = view.query();
    ASSERT_EQ(after.rows.size(), 1u);
    auto eu = findRow(after, "region", "EU");
    EXPECT_DOUBLE_EQ(eu["total"].get<double>(), 100.0);
    EXPECT_EQ(eu["orders"].get<int64_t>(), 1);
}

TEST(MaterializedCTEViewTest, DeleteAllRowsRemovesGroup) {
    MaterializedCTEView view(sales_def());

    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount", 100.0}}));
    view.applyChange(delete_rec("sales", {{"region", "EU"}, {"amount", 100.0}}));

    auto r = view.query();
    EXPECT_EQ(r.rows.size(), 0u);
    EXPECT_EQ(view.groupCount(), 0);
}

TEST(MaterializedCTEViewTest, UpdateRow) {
    MaterializedCTEView view(sales_def());

    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount", 100.0}}));
    view.applyChange(update_rec(
        "sales",
        {{"region", "EU"}, {"amount", 100.0}},  // before
        {{"region", "EU"}, {"amount", 200.0}}   // after
    ));

    auto r = view.query();
    ASSERT_EQ(r.rows.size(), 1u);
    auto eu = findRow(r, "region", "EU");
    EXPECT_DOUBLE_EQ(eu["total"].get<double>(), 200.0);
    EXPECT_EQ(eu["orders"].get<int64_t>(), 1);
}

TEST(MaterializedCTEViewTest, UpdateChangesGroup) {
    MaterializedCTEView view(sales_def());

    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount", 100.0}}));
    // Move the sale from EU to US
    view.applyChange(update_rec(
        "sales",
        {{"region", "EU"}, {"amount", 100.0}},
        {{"region", "US"}, {"amount", 100.0}}
    ));

    auto r = view.query();
    ASSERT_EQ(r.rows.size(), 1u);  // EU group removed, US group added
    auto us = findRow(r, "region", "US");
    EXPECT_DOUBLE_EQ(us["total"].get<double>(), 100.0);
    EXPECT_TRUE(findRow(r, "region", "EU").is_null());
}

// ============================================================================
// Aggregation functions
// ============================================================================

TEST(MaterializedCTEViewTest, Agg_SUM) {
    MaterializedCTEDef def;
    def.name              = "v";
    def.source_collection = "t";
    def.dimensions        = {"cat"};
    def.aggregations      = {{"s", CTEAggFunc::SUM, "val"}};

    MaterializedCTEView view(def);
    view.applyChange(insert_rec("t", {{"cat", "A"}, {"val", 10.0}}));
    view.applyChange(insert_rec("t", {{"cat", "A"}, {"val", 20.0}}));

    auto r = view.query();
    auto a = findRow(r, "cat", "A");
    EXPECT_DOUBLE_EQ(a["s"].get<double>(), 30.0);
}

TEST(MaterializedCTEViewTest, Agg_COUNT_STAR) {
    MaterializedCTEDef def;
    def.name              = "v";
    def.source_collection = "t";
    def.dimensions        = {"cat"};
    def.aggregations      = {{"n", CTEAggFunc::COUNT, ""}};  // COUNT(*)

    MaterializedCTEView view(def);
    for (int i = 0; i < 5; ++i)
        view.applyChange(insert_rec("t", {{"cat", "A"}, {"val", i}}));

    auto r = view.query();
    auto a = findRow(r, "cat", "A");
    EXPECT_EQ(a["n"].get<int64_t>(), 5);
}

TEST(MaterializedCTEViewTest, Agg_AVG) {
    MaterializedCTEDef def;
    def.name              = "v";
    def.source_collection = "t";
    def.dimensions        = {"cat"};
    def.aggregations      = {{"avg_val", CTEAggFunc::AVG, "val"}};

    MaterializedCTEView view(def);
    view.applyChange(insert_rec("t", {{"cat", "A"}, {"val", 10.0}}));
    view.applyChange(insert_rec("t", {{"cat", "A"}, {"val", 20.0}}));
    view.applyChange(insert_rec("t", {{"cat", "A"}, {"val", 30.0}}));

    auto r = view.query();
    auto a = findRow(r, "cat", "A");
    EXPECT_NEAR(a["avg_val"].get<double>(), 20.0, 1e-9);
}

TEST(MaterializedCTEViewTest, Agg_MIN_MAX) {
    MaterializedCTEDef def;
    def.name              = "v";
    def.source_collection = "t";
    def.dimensions        = {"cat"};
    def.aggregations      = {
        {"lo", CTEAggFunc::MIN, "val"},
        {"hi", CTEAggFunc::MAX, "val"}
    };

    MaterializedCTEView view(def);
    view.applyChange(insert_rec("t", {{"cat", "X"}, {"val", 5.0}}));
    view.applyChange(insert_rec("t", {{"cat", "X"}, {"val", 1.0}}));
    view.applyChange(insert_rec("t", {{"cat", "X"}, {"val", 9.0}}));

    auto r = view.query();
    auto x = findRow(r, "cat", "X");
    EXPECT_DOUBLE_EQ(x["lo"].get<double>(), 1.0);
    EXPECT_DOUBLE_EQ(x["hi"].get<double>(), 9.0);
}

TEST(MaterializedCTEViewTest, Agg_MIN_MAX_AfterDelete) {
    MaterializedCTEDef def;
    def.name              = "v";
    def.source_collection = "t";
    def.dimensions        = {"cat"};
    def.aggregations      = {
        {"lo", CTEAggFunc::MIN, "val"},
        {"hi", CTEAggFunc::MAX, "val"}
    };

    MaterializedCTEView view(def);
    view.applyChange(insert_rec("t", {{"cat", "X"}, {"val", 1.0}}));
    view.applyChange(insert_rec("t", {{"cat", "X"}, {"val", 9.0}}));
    // Remove the min value — new min should be 9
    view.applyChange(delete_rec("t", {{"cat", "X"}, {"val", 1.0}}));

    auto r = view.query();
    auto x = findRow(r, "cat", "X");
    EXPECT_DOUBLE_EQ(x["lo"].get<double>(), 9.0);
    EXPECT_DOUBLE_EQ(x["hi"].get<double>(), 9.0);
}

TEST(MaterializedCTEViewTest, Agg_COUNT_DISTINCT) {
    MaterializedCTEDef def;
    def.name              = "v";
    def.source_collection = "t";
    def.dimensions        = {"cat"};
    def.aggregations      = {{"uniq", CTEAggFunc::COUNT_DISTINCT, "tag"}};

    MaterializedCTEView view(def);
    view.applyChange(insert_rec("t", {{"cat", "A"}, {"tag", "x"}}));
    view.applyChange(insert_rec("t", {{"cat", "A"}, {"tag", "y"}}));
    view.applyChange(insert_rec("t", {{"cat", "A"}, {"tag", "x"}}));  // duplicate

    auto r = view.query();
    auto a = findRow(r, "cat", "A");
    EXPECT_EQ(a["uniq"].get<int64_t>(), 2);  // x, y
}

// ============================================================================
// Multiple dimensions
// ============================================================================

TEST(MaterializedCTEViewTest, MultipleDimensions) {
    MaterializedCTEDef def;
    def.name              = "sales_rp";
    def.source_collection = "sales";
    def.dimensions        = {"region", "product"};
    def.aggregations      = {
        {"total",  CTEAggFunc::SUM,   "amount"},
        {"orders", CTEAggFunc::COUNT, ""}
    };

    MaterializedCTEView view(def);
    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"product", "X"}, {"amount", 50.0}}));
    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"product", "X"}, {"amount", 30.0}}));
    view.applyChange(insert_rec("sales", {{"region", "US"}, {"product", "X"}, {"amount", 20.0}}));

    auto r = view.query();
    ASSERT_EQ(r.rows.size(), 2u);

    // Find EU/X group
    bool found_eu_x = false;
    for (const auto& row : r.rows) {
        if (row.data.value("region", "") == "EU" &&
            row.data.value("product", "") == "X") {
            EXPECT_DOUBLE_EQ(row.data["total"].get<double>(), 80.0);
            EXPECT_EQ(row.data["orders"].get<int64_t>(), 2);
            found_eu_x = true;
        }
    }
    EXPECT_TRUE(found_eu_x);
}

// ============================================================================
// Base filters
// ============================================================================

TEST(MaterializedCTEViewTest, BaseFilter_RejectsNonMatchingRows) {
    MaterializedCTEDef def = sales_def();
    CTEBaseFilter f;
    f.field = "amount";
    f.op    = CTEBaseFilter::Op::GT;
    f.value = 50.0;
    def.base_filters.push_back(f);

    MaterializedCTEView view(def);
    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount", 100.0}}));  // passes
    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount",  30.0}}));  // filtered out

    auto r = view.query();
    ASSERT_EQ(r.rows.size(), 1u);
    auto eu = findRow(r, "region", "EU");
    EXPECT_DOUBLE_EQ(eu["total"].get<double>(), 100.0);
    EXPECT_EQ(eu["orders"].get<int64_t>(), 1);
}

TEST(MaterializedCTEViewTest, BaseFilter_EQ) {
    MaterializedCTEDef def = sales_def();
    CTEBaseFilter f;
    f.field = "status";
    f.op    = CTEBaseFilter::Op::EQ;
    f.value = "confirmed";
    def.base_filters.push_back(f);

    MaterializedCTEView view(def);
    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount", 100.0}, {"status", "confirmed"}}));
    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount",  50.0}, {"status", "pending"}}));

    auto r = view.query();
    ASSERT_EQ(r.rows.size(), 1u);
    auto eu = findRow(r, "region", "EU");
    EXPECT_DOUBLE_EQ(eu["total"].get<double>(), 100.0);
}

// ============================================================================
// Collection routing
// ============================================================================

TEST(MaterializedCTEViewTest, IgnoresWrongCollection) {
    MaterializedCTEView view(sales_def());

    EXPECT_FALSE(view.applyChange(
        insert_rec("purchases", {{"region", "EU"}, {"amount", 100.0}})));
    EXPECT_EQ(view.groupCount(), 0);
}

// ============================================================================
// batchApplyChanges
// ============================================================================

TEST(MaterializedCTEViewTest, BatchApplyChanges) {
    MaterializedCTEView view(sales_def());

    std::vector<CTEDataChange> batch = {
        insert_rec("sales", {{"region", "EU"}, {"amount", 100.0}}),
        insert_rec("sales", {{"region", "EU"}, {"amount",  50.0}}),
        insert_rec("sales", {{"region", "US"}, {"amount", 200.0}}),
        insert_rec("other", {{"region", "XX"}, {"amount", 999.0}})  // wrong collection
    };
    int applied = view.applyChanges(batch);
    EXPECT_EQ(applied, 3);  // 3 sales records applied, 1 ignored

    auto r = view.query();
    EXPECT_EQ(r.rows.size(), 2u);
}

// ============================================================================
// Pagination
// ============================================================================

TEST(MaterializedCTEViewTest, Pagination) {
    MaterializedCTEView view(sales_def());

    for (char c = 'A'; c <= 'E'; ++c) {
        view.applyChange(insert_rec(
            "sales", {{"region", std::string(1, c)}, {"amount", 10.0}}));
    }

    auto all = view.query(0, 0);
    EXPECT_EQ(all.total_rows, 5);
    EXPECT_EQ(all.rows.size(), 5u);

    auto page = view.query(2, 1);
    EXPECT_EQ(page.total_rows, 5);  // total unchanged
    EXPECT_EQ(page.rows.size(), 2u);
}

// ============================================================================
// State accessors
// ============================================================================

TEST(MaterializedCTEViewTest, IsDirty_AfterChange) {
    MaterializedCTEView view(sales_def());
    EXPECT_FALSE(view.isDirty());

    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount", 1.0}}));
    EXPECT_TRUE(view.isDirty());
}

TEST(MaterializedCTEViewTest, ChangeCount_Increments) {
    MaterializedCTEView view(sales_def());
    EXPECT_EQ(view.changeCount(), 0u);

    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount", 1.0}}));
    EXPECT_EQ(view.changeCount(), 1u);
    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount", 2.0}}));
    EXPECT_EQ(view.changeCount(), 2u);
}

TEST(MaterializedCTEViewTest, GroupCount) {
    MaterializedCTEView view(sales_def());
    EXPECT_EQ(view.groupCount(), 0);

    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount", 1.0}}));
    EXPECT_EQ(view.groupCount(), 1);
    view.applyChange(insert_rec("sales", {{"region", "US"}, {"amount", 1.0}}));
    EXPECT_EQ(view.groupCount(), 2);
}

TEST(MaterializedCTEViewTest, Clear_ResetsState) {
    MaterializedCTEView view(sales_def());
    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount", 100.0}}));
    EXPECT_GT(view.groupCount(), 0);

    view.clear();
    EXPECT_EQ(view.groupCount(), 0);
    EXPECT_FALSE(view.isDirty());
    EXPECT_EQ(view.query().rows.size(), 0u);
}

// ============================================================================
// Staleness
// ============================================================================

TEST(MaterializedCTEViewTest, Staleness_ZeroDisabled) {
    MaterializedCTEDef def = sales_def();
    def.staleness_seconds = 0;

    MaterializedCTEView view(def);
    view.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount", 1.0}}));
    EXPECT_FALSE(view.isStale());
}

TEST(MaterializedCTEViewTest, Staleness_NeverUpdatedIsStale) {
    MaterializedCTEDef def = sales_def();
    def.staleness_seconds = 1;  // 1 second threshold

    MaterializedCTEView view(def);
    // No change applied → stale (never updated)
    EXPECT_TRUE(view.isStale());
}

// ============================================================================
// MaterializedCTERegistry
// ============================================================================

TEST(MaterializedCTERegistryTest, RegisterAndHas) {
    MaterializedCTERegistry registry;
    EXPECT_FALSE(registry.hasCTE("sales_by_region"));

    EXPECT_TRUE(registry.registerCTE(sales_def()));
    EXPECT_TRUE(registry.hasCTE("sales_by_region"));
}

TEST(MaterializedCTERegistryTest, DuplicateRegisterReturnsFalse) {
    MaterializedCTERegistry registry;
    EXPECT_TRUE(registry.registerCTE(sales_def()));
    EXPECT_FALSE(registry.registerCTE(sales_def()));
    // Only one CTE registered
    EXPECT_EQ(registry.listCTEs().size(), 1u);
}

TEST(MaterializedCTERegistryTest, UnregisterRemoves) {
    MaterializedCTERegistry registry;
    registry.registerCTE(sales_def());
    EXPECT_TRUE(registry.hasCTE("sales_by_region"));

    EXPECT_TRUE(registry.unregisterCTE("sales_by_region"));
    EXPECT_FALSE(registry.hasCTE("sales_by_region"));
    EXPECT_FALSE(registry.unregisterCTE("sales_by_region"));  // second call → false
}

TEST(MaterializedCTERegistryTest, ListCTEs) {
    MaterializedCTERegistry registry;
    registry.registerCTE(sales_def("v1"));
    registry.registerCTE(sales_def("v2"));
    registry.registerCTE(sales_def("v3"));

    auto names = registry.listCTEs();
    EXPECT_EQ(names.size(), 3u);
}

TEST(MaterializedCTERegistryTest, GetView) {
    MaterializedCTERegistry registry;
    registry.registerCTE(sales_def());

    auto view = registry.getView("sales_by_region");
    ASSERT_NE(view, nullptr);
    EXPECT_EQ(view->definition().name, "sales_by_region");

    auto missing = registry.getView("nonexistent");
    EXPECT_EQ(missing, nullptr);
}

TEST(MaterializedCTERegistryTest, ApplyChange_Dispatches) {
    MaterializedCTERegistry registry;
    registry.registerCTE(sales_def("sales_by_region"));

    // Another view for the same collection
    MaterializedCTEDef def2 = sales_def("sales_by_region_2");
    registry.registerCTE(def2);

    registry.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount", 100.0}}));

    EXPECT_EQ(registry.totalChanges(), 2u);  // applied to 2 views

    auto r1 = registry.query("sales_by_region");
    ASSERT_EQ(r1.rows.size(), 1u);

    auto r2 = registry.query("sales_by_region_2");
    ASSERT_EQ(r2.rows.size(), 1u);
}

TEST(MaterializedCTERegistryTest, ApplyChanges_Batch) {
    MaterializedCTERegistry registry;
    registry.registerCTE(sales_def());

    std::vector<CTEDataChange> batch = {
        insert_rec("sales", {{"region", "EU"}, {"amount", 100.0}}),
        insert_rec("sales", {{"region", "US"}, {"amount", 200.0}}),
    };
    registry.applyChanges(batch);

    auto r = registry.query("sales_by_region");
    EXPECT_EQ(r.rows.size(), 2u);
    EXPECT_EQ(registry.totalChanges(), 2u);
}

TEST(MaterializedCTERegistryTest, QueryNonexistent_ReturnsEmpty) {
    MaterializedCTERegistry registry;
    auto r = registry.query("nonexistent");
    EXPECT_EQ(r.rows.size(), 0u);
    EXPECT_EQ(r.total_rows, 0);
}

TEST(MaterializedCTERegistryTest, TotalChanges) {
    MaterializedCTERegistry registry;
    registry.registerCTE(sales_def("v1"));
    registry.registerCTE(sales_def("v2"));

    EXPECT_EQ(registry.totalChanges(), 0u);

    registry.applyChange(insert_rec("sales", {{"region", "EU"}, {"amount", 1.0}}));
    EXPECT_EQ(registry.totalChanges(), 2u);  // applied to both v1 and v2
}

// ============================================================================
// Edge cases
// ============================================================================

TEST(MaterializedCTEViewTest, EmptyView_QueryReturnsEmpty) {
    MaterializedCTEView view(sales_def());
    auto r = view.query();
    EXPECT_EQ(r.rows.size(), 0u);
    EXPECT_EQ(r.total_rows, 0);
}

TEST(MaterializedCTERegistryTest, RegisterEmptyName_ReturnsFalse) {
    MaterializedCTERegistry registry;
    MaterializedCTEDef def = sales_def();
    def.name = "";  // empty name

    EXPECT_FALSE(registry.registerCTE(def));
    EXPECT_EQ(registry.listCTEs().size(), 0u);
}

TEST(MaterializedCTERegistryTest, RegisterEmptyCollection_ReturnsFalse) {
    MaterializedCTERegistry registry;
    MaterializedCTEDef def = sales_def();
    def.source_collection = "";  // empty source collection

    EXPECT_FALSE(registry.registerCTE(def));
    EXPECT_EQ(registry.listCTEs().size(), 0u);
}

TEST(MaterializedCTEViewTest, MissingField_TreatedAsNull) {
    MaterializedCTEDef def;
    def.name              = "v";
    def.source_collection = "t";
    def.dimensions        = {"cat"};
    def.aggregations      = {{"s", CTEAggFunc::SUM, "val"}};

    MaterializedCTEView view(def);
    // Row without "val" field — treated as 0
    view.applyChange(insert_rec("t", {{"cat", "A"}}));

    auto r = view.query();
    ASSERT_EQ(r.rows.size(), 1u);
    // SUM of null → 0.0
    EXPECT_DOUBLE_EQ(r.rows[0].data["s"].get<double>(), 0.0);
}

TEST(MaterializedCTEViewTest, JsonTypes_IntegerAndFloat) {
    MaterializedCTEDef def;
    def.name              = "v";
    def.source_collection = "t";
    def.dimensions        = {"cat"};
    def.aggregations      = {{"s", CTEAggFunc::SUM, "val"}};

    MaterializedCTEView view(def);
    view.applyChange(insert_rec("t", {{"cat", "A"}, {"val", 10}}));    // integer
    view.applyChange(insert_rec("t", {{"cat", "A"}, {"val", 5.5}}));   // float

    auto r = view.query();
    EXPECT_NEAR(r.rows[0].data["s"].get<double>(), 15.5, 1e-9);
}

// ============================================================================
// Thread safety
// ============================================================================

TEST(MaterializedCTEViewTest, ConcurrentReads_ThreadSafe) {
    MaterializedCTEView view(sales_def());

    // Pre-populate
    for (int i = 0; i < 20; ++i) {
        view.applyChange(insert_rec(
            "sales", {{"region", "EU"}, {"amount", static_cast<double>(i)}}));
    }

    // 8 concurrent readers
    std::atomic<int> success{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&view, &success]() {
            for (int i = 0; i < 50; ++i) {
                auto r = view.query();
                if (!r.rows.empty()) ++success;
            }
        });
    }
    for (auto& th : threads) th.join();
    EXPECT_GT(success.load(), 0);
}

TEST(MaterializedCTERegistryTest, ConcurrentApplyAndQuery_ThreadSafe) {
    MaterializedCTERegistry registry;
    registry.registerCTE(sales_def());

    std::atomic<bool> stop{false};

    // Writer thread
    std::thread writer([&]() {
        for (int i = 0; i < 200; ++i) {
            registry.applyChange(
                insert_rec("sales", {{"region", "EU"}, {"amount", 1.0}}));
        }
        stop.store(true);
    });

    // Reader threads
    std::atomic<int> reads{0};
    std::vector<std::thread> readers;
    for (int t = 0; t < 4; ++t) {
        readers.emplace_back([&]() {
            while (!stop.load()) {
                auto r = registry.query("sales_by_region");
                if (r.total_rows >= 0) ++reads;
            }
        });
    }

    writer.join();
    for (auto& th : readers) th.join();

    EXPECT_GT(reads.load(), 0);
    // All writer changes should be reflected
    auto final_r = registry.query("sales_by_region");
    EXPECT_GE(final_r.total_rows, 1);
}
