> **Build:** `cmake --preset release && cmake --build build/release`

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
| [`audit_logger.h`](audit_logger.h) | Audit event logging interface for security-relevant API operations |
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

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "api/graphql.h"
```
