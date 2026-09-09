# Architecture - API Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The API module composes protocol adapters and transport middleware for client-facing access paths. It bridges external protocol surfaces to internal service and handler layers.

## Main Execution Planes

1. GraphQL plane
- parse and execute GraphQL requests
- support WebSocket-based GraphQL subscription transport

2. gRPC plane
- host gRPC server lifecycle and service registration
- map protobuf-based requests to internal handlers

3. WebSocket and CDC plane
- handle upgrade and message lifecycle for CDC-related streams
- enforce module-level transport boundaries

4. Tracing and observability plane
- propagate request correlation context
- export trace data through OTLP path where configured

## Core Contracts

| Contract | Behavior |
|---|---|
| GraphQL interfaces | query parsing/execution and subscription transport orchestration |
| gRPC interfaces | RPC lifecycle and service adapter behavior |
| WebSocket interfaces | stream/session handling and CDC message transport behavior |
| tracing interfaces | correlation and OTLP emission pipeline |

## Failure Semantics

- unsupported capability paths return structured failures rather than silent fallback.
- API adapters delegate business logic execution and preserve transport-level error normalization.
- observability export failures do not redefine transport business semantics.

## Sourcecode Verification (Module: api/architecture)

- Verified files:
  - src/api/graphql.cpp
  - src/api/graphql_ws_handler.cpp
  - src/api/grpc_server.cpp
  - src/api/themisdb_grpc_service.cpp
  - src/api/ws_handler.cpp
  - src/api/tracing_middleware.cpp
  - src/api/otlp_exporter.cpp
- Verified architecture claims:
  - protocol-adapter role of API module
  - distinct transport planes for GraphQL/gRPC/WebSocket
  - dedicated tracing/export middleware presence
---

### Direct Downstream Consumers (modules that use this module)

| Module | Via | Notes |
|--------|-----|-------|
| `server` | `include/api/graphql.h`, `include/api/graphql_aql_resolver.h`, `include/api/tracing_middleware.h`, `include/api/ws_handler.h`, `include/api/geo_index_hooks.h` | HTTP server embeds GraphQL execution, AQL resolver, WebSocket handler, tracing middleware, and geo-index hooks (`include/server/http_server.h`, `include/server/graphql_api_handler.h`, `src/server/entity_api_handler.cpp`) |
| `query` | `include/api/graphql.h`, `include/api/graphql_aql_resolver.h` | GraphQL function registry uses `graphql.h` and `graphql_aql_resolver.h` for field resolution against query engine (`include/query/functions/graphql_functions.h`) |
| `observability` | `include/api/otlp_exporter.h` | OpenTelemetry tracer forwards span data via the OTLP exporter path (`src/observability/opentelemetry_tracer.cpp`) |
| `main` | `api/themisdb_grpc_service.h` | Server entry point registers the gRPC service at startup (`src/main_server.cpp`) |
