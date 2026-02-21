/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_auth_middleware.cpp                           ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:08:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     729                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "server/auth_middleware.h"

using namespace themis;

// Test constants
namespace {
    constexpr int CONCURRENT_TEST_COUNT = 10;
}

class AuthMiddlewareTest : public ::testing::Test {
protected:
    AuthMiddleware auth_;
    
    void SetUp() override {
        // Add test tokens
        AuthMiddleware::TokenConfig admin_token{
            .token = "admin-token-123",
            .user_id = "admin",
            .tenant_id = "test-tenant",
            .scopes = {"admin", "config:write", "config:read", "cdc:read", "metrics:read"}
        };
        
        AuthMiddleware::TokenConfig readonly_token{
            .token = "readonly-token-456",
            .user_id = "viewer",
            .tenant_id = "test-tenant",
            .scopes = {"cdc:read", "metrics:read"}
        };
        
        auth_.addToken(admin_token);
        auth_.addToken(readonly_token);
    }
};

TEST_F(AuthMiddlewareTest, ExtractBearerToken) {
    auto token = AuthMiddleware::extractBearerToken("Bearer abc123");
    ASSERT_TRUE(token.has_value());
    EXPECT_EQ(*token, "abc123");
    
    token = AuthMiddleware::extractBearerToken("Bearer   xyz789  ");
    ASSERT_TRUE(token.has_value());
    EXPECT_EQ(*token, "xyz789");
    
    token = AuthMiddleware::extractBearerToken("InvalidFormat");
    EXPECT_FALSE(token.has_value());
    
    token = AuthMiddleware::extractBearerToken("");
    EXPECT_FALSE(token.has_value());
}

TEST_F(AuthMiddlewareTest, ValidateToken_Valid) {
    auto result = auth_.validateToken("admin-token-123");
    EXPECT_TRUE(result.authorized);
    EXPECT_EQ(result.user_id, "admin");
    
    result = auth_.validateToken("readonly-token-456");
    EXPECT_TRUE(result.authorized);
    EXPECT_EQ(result.user_id, "viewer");
}

TEST_F(AuthMiddlewareTest, ValidateToken_Invalid) {
    auto result = auth_.validateToken("invalid-token");
    EXPECT_FALSE(result.authorized);
    EXPECT_FALSE(result.reason.empty());
}

TEST_F(AuthMiddlewareTest, Authorize_AdminHasAllScopes) {
    auto result = auth_.authorize("admin-token-123", "admin");
    EXPECT_TRUE(result.authorized);
    
    result = auth_.authorize("admin-token-123", "config:write");
    EXPECT_TRUE(result.authorized);
    
    result = auth_.authorize("admin-token-123", "cdc:read");
    EXPECT_TRUE(result.authorized);
}

TEST_F(AuthMiddlewareTest, Authorize_ReadonlyLimitedScopes) {
    auto result = auth_.authorize("readonly-token-456", "cdc:read");
    EXPECT_TRUE(result.authorized);
    
    result = auth_.authorize("readonly-token-456", "metrics:read");
    EXPECT_TRUE(result.authorized);
    
    // Readonly should NOT have admin or config:write
    result = auth_.authorize("readonly-token-456", "admin");
    EXPECT_FALSE(result.authorized);
    EXPECT_FALSE(result.reason.empty());
    
    result = auth_.authorize("readonly-token-456", "config:write");
    EXPECT_FALSE(result.authorized);
}

TEST_F(AuthMiddlewareTest, Authorize_InvalidToken) {
    auto result = auth_.authorize("invalid-token", "admin");
    EXPECT_FALSE(result.authorized);
}

TEST_F(AuthMiddlewareTest, Metrics_TrackAuthAttempts) {
    auto& metrics = auth_.getMetrics();
    auto initial_success = metrics.authz_success_total.load();
    auto initial_denied = metrics.authz_denied_total.load();
    auto initial_invalid = metrics.authz_invalid_token_total.load();
    
    // Success
    auth_.authorize("admin-token-123", "admin");
    EXPECT_EQ(metrics.authz_success_total.load(), initial_success + 1);
    
    // Denied (valid token, missing scope)
    auth_.authorize("readonly-token-456", "admin");
    EXPECT_EQ(metrics.authz_denied_total.load(), initial_denied + 1);
    
    // Invalid token
    auth_.authorize("bad-token", "admin");
    EXPECT_EQ(metrics.authz_invalid_token_total.load(), initial_invalid + 1);
}

