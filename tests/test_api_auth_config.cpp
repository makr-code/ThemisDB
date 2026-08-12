#include <gtest/gtest.h>
#include "server/api_auth_config.h"

using namespace themis::server;

class ApiAuthConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

TEST_F(ApiAuthConfigTest, SecureDefaultsHaveAuthEnabled) {
    auto config = ApiAuthConfig::createSecureDefaults();
    
    EXPECT_TRUE(config.auth_enabled);
    EXPECT_TRUE(config.rate_limiting_enabled);
    EXPECT_EQ(config.global_rate_limit_per_minute, 100);
    EXPECT_EQ(config.global_rate_limit_burst, 100);
    EXPECT_EQ(config.audit_rate_limit_per_minute, 50);
}

TEST_F(ApiAuthConfigTest, DevDefaultsHaveAuthDisabled) {
    auto config = ApiAuthConfig::createDevDefaults();
    
    EXPECT_FALSE(config.auth_enabled);
    EXPECT_TRUE(config.rate_limiting_enabled);
    EXPECT_EQ(config.global_rate_limit_per_minute, 10000);
    EXPECT_EQ(config.global_rate_limit_burst, 1000);
}

TEST_F(ApiAuthConfigTest, SecureDefaultsIncludeEndpointConfigs) {
    auto config = ApiAuthConfig::createSecureDefaults();
    
    EXPECT_FALSE(config.endpoint_configs.empty());
    
    // Check that critical endpoints are configured
    bool found_entities_get = false;
    bool found_entities_post = false;
    bool found_query = false;
    bool found_admin = false;
    bool found_audit = false;
    bool found_health = false;
    bool found_config_get = false;
    bool found_config_post = false;
    
    for (const auto& endpoint : config.endpoint_configs) {
        if (endpoint.endpoint_pattern == "/entities/*" && endpoint.http_method == "GET") {
            found_entities_get = true;
            EXPECT_EQ(endpoint.required_scope, "data:read");
            EXPECT_TRUE(endpoint.auth_required);
            EXPECT_EQ(endpoint.rate_limit_per_minute, 1000);
        }
        else if (endpoint.endpoint_pattern == "/entities/*" && endpoint.http_method == "POST") {
            found_entities_post = true;
            EXPECT_EQ(endpoint.required_scope, "data:write");
            EXPECT_TRUE(endpoint.auth_required);
            EXPECT_EQ(endpoint.rate_limit_per_minute, 500);
        }
        else if (endpoint.endpoint_pattern == "/query") {
            found_query = true;
            EXPECT_EQ(endpoint.required_scope, "data:read");
            EXPECT_TRUE(endpoint.auth_required);
            EXPECT_EQ(endpoint.rate_limit_per_minute, 500);
        }
        else if (endpoint.endpoint_pattern == "/admin/*") {
            found_admin = true;
            EXPECT_EQ(endpoint.required_scope, "admin");
            EXPECT_TRUE(endpoint.auth_required);
            EXPECT_EQ(endpoint.rate_limit_per_minute, 50);
        }
        else if (endpoint.endpoint_pattern == "/api/audit/*") {
            found_audit = true;
            EXPECT_EQ(endpoint.required_scope, "audit:read");
            EXPECT_TRUE(endpoint.auth_required);
            EXPECT_EQ(endpoint.rate_limit_per_minute, 50);
        }
        else if (endpoint.endpoint_pattern == "/health") {
            found_health = true;
            EXPECT_FALSE(endpoint.auth_required);
            EXPECT_EQ(endpoint.rate_limit_per_minute, 1000);
        }
        else if (endpoint.endpoint_pattern == "/config" && endpoint.http_method == "GET") {
            found_config_get = true;
            EXPECT_EQ(endpoint.required_scope, "config:read");
            EXPECT_TRUE(endpoint.auth_required);
        }
        else if (endpoint.endpoint_pattern == "/config" && endpoint.http_method == "POST") {
            found_config_post = true;
            EXPECT_EQ(endpoint.required_scope, "config:write");
            EXPECT_TRUE(endpoint.auth_required);
        }
    }
    
    EXPECT_TRUE(found_entities_get) << "Entities GET endpoint not configured";
    EXPECT_TRUE(found_entities_post) << "Entities POST endpoint not configured";
    EXPECT_TRUE(found_query) << "Query endpoint not configured";
    EXPECT_TRUE(found_admin) << "Admin endpoint not configured";
    EXPECT_TRUE(found_audit) << "Audit endpoint not configured";
    EXPECT_TRUE(found_health) << "Health endpoint not configured";
    EXPECT_TRUE(found_config_get) << "Config GET endpoint not configured";
    EXPECT_TRUE(found_config_post) << "Config POST endpoint not configured";
}

