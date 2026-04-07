# API Authentication and Authorization

**Version:** 1.5.0  
**Status:** Production Ready  
**Last Updated:** April 2026

---

## Overview

ThemisDB implements comprehensive authentication and authorization across all API endpoints using a fail-closed security model. This document describes the enforced RBAC (Role-Based Access Control) system and scope-based authorization for all API layers.

---

## Security Model

### Fail-Closed by Default

**Critical**: All production endpoints implement fail-closed security:
- Missing authentication tokens are **denied**
- Invalid authentication tokens are **denied**
- Insufficient scopes/permissions are **denied**
- Only explicitly authorized requests are **allowed**

### Exception for Backward Compatibility

When AuthMiddleware is not configured or explicitly disabled:
- Requests are allowed with a warning logged
- This is for development/testing environments only
- **Production deployments must always enable authentication**
- All Policy API handlers, RPC service, and Changefeed endpoints support this backward-compatible fail-open mode
- Warnings are logged to alert operators when unauthenticated access is granted

---

## Built-in RBAC Roles

ThemisDB provides four built-in roles via `RBAC::getBuiltinRoles()`:

| Role | Permissions | Inherits | Use Case |
|------|-------------|----------|----------|
| **admin** | `*:*` (all resources, all actions) | None | System administrators, full access |
| **operator** | `data:*`, `keys:read`, `keys:rotate`, `audit:read` | analyst | Operations team, data management |
| **analyst** | `data:read`, `audit:read`, `metrics:read` | readonly | Business analysts, read-only data access |
| **readonly** | `metrics:read`, `health:read` | None | Monitoring systems, health checks |

### Permission Structure

Permissions follow the format: `resource:action`

**Resources:** data, keys, config, audit, metrics, health, policy, cdc (changefeed), rpc

**Actions:** read, write, delete, rotate, admin

**Wildcards:** `*:*` grants all permissions, `data:*` grants all actions on data resource

---

## API Scope Mappings

### Policy APIs

All policy-related endpoints enforce scope-based authorization:

| Endpoint | Required Scope | Description |
|----------|---------------|-------------|
| `GET /policies/rules` | `policy:read` | List all policy rules |
| `GET /policies/rules/:id` | `policy:read` | Get specific policy rule |
| `POST /policies/rules` | `policy:write` | Create new policy rule |
| `PUT /policies/rules/:id` | `policy:write` | Update policy rule |
| `DELETE /policies/rules/:id` | `policy:write` | Delete policy rule |
| `POST /policies/evaluate` | `policy:read` | Evaluate policy |
| `GET /policies/stats` | `policy:read` | Get policy statistics |
| `POST /policies/validate` | `policy:read` | Validate policy ruleset |
| `POST /policies/validate/rule` | `policy:read` | Validate single rule |
| `GET /policies/validation/report` | `policy:read` | Get validation report |
| `GET /policies/metrics` | `policy:read` | Get policy metrics |
| `GET /policies/rules/:id/versions` | `policy:read` | List rule versions |
| `GET /policies/rules/:id/versions/:v` | `policy:read` | Get specific version |
| `POST /policies/rules/:id/rollback/:v` | `policy:write` | Rollback to version |
| `GET /policies/rules/:id/diff/:v1/:v2` | `policy:read` | Compare versions |
| `GET /policies/audit` | `policy:read` | Query audit trail |
| `GET /policies/templates` | `policy:read` | List policy templates |
| `POST /policies/templates` | `policy:write` | Create policy template |
| `GET /policies/reviews/pending` | `policy:read` | List pending reviews |
| `POST /policies/reviews/:id/schedule` | `policy:write` | Schedule review |

### RPC API

RPC methods enforce operation-specific scopes:

