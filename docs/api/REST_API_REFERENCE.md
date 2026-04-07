# REST/HTTP API Reference

**Version:** 1.5.0-dev  
**Last Updated:** 2026-04-06

## Overview

ThemisDB provides a comprehensive REST/HTTP API for all database operations. This document describes the available endpoints, authentication requirements, and rate limiting policies.

## Base URL

```
http://localhost:8080
```

For HTTPS/TLS:
```
https://localhost:8080
```

### Protocol Support

ThemisDB supports multiple HTTP versions with automatic negotiation:

- **HTTP/1.1**: Fully supported (default for curl and most clients)
- **HTTP/2**: Fully supported with Server Push (enabled by default)
- **HTTP/3 (QUIC)**: Planned for v1.6+ (experimental support via `enable_http3` flag)

> **📖 Port Reference:** See [docs/de/deployment/PORT_REFERENCE.md](../de/deployment/PORT_REFERENCE.md) for complete port mapping across deployment platforms.

### Default Ports

| Port | Protocol | Purpose |
|------|----------|---------|
| `8080` | HTTP/1.1, HTTP/2 | REST API, GraphQL |
| `18765` | Binary Wire Protocol | gRPC, inter-shard communication |
| `4318` | HTTP | OpenTelemetry/Prometheus metrics |

**Note:** As of v1.3.0+, ports are standardized. Some legacy configs may use port 8765 for backwards compatibility. For current deployments, use the standard ports above. See [PORT_REFERENCE.md](../de/deployment/PORT_REFERENCE.md) for complete details.

## Authentication

### Overview

ThemisDB supports JWT-based authentication with scope-based authorization. When authentication is enabled, most endpoints require a valid JWT token in the `Authorization` header.

### Authentication Header

```http
Authorization: Bearer <jwt_token>
```

### Scopes

The following scopes are used to control access to different API endpoints:

| Scope | Description | Endpoints |
|-------|-------------|-----------|
| `data:read` | Read access to data | GET /entities, /query, /vector, /graph |
| `data:write` | Write access to data | PUT/POST/DELETE /entities, /index |
| `content:read` | Read content and documents | GET /content, /contentfs |
| `content:write` | Write content and documents | POST /content, /contentfs |
| `cdc:read` | Access to change data capture | GET /changefeed |
| `timeseries:read` | Read time-series data | GET /timeseries |
| `timeseries:write` | Write time-series data | POST /timeseries |
| `cache:read` | Query semantic cache | POST /cache/query |
| `llm:read` | Read prompt templates | GET /prompt |
| `llm:write` | Manage prompt templates | POST /prompt |
| `pki:sign` | Sign documents with PKI | POST /api/pki/sign |
| `pki:verify` | Verify PKI signatures | POST /api/pki/verify |
| `pii:read` | Access PII mappings | GET /pii |
| `pii:write` | Manage PII mappings | POST /pii |
| `audit:read` | Query audit logs | GET /api/audit |
| `config:read` | View configuration | GET /config |
| `config:write` | Modify configuration | POST /config |
| `admin` | Administrative operations | POST /admin/backup, /admin/restore |

### Public Endpoints

The following endpoints do not require authentication:

- `GET /health` - Health check
- `GET /version` - Version information
- `GET /stats` - Server statistics
- `GET /capabilities` - Feature capabilities
- `GET /metrics` - Prometheus metrics

## Rate Limiting

### Global Rate Limits

By default, all endpoints are rate-limited to prevent abuse and ensure fair usage:

- **Global limit**: 100 requests per minute per IP/user
- **Burst capacity**: 100 requests

### Per-Endpoint Rate Limits

Certain endpoints have more restrictive rate limits:

| Endpoint | Requests/min | Burst | Reason |
|----------|--------------|-------|--------|
| `/api/audit/*` | 50 | 10 | Expensive queries |
| `/admin/*` | 50 | 10 | Administrative operations |
| `/config` | 10 | 5 | Configuration changes |
| `/pii/*` | 50 | 10 | Sensitive operations |
| `/query/aql` | 300 | 30 | Complex queries |
| `/index/*` | 100 | 20 | Resource-intensive |

### Rate Limit Headers

When a request is rate-limited, the response includes the following headers:

```http
HTTP/1.1 429 Too Many Requests
X-RateLimit-Limit: 100
X-RateLimit-Remaining: 0
X-RateLimit-Reset: 1612345678
Retry-After: 60
```

## API Endpoints

### Entity Operations

#### Create/Update Entity

```http
PUT /entities/:id
POST /entities
Content-Type: application/json

{
  "id": "user:123",
  "data": {
    "name": "John Doe",
    "email": "john@example.com"
  }
}
```

**Authentication**: Required (`data:write`)  
**Rate Limit**: 500 req/min

