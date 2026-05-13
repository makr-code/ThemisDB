# ThemisDB API Reference

> Complete reference for all ThemisDB interfaces: REST, GraphQL, gRPC, and the AQL query language.

---

## 📄 Documents in This Directory

| Document | Description |
|----------|-------------|
| [API_REFERENCE.md](API_REFERENCE.md) | Complete API reference (all endpoints) |
| [REST_API_REFERENCE.md](REST_API_REFERENCE.md) | REST API endpoint reference |
| [API_VERSIONING.md](API_VERSIONING.md) | API versioning policy |
| [API_VERSIONING_IMPLEMENTATION.md](API_VERSIONING_IMPLEMENTATION.md) | Implementation notes for API versioning |
| [AUTHENTICATION_AND_RATE_LIMITING.md](AUTHENTICATION_AND_RATE_LIMITING.md) | Auth flows and rate-limit configuration |
| [DEPRECATION_REGISTRY.md](DEPRECATION_REGISTRY.md) | Deprecated endpoints and migration paths |
| [RPC_GEOSPATIAL_QUERY.md](RPC_GEOSPATIAL_QUERY.md) | gRPC geospatial query API |
| [rope_rest_api.md](rope_rest_api.md) | ROPE (Rotary Positional Embedding) REST API |
| [BRANCH_API_OPENAPI.md](BRANCH_API_OPENAPI.md) | Branch/versioning API (OpenAPI spec) |
| [RESOURCE_LIMITS_GUIDE.md](RESOURCE_LIMITS_GUIDE.md) | Resource limits and quota configuration |

---

## 🔌 Interface Overview

### REST API (Port 8080)

The primary interface for all database operations.

```bash
# Example: store an entity
curl -X PUT http://localhost:8080/entities/users:alice \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"name\":\"Alice\"}"}'

# Example: query
curl -X POST http://localhost:8080/query \
  -H "Content-Type: application/json" \
  -d '{"table":"users","predicates":[{"column":"name","value":"Alice"}]}'
```

See [REST_API_REFERENCE.md](REST_API_REFERENCE.md) for the full endpoint list.

### GraphQL (Port 8080, path `/graphql`)

```graphql
query {
  entity(id: "users:alice") {
    id
    blob
    version
  }
}
```

### gRPC / Binary Wire Protocol (Port 18765)

Protocol buffer definitions are in [`proto/`](../../proto/).

```bash
# List available gRPC services
grpcurl -plaintext localhost:18765 list
```

### AQL – Advanced Query Language

AQL provides SQL-like syntax extended with graph traversal and vector operations.

```sql
-- Relational query
SELECT * FROM users WHERE age > 25 LIMIT 10;

-- Graph traversal
TRAVERSE OUTGOING users:alice DEPTH 3;

-- Vector similarity
SELECT * FROM embeddings ORDER BY DISTANCE(vector, [0.1, 0.2, 0.3]) LIMIT 5;
```

See [de/aql/aql_syntax.md](../de/aql/aql_syntax.md) for the full AQL reference.

---

## 🔑 Authentication

ThemisDB uses JWT-based authentication. See [AUTHENTICATION_AND_RATE_LIMITING.md](AUTHENTICATION_AND_RATE_LIMITING.md) for setup.

```bash
# Obtain a token
curl -X POST http://localhost:8080/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"changeme"}'

# Use token in subsequent requests
curl http://localhost:8080/entities/users:alice \
  -H "Authorization: Bearer <token>"
```

---

## 🔗 Related

- [Quick Reference](../QUICK_REFERENCE.md) – common commands cheat sheet
- [Integration Guide](../INTEGRATION_GUIDE.md) – client SDK and integration patterns
- [Security docs](../security/README.md) – auth, TLS, and HSM configuration

