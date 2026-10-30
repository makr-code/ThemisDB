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
Canonical references: `include/api/api_transport_contracts.h`, `include/api/api_transport_policy.h`, `include/api/grpc_server.h`, `src/api/grpc_server.cpp`, `src/api/graphql.cpp`, `src/api/ws_handler.cpp`, `tests/api/test_api_transport_hardening.cpp`
Related issues: #7093, #7167

# API transport implementation

## Purpose and boundary

For developers changing transport adapters and shared request policy in `src/api/`. Server-side request routing/admission belongs to `src/server/`; this module supplies transport-facing contracts and adapters.

## Implementation map

- `api_transport_contracts.h` defines versions, payload/path/header bounds, capability flags, and failure classes.
- `api_transport_policy.h` implements a shared HTTP-handler decorator with configurable bounds and transport validation.
- `grpc_server.cpp`, `graphql.cpp`, and `ws_handler.cpp` implement protocol-specific paths; tracing/export behavior is in `tracing_middleware.cpp` and `otlp_exporter.cpp`.

## Invariants and failure cases

- The shared contract lists v1/v2 and a 10 MiB default maximum payload; the policy config is normalized so its configured payload/path bounds cannot exceed the contract maxima.
- Preserve explicit malformed-request, oversized-payload, unsupported-version, missing/mismatched-content-type, unauthorized, rate-limit, and unavailable-capability classifications.
- Transport-specific optional features must not be documented as universal; check the adapter’s capabilities and compile/configuration guards.
- gRPC initialization/start return boolean success; propagate failure rather than assuming a bound server.

## Change and validation steps

1. Change the shared contract only when all affected adapters and callers are understood.
2. Keep protocol-specific parsing and shared policy boundaries separate.
3. Use `tests/api/test_api_transport_hardening.cpp`, `test_api_grpc_server.cpp`, or the closest adapter test.
4. Run the affected target registered by `tests/api/CMakeLists.txt`.

See `API.md`, `MODULE_GUIDE.md`, and `SECURITY.md`.
