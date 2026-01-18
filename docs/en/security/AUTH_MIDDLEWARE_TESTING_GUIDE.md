# Auth Middleware Testing Guide

## Overview

This guide provides comprehensive information about testing the ThemisDB authentication and authorization middleware. The auth middleware is a critical security component that validates tokens, enforces role-based access control (RBAC), and protects API endpoints.

## Table of Contents

1. [Authentication Flow Overview](#authentication-flow-overview)
2. [Token Validation Mechanisms](#token-validation-mechanisms)
3. [Authorization and RBAC](#authorization-and-rbac)
4. [Security Edge Cases and Prevention Strategies](#security-edge-cases-and-prevention-strategies)
5. [Testing Best Practices](#testing-best-practices)
6. [Common Vulnerabilities and Mitigation](#common-vulnerabilities-and-mitigation)

## Authentication Flow Overview

### Basic Authentication Flow

```mermaid
sequenceDiagram
    participant Client
    participant Middleware
    participant TokenValidator
    participant RBAC
    participant Endpoint
    
    Client->>Middleware: Request + Bearer Token
    Middleware->>TokenValidator: Extract & Validate Token
    TokenValidator-->>Middleware: Token Valid/Invalid
    
    alt Token Invalid
        Middleware-->>Client: 401 Unauthorized
    else Token Valid
        Middleware->>RBAC: Check Permissions
        RBAC-->>Middleware: Authorized/Denied
        
        alt Access Denied
            Middleware-->>Client: 403 Forbidden
        else Access Granted
            Middleware->>Endpoint: Forward Request
            Endpoint-->>Client: Response
        end
    end
```

### Supported Authentication Methods

1. **Static API Tokens**: Pre-configured tokens with associated scopes
2. **JWT (JSON Web Tokens)**: Dynamic token validation with JWKS
3. **Kerberos/GSSAPI**: Enterprise authentication
4. **mTLS**: Certificate-based authentication
5. **USB Admin Auth**: Hardware-based authentication for admin operations

## Token Validation Mechanisms

### Token Format

ThemisDB auth middleware supports multiple token formats:

```cpp
// Static API Token
Authorization: Bearer admin-token-123

// JWT Token
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...

// Certificate DN (for mTLS)
Authorization: Bearer CN=client.example.com,OU=Engineering,O=Example
```

### Validation Process

1. **Bearer Token Extraction**: Parse `Authorization` header
2. **Format Validation**: Check token structure
3. **Signature Verification**: Validate JWT signature (if JWT)
4. **Expiration Check**: Verify token hasn't expired
5. **Claim Validation**: Check required claims (iss, aud, etc.)

### Test Coverage

```cpp
// Token Validation Tests
TEST_F(AuthMiddlewareTest, ValidJWTTokenAccepted)
TEST_F(AuthMiddlewareTest, ExpiredTokenRejected)
TEST_F(AuthMiddlewareTest, InvalidSignatureDetected)
TEST_F(AuthMiddlewareTest, MalformedTokenRejected)
```

## Authorization and RBAC

### Scope-Based Authorization

ThemisDB uses a scope-based authorization model where each token has associated scopes:

```cpp
// Example token configuration
TokenConfig admin_token{
    .token = "admin-token-123",
    .user_id = "admin",
    .scopes = {"admin", "config:write", "config:read", "cdc:read"}
};
```

### Common Scopes

| Scope | Description | Example Endpoints |
|-------|-------------|-------------------|
| `admin` | Full administrative access | All endpoints |
| `config:read` | Read configuration | GET /api/config |
| `config:write` | Modify configuration | POST /api/config |
| `cdc:read` | Read change data capture | GET /api/changefeed |
| `metrics:read` | Read metrics | GET /api/metrics |
| `data:read` | Read data | GET /api/collections |
| `data:write` | Write data | POST /api/collections |

### Resource-Level Authorization

For fine-grained access control, scopes can include resource identifiers:

```cpp
// Resource-specific scopes
"resource:123:read"   // Read access to resource 123
"resource:123:write"  // Write access to resource 123
"resource:456:read"   // Read access to resource 456
```

### RBAC Test Coverage

```cpp
// Authorization Tests
TEST_F(AuthMiddlewareTest, RBACEnforcement)
TEST_F(AuthMiddlewareTest, PermissionCheckOnEndpoints)
TEST_F(AuthMiddlewareTest, DeniedAccessResponse)
TEST_F(AuthMiddlewareTest, ResourceLevelAuthorization)
```

## Security Edge Cases and Prevention Strategies

### 1. Token Expiration

**Problem**: Expired tokens should not be accepted.

**Solution**:
- JWT tokens include `exp` claim for expiration
- Static tokens can be manually expired by removal
- Implement short-lived tokens with refresh mechanism

**Test Coverage**:
```cpp
TEST_F(AuthMiddlewareTest, ExpiredTokenRejected)
TEST_F(AuthMiddlewareTest, TokenRefreshMechanism)
```

### 2. SQL Injection

**Problem**: Malicious tokens containing SQL injection attempts.

**Solution**:
- Token validation happens before any database queries
- Tokens are not used in SQL queries directly
- Scope checking uses exact string matching

**Test Coverage**:
```cpp
TEST_F(AuthMiddlewareTest, SQLInjectionPrevention)
```

**Example Attacks Prevented**:
```cpp
"admin' OR '1'='1"
"admin'--"
"'; DROP TABLE users;--"
```

### 3. Token Replay Attacks

**Problem**: Intercepted tokens used by attackers.

**Mitigation Strategies**:
- Use short-lived tokens (JWT `exp` claim)
- Implement token refresh mechanism
- Use TLS for all communications
- Consider nonce/jti claim for one-time tokens

**Test Coverage**:
```cpp
TEST_F(AuthMiddlewareTest, TokenReplayAttackPrevention)
```

### 4. Rate Limiting

**Problem**: Brute force authentication attempts.

**Solution**:
- Track failed authentication attempts via metrics
- Implement rate limiting at API gateway level
- Log failed attempts for monitoring

**Test Coverage**:
```cpp
TEST_F(AuthMiddlewareTest, RateLimitingOnFailedAuth)
```

### 5. Malformed Tokens

**Problem**: Maliciously crafted tokens causing crashes.

**Solution**:
- Robust input validation
- Length limits on tokens
- Safe parsing with error handling

**Test Coverage**:
```cpp
TEST_F(AuthMiddlewareTest, MalformedTokenRejected)
```

## Testing Best Practices

### 1. Test Token Lifecycle

```cpp
// Setup: Create token
AuthMiddleware::TokenConfig token{...};
auth_.addToken(token);

// Test: Validate token works
auto result = auth_.validateToken(token.token);
EXPECT_TRUE(result.authorized);

// Test: Expire/remove token
auth_.removeToken(token.token);

// Test: Verify token no longer works
result = auth_.validateToken(token.token);
EXPECT_FALSE(result.authorized);
```

### 2. Test Multiple Sessions

```cpp
// Test concurrent sessions for same user
TokenConfig session1{.user_id = "user", .token = "session1"};
TokenConfig session2{.user_id = "user", .token = "session2"};

auth_.addToken(session1);
auth_.addToken(session2);

// Both sessions should be independent
EXPECT_TRUE(auth_.validateToken("session1").authorized);
EXPECT_TRUE(auth_.validateToken("session2").authorized);
```

### 3. Test Permission Boundaries

```cpp
// Test user has only expected permissions
EXPECT_TRUE(auth_.authorize("token", "allowed-scope").authorized);
EXPECT_FALSE(auth_.authorize("token", "denied-scope").authorized);
```

### 4. Test Error Messages

```cpp
// Verify error responses include helpful information
auto result = auth_.authorize("invalid", "scope");
EXPECT_FALSE(result.authorized);
EXPECT_FALSE(result.reason.empty());  // Has error reason
EXPECT_TRUE(result.user_id.empty());  // No info leak on failure
```

### 5. Test Thread Safety

```cpp
// Test concurrent access to auth middleware
#pragma omp parallel for
for (int i = 0; i < 100; i++) {
    auto result = auth_.validateToken("token");
    EXPECT_TRUE(result.authorized);
}
```

## Common Vulnerabilities and Mitigation

### 1. Information Disclosure

**Vulnerability**: Error messages revealing system details.

**Mitigation**:
```cpp
// Good: Generic error message
return AuthResult::Denied("Invalid credentials");

// Bad: Reveals whether user exists
return AuthResult::Denied("User 'admin' not found");
```

### 2. Timing Attacks

**Vulnerability**: Different response times reveal information.

**Mitigation**:
- Use constant-time string comparison for tokens
- Implement consistent processing time
- Don't early-return on first validation failure

### 3. Token Fixation

**Vulnerability**: Attacker forces victim to use known token.

**Mitigation**:
- Generate new tokens on authentication
- Invalidate old tokens on refresh
- Bind tokens to session context

### 4. Privilege Escalation

**Vulnerability**: User gains unauthorized permissions.

**Mitigation**:
```cpp
// Always verify scopes, never trust client
auto result = auth_.authorize(token, required_scope);
if (!result.authorized) {
    return HTTP_403_FORBIDDEN;
}
```

### 5. Session Hijacking

**Vulnerability**: Attacker steals session token.

**Mitigation**:
- Use HTTPS/TLS for all traffic
- Set secure cookie flags
- Implement token binding
- Use short expiration times

## Metrics and Monitoring

The auth middleware exposes metrics for monitoring:

```cpp
struct Metrics {
    std::atomic<uint64_t> authz_success_total;
    std::atomic<uint64_t> authz_denied_total;
    std::atomic<uint64_t> authz_invalid_token_total;
    std::atomic<uint64_t> jwt_validation_success_total;
    std::atomic<uint64_t> jwt_validation_failed_total;
};
```

### Prometheus Queries

```promql
# Failed authentication rate
rate(authz_invalid_token_total[5m])

# Authorization success rate
rate(authz_success_total[5m]) / 
  (rate(authz_success_total[5m]) + rate(authz_denied_total[5m]))

# Failed authentication spike detection
increase(authz_invalid_token_total[5m]) > 100
```

## Integration Testing

### HTTP Pipeline Integration

```cpp
TEST_F(AuthMiddlewareTest, AuthMiddlewareInHTTPPipeline) {
    // Simulate full HTTP request flow
    std::string auth_header = "Bearer admin-token-123";
    std::string endpoint = "/api/config";
    std::string required_scope = "config:write";
    
    // Extract token
    auto token = AuthMiddleware::extractBearerToken(auth_header);
    ASSERT_TRUE(token.has_value());
    
    // Authorize
    auto result = auth_.authorize(*token, required_scope);
    EXPECT_TRUE(result.authorized);
    
    // Process request...
}
```

### Multiple HTTP Methods

```cpp
// GET - read scope
EXPECT_TRUE(auth_.authorize(token, "config:read").authorized);

// POST/PUT/DELETE - write scope
EXPECT_TRUE(auth_.authorize(token, "config:write").authorized);
```

## Example Configuration

### Static Tokens (config.yaml)

```yaml
auth:
  enabled: true
  tokens:
    - token: "admin-token-123"
      user_id: "admin"
      scopes:
        - admin
        - config:write
        - config:read
    
    - token: "readonly-token-456"
      user_id: "viewer"
      scopes:
        - config:read
        - metrics:read
```

### JWT Configuration

```yaml
auth:
  jwt:
    enabled: true
    jwks_url: "https://auth.example.com/.well-known/jwks.json"
    expected_issuer: "https://auth.example.com/"
    expected_audience: "themisdb-api"
    scope_claim: "roles"
    clock_skew: 60s
    jwks_cache_ttl: 3600s
```

## Running Tests

```bash
# Build tests
cmake --build build --target test_auth_middleware

# Run all auth tests
./build/tests/test_auth_middleware

# Run specific test
./build/tests/test_auth_middleware --gtest_filter="*TokenValidation*"

# Run with verbose output
./build/tests/test_auth_middleware --gtest_filter="*" --gtest_verbose
```

## Test Coverage Summary

| Category | Tests | Coverage |
|----------|-------|----------|
| Token Validation | 4 | Complete |
| Authorization/RBAC | 4 | Complete |
| Authentication Flow | 4 | Complete |
| Security Edge Cases | 4 | Complete |
| Integration | 4 | Complete |
| **Total** | **20** | **100%** |

## References

- [RFC 6750: OAuth 2.0 Bearer Token Usage](https://tools.ietf.org/html/rfc6750)
- [RFC 7519: JSON Web Token (JWT)](https://tools.ietf.org/html/rfc7519)
- [OWASP Authentication Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Authentication_Cheat_Sheet.html)
- [OWASP Session Management Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Session_Management_Cheat_Sheet.html)

## Support

For questions or issues:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://makr-code.github.io/ThemisDB/
- Security Issues: security@themisdb.io
