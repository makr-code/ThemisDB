# ThemisDB Resource Limits and Rate Limiting Guide

## Overview

ThemisDB implements comprehensive resource limits and rate limiting to ensure system stability, fair resource allocation, and protection against abuse. This guide covers all resource limit mechanisms and their configuration.

## Table of Contents

1. [Rate Limiting](#rate-limiting)
2. [Tenant Quotas](#tenant-quotas)
3. [SSE/Changefeed Limits](#ssechangefeed-limits)
4. [Connection Limits](#connection-limits)
5. [Configuration Reference](#configuration-reference)

---

## Rate Limiting

ThemisDB provides two rate limiting implementations:

### Rate Limiter V1 (Legacy)

**Algorithm**: Token Bucket with continuous refill

**Features**:
- Per-IP and per-user rate limiting
- Configurable burst capacity and refill rate
- IP whitelist support
- Custom rate limits per IP/user
- Automatic cleanup of idle buckets (5-minute interval)

**Configuration** (`config.yaml`):
```yaml
rate_limiting:
  bucket_capacity: 100              # Maximum burst capacity (tokens)
  refill_rate: 100                  # Tokens refilled per minute
  window_seconds: 60                # Time window for rate limit
  per_ip_enabled: true              # Enable per-IP rate limiting
  per_user_enabled: true            # Enable per-user rate limiting
```

**Usage**: Automatically applied in HTTP server via `checkRateLimit()`

**Response on Limit Exceeded**:
- HTTP 429 Too Many Requests
- `Retry-After` header with seconds until next request allowed
- JSON error body with retry information

### Rate Limiter V2 (Preferred)

**Algorithm**: Token Bucket with Priority Lanes

**Features**:
- **Priority Lanes**: HIGH/NORMAL/LOW for VIP client support
- Per-client rate limiting with unique identifiers
- Separate capacity/refill rates per priority level
- Atomic metrics collection (total requests, rejections, available tokens)
- Automatic idle client cleanup

**Priority Configuration**:
- **HIGH** (VIP): 2000 capacity, 200 tokens/s (via `high_capacity`, `high_refill_rate`)
- **NORMAL**: Uses `capacity_per_client`, `refill_rate_per_client` (default: 100 capacity, 10 tokens/s)
- **LOW**: 500 capacity, 50 tokens/s (via `low_capacity`, `low_refill_rate`)

**Note**: The base configuration (`capacity_per_client`, `refill_rate_per_client`) defines the NORMAL priority lane. HIGH and LOW lanes override these with their specific settings when `enable_priority_lanes: true`.

**Configuration** (`config.yaml`):
```yaml
rate_limiting:
  v2:
    enabled: true                   # Enable V2 rate limiter (recommended)
    capacity_per_client: 100        # NORMAL lane: tokens per client (burst)
    refill_rate_per_client: 10      # NORMAL lane: tokens per second per client
    max_clients: 10000              # Maximum tracked clients
    cleanup_interval_minutes: 5     # Idle client cleanup interval
    
    # Priority lane configuration
    enable_priority_lanes: true
    high_capacity: 2000             # VIP client burst capacity
    high_refill_rate: 200           # VIP client sustained rate (tokens/s)
    low_capacity: 500               # Low-priority burst capacity
    low_refill_rate: 50             # Low-priority sustained rate (tokens/s)
```

**Integration**:
- API Gateway constructor accepts `PerClientRateLimiter` for V2
- `checkRateLimit()` prefers V2 when available, falls back to V1
- Client priority determined from JWT claims or user attributes

**Metrics**:
- `total_requests`: Total requests processed
- `total_rejections`: Requests rejected due to rate limit
- `available_tokens`: Current token count per priority lane
- `active_clients`: Number of clients being tracked

---

## Tenant Quotas

ThemisDB supports multi-tenancy with per-tenant resource quotas for resource isolation and fair usage.

### Tenant Identification

Tenants are identified via:
1. **HTTP Header**: `X-Tenant-ID` (configurable)
2. **Path Prefix**: `/tenants/{tenant_id}/` (configurable)
3. **Default Tenant**: Fallback for single-tenant deployments

### Resource Quotas

Each tenant has configurable limits:

| Resource | Quota Type | Default | Description |
|----------|-----------|---------|-------------|
| Storage | `max_storage_bytes` | Unlimited (0) | Maximum storage usage in bytes |
| Documents | `max_documents` | Unlimited (0) | Maximum document count |
| Collections | `max_collections` | Unlimited (0) | Maximum collection count |
| Connections | `max_connections` | 50 | Simultaneous connection limit |
| Queries | `max_concurrent_queries` | 100 | Concurrent query execution limit |
| Rate Limit | `requests_per_second` | 1000 | Requests per second per tenant |
| Burst | `burst_size` | 100 | Token bucket burst capacity |

### Quota Enforcement

Quotas are enforced at the HTTP server level:

1. **Connection Quotas**: Checked on every incoming request
   - Returns HTTP 503 if connection limit exceeded
   - Automatically released on request completion

2. **Query Quotas**: Checked on query endpoints (`/query`, `/search/*`, `/api/aql`)
   - Returns HTTP 503 if query slot limit exceeded
   - Automatically released on query completion

3. **Storage/Document Quotas**: Checked during write operations
   - Enforced via `TenantManager::checkQuota()`
   - Operations rejected if quota would be exceeded

### Configuration

```yaml
tenants:
  enforce_quotas: true              # Enable quota enforcement
  tenant_header: "X-Tenant-ID"      # HTTP header for tenant ID
  tenant_path_prefix: "/tenants/"   # Path prefix for tenant routing
  default_tenant_id: "default"      # Default tenant ID
  allow_default_tenant: true        # Allow requests without tenant ID
  max_tenants: 1000                 # Global tenant limit
  
  default_quotas:
    max_storage_bytes: 0            # 0 = unlimited
    max_documents: 0                # 0 = unlimited
    max_collections: 0              # 0 = unlimited
    max_concurrent_queries: 100
    max_connections: 50
    requests_per_second: 1000
    burst_size: 100
```

### Usage Tracking

`TenantManager` tracks real-time usage:

```cpp
// Acquire connection slot (RAII guard)
TenantContextGuard guard(tenant_context);
if (!guard.hasConnection()) {
    // Connection quota exceeded
}

// Acquire query slot
if (!guard.acquireQuerySlot()) {
    // Query quota exceeded
}

// Track storage/documents
tenant_mgr.incrementStorage(tenant_id, bytes);
tenant_mgr.incrementDocuments(tenant_id, count);
```

### Monitoring

Tenant metrics available via `/metrics` endpoint:
- `tenant_storage_bytes{tenant="..."}`: Current storage usage
- `tenant_documents{tenant="..."}`: Current document count
- `tenant_active_connections{tenant="..."}`: Active connections
- `tenant_active_queries{tenant="..."}`: Active queries
- `tenant_rate_limited_requests{tenant="..."}`: Rate limited request count

---

## SSE/Changefeed Limits

Server-Sent Events (SSE) for changefeed streaming have dedicated per-connection rate limits and buffer management.

### Rate Limiting

**Per-Connection Rate Cap**: `max_events_per_second`
- Default: 0 (unlimited)
- Recommended: 100-1000 for production
- Enforced via sliding window algorithm
- Prevents overwhelming slow clients

**Implementation**:
```cpp
// In SseConnectionManager::pollEvents()
if (config_.max_events_per_second > 0) {
    // Check if within 1-second window budget
    auto elapsed_ms = now - conn->window_start;
    if (elapsed_ms >= 1000) {
        conn->window_start = now;
        conn->sent_in_window = 0;
    }
    
    uint32_t budget = max_events_per_second - sent_in_window;
    if (budget == 0) {
        return {}; // No budget, defer sending
    }
    
    // Send up to budget
    size_t allowed = min(max_events, budget);
    // ... send events ...
    conn->sent_in_window += sent;
}
```

### Buffer Management

**Buffer Limit**: `max_buffered_events`
- Default: 1000 events per connection
- Recommended: 1000-10000 depending on memory
- Prevents memory exhaustion from slow consumers

**Overflow Policy**: `drop_oldest_on_overflow`
- `true` (default): Drop oldest events when buffer full (prevent memory issues)
- `false`: Drop newest events (preserve historical order)

**Metrics**:
- `total_events_sent`: Total events sent across all connections
- `total_dropped_events`: Events dropped due to buffer overflow
- `active_connections`: Current SSE connection count

### Heartbeat

**Purpose**: Prevent connection timeout during idle periods

**Configuration**:
- `heartbeat_interval_ms`: 15000 (15 seconds default)
- Automatically sent when no events for interval duration
- SSE comment format: `: heartbeat\n\n`

### Configuration

```yaml
sse:
  max_events_per_second: 100        # Events per second per connection (0 = unlimited)
  max_buffered_events: 1000         # Maximum buffered events per connection
  heartbeat_interval_ms: 15000      # Heartbeat interval (prevent timeout)
  event_poll_interval_ms: 500       # Changefeed polling interval
  drop_oldest_on_overflow: true     # Drop oldest events on buffer full
```

### Client Reconnection

**Retry Hint**: `retry_ms`
- Default: 3000 (3 seconds)
- Sent via SSE `retry:` field
- Client should respect this hint for reconnection

**Last Event ID**: Sequence numbers
- Client sends `Last-Event-ID` header or `from_seq` query param
- Server resumes from last acknowledged event
- Ensures no event loss on reconnection

---

## Connection Limits

### HTTP Server Limits

**Max Request Body Size**: `max_body_bytes`
- Default: Configured per deployment
- Enforced before processing request
- Returns HTTP 413 Payload Too Large

**Concurrent Request Limit**: `max_concurrent_requests`
- API Gateway configuration (default: 1000)
- Load shedding at 90% capacity threshold
- Returns HTTP 503 Service Unavailable

### HTTP/2 Limits

**Max Concurrent Streams**: `http2_max_concurrent_streams`
- Default: 100
- Per-connection multiplexing limit
- Configured in HTTP server settings

### WebSocket Limits

**Max Message Size**: Configurable
- Default: 1 MB
- Prevents memory exhaustion from large messages

**Ping Interval**: `websocket_ping_interval_ms`
- Default: 30000 (30 seconds)
- Keep-alive mechanism

---

## Configuration Reference

### Complete Example (`config.yaml`)

```yaml
# Rate Limiting (V1 - Legacy)
rate_limiting:
  bucket_capacity: 100
  refill_rate: 100
  window_seconds: 60
  per_ip_enabled: true
  per_user_enabled: true
  audit_rate_limit_per_minute: 100
  
  # Rate Limiting V2 (Preferred)
  v2:
    enabled: true
    capacity_per_client: 100
    refill_rate_per_client: 10
    max_clients: 10000
    cleanup_interval_minutes: 5
    enable_priority_lanes: true
    high_capacity: 2000
    high_refill_rate: 200
    low_capacity: 500
    low_refill_rate: 50

# Tenant Resource Quotas
tenants:
  enforce_quotas: true
  tenant_header: "X-Tenant-ID"
  tenant_path_prefix: "/tenants/"
  default_tenant_id: "default"
  allow_default_tenant: true
  max_tenants: 1000
  default_quotas:
    max_storage_bytes: 0
    max_documents: 0
    max_collections: 0
    max_concurrent_queries: 100
    max_connections: 50
    requests_per_second: 1000
    burst_size: 100

# SSE/Changefeed Limits
sse:
  max_events_per_second: 100
  max_buffered_events: 1000
  heartbeat_interval_ms: 15000
  event_poll_interval_ms: 500
  drop_oldest_on_overflow: true

# HTTP Server Limits
http:
  max_concurrent_requests: 1000
  request_timeout_ms: 30000
  http2_max_concurrent_streams: 100
```

---

## Best Practices

### Production Recommendations

1. **Enable Rate Limiter V2**: Use priority lanes for VIP clients
2. **Set Realistic Quotas**: Base on expected workload, add 20% buffer
3. **Monitor Metrics**: Track quota violations, rate limit rejections
4. **Configure SSE Limits**: Set `max_events_per_second` based on client capacity
5. **Use Tenant Isolation**: Enable quotas for multi-tenant deployments
6. **Load Test**: Verify limits under expected load before production

### Monitoring

Key metrics to monitor:
- `rate_limiter_rejections`: Rate limit violations
- `tenant_quota_exceeded`: Tenant quota violations
- `sse_dropped_events`: SSE buffer overflows
- `http_503_responses`: Service unavailable responses

### Troubleshooting

**Rate Limit Exceeded (HTTP 429)**:
- Check client request rate vs configured limit
- Verify IP/user is not whitelisted incorrectly
- Consider increasing `bucket_capacity` or `refill_rate`

**Tenant Quota Exceeded (HTTP 503)**:
- Check tenant usage metrics
- Verify quota configuration is appropriate
- Consider upgrading tenant tier or increasing limits

**SSE Events Dropped**:
- Client not consuming events fast enough
- Increase `max_buffered_events` or lower `max_events_per_second`
- Optimize client event processing

---

## Security Considerations

1. **DoS Protection**: Rate limiting prevents denial-of-service attacks
2. **Resource Isolation**: Tenant quotas prevent noisy neighbor problems
3. **Fair Usage**: Ensures equitable resource distribution
4. **Memory Safety**: Buffer limits prevent memory exhaustion
5. **Connection Limits**: Prevents connection exhaustion attacks

---

## API Response Codes

| Code | Reason | Resolution |
|------|--------|------------|
| 429 | Rate limit exceeded | Wait for `Retry-After` seconds |
| 503 | Tenant quota exceeded | Reduce load or upgrade quota |
| 503 | Connection limit reached | Retry with exponential backoff |
| 413 | Payload too large | Reduce request body size |

---

## Future Enhancements

Planned improvements:
- [ ] Dynamic rate limit adjustment based on system load
- [ ] Per-endpoint rate limits
- [ ] Tenant quota analytics dashboard
- [ ] Rate limit bypass for health check endpoints
- [ ] Adaptive SSE buffer sizing

---

## Related Documentation

- [API Gateway Architecture](./ARCHITECTURE.md#api-gateway)
- [Multi-Tenancy Guide](./MULTI_TENANCY.md)
- [Monitoring and Metrics](./MONITORING.md)
- [Security Best Practices](./SECURITY.md)

---

*Last Updated: 2026-02-09*
