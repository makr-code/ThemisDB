/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_api_gateway.cpp                               ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     204                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Re-enabled API Gateway tests
#include <gtest/gtest.h>

#include "server/api_gateway.h"
#include "server/auth_middleware.h"
#include "server/rate_limiter.h"
#include "server/load_shedder.h"
#include <chrono>
#include <memory>

using namespace themis::server;

/**
 * @brief Test fixture for API Gateway tests
 */
class APIGatewayTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create minimal dependencies
        auth_ = std::make_shared<AuthMiddleware>();
        rate_limiter_ = std::make_shared<RateLimiter>();
        load_shedder_ = std::make_shared<LoadShedder>(LoadShedder::Config{});
        
        // Create gateway with default config (disable rate/load checks for tests)
        APIGateway::Config config;
        config.gateway_id = "test-gateway";
        config.datacenter = "test-dc";
        config.enable_sharding = false;
        config.enable_query_federation = false;
        config.enable_rate_limiting = false;
        config.enable_load_shedding = false;
        
        gateway_ = std::make_unique<APIGateway>(
            config,
            auth_,
            rate_limiter_,
            load_shedder_
        );
    }
    
    void TearDown() override {
        gateway_.reset();
    }
    
    std::shared_ptr<AuthMiddleware> auth_;
    std::shared_ptr<RateLimiter> rate_limiter_;
    std::shared_ptr<LoadShedder> load_shedder_;
    std::unique_ptr<APIGateway> gateway_;
};

/**
 * @brief Test basic gateway initialization
 */
TEST_F(APIGatewayTest, BasicInitialization) {
    ASSERT_NE(gateway_, nullptr);
    
    // Check health status
    auto health = gateway_->getHealthStatus();
    EXPECT_EQ(health["status"], "healthy");
    EXPECT_EQ(health["gateway_id"], "test-gateway");
}

/**
 * @brief Test gateway statistics
 */
TEST_F(APIGatewayTest, GetStatistics) {
    auto stats = gateway_->getStatistics();
    
    EXPECT_EQ(stats["gateway_id"], "test-gateway");
    EXPECT_EQ(stats["datacenter"], "test-dc");
    EXPECT_TRUE(stats.contains("requests"));
    EXPECT_TRUE(stats.contains("routing"));
    EXPECT_TRUE(stats.contains("features"));
    
    // Initial statistics should be zero
    EXPECT_EQ(stats["requests"]["total"], 0);
    EXPECT_EQ(stats["routing"]["local"], 0);
}

/**
 * @brief Test local request handling
 */
TEST_F(APIGatewayTest, HandleLocalRequest) {
    namespace http = boost::beast::http;
    
    // Create a test request
    http::request<http::string_body> req{http::verb::get, "/health", 11};
    req.set(http::field::host, "localhost");
    req.set(http::field::content_type, "application/json");
    
    // Create a simple local handler
    auto local_handler = [](const http::request<http::string_body>& r) {
        http::response<http::string_body> resp{http::status::ok, r.version()};
        resp.set(http::field::content_type, "application/json");
        resp.body() = R"({"status": "healthy"})";
        resp.prepare_payload();
        return resp;
    };
    
    // Handle the request
    auto response = gateway_->handleRequest(req, local_handler);
    
    // Verify response
    EXPECT_EQ(response.result(), http::status::ok);
    EXPECT_TRUE(response.body().find("healthy") != std::string::npos);
    
    // Check statistics were updated
    auto stats = gateway_->getStatistics();
    EXPECT_GT(stats["requests"]["total"], 0);
}

/**
 * @brief Test config update
 */
TEST_F(APIGatewayTest, UpdateConfig) {
    APIGateway::Config new_config;
    new_config.gateway_id = "updated-gateway";
    new_config.datacenter = "updated-dc";
    
    gateway_->updateConfig(new_config);
    
    auto stats = gateway_->getStatistics();
    EXPECT_EQ(stats["gateway_id"], "updated-gateway");
    EXPECT_EQ(stats["datacenter"], "updated-dc");
}

/**
 * @brief Test query federation (without shard router)
 */
