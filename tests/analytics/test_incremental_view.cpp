/**
 * Incremental Materialized Views unit + integration tests.
 *
 * Covers:
 *  - IncrementalView: INSERT / DELETE / UPDATE delta maintenance
 *  - All aggregation functions: COUNT / SUM / AVG / MIN / MAX /
 *                               STDDEV / VARIANCE / COUNT_DISTINCT / FIRST / LAST
 *  - GROUP BY with multiple dimensions
 *  - Base filter push-down (applied per change record)
 *  - Runtime query filters (applied at query time)
 *  - Pagination (limit / offset)
 *  - Staleness tracking
 *  - clear() / groupCount()
 *  - IncrementalViewManager: create / drop / list / has / applyChange /
 *                             applyChanges / query / totalChanges
 *  - MaterializedView::incrementalRefresh() (delta, not full re-scan)
 */

#include <gtest/gtest.h>
#include "analytics/incremental_view.h"
#include "analytics/olap.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <thread>
#include <chrono>

using namespace themisdb::analytics;

// ============================================================================
// Helpers
// ============================================================================

static ChangeRecord insert_rec(const std::string& collection,
                                std::initializer_list<std::pair<std::string, FieldValue>> fields) {
    ChangeRecord r;
    r.type = ChangeType::INSERT;
    r.collection = collection;
    r.change_time = std::chrono::system_clock::now();
    for (auto& [k, v] : fields) {
      r.after_row[k] = v;
    }
    return r;
}

static ChangeRecord delete_rec(const std::string& collection,
                                std::initializer_list<std::pair<std::string, FieldValue>> fields) {
    ChangeRecord r;
    r.type = ChangeType::DELETE;
    r.collection = collection;
    r.change_time = std::chrono::system_clock::now();
    for (auto& [k, v] : fields) {
      r.before_row[k] = v;
    }
    return r;
}

static ChangeRecord update_rec(const std::string& collection,
                                std::initializer_list<std::pair<std::string, FieldValue>> before,
                                std::initializer_list<std::pair<std::string, FieldValue>> after) {
    ChangeRecord r;
    r.type = ChangeType::UPDATE;
    r.collection = collection;
    r.change_time = std::chrono::system_clock::now();
    for (auto& [k, v] : before) {
      r.before_row[k] = v;
    }
    for (auto& [k, v] : after) {
      r.after_row[k]  = v;
    }
    return r;
}

static ViewDefinition sales_view(const std::string& name = "sales_by_region") {
    ViewDefinition def;
    def.name = name;
    def.source_collection = "sales";
    def.dimensions = {"region"};
    def.aggregations = {
        {"total",  ViewAggFunc::SUM,   "amount"},
        {"orders", ViewAggFunc::COUNT, ""}
    };
    return def;
}

static double getDouble(const ViewQueryResult& r,
                         const std::string& dim_val,
                         const std::string& agg_name) {
    for (const auto& row : r.rows) {
        auto dit = row.group_key.find("region");
        if (dit != row.group_key.end() && dit->second == dim_val) {
            auto ait = row.values.find(agg_name);
            if (ait != row.values.end()) {
                if (auto* d = std::get_if<double>(&ait->second)) {
                  return *d;
                }
                if (auto* i = std::get_if<int64_t>(&ait->second)) {
                  return static_cast<double>(*i);
                }
            }
        }
    }
    return 0.0;
}

// ============================================================================
// IncrementalView – INSERT
// ============================================================================

TEST(IncrementalViewTest, InsertUpdatesGroupCount) {
    IncrementalView view(sales_view());
    EXPECT_EQ(view.groupCount(), 0);

    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 100.0}}));
    EXPECT_EQ(view.groupCount(), 1);

    view.applyChange(insert_rec("sales", {{"region", std::string("US")}, {"amount", 200.0}}));
    EXPECT_EQ(view.groupCount(), 2);
}

TEST(IncrementalViewTest, InsertSumAggregation) {
    IncrementalView view(sales_view());
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 50.0}}));
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 75.0}}));
    view.applyChange(insert_rec("sales", {{"region", std::string("US")}, {"amount", 200.0}}));

    auto r = view.query();
    EXPECT_EQ(r.total_rows, 2);
    EXPECT_DOUBLE_EQ(getDouble(r, "EU", "total"), 125.0);
    EXPECT_DOUBLE_EQ(getDouble(r, "US", "total"), 200.0);
}

