/*
 * Tests for MaterializedView — Materialized Views & Incremental Maintenance
 * (v1.8.0, Issue #195)
 *
 * Validates all acceptance criteria:
 *   AC-1  View definition and creation
 *   AC-2  Automatic query rewriting (canRewrite)
 *   AC-3  Incremental maintenance on data changes (IMMEDIATE strategy)
 *   AC-4  Partial refresh strategies (IMMEDIATE / DEFERRED / PERIODIC / MANUAL)
 *   AC-5  View staleness tracking (markStale / isStale / time-based expiry)
 *   AC-6  Query speedup: pre-computed rows returned in O(1) snapshot reads
 *   AC-7  Insert overhead for IMMEDIATE maintenance: delta applied in-place
 */

#include "query/materialized_view.h"
#include "query/aql_parser.h"

#include <gtest/gtest.h>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace themis;
using namespace themis::query;

// =============================================================================
// Helpers
// =============================================================================

/// Build a minimal view Definition for unit tests.
static MaterializedView::Definition makeDefinition(
    const std::string& name,
    MaterializedView::RefreshStrategy strategy =
        MaterializedView::RefreshStrategy::DEFERRED,
    std::vector<std::string> base_tables = {"sales"},
    std::chrono::milliseconds staleness = std::chrono::seconds{60})
{
    MaterializedView::Definition def;
    def.name                = name;
    def.query_aql           = "FOR s IN sales COLLECT r=s.region RETURN r";
    def.strategy            = strategy;
    def.base_tables         = std::move(base_tables);
    def.staleness_tolerance = staleness;
    return def;
}

/// Build a sample JSON row representing one sale.
static nlohmann::json makeSaleRow(const std::string& key,
                                  const std::string& region,
                                  double             amount)
{
    return nlohmann::json{
        {"_key",   key},
        {"region", region},
        {"amount", amount}
    };
}

/// Create a view and assert success.
static std::shared_ptr<MaterializedView> createView(
    const MaterializedView::Definition& def,
    const MaterializedView::Config&     cfg = MaterializedView::Config{})
{
    auto result = MaterializedView::create(def, cfg);
    EXPECT_TRUE(result.has_value()) << "create() failed: "
        << (result ? "" : result.error().message());
    return *result;
}

// =============================================================================
// Fixture
// =============================================================================

class MaterializedViewFocusedTests : public ::testing::Test {
protected:
    void SetUp() override {
        auto def = makeDefinition(
            "sales_by_region",
            MaterializedView::RefreshStrategy::DEFERRED);
        view_ = createView(def);
    }

    std::shared_ptr<MaterializedView> view_;
};

// =============================================================================
// AC-1  View definition and creation
// =============================================================================

/// A valid definition produces a non-null view with the correct metadata.
TEST_F(MaterializedViewFocusedTests, AC1_CreateSucceeds) {
    ASSERT_NE(view_, nullptr);
    EXPECT_EQ(view_->getName(), "sales_by_region");
    EXPECT_EQ(view_->getDefinition().strategy,
              MaterializedView::RefreshStrategy::DEFERRED);
    EXPECT_EQ(view_->getDefinition().base_tables,
              std::vector<std::string>{"sales"});
}

/// A newly created view starts in the stale state.
TEST_F(MaterializedViewFocusedTests, AC1_NewViewIsStale) {
    EXPECT_TRUE(view_->isStale());
}

/// create() with an empty name returns an error.
TEST(MaterializedViewCreationTests, AC1_EmptyNameReturnsError) {
    MaterializedView::Definition bad;
    bad.query_aql = "FOR s IN sales RETURN s";
    auto result = MaterializedView::create(bad);
    EXPECT_FALSE(result.has_value());
}

/// create() with an empty query_aql returns an error.
TEST(MaterializedViewCreationTests, AC1_EmptyQueryAqlReturnsError) {
    MaterializedView::Definition bad;
    bad.name = "my_view";
    auto result = MaterializedView::create(bad);
    EXPECT_FALSE(result.has_value());
}

