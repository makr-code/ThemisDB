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

# API Module Developer Guide

## Purpose

This guide is the entry point for working on the `api` module. It connects the public contract, implementation files, and operational expectations without repeating the full architecture or roadmap docs.

## Recommended reading order

1. `README.md` — module purpose and interface summary
2. `ARCHITECTURE.md` — boundaries, execution planes, and failure domains
3. `API.md` — public contract expectations and failure semantics
4. `IMPLEMENTATION.md` — implementation map and key invariants
5. `ROADMAP.md` and `FUTURE_ENHANCEMENTS.md` — active status and pending hardening work
6. `RUNBOOK.md` and `OPERATIONS.md` — diagnosis, operator action, and recovery flows
7. `TRUST.md` — security posture and boundary assumptions

## Ownership map

- Public API: `include/api/`
- Internal implementation: `src/api/`
- Related runtime integration: `src/server/`, `src/observability/`, and entry points under `src/main*`

## Key subsystem entry points

### GraphQL

- `include/api/graphql.h`
- `src/api/graphql.cpp`
- `include/api/graphql_aql_resolver.h`

### gRPC

- `include/api/grpc_server.h`
- `src/api/grpc_server.cpp`
- `src/api/themisdb_grpc_service.cpp`

### WebSocket / CDC

- `include/api/ws_handler.h`
- `src/api/ws_handler.cpp`
- `src/api/graphql_ws_handler.cpp`

### Export and tracing

- `include/api/tracing_middleware.h`
- `src/api/tracing_middleware.cpp`
- `include/api/otlp_exporter.h`
- `src/api/otlp_exporter.cpp`

## Change checklist

Before merging a transport or API change, confirm:

- malformed request input is rejected explicitly
- auth and subscription checks remain in the transport layer
- optional capabilities degrade predictably and remain observable
- resource limits are still enforced and documented
- tests or benchmarking updates cover the changed behavior

## Known quality bar

The module is production-facing and boundary-heavy. A patch is not considered complete if it only changes syntax or improves local readability without preserving the API contract and observable failure semantics.
