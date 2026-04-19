# Server Module — Architecture Guide

**Version:** 1.1
**Last Updated:** 2026-04-06
**Status:** `current`
**Validated:** 2026-03-10 (Commit `a04b89b`)
**Module Path:** `src/server/`

---

## 1. Overview

The Server module provides ThemisDB's complete API surface: HTTP/1.1, HTTP/2, HTTP/3,
WebSocket, MQTT, PostgreSQL wire protocol, gRPC, and a Model Context Protocol (MCP) server
for AI integrations. Built on Boost.Beast/Asio, it exposes 40+ specialized REST endpoints
for multi-model data operations, governance, LLM inference, observability, and administration.

---

## 2. Design Principles

- **Handler-Per-Domain** – each API domain (entities, LLM, graph, timeseries, CDC, etc.)
  has its own handler file, keeping the routing table small and handlers focused.
- **Middleware Pipeline** – every request passes through auth, rate limiting, and logging
  middleware before reaching a handler; middleware is composable.
- **Async I/O** – all I/O is non-blocking via Boost.Asio; the server scales to thousands
  of concurrent connections on a fixed thread pool.
- **Protocol Multiplexing** – a single server process handles HTTP, WebSocket, gRPC, MQTT,
  and PostgreSQL wire on dedicated ports.
- **Multi-Tenant** – tenant isolation is enforced at the middleware level; no handler
  needs to implement tenant filtering manually.

---

## 3. Component Architecture

### 3.1 Key Components (selected)

