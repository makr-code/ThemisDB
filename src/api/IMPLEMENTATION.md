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

# API Module Implementation Guide

## Primary implementation map

The module's implementation is organized around protocol-specific entry points and shared policy enforcement.

### GraphQL

- `src/api/graphql.cpp` implements parsing, validation, and query-cache behavior.
- The parser checks query size, nesting depth, field count, and AST node count before accepting a document.
- Unsupported fragment definitions are explicitly rejected with a versioned parser error.

### gRPC

- `src/api/grpc_server.cpp` owns startup validation and registration state.
- The server validates port, host, TLS parameters, and message-size limits before binding.
- The lifecycle uses a mutex to serialize startup and shutdown while avoiding lock contention across the expensive socket bind path.

### WebSocket and CDC

- `src/api/ws_handler.cpp` validates the target path and optional auth token.
- CDC subscription requests are required to present the `cdc:subscribe` authorization scope.
- Query-string parser logic accepts `from_sequence` and `key_prefix`, and invalid values fall back to safe defaults rather than causing ambiguous stream behavior.

### Tracing and telemetry

- `src/api/tracing_middleware.cpp` adds correlation data and request context propagation.
- `src/api/otlp_exporter.cpp` owns OTLP export, queueing, flush semantics, and bounded degradation under pressure.

## Core invariants

1. Fail closed on invalid protocol input.
2. Keep resource limits explicit and bounded.
3. Do not allow an optional capability to mask a transport error.
4. Preserve clear operator-visible diagnostics in the failure path.
5. Keep request-state and transport state local to the request or connection.

## Edge cases handled by implementation

- empty or whitespace-only GraphQL document
- GraphQL depth or field-count violations
- invalid gRPC port or empty host
- gRPC server with no registered services
- unauthorized WebSocket CDC subscription
- malformed sequence or key prefix query values
- exporter queue pressure or flush failure

## Validation expectations

Implementation work in this module should validate:

- parser boundaries for empty and invalid documents
- gRPC startup with invalid or partially configured values
- WebSocket access denial and valid subscription flow
- OTLP exporter queue and retry behavior under overload
- transport policy rejection logic for malformed or unsupported requests

## Practical guidance

When modifying the API layer, keep changes small and explicit. The module's value is not complexity; it is predictable transport behavior. Prefer design that makes invalid requests fail clearly and makes optional components degrade in a traceable way.