TEST_F(ApiAuthConfigTest, GetEndpointConfigExactMatch) {
    auto config = ApiAuthConfig::createSecureDefaults();
    
    auto health_config = config.getEndpointConfig("/health");
    ASSERT_TRUE(health_config.has_value());
    EXPECT_EQ(health_config->endpoint_pattern, "/health");
    EXPECT_FALSE(health_config->auth_required);
}

TEST_F(ApiAuthConfigTest, GetEndpointConfigWildcardMatch) {
    auto config = ApiAuthConfig::createSecureDefaults();
    
    // Test wildcard pattern matching with GET method
    auto entities_config = config.getEndpointConfig("/entities/user:123", "GET");
    ASSERT_TRUE(entities_config.has_value());
    EXPECT_EQ(entities_config->endpoint_pattern, "/entities/*");
    EXPECT_EQ(entities_config->http_method, "GET");
    EXPECT_EQ(entities_config->required_scope, "data:read");
    EXPECT_TRUE(entities_config->auth_required);
    
    // Test with POST method - should get different scope
    entities_config = config.getEndpointConfig("/entities/user:123", "POST");
    ASSERT_TRUE(entities_config.has_value());
    EXPECT_EQ(entities_config->endpoint_pattern, "/entities/*");
    EXPECT_EQ(entities_config->http_method, "POST");
    EXPECT_EQ(entities_config->required_scope, "data:write");
    
    // Test with different entity ID
    entities_config = config.getEndpointConfig("/entities/doc:abc", "GET");
    ASSERT_TRUE(entities_config.has_value());
    EXPECT_EQ(entities_config->endpoint_pattern, "/entities/*");
    EXPECT_EQ(entities_config->required_scope, "data:read");
}

TEST_F(ApiAuthConfigTest, GetEndpointConfigNoMatch) {
    auto config = ApiAuthConfig::createSecureDefaults();
    
    auto result = config.getEndpointConfig("/nonexistent/endpoint");
    EXPECT_FALSE(result.has_value());
}

TEST_F(ApiAuthConfigTest, GetEndpointConfigMultiLevelWildcard) {
    auto config = ApiAuthConfig::createSecureDefaults();
    
    // Test admin wildcard
    auto admin_config = config.getEndpointConfig("/admin/backup");
    ASSERT_TRUE(admin_config.has_value());
    EXPECT_EQ(admin_config->endpoint_pattern, "/admin/*");
    EXPECT_EQ(admin_config->required_scope, "admin");
    
    // Test PKI wildcard - should match more specific /api/pki/* not less specific /api/*
    auto pki_config = config.getEndpointConfig("/api/pki/sign");
    ASSERT_TRUE(pki_config.has_value());
    EXPECT_EQ(pki_config->endpoint_pattern, "/api/pki/*");
    EXPECT_EQ(pki_config->required_scope, "pki:read");
    
    // Test more specific PKI patterns
    auto pki_hsm_config = config.getEndpointConfig("/api/pki/hsm/keys");
    ASSERT_TRUE(pki_hsm_config.has_value());
    EXPECT_EQ(pki_hsm_config->endpoint_pattern, "/api/pki/hsm/*");
    EXPECT_EQ(pki_hsm_config->required_scope, "pki:sign");
}

TEST_F(ApiAuthConfigTest, RateLimitsAreReasonable) {
    auto config = ApiAuthConfig::createSecureDefaults();
    
    // Check that rate limits are in reasonable ranges
    for (const auto& endpoint : config.endpoint_configs) {
        EXPECT_GT(endpoint.rate_limit_per_minute, 0) 
            << "Rate limit should be positive for " << endpoint.endpoint_pattern;
        EXPECT_LE(endpoint.rate_limit_per_minute, 10000)
            << "Rate limit seems too high for " << endpoint.endpoint_pattern;
        
        if (endpoint.rate_limit_burst > 0) {
            EXPECT_LE(endpoint.rate_limit_burst, endpoint.rate_limit_per_minute / 2)
                << "Burst should be reasonable relative to rate limit for " << endpoint.endpoint_pattern;
        }
    }
}

TEST_F(ApiAuthConfigTest, SensitiveEndpointsHaveRestrictiveLimits) {
    auto config = ApiAuthConfig::createSecureDefaults();
    
    // Audit endpoints should have low limits
    auto audit_config = config.getEndpointConfig("/api/audit/query");
    ASSERT_TRUE(audit_config.has_value());
    EXPECT_LE(audit_config->rate_limit_per_minute, 100)
        << "Audit endpoints should have restrictive rate limits";
    
    // Admin endpoints should have low limits
    auto admin_config = config.getEndpointConfig("/admin/backup");
    ASSERT_TRUE(admin_config.has_value());
    EXPECT_LE(admin_config->rate_limit_per_minute, 100)
        << "Admin endpoints should have restrictive rate limits";
    
    // Config endpoints should have very low limits
    auto config_result = config.getEndpointConfig("/config");
    ASSERT_TRUE(config_result.has_value());
    EXPECT_LE(config_result->rate_limit_per_minute, 20)
        << "Config endpoints should have very restrictive rate limits";
}

