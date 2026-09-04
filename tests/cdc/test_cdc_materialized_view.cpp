/**
 * Unit tests for CDCMaterializedViewMaintainer
 *
 * Tests CDC change event → incremental view maintenance bridge:
 *  - INSERT events update view aggregations
 *  - DELETE events roll back aggregations
 *  - UPDATE events apply before/after delta
 *  - TRANSACTION_* events are silently skipped
 *  - Collection derivation from key prefix (before ':')
 *  - Value-field fallback when snapshots are absent
 *  - Multiple views dispatched from a single applyEvents() call
 *  - View lifecycle: createView / dropView / hasView / listViews / getView
 *  - totalEventsProcessed() counter
 *  - Query filters, limit, offset
 *  - Unknown collection events are silently ignored by unrelated views
 */

#include <gtest/gtest.h>
#include "cdc/cdc_materialized_view.h"

#include <chrono>
#include <cmath>
#include <string>
#include <vector>

using namespace themis;
using namespace themis::cdc;
using namespace themisdb::analytics;

// ============================================================================
// Helpers
// ============================================================================

static int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

/** Build a PUT event (INSERT when no before_snapshot). */
static Changefeed::ChangeEvent makePut(
    const std::string& key,
    const std::string& after_snapshot,
    std::optional<std::string> before_snapshot = std::nullopt)
{
    Changefeed::ChangeEvent ev;
    ev.sequence        = 0;
    ev.type            = Changefeed::ChangeEventType::EVENT_PUT;
    ev.key             = key;
    ev.value           = after_snapshot;
    ev.timestamp_ms    = nowMs();
    ev.before_snapshot = before_snapshot;
    ev.after_snapshot  = after_snapshot;
    return ev;
}

/** Build a DELETE event. */
static Changefeed::ChangeEvent makeDelete(
    const std::string& key,
    const std::string& before_snapshot)
{
    Changefeed::ChangeEvent ev;
    ev.sequence        = 0;
    ev.type            = Changefeed::ChangeEventType::EVENT_DELETE;
    ev.key             = key;
    ev.timestamp_ms    = nowMs();
    ev.before_snapshot = before_snapshot;
    return ev;
}

/** Build a TRANSACTION_COMMIT event (should be skipped). */
static Changefeed::ChangeEvent makeTxCommit() {
    Changefeed::ChangeEvent ev;
    ev.sequence     = 0;
    ev.type         = Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT;
    ev.key          = "";
    ev.timestamp_ms = nowMs();
    return ev;
}

/** Build a TRANSACTION_ROLLBACK event (should be skipped). */
static Changefeed::ChangeEvent makeTxRollback() {
    Changefeed::ChangeEvent ev;
    ev.sequence     = 0;
    ev.type         = Changefeed::ChangeEventType::EVENT_TRANSACTION_ROLLBACK;
    ev.key          = "";
    ev.timestamp_ms = nowMs();
    return ev;
}

/** Build a simple COUNT/SUM-by-region view on the "sales" collection. */
static ViewDefinition salesViewDef(const std::string& name = "sales_by_region") {
    ViewDefinition def;
    def.name              = name;
    def.source_collection = "sales";
    def.dimensions        = {"region"};
    def.aggregations      = {
        {"total",  ViewAggFunc::SUM,   "amount"},
        {"orders", ViewAggFunc::COUNT, ""}
    };
    return def;
}

/** Find the aggregated value for a given dimension key in the result. */
static FieldValue findValue(
    const ViewQueryResult& result,
    const std::string& dim_val,
    const std::string& agg_name)
{
    for (const auto& row : result.rows) {
        auto it = row.group_key.find("region");
        if (it != row.group_key.end() && it->second == dim_val) {
            auto vit = row.values.find(agg_name);
            if (vit != row.values.end()) {
              return vit->second;
            }
        }
    }
    return FieldValue{nullptr};
}

// ============================================================================
// Tests: view lifecycle
// ============================================================================

