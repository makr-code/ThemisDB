---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

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

All production-required public headers are present and `#pragma once` guarded. gRPC headers
(`grpc_server.h`, `themisdb_grpc_service.h`, and the service factory) require
`THEMIS_ENABLE_GRPC`; `ws_handler.h` and `graphql_ws_handler.h` require
`THEMIS_ENABLE_WEBSOCKET`. `otlp_exporter.h` includes libcurl directly; optional Prometheus
integration uses `THEMIS_HAS_PROMETHEUS`.

The header API surface is **stable** for all types introduced in v1.x. Breaking changes (virtual table reordering, removed members) are tracked below.

---

## Completed ✅

- [x] `graphql.h` — `Value`, `Field`, `SelectionSet`, `Parser`, `Executor`, `QueryLimits`, `Schema`
- [x] `graphql_cache.h` — `QueryPlanCache`, `ResponseCache` with LRU eviction and pattern-based invalidation
- [x] `graphql_metrics.h` — `Metrics`, `QueryTimer` RAII guard
- [x] `graphql_schema_builder.h` — `IGraphQLSchemaBuilder`, `GraphQLTypeDescriptor`, `SchemaValidationResult`
- [x] `graphql_aql_resolver.h` — `GraphQLAqlResolverFactory` and `GraphQLComplexityEstimator` for AQL-backed GraphQL resolution
- [x] `graphql_ws_handler.h` — `GraphQLWsHandler` with `alive_` use-after-free protection and `validateVariables()`
- [x] `grpc_server.h` — `GrpcApiServer`, `GrpcServerConfig` (THEMIS_ENABLE_GRPC)
- [x] `grpc_bridge.h` — `IGRPCBridge`, `ServiceDescriptor`, `GRPCRequest`, `GRPCMetadata`
- [x] `themisdb_grpc_service.h` — `ThemisDBGrpcService` gRPC handler (`THEMIS_ENABLE_GRPC`)
- [x] `themisdb_grpc_service_factory.h` — `ThemisDBGrpcServiceFactory` fluent builder
- [x] `http_handler.h` — `IHttpHandler`, `HttpRequest`, `HttpResponse`, `Result<T>`
- [x] `websocket_handler.h` — `IWebSocketHandler`, `WebSocketSession`, `IWebSocketFrameCallback`
- [x] `ws_handler.h` — `WsChangeHandler` (THEMIS_ENABLE_WEBSOCKET)
- [x] `subscription_multiplexer.h` — `ISubscriptionMultiplexer` live-query fan-out interface
- [x] `tracing_middleware.h` — `TracingMiddleware`, OTLP integration
- [x] `correlation_id.h` — `CorrelationId`, `ICorrelationIDProvider`
- [x] `rate_limiter.h` — `RateLimiter`, `OperationRateLimiter` with `std::shared_mutex` and stale-bucket eviction
- [x] `graphql_audit_logger.h` — `graphql::AuditLogger`, `AuditLogEntry`, `AuditLogBuilder`, `FileAuditLogHandler`; handler dispatch outside the buffer lock
- [x] `otlp_exporter.h` — `OtlpExporter`, `OtlpExporterConfig`, `SpanData`; persistent `CURL*` handle; `std::deque` queue
- [x] `persisted_queries.h` — `PersistedQueryRegistry`, `QueryAllowList`, `QueryHasher`
- [x] `geo_index_hooks.h` — `GeoIndexHooks` storage write/delete hooks
- [x] `api_gateway_hook.h` — `IAPIGatewayHook`, `IGatewayHookRegistry`, `GatewayHookPhase`, `GatewayHookContext`
- [x] `api_version_router.h` — `IAPIVersionRouter`, `VersionDescriptor`
- [x] `aql_utils.h` — `aqlEscapeLiteral()`, `isValidAqlIdentifier()` helpers (v1.9.1)
- [x] `federation_admin_handler.h` — `FederationAdminHandler`
- [x] `Value::Type::VariableRef` added; `Value::variableRef()`, `isVariableRef()`, `asVariableRef()` — v2.0.0
- [x] `grpc_server.h` — `GrpcServerConfig::max_message_size_bytes` runtime field (100 MiB default), with `grpc.max_message_size_mb` configuration loading

---

## In Progress 🚧

- [I] OpenAPI 3.x spec completeness for all header-exposed endpoints (Issue: #1491)

---

## Planned Features 📋

### Short-term (Next 3–6 months)

- [ ] `GrpcBridgeImpl` — concrete implementation of `IGRPCBridge` in `src/api/grpc_bridge.cpp`; header contract is stable (Target: Q3 2026)
- [?] `QueryAllowList` — define and verify how the `enabled_` flag participates in request enforcement before adding a production warning (see `FUTURE_ENHANCEMENTS.md`)

### Medium-term (6–12 months)

- [ ] `graphql.h` — add explicitly specified inline-fragment semantics; named fragments and `@include`/`@skip` field directives already exist (Target: Q4 2026)
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
- `graphql.h::Parser` supports named fragment definitions/spreads and `@include`/`@skip` field directives; inline fragments and other directives remain unsupported.
- `persisted_queries.h::QueryAllowList` has an `enabled_` flag that defaults to false, but `isAllowed()` checks membership independently. The production enforcement contract remains to be verified.

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