TEST_F(ApiAuthConfigTest, HighTrafficEndpointsHaveHigherLimits) {
    auto config = ApiAuthConfig::createSecureDefaults();
    
    // Health check should have high limits
    auto health_config = config.getEndpointConfig("/health");
    ASSERT_TRUE(health_config.has_value());
    EXPECT_GE(health_config->rate_limit_per_minute, 1000)
        << "Health check should have high rate limits";
    
    // Entity read operations should have high limits
    auto entities_config = config.getEndpointConfig("/entities/user:123", "GET");
    ASSERT_TRUE(entities_config.has_value());
    EXPECT_GE(entities_config->rate_limit_per_minute, 500)
        << "Entity operations should have reasonable rate limits";
}

TEST_F(ApiAuthConfigTest, AllSecuritySensitiveEndpointsRequireAuth) {
    auto config = ApiAuthConfig::createSecureDefaults();
    
    // Check that security-sensitive endpoints require authentication
    std::vector<std::string> sensitive_patterns = {
        "/admin/*",
        "/api/audit/*",
        "/config",
        "/pii/*",
        "/api/pki/*"
    };
    
    for (const auto& pattern : sensitive_patterns) {
        bool found = false;
        for (const auto& endpoint : config.endpoint_configs) {
            if (endpoint.endpoint_pattern == pattern) {
                found = true;
                EXPECT_TRUE(endpoint.auth_required)
                    << "Sensitive endpoint " << pattern << " should require authentication";
                EXPECT_FALSE(endpoint.required_scope.empty())
                    << "Sensitive endpoint " << pattern << " should have a required scope";
                break;
            }
        }
        EXPECT_TRUE(found) << "Sensitive endpoint pattern " << pattern << " not found in config";
    }
}

TEST_F(ApiAuthConfigTest, PublicEndpointsDoNotRequireAuth) {
    auto config = ApiAuthConfig::createSecureDefaults();
    
    // Check that public endpoints don't require authentication
    std::vector<std::string> public_endpoints = {
        "/health",
        "/version",
        "/stats",
        "/capabilities",
        "/metrics"
    };
    
    for (const auto& endpoint : public_endpoints) {
        auto result = config.getEndpointConfig(endpoint);
        ASSERT_TRUE(result.has_value()) << "Public endpoint " << endpoint << " not configured";
        EXPECT_FALSE(result->auth_required)
            << "Public endpoint " << endpoint << " should not require authentication";
    }
}

TEST_F(ApiAuthConfigTest, CustomEndpointConfiguration) {
    ApiAuthConfig config;
    
    // Add custom endpoint
    EndpointAuthConfig custom;
    custom.endpoint_pattern = "/custom/api/*";
    custom.http_method = "POST";
    custom.required_scope = "custom:access";
    custom.action = "custom";
    custom.auth_required = true;
    custom.rate_limit_per_minute = 200;
    custom.rate_limit_burst = 40;
    
    config.endpoint_configs.push_back(custom);
    
    // Verify custom endpoint with POST
    auto result = config.getEndpointConfig("/custom/api/test", "POST");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->endpoint_pattern, "/custom/api/*");
    EXPECT_EQ(result->http_method, "POST");
    EXPECT_EQ(result->required_scope, "custom:access");
    EXPECT_EQ(result->action, "custom");
    EXPECT_TRUE(result->auth_required);
    EXPECT_EQ(result->rate_limit_per_minute, 200);
    EXPECT_EQ(result->rate_limit_burst, 40);
    
    // Should not match with GET
    result = config.getEndpointConfig("/custom/api/test", "GET");
    EXPECT_FALSE(result.has_value());
}

TEST_F(ApiAuthConfigTest, ExactMatchTakesPrecedenceOverWildcard) {
    ApiAuthConfig config;
    
    // Add wildcard pattern
    EndpointAuthConfig wildcard;
    wildcard.endpoint_pattern = "/api/*";
    wildcard.required_scope = "api:general";
    wildcard.rate_limit_per_minute = 100;
    config.endpoint_configs.push_back(wildcard);
    
    // Add exact match
    EndpointAuthConfig exact;
    exact.endpoint_pattern = "/api/special";
    exact.required_scope = "api:special";
    exact.rate_limit_per_minute = 50;
    config.endpoint_configs.push_back(exact);
    
    // Exact match should be found first
    auto result = config.getEndpointConfig("/api/special");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->endpoint_pattern, "/api/special");
    EXPECT_EQ(result->required_scope, "api:special");
    EXPECT_EQ(result->rate_limit_per_minute, 50);
    
    // Other paths should match wildcard
    result = config.getEndpointConfig("/api/other");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->endpoint_pattern, "/api/*");
    EXPECT_EQ(result->required_scope, "api:general");
    EXPECT_EQ(result->rate_limit_per_minute, 100);
}

