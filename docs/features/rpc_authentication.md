# RPC Service Authentication

**Version:** 1.4.2+  
**Status:** Production Ready  
**Last Updated:** April 2026

---

## Overview

ThemisDB RPC service supports comprehensive authentication and authorization via the integrated AuthMiddleware system. This document explains how to configure and use authentication for RPC endpoints.

---

## Authentication Methods

### 1. JWT Token Authentication (Recommended)

JWT (JSON Web Token) authentication with Keycloak/OIDC providers.

**Features:**
- RS256 signature verification
- JWKS (JSON Web Key Set) caching
- Claims extraction (user ID, email, roles, groups)
- Configurable clock skew tolerance
- Group-based access control

**Configuration:**

```cpp
// Server initialization
auto auth = std::make_shared<AuthMiddleware>();

AuthMiddleware::JWTConfig jwt_config{
    .jwks_url = "https://keycloak.example.com/realms/themis/protocol/openid-connect/certs",
    .expected_issuer = "https://keycloak.example.com/realms/themis",
    .expected_audience = "themis-app",
    .jwks_cache_ttl = std::chrono::seconds(3600),
    .clock_skew = std::chrono::seconds(60),
    .scope_claim = "roles"
};

auth->enableJWT(jwt_config);

// Create RPC service with auth
auto rpc_service = std::make_unique<ThemisRPCService>(
    storage,
    spatial_index,
    auth,
    &server_start_time
);
```

**Client Usage:**

```bash
# Obtain JWT token from your identity provider
TOKEN="eyJhbGciOiJSUzI1NiIs..."

# Make RPC call with Bearer token in metadata
grpcurl -H "Authorization: Bearer $TOKEN" \
    -d '{"model": "users", "collection": "accounts", "uuid": "user123"}' \
    localhost:50051 themis.rpc.ThemisRPC/Get
```

### 2. Static API Tokens

Pre-configured API tokens with specific scopes.

**Configuration:**

```cpp
AuthMiddleware::TokenConfig token_config{
    .token = "themis_api_key_abc123...",
    .user_id = "api-service",
    .scopes = {"rpc:read", "rpc:write", "admin"}
};

auth->addToken(token_config);
```

**Client Usage:**

```bash
TOKEN="themis_api_key_abc123..."

grpcurl -H "Authorization: Bearer $TOKEN" \
    -d '{"aql": "FOR doc IN users RETURN doc"}' \
    localhost:50051 themis.rpc.ThemisRPC/Query
```

### 3. Backward Compatibility (No Auth)

If AuthMiddleware is not configured or disabled, all requests are allowed.

```cpp
// RPC service without authentication (backward compatible)
auto rpc_service = std::make_unique<ThemisRPCService>(
    storage,
    spatial_index,
    nullptr,  // No auth middleware
    &server_start_time
);
```

---

## Authorization Scopes

RPC service enforces the following authorization scopes:

| Scope | Operations | Description |
|-------|-----------|-------------|
| `rpc:read` | GET, BatchGet, Search, Query, PaginatedQuery, VectorSearch, GraphTraverse, GeoQuery, TimeSeriesQuery, GetIndexOperations, ListCollections, GetCollectionMetadata, AggregationPipeline | Read-only operations |
| `rpc:write` | PUT, BatchPut, Delete, UpdateEntity, BatchUpdate | Write operations |
| `rpc:admin` | CreateIndex, DropIndex, Stats | Administrative operations |
| `transaction:write` | TransactionBegin, TransactionCommit, TransactionAbort | Transaction management |
| `admin` | All operations | Full administrative access (wildcard scope) |

**Important:** Scope checking is now enforced in the RPC service dispatch layer. Each method requires authentication and the appropriate scope. Requests with missing or invalid tokens are denied (fail-closed behavior).

**Health Check:** The `health_check` method does not require authentication for monitoring purposes.

**Authenticate Method:** The `authenticate` method itself does not require prior authentication.

---

## Token Verification Flow