TEST_F(APIGatewayTest, FederatedQueryWithoutRouter) {
    AuthContext auth_ctx;
    auth_ctx.user_id = "test-user";
    
    // Should throw error because query federation is not enabled
    EXPECT_THROW(
        gateway_->executeFederatedQuery("FOR doc IN collection RETURN doc", auth_ctx),
        std::exception
    );
}

/**
 * @brief Test handler registration
 */
TEST_F(APIGatewayTest, RegisterHandler) {
    namespace http = boost::beast::http;
    
    bool handler_called = false;
    
    auto test_handler = [&handler_called](const http::request<http::string_body>& r) {
        handler_called = true;
        http::response<http::string_body> resp{http::status::ok, r.version()};
        resp.body() = "test";
        resp.prepare_payload();
        return resp;
    };
    
    gateway_->registerHandler("/test/*", test_handler);
    
    // Verify handler was registered (implementation detail)
    // Note: This test assumes the handler is actually used when matching
}

/**
 * @brief Test health status with errors
 */
TEST_F(APIGatewayTest, HealthStatusWithErrors) {
    // Gateway starts healthy
    auto health = gateway_->getHealthStatus();
    EXPECT_EQ(health["status"], "healthy");
    
    // After handling requests, health should still be good if no errors
    // (detailed testing would require mocking request failures)
}

/**
 * @brief Test that no deprecation headers appear for non-deprecated endpoints
 */
TEST_F(APIGatewayTest, NoDeprecationHeaderForFreshEndpoint) {
    namespace http = boost::beast::http;

    http::request<http::string_body> req{http::verb::get, "/api/v1/entities", 11};
    req.set(http::field::host, "localhost");

    auto local_handler = [](const http::request<http::string_body>& r) {
        http::response<http::string_body> resp{http::status::ok, r.version()};
        resp.body() = R"({"result": []})";
        resp.prepare_payload();
        return resp;
    };

    auto response = gateway_->handleRequest(req, local_handler);
    EXPECT_EQ(response.result(), http::status::ok);
    // API-Version header must be present (versioning is enabled by default)
    EXPECT_NE(response.find(APIHeaders::API_VERSION), response.end());
    // No deprecation header for a non-deprecated endpoint
    EXPECT_EQ(response.find(APIHeaders::DEPRECATION_WARNING), response.end());
    EXPECT_EQ(response.find(APIHeaders::SUNSET), response.end());
}

/**
 * @brief Test that Deprecation and Sunset headers appear for deprecated endpoints
 */
TEST_F(APIGatewayTest, DeprecationHeadersOnDeprecatedEndpoint) {
    namespace http = boost::beast::http;

    // Register a deprecation for this endpoint
    APIDeprecationInfo info;
    info.deprecated_in = APIVersion{1, 0, 0};
    info.removed_in = APIVersion{2, 0, 0};
    info.deprecation_date = std::chrono::system_clock::now();
    info.removal_date = std::chrono::system_clock::now() + std::chrono::hours(24 * 365); // ~1 year
    info.reason = "Replaced by /api/v2/entities";
    info.migration_guide_url = "https://docs.themisdb.com/migration/v1-to-v2";
    info.alternative = "/api/v2/entities";
    gateway_->registerDeprecation("/api/v1/old-endpoint", info);

    http::request<http::string_body> req{http::verb::get, "/api/v1/old-endpoint", 11};
    req.set(http::field::host, "localhost");

    auto local_handler = [](const http::request<http::string_body>& r) {
        http::response<http::string_body> resp{http::status::ok, r.version()};
        resp.body() = R"({"result": []})";
        resp.prepare_payload();
        return resp;
    };

    auto response = gateway_->handleRequest(req, local_handler);
    EXPECT_EQ(response.result(), http::status::ok);
    // Deprecation header must be present
    EXPECT_NE(response.find(APIHeaders::DEPRECATION_WARNING), response.end());
    // Sunset header (RFC 8594) must be present
    EXPECT_NE(response.find(APIHeaders::SUNSET), response.end());
    // Link header pointing to migration guide must be present
    EXPECT_NE(response.find(APIHeaders::LINK), response.end());
}

/**
 * @brief Test that entity paths without a shard router fall back to local execution
 */
