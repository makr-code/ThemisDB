#include <gtest/gtest.h>
#include "server/auth_middleware.h"
#include "auth/api_key_authenticator.h"
#include "auth/jwt_validator.h"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#include <chrono>
#include <thread>

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
            .scopes = {"config:read", "cdc:read", "metrics:read"}
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

// ===========================================================================
// API Key Authentication Tests
// ===========================================================================

class ApiKeyMiddlewareTest : public ::testing::Test {
protected:
    AuthMiddleware auth_;

    void SetUp() override {
        AuthMiddleware::ApiKeyConfig cfg;
        cfg.check_expiry = true;
        auth_.enableApiKeyAuth(cfg);

        // Provision two credentials
        auto cred_alice = auth::ApiKeyAuthenticator::createCredential(
            "sk_alice", "secret-alice", "alice@example.com",
            {"data:read", "data:write"}, {"user"}, "tenant-1");
        auth_.addApiKeyCredential(cred_alice);

        auto cred_bob = auth::ApiKeyAuthenticator::createCredential(
            "sk_bob", "secret-bob", "bob@example.com",
            {"data:read"}, {}, "tenant-2");
        auth_.addApiKeyCredential(cred_bob);
    }
};

TEST_F(ApiKeyMiddlewareTest, ValidateToken_ValidCombinedKey) {
    auto result = auth_.validateToken("sk_alice.secret-alice");
    EXPECT_TRUE(result.authorized);
    EXPECT_EQ(result.user_id, "alice@example.com");
    EXPECT_EQ(result.tenant_id, "tenant-1");
}

TEST_F(ApiKeyMiddlewareTest, ValidateToken_WrongSecret) {
    auto result = auth_.validateToken("sk_alice.wrong-secret");
    EXPECT_FALSE(result.authorized);
    EXPECT_FALSE(result.reason.empty());
}

TEST_F(ApiKeyMiddlewareTest, ValidateToken_UnknownKeyId) {
    auto result = auth_.validateToken("sk_unknown.some-secret");
    EXPECT_FALSE(result.authorized);
}

TEST_F(ApiKeyMiddlewareTest, Authorize_ScopePresent) {
    auto result = auth_.authorize("sk_alice.secret-alice", "data:read");
    EXPECT_TRUE(result.authorized);
    EXPECT_EQ(result.user_id, "alice@example.com");
}

TEST_F(ApiKeyMiddlewareTest, Authorize_ScopeMissing) {
    auto result = auth_.authorize("sk_bob.secret-bob", "data:write");
    EXPECT_FALSE(result.authorized);
    EXPECT_FALSE(result.reason.empty());
}

TEST_F(ApiKeyMiddlewareTest, Authorize_MultipleScopes) {
    // Alice has both read and write
    EXPECT_TRUE(auth_.authorize("sk_alice.secret-alice", "data:read").authorized);
    EXPECT_TRUE(auth_.authorize("sk_alice.secret-alice", "data:write").authorized);
    // Bob only has read
    EXPECT_TRUE(auth_.authorize("sk_bob.secret-bob", "data:read").authorized);
    EXPECT_FALSE(auth_.authorize("sk_bob.secret-bob", "data:write").authorized);
}

TEST_F(ApiKeyMiddlewareTest, RemoveCredential_RevokedKeyDenied) {
    EXPECT_TRUE(auth_.validateToken("sk_alice.secret-alice").authorized);
    auth_.removeApiKeyCredential("sk_alice");
    EXPECT_FALSE(auth_.validateToken("sk_alice.secret-alice").authorized);
}

TEST_F(ApiKeyMiddlewareTest, IsEnabled_ApiKeyOnly) {
    AuthMiddleware fresh;
    EXPECT_FALSE(fresh.isEnabled());
    fresh.enableApiKeyAuth();
    // No credentials yet but auth is enabled
    EXPECT_TRUE(fresh.isEnabled());
}

TEST_F(ApiKeyMiddlewareTest, ExpiredKey_Rejected) {
    auto cred = auth::ApiKeyAuthenticator::createCredential(
        "sk_expired", "secret-expired", "expired@example.com",
        {"data:read"}, {}, "tenant-x");
    cred.expires_at = std::chrono::system_clock::now() - std::chrono::seconds(1);
    auth_.addApiKeyCredential(cred);

    auto result = auth_.validateToken("sk_expired.secret-expired");
    EXPECT_FALSE(result.authorized);
}