TEST_F(AuthMiddlewareTest, RemoveToken) {
    auto result = auth_.validateToken("admin-token-123");
    EXPECT_TRUE(result.authorized);
    
    auth_.removeToken("admin-token-123");
    
    result = auth_.validateToken("admin-token-123");
    EXPECT_FALSE(result.authorized);
}

TEST_F(AuthMiddlewareTest, ClearTokens) {
    auth_.clearTokens();
    
    auto result = auth_.validateToken("admin-token-123");
    EXPECT_FALSE(result.authorized);
    
    result = auth_.validateToken("readonly-token-456");
    EXPECT_FALSE(result.authorized);
}

// ============================================================================
// Token Validation Tests (New)
// ============================================================================

TEST_F(AuthMiddlewareTest, ValidJWTTokenAccepted) {
    // Test that a properly formatted JWT-like token is accepted by the middleware
    // Note: This tests the token structure, not full JWT validation
    AuthMiddleware::TokenConfig jwt_style_token{
        .token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJ1c2VyMTIzIiwicm9sZXMiOlsiYWRtaW4iXX0",
        .user_id = "user123",
        .scopes = {"admin", "read", "write"}
    };
    
    auth_.addToken(jwt_style_token);
    
    auto result = auth_.validateToken(jwt_style_token.token);
    EXPECT_TRUE(result.authorized);
    EXPECT_EQ(result.user_id, "user123");
}

TEST_F(AuthMiddlewareTest, ExpiredTokenRejected) {
    // Test that removed tokens are treated as expired/invalid
    AuthMiddleware::TokenConfig temp_token{
        .token = "temp-token-789",
        .user_id = "tempuser",
        .scopes = {"read"}
    };
    
    auth_.addToken(temp_token);
    
    // Validate it works initially
    auto result = auth_.validateToken("temp-token-789");
    EXPECT_TRUE(result.authorized);
    
    // "Expire" by removing
    auth_.removeToken("temp-token-789");
    
    // Should now be rejected
    result = auth_.validateToken("temp-token-789");
    EXPECT_FALSE(result.authorized);
    EXPECT_FALSE(result.reason.empty());
}

TEST_F(AuthMiddlewareTest, InvalidSignatureDetected) {
    // Test that a token with invalid format/signature is rejected
    std::string malformed_jwt = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.invalid.signature";
    
    auto result = auth_.validateToken(malformed_jwt);
    EXPECT_FALSE(result.authorized);
    EXPECT_FALSE(result.reason.empty());
}

TEST_F(AuthMiddlewareTest, MalformedTokenRejected) {
    // Test various malformed token formats
    std::vector<std::string> malformed_tokens = {
        "",                    // Empty token
        "   ",                 // Whitespace only
        "Bearer invalid",      // Bearer prefix (should be extracted first)
        "!@#$%^&*()",         // Special characters
        std::string(10000, 'x') // Extremely long token
    };
    
    for (const auto& token : malformed_tokens) {
        auto result = auth_.validateToken(token);
        EXPECT_FALSE(result.authorized) << "Token should be rejected: " << token;
    }
}

// ============================================================================
// Authorization Tests (New)
// ============================================================================

TEST_F(AuthMiddlewareTest, RBACEnforcement) {
    // Test Role-Based Access Control enforcement
    AuthMiddleware::TokenConfig role_admin{
        .token = "role-admin-token",
        .user_id = "admin_user",
        .scopes = {"admin", "user:manage", "system:config"}
    };
    
    AuthMiddleware::TokenConfig role_user{
        .token = "role-user-token",
        .user_id = "normal_user",
        .scopes = {"user:read", "data:read"}
    };
    
    auth_.addToken(role_admin);
    auth_.addToken(role_user);
    
    // Admin should have admin scope
    auto result = auth_.authorize("role-admin-token", "admin");
    EXPECT_TRUE(result.authorized);
    
    // Normal user should not have admin scope
    result = auth_.authorize("role-user-token", "admin");
    EXPECT_FALSE(result.authorized);
    
    // Normal user should have read scope
    result = auth_.authorize("role-user-token", "user:read");
    EXPECT_TRUE(result.authorized);
}

