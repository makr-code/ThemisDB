---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "MAI-Code-1.1-Flash"
AI_Contributor: "MAI-Code-1.1-Flash"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# API Module Embedding and Integration Notes

## Scope

The `api` module is embedded into ThemisDB as the externally-visible integration boundary. It exposes protocol entry points and delegates business execution to other subsystem modules while remaining responsible for validation and observability.

## Integration boundaries

### Upstream consumers

The API module is consumed by the server-side runtime and by protocol adaptation layers that register HTTP, gRPC, and WebSocket endpoints. In the repository, the module's interfaces are used by broader runtime components such as the HTTP server and the main server bootstrap.

### Downstream responsibilities

The API layer does not own storage logic or query execution. Instead, it coordinates transport concerns:

- GraphQL parsing and request validation
- gRPC service binding and lifecycle
- WebSocket channel auth and subscription request validation
- tracing context propagation and export
- optional geospatial and federation admin hooks

## Concrete embedding points

### GraphQL embedding

`include/api/graphql.h` provides the AST and execution model. The parser implementation in `src/api/graphql.cpp` enforces strict limits before a query is accepted. This keeps query parsing from getting into a runaway state when a hostile or malformed request arrives.

### gRPC embedding

`src/api/grpc_server.cpp` implements the service registration and startup state machine. It validates configuration before binding sockets and refuses invalid or empty startup state. This is important because the module must not start a listening service when the runtime cannot safely explain its inbound contract.

### WebSocket embedding

`src/api/ws_handler.cpp` validates the change-stream path, optional token, and required authorization scope before upgrading the connection. This is a classic boundary check: the transport layer ensures only valid CDC subscribers receive a stream.

### Telemetry embedding

`src/api/tracing_middleware.cpp` and `src/api/otlp_exporter.cpp` make observability part of the execution path without absorbing the business logic itself. These components attach request correlation and emit telemetry, but they keep their own failure semantics explicit and bounded.

## Why this embedding model matters

The API module is intentionally positioned as a boundary layer. It absorbs traffic variance and protocol complexity without becoming a domain implementation. That makes it easier to enforce consistent runtime semantics, share fail-closed validation rules across transports, and keep operational diagnostics aligned with the actual request path.

## Boundary and failure constraints

The integration contract assumes:

- invalid protocol state is rejected before service dispatch
- auth is validated before the stream or endpoint is opened
- observability export may degrade under pressure, but not by silently blocking traffic
- API-level hooks remain adapters, not business owners

This discipline keeps the rest of ThemisDB and its services from being forced to reason about raw transport-level variability.