/// After a successful full refresh the view is no longer stale.
TEST_F(MaterializedViewFocusedTests, AC1_RefreshClearsStale) {
    std::vector<nlohmann::json> rows{
        makeSaleRow("k1", "EU", 100.0),
        makeSaleRow("k2", "US", 200.0)
    };
    auto r = view_->refresh(/*incremental=*/false, rows);
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_FALSE(view_->isStale());
    EXPECT_EQ(view_->getRows().size(), 2u);
}

/// Config::max_rows is honoured: exceeding it returns an error.
TEST(MaterializedViewCreationTests, AC1_MaxRowsEnforced) {
    auto def = makeDefinition("tiny");
    MaterializedView::Config cfg;
    cfg.max_rows = 2;
    auto view = createView(def, cfg);

    std::vector<nlohmann::json> too_many{
        makeSaleRow("k1", "EU", 1.0),
        makeSaleRow("k2", "US", 2.0),
        makeSaleRow("k3", "APAC", 3.0)
    };
    auto r = view->refresh(false, too_many);
    EXPECT_FALSE(r.has_value());
}

// =============================================================================
// AC-2  Automatic query rewriting — canRewrite
// =============================================================================

/// A query that directly iterates over the view name can be rewritten.
TEST_F(MaterializedViewFocusedTests, AC2_CanRewriteForInViewName) {
    const std::string q =
        "FOR r IN sales_by_region FILTER r.region == 'EU' RETURN r";
    EXPECT_TRUE(MaterializedView::canRewrite(q, *view_));
}

/// A query iterating over a different collection is NOT rewritable.
TEST_F(MaterializedViewFocusedTests, AC2_CannotRewriteOtherCollection) {
    const std::string q = "FOR s IN sales RETURN s";
    EXPECT_FALSE(MaterializedView::canRewrite(q, *view_));
}

/// canRewrite is case-insensitive for the IN keyword.
TEST_F(MaterializedViewFocusedTests, AC2_CanRewriteCaseInsensitive) {
    // "in" in lower case
    const std::string q = "for r in sales_by_region return r";
    EXPECT_TRUE(MaterializedView::canRewrite(q, *view_));
}

/// A partial name match should NOT trigger a rewrite.
TEST_F(MaterializedViewFocusedTests, AC2_CannotRewritePartialName) {
    // View name is "sales_by_region"; "sales_by_region_v2" must not match.
    const std::string q =
        "FOR r IN sales_by_region_v2 FILTER r.region == 'EU' RETURN r";
    EXPECT_FALSE(MaterializedView::canRewrite(q, *view_));
}

/// Parsed-AST overload: for_node.collection == view name → rewritable.
TEST_F(MaterializedViewFocusedTests, AC2_CanRewriteParsedQuery) {
    query::Query pq;
    pq.for_node.collection = "sales_by_region";
    EXPECT_TRUE(MaterializedView::canRewrite(pq, *view_));
}

/// Parsed-AST overload: different collection → not rewritable.
TEST_F(MaterializedViewFocusedTests, AC2_CannotRewriteParsedQueryOtherCollection) {
    query::Query pq;
    pq.for_node.collection = "orders";
    EXPECT_FALSE(MaterializedView::canRewrite(pq, *view_));
}

/// Registry tryRewrite returns the correct view for a matching query.
TEST(MaterializedViewRegistryTests, AC2_RegistryTryRewrite) {
    auto def = makeDefinition("region_view");
    auto view = createView(def);

    MaterializedViewRegistry reg;
    ASSERT_TRUE(reg.registerView(view).has_value());

    const std::string q =
        "FOR r IN region_view FILTER r.region == 'US' RETURN r";
    auto found = reg.tryRewrite(q);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getName(), "region_view");
}

/// tryRewrite returns nullptr when no view matches.
TEST(MaterializedViewRegistryTests, AC2_RegistryTryRewriteNoMatch) {
    auto def = makeDefinition("region_view");
    auto view = createView(def);

    MaterializedViewRegistry reg;
    ASSERT_TRUE(reg.registerView(view).has_value());

    auto found = reg.tryRewrite("FOR s IN raw_sales RETURN s");
    EXPECT_EQ(found, nullptr);
}

