---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: module-behavior
Canonical references: `include/api/`, `src/api/`, `tests/api/CMakeLists.txt`, `include/api/api_transport_contracts.h`
Related issues: #7093, #7167

# API module guide

## Audience and ownership

For maintainers changing HTTP-facing transport policy, GraphQL/gRPC/WebSocket adapters, or telemetry export. The API module owns transport adaptation; server routing and endpoint policy are maintained in `src/server/`.

## Choose the relevant boundary

- Shared version/size/failure policy: `api_transport_contracts.h`, `api_transport_policy.h`.
- Protocol-specific behavior: use the matching file under `include/api/` and `src/api/`.
- gRPC lifecycle: `grpc_server.h` / `grpc_server.cpp`.
- WebSocket change-stream validation: `ws_handler.h` / `ws_handler.cpp`.

## Maintainer workflow

1. Check whether the behavior is shared policy or adapter-specific.
2. Verify bounds, capability declarations, return types, and failure class in the public header.
3. Exercise invalid/oversized input and optional-feature absence, not only the success path.
4. Add or run a focused test under `tests/api/`; transport hardening coverage is in `test_api_transport_hardening.cpp`.
5. Update `API.md` for changed client-visible behavior and `IMPLEMENTATION.md` for changed internal flow.

See `ARCHITECTURE.md`, `RUNBOOK.md`, and `SECURITY.md` for wider boundaries.
