# API Troubleshooting Guide

The `api` module implements ThemisDB's HTTP/REST, GraphQL, and gRPC API servers, with WebSocket support for real-time change subscriptions, HTTP/2, CORS, and rate limiting.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| HTTP 503 on all endpoints | Server not fully started or port conflict | Check startup logs; verify `api.http.port` |
| `CORS: origin not allowed` | Origin not in allowlist | Add origin to `api.cors.allowed_origins` |
| gRPC `UNAVAILABLE` | TLS misconfiguration | Check server cert and `api.grpc.tls.*` config |
| WebSocket handshake fails with 401 | JWT not passed in `Authorization` header | Pass token as `?token=` query param or header |
| GraphQL depth limit exceeded | Query too deeply nested | Increase `api.graphql.max_depth` |
| HTTP/2 `GOAWAY` frames | Keep-alive timeout too short | Increase `api.http2.keep_alive_timeout_ms` |
| Rate limiter blocks all requests | Global limit too low | Check `api.rate_limiter.global_limit_rps` |
| REST response truncated | Response size limit exceeded | Increase `api.max_response_size_mb` |
| gRPC `RESOURCE_EXHAUSTED` | Connection pool exhausted | Increase `api.grpc.max_concurrent_streams` |
| `404 Not Found` on valid route | API version prefix mismatch | Use `/api/v2/` prefix; check `api.version_prefix` |

## Common Issues

### Issue 1: Server Fails to Bind Port

**Description:** ThemisDB cannot start the API server because the port is already in use.

**Symptoms:**
- Log: `HttpServer: failed to bind port 8080: Address already in use`
- Server exits immediately

**Cause:** Another process is listening on the configured port.

**Solution:**
```bash
# Find the conflicting process
ss -tlnp | grep 8080
lsof -i :8080

# Kill the conflicting process or change ThemisDB's port
```
```yaml
api:
  http:
    port: 8081          # change to free port
    bind_address: 0.0.0.0
```

---

### Issue 2: CORS Preflight Request Fails

**Description:** Browser requests from a frontend application fail with CORS errors.

**Symptoms:**
- Browser console: `CORS: Access-Control-Allow-Origin missing`
- OPTIONS preflight returns `403`

**Cause:** Frontend origin is not in the CORS allowlist.

**Solution:**
```yaml
api:
  cors:
    enabled: true
    allowed_origins:
      - https://app.example.com
      - https://admin.example.com
    allowed_methods: [GET, POST, PUT, DELETE, PATCH, OPTIONS]
    allowed_headers: [Authorization, Content-Type, X-Request-ID]
    allow_credentials: true
    max_age_seconds: 3600
```

---

### Issue 3: gRPC TLS Handshake Failure

**Description:** gRPC clients cannot connect due to TLS certificate errors.

**Symptoms:**
- gRPC error: `UNAVAILABLE: Ssl handshake failed`
- Log: `GrpcServer: TLS handshake error: certificate verify failed`

**Cause:** Server certificate is self-signed and the CA is not trusted by the client, or certificate has expired.

**Solution:**
```bash
# Check certificate expiry
openssl x509 -in /etc/themisdb/tls/server.crt -noout -dates

# Test gRPC TLS
grpcurl -cacert /etc/themisdb/tls/ca.crt \
        localhost:9090 list
```
```yaml
api:
  grpc:
    enabled: true
    port: 9090
    tls:
      enabled: true
      cert_file: /etc/themisdb/tls/server.crt
      key_file: /etc/themisdb/tls/server.key
      ca_file: /etc/themisdb/tls/ca.crt
      client_auth: optional        # "none" | "optional" | "required"
```

---

### Issue 4: WebSocket Connection Drops After 60 Seconds

**Description:** WebSocket connections for change subscriptions are closed unexpectedly.

**Symptoms:**
- Client WebSocket closes with code `1001 Going Away`
- Log: `WsHandler: connection idle timeout after 60000ms`

**Cause:** Default idle timeout is 60 seconds; clients that don't send pings are disconnected.

**Solution:**
```yaml
api:
  websocket:
    idle_timeout_ms: 300000        # increase to 5 minutes
    ping_interval_ms: 30000        # server-initiated pings
    pong_timeout_ms: 10000
    max_message_size_bytes: 1048576
```

---

### Issue 5: GraphQL Query Exceeds Depth Limit

**Description:** A valid GraphQL query is rejected because it is too deeply nested.

**Symptoms:**
- GraphQL error: `{"errors": [{"message": "max depth 5 exceeded"}]}`

**Cause:** Default `max_depth` is 5; complex relational queries need deeper nesting.

**Solution:**
```yaml
api:
  graphql:
    enabled: true
    max_depth: 10                  # increase from 5
    max_complexity: 1000
    introspection: false           # disable in production
    playground: false
```

---

### Issue 6: Rate Limiter Returns 429 for Internal Services

**Description:** Internal microservices hitting the API are rate-limited unexpectedly.

**Symptoms:**
- Log: `RateLimiter: IP 10.0.1.50 exceeded 100 req/s`
- Internal pipelines fail with HTTP 429

**Cause:** Internal services share the same IP-based rate limit as external clients.