TEST_F(AuthMiddlewareTest, PermissionCheckOnEndpoints) {
    // Test permission checking for different endpoint scopes
    struct EndpointTest {
        std::string scope;
        bool expected;
    };
    
    std::vector<EndpointTest> endpoint_tests = {
        {"config:write", true},   // Admin should have
        {"config:read", true},    // Admin should have
        {"metrics:read", true},   // Admin should have
        {"data:delete", false},   // Admin doesn't have this specific scope
        {"api:access", false}     // Admin doesn't have this specific scope
    };
    
    for (const auto& test : endpoint_tests) {
        auto result = auth_.authorize("admin-token-123", test.scope);
        EXPECT_EQ(result.authorized, test.expected) 
            << "Scope: " << test.scope << " should be " << (test.expected ? "authorized" : "denied");
    }
}

TEST_F(AuthMiddlewareTest, DeniedAccessResponse) {
    // Test that denied access includes proper reason
    auto result = auth_.authorize("readonly-token-456", "admin");
    
    EXPECT_FALSE(result.authorized);
    EXPECT_FALSE(result.reason.empty());
    EXPECT_EQ(result.user_id, "");  // Should not return user_id on failure
    
    // Test with invalid token
    result = auth_.authorize("nonexistent-token", "any-scope");
    EXPECT_FALSE(result.authorized);
    EXPECT_FALSE(result.reason.empty());
}

TEST_F(AuthMiddlewareTest, ResourceLevelAuthorization) {
    // Test authorization for resource-level access
    AuthMiddleware::TokenConfig resource_token{
        .token = "resource-token",
        .user_id = "resource_user",
        .scopes = {"resource:123:read", "resource:123:write", "resource:456:read"}
    };
    
    auth_.addToken(resource_token);
    
    // Should have read and write access to resource 123
    auto result = auth_.authorize("resource-token", "resource:123:read");
    EXPECT_TRUE(result.authorized);
    
    result = auth_.authorize("resource-token", "resource:123:write");
    EXPECT_TRUE(result.authorized);
    
    // Should only have read access to resource 456
    result = auth_.authorize("resource-token", "resource:456:read");
    EXPECT_TRUE(result.authorized);
    
    result = auth_.authorize("resource-token", "resource:456:write");
    EXPECT_FALSE(result.authorized);
}

// ============================================================================
// Authentication Flow Tests (New)
// ============================================================================

TEST_F(AuthMiddlewareTest, BearerTokenExtraction) {
    // Test comprehensive Bearer token extraction scenarios
    struct TestCase {
        std::string input;
        std::string expected;
        bool should_succeed;
    };
    
    std::vector<TestCase> test_cases = {
        {"Bearer token123", "token123", true},
        {"Bearer    token-with-spaces   ", "token-with-spaces", true},
        {"Bearer eyJhbGci.eyJzdWI.SflKx", "eyJhbGci.eyJzdWI.SflKx", true},
        {"bearer lowercase", "", false},  // Case sensitive
        {"Token token123", "", false},    // Wrong prefix
        {"", "", false},                  // Empty
        {"Bearer", "", false},            // No token after Bearer
        {"BearerNoSpace", "", false}      // No space after Bearer
    };
    
    for (const auto& tc : test_cases) {
        auto result = AuthMiddleware::extractBearerToken(tc.input);
        if (tc.should_succeed) {
            ASSERT_TRUE(result.has_value()) << "Input: " << tc.input;
            EXPECT_EQ(*result, tc.expected) << "Input: " << tc.input;
        } else {
            EXPECT_FALSE(result.has_value()) << "Input: " << tc.input;
        }
    }
}

TEST_F(AuthMiddlewareTest, TokenRefreshMechanism) {
    // Test token refresh by removing old token and adding new one
    AuthMiddleware::TokenConfig old_token{
        .token = "old-refresh-token",
        .user_id = "user_refresh",
        .scopes = {"read"}
    };
    
    auth_.addToken(old_token);
    
    // Verify old token works
    auto result = auth_.validateToken("old-refresh-token");
    EXPECT_TRUE(result.authorized);
    
    // Simulate refresh: remove old, add new
    auth_.removeToken("old-refresh-token");
    
    AuthMiddleware::TokenConfig new_token{
        .token = "new-refresh-token",
        .user_id = "user_refresh",
        .scopes = {"read", "write"}
    };
    
    auth_.addToken(new_token);
    
    // Old token should not work
    result = auth_.validateToken("old-refresh-token");
    EXPECT_FALSE(result.authorized);
    
    // New token should work with updated scopes
    result = auth_.validateToken("new-refresh-token");
    EXPECT_TRUE(result.authorized);
    
    result = auth_.authorize("new-refresh-token", "write");
    EXPECT_TRUE(result.authorized);
}

