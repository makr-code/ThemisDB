> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-05-11 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/api/ARCHITECTURE.md -->

# API Module — Public Header Architecture

**Module Path:** `include/api/`
**Implementation:** `../../src/api/`
**Canonical architecture doc:** [`../../src/api/ARCHITECTURE.md`](../../src/api/ARCHITECTURE.md)

---

## 1. Overview

The `include/api/` directory contains the **public C++ header contract** for ThemisDB's multi-protocol API layer. Headers define types, interfaces, and configuration structures that are consumed both by internal implementation files and by embedders that integrate ThemisDB as a library.

All headers are `#pragma once` guarded and contain no implementation code (inline bodies in template/header-only helpers are documented per file).

For full architectural details — data flow diagrams, threading model, integration point map — see the canonical document:

→ [`../../src/api/ARCHITECTURE.md`](../../src/api/ARCHITECTURE.md)

---

## 2. Namespace Layout

All public types live under `themis::` or one of its sub-namespaces:

| Namespace | Headers | Purpose |
|-----------|---------|---------|
| `themis::graphql` | `graphql.h`, `graphql_cache.h`, `graphql_metrics.h`, `graphql_schema_builder.h`, `graphql_aql_resolver.h`, `graphql_ws_handler.h` | GraphQL parser, executor, cache, metrics, schema builder, WebSocket handler |
| `themis::api` | `grpc_server.h`, `grpc_bridge.h`, `themisdb_grpc_service.h`, `themisdb_grpc_service_factory.h`, `http_handler.h`, `websocket_handler.h`, `ws_handler.h`, `subscription_multiplexer.h`, `tracing_middleware.h`, `correlation_id.h`, `rate_limiter.h`, `audit_logger.h`, `otlp_exporter.h`, `persisted_queries.h`, `geo_index_hooks.h`, `api_gateway_hook.h`, `api_version_router.h`, `aql_utils.h`, `federation_admin_handler.h` | Transport adapters, middleware, rate limiting, audit, OTLP, geo hooks, gateway hooks, routing |

---

## 3. Public Type Hierarchy

### 3.1 GraphQL Subsystem (`themis::graphql`)

```
graphql::Value                    — tagged union: null / bool / int / float / string / list / object / VariableRef
graphql::Field                    — name + arguments (Value map) + selection set
graphql::SelectionSet             — ordered list of Field
graphql::Operation                — type (query/mutation/subscription) + name + variable definitions + selection
graphql::Document                 — list of Operation
graphql::QueryLimits              — maxDepth / maxComplexity / maxSubscriptions / allowIntrospection
graphql::Parser                   — parse(query, limits) → Document; token-based recursive descent
graphql::Executor                 — execute(Document, variables, resolver) → JSON result
graphql::Schema                   — type registry + introspect()
graphql::QueryPlanCache           — LRU cache: hash(query_string) → Document (graphql_cache.h)
graphql::ResponseCache            — LRU cache: hash(Document+vars) → JSON (graphql_cache.h)
graphql::Metrics / QueryTimer     — Prometheus-style counters + RAII timer (graphql_metrics.h)
graphql::IGraphQLSchemaBuilder    — pure-virtual programmatic schema construction (graphql_schema_builder.h)
graphql::GraphQLAQLResolver       — maps GraphQL selections to AQL queries (graphql_aql_resolver.h)
graphql::GraphQLWsHandler         — graphql-transport-ws protocol handler (graphql_ws_handler.h)
```

### 3.2 Transport and Middleware (`themis::api`)