TEST_F(ApiAuthConfigTest, EmptyPatternIsHandledSafely) {
    ApiAuthConfig config;
    
    // Add empty pattern (should not crash)
    EndpointAuthConfig empty;
    empty.endpoint_pattern = "";
    empty.required_scope = "none";
    empty.rate_limit_per_minute = 100;
    config.endpoint_configs.push_back(empty);
    
    // Should not match anything
    auto result = config.getEndpointConfig("/any/path");
    EXPECT_FALSE(result.has_value());
    
    // Empty pattern should only match empty path
    result = config.getEndpointConfig("");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->endpoint_pattern, "");
}

TEST_F(ApiAuthConfigTest, HttpMethodDifferentiation) {
    auto config = ApiAuthConfig::createSecureDefaults();
    
    // Test that GET /config requires config:read
    auto config_get = config.getEndpointConfig("/config", "GET");
    ASSERT_TRUE(config_get.has_value());
    EXPECT_EQ(config_get->required_scope, "config:read");
    
    // Test that POST /config requires config:write
    auto config_post = config.getEndpointConfig("/config", "POST");
    ASSERT_TRUE(config_post.has_value());
    EXPECT_EQ(config_post->required_scope, "config:write");
    
    // Test exact /entities path with different methods
    auto entities_get = config.getEndpointConfig("/entities", "GET");
    ASSERT_TRUE(entities_get.has_value());
    EXPECT_EQ(entities_get->required_scope, "data:read");
    
    auto entities_post = config.getEndpointConfig("/entities", "POST");
    ASSERT_TRUE(entities_post.has_value());
    EXPECT_EQ(entities_post->required_scope, "data:write");
    
    auto entities_delete = config.getEndpointConfig("/entities", "DELETE");
    ASSERT_TRUE(entities_delete.has_value());
    EXPECT_EQ(entities_delete->required_scope, "data:write");
}

TEST_F(ApiAuthConfigTest, MostSpecificWildcardMatching) {
    ApiAuthConfig config;
    
    // Add less specific wildcard first
    EndpointAuthConfig general;
    general.endpoint_pattern = "/api/*";
    general.required_scope = "api:general";
    general.rate_limit_per_minute = 100;
    config.endpoint_configs.push_back(general);
    
    // Add more specific wildcard
    EndpointAuthConfig specific;
    specific.endpoint_pattern = "/api/pki/*";
    specific.required_scope = "pki:specific";
    specific.rate_limit_per_minute = 50;
    config.endpoint_configs.push_back(specific);
    
    // Should match the more specific pattern
    auto result = config.getEndpointConfig("/api/pki/sign");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->endpoint_pattern, "/api/pki/*");
    EXPECT_EQ(result->required_scope, "pki:specific");
    EXPECT_EQ(result->rate_limit_per_minute, 50);
    
    // Should match the less specific pattern
    result = config.getEndpointConfig("/api/other/endpoint");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->endpoint_pattern, "/api/*");
    EXPECT_EQ(result->required_scope, "api:general");
    EXPECT_EQ(result->rate_limit_per_minute, 100);
}

TEST_F(ApiAuthConfigTest, WildcardMethodMatching) {
    ApiAuthConfig config;
    
    // Add config with wildcard method
    EndpointAuthConfig wildcard_method;
    wildcard_method.endpoint_pattern = "/query";
    wildcard_method.http_method = "*";
    wildcard_method.required_scope = "data:read";
    wildcard_method.rate_limit_per_minute = 500;
    config.endpoint_configs.push_back(wildcard_method);
    
    // Should match any HTTP method
    auto get_result = config.getEndpointConfig("/query", "GET");
    ASSERT_TRUE(get_result.has_value());
    EXPECT_EQ(get_result->required_scope, "data:read");
    
    auto post_result = config.getEndpointConfig("/query", "POST");
    ASSERT_TRUE(post_result.has_value());
    EXPECT_EQ(post_result->required_scope, "data:read");
    
    auto put_result = config.getEndpointConfig("/query", "PUT");
    ASSERT_TRUE(put_result.has_value());
    EXPECT_EQ(put_result->required_scope, "data:read");
}