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

<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · AUDIT.md · CHANGELOG.md · SECURITY.md · ../../src/api/README.md -->

# API Module — Public Headers

**Module Path:** `include/api/`
**Implementation:** `../../src/api/`
**Status:** Public header contracts; optional transports depend on build configuration.

This directory contains the public C++ header files (`.h`) for the `api` module.
All headers use `#pragma once`. Some provide inline implementation, including validation and error-mapping helpers.

## Header Overview

| File | Purpose |
|------|---------|
| [`graphql.h`](graphql.h) | GraphQL value, operation, fragment, parser, query-limit, executor, and schema types |
| [`graphql_cache.h`](graphql_cache.h) | `Cache`, `QueryPlanCache`, and `ResponseCache` |
| [`graphql_metrics.h`](graphql_metrics.h) | `Metrics` and `QueryTimer` for GraphQL request metrics |
| [`graphql_schema_builder.h`](graphql_schema_builder.h) | `IGraphQLSchemaBuilder` and schema descriptor/result types |
| [`graphql_aql_resolver.h`](graphql_aql_resolver.h) | `GraphQLComplexityEstimator` and `GraphQLAQLResolverFactory` for GraphQL-to-AQL integration |
| [`graphql_ws_handler.h`](graphql_ws_handler.h) | `api::GraphQLWsHandler` — GraphQL over the `graphql-transport-ws` protocol |
| [`grpc_server.h`](grpc_server.h) | `GrpcApiServer` and `GrpcServerConfig` — gRPC server lifecycle (compiled only when `THEMIS_ENABLE_GRPC` is defined) |
| [`grpc_bridge.h`](grpc_bridge.h) | `IGRPCBridge` and gRPC service/request metadata types; no concrete bridge implementation is declared here |
| [`themisdb_grpc_service.h`](themisdb_grpc_service.h) | `ThemisDBGrpcService` — gRPC service implementation |
| [`themisdb_grpc_service_factory.h`](themisdb_grpc_service_factory.h) | `ThemisDBGrpcServiceFactory` — injects service dependencies and builds the service |
| [`http_handler.h`](http_handler.h) | `IHttpHandler`, `HttpRequest`, `HttpResponse`, and `MiddlewareChain` contracts |
| [`websocket_handler.h`](websocket_handler.h) | `IWebSocketHandler`, `WebSocketSession`, and WebSocket frame interfaces |
| [`ws_handler.h`](ws_handler.h) | `WsChangeHandler` — validates CDC WebSocket upgrades and extracts subscription parameters |
| [`subscription_multiplexer.h`](subscription_multiplexer.h) | `ISubscriptionMultiplexer` and live-query subscription event/filter types |
| [`tracing_middleware.h`](tracing_middleware.h) | `TracingMiddleware` — `X-Correlation-ID` propagation and thread-local request context |
| [`correlation_id.h`](correlation_id.h) | `CorrelationId` — request correlation ID generation and propagation |
| [`rate_limiter.h`](rate_limiter.h) | `themis::graphql::RateLimiter` and `OperationRateLimiter` token buckets |
| [`graphql_audit_logger.h`](graphql_audit_logger.h) | `themis::graphql::AuditLogger`, audit event/builder types, and file handler |
| [`otlp_exporter.h`](otlp_exporter.h) | `OtlpExporter`, `OtlpExporterConfig`, and `SpanData` |
| [`persisted_queries.h`](persisted_queries.h) | `PersistedQueryRegistry`, `QueryAllowList`, and `QueryHasher` |
| [`geo_index_hooks.h`](geo_index_hooks.h) | GeoJSON validation hooks wired into the storage write/delete path |
| [`api_gateway_hook.h`](api_gateway_hook.h) | `IAPIGatewayHook` and `IGatewayHookRegistry` extension contracts |
| [`api_version_router.h`](api_version_router.h) | `IAPIVersionRouter` and versioned route descriptor types |
| [`aql_utils.h`](aql_utils.h) | Utility helpers for AQL query construction and parameter binding |
| [`federation_admin_handler.h`](federation_admin_handler.h) | `FederationAdminHandler` — administrative API for federated node management |
| [`api_transport_contracts.h`](api_transport_contracts.h) | Supported API versions, transport capabilities/failure classes, and contract validation |
| [`api_transport_policy.h`](api_transport_policy.h) | `TransportPolicyMiddleware` and configurable transport bounds |
| [`api_error_taxonomy.h`](api_error_taxonomy.h) | `ApiErrorTaxonomy` mappings for transport failures |

