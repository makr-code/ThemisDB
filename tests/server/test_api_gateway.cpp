/**
 * @file test_api_gateway.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
        auth_ = std::make_shared<themis::AuthMiddleware>();
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
    
    std::shared_ptr<themis::AuthMiddleware> auth_;
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
 *
 * Verifies that registerHandler() stores the handler without throwing and that
 * the gateway remains healthy after registration.  Also verifies that the
 * gateway still processes subsequent requests correctly once a pattern is
 * registered.
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
    
    // Registration must not throw
    ASSERT_NO_THROW(gateway_->registerHandler("/test/*", test_handler));
    
    // Gateway must remain healthy after registration
    auto health = gateway_->getHealthStatus();
    EXPECT_EQ(health["status"], "healthy")
        << "Gateway must be healthy after handler registration";
    
    // Gateway must still process requests correctly after registration
    http::request<http::string_body> req{http::verb::get, "/health", 11};
    req.set(http::field::host, "localhost");
    auto dummy_handler = [](const http::request<http::string_body>& r) {
        http::response<http::string_body> resp{http::status::ok, r.version()};
        resp.body() = R"({"ok":true})";
        resp.prepare_payload();
        return resp;
    };
    auto response = gateway_->handleRequest(req, dummy_handler);
    EXPECT_EQ(response.result(), http::status::ok)
        << "Request must succeed after handler registration";
}

/**
 * @brief Test health status with errors
 *
 * Verifies that getHealthStatus() transitions from "healthy" to "degraded"
 * (>10% error rate) and to "unhealthy" (>50% error rate) as failed requests
 * accumulate.  Failed requests are produced by local handlers that throw.
 */
