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

# API Module Modeling Notes

## Module model

The `api` module is modeled as a protocol adaptation layer. Its public contracts live under `include/api`, while the runtime enforcement and protocol lifecycles live under `src/api`.

### Structural model

1. GraphQL plane
   - `graphql.h` and `graphql.cpp` define the query representation, parser, AST model, and validation limits.
   - The parser enforces size, depth, field, and AST count checks before accepting the operation.

2. gRPC plane
   - `grpc_server.h` and `grpc_server.cpp` own the listener lifecycle, service registration, credential building, and bounded message sizing.
   - The server rejects invalid startup state before binding a port.

3. WebSocket and CDC plane
   - `ws_handler.h` and `ws_handler.cpp` validate the path and authorization context for `/v2/changes` and `/v2/cdc/stream`.
   - Validated parameters include the CDC filter state and sequence start point.

4. Observability plane
   - `tracing_middleware.cpp` and `otlp_exporter.cpp` handle correlated request tracing and export of spans with bounded queue behavior.

## Runtime data flow

A request enters through an adapter-specific entry point and then flows through the following stages:

1. transport validation
2. protocol-specific normalization
3. authorization or capability checks
4. business dispatch or stream setup
5. response serialization and observability emission

This can be seen in the middleware and server flows: `TransportPolicyMiddleware` checks protocol-level constraints before the inner handler is called, and the WebSocket handler validates both auth and query-string filtering before upgrading the connection.

## Primary contract objects

### Transport policy contract

`api_transport_contracts.h` and `api_transport_policy.h` model the relationship between transport capability, request metadata, and failure class. The central idea is that protocol misuse should map to a deterministic failure class, not a fuzzy runtime error.

### GraphQL AST and execution model

`graphql.h` models GraphQL values, fields, variables, and operations with a driven set of enum types and selection objects. The parser tries to keep resource usage bounded and explicit in its validation path.

### Service lifecycle model

`GrpcApiServer` is modeled as a stateful lifecycle component with initialization, service registration, startup, shutdown, and optional TLS configuration. State transitions are guarded so that the server cannot enter a conflicting lifecycle state.

## Failure model

The API layer treats faults as explicit outcomes:

- malformed or untrusted input is rejected
- authorization failures return an explicit unauthorized signal
- unsupported protocol capabilities produce a defined failure class
- exporter pressure causes bounded drop or warning rather than unbounded blocking

This is important because the API layer sits at the trust boundary. Its modeling goal is to keep external failures visible, not silent.

## Design invariants

- Requests are admitted only if they satisfy basic contract constraints.
- Transport state is scoped to the connection or request, never merged with unrelated runtime state.
- Failures are represented in a structured and operator-readable way.
- Optional transports are allowed to degrade, but not by hiding a valid failure behind a silent fallback.

## Model quality and reviewability

The design is source-grounded and reviewable because each main behavior is backed by a small, concrete implementation and a corresponding public contract. This makes the model stable enough for operator dashboards, audit review, and release-gate verification.