**Solution:**
```yaml
api:
  rate_limiter:
    enabled: true
    global_limit_rps: 1000
    per_ip_limit_rps: 200
    whitelist_cidrs:
      - 10.0.0.0/8               # whitelist internal network
      - 172.16.0.0/12
    per_api_key_limit: true       # use API key for internal services
```

---

### Issue 7: Large REST Response Truncated

**Description:** REST API responses for large result sets are cut off.

**Symptoms:**
- JSON response ends mid-object
- Log: `HttpServer: response truncated at max_response_size_mb=10`

**Cause:** Response exceeds the configured size limit.

**Solution:**
```yaml
api:
  max_response_size_mb: 100       # increase from 10
  pagination:
    default_page_size: 100
    max_page_size: 10000          # use pagination for large datasets
  streaming:
    enabled: true                 # use streaming for very large results
    chunk_size_kb: 64
```

---

### Issue 8: HTTP/2 Connections Fail Under Load

**Description:** Under high concurrency, HTTP/2 connections are reset.

**Symptoms:**
- Clients receive `RST_STREAM` frames
- Log: `Http2Server: stream limit reached (max_concurrent_streams=100)`

**Cause:** `max_concurrent_streams` is too low for the client concurrency.

**Solution:**
```yaml
api:
  http2:
    enabled: true
    max_concurrent_streams: 500   # increase from 100
    initial_window_size: 1048576
    keep_alive_timeout_ms: 60000
    max_frame_size: 16384
```

---

### Issue 9: API Versioning Returns 404 for Valid Routes

**Description:** Requests to valid API routes return `404 Not Found`.

**Symptoms:**
- `GET /api/v1/collections` returns 404
- `GET /api/v2/collections` works correctly

**Cause:** Client is using the old `v1` prefix but the server only serves `v2`.

**Solution:**
```yaml
api:
  version_prefix: /api/v2         # current version
  deprecated_versions:
    - prefix: /api/v1
      sunset_date: "2026-12-31"   # still serve v1 until sunset
      redirect_to_v2: false
```

---

### Issue 10: GeoJSON Validation Rejects Valid Coordinates

**Description:** The geo-index hooks in the API reject valid GeoJSON payloads.

**Symptoms:**
- Error: `GeoIndexHooks: invalid GeoJSON: coordinate out of range`
- Valid lat/lng values are rejected

**Cause:** Coordinates are in `[lat, lng]` order instead of GeoJSON standard `[lng, lat]`.

**Solution:**
```bash
# Correct GeoJSON format: [longitude, latitude]
# Wrong:  { "type": "Point", "coordinates": [52.5200, 13.4050] }
# Correct: { "type": "Point", "coordinates": [13.4050, 52.5200] }
```
```yaml
api:
  geo:
    coordinate_order: lng_lat     # enforce GeoJSON standard
    validate_on_insert: true
```

## Diagnostic Commands

```bash
# Check API server health
curl http://localhost:8080/health

# List active WebSocket connections
themisdb-admin api ws-connections

# Show rate limiter status
themisdb-admin api rate-limiter stats

# Test gRPC endpoint
grpcurl -plaintext localhost:9090 list

# Show API metrics
curl -s http://localhost:9100/metrics | grep themisdb_api

# Tail API logs
journalctl -u themisdb -f | grep -E "api|http|grpc|graphql|websocket|cors"

# Check active HTTP/2 streams
themisdb-admin api http2-stats
```

## Configuration Reference

```yaml
api:
  http:
    enabled: true
    port: 8080
    bind_address: 0.0.0.0
    tls:
      enabled: false
  http2:
    enabled: true
    max_concurrent_streams: 250
  grpc:
    enabled: true
    port: 9090
    tls:
      enabled: true
  websocket:
    enabled: true
    idle_timeout_ms: 300000
    ping_interval_ms: 30000
  graphql:
    enabled: true
    max_depth: 10
    max_complexity: 500
  cors:
    enabled: true
    allowed_origins: []
  rate_limiter:
    enabled: true
    global_limit_rps: 500
    per_ip_limit_rps: 100
  max_response_size_mb: 50
  version_prefix: /api/v2
```

**Common misconfigurations:**

| Key | Wrong | Correct |
|-----|-------|---------|
| `cors.allowed_origins` | `["*"]` in production | Explicit origin list |
| `graphql.introspection` | `true` in production | `false` |
| `grpc.tls.enabled` | `false` in production | `true` |
| `rate_limiter.whitelist_cidrs` | `[]` | Include internal CIDRs |

## Known Limitations

- GraphQL subscriptions over WebSocket use a custom protocol; standard Apollo subscription clients require configuration.
- HTTP/2 server push is not implemented; only multiplexing is supported.
- gRPC reflection is disabled in production builds for security; enable only in development.
- WebSocket change subscriptions do not support filtering by document field values, only by collection.

## Related Documentation

- [REST API Reference](../api/REST_API_REFERENCE.md)
- [API Reference](../api/API_REFERENCE.md)
- [API Versioning](../api/API_VERSIONING.md)
- [Authentication & Rate Limiting](../api/AUTHENTICATION_AND_RATE_LIMITING.md)
- [Wire Protocol](../architecture/wire-protocol.md)
