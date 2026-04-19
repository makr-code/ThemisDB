<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — API Module Public Headers

All notable changes to public headers in `include/api/`.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.8.0] — 2026-03-22
### Added
- `graphql_ws_handler.h`: `IGraphQLWSHandler` for graphql-ws protocol over WebSocket
- `otlp_exporter.h`: `IOTLPExporter` and `OTLPConfig` for OpenTelemetry OTLP export
- `tracing_middleware.h`: `ITracingMiddleware` for span creation and W3C trace-context propagation
- `persisted_queries.h`: `IPersistedQueryStore` for GraphQL automatic persisted queries (APQ)
- `geo_index_hooks.h`: `GeoIndexHooks` for geo index integration at the API boundary

### Changed
- `graphql.h`: `GraphQLRequest` extended with `persisted_query_id` field
- `rate_limiter.h`: Added `RateLimitPolicy` enum (token_bucket, sliding_window, fixed_window)
- `api_version_router.h`: Added `negotiate()` method for content-type negotiation

## [1.7.0] — 2026-03-09
### Added
- `graphql_metrics.h`: `GraphQLMetrics` descriptor for operation-level Prometheus metrics
- `graphql_cache.h`: `IGraphQLCache` and `GraphQLCacheKey` for response-level caching
- `graphql_schema_builder.h`: `GraphQLSchemaBuilder` for programmatic schema construction
- `grpc_bridge.h`: `IGRPCBridge` connecting gRPC to internal query engine
- `correlation_id.h`: `CorrelationID` and `CorrelationContext` for distributed tracing

## [1.6.0] — 2026-02-01
### Added
- Initial public header set: `graphql.h`, `grpc_server.h`, `http_handler.h`,
  `websocket_handler.h`, `ws_handler.h`
- `rate_limiter.h`: Token bucket rate limiter interface
- `audit_logger.h`: `IAuditLogger` and `AuditEvent`
- `api_version_router.h`: Version negotiation interface
- `themisdb_grpc_service.h`: ThemisDB gRPC service handler
