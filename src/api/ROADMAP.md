# API Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-03-22 -->

## Current Status
Core HTTP API server implemented with RESTful endpoints, AQL query execution, authentication, and TLS support. GraphQL WebSocket handler (`graphql-transport-ws` protocol) added with subscription management and `QueryLimits::max_subscriptions` enforcement. Versioned API routing (`/v1/`, `/v2/`), gRPC surface, OTLP tracing, geo-index hooks, and rate limiting are all production-ready. Outstanding: gRPC RPC stubs (ExecuteAQL, StreamAQL, VectorSearch, HybridSearch, FullTextSearch), OpenAPI 3.x completeness.

## Completed ✅
- [x] HTTP server integration (Crow/Beast)
- [x] RESTful document CRUD endpoints
- [x] AQL query execution endpoint
- [x] Graph operation endpoints
- [x] Authentication and authorization middleware
- [x] TLS/SSL support
- [x] Request/response handling pipeline
- [x] API middleware infrastructure
- [x] GraphQL API layer (Target: Q2 2026) (Issue: #1447)
- [x] Streaming query result endpoints (SSE/WebSocket) (Target: Q3 2026) (Issue: #1492)
- [x] GraphQL schema for multi-model queries (Issue: #1493)
- [x] WebSocket support for real-time change subscriptions (Issue: #1494)
- [x] Rate limiting middleware (Issue: #1495)
- [x] Request tracing and correlation IDs — `TracingMiddleware` with `X-Correlation-ID` propagation (Issue: #1496)
- [x] Bulk operation endpoints (batch insert, batch delete) (Issue: #1498)
- [x] API gateway integration (Kong, Nginx) (Issue: #1500)
- [x] SDK generation from OpenAPI spec (Python, JavaScript, Go) (Issue: #1501)
- [x] Multi-tenant namespace routing (Issue: #1503)
- [x] Async job API for long-running queries (Issue: #1504)
- [x] GraphQL over WebSocket subscription transport (`graphql-transport-ws` protocol) — `api/graphql_ws_handler.cpp`
- [x] `QueryLimits::max_subscriptions` per-connection cap to prevent fan-out DoS
- [x] Versioned API routing (`/v1/`/`/v2/` prefixes, 301 redirect for unversioned paths) — `include/server/route_version_router.h` (Issue: #1497)
- [x] GraphQL WebSocket CDC callback use-after-free protection (`alive_` atomic flag) — v1.8.0
- [x] GraphQL subscription variable type-validation in `handleSubscribe()` step 2 — v1.8.0

## In Progress 🚧
- [I] OpenAPI 3.x specification completeness (Target: Q2 2026) (Issue: #1491)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] API key management endpoint (Issue: #1502)

## Implementation Phases

### Phase 1: Core HTTP API (Status: Completed)
- [x] Integrated Crow/Beast HTTP server with request routing
- [x] Implemented RESTful CRUD endpoints for documents, graphs, and collections
- [x] Implemented AQL query execution endpoint (handled in `src/server/http_server.cpp`)
- [x] Implemented authentication and authorization middleware (`api/auth_middleware.cpp`)
- [x] Added TLS/SSL support with certificate configuration
- [x] Built request/response handling pipeline with error serialization

### Phase 2: GraphQL, WebSocket, and API Hardening (Status: Completed)
- [x] Implement GraphQL schema and resolver for multi-model queries (`api/graphql.cpp` + `server/graphql_api_handler.cpp`) (Issue: #1515)
- [x] Implement WebSocket upgrade handler for real-time change subscriptions (`api/ws_handler.cpp`) (Issue: #1516)
- [I] Complete OpenAPI 3.x spec for all existing endpoints (Issue: #1517)
- [x] Add rate limiting middleware with configurable per-client token bucket (Issue: #1518)
- [x] Add request correlation IDs propagated through all log lines — `TracingMiddleware` in `tracing_middleware.cpp` (Issue: #1519)
- [x] Implement GraphQL over WebSocket subscription handler (`api/graphql_ws_handler.cpp`) with `graphql-transport-ws` protocol and `QueryLimits::max_subscriptions` enforcement

### Phase 3: gRPC, Versioning, and SDK Generation (Status: In Progress 🚧)
- [x] Implement gRPC surface with proto definitions mirroring REST API (`api/grpc_server.cpp`, `proto/themisdb.proto`) (Issue: #1505)
- [x] Add versioned endpoint routing (`/v1/`, `/v2/` prefixes) with deprecation headers via `RouteVersionRouter` (`include/server/route_version_router.h`) (Issue: #1506)
- [x] Generate client SDKs from OpenAPI spec for Python, JavaScript, and Go (Issue: #1507)
- [x] Implement async job API for long-running AQL queries with polling endpoint (Issue: #1508)

### Phase 4: API Hardening and gRPC Stub Wiring (Status: In Progress 🚧)
- [x] GraphQL WebSocket CDC callback use-after-free prevention (`alive_` atomic flag, `reset()` ordered release) — v1.8.0
- [x] GraphQL subscription variable type-validation (required/non-null, list shape, scalar type matching) — v1.8.0
- [~] Wire gRPC RPC stubs: `ExecuteAQL`, `StreamAQL`, `VectorSearch`, `FilteredVectorSearch`, `HybridSearch`, `FullTextSearch` via `ThemisDBGrpcServiceFactory` injection (Issue: pending)
- [~] Fix `GrpcApiServer::start()` holding `mutex_` across `BuildAndStart()` blocking socket bind (Issue: pending)
- [~] Fix `GrpcApiServer::stop()` indefinite block — add 30-second `Shutdown()` deadline (Issue: pending)
- [I] Complete OpenAPI 3.x specification for all existing endpoints (Issue: #1491)

## Production Readiness Checklist
- [P] Unit tests coverage > 80% (Issue: #1509)
- [P] Integration tests (Issue: #1510)
- [P] Performance benchmarks (Issue: #1511)
- [x] Security audit (Issue: #1512)
- [x] Documentation complete (validated: 2026-03-22)
- [I] API stability guaranteed (Issue: #1514)

## Known Issues & Limitations
- OpenAPI specification may be incomplete for newer endpoints
- gRPC RPC handlers (`ExecuteAQL`, `StreamAQL`, `VectorSearch`, `FilteredVectorSearch`, `HybridSearch`, `FullTextSearch`) currently return `UNIMPLEMENTED`; engine injection via `ThemisDBGrpcServiceFactory` is pending
- `GrpcApiServer::start()` holds `mutex_` across `builder.BuildAndStart()` — can block `stop()`/`isRunning()` callers during a slow port bind
- `GrpcApiServer::stop()` calls `server_->Shutdown()` without a deadline — can block indefinitely if in-flight RPCs do not terminate
- GraphQL `Parser` does not yet support fragments, directives, or inline fragments (documented in `graphql.h`)
- `WsChangeHandler::validate()` does not URL-decode query-string parameters (`from_sequence`, `key_prefix`), so percent-encoded values are silently misinterpreted

## Breaking Changes
- GraphQL schema will be introduced as a new endpoint (non-breaking to REST)
- gRPC surface planned for a future major version