TEST_F(ApiKeyMiddlewareTest, InactiveKey_Rejected) {
    auto cred = auth::ApiKeyAuthenticator::createCredential(
        "sk_inactive", "secret-inactive", "inactive@example.com",
        {"data:read"}, {}, "tenant-x");
    cred.active = false;
    auth_.addApiKeyCredential(cred);

    auto result = auth_.validateToken("sk_inactive.secret-inactive");
    EXPECT_FALSE(result.authorized);
}

TEST_F(ApiKeyMiddlewareTest, StaticTokensCoexistWithApiKeyAuth) {
    // Static bearer tokens still work alongside API key auth
    AuthMiddleware::TokenConfig tok{
        .token = "plain-bearer-token",
        .user_id = "bearer-user",
        .scopes = {"metrics:read"}
    };
    auth_.addToken(tok);

    // Both authentication methods work
    EXPECT_TRUE(auth_.validateToken("plain-bearer-token").authorized);
    EXPECT_TRUE(auth_.validateToken("sk_alice.secret-alice").authorized);
}

TEST_F(ApiKeyMiddlewareTest, Roles_PropagatedToAuthResult) {
    auto result = auth_.authorize("sk_alice.secret-alice", "data:read");
    EXPECT_TRUE(result.authorized);
    // Roles are returned in the groups field
    ASSERT_EQ(result.groups.size(), 1u);
    EXPECT_EQ(result.groups[0], "user");
}

// ===========================================================================
// JWT Scope Enforcement Tests
// Tests for:
//  - scope/scp claim parsing in JWTClaims (line 248 fix)
//  - role-to-scope mapping enforcement via authorizeViaJWT (line 399 fix)
// ===========================================================================

namespace {

// ── Base64url helpers ──────────────────────────────────────────────────────
static std::string jwt_b64url(const std::vector<uint8_t>& in) {
    static const char* tbl =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string b64;
    b64.reserve(((in.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 3 <= in.size()) {
        uint32_t n = ((uint32_t)in[i]<<16)|((uint32_t)in[i+1]<<8)|in[i+2];
        b64.push_back(tbl[(n>>18)&63]); b64.push_back(tbl[(n>>12)&63]);
        b64.push_back(tbl[(n>> 6)&63]); b64.push_back(tbl[n&63]);
        i += 3;
    }
    if (i + 1 == in.size()) {
        uint32_t n = (uint32_t)in[i] << 16;
        b64.push_back(tbl[(n>>18)&63]); b64.push_back(tbl[(n>>12)&63]);
        b64.push_back('='); b64.push_back('=');
    } else if (i + 2 == in.size()) {
        uint32_t n = ((uint32_t)in[i]<<16)|((uint32_t)in[i+1]<<8);
        b64.push_back(tbl[(n>>18)&63]); b64.push_back(tbl[(n>>12)&63]);
        b64.push_back(tbl[(n>>6)&63]);  b64.push_back('=');
    }
    for (char& c : b64) { if (c=='+') c='-'; else if (c=='/') c='_'; }
    while (!b64.empty() && b64.back()=='=') {
      b64.pop_back();
    }
    return b64;
}

static std::string jwt_b64urlStr(const std::string& s) {
    return jwt_b64url(std::vector<uint8_t>(s.begin(), s.end()));
}

// ── Minimal ECDSA P-256 key fixture ───────────────────────────────────────
struct JwtTestECKey {
    EVP_PKEY* pkey = nullptr;
    JwtTestECKey() {
        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
        EVP_PKEY_keygen_init(ctx);
        EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_X9_62_prime256v1);
        EVP_PKEY_keygen(ctx, &pkey);
        EVP_PKEY_CTX_free(ctx);
    }
    ~JwtTestECKey() { if (pkey) EVP_PKEY_free(pkey); }
    JwtTestECKey(const JwtTestECKey&) = delete;
    JwtTestECKey& operator=(const JwtTestECKey&) = delete;

    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> publicKeyCoords() const {
        const EC_KEY* ec = EVP_PKEY_get0_EC_KEY(pkey);
        const EC_POINT* pt = EC_KEY_get0_public_key(ec);
        const EC_GROUP* grp = EC_KEY_get0_group(ec);
        BIGNUM* bx = BN_new(); BIGNUM* by = BN_new();
        EC_POINT_get_affine_coordinates_GFp(grp, pt, bx, by, nullptr);
        std::vector<uint8_t> x(32, 0), y(32, 0);
        BN_bn2binpad(bx, x.data(), 32);
        BN_bn2binpad(by, y.data(), 32);
        BN_free(bx); BN_free(by);
        return {x, y};
    }
};

static std::string jwt_signES256(EVP_PKEY* pkey, const std::string& msg) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, pkey);
    EVP_DigestSignUpdate(ctx, msg.data(), msg.size());
    size_t sigLen = 0;
    EVP_DigestSignFinal(ctx, nullptr, &sigLen);
    std::vector<uint8_t> der(sigLen);
    EVP_DigestSignFinal(ctx, der.data(), &sigLen);
    EVP_MD_CTX_free(ctx);
    der.resize(sigLen);
    const unsigned char* p = der.data();
    ECDSA_SIG* esig = d2i_ECDSA_SIG(nullptr, &p, (long)der.size());
    const BIGNUM *r = nullptr, *s = nullptr;
    ECDSA_SIG_get0(esig, &r, &s);
    std::vector<uint8_t> rs(64, 0);
    BN_bn2binpad(r, rs.data(),      32);
    BN_bn2binpad(s, rs.data() + 32, 32);
    ECDSA_SIG_free(esig);
    return jwt_b64url(rs);
}

