> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-05-11 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · AUDIT.md · CHANGELOG.md · SECURITY.md · ../../src/api/README.md -->

# API Module — Public Headers

**Module Path:** `include/api/`
**Implementation:** `../../src/api/`
**Status:** ✅ Production Ready

This directory contains the public C++ header files (`.h`) for the `api` module.
All headers are `#pragma once` guarded and contain no implementation code.

## Header Overview

| File | Purpose |
|------|---------|
| [`graphql.h`](graphql.h) | GraphQL `Value`, `Field`, `SelectionSet`, `Parser`, `QueryLimits` and `Executor` types |
| [`graphql_cache.h`](graphql_cache.h) | LRU query-plan cache (`GraphQLCache`) for parsed GraphQL documents |
| [`graphql_metrics.h`](graphql_metrics.h) | Prometheus-style metrics collector for GraphQL requests and errors |
| [`graphql_schema_builder.h`](graphql_schema_builder.h) | `GraphQLSchemaBuilder` — programmatic schema construction |
| [`graphql_aql_resolver.h`](graphql_aql_resolver.h) | `GraphQLAQLResolver` — maps GraphQL selections to AQL queries |
| [`graphql_ws_handler.h`](graphql_ws_handler.h) | `GraphQLWsHandler` — GraphQL over WebSocket (graphql-ws protocol) |
| [`grpc_server.h`](grpc_server.h) | `GrpcApiServer` and `GrpcServerConfig` — gRPC server lifecycle (compiled only when `THEMIS_ENABLE_GRPC` is defined) |
| [`grpc_bridge.h`](grpc_bridge.h) | `GrpcBridge` — translates gRPC calls to internal engine requests |
| [`themisdb_grpc_service.h`](themisdb_grpc_service.h) | `ThemisDBService` gRPC handler — document CRUD and AQL execution over protobuf |
| [`themisdb_grpc_service_factory.h`](themisdb_grpc_service_factory.h) | `ThemisDBServiceFactory` — constructs `ThemisDBService` instances |
| [`http_handler.h`](http_handler.h) | `HttpHandler` — HTTP/1.1 and HTTP/2 request dispatcher |
| [`websocket_handler.h`](websocket_handler.h) | `WebSocketHandler` — low-level WebSocket frame handler |
| [`ws_handler.h`](ws_handler.h) | `WsChangeHandler` — WebSocket upgrade validator and frame dispatcher for `/v2/changes` and `/v2/cdc/stream` |
| [`subscription_multiplexer.h`](subscription_multiplexer.h) | `SubscriptionMultiplexer` — fan-out for live query subscriptions |
| [`tracing_middleware.h`](tracing_middleware.h) | `TracingMiddleware` — `X-Correlation-ID` propagation and thread-local request context |
| [`correlation_id.h`](correlation_id.h) | `CorrelationId` — request correlation ID generation and propagation |
| [`rate_limiter.h`](rate_limiter.h) | Token-bucket rate limiter interface for per-client/per-tenant request throttling |
| [`graphql_audit_logger.h`](graphql_audit_logger.h) | Audit event logging interface for security-relevant API operations |
| [`otlp_exporter.h`](otlp_exporter.h) | `OTLPExporter` — OpenTelemetry OTLP trace/metric exporter |
| [`persisted_queries.h`](persisted_queries.h) | Persisted GraphQL query store — hash-keyed lookup to avoid repeated parsing |
| [`geo_index_hooks.h`](geo_index_hooks.h) | GeoJSON validation hooks wired into the storage write/delete path |
| [`api_gateway_hook.h`](api_gateway_hook.h) | `APIGatewayHook` — pluggable pre/post-processing for gateway requests |
| [`api_version_router.h`](api_version_router.h) | `APIVersionRouter` — routes requests to versioned handler sets |
| [`aql_utils.h`](aql_utils.h) | Utility helpers for AQL query construction and parameter binding |
| [`federation_admin_handler.h`](federation_admin_handler.h) | `FederationAdminHandler` — administrative API for federated node management |

## Build Conditionals

| Symbol | Guards |
|--------|--------|
| `THEMIS_ENABLE_GRPC` | `grpc_server.h`, `themisdb_grpc_service.h` |
| `THEMIS_ENABLE_WEBSOCKET` | `ws_handler.h` |

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

// Use production limits (disables introspection, caps depth/complexity)
QueryLimits limits = QueryLimits::production();

Parser parser;
auto result = parser.parse(query_string, limits);
if (!result) {
    // Handle parse error
    return;
}

