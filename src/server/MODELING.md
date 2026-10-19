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

# Server Runtime Modeling

## Purpose

This document models the `server` module as a request-handling system that sits between client connections and ThemisDB's internal execution modules. The operating model is visible across the module's routing, auth, restraint, and session services.

## Layered model

### 1. Ingress and transport layer

The transport/runtime boundary is represented by:

- `src/server/http_server.cpp`
- `src/server/websocket_session.cpp`
- `src/server/mqtt_session.cpp`
- `src/server/postgres_session.cpp`
- `src/server/themis_core_grpc_service.cpp`

These components own protocol lifecycle, connection management, and worker/scheduler-mediated request handling. They do not own long-lived data semantics; they accept a connection and hand off to the request execution flow.

### 2. Gateway and policy layer

The gateway layer is represented by:

- `src/server/api_gateway.cpp`
- `src/server/auth_middleware.cpp`
- `src/server/rate_limiting_middleware.cpp`
- `src/server/load_shedder.cpp`

This layer controls:

- request normalization and API version stripping,
- auth validation and role/scope checks,
- quota enforcement and retry metadata,
- load-driven admission decisions.

### 3. Endpoint and service layer

The endpoint layer is carried by files such as:

- `admin_api_handler.cpp`
- `entity_api_handler.cpp`
- `query_api_handler.cpp`
- `graph_api_handler.cpp`
- `vector_api_handler.cpp`
- `timeseries_api_handler.cpp`
- `llm_api_handler.cpp`

These handlers do not define the whole server; they materialize the API surface that the gateway exposes. A request can be routed locally or distributed based on call path, but the contract remains a server responsibility.

## Request flow model

The source code suggests the following flow:

1. a client establishes a transport session,
2. the HTTP or protocol runtime receives the request,
3. the server gateway validates auth and rate limits,
4. overload checks and route normalization occur,
5. the request is dispatched to a local or sharded target,
6. the endpoint/service handler executes implementation logic,
7. the response is marshaled, chunked, or streamed back to the client.

A simplified model is:

Client -> Transport -> Gateway/Auth/Quota -> Route Decision -> Handler -> Response

## State ownership model

### Request-local state

Request-local state is intentionally small and short-lived. It includes:

- headers, versions, and normalized path,
- auth results and rate-limit checks,
- request identifiers for tracing and response metadata.

### Session-local state

Session objects maintain connection lifecycle and transport conditions. They are scoped to a protocol connection and should not become a global data store.

### Shared server state

Shared server state is limited to runtime configuration, route metadata, limiter state, and connection/session registry information. The code already reflects this by locking configuration via `config_mutex_` and using atomic counters for load and limiter metrics.

## Failure domains

The server module acknowledges several distinct failure domains:

### Authentication failure domain

Auth failures are explicit and should terminate as 401 or 403. They are not recoverable by local fallback logic.

### Admission failure domain

Rate-limits and overload shedding produce 429 or 503 responses. This is intentional and forms the admission policy for the system.

### Routing failure domain

The gateway may fall back to local execution when a sharding signal cannot be resolved, but it does so only after the request is still valid. It preserves the request contract rather than silently dropping semantics.

### Session failure domain

Protocol-level disconnects or teardown conditions should only affect the active session. They should not cascade into unrelated server state.

## Operational invariants

- Request normalization happens before final route selection.
- Authentication is checked before dispatching business-level logic.
- Rate limiting and load shedding are explicit policy decisions, not hidden delays.
- Counter and state updates remain bounded by the module’s mutex/atomic patterns.
- Streaming and chunked responses remain under the server’s response manager rather than deep internal modules.

## Why this model matters

This model is the source of the module’s reliability. By separating ingress, policy, routing, and protocol state, the server layer prevents the rest of ThemisDB from needing to reason about external client behavior directly. That separation keeps the runtime explainable and makes failures diagnosable.