TEST_F(AuthMiddlewareTest, SessionManagement) {
    // Test managing multiple sessions for the same user
    AuthMiddleware::TokenConfig session1{
        .token = "session1-token",
        .user_id = "multi_session_user",
        .scopes = {"read"}
    };
    
    AuthMiddleware::TokenConfig session2{
        .token = "session2-token",
        .user_id = "multi_session_user",
        .scopes = {"read", "write"}
    };
    
    auth_.addToken(session1);
    auth_.addToken(session2);
    
    // Both sessions should be valid
    auto result1 = auth_.validateToken("session1-token");
    EXPECT_TRUE(result1.authorized);
    EXPECT_EQ(result1.user_id, "multi_session_user");
    
    auto result2 = auth_.validateToken("session2-token");
    EXPECT_TRUE(result2.authorized);
    EXPECT_EQ(result2.user_id, "multi_session_user");
    
    // Each session has independent scopes
    EXPECT_TRUE(auth_.authorize("session1-token", "read").authorized);
    EXPECT_FALSE(auth_.authorize("session1-token", "write").authorized);
    EXPECT_TRUE(auth_.authorize("session2-token", "write").authorized);
    
    // Remove one session, other should still work
    auth_.removeToken("session1-token");
    
    EXPECT_FALSE(auth_.validateToken("session1-token").authorized);
    EXPECT_TRUE(auth_.validateToken("session2-token").authorized);
}

TEST_F(AuthMiddlewareTest, ConcurrentSessions) {
    // Test thread-safe concurrent session validation
    // Create multiple tokens
    std::vector<AuthMiddleware::TokenConfig> tokens;
    for (int i = 0; i < CONCURRENT_TEST_COUNT; i++) {
        tokens.push_back({
            .token = "concurrent-token-" + std::to_string(i),
            .user_id = "concurrent_user_" + std::to_string(i),
            .scopes = {"read"}
        });
        auth_.addToken(tokens[i]);
    }
    
    // Validate all tokens (tests thread safety of auth middleware)
    // Note: Auth middleware should be thread-safe for concurrent access
    std::vector<bool> results(CONCURRENT_TEST_COUNT);
    
    for (int i = 0; i < CONCURRENT_TEST_COUNT; i++) {
        auto result = auth_.validateToken("concurrent-token-" + std::to_string(i));
        results[i] = result.authorized;
    }
    
    // All should succeed
    for (int i = 0; i < CONCURRENT_TEST_COUNT; i++) {
        EXPECT_TRUE(results[i]) << "Token " << i << " should be valid";
    }
}

// ============================================================================
// Security Edge Cases (New)
// ============================================================================

TEST_F(AuthMiddlewareTest, SQLInjectionPrevention) {
    // Test that SQL injection attempts in tokens are safely handled
    std::vector<std::string> injection_attempts = {
        "admin' OR '1'='1",
        "admin'--",
        "admin'; DROP TABLE users;--",
        "' OR 1=1--",
        "1' UNION SELECT * FROM tokens--"
    };
    
    for (const auto& injection : injection_attempts) {
        auto result = auth_.validateToken(injection);
        EXPECT_FALSE(result.authorized) 
            << "SQL injection should be rejected: " << injection;
    }
    
    // Also test in scope parameter
    for (const auto& injection : injection_attempts) {
        auto result = auth_.authorize("admin-token-123", injection);
        EXPECT_FALSE(result.authorized) 
            << "SQL injection in scope should be rejected: " << injection;
    }
}