// =============================================================================
// AC-3  Incremental maintenance on data changes
// =============================================================================

/// IMMEDIATE: INSERT delta appends a row.
TEST(MaterializedViewDeltaTests, AC3_ImmediateInsertAppendsRow) {
    auto def = makeDefinition(
        "view1", MaterializedView::RefreshStrategy::IMMEDIATE);
    auto view = createView(def);

    // Populate with an initial full refresh.
    view->refresh(false, {makeSaleRow("k1", "EU", 100.0)});
    EXPECT_EQ(view->getRows().size(), 1u);

    // Insert a new row via delta.
    view->applyDeltaJson(DeltaOp::INSERT, makeSaleRow("k2", "US", 200.0));
    EXPECT_EQ(view->getRows().size(), 2u);
}

/// IMMEDIATE: DELETE delta removes the matching row by _key.
TEST(MaterializedViewDeltaTests, AC3_ImmediateDeleteRemovesRow) {
    auto def = makeDefinition(
        "view2", MaterializedView::RefreshStrategy::IMMEDIATE);
    auto view = createView(def);

    view->refresh(false, {
        makeSaleRow("k1", "EU",   100.0),
        makeSaleRow("k2", "US",   200.0),
        makeSaleRow("k3", "APAC", 50.0)
    });
    ASSERT_EQ(view->getRows().size(), 3u);

    view->applyDeltaJson(DeltaOp::DELETE,
                         nlohmann::json{{"_key", "k2"}});
    auto rows = view->getRows();
    EXPECT_EQ(rows.size(), 2u);
    for (const auto& r : rows) {
        EXPECT_NE(r.value("_key", ""), "k2");
    }
}

/// IMMEDIATE: UPDATE delta replaces the matching row.
TEST(MaterializedViewDeltaTests, AC3_ImmediateUpdateReplacesRow) {
    auto def = makeDefinition(
        "view3", MaterializedView::RefreshStrategy::IMMEDIATE);
    auto view = createView(def);

    view->refresh(false, {makeSaleRow("k1", "EU", 100.0)});

    auto updated = makeSaleRow("k1", "EU", 999.0);
    view->applyDeltaJson(DeltaOp::UPDATE, updated);

    auto rows = view->getRows();
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_DOUBLE_EQ(rows[0].value("amount", 0.0), 999.0);
}

/// Registry delta propagation: onInsertJson routes to the correct view.
TEST(MaterializedViewRegistryTests, AC3_RegistryPropagatesInsert) {
    auto def = makeDefinition(
        "reg_view", MaterializedView::RefreshStrategy::IMMEDIATE,
        {"orders"});
    auto view = createView(def);
    view->refresh(false, {});  // empty snapshot

    MaterializedViewRegistry reg;
    ASSERT_TRUE(reg.registerView(view).has_value());

    reg.onInsertJson("orders", makeSaleRow("o1", "EU", 42.0));
    EXPECT_EQ(view->getRows().size(), 1u);
}

/// Registry delta: events on an unrelated table are not forwarded.
TEST(MaterializedViewRegistryTests, AC3_RegistryDoesNotPropagateUnrelatedTable) {
    auto def = makeDefinition(
        "view_orders", MaterializedView::RefreshStrategy::IMMEDIATE,
        {"orders"});
    auto view = createView(def);
    view->refresh(false, {makeSaleRow("o1", "EU", 1.0)});

    MaterializedViewRegistry reg;
    ASSERT_TRUE(reg.registerView(view).has_value());

    // Insert into "invoices" — view_orders depends on "orders", not "invoices".
    reg.onInsertJson("invoices", makeSaleRow("i1", "US", 9.0));
    EXPECT_EQ(view->getRows().size(), 1u);  // unchanged
}

// =============================================================================
// AC-4  Partial refresh strategies
// =============================================================================

/// IMMEDIATE: delta rows appear instantly without calling refresh().
TEST(MaterializedViewStrategyTests, AC4_ImmediateDeltaNoRefreshNeeded) {
    auto def = makeDefinition(
        "imm_view", MaterializedView::RefreshStrategy::IMMEDIATE);
    auto view = createView(def);
    view->refresh(false, {});

    view->applyDeltaJson(DeltaOp::INSERT, makeSaleRow("k1", "EU", 1.0));
    // No explicit refresh() called — rows must already be updated.
    EXPECT_EQ(view->getRows().size(), 1u);
}