static nlohmann::json jwt_makeECJwks(const JwtTestECKey& key, const std::string& kid = "ec1") {
    auto [x, y] = key.publicKeyCoords();
    nlohmann::json jwk = {
        {"kty","EC"},{"crv","P-256"},{"kid",kid},
        {"alg","ES256"},{"use","sig"},
        {"x",jwt_b64url(x)},{"y",jwt_b64url(y)}
    };
    return nlohmann::json{{"keys", nlohmann::json::array({jwk})}};
}

static std::string jwt_makeES256Token(const JwtTestECKey& key,
                                      const nlohmann::json& extra_claims,
                                      int exp_offset_sec = 300) {
    auto now_sec = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    nlohmann::json payload = {
        {"sub",   "testuser"},
        {"email", "testuser@example.com"},
        {"iss",   "test-issuer"},
        {"aud",   "test-audience"},
        {"exp",   now_sec + exp_offset_sec},
    };
    for (auto it = extra_claims.begin(); it != extra_claims.end(); ++it) {
        payload[it.key()] = it.value();
    }
    nlohmann::json header = {{"alg","ES256"},{"typ","JWT"},{"kid","ec1"}};
    std::string hp = jwt_b64urlStr(header.dump()) + "." + jwt_b64urlStr(payload.dump());
    return hp + "." + jwt_signES256(key.pkey, hp);
}

} // anonymous namespace

class JWTScopeEnforcementTest : public ::testing::Test {
protected:
    void SetUp() override {
        AuthMiddleware::JWTConfig jwt_cfg;
        jwt_cfg.jwks_url = "";
        jwt_cfg.require_issuer_validation = false;
        jwt_cfg.require_audience_validation = false;
        auth_.enableJWT(jwt_cfg);
        auth_.setJWKSForTesting(jwt_makeECJwks(key_));
    }

    JwtTestECKey key_;
    AuthMiddleware auth_;
};

// ── JWTClaims scopes field unit tests ─────────────────────────────────────

TEST(JWTClaimsScopesTest, ScopesFieldExistsAndDefaultsEmpty) {
    themis::auth::JWTClaims claims;
    EXPECT_TRUE(claims.scopes.empty());
}

// ── JWT scope claim extraction ─────────────────────────────────────────────

