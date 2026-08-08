# Geo Module - Future Enhancements

<!-- Status: current | validated: 2026-07-29 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

## Scope

- hardening and refinement of geospatial backend/index/query runtime behavior
- expansion of deterministic reliability under mixed CPU/GPU capability pressure
- stronger benchmark-backed guardrails for geo hot paths

## Design Constraints

- geo contracts remain backward compatible within major release line.
- geometry validation and backend fallback behavior remain explicit and deterministic.
- advanced query execution remains bounded and observable.
- runtime backend transitions remain auditable.

## Required Interfaces

| Interface | Requirement |
|---|---|
| backend interfaces | deterministic CPU/GPU dispatch and fallback semantics |
| index/geometry interfaces | stable indexing and geometry-validation contracts |
| advanced query interfaces | bounded join/clustering/raster/temporal behavior |
| integration interfaces | explicit capability detection and bridge behavior |

## Implementation Notes (Q3 2026 Status Sync)

### Geospatial GPU Backend - CUDA (Completed)
- baseline Haversine distance kernel is present (spherical earth computation, < 0.5% accuracy target); deterministic precision-mode parity hardening complete.
- Vincenty distance kernel is implemented (WGS-84 ellipsoid, ±0.5mm accuracy); falls back to Haversine for nearly-antipodal points.
- baseline point-in-polygon containment kernel is present (ray-casting); fallback and diagnostics parity across degraded runtime scenarios remains open.
- device memory lifecycle and host↔device transfer behavior maintain explicit failure signaling and bounded fallback semantics.
- Distance matrix computation is supported via both Haversine and Vincenty kernels (all-pairs distance computation via GeoDistanceFormula parameter).

### Geospatial GPU Backend - HIP (In Progress Parity)
- HIP Haversine and point-in-polygon runtime paths are present; full parity with CUDA diagnostics and mixed-capability behavior remains open.
- hipMemcpy / hipMalloc allocation and transfer paths require continued reliability verification under sustained mixed-load scenarios.
- ROCm optimization remains constrained by deterministic behavior requirements before benchmark re-baselining.

### Geospatial GPU Backend - OpenCL Path (Planned v1.5.0+)
- OpenCL kernels for broader GPU compatibility
- Portable kernel compilation pipeline
- Interop with existing CUDA/HIP dispatch layer

### Phase-Aligned Hardening Plan (Phase 1-6)
- Phase 1 (Design/API): preserve geo contract and error taxonomy compatibility while tightening deterministic backend-dispatch and precision guarantees.
- Phase 2 (Core): close remaining backend fallback parity gaps across CPU/CUDA/HIP dispatch and advanced query boundedness.
- Phase 3 (Error/Edge): standardize fail-closed geometry validation and backend-switch diagnostics for degraded-capability incidents.
- Phase 4 (Tests): expand regressions for mixed backend + precision permutations and complex join/raster validation edges.
- Phase 5 (Performance): stabilize benchmark envelopes for CPU/GPU geo kernels, indexing, and join hot paths with reproducible p95/p99 guardrails.
- Phase 6 (Documentation/Acceptance): keep roadmap/future/evidence synchronization current and traceable for issue-driven closure.

## Test Strategy

- unit and integration suites for backend, indexing, geometry, and advanced query paths.
- regressions for invalid geometry, unsupported features, and fallback transitions.
- deterministic stress runs for mixed CPU/GPU geospatial workloads.
- release-profile benchmark runs for mapped geo targets.

## Performance Targets

- CPU/GPU geo hot paths remain within regression budgets.
- spatial index and join workloads remain stable at p95/p99 envelopes.
- benchmark manifests for mapped geo targets reach no-missing-case status.

## Security / Reliability

- maintain strict geometry validation before geospatial execution.
- preserve explicit failure signaling for degraded or unsupported paths.
- enforce bounded behavior for advanced geo operations under pressure.
- keep diagnostics actionable for production geo incidents.