<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · AUDIT.md · SECURITY.md -->

# API Module — Public Header Architecture

**Version:** 1.8.0
**Last Updated:** 2026-04-06
**Module Path:** `include/api/`
**Implementation:** `../../src/api/`

---

## 1. Overview

The `include/api/` directory exposes public C++ headers for ThemisDB's multi-protocol API layer,
covering GraphQL (including subscriptions and persisted queries), gRPC, HTTP REST, WebSocket, rate
limiting, observability (tracing, OTLP), audit logging, and correlation ID propagation. These
headers define the contract between the API layer and the query engine, auth, and cache modules.

---

## 2. Design Principles

- **Protocol Unification** – All protocol handlers (`graphql.h`, `grpc_server.h`, `http_handler.h`,
  `websocket_handler.h`) expose a common `IRequestHandler` lifecycle for middleware integration.
- **Observability First** – `tracing_middleware.h`, `otlp_exporter.h`, and `correlation_id.h`
  are always-available interfaces; observability is not optional at the API layer.
- **Schema-Driven GraphQL** – `graphql_schema_builder.h` and `graphql.h` separate schema
  definition from execution; schemas are validated at construction time.
- **Rate Limiting as First-Class** – `rate_limiter.h` is a required interface for all request
  handlers; bypass is not supported by the public API.
- **Version Routing** – `api_version_router.h` handles protocol version negotiation and
  backwards-compatible routing.

---

## 3. Interface Inventory

| Header | Classes / Interfaces | Purpose |
|--------|----------------------|---------|
| `graphql.h` | `IGraphQLEngine`, `GraphQLRequest`, `GraphQLResponse` | GraphQL query/mutation/subscription execution |
| `graphql_schema_builder.h` | `GraphQLSchemaBuilder`, `GraphQLSchema` | Schema definition and validation |
| `graphql_cache.h` | `IGraphQLCache`, `GraphQLCacheKey` | GraphQL response caching |
| `graphql_metrics.h` | `GraphQLMetrics` | GraphQL operation metrics descriptors |
| `graphql_ws_handler.h` | `IGraphQLWSHandler` | GraphQL over WebSocket (graphql-ws protocol) |
| `grpc_server.h` | `IGRPCServer`, `GRPCServerConfig` | gRPC server lifecycle |
| `grpc_bridge.h` | `IGRPCBridge` | gRPC ↔ internal query engine bridge |
| `themisdb_grpc_service.h` | `ThemisDBGRPCService` | ThemisDB gRPC service handler |
| `http_handler.h` | `IHTTPHandler`, `HTTPRequest`, `HTTPResponse` | HTTP REST handler interface |
| `websocket_handler.h` | `IWebSocketHandler` | Generic WebSocket handler |
| `ws_handler.h` | `IWSHandler` | Low-level WebSocket frame handler |
| `rate_limiter.h` | `IRateLimiter`, `RateLimitResult` | Token bucket / sliding window rate limiting |
| `audit_logger.h` | `IAuditLogger`, `AuditEvent` | API-layer audit event emission |
| `correlation_id.h` | `CorrelationID`, `CorrelationContext` | Distributed trace correlation ID propagation |
| `tracing_middleware.h` | `ITracingMiddleware` | OpenTelemetry span creation and propagation |
| `otlp_exporter.h` | `IOTLPExporter`, `OTLPConfig` | OTLP span/metric export |
| `persisted_queries.h` | `IPersistedQueryStore`, `PersistedQueryID` | GraphQL persisted query store |
| `api_version_router.h` | `APIVersionRouter`, `APIVersion` | Protocol version negotiation |
| `geo_index_hooks.h` | `GeoIndexHooks` | Geo index integration hooks for API layer |

> **Implementation details:** `../../src/api/`
