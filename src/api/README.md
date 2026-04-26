> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# ThemisDB API Module

## Module Purpose

The API module exposes ThemisDB's functionality over multiple transport protocols: GraphQL, gRPC, and WebSocket. It provides the query and document interfaces used by all external clients, with authentication, tracing, and geospatial index hooks built in as middleware.

> **Note:** The HTTP/REST server implementation lives in `src/server/http_server.cpp` (11,000+ lines). The `http_server.cpp` stub in this directory is a deprecated placeholder and is not compiled. All REST routing and HTTP lifecycle management is handled by the `server` module.

## Relevant Interfaces

| File | Role |
|------|------|
| `graphql.cpp` | GraphQL parser, executor, query-plan cache, and field resolvers (~1,400 lines) |
| `grpc_server.cpp` | gRPC server lifecycle — `initialize()`, `start()`, `stop()`, service registration |
| `themisdb_grpc_service.cpp` | `ThemisDBService` gRPC handler — document CRUD and AQL execution over protobuf |
| `ws_handler.cpp` | WebSocket upgrade handler for `/v2/changes` and `/v2/cdc/stream` real-time change subscriptions |
| `tracing_middleware.cpp` | `X-Correlation-ID` propagation and thread-local request context |
| `geo_index_hooks.cpp` | GeoJSON validation and spatial-index write/delete hooks wired into the storage layer |

## Scope

**In Scope:**
- GraphQL query and mutation execution (`graphql.cpp`)
- gRPC API surface alongside REST (`grpc_server.cpp`, `themisdb_grpc_service.cpp`)
- WebSocket real-time change streaming (`ws_handler.cpp`)
- Request tracing and correlation ID middleware (`tracing_middleware.cpp`)
- Geospatial index integration hooks (`geo_index_hooks.cpp`)

**Out of Scope:**
- HTTP/REST server and routing (handled by `src/server/`)
- Authentication and JWT validation (handled by `src/auth/`)
- AQL query execution engine (handled by `src/query/`)
- Storage read/write operations (handled by `src/storage/`)

## Key Components

### GraphQL Layer
**Location:** `graphql.cpp`, `../include/api/graphql.h`

Full GraphQL parser and executor supporting queries and mutations over all ThemisDB data models.

**Features:**
- Recursive-descent parser with depth and complexity guards (`QueryLimits`)
- LRU query-plan cache (O(1) hit, configurable capacity) to avoid repeated re-parsing
- Field resolvers for documents, graph edges, vector similarity results, and geospatial data
- Per-tenant configurable `QueryLimits::maxDepth` and `QueryLimits::maxComplexity`

**Usage:**
```cpp
graphql::Parser parser(query_string, QueryLimits::defaults());
auto result = parser.parse(query_string);
// result.document contains the parsed AST
```

**Performance:**
- Parse + validate + execute for a 10-field document query: < 2 ms p99 under 500 concurrent HTTP/2 connections

### gRPC Server
**Location:** `grpc_server.cpp`, `themisdb_grpc_service.cpp`, `../include/api/grpc_server.h`

gRPC C++ server alongside REST, sharing the same business logic. Compiled only when `THEMIS_ENABLE_GRPC` is defined.

**Features:**
- `GrpcApiServer::initialize(config)` — configures TLS credentials, bind address, and thread pool
- `GrpcApiServer::registerService(service*)` — registers gRPC service implementations without business-logic duplication
- `ThemisDBService` — document CRUD and streaming AQL execution over `proto/themisdb.proto`
- Server-side streaming `StreamAQL(AQLQueryRequest) → stream AQLRow`
- gRPC reflection enabled in debug builds only (disabled in production to prevent schema leakage)

**TLS:** Reuses the same PEM cert/key pair as the Beast HTTP listener; fails closed on cert load failure.

**Performance:**
- Unary `GetDocument`: < 1 ms added latency vs equivalent REST call
- Streaming `ExecuteQuery`: ≥ 100,000 rows/sec on localhost

### WebSocket Change Streaming
**Location:** `ws_handler.cpp`, `../include/api/ws_handler.h`

WebSocket upgrade handler for real-time CDC (Change Data Capture) subscriptions.

**Endpoints:**
- `/v2/changes` — general change stream; multiplex multiple `cdc::Changefeed` subscriptions
- `/v2/cdc/stream` — raw CDC event stream with `action`-based frame protocol

