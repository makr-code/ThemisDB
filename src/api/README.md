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

# ThemisDB API Module

<!-- Links: ARCHITECTURE.md · API.md · IMPLEMENTATION.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The API module provides transport-facing integration surfaces for GraphQL, gRPC, WebSocket, tracing, and geospatial API hooks.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| graphql.cpp | GraphQL parsing and execution support paths |
| graphql_ws_handler.cpp | GraphQL over WebSocket subscription handling |
| grpc_server.cpp | gRPC server lifecycle integration |
| themisdb_grpc_service.cpp | gRPC service method handling |
| ws_handler.cpp | WebSocket CDC-related transport handling |
| tracing_middleware.cpp | request correlation and tracing middleware |
| otlp_exporter.cpp | OTLP export path for tracing data |
| geo_index_hooks.cpp | API-level geospatial index hook integration |
| federation_admin_handler.cpp | federation-facing API admin handling |

## Request and event flow

- Protocol adapters parse and validate requests before delegating to configured query, database,
  authentication, or changefeed collaborators.
- `TransportPolicyMiddleware` applies shared HTTP request checks only where it is installed in the
  handler chain; it is not an implicit global server filter.
- GraphQL parsing produces an operation document; execution uses resolver callbacks from an
  `ExecutionContext`. The AQL resolver factory binds supported GraphQL fields to the query engine.
- WebSocket handlers process protocol frames or validate CDC upgrade requests; the primary HTTP
  listener and general route dispatch remain in `src/server/`.
- `TracingMiddleware` manages correlation/span context, and `OtlpExporter` handles span delivery
  separately from database-operation results.

## Configuration and build conditions

- The shared transport contract currently advertises API versions `v1` and `v2`, a 10 MiB payload
  maximum, and a 4096-byte path maximum. `TransportPolicyConfig` may lower the payload/path bounds.
- `GrpcApiServer` and `ThemisDBGrpcService` require `THEMIS_ENABLE_GRPC`; the WebSocket handlers
  `WsChangeHandler` and `GraphQLWsHandler` require `THEMIS_ENABLE_WEBSOCKET`.
- `GrpcServerConfig::max_message_size_bytes` defaults to 100 MiB. The server implementation can
  override it from `grpc.max_message_size_mb` in `config/networking/grpc.yaml`.
- `otlp_exporter.h` directly includes libcurl and is not guarded by `THEMIS_ENABLE_OTEL`;
  Prometheus integration is conditional on `THEMIS_HAS_PROMETHEUS`.

## Focused validation

`tests/api/CMakeLists.txt` discovers `test_*.cpp` files and registers focused API tests with the
`api` label. For a configured build directory, run the focused targets or
`ctest --test-dir <build-dir> -L api --output-on-failure`. Benchmark targets are described in
[`../../benchmarks/api/README.md`](../../benchmarks/api/README.md) and
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).

## Scope

In scope:
- GraphQL, gRPC, and WebSocket protocol integration paths
- tracing and observability transport middleware in api module
- API-facing geospatial and federation admin integration points

Out of scope:
- primary HTTP server runtime in src/server
- authentication internals in src/auth
- query and storage core business logic internals

## Runtime Behavior and Limits

- API transport surfaces are capability- and configuration-sensitive.
- integration paths are designed as protocol adaptation layers with structured error handling.
- optional transport and observability surfaces can degrade based on environment configuration.

## Sourcecode Verification (Module: api/readme)

- Verified files:
  - src/api/graphql.cpp
  - src/api/graphql_ws_handler.cpp
  - src/api/grpc_server.cpp
  - src/api/themisdb_grpc_service.cpp
  - src/api/ws_handler.cpp
  - src/api/tracing_middleware.cpp
  - src/api/otlp_exporter.cpp
  - src/api/geo_index_hooks.cpp
  - src/api/federation_admin_handler.cpp
- Verified behavior surfaces:
  - protocol adaptation for GraphQL/gRPC/WebSocket
  - tracing and OTLP export middleware surfaces
  - geo and federation admin API integration paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical completion remains in CHANGELOG.md
  - the generated Doxygen snapshot is not canonical; see DOXYGEN.md for its freshness status