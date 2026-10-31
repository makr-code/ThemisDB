# Geo production integration checks

## Purpose and authority

This is an integration checklist for service and deployment owners. It is not
an independent security, authorization, or production-readiness contract.
Public geo declarations and operation-specific behavior are defined by headers
under `include/geo/`; server authentication,
authorization, and deployment controls belong to the owning service and the
project security policy.

## Verify before deployment

- Confirm that the server/API integration enforces authentication and
  collection-level authorization. `SpatialQueryPolicy::allowed_collections`
  is advisory and does not replace the server's authorization decision.
- Trace the concrete request path to its validator and resource-policy
  consumer. Contract constants and policy fields do not establish that every
  operation enforces the same limits.
- Review zero/unlimited semantics and defaults for the specific operation.
  In particular, zero means unlimited for some policy fields and disables
  timeout enforcement for `timeout`.
- Confirm the selected build and runtime backend. CUDA/HIP compilation does not
  prove a usable device or GPU execution; check operation-specific fallback
  behavior.
- Handle optional features by their result/status contract. For example, a
  `NoOpRasterQueryImpl` returns `RasterStatus::NOT_SUPPORTED`.
- Preserve build revision, configuration, backend selection, and relevant
  diagnostic output for release and incident evidence. A benchmark threshold
  or a mock-based test is not a production-path pass.

## Configuration evidence and uncertainty

The repository's geo CMake configuration defines `THEMIS_GEO_CUDA` and
`THEMIS_GEO_HIP` selection (CUDA takes precedence), along with
`THEMIS_GEO_DIAGNOSTICS_DETAILED` and `THEMIS_GEO_STRICT_VALIDATION`.
`THEMIS_GEO_BOOST_BACKEND` is used as a compile definition for the Boost-backed
implementation when that build path is enabled.

`raster_query_interface.h` names `THEMIS_ENABLE_RASTER` as a compile-time guard.
The geo feature configuration does not define a matching user-facing CMake
option in the inspected sources. **TODO:** verify the owning build's raster
enablement path before documenting a deployment setting.

No evidence was found in the geo CMake configuration for the former
`THEMIS_PRODUCTION_MODE`, `THEMIS_ENVIRONMENT`, or `geo.*` runtime tuning
settings. Do not rely on those names as supported configuration.

## Canonical references

- Public contract: `include/geo/geo_api_contract.h`.
- Policy and advisory allowlist:
  `include/geo/geo_policy.h`.
- Geo build feature selection:
  `cmake/features/GeoFeatures.cmake`.
- Server integration:
  `src/server/spatial_api_handler.cpp`.
- Security policy: `SECURITY.md`.
- Operational behavior:
  `src/geo/OPERATIONS.md` and `src/geo/RUNBOOK.md`.

---

**AI_Used:** true · **AI_Models:** unknown · **AI_Contributor:** GitHub Copilot coding agent · **Human_Review:** false · **Human_Author:** ThemisDB Contributors
Exact model backend not verifiable from final document text alone.