TEST(IncrementalViewTest, InsertCountAggregation) {
    IncrementalView view(sales_view());
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 1.0}}));
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 2.0}}));
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 3.0}}));

    auto r = view.query();
    EXPECT_EQ(getDouble(r, "EU", "orders"), 3.0);
}

TEST(IncrementalViewTest, InsertAvgAggregation) {
    ViewDefinition def = sales_view("avg_view");
    def.aggregations = {{"mean", ViewAggFunc::AVG, "amount"}};
    IncrementalView view(def);

    for (double v : {10.0, 20.0, 30.0}) {
        view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", v}}));
    }
    auto r = view.query();
    EXPECT_DOUBLE_EQ(getDouble(r, "EU", "mean"), 20.0);
}

TEST(IncrementalViewTest, InsertMinMaxAggregation) {
    ViewDefinition def = sales_view("minmax_view");
    def.aggregations = {
        {"mn", ViewAggFunc::MIN, "amount"},
        {"mx", ViewAggFunc::MAX, "amount"}
    };
    IncrementalView view(def);

    for (double v : {5.0, 1.0, 9.0, 3.0}) {
        view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", v}}));
    }
    auto r = view.query();
    EXPECT_DOUBLE_EQ(getDouble(r, "EU", "mn"), 1.0);
    EXPECT_DOUBLE_EQ(getDouble(r, "EU", "mx"), 9.0);
}

TEST(IncrementalViewTest, InsertStddevVarianceAggregation) {
    ViewDefinition def = sales_view("stddev_view");
    def.aggregations = {
        {"std", ViewAggFunc::STDDEV,   "amount"},
        {"var", ViewAggFunc::VARIANCE, "amount"}
    };
    IncrementalView view(def);

    for (double v : {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}) {
        view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", v}}));
    }
    auto r = view.query();
    double std_val = getDouble(r, "EU", "std");
    double var_val = getDouble(r, "EU", "var");
    EXPECT_GT(std_val, 0.0);
    EXPECT_NEAR(var_val, std_val * std_val, 0.01);
}

TEST(IncrementalViewTest, InsertCountDistinctAggregation) {
    ViewDefinition def = sales_view("dc_view");
    def.aggregations = {{"dc", ViewAggFunc::COUNT_DISTINCT, "product"}};
    IncrementalView view(def);

    for (const std::string& p : {"X", "Y", "X", "Z", "Y"}) {
        view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"product", p}}));
    }
    auto r = view.query();
    // "X", "Y", "Z" → 3 distinct
    EXPECT_EQ(getDouble(r, "EU", "dc"), 3.0);
}

TEST(IncrementalViewTest, CountDistinctRefCountDeleteDuplicate) {
    // When the same distinct value appears multiple times, deleting one
    // instance should NOT decrement distinct count if others remain.
    ViewDefinition def = sales_view("dc_refcount");
    def.aggregations = {{"dc", ViewAggFunc::COUNT_DISTINCT, "product"}};
    IncrementalView view(def);

    // Insert "X" three times and "Y" once
    for (int i = 0; i < 3; ++i) {
        view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"product", std::string("X")}}));
    }
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"product", std::string("Y")}}));

    auto r = view.query();
    EXPECT_EQ(getDouble(r, "EU", "dc"), 2.0); // X and Y

    // Delete one instance of "X" — X still present twice, distinct count stays 2
    view.applyChange(delete_rec("sales", {{"region", std::string("EU")}, {"product", std::string("X")}}));
    r = view.query();
    EXPECT_EQ(getDouble(r, "EU", "dc"), 2.0);

    // Delete second instance of "X"
    view.applyChange(delete_rec("sales", {{"region", std::string("EU")}, {"product", std::string("X")}}));
    r = view.query();
    EXPECT_EQ(getDouble(r, "EU", "dc"), 2.0); // X still has one copy left

    // Delete last instance of "X"
    view.applyChange(delete_rec("sales", {{"region", std::string("EU")}, {"product", std::string("X")}}));
    r = view.query();
    EXPECT_EQ(getDouble(r, "EU", "dc"), 1.0); // only Y remains distinct
}

