# Security - Geo Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security review for geo focuses on geometry inputs, request-policy boundaries,
backend capability/fallback behavior, and diagnostic data. The geo module is
not the authorization boundary; server integration and project-level policy
own access decisions.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| malformed or adversarial geometry payloads | path-specific parsing/validation and operation error/status handling; trace the concrete request path |
| unsafe GPU execution path assumptions | compile-time selection and runtime capability reporting; fallback is operation-specific |
| excessive resource pressure in spatial workflows | API/policy limits where consumed; verify each execution path rather than assuming global enforcement |
| sensitive data in diagnostics or reports | redact payloads and identifiers before export; diagnostic strings may be retained as supplied |
| unauthorized collection access | enforce at the owning server boundary; `SpatialQueryPolicy::allowed_collections` is advisory |

## Implemented Security Controls

- `GeoJSONGeometry` exposes validation; callers must invoke the relevant
  validator and must not assume every API path performs the same checks.
- Backend/device report interfaces are available; compiling a GPU backend does
  not establish runtime availability or GPU execution.
- `NoOpRasterQueryImpl` returns `RasterStatus::NOT_SUPPORTED`.
- The geo policy documentation places authoritative authorization at the
  server boundary and identifies the collection list as advisory.

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
  - public geometry validation declarations and their typed validation result
  - geo policy's advisory collection-list boundary
  - explicit raster unsupported status and backend/device report declarations

These are control surfaces, not proof of uniform enforcement in every geo
operation or deployment.

## Canonical source references

- GeoJSON validation types:
  `include/geo/geo_json_geometry.h`.
- Policy fields and validation:
  `include/geo/geo_policy.h` and `src/geo/geo_policy.cpp`.
- Raster result status:
  `include/geo/raster_query_interface.h`.
- Backend reporting:
  `include/geo/spatial_backend.h`.
- Trust boundary and authorization details:
  `src/geo/TRUST.md`.

---

**AI_Used:** true · **AI_Models:** unknown · **AI_Contributor:** GitHub Copilot coding agent · **Human_Review:** false · **Human_Author:** ThemisDB Contributors
Exact model backend not verifiable from final document text alone.