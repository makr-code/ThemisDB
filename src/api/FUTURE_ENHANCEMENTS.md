# API Module - Future Enhancements

## Scope

This document covers implementation-specific future enhancements for the API module (`src/api/`), which exposes ThemisDB over HTTP using Crow/Beast (`http_server.cpp` is a legacy stub; the live implementation resides in `src/server/http_server.cpp`), GraphQL (`graphql.cpp`, 1,214 lines), and geospatial index hooks (`geo_index_hooks.cpp`). Enhancements to underlying AQL execution, storage, or authentication are out of scope; only the API surface, transport layer, and middleware pipeline are in scope here.

## Design Constraints

- `[ ]` REST endpoint signatures introduced in v1.x must remain backward-compatible; new capabilities are added via versioned prefixes (`/v2/`) or opt-in headers, not breaking changes to existing `/v1/` routes.
- `[ ]` The GraphQL parser in `graphql.cpp` uses `QueryLimits::defaults()` for depth/complexity guards; any new field resolver must enforce those limits to prevent query amplification.
- `[ ]` TLS is mandatory for all production transports; new WebSocket and gRPC transports must share the same TLS context as the existing HTTP listener.
- `[ ]` Auth middleware (`src/auth/`) is a hard dependency; no new transport may bypass JWT/JWKS validation enforced by `jwt_validator.cpp`.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `graphql::Parser::parse()` | GraphQL HTTP handler | `QueryLimits` must be configurable per-tenant |
| `geo_index_hooks` | REST geo query endpoints | Hook registration must be idempotent for hot-reload |
| `auth::JWTValidator` | All HTTP/WS/gRPC handlers | Must propagate tenant ID into request context |
| `cdc::Changefeed` | Planned WebSocket change-stream endpoint | Requires `Changefeed::subscribe()` returning an async event iterator |
| `aql::LLMAQLHandler` | AQL execution endpoint | Streaming result set needed for `/v2/query/stream` |

## Planned Features

### GraphQL Schema Completion and Subscription Support
**Priority:** High
**Target Version:** v1.7.0

`graphql.cpp` implements a full parser and query executor but lacks mutation resolvers, schema introspection (`__schema`, `__type`), and subscription over WebSocket. Complete the schema to cover documents, graph edges, vector search, and geospatial queries; add `subscription` operation support backed by `cdc::Changefeed`.

**Implementation Notes:**
- `[ ]` Add `SchemaRegistry` class to `graphql.cpp`; auto-build from registered `TypeDefinition` objects at server start.
- `[ ]` Implement `__schema` and `__type` introspection resolvers; required by all major GraphQL clients (Apollo, Relay).
- `[ ]` Subscription transport: use Boost.Beast WebSocket upgrades; create `graphql_ws_handler.cpp` implementing the `graphql-transport-ws` protocol (not the legacy `subscriptions-transport-ws`).
- `[ ]` Wire `cdc::Changefeed::subscribe(filter)` as the event source for `subscription { onChange(collection: "...") { ... } }`.
- `[ ]` Enforce `QueryLimits::maxSubscriptions` per connection to prevent fan-out DoS.

**Performance Targets:**
- GraphQL parse + validate + execute for a 10-field document query in < 2 ms (p99) under 500 concurrent HTTP/2 connections.
- Subscription event delivery latency < 50 ms from `Changefeed` event emission to WebSocket frame sent.

**API Sketch:**
```graphql
# New subscription type (graphql.cpp SchemaRegistry)
type Subscription {
  onChange(collection: String!, filter: ChangeFilter): ChangeEvent!
}

type ChangeEvent {
  sequence: Int!
  type: ChangeType!
  key: String!
  document: JSON
  timestampMs: Int!
}
```

---

### WebSocket Real-Time Change Streaming Endpoint
**Priority:** High
**Target Version:** v1.7.0

Add a dedicated WebSocket endpoint `/v2/changes` that multiplexes multiple `cdc::Changefeed` subscriptions over a single connection. This is distinct from the GraphQL subscription path and targets clients that need raw change events without the GraphQL envelope.

**Implementation Notes:**
- `[x]` Create `ws_handler.cpp` (`src/api/ws_handler.cpp`); register route `WS /v2/changes` in `src/server/http_server.cpp`.
- `[ ]` Frame format: newline-delimited JSON, each frame matching `Changefeed::ChangeEvent::toJson()` output.
- `[ ]` Client subscribes/unsubscribes by sending `{"action":"subscribe","collection":"orders","filter":{"type":"PUT"}}` control frames.
- `[x]` Implement per-connection back-pressure: if the outbound frame queue exceeds 1,000 entries, close with `1011 Internal Error` and log tenant/connection ID. (`WebSocketSession::kMaxQueueDepth = 1000`)
- `[x]` Reuse `auth::JWTValidator` middleware already wired for HTTP; extract Bearer token from the WebSocket upgrade `Authorization` header. (`WsChangeHandler::validate()` requires `cdc:read` scope)

**Performance Targets:**
- ≥ 10,000 concurrent WebSocket connections on a single node with < 50 MB additional RSS.
- Frame delivery latency p99 < 30 ms under 5,000 events/sec aggregate throughput.

---

