# 🌐 HTTP API Reference

> **Category:** Core API  
> **Since Version:** 1.3.0  
> **Status:** ✅ Stable  
> **Updated:** December 22, 2025

---

## Overview

This document provides a complete reference for all HTTP endpoints of the ThemisDB REST API. For machine-readable API specifications, see the [OpenAPI Specification](../openapi.yaml).

### Base URL

```
http://localhost:8765
```

In production environments, HTTPS should be used.

### Authentication

Most endpoints require Bearer token authentication:

```
Authorization: Bearer <api_key>
```

API keys can be managed via the [Key Management API](#api-key-management).

---

## Table of Contents

1. [System & Monitoring](#system--monitoring)
2. [Entities (CRUD)](#entities-crud)
3. [Query & AQL](#query--aql)
4. [Index Management](#index-management)
5. [Graph Operations](#graph-operations)
6. [Vector Search](#vector-search)
7. [Content Management](#content-management)
8. [Cache (Semantic Cache)](#cache-semantic-cache)
9. [LLM Integration](#llm-integration)
10. [Change Data Capture (CDC)](#change-data-capture-cdc)
11. [Transaction Management](#transaction-management)
12. [API Key Management](#api-key-management)
13. [PII Operations](#pii-operations)
14. [Audit & Compliance](#audit--compliance)
15. [Classification & Reports](#classification--reports)
16. [Error Handling](#error-handling)

---

## System & Monitoring

### GET /health

Health check endpoint for the server.

**Query Parameters:** None

**Response (200 OK):**
```json
{
  "status": "healthy",
  "version": "1.0.1",
  "database": "rocksdb",
  "uptime_seconds": 3600
}
```

**Error Handling:**
- Returns status 503 if database is unavailable

---

### GET /stats

Detailed server and database statistics.

**Query Parameters:** None

**Response (200 OK):**
```json
{
  "server": {
    "uptime_seconds": 3600,
    "total_requests": 12345,
    "total_errors": 42,
    "queries_per_second": 123.45,
    "threads": 8
  },
  "storage": {
    "rocksdb": {
      "block_cache_usage_bytes": 1048576,
      "block_cache_capacity_bytes": 8388608,
      "estimate_num_keys": 100000,
      "estimate_live_data_size_bytes": 52428800,
      "cache_hit_rate_percent": 95.5,
      "bytes_written": 104857600,
      "bytes_read": 209715200
    },
    "raw_stats": "..."
  }
}
```

**Error Handling:**
- `500 Internal Server Error`: Error retrieving statistics

---

### GET /metrics

Prometheus metrics in text exposition format.

**Query Parameters:** None

**Response (200 OK):**
```
Content-Type: text/plain

# HELP process_uptime_seconds Process uptime in seconds
# TYPE process_uptime_seconds gauge
process_uptime_seconds 3600

# HELP vccdb_requests_total Total HTTP requests handled
# TYPE vccdb_requests_total counter
vccdb_requests_total 12345

# HELP vccdb_errors_total Total HTTP errors
# TYPE vccdb_errors_total counter
vccdb_errors_total 42
```

**Error Handling:**
- `500 Internal Server Error`: Error generating metrics

---

## Entities (CRUD)

### GET /entities/{key}

Read an entity by primary key.

**Path Parameters:**
- `key` (string, required): Primary key in format `table:pk` (e.g., `users:123`)

**Query Parameters:** None

**Response (200 OK):**
```json
{
  "key": "users:123",
  "blob": "{\"name\":\"Alice\",\"age\":30,\"email\":\"alice@example.com\"}"
}
```

**Error Handling:**
- `404 Not Found`: Entity does not exist
- `400 Bad Request`: Invalid key format
- `500 Internal Server Error`: Database error

---

### PUT /entities/{key}

Create or update an entity (upsert).

**Path Parameters:**
- `key` (string, required): Primary key in format `table:pk`

**Request Body:**
```json
{
  "blob": "{\"name\":\"Alice\",\"age\":30,\"email\":\"alice@example.com\"}"
}
```

**Response (201 Created):**
```json
{
  "success": true,
  "key": "users:123",
  "blob_size": 58
}
```

**Error Handling:**
- `400 Bad Request`: Invalid request format or missing blob
- `500 Internal Server Error`: Database error

---

### DELETE /entities/{key}

Delete an entity.

**Path Parameters:**
- `key` (string, required): Primary key in format `table:pk`

**Query Parameters:** None

**Response (200 OK):**
```json
{
  "success": true,
  "key": "users:123"
}
```

**Error Handling:**
- `400 Bad Request`: Invalid key format
- `500 Internal Server Error`: Database error

---

### POST /entities

Create a new entity (with key in body or auto-generated).

**Query Parameters:** None

**Request Body:**
```json
{
  "key": "users:124",
  "blob": "{\"name\":\"Bob\",\"age\":25}"
}
```

If `key` is missing, a UUID is automatically generated.

**Response (201 Created):**
```json
{
  "success": true,
  "key": "users:124",
  "blob_size": 28
}
```

**Error Handling:**
- `400 Bad Request`: Invalid request format
- `500 Internal Server Error`: Database error

---

## Query & AQL

### POST /query

Execute a query with equality and range predicates.

**Query Parameters:** None

**Request Body:**
```json
{
  "table": "users",
  "predicates": [
    { "column": "city", "value": "Berlin" }
  ],
  "range": [
    {
      "column": "age",
      "gte": "25",
      "lte": "35",
      "includeLower": true,
      "includeUpper": true
    }
  ],
  "order_by": {
    "column": "age",
    "desc": false,
    "limit": 10
  },
  "return": "entities",
  "optimize": true,
  "allow_full_scan": false,
  "explain": false
}
```

**Response (200 OK) - with `return: "keys"`:**
```json
{
  "table": "users",
  "count": 42,
  "keys": ["users:123", "users:456", "..."],
  "plan": {
    "mode": "index_optimized",
    "order": [
      { "column": "city", "value": "Berlin" }
    ],
    "estimates": [
      {
        "column": "city",
        "estimated_count": 50,
        "index_exists": true
      }
    ]
  }
}
```

**Response (200 OK) - with `return: "entities"`:**
```json
{
  "table": "users",
  "count": 42,
  "entities": [
    "{\"name\":\"Alice\",\"age\":30,\"city\":\"Berlin\"}",
    "{\"name\":\"Bob\",\"age\":32,\"city\":\"Berlin\"}"
  ]
}
```

**Error Handling:**
- `400 Bad Request`: Invalid query syntax
- `403 Forbidden`: Full scan not allowed and no index available
- `500 Internal Server Error`: Query execution error

---

### POST /query/aql

Execute an AQL (Advanced Query Language) query.

**Query Parameters:** None

**Request Body:**
```json
{
  "query": "FOR user IN users FILTER user.city == 'Berlin' AND user.age >= 25 SORT user.age RETURN user",
  "bind_vars": {
    "minAge": 25
  },
  "options": {
    "profile": false,
    "fullCount": false,
    "maxWarnings": 10,
    "timeout": 30000
  }
}
```

**Response (200 OK):**
```json
{
  "result": [
    {"name": "Alice", "age": 30, "city": "Berlin"},
    {"name": "Bob", "age": 32, "city": "Berlin"}
  ],
  "hasMore": false,
  "cached": false,
  "extra": {
    "warnings": [],
    "stats": {
      "writesExecuted": 0,
      "writesIgnored": 0,
      "scannedFull": 0,
      "scannedIndex": 42,
      "filtered": 0,
      "httpRequests": 0,
      "executionTime": 0.012
    }
  }
}
```

**Error Handling:**
- `400 Bad Request`: Invalid AQL syntax
- `404 Not Found`: Referenced collection not found
- `500 Internal Server Error`: Query execution error

---

#### Pagination Support

AQL queries support multiple pagination strategies for efficient handling of large result sets.

**Request Parameters:**
```json
{
  "query": "FOR user IN users SORT user.name RETURN user",
  "use_cursor": true,
  "cursor": "eyJwayI6InVzZXJzOmFsaWNlIiwiY29sbGVjdGlvbiI6InVzZXJzIiwidmVyc2lvbiI6MX0=",
  "page_size": 100
}
```

**Pagination Parameters:**
- `use_cursor` (boolean): Enable cursor-based pagination
- `cursor` (string, optional): Base64-encoded cursor token from previous page
- `page_size` (integer, optional): Items per page (min: 1, max: 10,000, default: 100)

**Paginated Response:**
```json
{
  "items": [
    {"name": "Alice", "age": 30},
    {"name": "Bob", "age": 32}
  ],
  "has_more": true,
  "next_cursor": "eyJwayI6InVzZXJzOmJvYiIsImNvbGxlY3Rpb24iOiJ1c2VycyIsInZlcnNpb24iOjF9",
  "batch_size": 100,
  "page_info": {
    "page_size": 100,
    "has_next_page": true,
    "has_prev_page": false
  },
  "pagination_method": "cursor"
}
```

**Pagination Methods:**
- **cursor**: Stateless cursor-based pagination (recommended for distributed systems)
- **keyset**: Efficient ORDER BY-based pagination (O(log n) performance)
- **offset**: Traditional offset-based pagination (compatibility)

**Features:**
- ✅ Cursor expiration (1-hour TTL by default)
- ✅ ORDER BY value encoding for keyset pagination (eliminates database lookups)
- ✅ Configurable page size limits prevent memory exhaustion
- ✅ Backward compatible with non-paginated queries
- ✅ Stateless design suitable for distributed systems

**Example - First Page:**
```bash
curl -X POST http://localhost:8765/query/aql \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{
    "query": "FOR user IN users SORT user.name RETURN user",
    "use_cursor": true,
    "page_size": 50
  }'
```

**Example - Next Page:**
```bash
curl -X POST http://localhost:8765/query/aql \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer $TOKEN" \
  -d '{
    "query": "FOR user IN users SORT user.name RETURN user",
    "use_cursor": true,
    "cursor": "eyJwayI6InVzZXJzOmFsaWNlIiwiY29sbGVjdGlvbiI6InVzZXJzIn0=",
    "page_size": 50
  }'
```

**Error Handling:**
- `400 Bad Request`: Invalid cursor or expired cursor
- `400 Bad Request`: Page size out of range (< 1 or > 10,000)

---

## Index Management

### POST /index/create

Create an index on a table column.

**Query Parameters:** None

**Request Body:**
```json
{
  "table": "users",
  "column": "email",
  "index_type": "hash"
}
```

**Index Types:**
- `hash`: Equality lookups (default)
- `range`: Range queries and sorting
- `fulltext`: Full-text search
- `vector`: Vector similarity search (requires `dimension` parameter)

**Response (201 Created):**
```json
{
  "success": true,
  "table": "users",
  "column": "email",
  "index_type": "hash"
}
```

**Error Handling:**
- `400 Bad Request`: Invalid index configuration
- `409 Conflict`: Index already exists
- `500 Internal Server Error`: Index creation failed

---

### DELETE /index/delete

Delete an index.

**Query Parameters:** None

**Request Body:**
```json
{
  "table": "users",
  "column": "email"
}
```

**Response (200 OK):**
```json
{
  "success": true,
  "table": "users",
  "column": "email"
}
```

**Error Handling:**
- `404 Not Found`: Index does not exist
- `500 Internal Server Error`: Index deletion failed

---

### GET /index/list

List all indexes for a table.

**Query Parameters:**
- `table` (string, required): Table name

**Response (200 OK):**
```json
{
  "table": "users",
  "indexes": [
    {
      "column": "email",
      "index_type": "hash",
      "created_at": "2025-01-15T10:30:00Z"
    },
    {
      "column": "age",
      "index_type": "range",
      "created_at": "2025-01-15T10:31:00Z"
    }
  ]
}
```

**Error Handling:**
- `400 Bad Request`: Missing table parameter
- `500 Internal Server Error`: Failed to retrieve indexes

---

## Vector Search

### POST /vector/search

Perform vector similarity search.

**Query Parameters:** None

**Request Body:**
```json
{
  "table": "embeddings",
  "vector": [0.1, 0.2, 0.3, ...],
  "top_k": 10,
  "metric": "cosine",
  "filter": {
    "predicates": [
      { "column": "category", "value": "tech" }
    ]
  }
}
```

**Metrics:**
- `cosine`: Cosine similarity (default)
- `euclidean`: Euclidean distance (L2)
- `dot_product`: Dot product

**Response (200 OK):**
```json
{
  "results": [
    {
      "key": "embeddings:doc1",
      "score": 0.95,
      "blob": "{\"text\":\"Machine learning\",\"embedding\":[...]}"
    },
    {
      "key": "embeddings:doc2",
      "score": 0.89,
      "blob": "{\"text\":\"Deep learning\",\"embedding\":[...]}"
    }
  ],
  "count": 10,
  "search_time_ms": 15
}
```

**Error Handling:**
- `400 Bad Request`: Invalid vector dimensions or parameters
- `404 Not Found`: Vector index not found
- `500 Internal Server Error`: Search execution error

---

### POST /vector/hybrid

Hybrid search combining vector similarity and keyword search.

**Query Parameters:** None

**Request Body:**
```json
{
  "table": "documents",
  "text_query": "machine learning tutorial",
  "vector": [0.1, 0.2, 0.3, ...],
  "top_k": 10,
  "weights": {
    "bm25": 0.5,
    "vector": 0.5
  }
}
```

**Response (200 OK):**
```json
{
  "results": [
    {
      "key": "documents:doc1",
      "combined_score": 0.92,
      "bm25_score": 0.88,
      "vector_score": 0.95,
      "blob": "{\"text\":\"Complete ML tutorial\"}"
    }
  ],
  "count": 10
}
```

**Error Handling:**
- `400 Bad Request`: Invalid parameters or missing indexes
- `500 Internal Server Error`: Search execution error

---

## Graph Operations

### POST /graph/traverse

Traverse graph relationships.

**Query Parameters:** None

**Request Body:**
```json
{
  "start_vertex": "users:alice",
  "edge_collection": "follows",
  "direction": "outbound",
  "min_depth": 1,
  "max_depth": 3,
  "uniqueness": "vertices"
}
```

**Directions:**
- `outbound`: Follow edges from start vertex
- `inbound`: Follow edges to start vertex
- `any`: Follow edges in any direction

**Response (200 OK):**
```json
{
  "vertices": [
    {
      "key": "users:bob",
      "depth": 1,
      "data": "{\"name\":\"Bob\"}"
    },
    {
      "key": "users:charlie",
      "depth": 2,
      "data": "{\"name\":\"Charlie\"}"
    }
  ],
  "edges": [
    {
      "from": "users:alice",
      "to": "users:bob",
      "data": "{\"since\":\"2024-01-15\"}"
    }
  ],
  "paths": [
    ["users:alice", "users:bob", "users:charlie"]
  ]
}
```

**Error Handling:**
- `400 Bad Request`: Invalid traversal parameters
- `404 Not Found`: Start vertex not found
- `500 Internal Server Error`: Traversal execution error

---

## Transaction Management

### POST /transaction/begin

Start a new transaction.

**Query Parameters:** None

**Request Body:**
```json
{
  "isolation_level": "SNAPSHOT",
  "timeout_ms": 30000
}
```

**Isolation Levels:**
- `READ_COMMITTED`: Default isolation level
- `SNAPSHOT`: Snapshot isolation (MVCC)

**Response (201 Created):**
```json
{
  "transaction_id": "txn_abc123def456",
  "isolation_level": "SNAPSHOT",
  "started_at": "2025-12-23T14:00:00Z"
}
```

**Error Handling:**
- `400 Bad Request`: Invalid transaction parameters
- `500 Internal Server Error`: Failed to start transaction

---

### POST /transaction/{txn_id}/commit

Commit a transaction.

**Path Parameters:**
- `txn_id` (string, required): Transaction ID

**Query Parameters:** None

**Response (200 OK):**
```json
{
  "success": true,
  "transaction_id": "txn_abc123def456",
  "committed_at": "2025-12-23T14:01:00Z"
}
```

**Error Handling:**
- `404 Not Found`: Transaction not found
- `409 Conflict`: Transaction conflict detected
- `500 Internal Server Error`: Commit failed

---

### POST /transaction/{txn_id}/rollback

Rollback a transaction.

**Path Parameters:**
- `txn_id` (string, required): Transaction ID

**Query Parameters:** None

**Response (200 OK):**
```json
{
  "success": true,
  "transaction_id": "txn_abc123def456",
  "rolled_back_at": "2025-12-23T14:01:00Z"
}
```

**Error Handling:**
- `404 Not Found`: Transaction not found
- `500 Internal Server Error`: Rollback failed

---

## Error Handling

### Error Response Format

All errors follow this standard format:

```json
{
  "error": {
    "code": "INVALID_REQUEST",
    "message": "Invalid query syntax",
    "details": "Expected FOR keyword at line 1, column 5",
    "request_id": "req_abc123"
  }
}
```

### Common Error Codes

| Code | HTTP Status | Description |
|------|------------|-------------|
| `INVALID_REQUEST` | 400 | Malformed request body or parameters |
| `UNAUTHORIZED` | 401 | Missing or invalid authentication |
| `FORBIDDEN` | 403 | Insufficient permissions |
| `NOT_FOUND` | 404 | Resource not found |
| `CONFLICT` | 409 | Resource conflict (e.g., duplicate key) |
| `RATE_LIMITED` | 429 | Too many requests |
| `INTERNAL_ERROR` | 500 | Server internal error |
| `SERVICE_UNAVAILABLE` | 503 | Service temporarily unavailable |

---

## Rate Limiting

The API implements rate limiting to prevent abuse:

**Response Headers:**
```
X-RateLimit-Limit: 1000
X-RateLimit-Remaining: 995
X-RateLimit-Reset: 1703001600
```

**Rate Limit Exceeded (429):**
```json
{
  "error": {
    "code": "RATE_LIMITED",
    "message": "Rate limit exceeded",
    "retry_after_seconds": 60
  }
}
```

---

## Best Practices

1. **Use HTTPS in Production**: Always use TLS/SSL encryption
2. **Implement Retries**: Use exponential backoff for transient errors
3. **Cache API Keys**: Don't request new keys for every request
4. **Use Batch Operations**: Combine multiple operations when possible
5. **Monitor Rate Limits**: Track usage via response headers
6. **Enable Compression**: Use `Accept-Encoding: gzip` header
7. **Use Transactions**: For multi-operation consistency
8. **Index Appropriately**: Create indexes for frequently queried columns

---

## See Also

- [AQL Reference](../aql/README.md) - Query language documentation
- [OpenAPI Specification](apis_openapi.md) - Machine-readable API spec
- [Authentication Guide](../../de/security/auth.md) - Security configuration
- [GraphQL API](../../de/apis/apis_graphql.md) - GraphQL endpoint documentation

---

> **Note:** For the most detailed and up-to-date information, please refer to the [German HTTP API reference](../../de/apis/HTTP_API_REFERENCE.md).

**Version:** 1.3.0 | **License:** MIT | **Support:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
