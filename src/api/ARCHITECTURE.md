# API Module — Architecture Guide

**Version:** 1.0  
**Last Updated:** 2026-02-24  
**Module Path:** `src/api/`

---

## 1. Overview

The API module provides ThemisDB's multi-protocol frontend: REST/HTTP, gRPC, WebSocket, and
GraphQL. It is deliberately thin — it adapts wire protocols to internal request objects and
delegates all business logic to the `server` module's handler layer. This separation ensures
that adding a new protocol requires touching only `src/api/`, not the query or storage layers.

---

## 2. Design Principles

- **Protocol Adapters Only** – no business logic lives in this module; it translates wire
  formats to internal request structures.
- **Multi-Protocol** – REST, gRPC, WebSocket, and GraphQL are all first-class citizens.
- **Stateless Routing** – each request is independently routed; session state is held in
  the server/auth layers, not here.
- **Streaming-First** – WebSocket and gRPC streaming are designed for real-time
  subscriptions (CDC changefeeds, query results, LLM token streams).

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `http_server.cpp` | HTTP/1.1 and HTTP/2 server setup, routing table, middleware chain |
| `grpc_server.cpp` | gRPC server initialization and service registration |
| `themisdb_grpc_service.cpp` | gRPC service implementation (protocol buffer bridge) |
| `ws_handler.cpp` | WebSocket upgrade handler and frame dispatcher |
| `graphql.cpp` | GraphQL schema, resolver dispatch, and subscription support |
| `geo_index_hooks.cpp` | Geospatial API hooks for spatial index endpoints |

### 3.2 Component Diagram

```
┌──────────────────────────────────────────────────────────────────┐
│                    Network Clients                               │
│   HTTP/REST   │   gRPC   │   WebSocket   │   GraphQL            │
└───────┬───────┴────┬─────┴───────┬───────┴────┬─────────────────┘
        │            │             │             │
┌───────▼────────────▼─────────────▼─────────────▼─────────────────┐
│                       API Module (src/api/)                      │
│  ┌──────────────┐  ┌──────────────┐  ┌─────────┐  ┌──────────┐  │
│  │ http_server  │  │ grpc_server  │  │   ws    │  │ graphql  │  │
│  │     .cpp     │  │    .cpp      │  │handler  │  │  .cpp    │  │
│  └──────┬───────┘  └──────┬───────┘  └────┬────┘  └────┬─────┘  │
│         └─────────────────┴───────────────┴────────────┘         │
│                            │ internal request objects             │
└────────────────────────────┼──────────────────────────────────────┘
                             │
┌────────────────────────────▼──────────────────────────────────────┐
│                    Server Module (src/server/)                    │
│                  40+ API handlers / rate limiter                  │
└───────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 HTTP REST Request

```
Client HTTP request
    │
    ▼
http_server.cpp: parse headers, path, body
    │
    ▼
Auth middleware (delegated to src/auth/)
    │
    ▼
Route to src/server/<endpoint>_handler.cpp
    │
    ▼
Response serialized to JSON → HTTP response
```

### 4.2 gRPC Request

```
Client gRPC call
    │
    ▼
grpc_server.cpp: Protobuf deserialization
    │
    ▼
themisdb_grpc_service.cpp: map to internal request
    │
    ▼
src/server/ handler
    │
    ▼
Protobuf response → gRPC reply
```

### 4.3 WebSocket / Streaming

```
HTTP Upgrade → ws_handler.cpp
    │
    ▼
Subscribe to CDC changefeed or LLM token stream
    │
    ▼
Async push frames to client (fan-out via src/cdc/)
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Delegates to** | `src/server/` | All business logic handlers |
| **Uses** | `src/auth/` | Auth middleware before routing |
| **Uses** | `src/network/` | Low-level socket management |
| **Uses** | `src/cdc/` | Changefeed subscriptions over WebSocket |
| **Uses** | `src/observability/` | Request metrics and tracing |
| **Provides to** | external clients | HTTP/gRPC/WS/GraphQL endpoints |

---

## 6. Threading & Concurrency Model

- HTTP server uses an async I/O event loop (Boost.Asio or equivalent).
- gRPC server runs with a completion queue thread pool (default: `hardware_concurrency`).
- WebSocket connections each have a lightweight coroutine/strand for multiplexing.
- GraphQL subscriptions use a dedicated publish-subscribe event bus.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| HTTP/2 multiplexing | Single TCP connection handles multiple concurrent streams |
| gRPC streaming | Bidirectional streaming for large result sets |
| Connection pooling | Reuse connections for high-frequency clients |
| GraphQL DataLoader pattern | Batch and cache resolver calls to avoid N+1 queries |

---

## 8. Security Considerations

- All endpoints enforce auth middleware before reaching handlers.
- TLS 1.3 is required for all non-localhost connections (configured in `src/network/`).
- GraphQL depth and complexity limits prevent DoS via deeply nested queries.
- WebSocket origin validation prevents cross-site WebSocket hijacking.
- gRPC mTLS support for internal service-to-service communication.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `api.http.port` | 8080 | HTTP server port |
| `api.grpc.port` | 9090 | gRPC server port |
| `api.grpc.thread_pool_size` | CPU count | gRPC completion queue threads |
| `api.graphql.max_depth` | 10 | Max GraphQL query depth |
| `api.graphql.max_complexity` | 1000 | Max GraphQL query complexity score |
| `api.ws.max_connections` | 10000 | Max concurrent WebSocket connections |

---

## 10. Error Handling

| Error Type | HTTP Code | gRPC Code | Strategy |
|---|---|---|---|
| Auth failure | 401 / 403 | UNAUTHENTICATED / PERMISSION_DENIED | Reject at middleware |
| Malformed request | 400 | INVALID_ARGUMENT | Return structured error |
| Rate limited | 429 | RESOURCE_EXHAUSTED | Return Retry-After header |
| Internal error | 500 | INTERNAL | Log + return opaque error ID |
| Timeout | 504 | DEADLINE_EXCEEDED | Cancel downstream work |

---

## 11. Known Limitations & Future Work

- HTTP/3 (QUIC) support is planned but not yet implemented.
- GraphQL subscription scaling across multiple server instances requires a shared
  pub-sub broker (Redis/Kafka integration planned).
- gRPC reflection service for tooling (grpcurl) is not yet enabled.

---

## 12. References

- `src/api/README.md` — module overview
- `src/server/README.md` — handler layer documentation
- `docs/wire-protocol.md` — wire protocol specification
- `openapi/` — OpenAPI specification for REST endpoints
- `ARCHITECTURE.md` (root) — full system architecture
