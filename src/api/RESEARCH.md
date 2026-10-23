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

# API Module Research Notes

## Purpose

This note records the engineering assumptions behind the `api` module as it is implemented in `src/api` and in the public contracts under `include/api`. The module is not a business-logic layer; it is the protocol and transport boundary between external clients and ThemisDB runtime services.

## Evidence base

The analysis below is grounded in the following source files:

- `include/api/graphql.h` — public GraphQL AST and execution contracts
- `include/api/api_transport_contracts.h` — shared transport policy contract
- `include/api/api_transport_policy.h` — fail-closed request admission middleware
- `include/api/otlp_exporter.h` — tracing export/telemetry contract
- `src/api/graphql.cpp` — GraphQL parser and validation behavior
- `src/api/grpc_server.cpp` — gRPC lifecycle and configuration enforcement
- `src/api/ws_handler.cpp` — WebSocket CDC validation and auth gate
- `src/api/tracing_middleware.cpp` — request correlation and span propagation
- `src/api/otlp_exporter.cpp` — exporter queueing, backpressure, and drop behavior

## Research questions

### 1. What is the core runtime role of the module?

The module owns protocol adapters and runtime policy enforcement for GraphQL, gRPC, WebSocket, and tracing export. This is visible in `README.md` and `ARCHITECTURE.md`, and is also reflected in the implementation: the GraphQL parser validates query structure, `GrpcApiServer` owns the gRPC sockets and service registration, `WsChangeHandler` validates change-stream authorization and CDC query parameters, and the OTLP exporter emits diagnostics to external telemetry backends.

### 2. Which invariants are intentionally enforced?

The code emphasizes deterministic validation and explicit failure semantics rather than permissive fallback.

- Query parsing fails closed when the document is empty, malformed, or exceeds size, depth, field, or AST limits (`src/api/graphql.cpp`).
- gRPC startup is rejected if the bind address or port is invalid, if no service is registered, or if TLS requirements are incomplete (`src/api/grpc_server.cpp`).
- WebSocket CDC requests are rejected unless the path is valid and the `cdc:subscribe` authorization scope is accepted (`src/api/ws_handler.cpp`).
- OTLP export cannot silently hide backpressure or queue exhaustion; it prefers bounded degradation and explicit warning/error counts (`src/api/otlp_exporter.cpp`).

### 3. Why is the transport layer separated from storage/query logic?

The implementation keeps the API layer thin and adapter-like. It validates and normalizes inbound protocol traffic but does not absorb query execution or storage ownership. This separation reduces the likelihood that protocol bugs leak into query or persistence semantics. It also makes it easier to gate optional features by deployment profile while retaining a stable public contract.

## Assumptions and constraints

- The API module must work across more than one protocol surface without allowing one transport failure to change another transport's semantics.
- Telemetry export is valuable, but must be bounded and non-blocking to the request path.
- Malformed requests must be rejected before downstream handlers or services are invoked.
- The transport layer must remain explainable to operators under overload, invalid auth, or unsupported capability conditions.

## Trade-offs

### Fail-closed validation

The GraphQL and WebSocket code intentionally rejects malformed or unauthorized requests early. This increases the chance of returning explicit protocol errors, but it protects the system from ambiguous or hidden behavior.

### Bounded exporter behavior

The OTLP exporter is designed to maintain a bounded queue and degrade predictably under pressure. This means some spans may be dropped rather than queueing indefinitely, which is a deliberate trade-off to preserve service responsiveness.

### Capability-based deployment

The module supports optional protocol and telemetry capability toggles. This improves compatibility with different deployment profiles, but it requires careful operator configuration and error reporting so that disabled features are not mistaken for platform failures.

## Validation path

The engineering design is validated by the module's existing tests and benchmarks, especially the API transport hardening and release-gate checks tracked in `src/api/ROADMAP.md` and `src/api/FUTURE_ENHANCEMENTS.md`. The code also exposes explicit error taxonomies and status mapping in `include/api/api_error_taxonomy.h` and `include/api/api_transport_contracts.h`, which makes the runtime behavior reviewable.

## Operational relevance

This module is security-relevant because it is one of the first points where external traffic enters the system. The long-term risk is not just protocol correctness; it is silent request acceptance, rogue subscription creation, or hidden backpressure that makes the system seem healthy while it is actually degraded. The design explicitly counters these failure modes by keeping transport validation strict and audit-friendly.