| Scope | Operations | Description |
|-------|-----------|-------------|
| `rpc:read` | GET, BatchGet, Search, Query, PaginatedQuery, VectorSearch, GraphTraverse, GeoQuery, TimeSeriesQuery, GetIndexOperations, ListCollections, GetCollectionMetadata, AggregationPipeline | Read-only data operations |
| `rpc:write` | PUT, BatchPut, Delete, UpdateEntity, BatchUpdate | Write and modify data |
| `rpc:admin` | CreateIndex, DropIndex, Stats | Administrative database operations |
| `transaction:write` | TransactionBegin, TransactionCommit, TransactionAbort | Transaction management |

**Exceptions:** 
- `health_check` - No authentication required (for monitoring)
- `authenticate` - No prior authentication required

### Changefeed API

Changefeed endpoints enforce CDC (Change Data Capture) scopes:

| Endpoint | Required Scope | Description |
|----------|---------------|-------------|
| `GET /changefeed` | `cdc:read` | Poll for events (with long-poll support) |
| `GET /changefeed/stream` | `cdc:read` | Stream events via Server-Sent Events (SSE) |
| `GET /changefeed/stats` | `cdc:admin` | Get changefeed statistics |
| `POST /changefeed/retention` | `cdc:admin` | Configure retention policy |

**Long-lived Connections:** SSE streaming connections maintain the authenticated session for the duration of the stream. Clients should implement reconnection logic with fresh token validation on reconnect.

### Compliance and Audit APIs

Compliance reporting endpoints enforce audit scopes:

| Endpoint | Required Scope | Description |
|----------|---------------|-------------|
| `POST /compliance/coverage` | `audit:read` | Analyze compliance coverage |
| `POST /compliance/report` | `audit:read` | Generate compliance report |
| `GET /compliance/frameworks` | `audit:read` | List supported frameworks |

---

## Authentication Flow

### 1. Token-Based Authentication

All requests must include a Bearer token in the Authorization header:

```http
Authorization: Bearer <token>
```

### 2. Token Validation

For each request:
1. Extract Bearer token from Authorization header
2. Validate token using AuthMiddleware
3. Check required scope for the endpoint
4. Deny if token is invalid or lacks required scope
5. Log authentication result for audit

### 3. Scope Enforcement

```cpp
// Example: Policy API endpoint
auto token = AuthMiddleware::extractBearerToken(auth_header);
auto auth_result = auth_->authorize(*token, "policy:write");
if (!auth_result.authorized) {
    // Log failure with user_id and reason
    return 403 Forbidden;
}
```

---

## Configuration

### Enable Authentication

```cpp
// Server initialization
auto auth = std::make_shared<AuthMiddleware>();

// Option 1: JWT with Keycloak/OIDC
AuthMiddleware::JWTConfig jwt_config{
    .jwks_url = "https://keycloak.example.com/realms/themis/protocol/openid-connect/certs",
    .expected_issuer = "https://keycloak.example.com/realms/themis",
    .expected_audience = "themis-app",
    .scope_claim = "roles"
};
auth->enableJWT(jwt_config);

// Option 2: Static API Tokens
AuthMiddleware::TokenConfig token_config{
    .token = "themis_api_key_...",
    .user_id = "api-service",
    .scopes = {"rpc:read", "rpc:write", "policy:read"}
};
auth->addToken(token_config);
```

### RBAC Configuration

```yaml
# rbac_config.yaml
roles:
  - name: admin
    description: System administrator
    permissions:
      - resource: "*"
        action: "*"
  
  - name: operator
    description: Operations team
    inherits: [analyst]
    permissions:
      - resource: data
        action: "*"
      - resource: keys
        action: read
      - resource: keys
        action: rotate
      - resource: audit
        action: read
  
  - name: analyst
    description: Data analyst
    inherits: [readonly]
    permissions:
      - resource: data
        action: read
      - resource: audit
        action: read
      - resource: metrics
        action: read
```

---

## Client Examples

### Python gRPC Client

```python
import grpc

# Create channel with credentials
credentials = grpc.ssl_channel_credentials()
channel = grpc.secure_channel('themis.example.com:50051', credentials)

# Add authorization metadata
token = "your_jwt_token"
metadata = [('authorization', f'Bearer {token}')]

# Make authenticated request
stub = ThemisRPCStub(channel)
response = stub.Get(request, metadata=metadata)
```