TEST(CDCMaterializedViewMaintainer, CreateAndHasView) {
    CDCMaterializedViewMaintainer m;
    EXPECT_FALSE(m.hasView("sales_by_region"));
    EXPECT_TRUE(m.createView(salesViewDef()));
    EXPECT_TRUE(m.hasView("sales_by_region"));
}

TEST(CDCMaterializedViewMaintainer, DuplicateCreateReturnsFalse) {
    CDCMaterializedViewMaintainer m;
    EXPECT_TRUE(m.createView(salesViewDef()));
    EXPECT_FALSE(m.createView(salesViewDef()));
}

TEST(CDCMaterializedViewMaintainer, DropView) {
    CDCMaterializedViewMaintainer m;
    m.createView(salesViewDef());
    EXPECT_TRUE(m.dropView("sales_by_region"));
    EXPECT_FALSE(m.hasView("sales_by_region"));
    EXPECT_FALSE(m.dropView("sales_by_region"));  // already gone
}

TEST(CDCMaterializedViewMaintainer, ListViews) {
    CDCMaterializedViewMaintainer m;
    EXPECT_TRUE(m.listViews().empty());
    m.createView(salesViewDef("v1"));
    m.createView(salesViewDef("v2"));
    auto names = m.listViews();
    ASSERT_EQ(names.size(), 2u);
}

TEST(CDCMaterializedViewMaintainer, GetView) {
    CDCMaterializedViewMaintainer m;
    EXPECT_EQ(m.getView("missing"), nullptr);
    m.createView(salesViewDef());
    EXPECT_NE(m.getView("sales_by_region"), nullptr);
}

// ============================================================================
// Tests: INSERT events
// ============================================================================

TEST(CDCMaterializedViewMaintainer, InsertEventUpdatesView) {
    CDCMaterializedViewMaintainer m;
    m.createView(salesViewDef());

    m.applyEvent(makePut("sales:1", R"({"region":"EU","amount":100.0})"));
    m.applyEvent(makePut("sales:2", R"({"region":"EU","amount":50.0})"));
    m.applyEvent(makePut("sales:3", R"({"region":"US","amount":200.0})"));

    auto result = m.query("sales_by_region");
    ASSERT_EQ(result.rows.size(), 2u);

    auto eu_total  = findValue(result, "EU", "total");
    auto eu_orders = findValue(result, "EU", "orders");
    auto us_total  = findValue(result, "US", "total");

    EXPECT_DOUBLE_EQ(std::get<double>(eu_total), 150.0);
    EXPECT_EQ(std::get<int64_t>(eu_orders), 2);
    EXPECT_DOUBLE_EQ(std::get<double>(us_total), 200.0);
}

TEST(CDCMaterializedViewMaintainer, InsertEventCountsProcessed) {
    CDCMaterializedViewMaintainer m;
    m.createView(salesViewDef());

    m.applyEvent(makePut("sales:1", R"({"region":"EU","amount":10.0})"));
    m.applyEvent(makePut("sales:2", R"({"region":"US","amount":20.0})"));
    EXPECT_EQ(m.totalEventsProcessed(), 2u);
}

// ============================================================================
// Tests: DELETE events
// ============================================================================

TEST(CDCMaterializedViewMaintainer, DeleteEventRollsBackAggregation) {
    CDCMaterializedViewMaintainer m;
    m.createView(salesViewDef());

    m.applyEvent(makePut("sales:1", R"({"region":"EU","amount":100.0})"));
    m.applyEvent(makePut("sales:2", R"({"region":"EU","amount":50.0})"));
    m.applyEvent(makeDelete("sales:1", R"({"region":"EU","amount":100.0})"));

    auto result    = m.query("sales_by_region");
    auto eu_total  = findValue(result, "EU", "total");
    auto eu_orders = findValue(result, "EU", "orders");

    EXPECT_DOUBLE_EQ(std::get<double>(eu_total), 50.0);
    EXPECT_EQ(std::get<int64_t>(eu_orders), 1);
}