## Build Conditionals

| Symbol | Guards |
|--------|--------|
| `THEMIS_ENABLE_GRPC` | `grpc_server.h`, `themisdb_grpc_service.h`, `themisdb_grpc_service_factory.h` |
| `THEMIS_ENABLE_WEBSOCKET` | `ws_handler.h`, `graphql_ws_handler.h` |
| `THEMIS_HAS_PROMETHEUS` | Optional Prometheus integration in `otlp_exporter.h` |

`otlp_exporter.h` does not use a `THEMIS_ENABLE_OTEL` header guard; it includes libcurl directly.

## Documentation

- Module overview: [`../../src/api/README.md`](../../src/api/README.md)
- Architecture guide: [`../../src/api/ARCHITECTURE.md`](../../src/api/ARCHITECTURE.md)
- Header type hierarchy: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Roadmap: [`ROADMAP.md`](ROADMAP.md)
- Future enhancements: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- Changelog: [`CHANGELOG.md`](CHANGELOG.md)
- Audit report: [`AUDIT.md`](AUDIT.md)
- Security: [`SECURITY.md`](SECURITY.md)
- Full implementation roadmap: [`../../src/api/ROADMAP.md`](../../src/api/ROADMAP.md)
- Full future enhancements: [`../../src/api/FUTURE_ENHANCEMENTS.md`](../../src/api/FUTURE_ENHANCEMENTS.md)

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

### GraphQL Parsing and Execution

```cpp
#include "api/graphql.h"

using namespace themis::graphql;

// Production limits retain the parser's default bounds and disable introspection.
QueryLimits limits = QueryLimits::production();

auto parsed = Parser::parse(query_string, limits);
if (!parsed.success) {
    for (const auto& error : parsed.errors) {
        // Report or map the parse error.
    }
    return;
}

ExecutionContext context;
context.resolvers["health"] = [](const Field&,
                                 const std::shared_ptr<Value>&,
                                 const ExecutionContext&) {
    return Value::string("ok");
};
Executor executor;
auto result = executor.execute(parsed.document, context);
```

### gRPC Server Setup (requires THEMIS_ENABLE_GRPC)

```cpp
#include "api/grpc_server.h"
#include "api/themisdb_grpc_service_factory.h"

using namespace themis;
api::GrpcServerConfig cfg;
cfg.host = "0.0.0.0";
cfg.port = 9090;
cfg.tls_enabled  = true;
cfg.tls_cert_path = "/etc/themisdb/server.pem";
cfg.tls_key_path  = "/etc/themisdb/server.key";

api::GrpcApiServer server;
if (!server.initialize(cfg)) {
    return;
}

auto svc = api::ThemisDBGrpcServiceFactory{}
               .withDb(db_instance)
               .withQueryEngine(aql_engine)
               .build();
server.registerService(svc.get());
if (!server.start()) {
    return;
}
// ... later:
server.stop();
```

### WebSocket CDC Subscription (requires THEMIS_ENABLE_WEBSOCKET)

```cpp
#include "api/ws_handler.h"

using namespace themis;

// In HTTP upgrade handler:
api::WsChangeHandler handler(auth_middleware, changefeed);
const auto decision = handler.validate(upgrade_request);
if (!decision.should_upgrade) {
    // Reject with decision.reject_status and decision.reject_reason.
    return;
}
// The server's HTTP adapter performs the WebSocket protocol upgrade.
```

