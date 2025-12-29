# 🔌 API & Ingestion Documentation

> **Category:** Core API  
> **Since Version:** 1.3.0  
> **Status:** ✅ Stable  
> **Updated:** December 22, 2025

---

## 📋 Table of Contents

- [🎯 Overview](#-overview)
- [📊 Available APIs](#-available-apis)
- [🚀 Getting Started](#-getting-started)
- [📖 API Documentation](#-api-documentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 See Also](#-see-also)

---

## 🎯 Overview

Data ingestion, queries, and API documentation for ThemisDB. This documentation covers all HTTP APIs, GraphQL, OpenAPI, and additional protocols.

## Source Code Reference

| Component | Header | Source |
|-----------|--------|--------|
| ContentManager | `include/content/content_manager.h` | `src/content/content_manager.cpp` |
| ContentProcessor | `include/content/content_processor.h` | `src/content/content_processor.cpp` |

## Documentation in This Directory

| File | Description |
|------|-------------|
| [json_ingestion_spec.md](../../de/apis/json_ingestion_spec.md) | JSON Ingestion Specification |
| [HTTP_API_REFERENCE.md](../../de/apis/HTTP_API_REFERENCE.md) | HTTP API Reference |
| [apis_graphql.md](../../de/apis/apis_graphql.md) | GraphQL API |
| [apis_openapi.md](../../de/apis/apis_openapi.md) | OpenAPI Specification |
| [apis_hybrid_search.md](../../de/apis/apis_hybrid_search.md) | Hybrid Search API |
| [apis_contentfs.md](../../de/apis/apis_contentfs.md) | Content Filesystem API |

## Protocol Support

ThemisDB supports multiple modern protocols for different use cases:

### HTTP/2 & HTTP/3
- [HTTP2_HTTP3_PROTOCOL_SUPPORT.md](../../de/apis/HTTP2_HTTP3_PROTOCOL_SUPPORT.md) - Protocol support overview
- [HTTP2_HTTP3_USAGE_GUIDE.md](../../de/apis/HTTP2_HTTP3_USAGE_GUIDE.md) - Usage guide
- [HTTP2_SERVER_PUSH_CDC.md](../../de/apis/HTTP2_SERVER_PUSH_CDC.md) - Server push for change data capture

### Model Context Protocol (MCP)
- [MCP_PROTOCOL_SUPPORT.md](../../de/apis/MCP_PROTOCOL_SUPPORT.md) - MCP support overview
- [MCP_TRANSPORTS.md](../../de/apis/MCP_TRANSPORTS.md) - MCP transport layers
- [MCP_WINDOWS_SUPPORT.md](../../de/apis/MCP_WINDOWS_SUPPORT.md) - Windows support
- [MCP_OFFICE_PLUGINS.md](../../de/apis/MCP_OFFICE_PLUGINS.md) - Office plugin integration

### Additional Protocols
- [ADDITIONAL_PROTOCOLS.md](../../de/apis/ADDITIONAL_PROTOCOLS.md) - Overview of additional protocols
- [OPTIONAL_PROTOCOLS.md](../../de/apis/OPTIONAL_PROTOCOLS.md) - Optional protocol features
- [PROTOCOL_BUILD_SWITCHES.md](../../de/apis/PROTOCOL_BUILD_SWITCHES.md) - Build configuration

## Related Documentation

- [Content Module](../../de/content/README.md) - Content Pipeline
- [Query Language (AQL)](../../de/aql/) - Advanced Query Language
- [REST API](../../de/apis/HTTP_API_REFERENCE.md) - REST API Reference

---

## 📊 Available APIs

### 1. REST API (HTTP/1.1, HTTP/2, HTTP/3)
- **CRUD Operations**: Create, Read, Update, Delete entities
- **Batch Operations**: Bulk inserts and updates
- **Query Interface**: Execute AQL queries
- **Admin Endpoints**: System management and monitoring

### 2. GraphQL API
- **Schema Introspection**: Discover available types and operations
- **Flexible Queries**: Request exactly the data you need
- **Mutations**: Modify data through GraphQL
- **Subscriptions**: Real-time data updates

### 3. OpenAPI (Swagger)
- **Interactive Documentation**: Try APIs directly in browser
- **Code Generation**: Generate client libraries
- **Schema Validation**: Request/response validation

### 4. WebSocket API
- **Real-time Updates**: Subscribe to data changes
- **Streaming Queries**: Continuous query results
- **Bidirectional Communication**: Full-duplex communication

### 5. gRPC API
- **High Performance**: Binary protocol with HTTP/2
- **Streaming Support**: Server/client/bidirectional streaming
- **Type Safety**: Strongly typed with Protocol Buffers

---

## 🚀 Getting Started

### Quick Start with REST API

```bash
# Create an entity
curl -X POST http://localhost:8765/api/v1/entities \
  -H "Content-Type: application/json" \
  -d '{
    "collection": "users",
    "data": {
      "name": "Alice",
      "email": "alice@example.com",
      "age": 30
    }
  }'

# Query entities
curl -X POST http://localhost:8765/api/v1/query \
  -H "Content-Type: application/json" \
  -d '{
    "query": "FOR u IN users FILTER u.age > 25 RETURN u"
  }'
```

### Quick Start with GraphQL

```graphql
# Query
query {
  users(filter: { age: { gt: 25 } }) {
    id
    name
    email
    age
  }
}

# Mutation
mutation {
  createUser(input: {
    name: "Bob"
    email: "bob@example.com"
    age: 28
  }) {
    id
    name
  }
}

# Subscription
subscription {
  userChanges {
    operation
    user {
      id
      name
    }
  }
}
```

---

## 📖 API Documentation

### Authentication

All APIs support multiple authentication methods:

```bash
# API Key (Header)
curl -H "X-API-Key: your-api-key" http://localhost:8765/api/v1/entities

# Bearer Token (OAuth2/JWT)
curl -H "Authorization: Bearer your-jwt-token" http://localhost:8765/api/v1/entities

# Basic Auth
curl -u username:password http://localhost:8765/api/v1/entities
```

### Common Operations

#### 1. Entity Operations

```bash
# Create
POST /api/v1/entities
{
  "collection": "products",
  "data": { ... }
}

# Read
GET /api/v1/entities/{collection}/{id}

# Update
PUT /api/v1/entities/{collection}/{id}
{
  "data": { ... }
}

# Delete
DELETE /api/v1/entities/{collection}/{id}
```

#### 2. Query Operations

```bash
# Execute AQL Query
POST /api/v1/query
{
  "query": "FOR doc IN collection RETURN doc",
  "bindVars": { ... }
}

# Explain Query
POST /api/v1/query/explain
{
  "query": "..."
}
```

#### 3. Batch Operations

```bash
# Batch Insert
POST /api/v1/batch/insert
{
  "collection": "users",
  "documents": [
    { "name": "Alice" },
    { "name": "Bob" }
  ]
}

# Batch Update
POST /api/v1/batch/update
{
  "operations": [
    { "id": "1", "data": { ... } },
    { "id": "2", "data": { ... } }
  ]
}
```

#### 4. Index Operations

```bash
# Create Index
POST /api/v1/indexes
{
  "collection": "users",
  "fields": ["email"],
  "type": "hash"
}

# List Indexes
GET /api/v1/indexes/{collection}

# Drop Index
DELETE /api/v1/indexes/{collection}/{index_name}
```

---

## 💡 Best Practices

### 1. Use Batch Operations
```bash
# Bad: Multiple single requests
for item in items:
    POST /api/v1/entities  # Slow

# Good: One batch request
POST /api/v1/batch/insert  # 10-100x faster
```

### 2. Enable Compression
```bash
# Enable gzip compression
curl -H "Accept-Encoding: gzip" \
     --compressed \
     http://localhost:8765/api/v1/entities
```

### 3. Use Connection Pooling
```python
# Python example
from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util.retry import Retry

session = requests.Session()
retry = Retry(total=3, backoff_factor=0.3)
adapter = HTTPAdapter(max_retries=retry, pool_connections=10, pool_maxsize=10)
session.mount('http://', adapter)
```

### 4. Implement Pagination
```bash
# Use limit and offset
POST /api/v1/query
{
  "query": "FOR doc IN collection LIMIT @offset, @limit RETURN doc",
  "bindVars": {
    "offset": 0,
    "limit": 100
  }
}
```

### 5. Use Field Projection
```bash
# Only request needed fields
POST /api/v1/query
{
  "query": "FOR u IN users RETURN { name: u.name, email: u.email }"
}
```

---

## 🔧 Troubleshooting

### Common Issues

#### 1. Connection Timeout
```bash
# Increase timeout
curl --connect-timeout 30 \
     --max-time 60 \
     http://localhost:8765/api/v1/entities
```

#### 2. Rate Limiting
```bash
# Check rate limit headers
curl -I http://localhost:8765/api/v1/entities

# Response headers:
# X-RateLimit-Limit: 1000
# X-RateLimit-Remaining: 999
# X-RateLimit-Reset: 1640000000
```

#### 3. Invalid JSON
```bash
# Validate JSON before sending
echo '{ "data": {...} }' | jq .

# Use proper Content-Type
curl -H "Content-Type: application/json" ...
```

#### 4. Authentication Errors
```bash
# Verify API key is valid
curl -H "X-API-Key: your-key" \
     http://localhost:8765/api/v1/auth/verify

# Check token expiration
jwt decode your-jwt-token
```

---

## 📚 See Also

- [AQL Query Language](../../de/aql/) - Advanced Query Language documentation
- [Client SDKs](../../clients/) - Language-specific client libraries
- [Security Documentation](../security/) - Authentication and authorization
- [Performance Tuning](../../de/performance/) - API optimization guides

---

> **Note:** Most detailed API documentation is currently available in German. English translations are in progress.  
> For the most up-to-date information, please refer to the [German API documentation](../../de/apis/).

---

**Version:** 1.3.0 | **License:** MIT | **Support:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