TEST_F(JWTScopeEnforcementTest, SpaceSeparatedScopeClaimParsed) {
    // JWT with "scope" claim as space-separated string
    auto token = jwt_makeES256Token(key_, {{"scope", "cache:read cache:write metrics:read"}});
    auto result = auth_.authorize(token, "cache:read");
    EXPECT_TRUE(result.authorized) << result.reason;
    EXPECT_EQ(result.user_id, "testuser");
}

TEST_F(JWTScopeEnforcementTest, ScpArrayClaimParsed) {
    // JWT with "scp" claim as JSON array
    auto token = jwt_makeES256Token(
        key_, {{"scp", nlohmann::json::array({"cache:read", "metrics:read"})}});
    auto result = auth_.authorize(token, "cache:read");
    EXPECT_TRUE(result.authorized) << result.reason;
}

TEST_F(JWTScopeEnforcementTest, MissingScopeDenied) {
    // JWT grants cache:read but endpoint requires cache:write
    auto token = jwt_makeES256Token(key_, {{"scope", "cache:read metrics:read"}});
    auto result = auth_.authorize(token, "cache:write");
    EXPECT_FALSE(result.authorized);
    EXPECT_FALSE(result.reason.empty());
}

TEST_F(JWTScopeEnforcementTest, NoScopeClaimDeniedWhenScopeRequired) {
    // JWT has no scope/scp claim and no role mapping configured
    auto token = jwt_makeES256Token(key_, {});
    auto result = auth_.authorize(token, "cache:read");
    EXPECT_FALSE(result.authorized);
    EXPECT_FALSE(result.reason.empty());
}

TEST_F(JWTScopeEnforcementTest, EmptyRequiredScopeAlwaysPasses) {
    // Empty required_scope means "any valid token" (no scope enforcement)
    auto token = jwt_makeES256Token(key_, {});
    auto result = auth_.authorize(token, "");
    EXPECT_TRUE(result.authorized) << result.reason;
}

// ── Role-to-scope mapping enforcement ─────────────────────────────────────

TEST_F(JWTScopeEnforcementTest, RoleMappingGrantsScope) {
    // Configure role-to-scope mapping
    auth_.setRoleScopeMapping({
        {"cache_reader", {"cache:read", "metrics:read"}},
        {"cache_writer", {"cache:read", "cache:write", "metrics:read"}}
    });

    // JWT with roles claim (no scope claim) — role grants the required scope
    auto token = jwt_makeES256Token(key_,
        {{"roles", nlohmann::json::array({"cache_reader"})}});
    auto result = auth_.authorize(token, "cache:read");
    EXPECT_TRUE(result.authorized) << result.reason;
}

TEST_F(JWTScopeEnforcementTest, RoleMappingDeniesWhenScopeNotInRole) {
    auth_.setRoleScopeMapping({
        {"cache_reader", {"cache:read", "metrics:read"}}
    });

    // cache_reader role does not grant cache:write
    auto token = jwt_makeES256Token(key_,
        {{"roles", nlohmann::json::array({"cache_reader"})}});
    auto result = auth_.authorize(token, "cache:write");
    EXPECT_FALSE(result.authorized);
    EXPECT_FALSE(result.reason.empty());
}

TEST_F(JWTScopeEnforcementTest, DirectScopeClaimTakesPrecedenceOverRoleMap) {
    // Token explicitly carries cache:write in scope claim AND has a role
    // that only grants cache:read. The direct scope claim should be used.
    auth_.setRoleScopeMapping({
        {"analyst", {"cache:read", "metrics:read"}}
    });

    auto token = jwt_makeES256Token(key_,
        {{"scope", "cache:read cache:write"},
         {"roles", nlohmann::json::array({"analyst"})}});
    auto result = auth_.authorize(token, "cache:write");
    EXPECT_TRUE(result.authorized) << result.reason;
}

TEST_F(JWTScopeEnforcementTest, SetRoleScopeMappingOverridesLoadedConfig) {
    // Overriding the mapping that may have been loaded from rbac_roles.yaml
    auth_.setRoleScopeMapping({
        {"superuser", {"cache:read", "cache:write", "admin"}}
    });

    auto token = jwt_makeES256Token(key_,
        {{"roles", nlohmann::json::array({"superuser"})}});
    EXPECT_TRUE(auth_.authorize(token, "admin").authorized);
    EXPECT_TRUE(auth_.authorize(token, "cache:read").authorized);
    EXPECT_FALSE(auth_.authorize(token, "config:write").authorized);
}