### Versioned API Routing and `/v2/` Prefix
**Priority:** High
**Target Version:** v1.8.0

Current REST routes use unversioned paths (e.g., `/documents/{id}`). Introduce a `/v1/` prefix retroactively (with redirect from unversioned) and implement `/v2/` routes that support bulk operations, streaming query results, and async job tracking.

**Implementation Notes:**
- `[ ]` Add `RouteVersionRouter` middleware in `src/server/http_server.cpp`; intercept requests at path prefix, rewrite to versioned handler.
- `[ ]` `/v1/` routes: exact current behaviour; unversioned paths redirect 301 to `/v1/`.
- `[ ]` `/v2/documents` — bulk insert endpoint accepting `application/x-ndjson` body (newline-delimited JSON documents, up to 10,000 per request).
- `[ ]` `/v2/query/stream` — SSE endpoint returning result rows as they are produced by the AQL executor; wire to `aql::LLMAQLHandler` streaming API.
- `[ ]` `/v2/jobs/{id}` — async job status for long-running queries; store job state in `cache::AdaptiveQueryCache` with TTL = 1 hour.

**Performance Targets:**
- Bulk insert of 10,000 256-byte documents in < 500 ms end-to-end (network excluded).
- SSE streaming first-byte latency < 5 ms after query planning completes.

---

### gRPC API Surface
**Priority:** Medium
**Target Version:** v2.0.0

Add a gRPC service alongside REST, sharing the same business logic. Define a `ThemisDB` protobuf service in `proto/themisdb.proto` covering document CRUD, AQL execution, and vector search. This is a major version addition and must not affect REST.

**Implementation Notes:**
- `[x]` Create `src/api/grpc_server.cpp`; gRPC C++ server using `grpc::ServerBuilder` (synchronous dispatch model, consistent with the rest of the codebase).
- `[x]` Reuse existing service-layer infrastructure via `GrpcApiServer::registerService()`; no business logic duplication — service implementations are registered externally.
- `[ ]` Implement server-side streaming RPC `StreamAQL(AQLQueryRequest) returns (stream AQLRow)` service-layer handler that delegates to `aql::LLMAQLHandler`.
- `[x]` TLS: `grpc::SslServerCredentials` using the same PEM cert/key pair as the Beast HTTP listener; fail-closed on cert load failure.
- `[x]` Expose gRPC reflection service in debug builds only to prevent schema leakage in production.

**Performance Targets:**
- gRPC unary `GetDocument` < 1 ms added latency vs equivalent REST call (same process).
- gRPC streaming `ExecuteQuery` sustains ≥ 100,000 rows/sec on localhost.

---

### Request Tracing and Correlation IDs
**Priority:** Medium
**Target Version:** v1.7.0

All inbound requests must carry or receive a `X-Correlation-ID` header that propagates through the entire call stack (API → AQL → storage → cache) and appears in all log lines and error responses.

**Implementation Notes:**
- `[ ]` Add `TracingMiddleware` in `src/api/tracing_middleware.cpp`; generate UUID v4 if `X-Correlation-ID` absent; inject into thread-local `RequestContext`.
- `[ ]` Forward `RequestContext::correlationId` to `utils/logger.h` log macros via a structured field (`correlation_id`).
- `[ ]` Echo back `X-Correlation-ID` in all responses including errors and SSE streams.
- `[ ]` Export span data to OpenTelemetry collector via OTLP HTTP exporter (configurable endpoint in `config/networking/`).
- `[?]` Decision needed: use W3C `traceparent` header format vs proprietary `X-Correlation-ID` — affects SDK compatibility.

**Performance Targets:**
- Middleware overhead < 10 µs per request (UUID generation + thread-local write).
- Zero correlation ID collision probability for ≥ 1 billion requests (UUID v4 guarantee).

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >80% new code | Test `graphql::Parser` new resolvers with `QueryLimits` boundary cases; mock `Changefeed` for subscription tests |
| Integration | All `/v1/` routes ≥ 95% | `tests/api/rest_integration_test.cpp`; add WebSocket client tests for `/v2/changes` |
| Performance | Regression ≤ 5% on existing endpoints | Benchmark with `wrk` at 500 concurrent connections; alert on p99 regression |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| GraphQL parse+execute (10-field query) | ~5 ms (estimate) | < 2 ms p99 | `tests/api/graphql_bench.cpp` |
| WebSocket concurrent connections | 0 (not implemented) | ≥ 10,000 | Load test with `k6` |
| Bulk insert 10K docs via `/v2/documents` | N/A | < 500 ms | `tests/api/bulk_bench.cpp` |
| Correlation ID middleware overhead | N/A | < 10 µs/req | microbenchmark in `benchmarks/api_bench.cpp` |

## Security / Reliability

- `[ ]` All WebSocket upgrade requests must be validated by `auth::JWTValidator` before the upgrade handshake completes; reject with HTTP 401 before protocol switch.
- `[ ]` GraphQL `__schema` introspection must be disabled via `QueryLimits::allowIntrospection = false` in production deployments; expose a config flag in `config/networking/`.
- `[ ]` Rate limiting middleware (`auth::AuthRateLimiter`) must be applied to `/v2/` routes from first release to prevent bulk-insert abuse; default limit 100 req/s per tenant.