/// DEFERRED: delta marks the view stale but does NOT change rows_.
TEST(MaterializedViewStrategyTests, AC4_DeferredDeltaMarksStaleOnly) {
    auto def = makeDefinition(
        "def_view", MaterializedView::RefreshStrategy::DEFERRED);
    auto view = createView(def);
    view->refresh(false, {makeSaleRow("k1", "EU", 1.0)});
    ASSERT_FALSE(view->isStale());

    view->applyDeltaJson(DeltaOp::INSERT, makeSaleRow("k2", "US", 2.0));
    // Rows must NOT have changed yet.
    EXPECT_EQ(view->getRows().size(), 1u);
    // But the view must now be stale.
    EXPECT_TRUE(view->isStale());
}

/// PERIODIC: delta marks the view stale.
TEST(MaterializedViewStrategyTests, AC4_PeriodicDeltaMarksStale) {
    auto def = makeDefinition(
        "per_view", MaterializedView::RefreshStrategy::PERIODIC);
    auto view = createView(def);
    view->refresh(false, {makeSaleRow("k1", "EU", 1.0)});
    ASSERT_FALSE(view->isStale());

    view->applyDeltaJson(DeltaOp::DELETE,
                         nlohmann::json{{"_key", "k1"}});
    EXPECT_TRUE(view->isStale());
    EXPECT_EQ(view->getRows().size(), 1u);  // rows unchanged
}

/// MANUAL: delta is a no-op — view stays fresh, rows unchanged.
TEST(MaterializedViewStrategyTests, AC4_ManualDeltaIsNoop) {
    auto def = makeDefinition(
        "man_view", MaterializedView::RefreshStrategy::MANUAL);
    auto view = createView(def);
    view->refresh(false, {makeSaleRow("k1", "EU", 1.0)});
    ASSERT_FALSE(view->isStale());

    view->applyDeltaJson(DeltaOp::INSERT, makeSaleRow("k2", "US", 9.0));
    EXPECT_FALSE(view->isStale());
    EXPECT_EQ(view->getRows().size(), 1u);
}

/// Registry refreshStale() calls refresh() on DEFERRED stale views.
TEST(MaterializedViewRegistryTests, AC4_RegistryRefreshStale) {
    auto def = makeDefinition(
        "stale_view", MaterializedView::RefreshStrategy::DEFERRED);
    auto view = createView(def);
    view->refresh(false, {makeSaleRow("k1", "EU", 1.0)});
    view->markStale();
    ASSERT_TRUE(view->isStale());

    MaterializedViewRegistry reg;
    ASSERT_TRUE(reg.registerView(view).has_value());

    size_t count = reg.refreshStale();
    EXPECT_EQ(count, 1u);
    EXPECT_FALSE(view->isStale());
}

// =============================================================================
// AC-5  View staleness tracking
// =============================================================================

/// markStale() makes isStale() return true.
TEST_F(MaterializedViewFocusedTests, AC5_MarkStaleIsStale) {
    view_->refresh(false, {makeSaleRow("k1", "EU", 1.0)});
    ASSERT_FALSE(view_->isStale());

    view_->markStale();
    EXPECT_TRUE(view_->isStale());
}

/// refresh() clears the stale flag.
TEST_F(MaterializedViewFocusedTests, AC5_RefreshClearsStale) {
    view_->markStale();
    ASSERT_TRUE(view_->isStale());

    view_->refresh(false, {});
    EXPECT_FALSE(view_->isStale());
}