### Request Tracing with Correlation IDs

```cpp
#include "api/tracing_middleware.h"

using namespace themis;

api::TracingMiddleware tracing;
const auto correlation_id = tracing.processRequest(incoming_correlation_id);
tracing.finishSpan("HTTP GET /v2/entities", 200);
api::TracingMiddleware::clearContext();
```

### Rate Limiting

```cpp
#include "api/rate_limiter.h"

using namespace themis;

// Per-tenant token bucket: 100 tokens capacity, refill 10/sec
graphql::RateLimiter::Config cfg;
cfg.capacity    = 100;
cfg.refill_rate = 10;

graphql::RateLimiter limiter(cfg);
if (!limiter.allow(tenant_id)) {
    // The calling transport adapter chooses the rejection response.
}
```

### Audit Logging

```cpp
#include "api/graphql_audit_logger.h"

using namespace themis;

// Register a file-backed handler (JSONL, one entry per line)
graphql::AuditLogger logger;
logger.addFileHandler("/var/log/themisdb/audit.jsonl");

// Log a security event
graphql::AuditLogEntry entry{};
entry.event_type = graphql::AuditLogEntry::EventType::MutationExecution;
entry.operation_name = "document.delete";
entry.tenant_id = tenant_id;
entry.success = true;
logger.log(entry);
```

## Troubleshooting

### `GrpcApiServer` is unavailable

The server API is guarded by `THEMIS_ENABLE_GRPC`. Enable gRPC when configuring the project and link the gRPC target:

```cmake
target_compile_definitions(your_target PRIVATE THEMIS_ENABLE_GRPC)
find_package(gRPC REQUIRED)
target_link_libraries(your_target gRPC::grpc++)
```

### GraphQL fragment or directive parsing

Named fragment definitions/spreads and the `@include`/`@skip` field directives are supported. Inline
fragments and directives other than `@include` and `@skip` are rejected by the current parser.

### `QueryAllowList` not enforcing restrictions

`QueryAllowList` exposes an `enabled` flag that defaults to `false`; `isAllowed()` itself checks
membership and does not consult that flag. Add a hash with `allow()` and query membership as needed:

```cpp
#include "api/persisted_queries.h"

themis::graphql::QueryAllowList::instance().setEnabled(true);
themis::graphql::QueryAllowList::instance().allow("<query-hash>");
const bool allowed = themis::graphql::QueryAllowList::instance().isAllowed("<query-hash>");
```

TODO: Verify and document the production call-site enforcement path before treating `enabled` as a
security control; this header does not connect the flag to `isAllowed()`.

### gRPC `start()` appears to deadlock

If `GrpcApiServer::start()` hangs, verify that no external code is calling `stop()` or `isRunning()`
concurrently before `start()` returns — the mutex is released before `BuildAndStart()` since v1.9.0.

### OTLP traces not appearing in the collector

1. Verify `OtlpExporterConfig::endpoint` points to your collector (default: `http://localhost:4318/v1/traces`).
2. Check `OtlpExporter::droppedSpanCount()` — if > 0, the collector may be unreachable or returning 5xx.
3. Set `OtlpExporterConfig::enabled` to `true`, start the exporter, and ensure libcurl is available.

### WebSocket CDC query parameters

`WsChangeHandler::validate()` URL-decodes valid `%HH` escapes in `from_sequence` and `key_prefix`
before parsing them. Malformed or incomplete escape sequences are retained as literal characters;
check the decoded value and `UpgradeDecision` when diagnosing a rejected upgrade.

## See Also

- [Module overview](../../src/api/README.md)
- [Architecture guide](../../src/api/ARCHITECTURE.md)
- [Roadmap](../../src/api/ROADMAP.md)
- [Future enhancements](../../src/api/FUTURE_ENHANCEMENTS.md)
- [Security](../../src/api/SECURITY.md)
- [Changelog](../../src/api/CHANGELOG.md)
