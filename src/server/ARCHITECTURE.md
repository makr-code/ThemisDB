# Architecture - Server Module

<!-- Status: current | validated: 2026-08-17 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The server module composes gateway routing, middleware enforcement, endpoint dispatch, stream/session handling, and operational server surfaces into the client-facing runtime boundary of ThemisDB.

## Main Execution Planes

1. Request lifecycle plane
- HTTP server ingress, request parsing, middleware execution, and handler dispatch behavior

2. Endpoint plane
- administrative, data, governance, observability, and specialized API handler behavior

3. Session and protocol plane
- WebSocket, MQTT, PostgreSQL-wire, and gRPC service behavior owned by the server runtime

## Core Contracts

| Contract | Behavior |
|---|---|
| ingress contract | explicit request acceptance, parsing, and dispatch semantics |
| middleware contract | bounded auth, rate-limit, and overload-control behavior |
| endpoint contract | handler-local API behavior behind shared server lifecycle control |
| session contract | explicit protocol/session behavior for server-owned transports |

## Failure Semantics

- request rejection and overload behavior remain explicit through middleware and health signaling.
- endpoint failures remain bounded to handler and response paths rather than widening into transport control.
- protocol/session failures remain observable through server-owned runtime surfaces.

## Retry, Timeout, and Graceful-Shutdown Patterns (Phase 5, Q3 2026)

Phase 5 hardening delivered two runtime resilience patterns that are now part of the production architecture:

- **P5-S01 Wire-protocol retry** — exponential-backoff retry with configurable `max_retries`, `base_delay`, global budget cap, and optional jitter. Retry eligibility is gated at the transport layer (kTransient only; kFatal/kInvalidArg fail-fast). Per-request retry-count tracking is thread-safe with explicit reset semantics. Evidence: 16 deterministic WSR tests in `tests/server/test_server_phase5_hardening.cpp`.
- **P5-S02 HTTP timeout and graceful-shutdown drain** — per-request deadline enforcement (kTimedOut on deadline overrun); server lifecycle state machine (kRunning → kDraining → kStopped) with explicit in-flight drain before stop; idle-connection and keepalive-timeout recycling. Evidence: 12 deterministic HST tests in `tests/server/test_server_phase5_hardening.cpp`.

These patterns are covered by 8 benchmark release gates SVR-01..SVR-08 in `benchmarks/server/bench_server_hotpaths.cpp`.

## Sourcecode Verification (Module: server/architecture)

- Verified files:
  - src/server/http_server.cpp
  - src/server/api_gateway.cpp
  - src/server/auth_middleware.cpp
  - src/server/rate_limiting_middleware.cpp
  - src/server/load_shedder.cpp
  - src/server/chunked_response_writer.cpp
  - src/server/websocket_session.cpp
  - src/server/mqtt_session.cpp
  - src/server/postgres_session.cpp
  - src/server/themis_core_grpc_service.cpp
- Verified architecture claims:
  - request lifecycle, endpoint, and session/protocol plane split
  - explicit middleware and overload boundaries
  - module-local ownership of client-facing server runtime behavior