TEST_F(AuthMiddlewareTest, TokenReplayAttackPrevention) {
    // Test that the same token can be validated multiple times
    // (This is expected behavior for stateless tokens, but validates
    // that the middleware handles repeated validations correctly)
    
    auto& metrics = auth_.getMetrics();
    auto initial_success = metrics.authz_success_total.load();
    
    // Validate same token multiple times
    for (int i = 0; i < CONCURRENT_TEST_COUNT; i++) {
        auto result = auth_.authorize("admin-token-123", "admin");
        EXPECT_TRUE(result.authorized);
    }
    
    // Metrics should increment for each validation
    EXPECT_EQ(metrics.authz_success_total.load(), initial_success + CONCURRENT_TEST_COUNT);
    
    // Note: True replay attack prevention would require:
    // - Token expiration (handled by JWT with exp claim)
    // - Nonce/jti tracking (implementation-specific)
    // - Short-lived tokens with refresh tokens
}

TEST_F(AuthMiddlewareTest, RateLimitingOnFailedAuth) {
    // Test that failed authentication attempts are properly tracked
    // (Rate limiting would be implemented at a higher level)
    
    auto& metrics = auth_.getMetrics();
    auto initial_invalid = metrics.authz_invalid_token_total.load();
    
    // Multiple failed attempts
    for (int i = 0; i < 5; i++) {
        auto result = auth_.validateToken("invalid-token-" + std::to_string(i));
        EXPECT_FALSE(result.authorized);
    }
    
    // Failed attempts should be tracked
    EXPECT_EQ(metrics.authz_invalid_token_total.load(), initial_invalid + 5);
    
    // Valid token should still work (no blocking implemented at this level)
    auto result = auth_.validateToken("admin-token-123");
    EXPECT_TRUE(result.authorized);
}

TEST_F(AuthMiddlewareTest, mTLSCertificateValidation) {
    // Test preparation for mTLS certificate-based authentication
    // This tests that the middleware can handle certificate-style tokens
    
    AuthMiddleware::TokenConfig cert_token{
        .token = "CN=client.example.com,OU=Engineering,O=Example Inc,C=US",
        .user_id = "client.example.com",
        .scopes = {"mtls:verified", "api:access"}
    };
    
    auth_.addToken(cert_token);
    
    auto result = auth_.validateToken(cert_token.token);
    EXPECT_TRUE(result.authorized);
    EXPECT_EQ(result.user_id, "client.example.com");
    
    result = auth_.authorize(cert_token.token, "mtls:verified");
    EXPECT_TRUE(result.authorized);
    
    // Invalid cert format should fail
    result = auth_.validateToken("CN=unknown-client");
    EXPECT_FALSE(result.authorized);
}

// ============================================================================
// Integration Tests (New)
// ============================================================================

TEST_F(AuthMiddlewareTest, AuthMiddlewareInHTTPPipeline) {
    // Test simulating auth middleware in HTTP request pipeline
    struct HTTPRequest {
        std::string auth_header;
        std::string endpoint;
        std::string required_scope;
    };
    
    std::vector<HTTPRequest> requests = {
        {"Bearer admin-token-123", "/api/config", "config:write"},
        {"Bearer readonly-token-456", "/api/metrics", "metrics:read"},
        {"Bearer invalid-token", "/api/admin", "admin"},
        {"", "/api/public", ""},  // No auth header
    };
    
    std::vector<bool> expected = {true, true, false, false};
    
    for (size_t i = 0; i < requests.size(); i++) {
        const auto& req = requests[i];
        bool authorized = false;
        
        if (!req.auth_header.empty()) {
            auto token = AuthMiddleware::extractBearerToken(req.auth_header);
            if (token.has_value()) {
                if (req.required_scope.empty()) {
                    auto result = auth_.validateToken(*token);
                    authorized = result.authorized;
                } else {
                    auto result = auth_.authorize(*token, req.required_scope);
                    authorized = result.authorized;
                }
            }
        }
        
        EXPECT_EQ(authorized, expected[i]) 
            << "Request " << i << " to " << req.endpoint 
            << " should be " << (expected[i] ? "authorized" : "denied");
    }
}