TEST(IncrementalViewTest, InsertFirstLastAggregation) {
    ViewDefinition def = sales_view("fl_view");
    def.aggregations = {
        {"first", ViewAggFunc::FIRST, "amount"},
        {"last",  ViewAggFunc::LAST,  "amount"}
    };
    IncrementalView view(def);
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 1.0}}));
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 2.0}}));
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 3.0}}));

    auto r = view.query();
    EXPECT_DOUBLE_EQ(getDouble(r, "EU", "first"), 1.0);
    EXPECT_DOUBLE_EQ(getDouble(r, "EU", "last"),  3.0);
}

// ============================================================================
// IncrementalView – DELETE
// ============================================================================

TEST(IncrementalViewTest, DeleteReducesSum) {
    IncrementalView view(sales_view());
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 100.0}}));
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 200.0}}));
    view.applyChange(delete_rec("sales", {{"region", std::string("EU")}, {"amount", 100.0}}));

    auto r = view.query();
    EXPECT_DOUBLE_EQ(getDouble(r, "EU", "total"), 200.0);
}

TEST(IncrementalViewTest, DeleteAllRemovesGroup) {
    IncrementalView view(sales_view());
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 50.0}}));
    EXPECT_EQ(view.groupCount(), 1);

    view.applyChange(delete_rec("sales", {{"region", std::string("EU")}, {"amount", 50.0}}));
    EXPECT_EQ(view.groupCount(), 0);
}

TEST(IncrementalViewTest, DeleteUpdateMinMax) {
    ViewDefinition def = sales_view("minmax_del");
    def.aggregations = {
        {"mn", ViewAggFunc::MIN, "amount"},
        {"mx", ViewAggFunc::MAX, "amount"}
    };
    IncrementalView view(def);
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 1.0}}));
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 5.0}}));
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 9.0}}));

    // Remove current min (1.0)
    view.applyChange(delete_rec("sales", {{"region", std::string("EU")}, {"amount", 1.0}}));
    auto r = view.query();
    EXPECT_DOUBLE_EQ(getDouble(r, "EU", "mn"), 5.0); // new min after deletion

    // Remove current max (9.0)
    view.applyChange(delete_rec("sales", {{"region", std::string("EU")}, {"amount", 9.0}}));
    r = view.query();
    EXPECT_DOUBLE_EQ(getDouble(r, "EU", "mx"), 5.0); // new max after deletion
}

// ============================================================================
// IncrementalView – UPDATE
// ============================================================================

TEST(IncrementalViewTest, UpdateAdjustsSumCorrectly) {
    IncrementalView view(sales_view());
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 100.0}}));
    // Update: amount 100 → 150
    view.applyChange(update_rec("sales",
        {{"region", std::string("EU")}, {"amount", 100.0}},
        {{"region", std::string("EU")}, {"amount", 150.0}}));

    auto r = view.query();
    EXPECT_DOUBLE_EQ(getDouble(r, "EU", "total"), 150.0);
}

TEST(IncrementalViewTest, UpdateChangingGroupKey) {
    IncrementalView view(sales_view());
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 100.0}}));
    // Move record from EU to US
    view.applyChange(update_rec("sales",
        {{"region", std::string("EU")}, {"amount", 100.0}},
        {{"region", std::string("US")}, {"amount", 100.0}}));

    auto r = view.query();
    EXPECT_DOUBLE_EQ(getDouble(r, "EU", "total"), 0.0); // EU group removed
    EXPECT_DOUBLE_EQ(getDouble(r, "US", "total"), 100.0);
    EXPECT_EQ(view.groupCount(), 1); // only US
}

// ============================================================================
// IncrementalView – Multiple dimensions
// ============================================================================

