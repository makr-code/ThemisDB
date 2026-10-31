---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Security - API Module

<!-- Status: review | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the API module focuses on transport boundary enforcement, protocol-surface hardening, and observability-safe behavior across GraphQL/gRPC/WebSocket pathways.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| unauthenticated or unauthorized transport access | `WsChangeHandler::validate()` performs CDC authentication/authorization checks before returning an upgrade decision; other routes rely on their configured server middleware |
| protocol-level resource exhaustion | GraphQL query limits and gRPC message-size bounds are explicit; shared HTTP policy limits apply only where `TransportPolicyMiddleware` is installed |
| malformed request payload abuse | GraphQL parsing and transport policy return explicit failures for their covered input paths |
| trace/metadata misuse | tracing middleware manages correlation/span context; OTLP export is separate from request success |
| CDC/WebSocket stream abuse | CDC path/scope validation occurs in `WsChangeHandler`; GraphQL WebSocket message handling is a separate protocol surface |

## Implemented Security Controls

- `TransportPolicyMiddleware` rejects its configured policy violations before calling the inner handler; it is not an implicit global filter.
- GraphQL `QueryLimits` and transport error classes provide explicit validation/failure surfaces; schema authorization must be supplied by the integration.
- `WsChangeHandler` checks the CDC subscription authorization scope before the host server performs an upgrade.
- tracing/export surfaces are separate from request success; exporter failure is not an audit record or database-operation result.

## Security Follow-ups

- verify which production HTTP routes install `TransportPolicyMiddleware` and authentication middleware.
- verify the production request-enforcement call site for `QueryAllowList`; its `enabled_` flag does not affect `isAllowed()`.
- verify deployment-specific limits, logging, and alerting; source-level controls do not establish deployment configuration.

## Source review (2026-10-31)

- Verified files:
  - src/api/graphql.cpp
  - src/api/graphql_ws_handler.cpp
  - src/api/grpc_server.cpp
  - src/api/ws_handler.cpp
  - src/api/tracing_middleware.cpp
  - src/api/otlp_exporter.cpp
- Verified controls:
  - GraphQL parser limits and `QueryLimits` production mode
  - transport policy checks, where installed by a caller
  - CDC authorization validation and upgrade decision
  - tracing/export separation from request business flow
- This source review did not execute security tests, inspect deployment configuration, or constitute a security audit.