```
┌─────────────┐
│   Client    │
└──────┬──────┘
       │ 1. gRPC Request with "Authorization: Bearer <token>"
       ▼
┌─────────────────┐
│  gRPC Plugin    │
│  (Metadata      │
│   Extraction)   │
└──────┬──────────┘
       │ 2. Pass metadata to RPC Service
       ▼
┌─────────────────┐
│  RPC Service    │
│  verifyAuth()   │
└──────┬──────────┘
       │ 3. Extract Bearer token
       ▼
┌─────────────────┐
│ AuthMiddleware  │
│  validateToken()│
└──────┬──────────┘
       │ 4. JWT validation or static token lookup
       ▼
┌─────────────────┐
│  JWKS/KeyStore  │
│  (Cached)       │
└──────┬──────────┘
       │ 5. Signature verification
       ▼
┌─────────────────┐
│  Return         │
│  AuthResult     │
│  (user, groups) │
└─────────────────┘
```

---

## Implementation Details

### RPC Service Constructor

```cpp
class ThemisRPCService {
public:
    explicit ThemisRPCService(
        RocksDBWrapper* storage,
        SpatialIndexManager* spatial_index = nullptr,
        std::shared_ptr<AuthMiddleware> auth = nullptr,
        const std::chrono::steady_clock::time_point* start_time = nullptr
    );
};
```

### Token Verification

```cpp
bool ThemisRPCService::verifyAuth(
    const RPCRequestContext& context,
    std::string& username
) {
    // Backward compatible: allow if auth not configured
    if (!auth_ || !auth_->isEnabled()) {
        username = context.username.empty() ? "anonymous" : context.username;
        return true;
    }
    
    // Extract token from gRPC metadata
    auto it = context.metadata.find("authorization");
    if (it == context.metadata.end()) {
        return false;
    }
    
    auto bearer_token = AuthMiddleware::extractBearerToken(it->second);
    if (!bearer_token) {
        return false;
    }
    
    // Validate using auth middleware
    auto result = auth_->validateToken(*bearer_token);
    if (!result.authorized) {
        return false;
    }
    
    username = result.user_id;
    return true;
}
```

---

## Health Check Enhancements

The health check endpoint now reports server uptime:

```json
{
  "status": "serving",
  "version": "1.4.2",
  "uptime_seconds": 3600
}
```

**Implementation:**

```cpp
json ThemisRPCService::handleHealthCheck(const json& params) {
    int64_t uptime_seconds = 0;
    if (start_time_) {
        auto now = std::chrono::steady_clock::now();
        uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(
            now - *start_time_
        ).count();
    }
    
    return createSuccess({
        {"status", "serving"},
        {"version", THEMIS_VERSION_STRING},
        {"uptime_seconds", uptime_seconds}
    });
}
```

---

## Optional Features

Several RPC endpoints require optional build-time modules:

### AQL Query Engine

Requires: `-DTHEMIS_ENABLE_AQL=ON`

```cpp
// If AQL module not enabled:
{
  "results": [],
  "note": "AQL query engine module not available. Use 'search' or 'paginated_query' for basic filtering."
}
```

**Alternatives:** Use `search` or `paginated_query` methods for simple filtering.

### Vector Search

Requires: `-DTHEMIS_ENABLE_VECTOR_INDEX=ON` (FAISS integration)

```cpp
{
  "results": [],
  "note": "Vector search requires vector index module (enable with -DTHEMIS_ENABLE_VECTOR_INDEX=ON)"
}
```

### Graph Traversal

Requires: `-DTHEMIS_ENABLE_GRAPH_INDEX=ON`

```cpp
{
  "vertices": [],
  "edges": [],
  "note": "Graph traversal requires graph index module (enable with -DTHEMIS_ENABLE_GRAPH_INDEX=ON)"
}
```

### Time Series Queries

Requires: `-DTHEMIS_ENABLE_TIMESERIES_INDEX=ON`

```cpp
{
  "buckets": [],
  "note": "Time series queries require time series index module (enable with -DTHEMIS_ENABLE_TIMESERIES_INDEX=ON)"
}
```

---

## Testing

### Integration Tests