// ============================================================================
// Tests: UPDATE events
// ============================================================================

TEST(CDCMaterializedViewMaintainer, UpdateEventAppliesDelta) {
    CDCMaterializedViewMaintainer m;
    m.createView(salesViewDef());

    m.applyEvent(makePut("sales:1", R"({"region":"EU","amount":100.0})"));

    // Update: amount changes from 100 to 150
    m.applyEvent(makePut(
        "sales:1",
        R"({"region":"EU","amount":150.0})",   // after
        R"({"region":"EU","amount":100.0})"    // before
    ));

    auto result   = m.query("sales_by_region");
    auto eu_total = findValue(result, "EU", "total");
    EXPECT_DOUBLE_EQ(std::get<double>(eu_total), 150.0);
}

// ============================================================================
// Tests: TRANSACTION_* events are skipped
// ============================================================================

TEST(CDCMaterializedViewMaintainer, TransactionEventsAreSkipped) {
    CDCMaterializedViewMaintainer m;
    m.createView(salesViewDef());

    m.applyEvent(makeTxCommit());
    m.applyEvent(makeTxRollback());

    EXPECT_EQ(m.totalEventsProcessed(), 0u);
    auto result = m.query("sales_by_region");
    EXPECT_TRUE(result.rows.empty());
}

// ============================================================================
// Tests: collection derivation from key prefix
// ============================================================================

TEST(CDCMaterializedViewMaintainer, CollectionExtractedFromKeyPrefix) {
    CDCMaterializedViewMaintainer m;
    // View watches "orders", not "sales"
    ViewDefinition def;
    def.name              = "orders_count";
    def.source_collection = "orders";
    def.dimensions        = {"status"};
    def.aggregations      = {{"cnt", ViewAggFunc::COUNT, ""}};
    m.createView(def);

    // "sales:1" → collection "sales" — should not match the "orders" view
    m.applyEvent(makePut("sales:1", R"({"status":"new"})"));
    // "orders:1" → collection "orders" — should match
    m.applyEvent(makePut("orders:1", R"({"status":"new"})"));
    m.applyEvent(makePut("orders:2", R"({"status":"shipped"})"));

    EXPECT_EQ(m.totalEventsProcessed(), 3u);  // all three were data events

    auto result = m.query("orders_count");
    EXPECT_EQ(result.rows.size(), 2u);  // "new" and "shipped"
}

TEST(CDCMaterializedViewMaintainer, KeyWithoutColonUsedAsCollection) {
    CDCMaterializedViewMaintainer m;
    ViewDefinition def;
    def.name              = "simple_count";
    def.source_collection = "items";
    def.dimensions        = {"cat"};
    def.aggregations      = {{"cnt", ViewAggFunc::COUNT, ""}};
    m.createView(def);

    // No ':' in the key — full key is the collection name
    m.applyEvent(makePut("items", R"({"cat":"A"})"));
    auto result = m.query("simple_count");
    EXPECT_EQ(result.rows.size(), 1u);
}

// ============================================================================
// Tests: value-field fallback (no snapshots)
// ============================================================================

TEST(CDCMaterializedViewMaintainer, ValueFallbackForInsert) {
    CDCMaterializedViewMaintainer m;
    m.createView(salesViewDef());

    // Build an event with no snapshots — maintainer falls back to 'value'
    Changefeed::ChangeEvent ev;
    ev.sequence     = 0;
    ev.type         = Changefeed::ChangeEventType::EVENT_PUT;
    ev.key          = "sales:99";
    ev.value        = R"({"region":"AP","amount":75.0})";
    ev.timestamp_ms = nowMs();
    // before_snapshot and after_snapshot left unset (nullopt)

    m.applyEvent(ev);

    auto result   = m.query("sales_by_region");
    auto ap_total = findValue(result, "AP", "total");
    EXPECT_DOUBLE_EQ(std::get<double>(ap_total), 75.0);
}

