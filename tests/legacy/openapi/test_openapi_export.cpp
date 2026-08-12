/**
 * @file test_openapi_export.cpp
 * @brief Tests for the OpenAPI 3.1 specification export endpoint
 *
 * Validates that MonitoringApiHandler::handleOpenApi() returns a valid
 * OpenAPI 3.1.0 document containing the required fields, paths, and schemas.
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <boost/beast/http.hpp>
#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include "server/monitoring_api_handler.h"
#include "server/openapi_route_registry.h"

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
    EXPECT_NO_THROW({
        auto parsed = json::parse(res_.body());
        static_cast<void>(parsed);
    });
}

// ---------------------------------------------------------------------------
// OpenAPI 3.1 Top-Level Fields
// ---------------------------------------------------------------------------

TEST_F(OpenApiExportTest, HasOpenApiVersion) {
    ASSERT_TRUE(body_.contains("openapi"));
    std::string v = body_["openapi"];
    EXPECT_EQ(v.substr(0, 1), "3"); // Major version 3
}

TEST_F(OpenApiExportTest, OpenApiVersionIs310) {
    EXPECT_EQ(body_["openapi"], "3.1.0");
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

// ---------------------------------------------------------------------------
// RouteRegistry – unit tests for annotation-based route registration
// ---------------------------------------------------------------------------

class RouteRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Start each test with a clean registry state.
        themis::server::RouteRegistry::instance().clear();
    }
    void TearDown() override {
        // Restore monitoring routes so other tests that rely on the registry
        // are unaffected.
        themis::server::MonitoringApiHandler::registerRoutes();
    }
};

TEST_F(RouteRegistryTest, EmptyRegistryProducesValidSpec) {
    auto spec = themis::server::RouteRegistry::instance().buildOpenApiSpec("0.0.1");
    ASSERT_TRUE(spec.contains("openapi"));
    EXPECT_EQ(spec["openapi"], "3.1.0");
    EXPECT_TRUE(spec.contains("paths"));
    EXPECT_TRUE(spec["paths"].is_object());
}

TEST_F(RouteRegistryTest, RegisteredRouteAppearsInPaths) {
    themis::server::RouteRegistry::instance().registerRoute({
        "/test/resource", "get",
        {"Get test resource", "", "getTestResource", {"test"}, {}, {},
         {{"200", {{"description", "OK"}}}}}
    });
    auto spec = themis::server::RouteRegistry::instance().buildOpenApiSpec("0.0.1");
    EXPECT_TRUE(spec["paths"].contains("/test/resource"));
    EXPECT_TRUE(spec["paths"]["/test/resource"].contains("get"));
}

TEST_F(RouteRegistryTest, OperationIdIsPreserved) {
    themis::server::RouteRegistry::instance().registerRoute({
        "/op/test", "post",
        {"Create op", "", "createOpTest", {"ops"}, {}, {},
         {{"201", {{"description", "Created"}}}}}
    });
    auto spec = themis::server::RouteRegistry::instance().buildOpenApiSpec("1.0.0");
    ASSERT_TRUE(spec["paths"].contains("/op/test"));
    EXPECT_EQ(spec["paths"]["/op/test"]["post"]["operationId"], "createOpTest");
}

TEST_F(RouteRegistryTest, TagsAreCollectedFromRegistrations) {
    themis::server::RouteRegistry::instance().registerRoute({
        "/tagged", "get",
        {"Tagged endpoint", "", "getTagged", {"custom-tag"}, {}, {},
         {{"200", {{"description", "OK"}}}}}
    });
    auto spec = themis::server::RouteRegistry::instance().buildOpenApiSpec("1.0.0");
    bool found = false;
    for (const auto& tag : spec["tags"]) {
        if (tag["name"] == "custom-tag") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST_F(RouteRegistryTest, PathParameterAppearsInParameters) {
    using themis::server::RouteParam;
    themis::server::RouteRegistry::instance().registerRoute({
        "/items/{id}", "get",
        {"Get item", "", "getItem", {"items"},
         {RouteParam{"id","path",true,"Item id",{{"type","string"}}}},
         {},
         {{"200", {{"description", "OK"}}}}}
    });
    auto spec = themis::server::RouteRegistry::instance().buildOpenApiSpec("1.0.0");
    ASSERT_TRUE(spec["paths"].contains("/items/{id}"));
    const auto& params = spec["paths"]["/items/{id}"]["get"]["parameters"];
    ASSERT_TRUE(params.is_array());
    ASSERT_FALSE(params.empty());
    EXPECT_EQ(params[0]["name"], "id");
    EXPECT_EQ(params[0]["in"], "path");
    EXPECT_TRUE(params[0]["required"].get<bool>());
}

TEST_F(RouteRegistryTest, LastRegistrationWinsForSamePathAndMethod) {
    themis::server::RouteRegistry::instance().registerRoute({
        "/dup", "get",
        {"First", "", "firstOp", {"t"}, {}, {}, {{"200", {{"description","v1"}}}}}
    });
    themis::server::RouteRegistry::instance().registerRoute({
        "/dup", "get",
        {"Second", "", "secondOp", {"t"}, {}, {}, {{"200", {{"description","v2"}}}}}
    });
    auto spec = themis::server::RouteRegistry::instance().buildOpenApiSpec("1.0.0");
    EXPECT_EQ(spec["paths"]["/dup"]["get"]["operationId"], "secondOp");
}

TEST_F(RouteRegistryTest, SpecVersionIsAlways310) {
    auto spec = themis::server::RouteRegistry::instance().buildOpenApiSpec("99.0.0");
    EXPECT_EQ(spec["openapi"], "3.1.0");
}

TEST_F(RouteRegistryTest, InfoVersionMatchesArgument) {
    auto spec = themis::server::RouteRegistry::instance().buildOpenApiSpec("2.5.3");
    EXPECT_EQ(spec["info"]["version"], "2.5.3");
}

TEST_F(RouteRegistryTest, ComponentsContainsBearerAuth) {
    auto spec = themis::server::RouteRegistry::instance().buildOpenApiSpec("0.0.1");
    ASSERT_TRUE(spec.contains("components"));
    ASSERT_TRUE(spec["components"].contains("securitySchemes"));
    ASSERT_TRUE(spec["components"]["securitySchemes"].contains("BearerAuth"));
    EXPECT_EQ(spec["components"]["securitySchemes"]["BearerAuth"]["scheme"], "bearer");
}

TEST_F(RouteRegistryTest, MonitoringRoutesRegisteredViaRegisterRoutes) {
    themis::server::MonitoringApiHandler::registerRoutes();
    const auto entries = themis::server::RouteRegistry::instance().entries();
    EXPECT_FALSE(entries.empty());
    bool has_health = false;
    for (const auto& e : entries) {
        if (e.path == "/health" && e.method == "get") { has_health = true; break; }
    }
    EXPECT_TRUE(has_health);
}

TEST_F(RouteRegistryTest, DeprecatedRouteMarkedInSpec) {
    themis::server::RouteRegistry::instance().registerRoute({
        "/old/endpoint", "get",
        {"Old endpoint", "", "getOld", {"legacy"}, {}, {},
         {{"200", {{"description", "OK"}}}},
         /*deprecated=*/true}
    });
    auto spec = themis::server::RouteRegistry::instance().buildOpenApiSpec("1.0.0");
    ASSERT_TRUE(spec["paths"].contains("/old/endpoint"));
    const auto& op = spec["paths"]["/old/endpoint"]["get"];
    ASSERT_TRUE(op.contains("deprecated"));
    EXPECT_TRUE(op["deprecated"].get<bool>());
}

TEST_F(RouteRegistryTest, QueryParameterAppearsInParameters) {
    using themis::server::RouteParam;
    themis::server::RouteRegistry::instance().registerRoute({
        "/search", "get",
        {"Search resources", "", "searchResources", {"search"},
         {RouteParam{"q", "query", false, "Search query string", {{"type","string"}}}},
         {},
         {{"200", {{"description", "OK"}}}}}
    });
    auto spec = themis::server::RouteRegistry::instance().buildOpenApiSpec("1.0.0");
    ASSERT_TRUE(spec["paths"].contains("/search"));
    const auto& params = spec["paths"]["/search"]["get"]["parameters"];
    ASSERT_TRUE(params.is_array());
    ASSERT_FALSE(params.empty());
    EXPECT_EQ(params[0]["name"], "q");
    EXPECT_EQ(params[0]["in"], "query");
    EXPECT_FALSE(params[0]["required"].get<bool>());
}
