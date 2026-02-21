/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_openapi_export.cpp                            ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:46:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     328                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 6ecc84977  2026-02-20  Server Module: Production Hardening (TLS hot-reload, grac... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_openapi_export.cpp
 * @brief Tests for the OpenAPI 3.0 specification export endpoint
 *
 * Validates that MonitoringApiHandler::handleOpenApi() returns a valid
 * OpenAPI 3.0.3 document containing the required fields, paths, and schemas.
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <boost/beast/http.hpp>
#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include "server/monitoring_api_handler.h"

namespace http = boost::beast::http;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static http::request<http::string_body> make_get(const std::string& target) {
    http::request<http::string_body> req{http::verb::get, target, 11};
    req.set(http::field::host, "localhost");
    req.prepare_payload();
    return req;
}

static std::unique_ptr<themis::server::MonitoringApiHandler> make_handler() {
    static std::atomic<uint64_t> req_count{0};
    static std::atomic<uint64_t> err_count{0};
    static std::atomic<bool>     running{true};
    static std::atomic<uint64_t> active_reqs{0};
    static std::atomic<uint64_t> active_conns{0};
    static auto start = std::chrono::steady_clock::now();

    return std::make_unique<themis::server::MonitoringApiHandler>(
        nullptr, nullptr,
        &req_count, &err_count, &start,
        nullptr, nullptr, nullptr,
        &running, &active_reqs, &active_conns
    );
}

// ---------------------------------------------------------------------------
// HTTP Response Tests
// ---------------------------------------------------------------------------

class OpenApiExportTest : public ::testing::Test {
protected:
    void SetUp() override {
        handler_ = make_handler();
        req_ = make_get("/api/openapi.json");
        res_ = handler_->handleOpenApi(req_);
        body_ = json::parse(res_.body());
    }

    std::unique_ptr<themis::server::MonitoringApiHandler> handler_;
    http::request<http::string_body> req_;
    http::response<http::string_body> res_;
    json body_;
};

TEST_F(OpenApiExportTest, Returns200) {
    EXPECT_EQ(res_.result(), http::status::ok);
}

TEST_F(OpenApiExportTest, ContentTypeIsJson) {
    EXPECT_EQ(res_[http::field::content_type], "application/json");
}

TEST_F(OpenApiExportTest, BodyIsValidJson) {
    EXPECT_NO_THROW(json::parse(res_.body()));
}

// ---------------------------------------------------------------------------
// OpenAPI 3.0 Top-Level Fields
// ---------------------------------------------------------------------------

TEST_F(OpenApiExportTest, HasOpenApiVersion) {
    ASSERT_TRUE(body_.contains("openapi"));
    std::string v = body_["openapi"];
    EXPECT_EQ(v.substr(0, 1), "3"); // Major version 3
}

TEST_F(OpenApiExportTest, OpenApiVersionIs303) {
    EXPECT_EQ(body_["openapi"], "3.0.3");
}

TEST_F(OpenApiExportTest, HasInfoObject) {
    ASSERT_TRUE(body_.contains("info"));
    EXPECT_TRUE(body_["info"].is_object());
}

TEST_F(OpenApiExportTest, InfoHasTitle) {
    ASSERT_TRUE(body_["info"].contains("title"));
    EXPECT_FALSE(body_["info"]["title"].get<std::string>().empty());
}

TEST_F(OpenApiExportTest, InfoHasVersion) {
    ASSERT_TRUE(body_["info"].contains("version"));
    EXPECT_FALSE(body_["info"]["version"].get<std::string>().empty());
}

TEST_F(OpenApiExportTest, HasPathsObject) {
    ASSERT_TRUE(body_.contains("paths"));
    EXPECT_TRUE(body_["paths"].is_object());
    EXPECT_FALSE(body_["paths"].empty());
}

TEST_F(OpenApiExportTest, HasServersArray) {
    ASSERT_TRUE(body_.contains("servers"));
    EXPECT_TRUE(body_["servers"].is_array());
    EXPECT_FALSE(body_["servers"].empty());
}

// ---------------------------------------------------------------------------
// Required Paths
// ---------------------------------------------------------------------------

TEST_F(OpenApiExportTest, PathsContainsHealthEndpoint) {
    EXPECT_TRUE(body_["paths"].contains("/health"));
}

TEST_F(OpenApiExportTest, PathsContainsHealthLiveEndpoint) {
    EXPECT_TRUE(body_["paths"].contains("/health/live"));
}

TEST_F(OpenApiExportTest, PathsContainsHealthReadyEndpoint) {
    EXPECT_TRUE(body_["paths"].contains("/health/ready"));
}

TEST_F(OpenApiExportTest, PathsContainsVersionEndpoint) {
    EXPECT_TRUE(body_["paths"].contains("/version"));
}

TEST_F(OpenApiExportTest, PathsContainsMetricsEndpoint) {
    EXPECT_TRUE(body_["paths"].contains("/metrics"));
}

TEST_F(OpenApiExportTest, PathsContainsStatsEndpoint) {
    EXPECT_TRUE(body_["paths"].contains("/stats"));
}

TEST_F(OpenApiExportTest, PathsContainsEntitiesEndpoint) {
    EXPECT_TRUE(body_["paths"].contains("/entities"));
}

TEST_F(OpenApiExportTest, PathsContainsEntityByKeyEndpoint) {
    EXPECT_TRUE(body_["paths"].contains("/entities/{key}"));
}

TEST_F(OpenApiExportTest, PathsContainsQueryEndpoint) {
    EXPECT_TRUE(body_["paths"].contains("/query"));
}

TEST_F(OpenApiExportTest, PathsContainsAqlQueryEndpoint) {
    EXPECT_TRUE(body_["paths"].contains("/query/aql"));
}

TEST_F(OpenApiExportTest, PathsContainsOpenApiSelfReference) {
    EXPECT_TRUE(body_["paths"].contains("/api/openapi.json"));
}

// ---------------------------------------------------------------------------
// HTTP Methods on Paths
// ---------------------------------------------------------------------------

TEST_F(OpenApiExportTest, HealthEndpointHasGetMethod) {
    EXPECT_TRUE(body_["paths"]["/health"].contains("get"));
}

TEST_F(OpenApiExportTest, EntitiesEndpointHasGetMethod) {
    EXPECT_TRUE(body_["paths"]["/entities"].contains("get"));
}

TEST_F(OpenApiExportTest, EntitiesEndpointHasPostMethod) {
    EXPECT_TRUE(body_["paths"]["/entities"].contains("post"));
}

TEST_F(OpenApiExportTest, EntityByKeyHasGetMethod) {
    EXPECT_TRUE(body_["paths"]["/entities/{key}"].contains("get"));
}

TEST_F(OpenApiExportTest, EntityByKeyHasPutMethod) {
    EXPECT_TRUE(body_["paths"]["/entities/{key}"].contains("put"));
}

TEST_F(OpenApiExportTest, EntityByKeyHasDeleteMethod) {
    EXPECT_TRUE(body_["paths"]["/entities/{key}"].contains("delete"));
}

// ---------------------------------------------------------------------------
// Operation Fields
// ---------------------------------------------------------------------------

TEST_F(OpenApiExportTest, HealthGetHasOperationId) {
    EXPECT_TRUE(body_["paths"]["/health"]["get"].contains("operationId"));
}

TEST_F(OpenApiExportTest, HealthGetHasTags) {
    ASSERT_TRUE(body_["paths"]["/health"]["get"].contains("tags"));
    EXPECT_FALSE(body_["paths"]["/health"]["get"]["tags"].empty());
}

TEST_F(OpenApiExportTest, HealthGetHasResponses) {
    EXPECT_TRUE(body_["paths"]["/health"]["get"].contains("responses"));
}

TEST_F(OpenApiExportTest, EntityPostHasRequestBody) {
    EXPECT_TRUE(body_["paths"]["/entities"]["post"].contains("requestBody"));
}

// ---------------------------------------------------------------------------
// Components / Schemas
// ---------------------------------------------------------------------------

TEST_F(OpenApiExportTest, HasComponentsObject) {
    ASSERT_TRUE(body_.contains("components"));
    EXPECT_TRUE(body_["components"].is_object());
}

TEST_F(OpenApiExportTest, ComponentsHasSchemas) {
    ASSERT_TRUE(body_["components"].contains("schemas"));
}

TEST_F(OpenApiExportTest, SchemasContainsError) {
    EXPECT_TRUE(body_["components"]["schemas"].contains("Error"));
}

TEST_F(OpenApiExportTest, SchemasContainsHealthStatus) {
    EXPECT_TRUE(body_["components"]["schemas"].contains("HealthStatus"));
}

TEST_F(OpenApiExportTest, SchemasContainsReadinessStatus) {
    EXPECT_TRUE(body_["components"]["schemas"].contains("ReadinessStatus"));
}

TEST_F(OpenApiExportTest, ReadinessStatusHasChecksWithConnectionFields) {
    const auto& rs = body_["components"]["schemas"]["ReadinessStatus"];
    ASSERT_TRUE(rs.contains("properties"));
    const auto& checks = rs["properties"]["checks"];
    ASSERT_TRUE(checks.contains("properties"));
    EXPECT_TRUE(checks["properties"].contains("active_connections"));
    EXPECT_TRUE(checks["properties"].contains("active_requests"));
    EXPECT_TRUE(checks["properties"].contains("memory_rss_bytes"));
}

// ---------------------------------------------------------------------------
// Security Schemes
// ---------------------------------------------------------------------------

TEST_F(OpenApiExportTest, ComponentsHasSecuritySchemes) {
    ASSERT_TRUE(body_["components"].contains("securitySchemes"));
}

TEST_F(OpenApiExportTest, HasBearerAuthScheme) {
    ASSERT_TRUE(body_["components"]["securitySchemes"].contains("BearerAuth"));
    const auto& auth = body_["components"]["securitySchemes"]["BearerAuth"];
    EXPECT_EQ(auth["type"], "http");
    EXPECT_EQ(auth["scheme"], "bearer");
}

// ---------------------------------------------------------------------------
// Tags
// ---------------------------------------------------------------------------

TEST_F(OpenApiExportTest, HasTagsArray) {
    ASSERT_TRUE(body_.contains("tags"));
    EXPECT_TRUE(body_["tags"].is_array());
    EXPECT_FALSE(body_["tags"].empty());
}

TEST_F(OpenApiExportTest, TagsContainMonitoring) {
    bool found = false;
    for (const auto& tag : body_["tags"]) {
        if (tag["name"] == "monitoring") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST_F(OpenApiExportTest, TagsContainEntities) {
    bool found = false;
    for (const auto& tag : body_["tags"]) {
        if (tag["name"] == "entities") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

// ---------------------------------------------------------------------------
// Idempotency
// ---------------------------------------------------------------------------

TEST_F(OpenApiExportTest, CalledTwiceReturnsSameSpec) {
    auto req2 = make_get("/api/openapi.json");
    auto res2 = handler_->handleOpenApi(req2);

    EXPECT_EQ(res_.result(), res2.result());
    EXPECT_EQ(res_.body(), res2.body());
}