**Response**:
```json
{
  "success": true,
  "id": "user:123",
  "version": 1
}
```

#### Get Entity

```http
GET /entities/:id
```

**Authentication**: Required (`data:read`)  
**Rate Limit**: 1000 req/min

**Response**:
```json
{
  "id": "user:123",
  "data": {
    "name": "John Doe",
    "email": "john@example.com"
  },
  "version": 1,
  "created_at": "2024-01-15T10:30:00Z",
  "updated_at": "2024-01-15T10:30:00Z"
}
```

#### Delete Entity

```http
DELETE /entities/:id
```

**Authentication**: Required (`data:write`)  
**Rate Limit**: 500 req/min

**Response**:
```json
{
  "success": true,
  "id": "user:123"
}
```

#### Batch Operations

Atomically execute up to **10,000** `put` or `delete` operations in a single request.

**Request:**
```http
POST /entities/batch
Content-Type: application/json

{
  "operations": [
    {
      "op": "put",
      "key": "users:123",
      "blob": "{\"name\":\"Alice\",\"age\":30}"
    },
    {
      "op": "delete",
      "key": "users:456"
    }
  ]
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `op` | string | yes | `"put"` (insert/upsert) or `"delete"` |
| `key` | string | yes | Entity key in `"table:pk"` format (e.g. `"users:123"`) |
| `blob` | string | put only | JSON-encoded entity document as a string |

**Response** `200 OK`:
```json
{
  "success": true,
  "total": 2,
  "succeeded": 2,
  "failed": 0
}
```

When some operations fail the response includes an `"errors"` array with per-item details:
```json
{
  "success": true,
  "total": 3,
  "succeeded": 2,
  "failed": 1,
  "errors": [
    {"index": 1, "key": "users:bad", "error": "Key must be in format 'table:pk'"}
  ]
}
```

**Authentication**: Required (`data:write`)  
**Rate Limit**: 500 req/min

### Query Operations

#### Query Entities

```http
POST /query
Content-Type: application/json

{
  "filter": {
    "name": "John*"
  },
  "limit": 10
}
```

**Authentication**: Required (`data:read`)  
**Rate Limit**: 500 req/min

**Response**:
```json
{
  "results": [
    {
      "id": "user:123",
      "data": {...}
    }
  ],
  "count": 1,
  "has_more": false
}
```

#### AQL Query

```http
POST /query/aql
Content-Type: application/json

{
  "query": "FOR u IN users FILTER u.age > @minAge RETURN u",
  "bind_vars": {
    "minAge": 18
  }
}
```

**Authentication**: Required (`data:read`)  
**Rate Limit**: 300 req/min (more restrictive due to complexity)

### Index Management

#### Create Index

```http
POST /index/create
Content-Type: application/json

{
  "name": "idx_email",
  "field": "email",
  "type": "hash"
}
```

**Authentication**: Required (`data:write`)  
**Rate Limit**: 100 req/min

#### Drop Index

```http
POST /index/drop
Content-Type: application/json

{
  "name": "idx_email"
}
```

**Authentication**: Required (`data:write`)  
**Rate Limit**: 100 req/min

#### Index Statistics

```http
GET /index/stats?name=idx_email
```

**Authentication**: Required (`data:read`)  
**Rate Limit**: 100 req/min

#### Rebuild Index

```http
POST /index/rebuild
Content-Type: application/json

{
  "name": "idx_email"
}
```

**Authentication**: Required (`data:write`)  
**Rate Limit**: 100 req/min

### Vector Operations

#### Vector Search

```http
POST /vector/search
Content-Type: application/json

{
  "collection": "documents",
  "vector": [0.1, 0.2, ...],
  "k": 10,
  "metric": "cosine"
}
```

**Authentication**: Required (`data:read`)  
**Rate Limit**: 500 req/min

#### Batch Insert Vectors

```http
POST /vector/batch_insert
Content-Type: application/json

{
  "collection": "documents",
  "vectors": [
    {
      "id": "doc1",
      "vector": [0.1, 0.2, ...],
      "metadata": {...}
    }
  ]
}
```

**Authentication**: Required (`data:write`)  
**Rate Limit**: 500 req/min

### Graph Operations

#### Traverse Graph

```http
POST /graph/traverse
Content-Type: application/json

{
  "start_id": "user:123",
  "direction": "outbound",
  "edge_type": "follows",
  "max_depth": 3
}
```

**Authentication**: Required (`data:read`)  
**Rate Limit**: 500 req/min

#### Create Edge

```http
POST /graph/edge
Content-Type: application/json