TEST(IncrementalViewTest, MultipleDimensions) {
    ViewDefinition def;
    def.name = "multi_dim";
    def.source_collection = "sales";
    def.dimensions = {"region", "product"};
    def.aggregations = {{"total", ViewAggFunc::SUM, "amount"}};

    IncrementalView view(def);
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"product", std::string("X")}, {"amount", 10.0}}));
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"product", std::string("Y")}, {"amount", 20.0}}));
    view.applyChange(insert_rec("sales", {{"region", std::string("US")}, {"product", std::string("X")}, {"amount", 30.0}}));

    EXPECT_EQ(view.groupCount(), 3);
    auto r = view.query();
    EXPECT_EQ(r.total_rows, 3);
}

// ============================================================================
// IncrementalView – Base filters
// ============================================================================

TEST(IncrementalViewTest, BaseFilterExcludesNonMatchingRows) {
    ViewDefinition def = sales_view("filtered_view");
    ViewFilter f;
    f.field = "region";
    f.op    = ViewFilter::Op::EQ;
    f.value = std::string("EU");
    def.base_filters = {f};

    IncrementalView view(def);
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 100.0}}));
    view.applyChange(insert_rec("sales", {{"region", std::string("US")}, {"amount", 200.0}})); // filtered out

    EXPECT_EQ(view.groupCount(), 1); // only EU
    auto r = view.query();
    EXPECT_EQ(r.total_rows, 1);
    EXPECT_DOUBLE_EQ(getDouble(r, "EU", "total"), 100.0);
}

TEST(IncrementalViewTest, BaseFilterGTOperator) {
    ViewDefinition def = sales_view("gt_filter");
    ViewFilter f;
    f.field = "amount";
    f.op    = ViewFilter::Op::GT;
    f.value = 50.0;
    def.base_filters = {f};

    IncrementalView view(def);
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 100.0}})); // passes
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 10.0}}));  // filtered

    auto r = view.query();
    EXPECT_DOUBLE_EQ(getDouble(r, "EU", "total"), 100.0);
}

// ============================================================================
// IncrementalView – Runtime query filters
// ============================================================================

TEST(IncrementalViewTest, RuntimeFilterByDimension) {
    IncrementalView view(sales_view());
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 100.0}}));
    view.applyChange(insert_rec("sales", {{"region", std::string("US")}, {"amount", 200.0}}));

    ViewFilter f;
    f.field = "region";
    f.op    = ViewFilter::Op::EQ;
    f.value = std::string("EU");
    auto r = view.query({f});

    EXPECT_EQ(r.rows.size(), 1u);
    EXPECT_EQ(r.rows[0].group_key.at("region"), "EU");
}

// ============================================================================
// IncrementalView – Pagination
// ============================================================================

TEST(IncrementalViewTest, PaginationLimitOffset) {
    IncrementalView view(sales_view());
    for (const std::string& r : {"A", "B", "C", "D", "E"}) {
        view.applyChange(insert_rec("sales", {{"region", r}, {"amount", 1.0}}));
    }
    EXPECT_EQ(view.groupCount(), 5);

    auto page1 = view.query({}, 2, 0); // first 2
    EXPECT_EQ(page1.rows.size(), 2u);
    EXPECT_EQ(page1.total_rows, 5);

    auto page2 = view.query({}, 2, 2); // next 2
    EXPECT_EQ(page2.rows.size(), 2u);
}

// ============================================================================
// IncrementalView – Metadata
// ============================================================================

TEST(IncrementalViewTest, ChangeCountIncrements) {
    IncrementalView view(sales_view());
    EXPECT_EQ(view.changeCount(), 0u);
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 1.0}}));
    EXPECT_EQ(view.changeCount(), 1u);
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 2.0}}));
    EXPECT_EQ(view.changeCount(), 2u);
}

TEST(IncrementalViewTest, DirtyFlagSetOnChange) {
    IncrementalView view(sales_view());
    EXPECT_FALSE(view.isDirty());
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 1.0}}));
    EXPECT_TRUE(view.isDirty());
}

TEST(IncrementalViewTest, ClearResetsState) {
    IncrementalView view(sales_view());
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 50.0}}));
    view.applyChange(insert_rec("sales", {{"region", std::string("US")}, {"amount", 75.0}}));
    EXPECT_GT(view.groupCount(), 0);

    view.clear();
    EXPECT_EQ(view.groupCount(), 0);
    EXPECT_FALSE(view.isDirty());
    EXPECT_EQ(view.changeCount(), 2u); // change_count not reset by clear
}

