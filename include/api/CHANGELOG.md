> ⚠️ **Header Changelog** — entries describe the public API contract changes only. For full implementation changelog see `../../src/api/CHANGELOG.md`.

<!-- Status: current | validated: 2026-05-11 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/api/CHANGELOG.md -->

# Changelog — API Module Public Headers

All notable changes to the **public header interface** (`include/api/`) are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

For full implementation-level changes (source files, tests, build system) see:

→ [`../../src/api/CHANGELOG.md`](../../src/api/CHANGELOG.md)

---

## [Unreleased]

### In Progress
- Fragment and directive support in `graphql.h::Parser` — Issue #1491
- `GrpcBridgeImpl` concrete implementation for `IGRPCBridge` — `FUTURE_ENHANCEMENTS.md`
- `QueryAllowList` production startup warning — `FUTURE_ENHANCEMENTS.md`

---

## [2.0.0]

### Added
- `graphql.h`: `Value::Type::VariableRef` enum value — represents a `$variable` reference node in a GraphQL argument value
- `graphql.h`: `Value::variableRef(name)` static factory method
- `graphql.h`: `Value::isVariableRef()` / `Value::asVariableRef()` accessors
- `graphql.h`: `Executor::resolveValue()` private helper (resolves `VariableRef` → bound value or null)
- `graphql.h`: `Executor::executeOperation()` now merges operation default values into `ExecutionContext::variables`

### Migration Notes
- Any exhaustive `switch` on `Value::Type` must add a `VariableRef` case; the compiler will warn on unhandled enum values.

---

## [1.9.1]

### Added
- `aql_utils.h` (new header): `themis::api::aqlEscapeLiteral()` and `themis::api::isValidAqlIdentifier()` — header-only helpers for AQL string escaping and identifier validation; usable by both production code and unit tests

### Security
- `aql_utils.h`: `isValidAqlIdentifier()` enforces `^[a-zA-Z_][a-zA-Z0-9_]*$` pattern; prevents AQL identifier injection via `HybridSearch`/`FullTextSearch` collection names

---

## [1.9.0]

### Added
- `themisdb_grpc_service_factory.h` (new header): `ThemisDBServiceFactory` fluent builder — `withDb()`, `withTxnMgr()`, `withQueryEngine()`, `withVectorIndex()`, `build()`
- `grpc_server.h`: `GrpcApiServer::start()` now releases `mutex_` before the blocking `BuildAndStart()` socket-bind call
- `grpc_server.h`: `GrpcApiServer::stop()` applies a 30-second hard deadline to `server_->Shutdown()`

---

## [1.8.0]

### Added
- `graphql_ws_handler.h`: `alive_` `std::shared_ptr<std::atomic<bool>>` flag — prevents CDC callback use-after-free
- `graphql_ws_handler.h`: `GraphQLWsHandler::reset()` documented to set `alive_` to `false` with `memory_order_release` before clearing subscriptions
- `graphql_ws_handler.h`: `validateVariables()` private static helper for subscription variable type-validation

---

## [1.7.0]

### Added
- `graphql_ws_handler.h` (new header): `GraphQLWsHandler` — `graphql-transport-ws` protocol handler with `QueryLimits::max_subscriptions` enforcement
- `grpc_server.h` (new header): `GrpcApiServer`, `GrpcServerConfig`
- `themisdb_grpc_service.h` (new header): `ThemisDBService` gRPC handler
- `grpc_bridge.h` (new header): `IGRPCBridge`, `ServiceDescriptor`, `GRPCRequest`, `GRPCMetadata`
- `graphql.h`: `QueryLimits::max_subscriptions` field added

---

## [1.6.0]

### Added
- `graphql.h`: Full public type set — `Value`, `Field`, `SelectionSet`, `Operation`, `Document`, `QueryLimits`, `Parser`, `Executor`, `Schema`
- `graphql_cache.h` (new header): `QueryPlanCache`, `ResponseCache` with LRU eviction
- `graphql_metrics.h` (new header): `Metrics`, `QueryTimer`
- `graphql_schema_builder.h` (new header): `IGraphQLSchemaBuilder`, `GraphQLTypeDescriptor`, `SchemaValidationResult`
- `graphql_aql_resolver.h` (new header): `GraphQLAQLResolver`
- `ws_handler.h` (new header): `WsChangeHandler`
- `subscription_multiplexer.h` (new header): `SubscriptionMultiplexer`
- `tracing_middleware.h` (new header): `TracingMiddleware`
- `correlation_id.h` (new header): `CorrelationId`, `ICorrelationIDProvider`
- `rate_limiter.h` (new header): `RateLimiter`, `OperationRateLimiter`
- `audit_logger.h` (new header): `AuditLogger`, `AuditLogEntry`, `AuditLogBuilder`
- `otlp_exporter.h` (new header): `OtlpExporter`, `OtlpExporterConfig`, `SpanData`
- `persisted_queries.h` (new header): `PersistedQueryRegistry`, `QueryAllowList`, `QueryHasher`
- `geo_index_hooks.h` (new header): `GeoIndexHooks`
- `api_gateway_hook.h` (new header): `IAPIGatewayHook`, `IGatewayHookRegistry`, `GatewayHookPhase`, `GatewayHookContext`, `GatewayHookResult`
- `api_version_router.h` (new header): `IAPIVersionRouter`, `VersionDescriptor`
- `http_handler.h` (new header): `IHttpHandler`, `HttpRequest`, `HttpResponse`, `Result<T>`
- `websocket_handler.h` (new header): `IWebSocketHandler`, `WebSocketSession`, `IWebSocketFrameCallback`
- `federation_admin_handler.h` (new header): `FederationAdminHandler`

---

## [1.0.0]

### Added
- Initial public header set for HTTP REST, authentication middleware, and AQL query endpoints.

---

## References

- Full implementation changelog: [`../../src/api/CHANGELOG.md`](../../src/api/CHANGELOG.md)
- Roadmap: [`ROADMAP.md`](ROADMAP.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
