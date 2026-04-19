# Tenant Isolation Quick Reference

## For API Developers

### Required Headers

Every API request must include:

```
Authorization: Bearer <token>
X-Tenant-ID: <tenant-id>
```

Or use path-based routing:
```
GET /tenants/<tenant-id>/api/documents
```

### Adding Tenant Validation to API Handlers

```cpp
#include "server/tenant_manager.h"

// In your API handler
http::response<http::string_body> MyApiHandler::handleGet(const http::request<http::string_body>& req) {
    // Step 1: Authenticate and resolve tenant
    TenantAuthContext tenant_ctx;
    if (auto auth_resp = checkAuthAndResolveTenant(req, "required:scope", tenant_ctx)) {
        return *auth_resp;  // Return error response (401/403/400)
    }
    
    // Step 2: Use tenant context
    std::string tenant_id = tenant_ctx.tenant_id;
    std::string user_id = tenant_ctx.user_id;
    
    // Step 3: Check quota before writes
    auto& tm = TenantManager::instance();
    auto quota_check = tm.checkQuota(tenant_id, "documents", 1);
    if (!quota_check.allowed) {
        return makeErrorResponse(429, quota_check.reason);
    }
    
    // Step 4: Perform operation
    // ... your logic here ...
    
    // Step 5: Track usage
    tm.incrementDocuments(tenant_id, 1);
    tm.recordBytesWritten(tenant_id, data_size);
    
    return makeResponse(200, response_body);
}
```

### Implementing checkAuthAndResolveTenant

Add to your handler header:

```cpp
struct TenantAuthContext {
    std::string user_id;
    std::string tenant_id;
    std::vector<std::string> groups;
};

std::optional<http::response<http::string_body>> checkAuthAndResolveTenant(
    const http::request<http::string_body>& req,
    const std::string& required_scope,
    TenantAuthContext& out_context
);
```

Implementation template (copy from changefeed_api_handler.cpp):

```cpp
std::optional<http::response<http::string_body>> MyHandler::checkAuthAndResolveTenant(
    const http::request<http::string_body>& req,
    const std::string& required_scope,
    TenantAuthContext& out_context
) {
    // 1. Authenticate
    auto it = req.find(http::field::authorization);
    if (it == req.end()) {
        return makeErrorResponse(401, "Missing Authorization header");
    }
    
    auto token = AuthMiddleware::extractBearerToken(it->value());
    if (!token) {
        return makeErrorResponse(401, "Invalid Authorization header");
    }
    
    auto auth_result = auth_->authorize(*token, required_scope);
    if (!auth_result.authorized) {
        return makeErrorResponse(403, "Insufficient scope");
    }
    
    // 2. Resolve tenant
    auto& tm = TenantManager::instance();
    std::string tenant_id_from_auth = auth_result.tenant_id;
    
    std::unordered_map<std::string, std::string> headers_map;
    for (const auto& h : req) {
        headers_map[std::string(h.name_string())] = std::string(h.value());
    }
    
    auto tenant_id_from_request = tm.extractTenantId(headers_map, std::string(req.target()));
    
    std::string final_tenant_id;
    if (!tenant_id_from_auth.empty()) {
        final_tenant_id = tenant_id_from_auth;
        if (tenant_id_from_request && *tenant_id_from_request != tenant_id_from_auth) {
            return makeErrorResponse(403, "Tenant mismatch");
        }
    } else if (tenant_id_from_request) {
        final_tenant_id = *tenant_id_from_request;
    } else {
        return makeErrorResponse(400, "Missing tenant ID");
    }
    
    // 3. Validate tenant
    auto tenant_config = tm.getTenant(final_tenant_id);
    if (!tenant_config || !tenant_config->enabled) {
        return makeErrorResponse(403, "Invalid tenant");
    }
    
    // 4. Set output context
    out_context.user_id = auth_result.user_id;
    out_context.tenant_id = final_tenant_id;
    out_context.groups = auth_result.groups;
    
    tm.recordRequest(final_tenant_id);
    
    return std::nullopt;  // Success
}
```

