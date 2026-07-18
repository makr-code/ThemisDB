# ThemisDB API Module

<!-- Status: current | validated: 2026-07-18 -->
<!-- Agentic status sync: module issue #5618 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · AUDIT.md -->

## Module Purpose

The API module provides transport-facing integration surfaces for GraphQL, gRPC, WebSocket, tracing, and geospatial API hooks.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| graphql.cpp | GraphQL parsing and execution support paths |
| graphql_aql_resolver.cpp | GraphQL-to-AQL resolver bridge for query execution support |
| graphql_ws_handler.cpp | GraphQL over WebSocket subscription handling |
| grpc_server.cpp | gRPC server lifecycle integration |
| themisdb_grpc_service.cpp | gRPC service method handling |
| ws_handler.cpp | WebSocket CDC-related transport handling |
| tracing_middleware.cpp | request correlation and tracing middleware |
| otlp_exporter.cpp | OTLP export path for tracing data |
| geo_index_hooks.cpp | API-level geospatial index hook integration |
| federation_admin_handler.cpp | federation-facing API admin handling |

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
  - src/api/graphql_aql_resolver.cpp
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
  - GraphQL-to-AQL resolver bridging for query execution entry points
  - tracing and OTLP export middleware surfaces
  - geo and federation admin API integration paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical completion remains in CHANGELOG.md