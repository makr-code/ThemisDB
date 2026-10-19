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

# Server Research Notes

## Scope

This note is grounded in the runtime behavior in `src/server/http_server.cpp`, `src/server/api_gateway.cpp`, `src/server/auth_middleware.cpp`, `src/server/rate_limiting_middleware.cpp`, `src/server/load_shedder.cpp`, and the protocol/session files under the same module. It explains why the server layer exists in ThemisDB and which trade-offs are intentional at the client-facing boundary.

## Problem statement

The server module is not a generic HTTP wrapper. It is ThemisDB's production boundary for request ingress, authentication, rate control, overload protection, routing, protocol sessions, and API behavior. In practice, the module is responsible for turning inbound traffic into controlled execution decisions and explicit error responses instead of silently passing failure through unbounded internal systems.

The source evidence makes that explicit:

- `APIGateway::handleRequest()` normalizes versioned paths before routing and rejects requests early on auth, rate-limit, or load-shedding failure.
- `AuthMiddleware` supports JWT, Kerberos, mTLS, API-key, and USB-admin flows with fail-closed enforcement.
- `RateLimitingMiddleware` chooses per-path token-bucket limits and returns explicit retry metadata when a client exceeds capacity.
- `LoadShedder` computes a weighted load score from CPU, memory, and queue depth and rejects low-priority work before the system saturates.
- Protocol/session code (`websocket_session.cpp`, `mqtt_session.cpp`, `postgres_session.cpp`, `themis_core_grpc_service.cpp`) owns connection lifecycle and stream-specific state without broadening storage or query responsibilities.

## Research questions

### 1. What is the operational contract of the server module?

The module guarantees a bounded runtime contract: request validation, auth, quota enforcement, version-aware routing, and explicit error semantics. It is a boundary layer between external access and the rest of ThemisDB, not an implementation of storage or query logic itself.

### 2. Why is fail-closed behavior intentional?

The server code is deliberately strict on invalid or unavailable dependencies. Auth failures return 401/403; overload returns 429 or 503; absent shard routing can fall back to local execution only when the request is still valid; future linkages are designed to surface explicit operator signals rather than silent degraded behavior.

### 3. Which runtime surfaces matter most?

The main performance and reliability surfaces are:

- HTTP ingress and request normalization
- gateway route selection and version stripping
- middleware-based validation and per-client quotas
- queue and CPU-based overload shedding
- session management for WebSocket/MQTT/PostgreSQL/gRPC transports

## Assumptions and constraints

- The server is the first line of trust and the last place to fail closed before internal modules are reached.
- Authentication, quota enforcement, and overload control must remain explicit under concurrent request pressure.
- Session state is protocol-local; shared server modules manage cross-request state, not deep storage semantics.
- Operationally, the request lifecycle must remain explainable to operators through HTTP status codes, logs, and runtime metrics.

## Evidence from implementation

The architecture is consistent with the following implementation patterns:

- `api_gateway.cpp` builds a normalized request and dispatches to local, shard, or scatter-gather execution paths.
- `auth_middleware.cpp` maintains a mutex-protected auth configuration and validates tokens or credentials before a request reaches business logic.
- `rate_limiting_middleware.cpp` keeps token-bucket limits configurable per client and endpoint, with an override lookup that prefers the most specific matching prefix.
- `load_shedder.cpp` uses weighted load factors to reject lower-priority requests before the queue saturates.
- `http_server.cpp` composes the broader runtime, including request lifecycle wrappers, stream-entry services, and API handlers that sit above the storage/query stack.

## Evaluation criteria

The server module is viable when the following are true:

- invalid credentials fail immediately and precisely,
- overload is explicitly signaled rather than hidden,
- throughput remains bounded under mixed protocol traffic,
- protocol session failures remain isolated to transport/session lifecycle,
- API handlers remain behind explicit access and validation gates.

## Trade-offs

### Safety vs. permissive delivery

The server layer prefers explicit denial over magical recovery. That means 401/403/429/503 responses are not just error handling; they are formal operational contracts for safe degradation.

### Isolation vs. centralization

The module centralizes access control and request dispatch without taking over query or storage semantics. This keeps the contract narrow and makes the runtime understandable to maintainers, operators, and test authors.

### Throughput vs. fairness

Rate limiting and load shedding reduce peak throughput under duress. The trade-off is intentional: bounded admission is preferable to unbounded backlog or tail-latency collapse.

## Conclusion

The server module is best understood as a production safety boundary. It turns external traffic into controlled, observable, and fail-closed decisions. That boundary is the essential reason ThemisDB can keep query/storage internals reliable even when request load, invalid clients, or partial outages appear at the edges.
