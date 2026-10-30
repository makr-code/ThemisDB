---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
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
Canonical references: `include/server/api_gateway.h`, `include/server/auth_middleware.h`, `src/server/api_gateway.cpp`, `src/server/auth_middleware.cpp`, `tests/server/test_api_gateway.cpp`
Related issues: #7093, #7167

# Server implementation

## Purpose and boundary

For developers changing server-side request admission, gateway routing, or authentication configuration. This module owns server request/session behavior; transport adapters in `src/api/` and persistence/query internals are separate modules.

## Implementation map

- `api_gateway.cpp` implements `APIGateway::handleRequest()`, route selection, local/remote dispatch, version handling, and configured admission controls.
- `auth_middleware.cpp` implements credential setup and `authorize()` paths; `AuthResult` carries authorization, identity, and a reason.
- Protocol sessions and endpoint handlers have their own source/header pairs under `src/server/` and `include/server/`.

## Invariants and failure cases

- Gateway behavior depends on explicit `Config` switches and injected auth, rate-limiter, load-shedder, and optional router dependencies; do not assume every feature is enabled.
- Keep route/version handling and authentication decisions visible at the gateway boundary.
- A rejected authorization result is distinct from an allowed result; preserve its reason for the applicable caller/audit path.
- Request deadlines and configured concurrency/admission limits are part of the gateway config; test limit and failure paths when changing dispatch.

## Change and validation steps

1. Read the affected `include/server/` declaration and matching `src/server/` source.
2. Check construction/configuration and the exact `handleRequest()` route before changing handler behavior.
3. Use `tests/server/test_api_gateway.cpp` for gateway changes and `test_server_contract_hardening_focused.cpp` for contract edge cases.
4. Run the changed test target registered under `tests/server/CMakeLists.txt`.

See `API.md`, `MODULE_GUIDE.md`, `SECURITY.md`, and `RUNBOOK.md`.