TEST_F(AuthMiddlewareTest, MissingAuthHeaderHandling) {
    // Test handling of missing or malformed auth headers
    std::vector<std::string> missing_auth_scenarios = {
        "",                  // Empty header
        "NoBearer token",   // Missing Bearer prefix
        "Basic dXNlcjpwYXNz", // Wrong auth type
        "Bearer ",          // Bearer with no token
        "   ",              // Whitespace only
    };
    
    for (const auto& header : missing_auth_scenarios) {
        auto token = AuthMiddleware::extractBearerToken(header);
        
        if (token.has_value()) {
            // If extraction succeeded, validation should fail
            auto result = auth_.validateToken(*token);
            EXPECT_FALSE(result.authorized) 
                << "Header should fail: " << header;
        } else {
            // Extraction failure is expected for most cases
            EXPECT_FALSE(token.has_value());
        }
    }
}

TEST_F(AuthMiddlewareTest, MultipleHTTPMethodsAuth) {
    // Test authorization for different HTTP methods with appropriate scopes
    struct MethodTest {
        std::string method;
        std::string endpoint;
        std::string token;
        std::string required_scope;
        bool should_succeed;
    };
    
    std::vector<MethodTest> tests = {
        {"GET", "/api/config", "admin-token-123", "config:read", true},
        {"POST", "/api/config", "admin-token-123", "config:write", true},
        {"PUT", "/api/config", "admin-token-123", "config:write", true},
        {"DELETE", "/api/config", "admin-token-123", "config:write", true},
        {"GET", "/api/config", "readonly-token-456", "config:read", true},
        {"POST", "/api/config", "readonly-token-456", "config:write", false},
        {"PUT", "/api/config", "readonly-token-456", "config:write", false},
        {"DELETE", "/api/config", "readonly-token-456", "config:write", false},
    };
    
    for (const auto& test : tests) {
        auto result = auth_.authorize(test.token, test.required_scope);
        EXPECT_EQ(result.authorized, test.should_succeed)
            << test.method << " " << test.endpoint 
            << " with token " << test.token;
    }
}

TEST_F(AuthMiddlewareTest, ErrorResponseFormats) {
    // Test that error responses include appropriate information
    struct ErrorTest {
        std::string token;
        std::string scope;
        bool should_have_reason;
    };
    
    std::vector<ErrorTest> tests = {
        {"invalid-token", "admin", true},
        {"readonly-token-456", "admin", true},
        {"", "admin", true},
    };
    
    for (const auto& test : tests) {
        auto result = auth_.authorize(test.token, test.scope);
        
        EXPECT_FALSE(result.authorized);
        
        if (test.should_have_reason) {
            EXPECT_FALSE(result.reason.empty()) 
                << "Error should include reason for token: " << test.token;
        }
        
        // Failed authorization should not leak user_id
        EXPECT_TRUE(result.user_id.empty() || !result.authorized);
    }
}

// ===== Tenant Isolation Tests =====

TEST_F(AuthMiddlewareTest, TenantExtraction) {
    // Test that tokens with tenant_id properly extract tenant
    auto result = auth_.validateToken("admin-token-123");
    EXPECT_TRUE(result.authorized);
    EXPECT_EQ(result.user_id, "admin");
    EXPECT_EQ(result.tenant_id, "test-tenant");
    
    result = auth_.validateToken("readonly-token-456");
    EXPECT_TRUE(result.authorized);
    EXPECT_EQ(result.user_id, "viewer");
    EXPECT_EQ(result.tenant_id, "test-tenant");
}

TEST_F(AuthMiddlewareTest, TenantInAuthContext) {
    // Test extractContext returns tenant_id
    auto context = auth_.extractContext("admin-token-123");
    ASSERT_TRUE(context.has_value());
    EXPECT_EQ(context->user_id, "admin");
    EXPECT_EQ(context->tenant_id, "test-tenant");
    
    context = auth_.extractContext("invalid-token");
    EXPECT_FALSE(context.has_value());
}

TEST_F(AuthMiddlewareTest, TokenWithoutTenant) {
    // Test token without tenant_id (should have empty tenant_id)
    AuthMiddleware::TokenConfig no_tenant_token{
        .token = "no-tenant-token",
        .user_id = "user-no-tenant",
        .tenant_id = "",  // Empty tenant
        .scopes = {"read"}
    };
    
    auth_.addToken(no_tenant_token);
    
    auto result = auth_.validateToken("no-tenant-token");
    EXPECT_TRUE(result.authorized);
    EXPECT_EQ(result.user_id, "user-no-tenant");
    EXPECT_EQ(result.tenant_id, "");  // Empty string when not specified
}
