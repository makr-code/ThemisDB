<!-- Status: current | validated: 2026-04-19 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — API Module Public Headers

**Last Audit:** 2026-04-19
**Auditor:** Copilot
**Status:** ✅ Pass (15 open findings in src/ — see `../../src/api/AUDIT.md`)

---

## Summary

| Metric | Result |
|--------|--------|
| Public Header Files | 25 `.h` |
| Open Stubs | 0 (all interfaces implemented in `src/api/`) |
| Security Headers | ✅ (`audit_logger.h`, `rate_limiter.h`, `tracing_middleware.h`) |
| GraphQL Coverage | ✅ (query, subscriptions, WS, cache, metrics, persisted queries) |
| Open Findings (src/) | 15 (see `../../src/api/AUDIT.md`) |

---

## Header Files Audited

| File | Exported Symbols | Notes |
|------|-----------------|-------|
| `graphql.h` | `IGraphQLEngine`, `GraphQLRequest/Response` | Core GraphQL interface |
| `graphql_schema_builder.h` | `GraphQLSchemaBuilder`, `GraphQLSchema` | Schema construction |
| `graphql_cache.h` | `IGraphQLCache`, `GraphQLCacheKey` | Response caching |
| `graphql_metrics.h` | `GraphQLMetrics` | Metrics descriptors |
| `graphql_ws_handler.h` | `IGraphQLWSHandler` | graphql-ws protocol |
| `grpc_server.h` | `IGRPCServer`, `GRPCServerConfig` | gRPC server lifecycle |
| `grpc_bridge.h` | `IGRPCBridge` | gRPC ↔ query engine bridge |
| `themisdb_grpc_service.h` | `ThemisDBGRPCService` | Service handler |
| `http_handler.h` | `IHTTPHandler`, `HTTPRequest/Response` | REST handler |
| `websocket_handler.h` | `IWebSocketHandler` | WebSocket handler |
| `ws_handler.h` | `IWSHandler` | Frame-level WS handler |
| `rate_limiter.h` | `IRateLimiter`, `RateLimitResult` | Rate limiting |
| `audit_logger.h` | `IAuditLogger`, `AuditEvent` | Audit emission |
| `correlation_id.h` | `CorrelationID`, `CorrelationContext` | Trace correlation |
| `tracing_middleware.h` | `ITracingMiddleware` | OTel span management |
| `otlp_exporter.h` | `IOTLPExporter`, `OTLPConfig` | OTLP export |
| `persisted_queries.h` | `IPersistedQueryStore`, `PersistedQueryID` | Persisted queries |
| `api_version_router.h` | `APIVersionRouter`, `APIVersion` | Version negotiation |
| `geo_index_hooks.h` | `GeoIndexHooks` | Geo integration hooks |
| `api_gateway_hook.h` | `APIGatewayHook` | ✅ Reviewed |
| `aql_utils.h` | `AQLUtils` | ✅ Reviewed |
| `federation_admin_handler.h` | `FederationAdminHandler` | ✅ Reviewed |
| `graphql_aql_resolver.h` | `GraphQLAQLResolver` | ✅ Reviewed |
| `subscription_multiplexer.h` | `SubscriptionMultiplexer` | ✅ Reviewed |
| `themisdb_grpc_service_factory.h` | `ThemisDBGRPCServiceFactory` | ✅ Reviewed |

---

## Findings

### Resolved
- `rate_limiter.h` present and required by all handler interfaces.
- `audit_logger.h` covers all protocol handlers.
- `correlation_id.h` propagation available for all request types.

### Open
- 15 open findings at implementation level — see `../../src/api/AUDIT.md` for details.
- `ws_handler.h` and `websocket_handler.h` overlap should be clarified in v1.9.0.