TEST(IncrementalViewTest, StalenessCheck) {
    ViewDefinition def = sales_view("stale_view");
    def.staleness_seconds = 1;  // 1 second staleness threshold

    IncrementalView view(def);
    // Never updated → stale
    EXPECT_TRUE(view.isStale());

    // Update
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 1.0}}));
    EXPECT_FALSE(view.isStale()); // just updated → fresh

    // Wait for staleness
    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_TRUE(view.isStale());
}

TEST(IncrementalViewTest, WrongCollectionIgnored) {
    IncrementalView view(sales_view());
    bool ok = view.applyChange(insert_rec("orders", {{"region", std::string("EU")}, {"amount", 100.0}}));
    EXPECT_FALSE(ok);
    EXPECT_EQ(view.groupCount(), 0);
}

TEST(IncrementalViewTest, BatchApplyChanges) {
    IncrementalView view(sales_view());
    std::vector<ChangeRecord> changes = {
        insert_rec("sales", {{"region", std::string("EU")}, {"amount", 10.0}}),
        insert_rec("sales", {{"region", std::string("EU")}, {"amount", 20.0}}),
        insert_rec("sales", {{"region", std::string("US")}, {"amount", 30.0}}),
        insert_rec("orders", {{"region", std::string("EU")}, {"amount", 99.0}}), // wrong collection
    };
    int applied = view.applyChanges(changes);
    EXPECT_EQ(applied, 3);
    EXPECT_EQ(view.groupCount(), 2);
}

TEST(IncrementalViewTest, BatchDeleteRemovesEmptyGroups) {
    // Verify that applyChanges correctly removes groups after all rows are deleted,
    // consistent with the single applyChange path.
    IncrementalView view(sales_view());
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 50.0}}));
    view.applyChange(insert_rec("sales", {{"region", std::string("US")}, {"amount", 75.0}}));
    EXPECT_EQ(view.groupCount(), 2);

    // Delete both rows in a single batch
    std::vector<ChangeRecord> deletes = {
        delete_rec("sales", {{"region", std::string("EU")}, {"amount", 50.0}}),
        delete_rec("sales", {{"region", std::string("US")}, {"amount", 75.0}}),
    };
    int applied = view.applyChanges(deletes);
    EXPECT_EQ(applied, 2);
    EXPECT_EQ(view.groupCount(), 0); // Groups should be removed
}

TEST(IncrementalViewTest, BatchUpdateRemovesOldGroup) {
    // Verify that applyChanges UPDATE correctly removes the old group when
    // all rows in it are moved to a different group.
    IncrementalView view(sales_view());
    view.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 100.0}}));
    EXPECT_EQ(view.groupCount(), 1);

    // Batch UPDATE: move EU → US
    std::vector<ChangeRecord> updates = {
        update_rec("sales",
            {{"region", std::string("EU")}, {"amount", 100.0}},
            {{"region", std::string("US")}, {"amount", 100.0}}),
    };
    int applied = view.applyChanges(updates);
    EXPECT_EQ(applied, 1);
    EXPECT_EQ(view.groupCount(), 1); // Old EU group removed, new US group created
    auto r = view.query();
    // EU group must be completely absent from results (not just zero)
    bool eu_found = false;
    for (const auto& row : r.rows) {
        auto it = row.group_key.find("region");
        if (it != row.group_key.end() && it->second == "EU") { eu_found = true; break; }
    }
    EXPECT_FALSE(eu_found); // EU group should not exist at all
    EXPECT_DOUBLE_EQ(getDouble(r, "US", "total"), 100.0);
}

TEST(IncrementalViewManagerTest, CreateAndListViews) {
    IncrementalViewManager mgr;
    EXPECT_TRUE(mgr.createView(sales_view("v1")));
    EXPECT_TRUE(mgr.createView(sales_view("v2")));
    EXPECT_FALSE(mgr.createView(sales_view("v1"))); // duplicate

    auto names = mgr.listViews();
    EXPECT_EQ(names.size(), 2u);
}

