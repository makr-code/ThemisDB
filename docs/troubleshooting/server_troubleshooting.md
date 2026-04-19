# Server Troubleshooting Guide

The `server` module implements ThemisDB's main server process, including admin API handlers, authentication middleware, API gateway routing, API key management, async job handling, audit API, and multi-tenant request isolation.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `AdminApiHandler: 403 Forbidden` | Admin token expired or wrong | Rotate admin token; check `server.admin.token` |
| Server startup takes > 60s | Module initialisation hanging | Check per-module timeout in logs |
| `AuthMiddleware: no auth method matched` | Auth method not enabled | Enable required auth in `server.auth.*` |
| API gateway returns `502` | Upstream service unreachable | Check upstream health; tune `server.gateway.timeout_ms` |
| Async job never completes | Job worker pool exhausted | Increase `server.async_jobs.worker_threads` |
| Audit log write fails | Audit storage full | Clear old audit logs; increase storage |
| Tenant request bleeds to wrong tenant | Tenant isolation disabled | Enable `server.tenant_isolation: true` |
| `ApiVersionHandler: version deprecated` | Client using old API version | Update client to current API version |
| Rate limiting blocks admin operations | Admin role not exempt | Add admin to rate limiter bypass |
| `ApiKeyMgmtHandler: key already exists` | Duplicate key name | Use unique key names |

## Common Issues

### Issue 1: Admin API Returns 403

**Description:** Admin API calls return Forbidden even with the correct token.

**Symptoms:**
- `curl -H "Authorization: Bearer $ADMIN_TOKEN" http://localhost:9090/admin/status` returns 403
- Log: `AdminApiHandler: token validation failed`

**Cause:** Admin token is expired, wrong format, or admin API is restricted by IP.

**Solution:**
```bash
# Check admin token
themisdb-admin auth admin-token show

# Rotate admin token
themisdb-admin auth admin-token rotate

# Test admin access
curl -v -H "Authorization: Bearer $(themisdb-admin auth admin-token show)" \
  http://localhost:9090/admin/status
```
```yaml
server:
  admin:
    port: 9090
    bind_address: 127.0.0.1        # restrict to localhost by default
    allowed_cidrs:
      - 127.0.0.1/32
      - 10.0.0.0/8
    token_ttl_seconds: 86400
```

---

### Issue 2: Authentication Middleware Rejects Valid Requests

**Description:** The auth middleware rejects all requests even with valid credentials.

**Symptoms:**
- Log: `AuthMiddleware: no auth method matched request; returning 401`
- All API requests return 401

**Cause:** Auth method used by the client is not enabled in server config.

**Solution:**
```yaml
server:
  auth:
    methods:
      - jwt                         # enable JWT auth
      - api_key                     # enable API key auth
      - basic                       # disable basic auth in production
    bypass_paths:
      - /health
      - /metrics
      - /api/v2/public/*
```

---

### Issue 3: API Gateway Returns 502 Bad Gateway

**Description:** Requests proxied through the API gateway return 502.

**Symptoms:**
- Log: `ApiGateway: upstream themisdb-query returned 502`
- Specific API routes fail; others work

**Cause:** Upstream service is not reachable or is overloaded.

**Solution:**
```yaml
server:
  gateway:
    routes:
      - path: /api/v2/query
        upstream: http://localhost:8081
        timeout_ms: 30000          # increase from 5000
        retry_count: 2
        circuit_breaker:
          enabled: true
          failure_threshold: 5
```
```bash
# Check upstream health
curl http://localhost:8081/health

# Gateway routing table
themisdb-admin server gateway routes
```

---

### Issue 4: Async Jobs Queue Fills Up

**Description:** Long-running async jobs accumulate and new submissions are rejected.

**Symptoms:**
- Log: `AsyncJobApiHandler: job queue full (max=100); rejecting new jobs`
- API returns `503 Service Unavailable` for async job submissions

**Cause:** Worker thread pool is too small; jobs complete slower than they arrive.

**Solution:**
```yaml
server:
  async_jobs:
    worker_threads: 16             # increase from default 4
    max_queue_size: 1000
    job_timeout_ms: 600000
    result_ttl_ms: 3600000         # keep results for 1 hour
```

---