## For Client Developers

### Making Authenticated Requests

#### Using API Token

```bash
curl -X GET \
  -H "Authorization: Bearer sk_tenant123_abc..." \
  -H "X-Tenant-ID: tenant123" \
  https://api.themis.example.com/api/documents
```

#### Using JWT Token

```bash
# JWT already contains tenant_id claim
curl -X GET \
  -H "Authorization: Bearer eyJhbGc..." \
  https://api.themis.example.com/api/documents
```

#### Using Path-Based Routing

```bash
curl -X GET \
  -H "Authorization: Bearer sk_tenant123_abc..." \
  https://api.themis.example.com/tenants/tenant123/api/documents
```

### Client Libraries

#### Python

```python
import requests

class ThemisClient:
    def __init__(self, base_url, tenant_id, token):
        self.base_url = base_url
        self.tenant_id = tenant_id
        self.headers = {
            'Authorization': f'Bearer {token}',
            'X-Tenant-ID': tenant_id
        }
    
    def get_documents(self):
        response = requests.get(
            f'{self.base_url}/api/documents',
            headers=self.headers
        )
        return response.json()

# Usage
client = ThemisClient(
    base_url='https://api.themis.example.com',
    tenant_id='acme-corp',
    token='sk_acme_...'
)
docs = client.get_documents()
```

#### JavaScript/TypeScript

```typescript
class ThemisClient {
  constructor(
    private baseUrl: string,
    private tenantId: string,
    private token: string
  ) {}

  async getDocuments() {
    const response = await fetch(`${this.baseUrl}/api/documents`, {
      headers: {
        'Authorization': `Bearer ${this.token}`,
        'X-Tenant-ID': this.tenantId
      }
    });
    return response.json();
  }
}

// Usage
const client = new ThemisClient(
  'https://api.themis.example.com',
  'acme-corp',
  'sk_acme_...'
);
const docs = await client.getDocuments();
```

#### Go

```go
package themis

import (
    "net/http"
    "io/ioutil"
)

type Client struct {
    BaseURL  string
    TenantID string
    Token    string
}

func (c *Client) GetDocuments() ([]byte, error) {
    req, err := http.NewRequest("GET", c.BaseURL+"/api/documents", nil)
    if err != nil {
        return nil, err
    }
    
    req.Header.Set("Authorization", "Bearer "+c.Token)
    req.Header.Set("X-Tenant-ID", c.TenantID)
    
    resp, err := http.DefaultClient.Do(req)
    if err != nil {
        return nil, err
    }
    defer resp.Body.Close()
    
    return ioutil.ReadAll(resp.Body)
}

// Usage
client := &Client{
    BaseURL:  "https://api.themis.example.com",
    TenantID: "acme-corp",
    Token:    "sk_acme_...",
}
docs, err := client.GetDocuments()
```

## For DevOps/Operations

### Tenant Configuration

#### Create Tenant

```bash
curl -X POST https://api.themis.example.com/admin/tenants \
  -H "Authorization: Bearer ${ADMIN_TOKEN}" \
  -H "Content-Type: application/json" \
  -d '{
    "tenant_id": "acme-corp",
    "display_name": "ACME Corporation",
    "max_storage_bytes": 107374182400,
    "max_documents": 10000000,
    "max_collections": 1000,
    "max_connections": 50,
    "max_concurrent_queries": 100,
    "requests_per_second": 1000
  }'
```

#### Update Quotas

```bash
curl -X PUT https://api.themis.example.com/admin/tenants/acme-corp \
  -H "Authorization: Bearer ${ADMIN_TOKEN}" \
  -H "Content-Type: application/json" \
  -d '{
    "max_storage_bytes": 214748364800
  }'
```

#### Disable Tenant

```bash
curl -X POST https://api.themis.example.com/admin/tenants/acme-corp/disable \
  -H "Authorization: Bearer ${ADMIN_TOKEN}"
```

### Monitoring

#### Prometheus Metrics

```bash
curl https://api.themis.example.com/metrics
```

