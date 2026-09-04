// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Tests for:
//  - PrometheusMetrics: new MVCC/HLC metric methods
//  - MvccApiHandler: all HTTP endpoints (via a mock MVCCStore)

#include <gtest/gtest.h>
#include "sharding/prometheus_metrics.h"
#include "storage/hlc.h"
#include "storage/mvcc_store.h"
#include "server/mvcc_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <httplib.h>

using namespace themis;
using namespace themis::server;
using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// PrometheusMetrics MVCC methods
// ─────────────────────────────────────────────────────────────────────────────

class PrometheusMetricsMvccTest : public ::testing::Test {
protected:
    void SetUp() override {
        sharding::PrometheusMetrics::Config cfg;
        metrics_ = std::make_unique<sharding::PrometheusMetrics>(cfg);
    }
    std::unique_ptr<sharding::PrometheusMetrics> metrics_;
};

TEST_F(PrometheusMetricsMvccTest, RecordMvccWrite_AppearsInOutput) {
    metrics_->recordMvccWrite(5.0);
    metrics_->recordMvccWrite(10.0);
    std::string output = metrics_->getMetrics();
    EXPECT_NE(output.find("themis_mvcc_writes_total"), std::string::npos);
}

TEST_F(PrometheusMetricsMvccTest, RecordMvccRead_BothTypes) {
    metrics_->recordMvccRead("latest",   1.0);
    metrics_->recordMvccRead("snapshot", 2.0);
    std::string output = metrics_->getMetrics();
    EXPECT_NE(output.find("themis_mvcc_reads_total"), std::string::npos);
}

TEST_F(PrometheusMetricsMvccTest, RecordMvccGc) {
    metrics_->recordMvccGc(42);
    std::string output = metrics_->getMetrics();
    EXPECT_NE(output.find("themis_mvcc_gc_runs_total"), std::string::npos);
}

TEST_F(PrometheusMetricsMvccTest, SetMvccVersionCount) {
    metrics_->setMvccVersionCount(100);
    std::string output = metrics_->getMetrics();
    EXPECT_NE(output.find("themis_mvcc_version_entries"), std::string::npos);
}