Executor executor;
auto json_result = executor.execute(result.value(), variables, my_resolver);
```

### gRPC Server Setup (requires THEMIS_ENABLE_GRPC)

```cpp
#include "api/grpc_server.h"
#include "api/themisdb_grpc_service_factory.h"

api::GrpcServerConfig cfg;
cfg.bind_address = "0.0.0.0:9090";
cfg.tls_enabled  = true;
cfg.cert_path    = "/etc/themisdb/server.pem";
cfg.key_path     = "/etc/themisdb/server.key";

api::GrpcApiServer server;
server.initialize(cfg);

auto svc = api::ThemisDBServiceFactory{}
               .withDb(db_instance)
               .withQueryEngine(aql_engine)
               .build();
server.registerService(svc.get());
server.start();
// ... later:
server.stop();
```

### WebSocket CDC Subscription (requires THEMIS_ENABLE_WEBSOCKET)

```cpp
#include "api/ws_handler.h"

// In HTTP upgrade handler:
api::WsChangeHandler handler(auth_middleware, changefeed);
if (!handler.validate(upgrade_request)) {
    // Return HTTP 401/403
    return;
}
handler.accept(std::move(socket)); // hands off to async WebSocket loop
```

### Request Tracing with Correlation IDs

```cpp
#include "api/tracing_middleware.h"

api::TracingMiddleware tracing(otlp_exporter_ptr);
tracing.processRequest(request);  // generates/propagates X-Correlation-ID
// All log lines on this thread now carry the correlation ID
```

### Rate Limiting

```cpp
#include "api/rate_limiter.h"

// Per-tenant token bucket: 100 tokens capacity, refill 10/sec
api::RateLimiter::Config cfg;
cfg.capacity    = 100;
cfg.refill_rate = 10;

api::RateLimiter limiter(cfg);
if (!limiter.allow(tenant_id)) {
    // Return HTTP 429 Too Many Requests
}
```

### Audit Logging

```cpp
#include "api/graphql_audit_logger.h"

// Register a file-backed handler (JSONL, one entry per line)
api::AuditLogger::instance().addFileHandler("/var/log/themisdb/audit.jsonl");

// Log a security event
api::AuditLogger::instance().log({
    .actor    = tenant_id,
    .action   = "document.delete",
    .resource = collection + "/" + key,
    .outcome  = api::AuditOutcome::SUCCESS,
});
```

## Troubleshooting

### `fatal error: api/grpc_server.h: No such file or directory`

`grpc_server.h` is only compiled when `THEMIS_ENABLE_GRPC` is defined. Add to your CMake target:

```cmake
target_compile_definitions(your_target PRIVATE THEMIS_ENABLE_GRPC)
find_package(gRPC REQUIRED)
target_link_libraries(your_target gRPC::grpc++)
```

### GraphQL `ParseError`: "Not yet supported: Fragments"

The current `Parser` does not support GraphQL fragments or directives (v1.x limitation). Use flat
queries without `fragment ... on ...` syntax until fragment support is added in v2.1.0.

### `QueryAllowList` not enforcing restrictions

`QueryAllowList` is disabled by default (`enabled_ = false`). Enable it explicitly:

```cpp
#include "api/persisted_queries.h"

api::QueryAllowList::instance().setEnabled(true);
api::QueryAllowList::instance().registerQuery("sha256:<hash>", query_string);
```

### gRPC `start()` appears to deadlock

If `GrpcApiServer::start()` hangs, verify that no external code is calling `stop()` or `isRunning()`
concurrently before `start()` returns — the mutex is released before `BuildAndStart()` since v1.9.0.

### OTLP traces not appearing in the collector

1. Verify `OtlpExporterConfig::endpoint` points to your collector (default: `http://localhost:4318/v1/traces`).
2. Check `OtlpExporter::droppedSpanCount()` — if > 0, the collector may be unreachable or returning 5xx.
3. Confirm `THEMIS_ENABLE_OTEL` is defined and `otlp_exporter.h` is included.

### WebSocket `validate()` returns false for percent-encoded parameters

`WsChangeHandler::validate()` does not URL-decode `from_sequence` and `key_prefix` query-string
parameters (known issue, tracked in `FUTURE_ENHANCEMENTS.md`). Avoid percent-encoding these
parameters until the fix is released.

## See Also

- [Module overview](../../src/api/README.md)
- [Architecture guide](../../src/api/ARCHITECTURE.md)
- [Roadmap](../../src/api/ROADMAP.md)
- [Future enhancements](../../src/api/FUTURE_ENHANCEMENTS.md)
- [Security](../../src/api/SECURITY.md)
- [Changelog](../../src/api/CHANGELOG.md)