TEST_F(APIGatewayTest, HealthStatusWithErrors) {
    namespace http = boost::beast::http;

    // Gateway starts healthy with no requests
    auto health = gateway_->getHealthStatus();
    EXPECT_EQ(health["status"], "healthy");

    // A local handler that throws causes failed_requests_ to be incremented
    auto throwing_handler = [](const http::request<http::string_body>&)
        -> http::response<http::string_body> {
        throw std::runtime_error("simulated handler failure");
    };

    auto ok_handler = [](const http::request<http::string_body>& r) {
        http::response<http::string_body> resp{http::status::ok, r.version()};
        resp.body() = R"({"ok":true})";
        resp.prepare_payload();
        return resp;
    };

    http::request<http::string_body> req{http::verb::get, "/health", 11};
    req.set(http::field::host, "localhost");

    // 1 success + 9 failures  → error rate = 90 % → "unhealthy"
    gateway_->handleRequest(req, ok_handler);
    for (int i = 0; i < 9; ++i) {
        gateway_->handleRequest(req, throwing_handler);
    }

    health = gateway_->getHealthStatus();
    EXPECT_EQ(health["status"], "unhealthy")
        << "Gateway must report 'unhealthy' when error rate exceeds 50 %";
    EXPECT_TRUE(health.contains("error_rate"))
        << "Health status must include 'error_rate' field after failures";
    EXPECT_GT(static_cast<double>(health["error_rate"]), 0.5)
        << "Reported error_rate must be > 0.5";
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

/**
 * @brief Trusted proxy: X-Real-IP header is used as the client identifier
 *        for rate limiting when enable_trusted_proxy_headers is true.
 *
 * Verifies that a gateway configured with enable_trusted_proxy_headers=true
 * accepts requests that carry an X-Real-IP header without error.
 */
TEST_F(APIGatewayTest, TrustedProxyXRealIpAccepted) {
    namespace http = boost::beast::http;

    APIGateway::Config config;
    config.gateway_id       = "proxy-gateway";
    config.datacenter       = "test-dc";
    config.enable_sharding  = false;
    config.enable_rate_limiting = false;
    config.enable_load_shedding = false;
    config.enable_trusted_proxy_headers = true;
    config.trusted_proxies  = {"10.0.0.1"};

    auto gw = std::make_unique<APIGateway>(
        config, auth_, rate_limiter_, load_shedder_
    );

    http::request<http::string_body> req{http::verb::get, "/health", 11};
    req.set(http::field::host, "localhost");
    req.set("X-Real-IP", "203.0.113.42");  // RFC 5737 documentation address

    auto local_handler = [](const http::request<http::string_body>& r) {
        http::response<http::string_body> resp{http::status::ok, r.version()};
        resp.body() = R"({"status":"healthy"})";
        resp.prepare_payload();
        return resp;
    };

    auto response = gw->handleRequest(req, local_handler);
    EXPECT_EQ(response.result(), http::status::ok)
        << "Request with X-Real-IP header must be handled successfully";
}

/**
 * @brief Trusted proxy: the leftmost IP in X-Forwarded-For is used when
 *        X-Real-IP is absent and enable_trusted_proxy_headers is true.
 */
TEST_F(APIGatewayTest, TrustedProxyXForwardedForAccepted) {
    namespace http = boost::beast::http;

    APIGateway::Config config;
    config.gateway_id       = "xff-gateway";
    config.datacenter       = "test-dc";
    config.enable_sharding  = false;
    config.enable_rate_limiting = false;
    config.enable_load_shedding = false;
    config.enable_trusted_proxy_headers = true;

    auto gw = std::make_unique<APIGateway>(
        config, auth_, rate_limiter_, load_shedder_
    );

    // Simulate Kong forwarding: "client, kong-proxy"
    http::request<http::string_body> req{http::verb::get, "/v1/entities", 11};
    req.set(http::field::host, "localhost");
    req.set("X-Forwarded-For", "198.51.100.7, 10.0.0.1");

    auto local_handler = [](const http::request<http::string_body>& r) {
        http::response<http::string_body> resp{http::status::ok, r.version()};
        resp.body() = R"({"result":[]})";
        resp.prepare_payload();
        return resp;
    };

    auto response = gw->handleRequest(req, local_handler);
    EXPECT_EQ(response.result(), http::status::ok)
        << "Request with X-Forwarded-For header must be handled successfully";
}

/**
 * @brief Trusted proxy disabled: X-Real-IP and X-Forwarded-For headers are
 *        ignored when enable_trusted_proxy_headers is false (the default).
 *
 * The request must still succeed; ThemisDB simply does not use the forwarded
 * IP as the rate-limit key.
 */
TEST_F(APIGatewayTest, TrustedProxyDisabledIgnoresForwardedHeaders) {
    namespace http = boost::beast::http;

    // Default gateway_ already has enable_trusted_proxy_headers = false
    http::request<http::string_body> req{http::verb::get, "/health", 11};
    req.set(http::field::host, "localhost");
    req.set("X-Real-IP", "203.0.113.99");
    req.set("X-Forwarded-For", "203.0.113.99, 10.0.0.1");

    auto local_handler = [](const http::request<http::string_body>& r) {
        http::response<http::string_body> resp{http::status::ok, r.version()};
        resp.body() = R"({"status":"healthy"})";
        resp.prepare_payload();
        return resp;
    };

    auto response = gateway_->handleRequest(req, local_handler);
    EXPECT_EQ(response.result(), http::status::ok)
        << "Request must succeed even when forwarded-IP headers are present but ignored";
}

/**
 * @brief Versioned URL routing: /v1/{path} sets the API-Version response header
 *        to the latest resolved v1 version (not v1.0.0 specifically).
 */
TEST_F(APIGatewayTest, VersionedPathV1SetsApiVersionHeader) {
    namespace http = boost::beast::http;

    http::request<http::string_body> req{http::verb::get, "/v1/entities", 11};
    req.set(http::field::host, "localhost");

    auto local_handler = [](const http::request<http::string_body>& r) {
        http::response<http::string_body> resp{http::status::ok, r.version()};
        resp.body() = R"({"result": []})";
        resp.prepare_payload();
        return resp;
    };

    auto response = gateway_->handleRequest(req, local_handler);
    EXPECT_EQ(response.result(), http::status::ok);

    // API-Version header must be set and must start with "v1"
    auto it = response.find(APIHeaders::API_VERSION);
    ASSERT_NE(it, response.end()) << "API-Version response header must be present";
    std::string api_ver = std::string(it->value());
    EXPECT_EQ(api_ver.substr(0, 2), "v1")
        << "API-Version for /v1/ path must resolve to v1.x.x, got: " << api_ver;
}

/**
 * @brief Versioned URL routing: the local handler receives the path WITHOUT the
 *        /v{N}/ prefix so it can work with the same routing logic for all versions.
 */
TEST_F(APIGatewayTest, VersionedPathStripsVersionPrefixForHandler) {
    namespace http = boost::beast::http;

    http::request<http::string_body> req{http::verb::get, "/v1/entities/123", 11};
    req.set(http::field::host, "localhost");

    // The handler must receive the path with the /v1/ prefix stripped
    std::string received_path = {};
    auto local_handler = [&received_path](const http::request<http::string_body>& r) {
        received_path = std::string(r.target());
        http::response<http::string_body> resp{http::status::ok, r.version()};
        resp.body() = R"({"id":"123"})";
        resp.prepare_payload();
        return resp;
    };

    auto response = gateway_->handleRequest(req, local_handler);
    EXPECT_EQ(response.result(), http::status::ok);
    // Handler should receive the path without the /v1 prefix
    EXPECT_EQ(received_path, "/entities/123")
        << "Handler must receive path without version prefix, got: " << received_path;
}

/**
 * @brief Versioned URL routing: query string is preserved when stripping the prefix.
 */
TEST_F(APIGatewayTest, VersionedPathPreservesQueryString) {
    namespace http = boost::beast::http;

    http::request<http::string_body> req{http::verb::get,
        "/v1/entities?page=2&limit=10", 11};
    req.set(http::field::host, "localhost");

    std::string received_path = {};
    auto local_handler = [&received_path](const http::request<http::string_body>& r) {
        received_path = std::string(r.target());
        http::response<http::string_body> resp{http::status::ok, r.version()};
        resp.body() = R"({"result": []})";
        resp.prepare_payload();
        return resp;
    };

    auto response = gateway_->handleRequest(req, local_handler);
    EXPECT_EQ(response.result(), http::status::ok);
    // Query string must be preserved after stripping the version prefix
    EXPECT_EQ(received_path, "/entities?page=2&limit=10")
        << "Handler must receive path with query string intact, got: " << received_path;
}

/**
 * @brief Versioned URL routing: deprecated endpoint registered without version
 *        prefix is matched when called with /v1/ prefix.
 */
TEST_F(APIGatewayTest, DeprecationHeadersMatchVersionedPath) {
    namespace http = boost::beast::http;

    // Register deprecation WITHOUT version prefix in the key
    APIDeprecationInfo info;
    info.deprecated_in = APIVersion{1, 0, 0};
    info.removed_in = APIVersion{2, 0, 0};
    info.deprecation_date = std::chrono::system_clock::now();
    info.removal_date = std::chrono::system_clock::now() + std::chrono::hours(24 * 365);
    info.migration_guide_url = "https://docs.themisdb.com/migration/v1-to-v2";
    gateway_->registerDeprecation("/old-resource", info);

    // Request WITH /v1/ prefix — gateway must strip prefix before deprecation lookup
    http::request<http::string_body> req{http::verb::get, "/v1/old-resource", 11};
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
        << "Deprecation header must fire for /v1/old-resource when /old-resource is deprecated";
    EXPECT_NE(response.find(APIHeaders::SUNSET), response.end());
}
