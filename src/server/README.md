# ThemisDB Server Module

<!-- Status: current | validated: 2026-08-17 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The server module provides ThemisDB's client-facing runtime surface: HTTP and stream-facing server behavior, API handler integration, middleware and request lifecycle control, selected protocol services, and operational endpoints for health, administration, governance, and observability.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| http_server.cpp | core HTTP server lifecycle and dispatch behavior |
| api_gateway.cpp | routing and gateway behavior |
| auth_middleware.cpp | authentication and authorization middleware behavior |
| rate_limiting_middleware.cpp | request throttling behavior |
| adaptive_rate_limiter.cpp | adaptive rate control behavior |
| load_shedder.cpp | overload protection behavior |
| chunked_response_writer.cpp | streaming/chunked response behavior |
| changefeed_api_handler.cpp | SSE and changefeed endpoint behavior |
| graphql_api_handler.cpp | GraphQL endpoint behavior |
| health_error_service.cpp | health and error endpoint behavior |
| admin_api_handler.cpp | administrative endpoint behavior |
| entity_api_handler.cpp | document endpoint behavior |
| query_api_handler.cpp | query endpoint behavior |
| llm_api_handler.cpp | LLM endpoint behavior |
| vector_api_handler.cpp | vector endpoint behavior |
| graph_api_handler.cpp | graph endpoint behavior |
| timeseries_api_handler.cpp | timeseries endpoint behavior |
| websocket_session.cpp | WebSocket session behavior |
| mqtt_session.cpp | MQTT session behavior |
| postgres_session.cpp | PostgreSQL wire session behavior |
| themis_core_grpc_service.cpp | gRPC service behavior |
| wal_grpc_service.cpp | WAL gRPC behavior |

## Scope

In scope:
- HTTP request lifecycle, handler dispatch, and middleware behavior
- selected stream/protocol session behavior owned by server runtime
- health, admin, observability, governance, and API endpoint surfaces

Out of scope:
- storage, query execution, or index internals owned by other modules
- client SDKs and external consumer libraries outside server runtime boundaries

## Runtime Behavior and Limits

- server behavior depends on routing, middleware, and endpoint-handler composition.
- hot paths include HTTP request build/parse helpers, selected endpoint paths, and stream protocol helpers.
- degradation under overload must remain explicit through rate limiting, shedding, and health signaling.

## Sourcecode Verification (Module: server/readme)

- Verified files:
  - src/server/http_server.cpp
  - src/server/api_gateway.cpp
  - src/server/auth_middleware.cpp
  - src/server/rate_limiting_middleware.cpp
  - src/server/adaptive_rate_limiter.cpp
  - src/server/load_shedder.cpp
  - src/server/chunked_response_writer.cpp
  - src/server/changefeed_api_handler.cpp
  - src/server/graphql_api_handler.cpp
  - src/server/health_error_service.cpp
  - src/server/admin_api_handler.cpp
  - src/server/entity_api_handler.cpp
  - src/server/query_api_handler.cpp
  - src/server/llm_api_handler.cpp
  - src/server/vector_api_handler.cpp
  - src/server/graph_api_handler.cpp
  - src/server/timeseries_api_handler.cpp
  - src/server/websocket_session.cpp
  - src/server/mqtt_session.cpp
  - src/server/postgres_session.cpp
  - src/server/themis_core_grpc_service.cpp
  - src/server/wal_grpc_service.cpp
- Verified test suites:
  - tests/server/test_server_contract_hardening_focused.cpp (SCH-01..SCH-20; Phase 1 auth/contract hardening)
  - tests/server/test_server_phase5_hardening.cpp (P5-S01 WSR-01..16 wire-protocol retry; P5-S02 HST-01..12 HTTP timeout/shutdown)
- Verified benchmark anchors:
  - benchmarks/bench_api_endpoints.cpp
  - benchmarks/bench_stream_protocol.cpp