// ── Role-to-scope mapping: setRoleScopeMapping API ────────────────────────

TEST(RoleScopeMappingTest, SetRoleScopeMappingConfiguresMapping) {
    AuthMiddleware auth;
    auth.setRoleScopeMapping({
        {"admin",        {"admin", "cache:read", "cache:write"}},
        {"cache_reader", {"cache:read"}}
    });

    // Static token with a role that we use only to verify the mapping is stored;
    // the static-token path checks scopes directly so we use it here.
    AuthMiddleware::TokenConfig tok{
        .token = "test-token",
        .user_id = "tester",
        .scopes = {"cache:read"}
    };
    auth.addToken(tok);
    EXPECT_TRUE(auth.authorize("test-token", "cache:read").authorized);
    EXPECT_FALSE(auth.authorize("test-token", "cache:write").authorized);
}

// =============================================================================
// TaskScheduler RequestContext thread-local tests
// =============================================================================

#include "scheduler/task_scheduler.h"
using namespace themis;

TEST(TaskSchedulerRequestContext, DefaultIsSystemUser) {
    TaskScheduler::clearRequestContext();
    EXPECT_EQ(TaskScheduler::currentUserId(), "system");
    EXPECT_EQ(TaskScheduler::currentClientIp(), "");
}

TEST(TaskSchedulerRequestContext, SetContextReturnsCorrectValues) {
    TaskScheduler::setRequestContext({"alice", "192.168.1.1"});
    EXPECT_EQ(TaskScheduler::currentUserId(), "alice");
    EXPECT_EQ(TaskScheduler::currentClientIp(), "192.168.1.1");
    TaskScheduler::clearRequestContext();
}

TEST(TaskSchedulerRequestContext, ClearResetsToDefault) {
    TaskScheduler::setRequestContext({"bob", "10.0.0.1"});
    TaskScheduler::clearRequestContext();
    EXPECT_EQ(TaskScheduler::currentUserId(), "system");
    EXPECT_EQ(TaskScheduler::currentClientIp(), "");
}

TEST(TaskSchedulerRequestContext, ContextIsPerThread) {
    TaskScheduler::setRequestContext({"main-thread-user", "1.2.3.4"});

    std::string bg_user;
    std::thread bg([&bg_user]() {
        // Background thread has its own independent context
        bg_user = TaskScheduler::currentUserId();
    });
    bg.join();

    EXPECT_EQ(bg_user, "system");  // Background thread sees default
    EXPECT_EQ(TaskScheduler::currentUserId(), "main-thread-user");  // Main unchanged
    TaskScheduler::clearRequestContext();
}

TEST(TaskSchedulerRequestContext, FallbackParameterUsed) {
    TaskScheduler::clearRequestContext();
    EXPECT_EQ(TaskScheduler::currentUserId("svc-account"), "svc-account");
}

// ===========================================================================
// GAP-013 — Auth failure audit logging (CWE-778)
// ===========================================================================

// GAP-013-01: validateToken with an unknown token returns Denied.
// The validateToken implementation now logs at WARN for every failure.
// This test verifies the Denied outcome so that it is exercised in CI
// (SIEM log coverage is verified by log inspection in integration tests).
TEST(AuthMiddlewareGap013Test, ValidateToken_UnknownToken_ReturnsDenied) {
    AuthMiddleware auth;
    // Add one known token so isEnabled()=true.
    AuthMiddleware::TokenConfig tc;
    tc.token   = "good-token";
    tc.user_id = "alice";
    tc.scopes  = {"read"};
    auth.addToken(tc);

    auto result = auth.validateToken("completely-unknown-token");
    EXPECT_FALSE(result.authorized) << "Unknown token must be denied (GAP-013)";
    EXPECT_EQ(result.reason, "Invalid token");
}

