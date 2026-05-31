# Security - API Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the API module focuses on transport boundary enforcement, protocol-surface hardening, and observability-safe behavior across GraphQL/gRPC/WebSocket pathways.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| unauthenticated or unauthorized transport access | API transport paths integrate with centralized auth middleware layers |
| protocol-level resource exhaustion | connection and request processing are bounded by module and server guardrails |
| malformed request payload abuse | parser and adapter surfaces use structured validation/error behavior |
| trace/metadata misuse | tracing middleware isolates correlation semantics and controlled export paths |
| CDC/WebSocket stream abuse | dedicated ws handling and policy integration paths constrain stream behavior |

## Implemented Security Controls

- API module remains transport adapter focused and avoids embedding core business state mutation logic.
- protocol paths provide explicit structured failures on unsupported or invalid flows.
- tracing/export surfaces are isolated and configurable rather than mandatory for request success.

## Security Follow-ups

- continue endpoint and policy hardening for advanced protocol features.
- maintain bounded runtime behavior for high-concurrency transport scenarios.
- keep observability signals aligned with secure operational diagnostics.

## Sourcecode Verification (Module: api/security)

- Verified files:
  - src/api/graphql.cpp
  - src/api/graphql_ws_handler.cpp
  - src/api/grpc_server.cpp
  - src/api/ws_handler.cpp
  - src/api/tracing_middleware.cpp
  - src/api/otlp_exporter.cpp
- Verified controls:
  - protocol adapter separation
  - transport-specific handling surfaces
  - tracing/export separation from request business flow