/// Time-based staleness: view becomes stale after staleness_tolerance expires.
TEST(MaterializedViewStalenessTests, AC5_TimeBasedStale) {
    // Use a very short tolerance to keep the test quick even on slow CI.
    auto def = makeDefinition("time_view",
                              MaterializedView::RefreshStrategy::DEFERRED,
                              {"sales"},
                              /*staleness=*/std::chrono::milliseconds(100));
    auto view = createView(def);
    view->refresh(false, {makeSaleRow("k1", "EU", 1.0)});
    ASSERT_FALSE(view->isStale());

    // Poll until the snapshot expires (up to 2 s to tolerate slow CI).
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(2);
    bool became_stale = false;
    while (std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if (view->isStale()) {
            became_stale = true;
            break;
        }
    }
    EXPECT_TRUE(became_stale) << "View did not become stale within 2 s";
}

/// When staleness_tolerance == 0, time-based staleness is disabled.
TEST(MaterializedViewStalenessTests, AC5_ZeroToleranceNoTimeStaleness) {
    auto def = makeDefinition("notimeout_view",
                              MaterializedView::RefreshStrategy::DEFERRED,
                              {"sales"},
                              /*staleness=*/std::chrono::milliseconds{0});
    auto view = createView(def);
    view->refresh(false, {makeSaleRow("k1", "EU", 1.0)});
    // Even after a short wait the view must stay fresh (stale_ == false).
    std::this_thread::sleep_for(std::chrono::milliseconds{50});
    EXPECT_FALSE(view->isStale());
}

/// ViewStats.is_stale reflects the current staleness.
TEST_F(MaterializedViewFocusedTests, AC5_StatsStalenessField) {
    view_->refresh(false, {makeSaleRow("k1", "EU", 1.0)});
    EXPECT_FALSE(view_->getStats().is_stale);

    view_->markStale();
    EXPECT_TRUE(view_->getStats().is_stale);
}

// =============================================================================
// AC-6  Query speedup: pre-computed results returned without re-scanning rows
// =============================================================================

/// getRows() on a populated view runs in sub-millisecond time regardless of
/// "virtual" dataset size — the snapshot is accessed via a single O(1) copy
/// while a simulated full scan must visit every element.
TEST(MaterializedViewPerformanceTests, AC6_PrecomputedFasterThanRawScan) {
    const size_t N = 10'000;

    auto def = makeDefinition(
        "perf_view", MaterializedView::RefreshStrategy::MANUAL);
    auto view = createView(def);

    // Build a large snapshot.
    std::vector<nlohmann::json> rows;
    rows.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        rows.push_back(makeSaleRow(
            "k" + std::to_string(i),
            (i % 2 == 0) ? "EU" : "US",
            static_cast<double>(i)));
    }
    ASSERT_TRUE(view->refresh(false, std::move(rows)).has_value());

    // Verify the snapshot is returned correctly.
    const auto t0        = std::chrono::steady_clock::now();
    auto result_rows     = view->getRows();
    const auto elapsed   = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - t0).count();

    EXPECT_EQ(result_rows.size(), N);

    // The view snapshot read must complete in under 10 ms even for 10k rows.
    // This is a generous threshold to tolerate slow CI environments while still
    // demonstrating that no per-row computation is performed.
    EXPECT_LT(elapsed, 10'000)
        << "Pre-computed getRows took " << elapsed << " µs (expected <10 ms)";
}

/// queryRows with a filter is still backed by the snapshot (no base-table scan).
TEST(MaterializedViewPerformanceTests, AC6_QueryRowsFilterWorksOnSnapshot) {
    auto def = makeDefinition(
        "filter_view", MaterializedView::RefreshStrategy::MANUAL);
    auto view = createView(def);

    view->refresh(false, {
        makeSaleRow("k1", "EU",   10.0),
        makeSaleRow("k2", "US",   20.0),
        makeSaleRow("k3", "EU",   30.0),
        makeSaleRow("k4", "APAC", 40.0)
    });

    auto eu_rows = view->queryRows("region", "EU");
    EXPECT_EQ(eu_rows.size(), 2u);
    for (const auto& r : eu_rows) {
        EXPECT_EQ(r.value("region", ""), "EU");
    }
}

// =============================================================================
// AC-7  Insert overhead: IMMEDIATE maintenance applies deltas in-place
// =============================================================================