{
  "from_id": "user:123",
  "to_id": "user:456",
  "edge_type": "follows",
  "properties": {
    "since": "2024-01-15"
  }
}
```

**Authentication**: Required (`data:write`)  
**Rate Limit**: 500 req/min

#### Delete Edge

```http
DELETE /graph/edge?from=user:123&to=user:456&type=follows
```

**Authentication**: Required (`data:write`)  
**Rate Limit**: 500 req/min

### Content Management

#### Import Content

```http
POST /content/import
Content-Type: application/json

{
  "content": "Document content here...",
  "metadata": {
    "title": "My Document",
    "author": "John Doe"
  }
}
```

**Authentication**: Required (`content:write`)  
**Rate Limit**: 500 req/min

#### Search Content

```http
POST /content/search
Content-Type: application/json

{
  "query": "search terms",
  "filters": {
    "author": "John Doe"
  },
  "limit": 10
}
```

**Authentication**: Required (`content:read`)  
**Rate Limit**: 500 req/min

### Time-Series Operations

#### Insert Time-Series Data

```http
POST /timeseries/insert
Content-Type: application/json

{
  "series": "temperature",
  "timestamp": "2024-01-15T10:30:00Z",
  "value": 23.5,
  "tags": {
    "location": "office"
  }
}
```

**Authentication**: Required (`timeseries:write`)  
**Rate Limit**: 500 req/min

#### Query Time-Series

```http
POST /timeseries/query
Content-Type: application/json

{
  "series": "temperature",
  "start": "2024-01-15T00:00:00Z",
  "end": "2024-01-15T23:59:59Z",
  "aggregation": "avg",
  "interval": "1h"
}
```

**Authentication**: Required (`timeseries:read`)  
**Rate Limit**: 500 req/min

### Changefeed/CDC Operations

#### Subscribe to Changes

```http
GET /changefeed/stream?collection=users
Accept: text/event-stream
```

**Authentication**: Required (`cdc:read`)  
**Rate Limit**: 100 req/min

**Response**: Server-Sent Events (SSE) stream

#### Get Change History

```http
GET /changefeed/history?collection=users&since=2024-01-15T00:00:00Z
```

**Authentication**: Required (`cdc:read`)  
**Rate Limit**: 100 req/min

### Transaction Operations

#### Begin Transaction

```http
POST /transaction/begin
Content-Type: application/json

{
  "isolation_level": "snapshot"
}
```

**Authentication**: Required (`data:write`)  
**Rate Limit**: 100 req/min

#### Commit Transaction

```http
POST /transaction/commit
Content-Type: application/json

{
  "tx_id": "tx_123456"
}
```

**Authentication**: Required (`data:write`)  
**Rate Limit**: 100 req/min

#### Rollback Transaction

```http
POST /transaction/rollback
Content-Type: application/json

{
  "tx_id": "tx_123456"
}
```

**Authentication**: Required (`data:write`)  
**Rate Limit**: 100 req/min

### Cache Operations

#### Query Semantic Cache

```http
POST /cache/query
Content-Type: application/json

{
  "query": "What is the capital of France?",
  "threshold": 0.9
}
```

**Authentication**: Required (`cache:read`)  
**Rate Limit**: 1000 req/min

### PKI Operations

#### Sign Document

```http
POST /api/pki/sign
Content-Type: application/json

{
  "data": "Document to sign",
  "cert_id": "cert123"
}
```

**Authentication**: Required (`pki:sign`)  
**Rate Limit**: 100 req/min

#### Verify Signature

```http
POST /api/pki/verify
Content-Type: application/json

{
  "data": "Document that was signed",
  "signature": "...",
  "cert_id": "cert123"
}
```

**Authentication**: Required (`pki:verify`)  
**Rate Limit**: 100 req/min

### PII Operations

#### List PII Mappings

```http
GET /pii/mappings
```

**Authentication**: Required (`pii:read`)  
**Rate Limit**: 50 req/min

#### Create PII Mapping

```http
POST /pii/mappings
Content-Type: application/json

{
  "original_value": "john.doe@example.com",
  "pseudonymized_value": "abc123"
}
```

**Authentication**: Required (`pii:write`)  
**Rate Limit**: 50 req/min

### Audit Operations

#### Query Audit Log

```http
POST /api/audit/query
Content-Type: application/json

{
  "start_time": "2024-01-15T00:00:00Z",
  "end_time": "2024-01-15T23:59:59Z",
  "user_id": "user:123",
  "action": "read"
}
```

**Authentication**: Required (`audit:read`)  
**Rate Limit**: 50 req/min (very restrictive)

#### Export Audit Log (CSV)

```http
GET /api/audit/export/csv?start=2024-01-15T00:00:00Z&end=2024-01-15T23:59:59Z
```

**Authentication**: Required (`audit:read`)  
**Rate Limit**: 50 req/min

### Configuration Operations

#### Get Configuration

```http
GET /config
```

**Authentication**: Required (`config:read`)  
**Rate Limit**: 10 req/min

**Response**:
```json
{
  "server": {
    "port": 8080,
    "threads": 8,
    "request_timeout_ms": 30000
  },
  "features": {
    "semantic_cache": true,
    "llm_store": true,
    "cdc": true,
    "timeseries": true
  },
  "authentication": {
    "enabled": true,
    "rate_limiting_enabled": true
  }
}
```

#### Update Configuration

```http
POST /config
Content-Type: application/json