### Issue 5: Tenant Request Isolation Failure

**Description:** A request from Tenant A is processed with Tenant B's context.

**Symptoms:**
- Tenant A queries return Tenant B documents
- Audit log shows cross-tenant access

**Cause:** Tenant context not propagated through middleware stack; tenant isolation disabled.

**Solution:**
```yaml
server:
  tenant_isolation:
    enabled: true
    tenant_header: X-Tenant-ID
    tenant_from_jwt_claim: tenant_id
    strict_mode: true              # reject requests with no tenant context
    isolation_strategy: schema     # "schema" | "row_level" | "database"
```

---

### Issue 6: Audit Log Write Latency Spikes

**Description:** Audit log writes add 100ms+ to API request latency.

**Symptoms:**
- P99 latency spike correlates with audit log write rate
- Log: `AuditApiHandler: audit write took 150ms`

**Cause:** Audit log is written synchronously on the hot path.

**Solution:**
```yaml
server:
  audit:
    async_write: true             # write audit log asynchronously
    buffer_size: 10000
    flush_interval_ms: 1000
    sync_on_shutdown: true
```

---

### Issue 7: API Version Deprecated Warning Breaking Clients

**Description:** Old API clients receive deprecation warnings in response headers that break parsing.

**Symptoms:**
- Client receives `Deprecation: version 1` header and crashes
- Log: `ApiVersionHandler: serving deprecated v1 endpoint`

**Cause:** Client not handling deprecation headers.

**Solution:**
```yaml
server:
  api_versions:
    current: v2
    deprecated:
      v1:
        sunset_date: "2026-12-31"
        add_header: true           # set false to suppress header
        redirect_to_current: false
```

---

### Issue 8: Rate Limiter Blocks Admin API Calls

**Description:** Admin operations are rate-limited along with regular API traffic.

**Symptoms:**
- Log: `AuthMiddleware: admin user exceeded rate limit`
- HTTP 429 on admin endpoints

**Cause:** Admin users not excluded from rate limiting.

**Solution:**
```yaml
server:
  rate_limiter:
    enabled: true
    bypass_roles:
      - admin
      - system
    bypass_paths:
      - /admin/*
      - /metrics
    per_role_limits:
      analyst: 100                 # 100 rps for analysts
      developer: 500
```

## Diagnostic Commands

```bash
# Server health
curl http://localhost:9090/admin/status

# Active async jobs
themisdb-admin server async-jobs list

# Tenant context check
themisdb-admin server tenant-list

# API version info
themisdb-admin server api-version

# Auth middleware config
themisdb-admin server auth-config

# Live server metrics
curl -s http://localhost:9100/metrics | grep themisdb_server

# Tail server logs
journalctl -u themisdb -f | grep -E "server|admin|gateway|auth.middleware|tenant|async.job"
```

## Configuration Reference

```yaml
server:
  port: 8080
  admin:
    port: 9090
    bind_address: 127.0.0.1
    allowed_cidrs: []
  auth:
    methods: [jwt, api_key]
    bypass_paths: [/health, /metrics]
  tenant_isolation:
    enabled: true
    strict_mode: true
  async_jobs:
    worker_threads: 8
    max_queue_size: 500
  audit:
    async_write: true
    buffer_size: 5000
  rate_limiter:
    enabled: true
    bypass_roles: [admin, system]
```

## Known Limitations

- API gateway does not support gRPC proxying in the current implementation.
- Async job results are stored in-memory only by default; results are lost on server restart unless `async_jobs.persist_results: true`.
- Tenant isolation at `database` level requires separate RocksDB instances per tenant, which increases resource consumption.
- Admin API does not support OAuth2 authentication; only bearer token auth is supported.

## Related Documentation

- [Server Module ROADMAP](../../src/server/ROADMAP.md)
- [HTTP Server Refactoring](../ARCHIVED/implementation-summaries/HTTP_SERVER_REFACTORING.md)
- [API Reference](../api/REST_API_REFERENCE.md)
- [Tenant Isolation Guide](../de/security/TENANT_ISOLATION_GUIDE.md)
- [Audit Log Retention](../ARCHIVED/implementation-summaries/AUDIT_LOG_RETENTION_IMPLEMENTATION.md)
