# ThemisDB Integration Guide

> How to integrate ThemisDB into your application, CI/CD pipeline, or existing system.

**Version:** 1.9.0-beta | **Updated:** 2026-05

---

## Table of Contents

1. [Quick Integration Overview](#1-quick-integration-overview)
2. [REST API Integration](#2-rest-api-integration)
3. [Client SDKs](#3-client-sdks)
4. [Docker / Docker Compose](#4-docker--docker-compose)
5. [Kubernetes / Helm](#5-kubernetes--helm)
6. [Authentication Setup](#6-authentication-setup)
7. [Python Client Example](#7-python-client-example)
8. [JavaScript / Node.js Client Example](#8-javascript--nodejs-client-example)
9. [gRPC Integration](#9-grpc-integration)
10. [Environment Variables](#10-environment-variables)
11. [Health Checks & Monitoring](#11-health-checks--monitoring)
12. [Common Integration Patterns](#12-common-integration-patterns)

---

## 1. Quick Integration Overview

ThemisDB exposes three integration points:

| Interface | Port | Best For |
|-----------|------|----------|
| REST / HTTP | 8080 | Web apps, scripting, any language |
| gRPC / Binary | 18765 | High-throughput services, polyglot |
| GraphQL | 8080 (`/graphql`) | Flexible front-end queries |

**Minimum steps to integrate:**

```bash
# 1. Start ThemisDB
docker run -d --name themisdb -p 8080:8080 themisdb/themisdb:latest

# 2. Verify connectivity
curl http://localhost:8080/health
# → {"status":"ok","version":"1.9.0-beta"}

# 3. Write your first record
curl -X PUT http://localhost:8080/entities/myapp:record-1 \
  -H "Content-Type: application/json" \
  -d '{"blob":"{\"hello\":\"world\"}"}'

# 4. Read it back
curl http://localhost:8080/entities/myapp:record-1
```

---

## 2. REST API Integration

The REST API follows a consistent pattern across all resource types.

### Base URL

```
http://<host>:8080
```

### Key Endpoints

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/health` | Health check |
| `GET` | `/info` | Version & feature info |
| `PUT` | `/entities/{collection}:{id}` | Create or update entity |
| `GET` | `/entities/{collection}:{id}` | Read entity |
| `DELETE` | `/entities/{collection}:{id}` | Delete entity |
| `POST` | `/entities/batch` | Batch write |
| `POST` | `/query` | Query with predicates |
| `POST` | `/aql` | AQL query |
| `POST` | `/index/create` | Create secondary index |
| `POST` | `/vector/search` | kNN vector search |
| `POST` | `/graph/traverse` | Graph traversal |
| `POST` | `/transaction/begin` | Start transaction |
| `POST` | `/transaction/commit` | Commit transaction |
| `POST` | `/transaction/rollback` | Roll back transaction |

See [api/REST_API_REFERENCE.md](api/REST_API_REFERENCE.md) for the full reference.

---

## 3. Client SDKs

Native client libraries are available for several languages:

| Language | Location | Status |
|----------|----------|--------|
| Python | [`clients/python/`](../clients/) | Available |
| JavaScript/Node.js | [`clients/`](../clients/) | Available |
| Go | [`clients/`](../clients/) | Available |
| Java | [`clients/`](../clients/) | Available |
| C++ | Native (included in ThemisDB) | Available |

See [`clients/README.md`](../clients/README.md) for installation instructions per language.

---

## 4. Docker / Docker Compose

### Single Container

```bash
docker run -d \
  --name themisdb \
  -p 8080:8080 \
  -p 18765:18765 \
  -p 4318:4318 \
  -v themis_data:/data \
  -e THEMIS_LOG_LEVEL=info \
  themisdb/themisdb:latest
```

### Docker Compose (Production-Ready)

```yaml
# docker-compose.yml
version: "3.9"

services:
  themisdb:
    image: themisdb/themisdb:latest
    container_name: themisdb
    restart: unless-stopped
    ports:
      - "8080:8080"
      - "18765:18765"
      - "4318:4318"
    volumes:
      - themis_data:/data
      - ./config.yaml:/etc/themis/config.yaml:ro
    environment:
      - THEMIS_CONFIG=/etc/themis/config.yaml
      - THEMIS_LOG_LEVEL=info
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:8080/health"]
      interval: 30s
      timeout: 5s
      retries: 3

volumes:
  themis_data:
```

```bash
docker compose up -d
docker compose logs -f themisdb
```

---

## 5. Kubernetes / Helm

A Helm chart is available under [`helm/`](../helm/):

```bash
# Install with Helm
helm install themisdb ./helm/themisdb \
  --namespace themisdb \
  --create-namespace \
  --set persistence.size=50Gi \
  --set resources.limits.memory=8Gi

# Check status
kubectl get pods -n themisdb
kubectl logs -n themisdb deployment/themisdb
```

Refer to [`helm/themisdb/values.yaml`](../helm/themisdb/values.yaml) for all configuration options.

---

## 6. Authentication Setup

### JWT Authentication (Default)

```bash
# 1. Obtain a token
curl -X POST http://localhost:8080/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"changeme"}'
# → {"token":"<jwt>","expires_in":3600}

# 2. Use the token
curl http://localhost:8080/entities/users:alice \
  -H "Authorization: Bearer <jwt>"
```

### API Key Authentication

```bash
curl http://localhost:8080/entities/users:alice \
  -H "X-Api-Key: your-api-key-here"
```

For full auth configuration including RBAC, LDAP, and mTLS, see:
- [guides/ad_ldap_integration_guide.md](guides/ad_ldap_integration_guide.md)
- [de/guides/guides_rbac.md](de/guides/guides_rbac.md)
- [security/api_authentication_authorization.md](security/api_authentication_authorization.md)

---

## 7. Python Client Example

```python
import requests

BASE_URL = "http://localhost:8080"
HEADERS = {"Content-Type": "application/json"}

class ThemisClient:
    def __init__(self, base_url: str, token: str | None = None):
        self.base_url = base_url
        self.session = requests.Session()
        self.session.headers.update({"Content-Type": "application/json"})
        if token:
            self.session.headers["Authorization"] = f"Bearer {token}"

    def put(self, collection: str, entity_id: str, data: dict) -> dict:
        import json
        resp = self.session.put(
            f"{self.base_url}/entities/{collection}:{entity_id}",
            json={"blob": json.dumps(data)},
        )
        resp.raise_for_status()
        return resp.json()

    def get(self, collection: str, entity_id: str) -> dict:
        resp = self.session.get(
            f"{self.base_url}/entities/{collection}:{entity_id}"
        )
        resp.raise_for_status()
        return resp.json()

    def delete(self, collection: str, entity_id: str) -> dict:
        resp = self.session.delete(
            f"{self.base_url}/entities/{collection}:{entity_id}"
        )
        resp.raise_for_status()
        return resp.json()

    def query(self, table: str, predicates: list, limit: int = 100) -> dict:
        resp = self.session.post(
            f"{self.base_url}/query",
            json={"table": table, "predicates": predicates,
                  "limit": limit, "return": "entities"},
        )
        resp.raise_for_status()
        return resp.json()

    def vector_search(self, collection: str, vector: list, k: int = 10) -> dict:
        resp = self.session.post(
            f"{self.base_url}/vector/search",
            json={"collection": collection, "vector": vector,
                  "k": k, "metric": "cosine"},
        )
        resp.raise_for_status()
        return resp.json()


# --- Usage ---
client = ThemisClient("http://localhost:8080")

# Create
client.put("users", "alice", {"name": "Alice", "age": 30, "city": "Berlin"})

# Read
record = client.get("users", "alice")
print(record)

# Query
results = client.query("users", [{"column": "city", "value": "Berlin"}])
print(results)
```

---

## 8. JavaScript / Node.js Client Example

```javascript
// themis-client.js
const BASE_URL = process.env.THEMIS_URL ?? "http://localhost:8080";

async function request(method, path, body = undefined) {
  const res = await fetch(`${BASE_URL}${path}`, {
    method,
    headers: { "Content-Type": "application/json" },
    body: body !== undefined ? JSON.stringify(body) : undefined,
  });
  if (!res.ok) throw new Error(`ThemisDB ${method} ${path}: ${res.status}`);
  return res.json();
}

const themis = {
  put: (col, id, data) =>
    request("PUT", `/entities/${col}:${id}`, { blob: JSON.stringify(data) }),
  get: (col, id) =>
    request("GET", `/entities/${col}:${id}`),
  delete: (col, id) =>
    request("DELETE", `/entities/${col}:${id}`),
  query: (table, predicates, limit = 100) =>
    request("POST", "/query", { table, predicates, limit, return: "entities" }),
  vectorSearch: (collection, vector, k = 10) =>
    request("POST", "/vector/search", { collection, vector, k, metric: "cosine" }),
};

// --- Usage ---
(async () => {
  await themis.put("products", "widget-1", { name: "Widget", price: 9.99 });
  const rec = await themis.get("products", "widget-1");
  console.log(rec);
})();
```

---

## 9. gRPC Integration

```bash
# Protocol buffer definitions
ls proto/

# Generate client stubs (example for Python)
python -m grpc_tools.protoc \
  -I proto/ \
  --python_out=./client \
  --grpc_python_out=./client \
  proto/themis.proto

# Connect on port 18765
grpcurl -plaintext localhost:18765 list
```

See [`proto/`](../proto/) for all `.proto` files and [`docs/de/rpc_grpc/`](de/rpc_grpc/) for gRPC-specific docs.

---

## 10. Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `THEMIS_CONFIG` | `config.yaml` | Config file path |
| `THEMIS_LOG_LEVEL` | `info` | Log level (`debug`/`info`/`warn`/`error`) |
| `THEMIS_DATA_DIR` | `./data` | Storage directory |
| `THEMIS_PORT` | `8080` | HTTP port |
| `THEMIS_GRPC_PORT` | `18765` | gRPC port |
| `THEMIS_ALLOW_STUB_HSM` | `false` | Allow development stub HSM (**not for production**) |

---

## 11. Health Checks & Monitoring

```bash
# Liveness
curl http://localhost:8080/health
# → {"status":"ok","version":"1.9.0-beta"}

# Readiness (storage ready)
curl http://localhost:8080/ready

# Prometheus metrics
curl http://localhost:4318/metrics

# Key metrics
# themis_requests_total          – total request count by endpoint
# themis_request_duration_ms     – request latency histogram
# themis_storage_bytes_used      – storage utilisation
# themis_vector_search_duration  – vector search latency
```

Grafana dashboard is available at [`grafana/`](../grafana/).

---

## 12. Common Integration Patterns

### Pattern 1: Connection Pooling

When making many concurrent requests from the same service, reuse HTTP connections:

```python
# Python – use a Session with pool_maxsize
session = requests.Session()
adapter = requests.adapters.HTTPAdapter(pool_maxsize=20)
session.mount("http://", adapter)
```

### Pattern 2: Transactional Writes

Use transactions when multiple writes must succeed or fail together:

```bash
TX=$(curl -s -X POST $BASE/transaction/begin | jq -r .tx_id)

curl -X PUT $BASE/entities/accounts:alice -H "X-Transaction-ID: $TX" \
  -d '{"blob":"{\"balance\":900}"}'
curl -X PUT $BASE/entities/accounts:bob   -H "X-Transaction-ID: $TX" \
  -d '{"blob":"{\"balance\":1100}"}'

curl -X POST $BASE/transaction/commit -d "{\"tx_id\":\"$TX\"}"
```

### Pattern 3: Retry with Back-off

```python
import time, requests

def with_retry(fn, max_attempts=3, base_delay=0.5):
    for attempt in range(max_attempts):
        try:
            return fn()
        except requests.HTTPError as e:
            if e.response.status_code < 500 or attempt == max_attempts - 1:
                raise
            time.sleep(base_delay * 2 ** attempt)
```

### Pattern 4: Bulk Loading

Use batch operations for large data loads:

```python
BATCH_SIZE = 500

def bulk_load(client_session, records):
    for i in range(0, len(records), BATCH_SIZE):
        batch = records[i:i + BATCH_SIZE]
        operations = [
            {"op": "put", "id": f"data:{r['id']}", "blob": json.dumps(r)}
            for r in batch
        ]
        client_session.post(f"{BASE_URL}/entities/batch",
                            json={"operations": operations}).raise_for_status()
```

---

## Related Documentation

| Document | Description |
|----------|-------------|
| [QUICK_REFERENCE.md](QUICK_REFERENCE.md) | Commands cheat sheet |
| [api/REST_API_REFERENCE.md](api/REST_API_REFERENCE.md) | Full REST endpoint reference |
| [api/AUTHENTICATION_AND_RATE_LIMITING.md](api/AUTHENTICATION_AND_RATE_LIMITING.md) | Auth & rate limiting |
| [de/guides/guides_deployment.md](de/guides/guides_deployment.md) | Deployment guide |
| [security/HSM_PRODUCTION_SETUP.md](security/HSM_PRODUCTION_SETUP.md) | Production security |
| [FAQ.md](FAQ.md) | Frequently asked questions |
