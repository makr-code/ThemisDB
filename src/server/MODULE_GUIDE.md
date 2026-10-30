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
Canonical references: `include/server/api_gateway.h`, `include/server/auth_middleware.h`, `src/server/api_gateway.cpp`, `tests/server/CMakeLists.txt`
Related issues: #7093, #7167

# Server module guide

## Audience and ownership

For maintainers changing gateway routing, auth/admission, HTTP handling, protocol sessions, or endpoint handlers. Do not place API transport-adapter or storage-engine behavior here unless the server owns that integration.

## Navigation

- Gateway/config: `api_gateway.h`, `src/server/api_gateway.cpp`.
- Authorization setup/results: `auth_middleware.h`, `src/server/auth_middleware.cpp`.
- Other handlers/sessions: match the header and source with the affected endpoint/protocol.
- Related transports: `src/api/`; broader system context: `ARCHITECTURE.md` and `RUNBOOK.md`.

## Maintainer workflow

1. Trace the request from gateway entry to the specific handler and back to the response.
2. Verify feature flags and injected dependencies before documenting or changing behavior.
3. Keep denial, timeout, overload, and dispatch failure paths distinguishable.
4. Add a focused test under `tests/server/`; use `test_api_gateway.cpp` for route-level changes.
5. Run the corresponding target from `tests/server/CMakeLists.txt` and update `API.md` for caller-visible changes.

See `IMPLEMENTATION.md` for runtime boundaries and `SECURITY.md` for trust-related material.