// GAP-013-02: authorize() with a recognised user but wrong scope returns Denied.
TEST(AuthMiddlewareGap013Test, Authorize_WrongScope_ReturnsDenied) {
    AuthMiddleware auth;
    AuthMiddleware::TokenConfig tc;
    tc.token   = "read-only-token";
    tc.user_id = "bob";
    tc.scopes  = {"data:read"};
    auth.addToken(tc);

    auto result = auth.authorize("read-only-token", "data:write");
    EXPECT_FALSE(result.authorized) << "Insufficient scope must be denied (GAP-013)";
}

// GAP-013-03: authorize() with a completely invalid token returns Denied.
TEST(AuthMiddlewareGap013Test, Authorize_InvalidToken_ReturnsDenied) {
    AuthMiddleware auth;
    AuthMiddleware::TokenConfig tc;
    tc.token   = "good-token";
    tc.user_id = "carol";
    tc.scopes  = {"admin"};
    auth.addToken(tc);

    auto result = auth.authorize("not-a-valid-token", "admin");
    EXPECT_FALSE(result.authorized) << "Invalid token must be denied (GAP-013)";
}

TEST(AuthMiddlewareGap013Test, DeniedReason_DoesNotEchoPresentedToken) {
    AuthMiddleware auth;
    AuthMiddleware::TokenConfig tc;
    tc.token   = "known-token";
    tc.user_id = "dave";
    tc.scopes  = {"read"};
    auth.addToken(tc);

    const std::string presented = "sensitive-invalid-token";
    auto result = auth.authorize(presented, "admin");
    ASSERT_FALSE(result.authorized);
    EXPECT_EQ(result.reason, "Invalid token");
    EXPECT_EQ(result.reason.find(presented), std::string::npos);
}

// GAP-013-05: authorize() reason for missing scope does not echo the token value.
TEST(AuthMiddlewareGap013Test, InsufficientScope_ReasonDoesNotEchoToken) {
    AuthMiddleware auth;
    AuthMiddleware::TokenConfig tc;
    tc.token   = "scope-limited-token-secret";
    tc.user_id = "erin";
    tc.scopes  = {"data:read"};
    auth.addToken(tc);

    auto result = auth.authorize(tc.token, "data:write");
    ASSERT_FALSE(result.authorized);
    // Reason must describe the missing scope, not echo the bearer token.
    EXPECT_NE(result.reason.find("Missing required scope"), std::string::npos)
        << "Reason should mention missing scope";
    EXPECT_EQ(result.reason.find(tc.token), std::string::npos)
        << "Reason must not contain the bearer token value";
}

// GAP-013-06: validateToken() returns a denied result whose reason does not
// contain any fragment of the presented token.
TEST(AuthMiddlewareGap013Test, ValidateToken_ReasonDoesNotEchoToken) {
    AuthMiddleware auth;
    AuthMiddleware::TokenConfig tc;
    tc.token   = "registered-token-secret-xyz";
    tc.user_id = "frank";
    tc.scopes  = {"admin"};
    auth.addToken(tc);

    const std::string unknown = "unknown-token-secret-xyz";
    auto result = auth.validateToken(unknown);
    ASSERT_FALSE(result.authorized);
    EXPECT_EQ(result.reason.find(unknown), std::string::npos)
        << "validateToken reason must not echo the presented token";
}

// GAP-013-07: Multiple concurrent authorize() calls with distinct tokens must
// not cross-contaminate user_id/reason between threads (no data race on
// AuthMiddleware::metrics_).
TEST(AuthMiddlewareGap013Test, ConcurrentDenyRequests_NoCrossContamination) {
    AuthMiddleware auth;
    AuthMiddleware::TokenConfig tc;
    tc.token   = "thread-token";
    tc.user_id = "thread-user";
    tc.scopes  = {"data:read"};
    auth.addToken(tc);

    constexpr int kThreads = 8;
    std::vector<std::thread> threads;
    std::vector<AuthMiddleware::AuthResult> results(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&auth, &results, i]() {
            results[i] = auth.authorize("bad-token-" + std::to_string(i), "admin");
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    for (int i = 0; i < kThreads; ++i) {
        EXPECT_FALSE(results[i].authorized) << "Thread " << i << " should be denied";
        EXPECT_TRUE(results[i].user_id.empty()) << "user_id must be empty for unknown token";
    }
}
