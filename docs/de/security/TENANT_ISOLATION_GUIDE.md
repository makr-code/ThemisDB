# Multi-Tenant Isolation Guide

## Overview

ThemisDB provides production-ready multi-tenant isolation with fail-closed security. Each tenant has its own isolated namespace with resource quotas, usage tracking, and access control enforcement.

## Key Features

- **Tenant Identification**: Via `X-Tenant-ID` header, path prefix (`/tenants/{id}/...`), or JWT claim
- **Fail-Closed Security**: Requests without valid tenant context are rejected (default behavior)
- **Resource Quotas**: Per-tenant limits for storage, documents, collections, connections, and queries
- **Usage Tracking**: Real-time metrics for tenant resource consumption
- **Cross-Tenant Protection**: Prevents unauthorized access to other tenants' data

## Configuration

### Secure Default Behavior (Production)

By default, ThemisDB requires explicit tenant identification. No requests are allowed without a valid tenant context:

```cpp
// Default configuration (secure)
TenantManager::Config config;
config.allow_default_tenant = false;  // Secure default
config.tenant_header = "X-Tenant-ID";
config.tenant_path_prefix = "/tenants/";
config.enforce_quotas = true;
```

### Backward Compatibility Mode

For single-tenant deployments or migration scenarios:

```cpp
// Enable backward compatibility
TenantManager::Config config;
config.allow_default_tenant = true;
config.default_tenant_id = "default";
config.tenant_header = "X-Tenant-ID";
config.tenant_path_prefix = "/tenants/";
```

## Tenant Management

### Creating Tenants

```cpp
#include "server/tenant_manager.h"

auto& tm = TenantManager::instance();

TenantConfig config;
config.tenant_id = "acme-corp";
config.display_name = "ACME Corporation";
config.enabled = true;

// Resource quotas
config.max_storage_bytes = 100ULL * 1024 * 1024 * 1024;  // 100 GB
config.max_documents = 10000000;  // 10 million documents
config.max_collections = 1000;
config.max_concurrent_queries = 100;
config.max_connections = 50;

// Rate limiting
config.requests_per_second = 1000;
config.burst_size = 100;

// Feature flags
config.allow_gpu_acceleration = true;
config.allow_vector_search = true;

// Create tenant
auto result = tm.createTenant(config);
if (result == TenantManager::CreateResult::Success) {
    // Tenant created successfully
}
```

### Updating Tenants

```cpp
// Update tenant configuration
config.max_storage_bytes = 200ULL * 1024 * 1024 * 1024;  // Increase to 200 GB
tm.updateTenant(config);
```

### Disabling Tenants

```cpp
// Temporarily disable tenant (preserves data)
tm.setTenantEnabled("acme-corp", false);

// Re-enable tenant
tm.setTenantEnabled("acme-corp", true);
```

## Request Authentication and Tenant Resolution

### Method 1: X-Tenant-ID Header (Recommended)

Most flexible approach - works with any endpoint:

```bash
curl -H "X-Tenant-ID: acme-corp" \
     -H "Authorization: Bearer <token>" \
     https://themis.example.com/api/documents
```

### Method 2: Path-Based Routing

Tenant ID embedded in URL path. The server automatically strips the `/tenants/{id}/`
prefix before dispatching the request, so the same handler logic is used for both
header-based and path-based routing:

```bash
# These two requests are functionally equivalent after server-side path rewriting:
curl -H "X-Tenant-ID: acme-corp" \
     -H "Authorization: Bearer <token>" \
     https://themis.example.com/api/documents

curl -H "Authorization: Bearer <token>" \
     https://themis.example.com/tenants/acme-corp/api/documents
```

The server rewrites `GET /tenants/acme-corp/api/documents` to
`GET /api/documents` + `X-Tenant-ID: acme-corp` before routing. No additional
configuration is required; path-based routing is always enabled when the
`tenant_path_prefix` (default: `/tenants/`) is set in `TenantManager::Config`.

### Method 3: JWT Claim (Most Secure)

Tenant bound to JWT token (prevents header spoofing):

```json
{
  "sub": "user@acme-corp.com",
  "tenant_id": "acme-corp",
  "roles": ["admin"],
  "exp": 1709107200
}
```

```bash
curl -H "Authorization: Bearer <jwt-with-tenant-claim>" \
     https://themis.example.com/api/documents
```

## JWT Configuration

### Configuring JWT with Tenant Claims