TEST_F(PrometheusMetricsMvccTest, RecordHlcAdvance) {
    metrics_->recordHlcAdvance("local");
    metrics_->recordHlcAdvance("received");
    std::string output = metrics_->getMetrics();
    EXPECT_NE(output.find("themis_hlc_advances_total"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// MvccApiHandler test fixture (backed by a real MVCCStore + temp RocksDB)
// ─────────────────────────────────────────────────────────────────────────────

class MvccApiHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "./data/themis_mvcc_api_test";
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        rocksdb_ = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(rocksdb_->open());

        auto clock = std::make_shared<HybridLogicalClock>();
        store_ = std::make_shared<MVCCStore>(rocksdb_, clock);

        handler_ = std::make_unique<MvccApiHandler>(store_);
    }

    void TearDown() override {
        handler_.reset();
        store_.reset();
        rocksdb_.reset();
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
    }

    // ─── Helpers ────────────────────────────────────────────────────────────

    /** Build a minimal httplib::Request with the given path, method, and body. */
    static httplib::Request makeRequest(
        const std::string& method,
        const std::string& path,
        const std::string& body = {},
        const std::map<std::string, std::string>& params = {}
    ) {
        httplib::Request req;
        req.method = method;
        req.path   = path;
        req.body   = body;
        for (auto& [k, v] : params) {
            req.params.emplace(k, v);
        }
        return req;
    }

    /** Set req.matches[1] = key (the captured regex group). */
    static httplib::Request makeKeyRequest(
        const std::string& method,
        const std::string& key,
        const std::string& body = {},
        const std::map<std::string, std::string>& params = {}
    ) {
        auto req = makeRequest(method, "/api/v1/mvcc/keys/" + key, body, params);
        // Simulate regex capture group for the key
        req.matches = httplib::Match{};
        req.matches.ready();
        // matches[0] = full match, matches[1] = captured key
        // We can't easily use std::match_results here, so we use the params trick:
        // Instead, set a custom marker and let the handler use params.
        // Actually httplib::Match wraps std::smatch – we need to use a workaround.
        // We'll inject the key via a private request header that extractKey checks.
        // Simplest: just construct the regex match properly.
        std::smatch m = {};
        std::string path = "/api/v1/mvcc/keys/" + key;
        std::regex re(R"(/api/v1/mvcc/keys/([^/]+))");
        std::regex_search(path, m, re);
        req.matches = m;
        return req;
    }

    /** Build a 'versions' path request. */
    static httplib::Request makeVersionsRequest(
        const std::string& method,
        const std::string& key,
        const std::string& body = {}
    ) {
        auto req = makeRequest(method,
            "/api/v1/mvcc/keys/" + key + "/versions", body);
        std::smatch m = {};
        std::string path = "/api/v1/mvcc/keys/" + key + "/versions";
        std::regex re(R"(/api/v1/mvcc/keys/([^/]+)/versions)");
        std::regex_search(path, m, re);
        req.matches = m;
        return req;
    }

    static json parseResponse(const httplib::Response& res) {
        return json::parse(res.body);
    }

    std::string db_path_;
    std::shared_ptr<RocksDBWrapper>  rocksdb_;
    std::shared_ptr<MVCCStore>       store_;
    std::unique_ptr<MvccApiHandler>  handler_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MvccApiHandlerTest, Construction_NullStoreThrows) {
    EXPECT_THROW(MvccApiHandler(nullptr), std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// GET clock
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MvccApiHandlerTest, GetClock_ReturnsTimestamp) {
    auto req = makeRequest("GET", "/api/v1/mvcc/clock");
    httplib::Response res;
    handler_->handleGetClock(req, res);

    EXPECT_EQ(res.status, 200);
    auto body = parseResponse(res);
    EXPECT_TRUE(body.contains("timestamp"));
    EXPECT_TRUE(body.contains("physical_ms"));
    EXPECT_TRUE(body.contains("logical"));
    EXPECT_GE(body["timestamp"].get<uint64_t>(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// POST (write) then GET (read latest)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MvccApiHandlerTest, PutAndGetLatest) {
    // Write
    auto put_req = makeKeyRequest("POST", "mykey",
        R"({"value":"hello"})");
    httplib::Response put_res;
    handler_->handlePutKey(put_req, put_res);

    EXPECT_EQ(put_res.status, 201);
    auto put_body = parseResponse(put_res);
    EXPECT_EQ(put_body["key"].get<std::string>(), "mykey");
    EXPECT_TRUE(put_body.contains("timestamp"));
    uint64_t ts = put_body["timestamp"].get<uint64_t>();
    EXPECT_GT(ts, 0u);

    // Read back (latest)
    auto get_req = makeKeyRequest("GET", "mykey");
    httplib::Response get_res;
    handler_->handleGetKey(get_req, get_res);

    EXPECT_EQ(get_res.status, 200);
    auto get_body = parseResponse(get_res);
    EXPECT_EQ(get_body["key"].get<std::string>(), "mykey");
    EXPECT_EQ(get_body["value"].get<std::string>(), "hello");
}

TEST_F(MvccApiHandlerTest, GetLatest_MissingKey_Returns404) {
    auto req = makeKeyRequest("GET", "nosuchkey");
    httplib::Response res;
    handler_->handleGetKey(req, res);

    EXPECT_EQ(res.status, 404);
    auto body = parseResponse(res);
    EXPECT_TRUE(body.contains("error"));
}

TEST_F(MvccApiHandlerTest, GetLatest_InvalidKeyTraversal_Returns400) {
    auto req = makeKeyRequest("GET", "../nosuchkey");
    httplib::Response res;
    handler_->handleGetKey(req, res);

    EXPECT_EQ(res.status, 400);
}

TEST_F(MvccApiHandlerTest, Put_MissingValueField_Returns400) {
    auto req = makeKeyRequest("POST", "k", R"({"wrong":"field"})");
    httplib::Response res;
    handler_->handlePutKey(req, res);
    EXPECT_EQ(res.status, 400);
}

TEST_F(MvccApiHandlerTest, Put_InvalidKeyTraversal_Returns400) {
    auto req = makeKeyRequest("POST", "../k", R"({"value":"hello"})");
    httplib::Response res;
    handler_->handlePutKey(req, res);
    EXPECT_EQ(res.status, 400);
}

TEST_F(MvccApiHandlerTest, Put_InvalidJson_Returns400) {
    auto req = makeKeyRequest("POST", "k", "{not json}");
    httplib::Response res;
    handler_->handlePutKey(req, res);
    EXPECT_EQ(res.status, 400);
}

TEST_F(MvccApiHandlerTest, Put_EmptyKey_Returns400) {
    // Simulate empty key capture
    httplib::Request req;
    req.method = "POST";
    req.body = R"({"value":"v"})";
    std::smatch m = {};
    std::string path = "/api/v1/mvcc/keys/";
    // matches[1] will be empty string – we simulate by clearing the body key capture
    // Use the "no match" branch: matches.size() < 2
    req.matches = httplib::Match{};
    httplib::Response res;
    handler_->handlePutKey(req, res);
    EXPECT_EQ(res.status, 400);
}

// ─────────────────────────────────────────────────────────────────────────────
// Snapshot reads
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MvccApiHandlerTest, GetAtTimestamp_ReturnsCorrectVersion) {
    // Write two versions
    auto put_req1 = makeKeyRequest("POST", "k", R"({"value":"v1"})");
    httplib::Response put_res1;
    handler_->handlePutKey(put_req1, put_res1);
    ASSERT_EQ(put_res1.status, 201);
    uint64_t ts1 = parseResponse(put_res1)["timestamp"].get<uint64_t>();

    auto put_req2 = makeKeyRequest("POST", "k", R"({"value":"v2"})");
    httplib::Response put_res2;
    handler_->handlePutKey(put_req2, put_res2);
    ASSERT_EQ(put_res2.status, 201);

    // Read at ts1 – should return v1
    auto get_req = makeKeyRequest("GET", "k", {},
        {{"timestamp", std::to_string(ts1)}});
    httplib::Response get_res;
    handler_->handleGetKey(get_req, get_res);

    ASSERT_EQ(get_res.status, 200);
    auto body = parseResponse(get_res);
    EXPECT_EQ(body["value"].get<std::string>(), "v1");
}

TEST_F(MvccApiHandlerTest, GetAtTimestamp_BeforeAnyVersion_Returns404) {
    auto put_req = makeKeyRequest("POST", "k", R"({"value":"v1"})");
    httplib::Response put_res;
    handler_->handlePutKey(put_req, put_res);
    ASSERT_EQ(put_res.status, 201);

    // Read at ts=0 (before any version)
    auto get_req = makeKeyRequest("GET", "k", {}, {{"timestamp", "0"}});
    httplib::Response get_res;
    handler_->handleGetKey(get_req, get_res);
    EXPECT_EQ(get_res.status, 404);
}

TEST_F(MvccApiHandlerTest, GetAtTimestamp_InvalidParam_Returns400) {
    auto req = makeKeyRequest("GET", "k", {}, {{"timestamp", "not_a_number"}});
    httplib::Response res;
    handler_->handleGetKey(req, res);
    EXPECT_EQ(res.status, 400);
}

// ─────────────────────────────────────────────────────────────────────────────
// Version history
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MvccApiHandlerTest, ListVersions_ReturnsAllVersions) {
    for (int i = 0; i < 3; ++i) {
        auto req = makeKeyRequest("POST", "k",
            fmt::format(R"({{"value":"v{}"}})", i));
        httplib::Response res;
        handler_->handlePutKey(req, res);
        ASSERT_EQ(res.status, 201);
    }

    auto list_req = makeVersionsRequest("GET", "k");
    httplib::Response list_res;
    handler_->handleListVersions(list_req, list_res);

    ASSERT_EQ(list_res.status, 200);
    auto body = parseResponse(list_res);
    EXPECT_EQ(body["key"].get<std::string>(), "k");
    EXPECT_EQ(body["versions"].size(), 3u);
}

TEST_F(MvccApiHandlerTest, ListVersions_EmptyKey_ReturnsEmptyArray) {
    auto req = makeVersionsRequest("GET", "nonexistent");
    httplib::Response res;
    handler_->handleListVersions(req, res);

    ASSERT_EQ(res.status, 200);
    auto body = parseResponse(res);
    EXPECT_TRUE(body["versions"].empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Garbage collection
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MvccApiHandlerTest, GcVersions_DeletesOldVersions) {
    auto put_req1 = makeKeyRequest("POST", "k", R"({"value":"old"})");
    httplib::Response put_res1;
    handler_->handlePutKey(put_req1, put_res1);
    ASSERT_EQ(put_res1.status, 201);
    uint64_t ts1 = parseResponse(put_res1)["timestamp"].get<uint64_t>();

    auto put_req2 = makeKeyRequest("POST", "k", R"({"value":"new"})");
    httplib::Response put_res2;
    handler_->handlePutKey(put_req2, put_res2);
    ASSERT_EQ(put_res2.status, 201);
    uint64_t ts2 = parseResponse(put_res2)["timestamp"].get<uint64_t>();

    // GC: delete versions before ts2
    std::string gc_body = fmt::format(
        R"({{"before_timestamp":{},"min_versions_to_keep":1}})", ts2);
    auto gc_req = makeVersionsRequest("DELETE", "k", gc_body);
    httplib::Response gc_res;
    handler_->handleGcVersions(gc_req, gc_res);

    ASSERT_EQ(gc_res.status, 200);
    auto gc_body_json = parseResponse(gc_res);
    EXPECT_EQ(gc_body_json["versions_deleted"].get<uint64_t>(), 1u);

    // Verify ts1 version is gone
    auto get_req = makeKeyRequest("GET", "k", {}, {{"timestamp", std::to_string(ts1)}});
    httplib::Response get_res;
    handler_->handleGetKey(get_req, get_res);
    EXPECT_EQ(get_res.status, 404);

    // Verify latest is still accessible
    auto latest_req = makeKeyRequest("GET", "k");
    httplib::Response latest_res;
    handler_->handleGetKey(latest_req, latest_res);
    ASSERT_EQ(latest_res.status, 200);
    EXPECT_EQ(parseResponse(latest_res)["value"].get<std::string>(), "new");
    (void)ts2;
}

TEST_F(MvccApiHandlerTest, GcVersions_MissingTimestamp_Returns400) {
    auto req = makeVersionsRequest("DELETE", "k", R"({"min_versions_to_keep":1})");
    httplib::Response res;
    handler_->handleGcVersions(req, res);
    EXPECT_EQ(res.status, 400);
}

TEST_F(MvccApiHandlerTest, GcVersions_InvalidKeyTraversal_Returns400) {
    auto req = makeVersionsRequest("DELETE", "../k", R"({"before_timestamp":1})");
    httplib::Response res;
    handler_->handleGcVersions(req, res);
    EXPECT_EQ(res.status, 400);
}

TEST_F(MvccApiHandlerTest, GcVersions_EmptyBody_Returns400) {
    auto req = makeVersionsRequest("DELETE", "k");
    httplib::Response res;
    handler_->handleGcVersions(req, res);
    EXPECT_EQ(res.status, 400);
}

// ─────────────────────────────────────────────────────────────────────────────
// Stats
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MvccApiHandlerTest, GetStats_ContainsExpectedFields) {
    // Perform some operations first
    auto put_req = makeKeyRequest("POST", "s", R"({"value":"val"})");
    httplib::Response put_res;
    handler_->handlePutKey(put_req, put_res);
    ASSERT_EQ(put_res.status, 201);

    auto get_req = makeKeyRequest("GET", "s");
    httplib::Response get_res;
    handler_->handleGetKey(get_req, get_res);
    ASSERT_EQ(get_res.status, 200);

    auto stats_req = makeRequest("GET", "/api/v1/mvcc/stats");
    httplib::Response stats_res;
    handler_->handleGetStats(stats_req, stats_res);

    ASSERT_EQ(stats_res.status, 200);
    auto body = parseResponse(stats_res);
    EXPECT_TRUE(body.contains("writes_total"));
    EXPECT_TRUE(body.contains("reads_latest_total"));
    EXPECT_TRUE(body.contains("reads_snapshot_total"));
    EXPECT_TRUE(body.contains("gc_runs_total"));
    EXPECT_TRUE(body.contains("gc_versions_deleted_total"));
    EXPECT_TRUE(body.contains("current_timestamp"));
    EXPECT_GE(body["writes_total"].get<uint64_t>(), 1u);
    EXPECT_GE(body["reads_latest_total"].get<uint64_t>(), 1u);
}
