<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — API Module

All notable changes to the API module are documented here.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
- Versioned API endpoint routing deprecation-header polish — Issue #1497
- API key management endpoint — Issue #1502

## [1.9.1] — 2026-04-07
### Fixed
- **Security (AQL identifier injection)**: `HybridSearch` and `FullTextSearch` now validate the `collection` field
  against `^[a-zA-Z_][a-zA-Z0-9_]*$` via the new `themis::api::isValidAqlIdentifier()` helper
  (`include/api/aql_utils.h`) before embedding it as a bare AQL identifier in `FOR doc IN <name>`.
  Previously `aqlEscape()` escaped single-quotes and backslashes but did **not** protect against
  injection through the identifier itself (e.g. `"col RETURN SLEEP(10) //"`).
- **`BatchWrite` partial-failure reporting**: `BatchWriteResponse::success` is now `false` when at
  least one upsert or delete fails. The response still reports `upserted_count` / `deleted_count`
  for the operations that succeeded; a new `error.code=207` / `error.message` details the fraction
  that completed.
- **Document version counter**: `CreateDocument` persists the initial version (`1`) to a
  `__ver/<collection>/<key>` meta-key in RocksDB. `UpdateDocument` reads and increments that counter
  atomically and returns the new value via `CreateDocumentResponse::version` /
  `UpdateDocumentResponse::version`. Previously both RPCs always returned `version=1` regardless
  of prior writes.
- **`HybridSearch` sparse hits missing from response**: the AQL full-text result is now parsed
  (nlohmann/json) and each matching document is added to `HybridSearchResponse::hits` with
  `score = 1 − alpha`.  Previously the AQL was executed but the result was discarded.

### Added
- `include/api/aql_utils.h`: header-only helpers `themis::api::aqlEscapeLiteral()` and
  `themis::api::isValidAqlIdentifier()`, usable by both production code and unit tests.
- 9 new tests in `test_themisdb_grpc_service.cpp` covering identifier validation, literal escaping,
  and edge cases for the injection fix.

## [1.9.0] — 2026-03-25
### Fixed
- `GrpcApiServer::start()` no longer holds `mutex_` across the blocking `BuildAndStart()` socket-bind call; lock is released before the network operation and re-acquired afterwards, preventing deadlock when `stop()` or any accessor is called concurrently (Phase 4)
- `GrpcApiServer::stop()` now applies a 30-second hard deadline to `server_->Shutdown()`, preventing indefinite blocks when in-flight RPC handlers stall (Phase 4)

### Added
- `ThemisDBGrpcService` extended constructor accepting `IQueryEngine` + `IVectorIndex`: when wired in, `ExecuteAQL`/`StreamAQL` delegate to the AQL engine; `VectorSearch`/`FilteredVectorSearch` delegate to the vector index; `HybridSearch` combines both; `FullTextSearch` constructs an AQL `FULLTEXT()` query (Phase 4)
- `ThemisDBGrpcServiceFactory` (header-only fluent builder in `include/api/themisdb_grpc_service_factory.h`): `withDb()`, `withTxnMgr()`, `withQueryEngine()`, `withVectorIndex()`, `build()` — the recommended assembly point for a fully-wired `ThemisDBGrpcService`
- `StreamAQL` RPC now streams each JSON array element as a separate `AQLRow` message (cancellation-aware via `ctx->IsCancelled()`)
- `HealthCheck` details now report `aql`/`vector` status as `"ok"` or `"not wired"` rather than a static string
- 8 new tests in `test_themisdb_grpc_service.cpp` covering extended constructor, factory build variants, and factory reuse


### Added
- GraphQL WebSocket CDC callback use-after-free protection: `alive_` `std::shared_ptr<std::atomic<bool>>` flag added to `GraphQLWsHandler`; set to `false` (with `memory_order_release`) in `reset()` before subscriptions are cleared; CDC lambda captures `alive` by value and loads with `memory_order_acquire` before dereferencing `self` (`graphql_ws_handler.cpp`, `graphql_ws_handler.h`)
- GraphQL subscription variable type-validation in `handleSubscribe()` step 2: new `validateVariables(const graphql::Operation&, const nlohmann::json&)` private helper validates required/non-null presence, null-value legality, list-vs-scalar shape, and built-in scalar type matching (String, ID, Int, Float, Boolean) before subscription registration (`graphql_ws_handler.cpp`)
- Versioned API routing via `RouteVersionRouter` (`include/server/route_version_router.h`): unversioned paths redirect 301 to `/v1/`; all new endpoints under `/v2/`; wired in `src/server/http_server.cpp` (Issue: #1497)

### Fixed
- `GrpcApiServer::start()` and `stop()` mutex notes documented as open issues (Target: v2.1.0)

## [1.7.0] — 2026-03-09
### Added
- GraphQL over WebSocket subscription transport (`graphql-transport-ws` protocol) — `api/graphql_ws_handler.cpp`
- `QueryLimits::max_subscriptions` per-connection cap to prevent GraphQL subscription fan-out DoS
- gRPC surface with proto definitions mirroring REST API (`api/grpc_server.cpp`, `proto/themisdb.proto`)
- Async job API for long-running AQL queries with polling endpoint (Issue #1504)
- Multi-tenant namespace routing for all endpoints (Issue #1503)
- Client SDK generation from OpenAPI spec (Python, JavaScript, Go) (Issue #1501)
- API gateway integration (Kong, Nginx) configuration support (Issue #1500)

### Changed
- OpenTelemetry tracing middleware (`tracing_middleware.cpp`) now propagates `X-Correlation-ID` through all log lines

## [1.6.0] — 2026-02-01
### Added
- GraphQL API layer with full schema for multi-model queries (`api/graphql.cpp`) (Issue #1447)
- Streaming query result endpoints via SSE and WebSocket (`api/ws_handler.cpp`) (Issue #1492)
- WebSocket support for real-time change subscriptions (Issue #1494)
- Rate limiting middleware with configurable per-client token bucket (Issue #1495)
- Request tracing middleware with `X-Correlation-ID` propagation via `TracingMiddleware` (Issue #1496)
- Bulk operation endpoints: batch insert, batch delete (Issue #1498)
- OTLP exporter integration for distributed tracing (`api/otlp_exporter.cpp`)
- gRPC service handler (`api/themisdb_grpc_service.cpp`)

### Fixed
- GraphQL schema validation for multi-model queries now correctly handles graph traversal responses

## [1.0.0] — 2024-01-01
### Added
- HTTP server integration (Crow/Beast) with RESTful document CRUD endpoints
- AQL query execution endpoint
- Graph operation endpoints
- Authentication and authorization middleware
- TLS/SSL support with certificate configuration
- Request/response handling pipeline with error serialization
- Geo index hooks for geospatial query routing (`api/geo_index_hooks.cpp`)