```cpp
#include "server/auth_middleware.h"

AuthMiddleware auth;
AuthMiddleware::JWTConfig jwt_config;

jwt_config.jwks_url = "https://keycloak.example.com/realms/production/protocol/openid-connect/certs";
jwt_config.expected_issuer = "https://keycloak.example.com/realms/production";
jwt_config.expected_audience = "themisdb";
jwt_config.tenant_claim = "tenant_id";  // JWT claim containing tenant ID
jwt_config.scope_claim = "roles";       // JWT claim containing scopes

auth.enableJWT(jwt_config);
```

### JWT Token Format

```json
{
  "iss": "https://keycloak.example.com/realms/production",
  "sub": "user@acme-corp.com",
  "aud": "themisdb",
  "exp": 1709107200,
  "iat": 1709103600,
  "tenant_id": "acme-corp",
  "roles": ["user", "admin"],
  "groups": ["engineering", "data-science"]
}
```

## API Token Configuration

For service accounts and API integrations:

```cpp
AuthMiddleware auth;

AuthMiddleware::TokenConfig token;
token.token = "sk_acme_1234567890abcdef";
token.user_id = "api-service-account";
token.tenant_id = "acme-corp";
token.scopes = {"read", "write", "admin"};

auth.addToken(token);
```

## Cross-Tenant Access Protection

### Automatic Validation

The system automatically validates tenant access on every request:

```cpp
// In API handlers (example from changefeed_api_handler.cpp)
TenantAuthContext tenant_ctx;
if (auto auth_resp = checkAuthAndResolveTenant(req, "cdc:read", tenant_ctx)) {
    return *auth_resp;  // Return 403 Forbidden
}

// tenant_ctx.tenant_id is validated and safe to use
// tenant_ctx.user_id contains authenticated user
```

### Validation Logic

1. Extract tenant from JWT claim (if present)
2. Extract tenant from request headers/path
3. **Fail if missing**: Return 400 Bad Request
4. **Verify JWT tenant matches request tenant**: Return 403 Forbidden on mismatch
5. Validate tenant exists and is enabled: Return 403 Forbidden if invalid
6. Record usage metrics

## Quota Enforcement

### Checking Quotas Before Operations

```cpp
auto& tm = TenantManager::instance();

// Check storage quota before write
auto quota_check = tm.checkQuota("acme-corp", "storage", write_size_bytes);
if (!quota_check.allowed) {
    // Return error: quota_check.reason
    return makeErrorResponse(429, quota_check.reason);
}

// Perform write operation
tm.incrementStorage("acme-corp", write_size_bytes);
tm.recordBytesWritten("acme-corp", write_size_bytes);
```

### Available Resource Types

- `storage` - Total bytes stored
- `documents` - Number of documents
- `collections` - Number of collections
- `connections` - Active connections
- `queries` - Concurrent queries

### Usage Tracking

```cpp
// Track requests
tm.recordRequest("acme-corp");

// Track queries
tm.recordQuery("acme-corp");

// Track data transfer
tm.recordBytesRead("acme-corp", bytes_read);
tm.recordBytesWritten("acme-corp", bytes_written);

// Track rate limiting
tm.recordRateLimited("acme-corp");
```

## Connection Management

### RAII-Based Connection Tracking

```cpp
// Automatic connection tracking with TenantContextGuard
TenantContext ctx = TenantContext::fromConfig(tenant_config, user_id, roles);
TenantContextGuard guard(ctx);

if (!guard.hasConnection()) {
    // Connection limit exceeded
    return makeErrorResponse(429, "Connection limit exceeded");
}

// Acquire query slot if needed
if (is_query && !guard.acquireQuerySlot()) {
    return makeErrorResponse(429, "Concurrent query limit exceeded");
}

// Process request
// ...

// Connections/queries automatically released when guard goes out of scope
```

## Metrics and Monitoring

### Prometheus Metrics

```cpp
auto& tm = TenantManager::instance();
std::string metrics = tm.getMetrics();
```

Available metrics:

```prometheus
# Tenant count
themis_tenant_count{} 42

# Per-tenant metrics
themis_tenant_storage_bytes{tenant="acme-corp"} 52428800
themis_tenant_documents{tenant="acme-corp"} 1500
themis_tenant_connections{tenant="acme-corp"} 5
themis_tenant_queries_active{tenant="acme-corp"} 2
themis_tenant_requests_total{tenant="acme-corp"} 125000
themis_tenant_rate_limited_total{tenant="acme-corp"} 150
```

## Error Responses

### Missing Tenant ID

```json
{
  "error": "missing_tenant",
  "message": "Tenant ID must be provided via X-Tenant-ID header, path, or JWT claim"
}
```
**HTTP Status**: 400 Bad Request