| File | Role |
|---|---|
| `server.cpp` | Server lifecycle: init, start, graceful shutdown |
| `api_gateway.cpp` | Request routing, versioning, API key validation |
| `auth_middleware.cpp` | Authentication and authorization middleware |
| `api_auth_config.cpp` | Per-endpoint auth requirements |
| `entity_api_handler.cpp` | CRUD for documents and collections |
| `llm_api_handler.cpp` | LLM INFER/RAG/EMBED API |
| `query_api_handler.cpp` | AQL query execution |
| `vector_api_handler.cpp` | Vector search and embedding management |
| `graph_api_handler.cpp` | Graph traversal and management |
| `timeseries_api_handler.cpp` | Time-series operations |
| `changefeed_api_handler.cpp` | CDC SSE subscriptions |
| `cache_api_handler.cpp` | Cache management API |
| `audit_api_handler.cpp` | Audit log access |
| `admin_api_handler.cpp` | Admin operations |
| `ethics_api_handler.cpp` | Ethics and governance API |
| `compliance_reporting_api_handler.cpp` | Compliance report generation |
| `export_api_handler.cpp` | Data export API |
| `import_api_handler.cpp` | Data import API |
| `bpmn_api_handler.cpp` | BPMN process management API |
| `distributed_txn_api_handler.cpp` | Distributed transaction API |
| `async_job_api_handler.cpp` | Async job management |
| `chunked_response_writer.cpp` | Chunked/streaming HTTP responses |
| `buffer_api_handler.cpp` / `buffer_binary_protocol.cpp` | Binary buffer API |
| `diff_api_handler.cpp` | Data diff and comparison API |
| `rpc/` | gRPC service implementations |
| `middleware/` | Auth, logging, rate limiting, CORS middleware |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                   Client (HTTP/WS/gRPC/MQTT/PG)                 │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                   Boost.Beast Async I/O                          │
│   HTTP/1.1 │ HTTP/2 │ HTTP/3 │ WebSocket │ gRPC │ MQTT │ PG     │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                     Middleware Pipeline                          │
│  auth_middleware → rate_limiter → logging → CORS → tenant_iso   │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                      API Gateway (Router)                        │
│   /api/v1/entities   → entity_api_handler                       │
│   /api/v1/query      → query_api_handler                        │
│   /api/v1/llm/*      → llm_api_handler                         │
│   /api/v1/vector/*   → vector_api_handler                      │
│   /api/v1/graph/*    → graph_api_handler                       │
│   /api/v1/changefeed → changefeed_api_handler (SSE)            │
│   /api/v1/export     → export_api_handler                      │
│   /metrics           → observability (Prometheus)               │
│   /health/ready|live → health probes                            │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 REST Request Lifecycle

```
HTTP GET /api/v1/entities/users/doc123
    │
    ├─ Boost.Beast: accept + parse HTTP frame
    ├─ auth_middleware: JWT/API key validation → principal
    ├─ rate_limiter: per-IP + per-tenant token bucket
    ├─ CORS: add headers if needed
    ├─ API gateway: route to entity_api_handler
    ├─ entity_api_handler: call storage.get("users", "doc123")
    ├─ apply field encryption + PII redaction (security module)
    ├─ serialize JSON response
    └─ HTTP 200 OK + response body
```

### 4.2 SSE Changefeed

```
GET /api/v1/changefeed?collection=users  (SSE long-poll)
    │
    ├─ auth_middleware: validate
    ├─ changefeed_api_handler: register SSE subscriber
    │       → SSE response stays open
    │
    ├─ CDC module: mutation event
    │       → filter → SSE frame written to response
    │
    └─ client disconnect → clean up subscription
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Delegates to** | `src/query/` | AQL execution |
| **Delegates to** | `src/llm/` | LLM inference API |
| **Delegates to** | `src/storage/` | Document CRUD |
| **Delegates to** | `src/index/` | Vector search, graph traversal |
| **Uses** | `src/auth/` | JWT/OIDC validation |
| **Uses** | `src/governance/` | Policy evaluation |
| **Uses** | `src/security/` | RBAC, RLS, encryption |
| **Uses** | `src/cdc/` | Changefeed SSE delivery |
| **Provides** | External clients | REST / WebSocket / gRPC / MQTT / PG wire |

---

## 6. Threading & Concurrency Model

- Boost.Asio thread pool (default: `hardware_concurrency` threads); all I/O is async.
- Each request is dispatched to a strand; no mutex needed for per-request state.
- gRPC completion queues run on separate threads.
- MQTT broker uses a dedicated event loop.
- Graceful shutdown drains in-flight requests before stopping.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Async I/O | Thousands of concurrent connections on a small thread pool |
| Keep-Alive | Persistent HTTP connections reduce TCP handshake overhead |
| Response compression | Gzip/Brotli/Zstd reduces bandwidth for large result sets |
| Chunked responses | Large result sets streamed in chunks without full buffering |
| Connection pool | Reuse connections to backend modules |

---

## 8. Security Considerations

- TLS 1.3 required for all external connections.
- Per-endpoint auth requirements configurable via `api_auth_config.cpp`.
- Rate limiting prevents DDoS and per-tenant abuse.
- Load shedding protects server from overload.
- Admin endpoints require USB token or elevated role.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `server.http.port` | 8080 | HTTP port |
| `server.https.port` | 8443 | HTTPS port |
| `server.threads` | cpu_count | Boost.Asio thread pool size |
| `server.max_connections` | 10000 | Max concurrent connections |
| `server.read_timeout_ms` | 30000 | Request read timeout |
| `server.write_timeout_ms` | 30000 | Response write timeout |
| `server.compression.enabled` | true | Enable response compression |
| `server.grpc.port` | 8484 | gRPC port |
| `server.mqtt.port` | 1883 | MQTT port |

---

## 10. Error Handling

| Error | HTTP Code | Strategy |
|---|---|---|
| Auth failure | 401/403 | Return WWW-Authenticate; log |
| Rate limited | 429 | Return Retry-After header |
| Query timeout | 408 | Cancel query; return error |
| Payload too large | 413 | Reject early; log |
| Backend error | 500/503 | Return structured error; log |
| Service overload | 503 | Load shedding: return 503 + Retry-After |

---

## 11. Known Limitations & Future Work

- HTTP/3 (QUIC) support is in progress.
- PostgreSQL wire protocol support is partial; full SQL compatibility planned.
- GraphQL endpoint is planned.
- MCP (Model Context Protocol) server is experimental.

---

## 12. References

- `src/server/README.md` — module overview
- `docs/api/` — REST API documentation
- `docs/API_REFERENCE.md` — API reference
- `ARCHITECTURE.md` (root) — full system architecture
