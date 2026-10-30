---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "api-contract"
Canonical_References:
  - "README.md"
  - "API.md"
  - "../../include/api/graphql.h"
  - "../../include/api/graphql_aql_resolver.h"
  - "../../include/api/grpc_server.h"
  - "../../include/api/ws_handler.h"
  - "../../include/api/api_transport_policy.h"
  - "../../include/api/tracing_middleware.h"
  - "graphql.cpp"
  - "graphql_aql_resolver.cpp"
  - "grpc_server.cpp"
  - "ws_handler.cpp"
Related_Issue: "#7167 (closes #7091)"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# API Module Architecture

## Scope, boundaries, and ownership

`src/api/` owns protocol-adaptation components implemented here: GraphQL parsing/resolution and subscription handling, gRPC lifecycle/service adapters, WebSocket change-stream handling, shared transport policy, and request tracing/export helpers. The primary HTTP server and general request routing are owned by `src/server/`; API adapters delegate query/data behavior to other modules.

## Components and public interfaces

| Component | Public interface | Source-backed responsibility |
|---|---|---|
| GraphQL and AQL bridge | [`graphql.h`](../../include/api/graphql.h), [`graphql_aql_resolver.h`](../../include/api/graphql_aql_resolver.h) | Parse GraphQL operations and resolve supported fields through the AQL runner. |
| GraphQL subscriptions | [`graphql_ws_handler.h`](../../include/api/graphql_ws_handler.h), [`ws_handler.h`](../../include/api/ws_handler.h) | Process GraphQL WebSocket messages and validate CDC WebSocket upgrades. |
| gRPC | [`GrpcApiServer`](../../include/api/grpc_server.h), [`ThemisDB gRPC service`](../../include/api/themisdb_grpc_service.h) | Configure/start/stop the optional gRPC server and bind service methods. |
| Admission and errors | [`api_transport_policy.h`](../../include/api/api_transport_policy.h), [`api_transport_contracts.h`](../../include/api/api_transport_contracts.h), [`api_error_taxonomy.h`](../../include/api/api_error_taxonomy.h) | Apply shared transport-level validation and map protocol failures. |
| Correlation/export | [`TracingMiddleware`](../../include/api/tracing_middleware.h), [`OtlpExporter`](../../include/api/otlp_exporter.h) | Maintain request correlation/span context and provide an OTLP export path. |

## Request and event flow

1. A transport adapter parses protocol input and applies its protocol/resource checks.
2. GraphQL resolution validates limits and translates supported operations to AQL; gRPC service methods delegate through configured core interfaces.
3. WebSocket handlers validate/upgrade a request and dispatch subscription/changefeed messages through their configured collaborators.
4. Tracing middleware records correlation/span information and may enqueue it for OTLP export.

See [`graphql.cpp`](graphql.cpp), [`graphql_aql_resolver.cpp`](graphql_aql_resolver.cpp), [`grpc_server.cpp`](grpc_server.cpp), [`ws_handler.cpp`](ws_handler.cpp), and [`tracing_middleware.cpp`](tracing_middleware.cpp).

## Failure domains and edge cases

- Malformed GraphQL, unsupported syntax, query complexity/resource-limit violations, and AQL execution errors are distinct stages.
- gRPC and WebSocket implementations are build/configuration-sensitive; invalid server configuration or an unavailable optional transport prevents that adapter from starting/serving.
- WebSocket change subscriptions require the configured authentication/permission checks; an upgrade rejection is not a query result.
- OTLP export/backpressure failure concerns telemetry delivery and must not be described as a database-operation success/failure.

## Explicit non-goals

- Owning the main HTTP listener, all REST/entity handlers, or authentication implementation.
- Owning query planning/execution, data persistence, or changefeed storage.
- Guaranteeing optional gRPC/WebSocket support is enabled in all builds.

## Verified cross-module references

| Boundary | Evidence and link |
|---|---|
| Query | `graphql_aql_resolver.cpp` calls the AQL runner and query resource-limit APIs; see [query architecture](../query/ARCHITECTURE.md). |
| Server/auth | `ws_handler.cpp` uses server `AuthMiddleware`; `src/server/http_server.cpp` includes base/runtime APIs; server remains the HTTP owner. |
| Observability | `opentelemetry_tracer.cpp` consumes `api/otlp_exporter.h`; see [observability architecture](../observability/ARCHITECTURE.md). |
| CDC | `ws_handler.cpp` accepts the CDC `Changefeed` interface; the API module does not own its storage. |

## References

- [Module README](README.md) · [API contract notes](API.md) · [module roadmap](ROADMAP.md)
- [`api_transport_contracts.h`](../../include/api/api_transport_contracts.h) · [`graphql_aql_resolver.h`](../../include/api/graphql_aql_resolver.h) · [`grpc_server.h`](../../include/api/grpc_server.h)
