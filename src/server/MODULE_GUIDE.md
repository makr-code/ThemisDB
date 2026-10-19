---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-19"
Status: "active"
AI_Used: true
AI_Models: "MAI-Code-1.1-Flash"
AI_Contributor: "MAI-Code-1.1-Flash"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Server Module Guide

## Who this module is for

This module is for developers working on runtime admission, API exposure, or connection-aware integration boundaries in ThemisDB. It is not the place to add deep storage logic or query semantics that belong elsewhere.

## Module responsibilities

The server module owns the client-facing runtime boundary. It is responsible for:

- HTTP and protocol transport serving,
- authentication and authorization policy,
- request routing and version normalization,
- admission control and overload handling,
- endpoint execution and response formatting,
- protocol/session health and teardown.

## Key files

- `src/server/http_server.cpp` — main server lifecycle and route composition
- `src/server/api_gateway.cpp` — request normalization, route target, and dispatch
- `src/server/auth_middleware.cpp` — auth mechanism selection and validation
- `src/server/rate_limiting_middleware.cpp` — token-bucket enforcement
- `src/server/load_shedder.cpp` — load-based rejection logic
- `src/server/websocket_session.cpp` — WebSocket lifecycle
- `src/server/mqtt_session.cpp` — MQTT lifecycle
- `src/server/postgres_session.cpp` — PostgreSQL wire protocol handling
- `src/server/themis_core_grpc_service.cpp` — gRPC service entry point

## Working conventions

### 1. Keep the gateway thin

The gateway should decide whether to route a request locally or to a distributed target. It should not become a dumping ground for storage or business logic.

### 2. Keep all request checks explicit

When editing request paths, ensure that auth, rate-limit, and load-shed logic remain visible and intentional. Silent bypasses or hidden fallback behavior should be treated as a correctness problem.

### 3. Prefer fail-closed semantics

A server change should prefer a clear denial or timeout over a vague success path. Rejected requests are part of the contract and should be implemented as such.

### 4. Keep protocol lifecycle local

Session and transport behavior belongs near the server layer, but deep entity semantics should remain in the relevant modules.

## Development checklist

Before landing a server change, verify:

- auth and route ordering still match the server contract,
- rate limit and load shedding remain explicit,
- versioned request targets still normalize correctly,
- new handlers do not bypass access gates,
- transport session teardown remains clean,
- tests cover the request path being changed.

## Testing guidance

Target the server tests closest to the behavior being modified, especially tests around:

- runtime contract hardening,
- protocol retry and timeout semantics,
- auth and authorization behavior,
- overload and rate-limit rejection.

## Summary

The server module should be easy to reason about: a runtime boundary with explicit policy, clear failure modes, and bounded state. If a change makes routing or auth less clear, it is likely in the wrong layer.