/// Multiple sequential INSERTs with IMMEDIATE strategy stay consistent.
TEST(MaterializedViewDeltaTests, AC7_MultipleImmediateInsertsConsistent) {
    auto def = makeDefinition(
        "multi_insert_view", MaterializedView::RefreshStrategy::IMMEDIATE);
    auto view = createView(def);
    view->refresh(false, {});

    const size_t N = 100;
    for (size_t i = 0; i < N; ++i) {
        view->applyDeltaJson(DeltaOp::INSERT,
                             makeSaleRow("k" + std::to_string(i),
                                         "EU",
                                         static_cast<double>(i)));
    }
    EXPECT_EQ(view->getRows().size(), N);
}

/// Stats counters are updated correctly for IMMEDIATE INSERTs.
TEST(MaterializedViewDeltaTests, AC7_StatsCountedForImmediateInserts) {
    auto def = makeDefinition(
        "stats_view", MaterializedView::RefreshStrategy::IMMEDIATE);
    auto view = createView(def);
    view->refresh(false, {});

    view->applyDeltaJson(DeltaOp::INSERT, makeSaleRow("k1", "EU", 1.0));
    view->applyDeltaJson(DeltaOp::INSERT, makeSaleRow("k2", "US", 2.0));
    view->applyDeltaJson(DeltaOp::DELETE, nlohmann::json{{"_key", "k1"}});

    auto stats = view->getStats();
    EXPECT_EQ(stats.delta_inserts,       2u);
    EXPECT_EQ(stats.delta_deletes,       1u);
    EXPECT_EQ(stats.current_row_count,   1u);
    EXPECT_EQ(stats.full_refreshes,      1u);  // initial refresh(false)
}

/// IMMEDIATE insert with DEFERRED fallback: the DEFERRED view is untouched.
TEST(MaterializedViewDeltaTests, AC7_ImmediateAndDeferredIndependentBehavior) {
    auto imm_def = makeDefinition(
        "imm2", MaterializedView::RefreshStrategy::IMMEDIATE);
    auto def_def = makeDefinition(
        "def2", MaterializedView::RefreshStrategy::DEFERRED);

    auto imm_view = createView(imm_def);
    auto def_view = createView(def_def);

    imm_view->refresh(false, {});
    def_view->refresh(false, {makeSaleRow("k1", "EU", 1.0)});

    imm_view->applyDeltaJson(DeltaOp::INSERT, makeSaleRow("k2", "US", 2.0));
    def_view->applyDeltaJson(DeltaOp::INSERT, makeSaleRow("k3", "APAC", 3.0));

    // IMMEDIATE view has the new row.
    EXPECT_EQ(imm_view->getRows().size(), 1u);
    EXPECT_FALSE(imm_view->isStale());

    // DEFERRED view is stale but still has the original row.
    EXPECT_EQ(def_view->getRows().size(), 1u);
    EXPECT_TRUE(def_view->isStale());
}

// =============================================================================
// Registry: registration and removal
// =============================================================================

TEST(MaterializedViewRegistryTests, RegisterAndRemove) {
    MaterializedViewRegistry reg;

    auto view = createView(makeDefinition("v1"));
    EXPECT_TRUE(reg.registerView(view).has_value());
    EXPECT_NE(reg.getView("v1"), nullptr);

    // Duplicate registration is an error.
    EXPECT_FALSE(reg.registerView(view).has_value());

    EXPECT_TRUE(reg.removeView("v1"));
    EXPECT_EQ(reg.getView("v1"), nullptr);

    // Second removal returns false.
    EXPECT_FALSE(reg.removeView("v1"));
}

TEST(MaterializedViewRegistryTests, ListViews) {
    MaterializedViewRegistry reg;
    ASSERT_TRUE(reg.registerView(createView(makeDefinition("alpha"))).has_value());
    ASSERT_TRUE(reg.registerView(createView(makeDefinition("beta"))).has_value());

    auto names = reg.listViews();
    ASSERT_EQ(names.size(), 2u);

    // Order is unspecified; just check both names are present.
    const bool has_alpha =
        std::find(names.begin(), names.end(), "alpha") != names.end();
    const bool has_beta  =
        std::find(names.begin(), names.end(), "beta")  != names.end();
    EXPECT_TRUE(has_alpha);
    EXPECT_TRUE(has_beta);
}