TEST(IncrementalViewManagerTest, HasAndGetView) {
    IncrementalViewManager mgr;
    mgr.createView(sales_view("sv"));
    EXPECT_TRUE(mgr.hasView("sv"));
    EXPECT_FALSE(mgr.hasView("nonexistent"));

    auto view = mgr.getView("sv");
    ASSERT_NE(view, nullptr);
    EXPECT_EQ(view->definition().name, "sv");

    EXPECT_EQ(mgr.getView("nonexistent"), nullptr);
}

TEST(IncrementalViewManagerTest, DropView) {
    IncrementalViewManager mgr;
    mgr.createView(sales_view("drop_me"));
    EXPECT_TRUE(mgr.dropView("drop_me"));
    EXPECT_FALSE(mgr.hasView("drop_me"));
    EXPECT_FALSE(mgr.dropView("drop_me")); // already dropped
}

TEST(IncrementalViewManagerTest, ApplyChangeDispatchedToCorrectView) {
    IncrementalViewManager mgr;

    ViewDefinition def_eu = sales_view("eu_view");
    def_eu.source_collection = "eu_sales";
    mgr.createView(def_eu);

    ViewDefinition def_us = sales_view("us_view");
    def_us.source_collection = "us_sales";
    mgr.createView(def_us);

    mgr.applyChange(insert_rec("eu_sales", {{"region", std::string("EU")}, {"amount", 100.0}}));
    mgr.applyChange(insert_rec("us_sales", {{"region", std::string("US")}, {"amount", 200.0}}));

    auto eu_res = mgr.query("eu_view");
    auto us_res = mgr.query("us_view");
    EXPECT_EQ(eu_res.total_rows, 1);
    EXPECT_EQ(us_res.total_rows, 1);
    EXPECT_DOUBLE_EQ(getDouble(eu_res, "EU", "total"), 100.0);
    EXPECT_DOUBLE_EQ(getDouble(us_res, "US", "total"), 200.0);
}

TEST(IncrementalViewManagerTest, ApplyChangeBroadcastsToAllCollectionViews) {
    IncrementalViewManager mgr;
    mgr.createView(sales_view("v1"));
    mgr.createView(sales_view("v2"));

    mgr.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 50.0}}));

    auto r1 = mgr.query("v1");
    auto r2 = mgr.query("v2");
    EXPECT_EQ(r1.total_rows, 1);
    EXPECT_EQ(r2.total_rows, 1);
}

TEST(IncrementalViewManagerTest, TotalChangesTracked) {
    IncrementalViewManager mgr;
    mgr.createView(sales_view("tc_view"));
    EXPECT_EQ(mgr.totalChanges(), 0u);

    mgr.applyChange(insert_rec("sales", {{"region", std::string("EU")}, {"amount", 1.0}}));
    mgr.applyChange(insert_rec("sales", {{"region", std::string("US")}, {"amount", 2.0}}));
    EXPECT_EQ(mgr.totalChanges(), 2u);
}

TEST(IncrementalViewManagerTest, QueryMissingViewReturnsEmpty) {
    IncrementalViewManager mgr;
    auto r = mgr.query("nonexistent");
    EXPECT_EQ(r.total_rows, 0);
    EXPECT_TRUE(r.rows.empty());
}

TEST(IncrementalViewManagerTest, BatchApplyChanges) {
    IncrementalViewManager mgr;
    mgr.createView(sales_view("batch_view"));

    std::vector<ChangeRecord> changes = {
        insert_rec("sales", {{"region", std::string("EU")}, {"amount", 10.0}}),
        insert_rec("sales", {{"region", std::string("US")}, {"amount", 20.0}}),
    };
    mgr.applyChanges(changes);

    auto r = mgr.query("batch_view");
    EXPECT_EQ(r.total_rows, 2);
}

// ============================================================================
// MaterializedView::incrementalRefresh integration test
// ============================================================================