TEST_F(APIGatewayTest, ShardRouteWithoutRouterFallsBackToLocal) {
    namespace http = boost::beast::http;

    // Enable sharding but provide no shard router (gateway constructed without one)
    APIGateway::Config config;
    config.gateway_id = "shard-fallback-gateway";
    config.datacenter = "test-dc";
    config.enable_sharding = true;
    config.enable_query_federation = false;
    config.enable_rate_limiting = false;
    config.enable_load_shedding = false;

    auto gw = std::make_unique<APIGateway>(
        config, auth_, rate_limiter_, load_shedder_
        // shard_router = nullptr (default)
    );

    http::request<http::string_body> req{http::verb::get,
        "/entities/urn:themis:relational:ns:users:550e8400-e29b-41d4-a716-446655440000", 11};
    req.set(http::field::host, "localhost");

    bool local_called = false;
    auto local_handler = [&local_called](const http::request<http::string_body>& r) {
        local_called = true;
        http::response<http::string_body> resp{http::status::ok, r.version()};
        resp.body() = R"({"id":"fallback"})";
        resp.prepare_payload();
        return resp;
    };

    auto response = gw->handleRequest(req, local_handler);
    // Without a shard router the gateway must fall back to local execution
    EXPECT_EQ(response.result(), http::status::ok);
    EXPECT_TRUE(local_called);
}

/**
 * @brief Test that non-URN entity paths fall back to local execution
 */
TEST_F(APIGatewayTest, ShardRouteWithInvalidUrnFallsBackToLocal) {
    namespace http = boost::beast::http;

    APIGateway::Config config;
    config.gateway_id = "shard-invalid-urn-gateway";
    config.datacenter = "test-dc";
    config.enable_sharding = true;
    config.enable_rate_limiting = false;
    config.enable_load_shedding = false;

    auto gw = std::make_unique<APIGateway>(
        config, auth_, rate_limiter_, load_shedder_
    );

    // Path with /entities/ but no valid URN
    http::request<http::string_body> req{http::verb::get,
        "/entities/not-a-urn", 11};
    req.set(http::field::host, "localhost");

    bool local_called = false;
    auto local_handler = [&local_called](const http::request<http::string_body>& r) {
        local_called = true;
        http::response<http::string_body> resp{http::status::ok, r.version()};
        resp.body() = R"({"fallback":true})";
        resp.prepare_payload();
        return resp;
    };

    auto response = gw->handleRequest(req, local_handler);
    EXPECT_EQ(response.result(), http::status::ok);
    EXPECT_TRUE(local_called);
}


/**
 * @brief Regression test: deprecation headers must fire even when the request
 *        contains a query string (e.g. /api/v1/old-endpoint?page=1).
 *
 * Bug: addDeprecationHeaders() used req.target() directly without stripping '?...',
 * so the deprecation registry lookup returned nullopt for requests with query params.
 */
TEST_F(APIGatewayTest, DeprecationHeadersWithQueryString) {
    namespace http = boost::beast::http;

    APIDeprecationInfo info;
    info.deprecated_in = APIVersion{1, 0, 0};
    info.removed_in = APIVersion{2, 0, 0};
    info.deprecation_date = std::chrono::system_clock::now();
    info.removal_date = std::chrono::system_clock::now() + std::chrono::hours(24 * 365);
    info.migration_guide_url = "https://docs.themisdb.com/migration/v1-to-v2";
    gateway_->registerDeprecation("/api/v1/old-endpoint", info);

    // Request with query parameters — must still match the registered path
    http::request<http::string_body> req{http::verb::get,
        "/api/v1/old-endpoint?page=2&limit=50", 11};
    req.set(http::field::host, "localhost");

    auto local_handler = [](const http::request<http::string_body>& r) {
        http::response<http::string_body> resp{http::status::ok, r.version()};
        resp.body() = R"({"result": []})";
        resp.prepare_payload();
        return resp;
    };

    auto response = gateway_->handleRequest(req, local_handler);
    EXPECT_EQ(response.result(), http::status::ok);
    EXPECT_NE(response.find(APIHeaders::DEPRECATION_WARNING), response.end())
        << "Deprecation header must be present even when path has query parameters";
    EXPECT_NE(response.find(APIHeaders::SUNSET), response.end())
        << "Sunset header must be present even when path has query parameters";
}