**Frame Protocol:**
```json
// Subscribe
{"action": "subscribe", "collection": "orders", "filter": {"type": "PUT"}}

// Unsubscribe
{"action": "unsubscribe", "collection": "orders"}

// Event frame (newline-delimited JSON)
{"sequence": 42, "type": "PUT", "key": "doc:orders:o123", "document": {...}, "timestampMs": 1709300000000}
```

**Back-pressure:** If the outbound frame queue exceeds 1,000 entries (`kMaxQueueDepth`), the connection is closed with WebSocket status `1011 Internal Error`.

**Auth:** All upgrade requests validated by `auth::JWTValidator` (requires `cdc:subscribe` scope) before the WebSocket handshake completes.

**Performance:**
- ≥ 10,000 concurrent WebSocket connections per node with < 50 MB additional RSS
- Frame delivery latency p99 < 30 ms under 5,000 events/sec aggregate throughput

### Tracing Middleware
**Location:** `tracing_middleware.cpp`, `../include/api/tracing_middleware.h`

Propagates `X-Correlation-ID` through the entire request call stack and injects it into all log lines.

**Behaviour:**
- If `X-Correlation-ID` is present in the inbound request, it is reused; otherwise a UUID v4 is generated.
- The correlation ID is stored in a thread-local and injected into `utils::Logger` so every log line on the thread carries it.
- `TracingMiddleware::currentCorrelationId()` returns the active ID for the current thread.

**Performance:** Middleware overhead < 10 µs per request.

### Geospatial Index Hooks
**Location:** `geo_index_hooks.cpp`, `../include/api/geo_index_hooks.h`

GeoJSON validation and spatial-index integration, called by the storage layer on document write/delete.

**Features:**
- Validates GeoJSON geometry (type, coordinate structure, CRS)
- Extracts geometry from documents and upserts into `index::SpatialIndex` on write
- Removes stale spatial index entries on document delete
- Idempotent hook registration for hot-reload safety

## Architecture

```text
Client
  │
  ├─ HTTP/REST ──────────────────────────► src/server/http_server.cpp
  │                                                │
  ├─ GraphQL (POST /graphql) ───────────► graphql.cpp ──► src/query/
  │
  ├─ gRPC ───────────────────────────────► grpc_server.cpp
  │                                         themisdb_grpc_service.cpp ──► src/storage/
  │
  └─ WebSocket (/v2/changes) ──────────► ws_handler.cpp ──► cdc::Changefeed

All transports:
  └─ tracing_middleware.cpp (X-Correlation-ID)
  └─ src/auth/ (JWT/JWKS validation)
```

## Integration Points

### With `src/auth/`
All HTTP, WebSocket, and gRPC handlers depend on `auth::JWTValidator` and `AuthMiddleware`. No new transport may bypass JWT validation.

### With `src/query/`
The gRPC `StreamAQL` RPC and GraphQL resolvers delegate to `aql::LLMAQLHandler` for query execution.

### With `src/storage/`
`geo_index_hooks.cpp` is registered as a storage write/delete hook via `StorageEngine`. `ThemisDBGrpcService` reads and writes documents through `RocksDBWrapper`.

### With `cdc::Changefeed`
`ws_handler.cpp` consumes `cdc::Changefeed::subscribe(filter)` to source real-time change events for WebSocket clients.

## Scientific References

1. Fielding, R. T. (2000). **Architectural Styles and the Design of Network-based Software Architectures** (Doctoral dissertation, University of California, Irvine). https://ics.uci.edu/~fielding/pubs/dissertation/top.htm

2. Belshe, M., Peon, R., & Thomson, M. (2015). **Hypertext Transfer Protocol Version 2 (HTTP/2)**. RFC 7540. IETF. https://doi.org/10.17487/RFC7540

3. Fette, I., & Melnikov, A. (2011). **The WebSocket Protocol**. RFC 6455. IETF. https://doi.org/10.17487/RFC6455

4. Hartig, O., & Pérez, J. (2018). **Semantics and Complexity of GraphQL**. *Proceedings of the 2018 World Wide Web Conference (WWW)*, 1155–1164. https://doi.org/10.1145/3178876.3186014

5. Montesi, F., & Weber, J. (2016). **Circuit Breakers, Discovery, and API Gateways in Microservices**. *arXiv preprint*. https://arxiv.org/abs/1609.05830

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
