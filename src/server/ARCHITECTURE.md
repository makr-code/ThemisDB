# Architecture - Server Module

<!-- Status: current | validated: 2026-05-31 -->
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