// ============================================================================
// Tests: applyEvents batch
// ============================================================================

TEST(CDCMaterializedViewMaintainer, ApplyEventsBatch) {
    CDCMaterializedViewMaintainer m;
    m.createView(salesViewDef());

    std::vector<Changefeed::ChangeEvent> batch = {
        makePut("sales:1", R"({"region":"EU","amount":10.0})"),
        makePut("sales:2", R"({"region":"EU","amount":20.0})"),
        makePut("sales:3", R"({"region":"US","amount":30.0})"),
        makeTxCommit(),     // must be skipped
        makeTxRollback(),   // must be skipped
    };
    m.applyEvents(batch);

    EXPECT_EQ(m.totalEventsProcessed(), 3u);

    auto result   = m.query("sales_by_region");
    auto eu_total = findValue(result, "EU", "total");
    EXPECT_DOUBLE_EQ(std::get<double>(eu_total), 30.0);
}

// ============================================================================
// Tests: multiple views
// ============================================================================

TEST(CDCMaterializedViewMaintainer, MultipleViewsDispatched) {
    CDCMaterializedViewMaintainer m;

    // Two views on the same collection
    ViewDefinition by_region = salesViewDef("by_region");
    ViewDefinition by_product;
    by_product.name              = "by_product";
    by_product.source_collection = "sales";
    by_product.dimensions        = {"product"};
    by_product.aggregations      = {{"total", ViewAggFunc::SUM, "amount"}};

    m.createView(by_region);
    m.createView(by_product);

    m.applyEvent(makePut("sales:1",
        R"({"region":"EU","product":"X","amount":50.0})"));

    auto r1 = m.query("by_region");
    auto r2 = m.query("by_product");

    EXPECT_EQ(r1.rows.size(), 1u);
    EXPECT_EQ(r2.rows.size(), 1u);

    auto eu_total = findValue(r1, "EU", "total");
    EXPECT_DOUBLE_EQ(std::get<double>(eu_total), 50.0);
}

// ============================================================================
// Tests: query with limit / offset
// ============================================================================

TEST(CDCMaterializedViewMaintainer, QueryLimitOffset) {
    CDCMaterializedViewMaintainer m;
    m.createView(salesViewDef());

    for (int i = 0; i < 5; ++i) {
        std::string key = "sales:" + std::to_string(i);
        std::string doc = R"({"region":"R)" + std::to_string(i) +
                          R"(","amount":1.0})";
        m.applyEvent(makePut(key, doc));
    }

    auto all    = m.query("sales_by_region");
    auto paged  = m.query("sales_by_region", {}, 2, 0);

    EXPECT_EQ(all.total_rows, 5);
    EXPECT_EQ(paged.rows.size(), 2u);
}

// ============================================================================
// Tests: query on unknown view returns empty result
// ============================================================================

TEST(CDCMaterializedViewMaintainer, QueryUnknownViewReturnsEmpty) {
    CDCMaterializedViewMaintainer m;
    auto result = m.query("does_not_exist");
    EXPECT_TRUE(result.rows.empty());
    EXPECT_EQ(result.total_rows, 0);
}

// ============================================================================
// Tests: invalid / empty JSON snapshot handled gracefully
// ============================================================================

TEST(CDCMaterializedViewMaintainer, InvalidJsonSnapshotHandledGracefully) {
    CDCMaterializedViewMaintainer m;
    m.createView(salesViewDef());

    Changefeed::ChangeEvent ev;
    ev.sequence        = 0;
    ev.type            = Changefeed::ChangeEventType::EVENT_PUT;
    ev.key             = "sales:bad";
    ev.value           = "not-valid-json{{{";
    ev.after_snapshot  = "not-valid-json{{{";
    ev.timestamp_ms    = nowMs();

    // Must not throw; the row will be empty and the view unchanged
    EXPECT_NO_THROW(m.applyEvent(ev));
    EXPECT_EQ(m.totalEventsProcessed(), 1u);
}