{
  "logging": {
    "level": "info"
  },
  "features": {
    "semantic_cache": true
  }
}
```

**Authentication**: Required (`config:write`)  
**Rate Limit**: 10 req/min (very restrictive)

### Administrative Operations

#### Backup Database

```http
POST /admin/backup
Content-Type: application/json

{
  "path": "/backups/backup_2024-01-15.db"
}
```

**Authentication**: Required (`admin`)  
**Rate Limit**: 50 req/min

#### Restore Database

```http
POST /admin/restore
Content-Type: application/json

{
  "path": "/backups/backup_2024-01-15.db"
}
```

**Authentication**: Required (`admin`)  
**Rate Limit**: 50 req/min

### Monitoring Endpoints

#### Health Check

```http
GET /health
```

**Authentication**: Not required  
**Rate Limit**: 1000 req/min

**Response**:
```json
{
  "status": "healthy",
  "version": "1.0.0",
  "uptime_seconds": 3600
}
```

#### Version Information

```http
GET /version
```

**Authentication**: Not required  
**Rate Limit**: 1000 req/min

**Response**:
```json
{
  "version": "1.0.0",
  "build_date": "2024-01-15",
  "git_commit": "abc123"
}
```

#### Server Statistics

```http
GET /stats
```

**Authentication**: Not required  
**Rate Limit**: 100 req/min

**Response**:
```json
{
  "requests_total": 10000,
  "errors_total": 5,
  "uptime_seconds": 3600,
  "memory_usage_mb": 512
}
```

#### Capabilities

```http
GET /capabilities
```

**Authentication**: Not required  
**Rate Limit**: 100 req/min

**Response**:
```json
{
  "features": {
    "vector_search": true,
    "graph_traversal": true,
    "time_series": true,
    "cdc": true,
    "semantic_cache": true
  },
  "edition": "enterprise"
}
```

#### Prometheus Metrics

```http
GET /metrics
```

**Authentication**: Not required  
**Rate Limit**: 100 req/min

**Response**: Prometheus-formatted metrics

## Error Handling

### Error Response Format

All errors follow this format:

```json
{
  "error": "Error message",
  "code": "ERROR_CODE",
  "status": 400
}
```

### Common Error Codes

| Status Code | Description |
|-------------|-------------|
| 400 | Bad Request - Invalid input |
| 401 | Unauthorized - Missing or invalid authentication |
| 403 | Forbidden - Insufficient permissions |
| 404 | Not Found - Resource does not exist |
| 429 | Too Many Requests - Rate limit exceeded |
| 500 | Internal Server Error - Server error |
| 503 | Service Unavailable - Service temporarily unavailable |

## Best Practices

### Authentication

1. Always use HTTPS in production to protect JWT tokens
2. Store JWT tokens securely (e.g., in httpOnly cookies)
3. Implement token refresh to avoid long-lived tokens
4. Use the minimum required scope for each operation

### Rate Limiting

1. Implement exponential backoff when receiving 429 responses
2. Check `Retry-After` header to determine when to retry
3. Cache results when possible to reduce API calls
4. Use batch operations to reduce the number of requests

### Performance

1. Use pagination for large result sets
2. Implement client-side caching for frequently accessed data
3. Use appropriate indexes for query operations
4. Consider using semantic cache for repeated queries

### Security

1. Never embed credentials in client-side code
2. Validate all input data
3. Use PII pseudonymization for sensitive data
4. Enable audit logging for compliance
5. Regularly rotate PKI certificates

## Configuration

### Enabling Authentication

In the server configuration file:

```yaml
server:
  auth_enabled: true
  jwt_secret: "your-secret-key"
  jwt_issuer: "themisdb"
  jwt_audience: "themisdb-api"
```

### Configuring Rate Limits

```yaml
server:
  rate_limiting_enabled: true
  global_rate_limit_per_minute: 100
  global_rate_limit_burst: 100
  audit_rate_limit_per_minute: 50
```

### Endpoint-Specific Configuration

```yaml
endpoints:
  - pattern: "/entities/*"
    scope: "data:read"
    rate_limit_per_minute: 1000
  - pattern: "/admin/*"
    scope: "admin"
    rate_limit_per_minute: 50
```

## Support

For issues or questions:
- GitHub: https://github.com/makr-code/ThemisDB
- Documentation: https://themisdb.readthedocs.io
- Email: support@themisdb.com
