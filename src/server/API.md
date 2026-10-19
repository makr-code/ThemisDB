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

# Server API and Contract Notes

## Purpose

This document captures the public contract of the `server` module as implemented in the runtime gateway, headers, middleware, and endpoint surfaces. It is intentionally focused on behavior that a caller or operator can infer from the code and runtime responses.

## Primary API surfaces

### Gateway surface

The `APIGateway` contract governs how inbound requests are normalized, validated, and dispatched. The major responsibilities are:

- verification of auth state,
- rate-limit and load-shed admission,
- route target selection,
- local or sharded dispatch,
- response construction for rejection and success.

### Middleware surfaces

The middleware stack defines the server's admission policy:

- `AuthMiddleware` validates credentials and roles,
- `RateLimitingMiddleware` applies per-client and per-path quota enforcement,
- `LoadShedder` blocks degraded or overloaded traffic, often in a priority-aware way.

### Session and protocol surfaces

The server provides protocol entry points for:

- HTTP request handling,
- WebSocket session management,
- MQTT consumer/session handling,
- PostgreSQL wire sessions,
- gRPC service exposure.

These are boundaries for transport semantics, not storage backends.

## Failure contract

The server API uses explicit request-level failure semantics, not silent degradation:

- 401 Unauthorized: authentication rejected or missing
- 403 Forbidden: authenticated client lacks required scope or policy rights
- 429 Too Many Requests: rate limit exceeded
- 503 Service Unavailable: overloaded or backend unavailable
- 504 Gateway Timeout: request exceeded deadline or server-side timeout policy

These codes are integral to the module's trust model and should stay consistent when changing server behavior.

## Versioning and routing behavior

The API gateway strips version prefixes from target paths for downstream routing, while keeping the original request available for validation and tracing. This allows the module to support versioned public URLs without forcing every downstream endpoint to understand route prefix semantics.

## Endpoint organization

Endpoint handlers in the server module are intended to wrap concrete ThemisDB operations, including but not limited to:

- admin and maintenance endpoints,
- entity, query, and schema endpoints,
- vector, graph, and timeseries execution,
- LLM, prompt, and model management,
- health and error surfaces.

The important invariant is that these endpoint handlers remain behind the server's admission and authorization gates.

## Threading and concurrency contract

The server layer's concurrency model is based on bounded, explicit state: mutexes protect mutable configuration and connection state, while atomic metrics and counters support load and quota tracking. This keeps the runtime safe under concurrent client traffic without conflating request state with deep database execution state.

## Edge-case expectations

- Requests with invalid or malformed metadata should be rejected before entering a handler.
- Overload conditions should be signaled to the client immediately rather than hidden through delayed processing.
- Disconnections and session teardown must not create stale, shared runtime state.
- Versioned or sharded routing should remain deterministic and explainable.

## Summary

The server API contract is intentionally explicit: clients get predictable responses, operators get clear failure modes, and internal modules remain protected from invalid or overloaded external traffic.
