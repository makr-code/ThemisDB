---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-19"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "unknown"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Server Implementation Guide

## Architectural implementation map

The `server` module is organized around runtime contracts rather than a single monolithic service. The implementation is split across a small number of core responsibilities:

- `http_server.cpp`: top-level HTTP lifecycle, request composition, and integration of handler surfaces
- `api_gateway.cpp`: request normalization, route target selection, and dispatch policy
- `auth_middleware.cpp`: auth system selection and validation flow
- `rate_limiting_middleware.cpp`: token-bucket enforcement and endpoint override matching
- `load_shedder.cpp`: weighted overload rejection based on CPU, memory, and queue depth
- `websocket_session.cpp`, `mqtt_session.cpp`, `postgres_session.cpp`, `themis_core_grpc_service.cpp`: protocol/session lifecycle management
- handler files under `src/server/`: per-endpoint runtime behavior and outbound service calls

## Core implementation invariants

### 1. Request normalization precedes routing

`APIGateway::handleRequest()` strips API version prefixes before route resolution. The original request remains available for auth and version-checking, but normalized paths are used for downstream route decisions. This prevents route mismatch and reduces drift between versioned public URLs and internal routing names.

### 2. Authentication is a hard gate

`AuthMiddleware` is expected to fail closed. If a configured auth mechanism is enabled but cannot validate a valid token or credential, the request is rejected before layer-specific business logic runs. This contracts the server's trust boundary and keeps invalid callers out.

### 3. Admission control is explicit and bounded

`RateLimitingMiddleware` and `LoadShedder` decide whether a request is permitted. They reject work before it saturates the system. The policy is not hidden behind downstream retries or silent queueing.

### 4. Session ownership remains local

Session objects own protocol lifecycle state, connection metadata, and transport-specific handling. They do not expose a large set of persistence responsibilities. This keeps session behavior understandable and avoids mixing transport state with storage logic.

## Important edge cases

### Missing or invalid credentials

Requests without a valid identity or token should never be forwarded to business handlers when auth is required. The middleware layers exist specifically to stop this from happening.

### Versioned routes

API version prefixes are normalized to keep downstream route matching constant. The gateway is expected to handle this consistently for both request parsing and evidence logging.

### Overload conditions

The server intentionally sheds lower-priority load before queue growth becomes pathological. The `LoadShedder` is designed to degrade predictably when CPU, memory, or queue load exceed safe thresholds.

### Protocol disconnects

A disconnected WebSocket or gRPC stream should invalidate session state and drop work without corrupting unrelated request objects or shared connection tables.

## Local implementation conventions

- Prefer small, explicit policy checks over broad catch-all behavior.
- Keep route-dispatch logic centralized in the gateway.
- Keep auth logic in dedicated middleware, not in endpoint-specific code.
- Preserve deterministic error motivations: 401, 403, 429, 503 are all explicit transitions with operational meaning.
- Preserve request and connection invariants around lifetime, version, and teardown.

## Validation expectations

Implementation work in this module should validate the following behaviors:

- route normalization remains stable across versioned paths,
- invalid tokens are rejected without partial execution,
- rate-limit overrides match the most specific prefix,
- overload shedding rejects low-priority work when thresholds are crossed,
- protocol teardown does not leave stale session state behind,
- multi-tenant or multi-protocol traffic preserves bounded operational semantics.

## Summary

The server implementation is a boundary-heavy design: it manages trust, compliance, and access at the edges while keeping the rest of ThemisDB focused on state and execution. The critical value is not in elegance alone; it is in predictable failure and bounded runtime behavior under load.