### Invalid or Disabled Tenant

```json
{
  "error": "invalid_tenant",
  "message": "Tenant not found or disabled"
}
```
**HTTP Status**: 403 Forbidden

### Tenant Mismatch

```json
{
  "error": "tenant_mismatch",
  "message": "Tenant ID in request does not match authenticated tenant"
}
```
**HTTP Status**: 403 Forbidden

### Quota Exceeded

```json
{
  "error": "quota_exceeded",
  "message": "Storage quota exceeded"
}
```
**HTTP Status**: 429 Too Many Requests

## Best Practices

### 1. Use JWT Claims for Tenant Binding

Most secure approach - prevents tenant spoofing:

```json
{
  "tenant_id": "acme-corp",
  "sub": "user@acme-corp.com"
}
```

### 2. Validate Tenant in Every Handler

Always use tenant validation helpers:

```cpp
TenantAuthContext tenant_ctx;
if (auto auth_resp = checkAuthAndResolveTenant(req, required_scope, tenant_ctx)) {
    return *auth_resp;
}
// Use tenant_ctx.tenant_id for all operations
```

### 3. Set Appropriate Quotas

Balance usability with resource protection:

```cpp
// Development tenant
config.max_storage_bytes = 10ULL * 1024 * 1024 * 1024;  // 10 GB
config.max_documents = 1000000;

// Production tenant
config.max_storage_bytes = 1000ULL * 1024 * 1024 * 1024;  // 1 TB
config.max_documents = 100000000;
```

### 4. Monitor Tenant Usage

Track usage patterns to detect anomalies:

```cpp
// Get tenant usage
auto* usage = tm.getUsage("acme-corp");
if (usage) {
    uint64_t storage_used = usage->storage_bytes_used.load();
    uint64_t doc_count = usage->document_count.load();
    uint32_t active_conns = usage->active_connections.load();
}
```

### 5. Implement Rate Limiting

Configure per-tenant rate limits:

```cpp
config.requests_per_second = 1000;
config.burst_size = 100;
```

## Security Considerations

### Fail-Closed by Default

- All requests without valid tenant context are rejected
- No implicit default tenant in production mode
- Cross-tenant access attempts are logged and blocked

### Tenant Isolation Guarantees

1. **Authentication**: Every request must be authenticated
2. **Authorization**: User must have access to requested tenant
3. **Validation**: Tenant must exist and be enabled
4. **Consistency**: JWT tenant must match request tenant (if both present)
5. **Auditing**: All tenant access is logged

### Defense in Depth

Multiple layers of tenant validation:
- Authentication layer (JWT/token validation)
- Request layer (tenant resolution)
- Handler layer (tenant context enforcement)
- Storage layer (tenant-prefixed keys)

## Migration Guide

### Migrating from Single-Tenant to Multi-Tenant

1. **Enable backward compatibility mode**:
   ```cpp
   config.allow_default_tenant = true;
   ```

2. **Create tenant for existing data**:
   ```cpp
   TenantConfig legacy_config;
   legacy_config.tenant_id = "default";
   legacy_config.display_name = "Legacy Tenant";
   tm.createTenant(legacy_config);
   ```

3. **Update clients to send X-Tenant-ID header**:
   ```bash
   curl -H "X-Tenant-ID: default" ...
   ```

4. **After migration, switch to secure mode**:
   ```cpp
   config.allow_default_tenant = false;
   ```

## Troubleshooting

### Tenant Not Found Errors

**Symptom**: `{"error": "invalid_tenant"}`

**Solutions**:
- Verify tenant exists: `tm.tenantExists("tenant-id")`
- Check tenant is enabled: `tm.getTenant("tenant-id")->enabled`
- Ensure tenant created before first request

### Quota Exceeded Errors

**Symptom**: `{"error": "quota_exceeded"}`

**Solutions**:
- Check current usage: `tm.getUsage("tenant-id")`
- Increase quotas: `tm.updateTenant(updated_config)`
- Clean up old data to free resources

### Cross-Tenant Access Denied

**Symptom**: `{"error": "tenant_mismatch"}`

**Solutions**:
- Verify JWT tenant claim matches request header
- Check JWT not reused across tenants
- Ensure correct tenant specified in request

## See Also

- [Authentication Guide](AUTH_GUIDE.md)
- [RBAC Authorization](guides_rbac_authorization.md)
- [Security Best Practices](SECURITY.md)
- [Monitoring and Metrics](MONITORING.md)