Example queries:
```promql
# Total storage used by tenant
themis_tenant_storage_bytes{tenant="acme-corp"}

# Request rate by tenant
rate(themis_tenant_requests_total{tenant="acme-corp"}[5m])

# Active connections by tenant
themis_tenant_connections{tenant="acme-corp"}

# Query concurrency by tenant
themis_tenant_queries_active{tenant="acme-corp"}

# Rate-limited requests
rate(themis_tenant_rate_limited_total{tenant="acme-corp"}[5m])
```

### Configuration Examples

#### Development Environment

```yaml
# config/dev.yaml
tenants:
  allow_default_tenant: true
  default_tenant_id: "dev"
  enforce_quotas: false
```

#### Production Environment

```yaml
# config/prod.yaml
tenants:
  allow_default_tenant: false  # Require explicit tenant
  enforce_quotas: true
  global_max_tenants: 1000
  tenant_header: "X-Tenant-ID"
  tenant_path_prefix: "/tenants/"
```

## Common Error Codes

| Code | Error | Cause | Solution |
|------|-------|-------|----------|
| 400 | `missing_tenant` | No tenant ID provided | Add X-Tenant-ID header or use path |
| 401 | `missing_authorization` | No Authorization header | Add Bearer token |
| 401 | `invalid_token` | Token malformed or expired | Refresh token |
| 403 | `invalid_tenant` | Tenant not found/disabled | Check tenant exists and enabled |
| 403 | `tenant_mismatch` | JWT tenant != request tenant | Use consistent tenant ID |
| 403 | `insufficient_scope` | Missing required permission | Request appropriate scope |
| 429 | `quota_exceeded` | Resource limit reached | Contact admin to increase quota |

## Testing

### Unit Tests

```cpp
TEST(TenantIsolationTest, RequiresTenantID) {
    auto& tm = TenantManager::instance();
    
    // Secure mode
    TenantManager::Config config;
    config.allow_default_tenant = false;
    tm.configure(config);
    
    std::unordered_map<std::string, std::string> headers;
    auto tenant_id = tm.extractTenantId(headers, "/api/docs");
    
    EXPECT_FALSE(tenant_id.has_value());  // Should fail
}

TEST(TenantIsolationTest, ExtractsFromHeader) {
    auto& tm = TenantManager::instance();
    
    std::unordered_map<std::string, std::string> headers;
    headers["X-Tenant-ID"] = "test-tenant";
    
    auto tenant_id = tm.extractTenantId(headers, "/api/docs");
    
    ASSERT_TRUE(tenant_id.has_value());
    EXPECT_EQ(*tenant_id, "test-tenant");
}
```

### Integration Tests

```bash
# Test missing tenant
curl -w "%{http_code}" https://api.themis.example.com/api/documents
# Expected: 400

# Test with tenant header
curl -w "%{http_code}" \
  -H "X-Tenant-ID: test-tenant" \
  -H "Authorization: Bearer ${TOKEN}" \
  https://api.themis.example.com/api/documents
# Expected: 200

# Test cross-tenant access
curl -w "%{http_code}" \
  -H "X-Tenant-ID: other-tenant" \
  -H "Authorization: Bearer ${TENANT_A_TOKEN}" \
  https://api.themis.example.com/api/documents
# Expected: 403
```

## Cheat Sheet

```bash
# Quick tenant check
curl -H "X-Tenant-ID: my-tenant" \
     -H "Authorization: Bearer $TOKEN" \
     https://api/health

# View tenant metrics
curl https://api/metrics | grep "tenant=\"my-tenant\""

# Test quota limits
for i in {1..1000}; do
  curl -X POST \
    -H "X-Tenant-ID: my-tenant" \
    -H "Authorization: Bearer $TOKEN" \
    https://api/documents \
    -d '{"data":"test"}'
done
```

## See Also

- [Full Tenant Isolation Guide](TENANT_ISOLATION_GUIDE.md)
- [Authentication Guide](AUTH_GUIDE.md)
- [API Reference](API_REFERENCE.md)