### cURL HTTP REST

```bash
# Policy API - List rules
curl -X GET https://themis.example.com/policies/rules \
  -H "Authorization: Bearer $JWT_TOKEN"

# Changefeed - Stream events
curl -X GET https://themis.example.com/changefeed/stream \
  -H "Authorization: Bearer $JWT_TOKEN" \
  -H "Accept: text/event-stream"
```

---

## Monitoring and Audit

### Authentication Metrics

AuthMiddleware provides metrics for Prometheus:

```cpp
const auto& metrics = auth->getMetrics();
// authz_success_total
// authz_denied_total
// authz_invalid_token_total
// jwt_validation_success_total
// jwt_validation_failed_total
```

### Audit Logging

All authentication failures are logged with:
- Timestamp
- Endpoint/operation attempted
- User ID (if token was valid but lacked scope)
- Required scope
- Failure reason
- Source IP (when available)

Example log:
```
[WARN] Authorization failed for policy endpoint - user: alice@example.com, required scope: policy:write, reason: insufficient_scope
```

---

## Migration Guide

### From Unauthenticated to Authenticated

**Step 1:** Enable authentication on server

```cpp
auto auth = std::make_shared<AuthMiddleware>();
auth->enableJWT(jwt_config);
// Pass auth to all API handlers
```

**Step 2:** Update clients to include tokens

```python
# Before
response = stub.Get(request)

# After
metadata = [('authorization', f'Bearer {token}')]
response = stub.Get(request, metadata=metadata)
```

**Step 3:** Configure user roles

```bash
# Assign roles to users
curl -X POST https://themis.example.com/admin/users/alice/roles \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  -d '{"role": "operator"}'
```

---

## Troubleshooting

### "Missing Authorization header"

**Cause:** Client not sending Authorization header  
**Solution:** Include `Authorization: Bearer <token>` in all requests

### "Invalid Authorization header format"

**Cause:** Malformed header (not "Bearer <token>" format)  
**Solution:** Ensure header follows Bearer token format

### "Authentication failed or insufficient scope"

**Cause:** Token is valid but lacks required scope  
**Solution:** Ensure user has appropriate role/scope assigned

### "AuthMiddleware not configured or disabled"

**Cause:** Server not configured with authentication  
**Solution:** Enable AuthMiddleware with JWT or static tokens

---

## Security Best Practices

1. **Always enable authentication in production**
   - Configure AuthMiddleware before starting server
   - Use JWT with trusted identity provider
   - Never deploy with auth disabled

2. **Use HTTPS/TLS**
   - Encrypt all traffic (HTTP REST and gRPC)
   - Enable mutual TLS for enhanced security
   - Protect tokens in transit

3. **Principle of Least Privilege**
   - Assign minimum required roles
   - Use readonly role for monitoring
   - Limit admin access

4. **Token Management**
   - Use short-lived JWT tokens (15-60 minutes)
   - Implement token refresh mechanism
   - Rotate static API keys regularly

5. **Monitoring**
   - Track authentication failures
   - Alert on anomalous patterns
   - Review audit logs regularly

6. **Scope Mapping**
   - Understand scope requirements for each operation
   - Map business roles to RBAC roles
   - Document custom scope assignments

---

## References

- [RPC Authentication Guide](../features/rpc_authentication.md)
- [Access Control Framework](./access_control_framework.md)
- [Production Hardening Checklist](./PRODUCTION_HARDENING_CHECKLIST.md)
- [RBAC Implementation](../../include/security/rbac.h)
- [AuthMiddleware Implementation](../../include/server/auth_middleware.h)

---

**Implementation Notes:**
- All policy API handlers: Implemented v1.5.0
- RPC service: Scope-based enforcement v1.5.0
- Changefeed: Auth enforcement v1.4.0+
- Built-in RBAC roles: Available in all versions

**Security Audits:**
- CODE_QUALITY_AUDIT.md: FIND-001 RESOLVED
- ACL enforcement hardened: February 2026
