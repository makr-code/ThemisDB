> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# API Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-04-06 -->

## Current Status
Core HTTP API server implemented with RESTful endpoints, AQL query execution, authentication, and TLS support. GraphQL WebSocket handler (`graphql-transport-ws` protocol) added with subscription management and `QueryLimits::max_subscriptions` enforcement. Versioned API routing (`/v1/`, `/v2/`), gRPC surface with all RPC stubs wired (`ThemisDBGrpcServiceFactory`), OTLP tracing, geo-index hooks, and rate limiting are all production-ready. GraphQL variable substitution fully implemented: `$variable` references in field arguments are resolved at execution time against `ExecutionContext::variables`; operation default values are merged automatically. Phase 5 complete.

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
- [x] GraphQL over WebSocket subscription transport (`graphql-transport-ws` protocol) — `src/api/graphql_ws_handler.cpp`
- [x] `QueryLimits::max_subscriptions` per-connection cap to prevent fan-out DoS
- [x] Versioned API routing (`/v1/`/`/v2/` prefixes, 301 redirect for unversioned paths) — `include/server/route_version_router.h` (Issue: #1497)
- [x] GraphQL WebSocket CDC callback use-after-free protection (`alive_` atomic flag) — v1.8.0
- [x] GraphQL subscription variable type-validation in `handleSubscribe()` step 2 — v1.8.0
- [x] gRPC factory wiring completed for core RPCs (`ExecuteAQL`, `StreamAQL`) and search paths are dependency/feature-gated via `ThemisDBGrpcServiceFactory` — v1.9.0
- [x] `GrpcApiServer::start()` mutex released before `BuildAndStart()` socket bind — v1.9.0
- [x] `GrpcApiServer::stop()` 30-second `Shutdown()` deadline — v1.9.0
- [x] GraphQL variable substitution: `$variable` in field arguments now resolved at execution time via `Executor::resolveValue()`; `Value::VariableRef` type added; operation default values merged in `executeOperation()` — v2.0.0

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
- [x] Implemented authentication and authorization middleware (`src/server/auth_middleware.cpp`)
- [x] Added TLS/SSL support with certificate configuration
- [x] Built request/response handling pipeline with error serialization

### Phase 2: GraphQL, WebSocket, and API Hardening (Status: Completed)
- [x] Implement GraphQL schema and resolver for multi-model queries (`src/api/graphql.cpp` + `src/server/graphql_api_handler.cpp`) (Issue: #1515)
- [x] Implement WebSocket upgrade handler for real-time change subscriptions (`src/api/ws_handler.cpp`) (Issue: #1516)
- [I] Complete OpenAPI 3.x spec for all existing endpoints (Issue: #1517)
- [x] Add rate limiting middleware with configurable per-client token bucket (Issue: #1518)
- [x] Add request correlation IDs propagated through all log lines — `TracingMiddleware` in `tracing_middleware.cpp` (Issue: #1519)
- [x] Implement GraphQL over WebSocket subscription handler (`src/api/graphql_ws_handler.cpp`) with `graphql-transport-ws` protocol and `QueryLimits::max_subscriptions` enforcement

### Phase 3: gRPC, Versioning, and SDK Generation (Status: In Progress 🚧)
- [x] Implement gRPC surface with proto definitions mirroring REST API (`src/api/grpc_server.cpp`, `proto/themisdb.proto`) (Issue: #1505)
- [x] Add versioned endpoint routing (`/v1/`, `/v2/` prefixes) with deprecation headers via `RouteVersionRouter` (`include/server/route_version_router.h`) (Issue: #1506)
- [x] Generate client SDKs from OpenAPI spec for Python, JavaScript, and Go (Issue: #1507)
- [x] Implement async job API for long-running AQL queries with polling endpoint (Issue: #1508)

### Phase 4: API Hardening and gRPC Stub Wiring (Status: Completed ✅)
- [x] GraphQL WebSocket CDC callback use-after-free prevention (`alive_` atomic flag, `reset()` ordered release) — v1.8.0
- [x] GraphQL subscription variable type-validation (required/non-null, list shape, scalar type matching) — v1.8.0
- [x] Wire gRPC core RPCs (`ExecuteAQL`, `StreamAQL`) via `ThemisDBGrpcServiceFactory`; advanced search RPCs remain feature-gated where optional engines are unavailable — v1.9.0
- [x] Fix `GrpcApiServer::start()` holding `mutex_` across `BuildAndStart()` blocking socket bind — v1.9.0
- [x] Fix `GrpcApiServer::stop()` indefinite block — 30-second `Shutdown()` deadline added — v1.9.0
- [I] Complete OpenAPI 3.x specification for all existing endpoints (Issue: #1491)

### Phase 5: GraphQL Variable Substitution (Status: Completed ✅)
- [x] Add `Value::Type::VariableRef` enum value, `Value::variableRef()` factory, `isVariableRef()` / `asVariableRef()` accessors — `include/api/graphql.h` — v2.0.0
- [x] Fix `parseValue()`: `$name` stores `Value::variableRef("name")` instead of `Value::string("$name")` — `src/api/graphql.cpp` — v2.0.0
- [x] `Executor::executeOperation()` merges operation default values into `ExecutionContext::variables` (runtime values take precedence) — v2.0.0
- [x] `Executor::resolveValue()` private helper: resolves `VariableRef` → bound value or `null` — v2.0.0
- [x] `Executor::executeField()` resolves all `VariableRef` arguments before invoking resolver — v2.0.0
- [x] 5 new execution tests (string/int substitution, default value, runtime override, unbound → null) — `tests/test_graphql_variables.cpp`

## Production Readiness Checklist
- [P] Unit tests coverage > 80% (Issue: #1509)
- [P] Integration tests (Issue: #1510)
- [P] Performance benchmarks (Issue: #1511)
- [x] Security audit (Issue: #1512)
- [x] Documentation complete (validated: 2026-04-06)
- [I] API stability guaranteed (Issue: #1514)

## Known Issues & Limitations
- OpenAPI specification may be incomplete for newer endpoints
- GraphQL `Parser` explicitly rejects fragments, directives, and inline fragments in v1.x with version-gated error messages (`graphql.cpp`); support planned for v2.0
- `WsChangeHandler::validate()` does not URL-decode query-string parameters (`from_sequence`, `key_prefix`), so percent-encoded values are silently misinterpreted

## Breaking Changes
- GraphQL schema will be introduced as a new endpoint (non-breaking to REST)
- gRPC surface planned for a future major version
