# Security - Geo Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in geo focuses on safe geometry input handling, controlled backend fallback behavior, deterministic query execution boundaries, and explicit failure behavior under unsupported or invalid geospatial conditions.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| malformed or adversarial geometry payloads | geometry parsing/validation and structured rejection |
| unsafe GPU execution path assumptions | capability checks with explicit CPU fallback behavior |
| excessive resource pressure in spatial workflows | bounded execution paths and controlled feature gates |
| hidden failure paths in advanced geo operations | explicit error reporting and runtime observability surfaces |

## Implemented Security Controls

- geometry inputs are validated before critical execution paths.
- backend selection is capability-aware with deterministic fallback behavior.
- unsupported feature paths return explicit non-silent outcomes.
- runtime diagnostics support production incident triage.

## Security Follow-ups

- continue hardening geometry validation edge and recursion scenarios.
- tighten diagnostics for high-load GPU fallback and degraded-path execution.
- expand stress coverage for large and complex geospatial workloads.

## Sourcecode Verification (Module: geo/security)

- Verified files:
  - src/geo/geo_json_geometry.cpp
  - src/geo/cpu_backend.cpp
  - src/geo/gpu_backend_stub.cpp
  - src/geo/gpu_backend_cuda.cu
  - src/geo/gpu_backend_hip.cpp
  - src/geo/device_detector.cpp
- Verified controls:
  - input validation and explicit geometry error handling
  - deterministic capability-aware backend fallback behavior
  - explicit unsupported/degraded path signaling