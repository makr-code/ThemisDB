/**
 * @file test_schema_api_lineage.cpp
 * @brief Unit tests for SchemaApiHandler column-lineage REST endpoints
 *
 * Tests cover:
 * - handleGetColumnLineage returns 503 when tracker not set
 * - handleRecordLineageDerivation returns 503 when tracker not set
 * - handleRecordLineageDerivation records an entry and returns 201
 * - handleGetColumnLineage returns table-level lineage array
 * - handleGetColumnLineage returns column-level provenance object
 * - handleGetColumnLineage returns 400 for missing table name
 * - handleRecordLineageDerivation returns 400 for missing target field
 * - handleRecordLineageDerivation returns 400 for invalid JSON
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <boost/beast/http.hpp>
#include <memory>
#include <string>

#include "server/schema_api_handler.h"
#include "metadata/column_lineage.h"

using json = nlohmann::json;
namespace http = boost::beast::http;
using namespace themis::metadata;
using namespace themis::server;

// ─── helpers ─────────────────────────────────────────────────────────────────

static http::request<http::string_body> makeGet(const std::string& target) {
    http::request<http::string_body> req{http::verb::get, target, 11};
    req.set(http::field::content_type, "application/json");
    req.prepare_payload();
    return req;
}

static http::request<http::string_body> makePost(const std::string& target,
                                                  const std::string& body) {
    http::request<http::string_body> req{http::verb::post, target, 11};
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.prepare_payload();
    return req;
}

// ─── Fixture ──────────────────────────────────────────────────────────────────

class SchemaApiLineageTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Construct handler with null storage/index/schema — the lineage handlers
        // do not touch those members, so nullptr is safe for these tests.
        handler_ = std::make_unique<SchemaApiHandler>(nullptr, nullptr, nullptr);
        tracker_ = std::make_unique<ColumnLineageTracker>();
    }

    SchemaApiHandler& handler() { return *handler_; }

    /// Wire the tracker into the handler.
    void attach() { handler_->setColumnLineageTracker(tracker_.get()); }

    ColumnLineageTracker& tracker() { return *tracker_; }

    std::unique_ptr<SchemaApiHandler>     handler_;
    std::unique_ptr<ColumnLineageTracker> tracker_;
};

// ─── 503 when tracker not attached ───────────────────────────────────────────

TEST_F(SchemaApiLineageTest, GetLineage_NoTracker_Returns503) {
    auto res = handler().handleGetColumnLineage(
        makeGet("/api/v1/metadata/lineage/users"));
    EXPECT_EQ(res.result(), http::status::service_unavailable);
}

TEST_F(SchemaApiLineageTest, RecordDerivation_NoTracker_Returns503) {
    json body;
    body["target"] = {{"table", "users"}, {"column", "full_name"}};
    auto res = handler().handleRecordLineageDerivation(
        makePost("/api/v1/metadata/lineage", body.dump()));
    EXPECT_EQ(res.result(), http::status::service_unavailable);
}

// ─── record derivation ────────────────────────────────────────────────────────

TEST_F(SchemaApiLineageTest, RecordDerivation_ValidEntry_Returns201) {
    attach();
    json body;
    body["target"] = {{"table", "users"}, {"column", "full_name"}};
    body["source_columns"] = json::array({
        {{"table", "users"}, {"column", "first_name"}},
        {{"table", "users"}, {"column", "last_name"}}
    });
    body["transformation"] = "COMPUTED";
    body["transformation_expression"] = "first_name || ' ' || last_name";
    body["performed_by"] = "test-service";

    auto res = handler().handleRecordLineageDerivation(
        makePost("/api/v1/metadata/lineage", body.dump()));

    EXPECT_EQ(res.result(), http::status::created);

    json resp = json::parse(res.body());
    EXPECT_EQ(resp["status"], "recorded");
    EXPECT_EQ(resp["total_entries"], 1u);

    // Verify entry in tracker
    EXPECT_EQ(tracker().totalEntryCount(), 1u);
    auto record = tracker().getColumnLineage({"users", "full_name"});
    ASSERT_EQ(record.entries.size(), 1u);
    EXPECT_EQ(record.entries[0].transformation, TransformationType::COMPUTED);
    EXPECT_EQ(record.entries[0].source_columns.size(), 2u);
}

TEST_F(SchemaApiLineageTest, RecordDerivation_NoTarget_Returns400) {
    attach();
    json body;
    body["transformation"] = "DIRECT_COPY";

    auto res = handler().handleRecordLineageDerivation(
        makePost("/api/v1/metadata/lineage", body.dump()));

    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(SchemaApiLineageTest, RecordDerivation_InvalidJSON_Returns400) {
    attach();
    auto res = handler().handleRecordLineageDerivation(
        makePost("/api/v1/metadata/lineage", "{not valid json"));
    EXPECT_EQ(res.result(), http::status::bad_request);
}

// ─── get table lineage ────────────────────────────────────────────────────────

TEST_F(SchemaApiLineageTest, GetTableLineage_ReturnsArray) {
    attach();

    // Pre-populate
    ColumnLineageEntry e;
    e.target_column  = {"orders", "price_eur"};
    e.source_columns = {{"orders", "price_cents"}};
    e.transformation = TransformationType::CAST;
    e.timestamp_ms   = 1000LL;
    tracker().recordDerivation(e);

    auto res = handler().handleGetColumnLineage(
        makeGet("/api/v1/metadata/lineage/orders"));

    EXPECT_EQ(res.result(), http::status::ok);
    json arr = json::parse(res.body());
    ASSERT_TRUE(arr.is_array());
    ASSERT_EQ(arr.size(), 1u);
    EXPECT_EQ(arr[0]["column"]["table"], "orders");
    EXPECT_EQ(arr[0]["column"]["column"], "price_eur");
}

TEST_F(SchemaApiLineageTest, GetTableLineage_UnknownTableReturnsEmptyArray) {
    attach();
    auto res = handler().handleGetColumnLineage(
        makeGet("/api/v1/metadata/lineage/nonexistent"));
    EXPECT_EQ(res.result(), http::status::ok);
    json arr = json::parse(res.body());
    EXPECT_TRUE(arr.is_array());
    EXPECT_TRUE(arr.empty());
}

// ─── get column provenance ────────────────────────────────────────────────────

TEST_F(SchemaApiLineageTest, GetColumnProvenance_ReturnsProvenanceObject) {
    attach();

    ColumnLineageEntry e;
    e.target_column  = {"users", "full_name"};
    e.source_columns = {{"users", "first_name"}, {"users", "last_name"}};
    e.transformation = TransformationType::COMPUTED;
    e.timestamp_ms   = 1000LL;
    tracker().recordDerivation(e);

    auto res = handler().handleGetColumnLineage(
        makeGet("/api/v1/metadata/lineage/users/full_name"));

    EXPECT_EQ(res.result(), http::status::ok);
    json prov = json::parse(res.body());
    EXPECT_TRUE(prov.contains("column"));
    EXPECT_TRUE(prov.contains("entries"));
    EXPECT_TRUE(prov.contains("upstream_columns"));
    EXPECT_TRUE(prov.contains("downstream_columns"));
    EXPECT_EQ(prov["column"]["table"],  "users");
    EXPECT_EQ(prov["column"]["column"], "full_name");
    EXPECT_EQ(prov["entries"].size(), 1u);
    EXPECT_EQ(prov["upstream_columns"].size(), 2u);
}

TEST_F(SchemaApiLineageTest, GetColumnProvenance_MissingTableName_Returns400) {
    attach();
    // Path is "/api/v1/metadata/lineage/" with no table name
    auto res = handler().handleGetColumnLineage(
        makeGet("/api/v1/metadata/lineage/"));
    EXPECT_EQ(res.result(), http::status::bad_request);
}

// ─── response content type ───────────────────────────────────────────────────

TEST_F(SchemaApiLineageTest, GetColumnLineage_ResponseIsJSON) {
    attach();
    auto res = handler().handleGetColumnLineage(
        makeGet("/api/v1/metadata/lineage/any_table"));
    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(res[http::field::content_type], "application/json");
}

TEST_F(SchemaApiLineageTest, RecordDerivation_ResponseIsJSON) {
    attach();
    json body;
    body["target"] = {{"table", "t"}, {"column", "c"}};
    auto res = handler().handleRecordLineageDerivation(
        makePost("/api/v1/metadata/lineage", body.dump()));
    EXPECT_EQ(res.result(), http::status::created);
    EXPECT_EQ(res[http::field::content_type], "application/json");
}