The RPC service includes comprehensive integration tests:

1. **Authentication Handling** - Tests parameter validation and error messages
2. **Health Check with Uptime** - Verifies uptime tracking
3. **Optional Feature Messages** - Ensures clear messaging for unavailable modules
4. **Token Verification** - Tests JWT validation flow

**Run tests:**

```bash
cd build
ctest -R rpc_service_integration_test -V
```

### Manual Testing

**Test health check:**

```bash
grpcurl -plaintext \
    -d '{}' \
    localhost:50051 themis.rpc.ThemisRPC/HealthCheck
```

**Test authenticated request:**

```bash
TOKEN="your_jwt_token"

grpcurl -plaintext \
    -H "Authorization: Bearer $TOKEN" \
    -d '{"model": "users", "collection": "accounts", "uuid": "test123"}' \
    localhost:50051 themis.rpc.ThemisRPC/Get
```

---

## Security Best Practices

1. **Always enable authentication in production**
   - Never deploy with auth middleware disabled
   - Use JWT tokens from trusted identity provider
   - **Fail-closed behavior**: All endpoints deny access when authentication is enabled but token is missing/invalid

2. **Use HTTPS/TLS for gRPC**
   - Encrypt tokens in transit
   - Enable mutual TLS for enhanced security

3. **Configure appropriate scopes**
   - Follow principle of least privilege
   - Separate read and write access
   - Use built-in RBAC roles: admin, operator, analyst, readonly (see RBAC::getBuiltinRoles())

4. **Monitor authentication metrics**
   - Track failed authentication attempts
   - Set up alerts for anomalous patterns
   - Review audit logs regularly

5. **Rotate tokens regularly**
   - Use short-lived JWT tokens
   - Implement token refresh mechanism

6. **Scope-based authorization**
   - Each RPC method enforces specific scope requirements
   - Missing scope results in authentication failure
   - Admin scope provides wildcard access (*:*)

---

## Troubleshooting

### "Authentication not configured on server"

**Cause:** AuthMiddleware not initialized or no JWT config provided.

**Solution:** Configure AuthMiddleware with JWT or static tokens:

```cpp
auth->enableJWT(jwt_config);
```

### "Missing Authorization header"

**Cause:** Client not sending Authorization metadata.

**Solution:** Include Bearer token in gRPC metadata:

```bash
grpcurl -H "Authorization: Bearer $TOKEN" ...
```

### "Token verification failed"

**Possible causes:**
- Expired JWT token
- Invalid signature
- Incorrect JWKS URL
- Network issues fetching JWKS

**Solution:** 
- Check token expiration
- Verify JWKS endpoint is accessible
- Check AuthMiddleware logs for details

---

## Migration Guide

### From v1.4.1 (No Auth) to v1.4.2 (With Auth)

**Step 1:** Enable authentication in server:

```cpp
// Before
auto rpc = std::make_unique<ThemisRPCService>(storage, spatial_index);

// After
auto auth = std::make_shared<AuthMiddleware>();
auth->enableJWT(jwt_config);
auto rpc = std::make_unique<ThemisRPCService>(
    storage, spatial_index, auth, &start_time
);
```

**Step 2:** Update clients to send tokens:

```python
# Python gRPC client
metadata = [('authorization', f'Bearer {jwt_token}')]
response = stub.Get(request, metadata=metadata)
```

**Step 3:** Test backward compatibility:

```cpp
// During migration, auth can be optional
auto rpc = std::make_unique<ThemisRPCService>(
    storage, spatial_index, nullptr  // No auth during transition
);
```

---

## References

- [AuthMiddleware Documentation](../security/auth_middleware.md)
- [JWT Validation Guide](../security/jwt_validation.md)
- [RPC Service Implementation](../../src/server/rpc/rpc_service_impl.cpp)
- [Integration Tests](../../tests/integration/rpc/rpc_service_integration_test.cpp)

---

**Audit Trail:**
- v1.4.2 (Feb 2026): Authentication integration completed, all TODOs resolved
- FIND-001 (CODE_QUALITY_AUDIT.md): RESOLVED
