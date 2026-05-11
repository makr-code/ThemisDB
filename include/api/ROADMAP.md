> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-05-11 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/api/ROADMAP.md -->

# API Module — Public Header Roadmap

**Module Path:** `include/api/`
**Canonical implementation roadmap:** [`../../src/api/ROADMAP.md`](../../src/api/ROADMAP.md)

---

## Overview

This document tracks the public API contract stability, planned header additions, and header-level breaking changes for `include/api/`. For feature roadmap items that affect both implementation and headers, see the canonical roadmap:

→ [`../../src/api/ROADMAP.md`](../../src/api/ROADMAP.md)

---

## Current Status

All production-required public headers are present and `#pragma once` guarded. gRPC headers (`grpc_server.h`, `themisdb_grpc_service.h`) require `THEMIS_ENABLE_GRPC`. WebSocket headers require `THEMIS_ENABLE_WEBSOCKET`. OTLP headers require `THEMIS_ENABLE_OTEL`.

The header API surface is **stable** for all types introduced in v1.x. Breaking changes (virtual table reordering, removed members) are tracked below.

---

## Completed ✅

- [x] `graphql.h` — `Value`, `Field`, `SelectionSet`, `Parser`, `Executor`, `QueryLimits`, `Schema`
- [x] `graphql_cache.h` — `QueryPlanCache`, `ResponseCache` with LRU eviction and pattern-based invalidation
- [x] `graphql_metrics.h` — `Metrics`, `QueryTimer` RAII guard
- [x] `graphql_schema_builder.h` — `IGraphQLSchemaBuilder`, `GraphQLTypeDescriptor`, `SchemaValidationResult`
- [x] `graphql_aql_resolver.h` — `GraphQLAQLResolver` mapping GraphQL selections to AQL
- [x] `graphql_ws_handler.h` — `GraphQLWsHandler` with `alive_` use-after-free protection and `validateVariables()`
- [x] `grpc_server.h` — `GrpcApiServer`, `GrpcServerConfig` (THEMIS_ENABLE_GRPC)
- [x] `grpc_bridge.h` — `IGRPCBridge`, `ServiceDescriptor`, `GRPCRequest`, `GRPCMetadata`
- [x] `themisdb_grpc_service.h` — `ThemisDBService` gRPC handler (THEMIS_ENABLE_GRPC)
- [x] `themisdb_grpc_service_factory.h` — `ThemisDBServiceFactory` fluent builder
- [x] `http_handler.h` — `IHttpHandler`, `HttpRequest`, `HttpResponse`, `Result<T>`
- [x] `websocket_handler.h` — `IWebSocketHandler`, `WebSocketSession`, `IWebSocketFrameCallback`
- [x] `ws_handler.h` — `WsChangeHandler` (THEMIS_ENABLE_WEBSOCKET)
- [x] `subscription_multiplexer.h` — `SubscriptionMultiplexer` fan-out for live queries
- [x] `tracing_middleware.h` — `TracingMiddleware`, OTLP integration
- [x] `correlation_id.h` — `CorrelationId`, `ICorrelationIDProvider`
- [x] `rate_limiter.h` — `RateLimiter`, `OperationRateLimiter` with `std::shared_mutex` and stale-bucket eviction
- [x] `audit_logger.h` — `AuditLogger`, `AuditLogEntry`, `AuditLogBuilder`, `FileAuditLogHandler`; non-blocking handler dispatch
- [x] `otlp_exporter.h` — `OtlpExporter`, `OtlpExporterConfig`, `SpanData`; persistent `CURL*` handle; `std::deque` queue
- [x] `persisted_queries.h` — `PersistedQueryRegistry`, `QueryAllowList`, `QueryHasher`
- [x] `geo_index_hooks.h` — `GeoIndexHooks` storage write/delete hooks
- [x] `api_gateway_hook.h` — `IAPIGatewayHook`, `IGatewayHookRegistry`, `GatewayHookPhase`, `GatewayHookContext`
- [x] `api_version_router.h` — `IAPIVersionRouter`, `VersionDescriptor`
- [x] `aql_utils.h` — `aqlEscapeLiteral()`, `isValidAqlIdentifier()` helpers (v1.9.1)
- [x] `federation_admin_handler.h` — `FederationAdminHandler`
- [x] `Value::Type::VariableRef` added; `Value::variableRef()`, `isVariableRef()`, `asVariableRef()` — v2.0.0

---

## In Progress 🚧

- [I] OpenAPI 3.x spec completeness for all header-exposed endpoints (Issue: #1491)

---

## Planned Features 📋

### Short-term (Next 3–6 months)

- [ ] `GrpcBridgeImpl` — concrete implementation of `IGRPCBridge` in `src/api/grpc_bridge.cpp`; header contract is stable (Target: Q3 2026)
- [ ] `QueryAllowList` — startup warning when `enabled_ = false` in a production build (`NDEBUG`); document activation path (Target: Q3 2026)
- [ ] `GrpcServerConfig::max_message_size_bytes` — expose as runtime config key `grpc.max_message_size_mb` (Target: Q3 2026)

### Medium-term (6–12 months)

- [ ] `graphql.h` — `Parser::parseFragmentDefinition()`, `parseInlineFragment()` (currently documented as "Not yet supported") (Target: Q4 2026)
- [ ] `graphql.h` — remove deprecated `Parser::error()` after migrating all call sites to `Result<T>` return types (Target: Q4 2026)
- [ ] `Schema::introspect()` — add `__typename`, `__Field`, `__InputValue`, `__EnumValue`, `__Directive` meta-types per GraphQL June 2018 spec (Target: Q4 2026)

---

## Production Readiness Checklist

- [x] All headers have `#pragma once` guard
- [x] All public factory methods marked `[[nodiscard]]`
- [x] Build conditionals documented in `README.md` and `ARCHITECTURE.md`
- [x] `deprecated` symbols annotated with `[[deprecated("...")]]`
- [P] Header-level unit tests (Issue: #1509)
- [I] OpenAPI spec synchronised with header-defined types (Issue: #1491)

---

## Known Issues & Limitations

- `IAPIGatewayHook::hookId()`, `IGatewayHookRegistry::registerHook()`, `unregisterHook()`, and `getHooks()` are marked `[[deprecated]]` — no external callers confirmed; tracked as `CANDIDATE_FOR_REMOVAL` in `src/ROADMAP.md`.
- `graphql.h::Parser` does not yet support fragments, directives, or inline fragments (v1.x limitation; documented in the header).
- `persisted_queries.h::QueryAllowList` has `enabled_ = false` by default; see `FUTURE_ENHANCEMENTS.md` for the production activation plan.

---

## Breaking Changes

| Version | Header | Change |
|---------|--------|--------|
| v2.0.0 | `graphql.h` | `Value::Type::VariableRef` enum value added; any exhaustive `switch` on `Value::Type` must add a `VariableRef` case |
| v1.9.1 | `aql_utils.h` | New header added; no breaking changes to existing headers |
| v1.9.0 | `themisdb_grpc_service_factory.h` | New header added |

---

## References

- Canonical implementation roadmap: [`../../src/api/ROADMAP.md`](../../src/api/ROADMAP.md)
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Future enhancements: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- Module overview: [`README.md`](README.md)