TEST(MaterializedViewIncrementalTest, IncrementalRefreshNotFullRescan) {
    using namespace themis::analytics;

    // Set up a view with two dimensions and SUM measure
    MaterializedView::Definition def;
    def.name = "sales_mv";
    def.source_collection = "sales";
    def.dimensions = {{"region"}, {"product"}};
    def.measures = {{"total", "amount", Measure::Function::Sum}};

    MaterializedView view(def);

    // Simulate incremental changes without touching source collection
    using Row = std::unordered_map<std::string, std::variant<std::nullptr_t, bool, int64_t, double, std::string>>;
    std::vector<Row> changes = {
        Row{{"region", std::string("EU")}, {"product", std::string("A")}, {"amount", 100.0}},
        Row{{"region", std::string("US")}, {"product", std::string("B")}, {"amount", 200.0}},
        Row{{"region", std::string("EU")}, {"product", std::string("A")}, {"amount",  50.0}},
    };

    view.incrementalRefresh(changes);

    // View should now have 2 groups (EU/A and US/B)
    EXPECT_EQ(view.rowCount(), 2);
    EXPECT_FALSE(view.isStale());

    // Additional incremental update
    std::vector<Row> more_changes = {
        Row{{"region", std::string("EU")}, {"product", std::string("A")}, {"amount", 25.0}},
    };
    view.incrementalRefresh(more_changes);

    // EU/A total should now be 175 (100+50+25)
    EXPECT_EQ(view.rowCount(), 2);

    // Query result
    auto result = view.query();
    bool found_eu = false;
    for (const auto& row : result.rows) {
        auto rg = row.values.find("region");
        auto pr = row.values.find("product");
        auto tot = row.values.find("total");
        if (rg != row.values.end() && pr != row.values.end() && tot != row.values.end()) {
            if (auto* rs = std::get_if<std::string>(&rg->second)) {
                if (*rs == "EU") {
                    found_eu = true;
                    if (auto* d = std::get_if<double>(&tot->second)) {
                        EXPECT_DOUBLE_EQ(*d, 175.0);
                    }
                }
            }
        }
    }
    EXPECT_TRUE(found_eu);
}

TEST(MaterializedViewIncrementalTest, QueryWithFilter) {
    using namespace themis::analytics;

    MaterializedView::Definition def;
    def.name = "filter_mv";
    def.source_collection = "orders";
    def.dimensions = {{"region"}};
    def.measures = {{"total", "amount", Measure::Function::Sum}};

    MaterializedView view(def);

    using Row = std::unordered_map<std::string, std::variant<std::nullptr_t, bool, int64_t, double, std::string>>;
    view.incrementalRefresh({
        Row{{"region", std::string("EU")}, {"amount", 100.0}},
        Row{{"region", std::string("US")}, {"amount", 200.0}},
        Row{{"region", std::string("EU")}, {"amount",  50.0}},
    });

    EXPECT_EQ(view.rowCount(), 2); // EU and US groups

    // Filter to only EU rows
    Filter f;
    f.field = "region";
    f.op    = Filter::Operator::Eq;
    f.value = std::string("EU");

    auto result = view.query({f});
    EXPECT_EQ(result.rows.size(), 1u);
    ASSERT_FALSE(result.rows.empty());

    auto tot = result.rows[0].values.find("total");
    ASSERT_NE(tot, result.rows[0].values.end());
    if (auto* d = std::get_if<double>(&tot->second)) {
        EXPECT_DOUBLE_EQ(*d, 150.0); // 100 + 50
    } else {
        FAIL() << "Expected 'total' to be of type double";
    }
}

// ============================================================================
// Utility
// ============================================================================

TEST(IncrementalViewUtilTest, FieldValueToStr) {
    EXPECT_EQ(fieldValueToStr(FieldValue{nullptr}),          "");
    EXPECT_EQ(fieldValueToStr(FieldValue{std::string("hi")}), "hi");
    EXPECT_EQ(fieldValueToStr(FieldValue{int64_t(42)}),      "42");
    EXPECT_EQ(fieldValueToStr(FieldValue{true}),             "true");
    EXPECT_EQ(fieldValueToStr(FieldValue{false}),            "false");
    // double representation is implementation-defined; just ensure non-empty
    EXPECT_FALSE(fieldValueToStr(FieldValue{3.14}).empty());
}