```
api::GrpcApiServer / GrpcServerConfig     — gRPC server lifecycle (grpc_server.h)
api::IGRPCBridge / ServiceDescriptor      — pure-virtual gRPC service bridge (grpc_bridge.h)
api::ThemisDBService                      — gRPC handler: document CRUD + AQL (themisdb_grpc_service.h)
api::ThemisDBServiceFactory               — fluent builder for ThemisDBService (themisdb_grpc_service_factory.h)
api::IHttpHandler / HttpRequest / HttpResponse — HTTP dispatch interface (http_handler.h)
api::IWebSocketHandler / WebSocketSession — RFC-6455 WebSocket interface (websocket_handler.h)
api::WsChangeHandler                      — CDC WebSocket upgrade + frame dispatcher (ws_handler.h)
api::SubscriptionMultiplexer              — live query fan-out (subscription_multiplexer.h)
api::TracingMiddleware                    — X-Correlation-ID propagation (tracing_middleware.h)
api::CorrelationId / ICorrelationIDProvider — 16-byte UUID correlation IDs (correlation_id.h)
api::RateLimiter / OperationRateLimiter   — token-bucket rate limiting (rate_limiter.h)
api::AuditLogger / AuditLogEntry          — audit event buffer + handler dispatch (audit_logger.h)
api::OtlpExporter / OtlpExporterConfig    — OTLP span export (otlp_exporter.h)
api::PersistedQueryRegistry / QueryAllowList — persisted query store + allow-list (persisted_queries.h)
api::GeoIndexHooks                        — GeoJSON validation + storage hooks (geo_index_hooks.h)
api::IAPIGatewayHook / IGatewayHookRegistry — pluggable request pipeline hooks (api_gateway_hook.h)
api::IAPIVersionRouter / VersionDescriptor — versioned routing interface (api_version_router.h)
api::FederationAdminHandler               — federated node admin API (federation_admin_handler.h)
```

---

## 4. Build Conditionals

Headers that require optional compile-time dependencies are guarded as follows:

| CMake Symbol | Headers Affected | Required Dependency |
|---|---|---|
| `THEMIS_ENABLE_GRPC` | `grpc_server.h`, `themisdb_grpc_service.h`, `themisdb_grpc_service_factory.h` | gRPC C++ library + protobuf |
| `THEMIS_ENABLE_WEBSOCKET` | `ws_handler.h`, `graphql_ws_handler.h`, `websocket_handler.h` | Boost.Beast or equivalent WS library |
| `THEMIS_ENABLE_OTEL` | `otlp_exporter.h` | libcurl; optional Prometheus registry |
| `THEMIS_HAS_PROMETHEUS` | `otlp_exporter.h` (Prometheus counter registration) | prometheus-cpp |

Embedders should check these symbols before including guarded headers, or use the umbrella
`target_compile_definitions` set by `find_package(ThemisDB)`.

---

## 5. Header Dependencies

The dependency graph between `include/api/` headers is intentionally minimal to support
selective inclusion:

```
graphql.h
  └── (no include/api/ dependencies; uses only standard library)

graphql_cache.h
  └── graphql.h

graphql_metrics.h
  └── graphql.h

graphql_schema_builder.h
  └── graphql.h

graphql_aql_resolver.h
  └── graphql.h

graphql_ws_handler.h
  └── graphql.h

tracing_middleware.h
  └── correlation_id.h

grpc_server.h          (THEMIS_ENABLE_GRPC)
  └── (no include/api/ dependencies)

themisdb_grpc_service.h (THEMIS_ENABLE_GRPC)
  └── (no include/api/ dependencies)

themisdb_grpc_service_factory.h (THEMIS_ENABLE_GRPC)
  └── themisdb_grpc_service.h

ws_handler.h           (THEMIS_ENABLE_WEBSOCKET)
  └── (no include/api/ dependencies)
```

---

## 6. Compatibility and Stability Guarantees

- **ABI stability:** Public types in `include/api/` follow semantic versioning. Breaking changes
  (member reordering, virtual table changes) trigger a major version bump.
- **No implementation code:** Headers contain only declarations, `struct` definitions, and
  inline `constexpr`/template helpers. No non-trivial logic is placed in headers.
- **`[[nodiscard]]`:** Factory functions and error-returning methods use `[[nodiscard]]` to prevent
  silently discarded results.

---

## 7. References

- Full architecture: [`../../src/api/ARCHITECTURE.md`](../../src/api/ARCHITECTURE.md)
- Module overview: [`../../src/api/README.md`](../../src/api/README.md)
- Roadmap: [`../../src/api/ROADMAP.md`](../../src/api/ROADMAP.md)
- Future enhancements: [`../../src/api/FUTURE_ENHANCEMENTS.md`](../../src/api/FUTURE_ENHANCEMENTS.md)
- Public header overview: [`README.md`](README.md)