TEST(IncrementalViewUtilTest, ViewAggFuncToString) {
    EXPECT_STREQ(viewAggFuncToString(ViewAggFunc::COUNT),          "COUNT");
    EXPECT_STREQ(viewAggFuncToString(ViewAggFunc::SUM),            "SUM");
    EXPECT_STREQ(viewAggFuncToString(ViewAggFunc::AVG),            "AVG");
    EXPECT_STREQ(viewAggFuncToString(ViewAggFunc::MIN),            "MIN");
    EXPECT_STREQ(viewAggFuncToString(ViewAggFunc::MAX),            "MAX");
    EXPECT_STREQ(viewAggFuncToString(ViewAggFunc::STDDEV),         "STDDEV");
    EXPECT_STREQ(viewAggFuncToString(ViewAggFunc::VARIANCE),       "VARIANCE");
    EXPECT_STREQ(viewAggFuncToString(ViewAggFunc::COUNT_DISTINCT), "COUNT_DISTINCT");
    EXPECT_STREQ(viewAggFuncToString(ViewAggFunc::FIRST),          "FIRST");
    EXPECT_STREQ(viewAggFuncToString(ViewAggFunc::LAST),           "LAST");
}

// ============================================================================
// Read-latency regression test (opt-in, set THEMIS_RUN_PERF_TESTS=1)
// ============================================================================

// AC-IVM-1: background writer calls applyChanges(10 000 rows) while a reader
// thread calls query() in a tight loop; assert reader P99 ≤ 10 ms.
TEST(IncrementalViewPerfTest, ReaderP99DuringBatchApply) {
    const char* run_perf = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!run_perf || std::string(run_perf) != "1") {
        GTEST_SKIP() << "Skipping read-latency regression test "
                        "(set THEMIS_RUN_PERF_TESTS=1 to enable).";
    }

    ViewDefinition def;
    def.name = "lat_test";
    def.source_collection = "col";
    def.dimensions = {"k"};
    def.aggregations = {{"cnt", ViewAggFunc::COUNT, ""}};
    IncrementalView view(def);

    // Seed some initial rows so query() has real groups to iterate over.
    {
        std::vector<ChangeRecord> seed = {};

        for (int i = 0; i < 500; ++i) {
            ChangeRecord r;
            r.type = ChangeType::INSERT;
            r.collection = "col";
            r.after_row["k"] = std::to_string(i % 20);
            seed.push_back(r);
        }
        view.applyChanges(seed);
    }

    // Build the 10 000-row batch that the writer will apply.
    std::vector<ChangeRecord> batch;
    batch.reserve(10000);
    for (int i = 0; i < 10000; ++i) {
        ChangeRecord r;
        r.type = ChangeType::INSERT;
        r.collection = "col";
        r.after_row["k"] = std::to_string(i % 20);
        batch.push_back(r);
    }

    std::atomic<bool> writer_done{false};
    // reader_ready is set to true once the reader has produced its first sample,
    // so the writer only starts the large batch after the reader is running.
    std::atomic<bool> reader_ready{false};
    std::vector<int64_t> latencies_us;
    latencies_us.reserve(50000);

    // Reader: call query() in a tight loop until the writer finishes.
    std::thread reader([&]() {
        while (!writer_done.load(std::memory_order_acquire)) {
            auto t0  = std::chrono::steady_clock::now();
            auto res = view.query();
            (void)res;
            auto t1 = std::chrono::steady_clock::now();
            latencies_us.push_back(
                std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
            // Signal the writer after recording the first sample.
            reader_ready.store(true, std::memory_order_release);
        }
    });

    // Wait until the reader has entered its loop and produced at least one
    // latency sample before starting the large write batch, so the writer
    // cannot finish before the reader has had a chance to observe any contention.
    while (!reader_ready.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    // Writer: apply the large batch while the reader runs concurrently.
    view.applyChanges(batch);
    writer_done.store(true, std::memory_order_release);
    reader.join();

    ASSERT_FALSE(latencies_us.empty()) << "Reader thread produced no samples";

    std::sort(latencies_us.begin(), latencies_us.end());
    const size_t  p99_idx = std::min(
        static_cast<size_t>(static_cast<double>(latencies_us.size()) * 0.99),
        latencies_us.size() - 1);
    const int64_t p99_us  = latencies_us[p99_idx];

    // P99 must be ≤ 10 ms = 10 000 µs
    EXPECT_LE(p99_us, 10000)
        << "Reader P99 latency " << p99_us
        << " µs exceeds the 10 ms IVM constraint";
}
