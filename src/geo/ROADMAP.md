# Geo Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-29 -->
<!-- Issue: #5646 (Development Status 2026-07-18) -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production geo runtime exists across CPU/GPU backends, spatial indexing, GeoJSON geometry processing, spatial joins, clustering, raster queries, temporal-spatial workflows, and tile integration.

Issue #5646 remains open with partial closure coverage: roadmap/future synchronization is refreshed, configure now succeeds in this Linux environment after dependency provisioning, but focused executable evidence is still blocked by a pre-existing cross-module compile error in `include/security/ai_snapshot_cleanup.h`.

## In Progress

- [x] GPU memory safety hardening: RAII wrappers for CUDA/HIP device memory, goto-pattern removal (Phase 1, 2026-07-29)
- [x] Performance hot-path fixes: GeoJSON serialization O(n²) → ostringstream, reserve() for R-Tree/GH arrays (Phase 2, 2026-07-29)
- [x] API completion: geo_policy.h + geo_policy.cpp, RFC 7946 Doxygen annotations, inline R-Tree for temporal queries (Phase 3, 2026-07-29)
- [x] hardening backend fallback parity for degraded and mixed-capability runtime paths (Target: Q3 2026)
- [x] benchmark stabilization for geo CPU/GPU, indexing, and join hot paths (Target: Q3 2026)
- [x] diagnostics consistency improvements for validation and backend-switch incidents (Target: Q3 2026)
- [x] GPU geospatial backend CUDA: Haversine distance and point-in-polygon kernels (Target: Q3 2026)
- [x] GPU geospatial backend CUDA: Vincenty distance path for batched per-pair kernel dispatch (Target: Q3 2026) (2026-08-08)
- [~] GPU geospatial backend HIP: AMD ROCm feature-parity implementation (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior for mixed backend and precision-mode edge permutations (Target: Q4 2026)
- [ ] expand regressions for geometry validation and complex join/raster edge cases (Target: Q4 2026)
- [ ] improve operator diagnostics for backend fallback and capability drift incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for geo CPU/GPU and spatial index workloads (Target: Q1 2027)
- [ ] broaden benchmark depth for clustering, raster, temporal-spatial, and k-NN bridge paths (Target: Q1 2027)
- [ ] harden long-running reliability under sustained heterogeneous geo workloads (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Freeze geo module API contract — GeoJSON validation, backend dispatch, spatial index, spatial join, error taxonomy (include/geo/geo_api_contract.h) (Target: Q3 2026)
- [x] Define explicit GeoErrorCode taxonomy (GEOMETRY_INVALID, BACKEND_UNAVAILABLE, INDEX_CORRUPTED, PRECISION_EXCEEDED, UNSUPPORTED_GEOMETRY_TYPE, …) (Target: Q3 2026)

### Phase 2: Core Implementation
- [~] complete hardening for backend dispatch, indexing, and geometry internals (Target: Q4 2026)
- [~] align advanced query features to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [~] standardize fail-closed behavior for invalid geometry and unsupported execution scenarios (Target: Q4 2026)
- [~] unify diagnostics across join/clustering/raster/temporal and fallback paths (Target: Q4 2026)

### Phase 4: Tests
- [x] Contract-hardening focused tests GCH-01..GCH-16 covering GeoJSON validation, backend dispatch, spatial index, and spatial join invariants (tests/geo/test_geo_contract_hardening_focused.cpp) (Target: Q4 2026)
- [x] Phase 1–3 hardening tests GCH-17..GCH-24: GPU RAII lifecycle, geo_policy validation, Vincenty Haversine fallback (tests/geo/test_geo_hardening_focused.cpp) (2026-07-29)
- [x] Expand focused regressions for backend/geometry/index edge scenarios (Target: Q4 2026)
- [x] Extend deterministic fixture coverage for spatial workload permutations (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [x] Lock benchmark-backed release gates for geo hot paths: GRG-01..GRG-06 in benchmarks/geo/bench_geo_release_gates.cpp (point-in-polygon 1k p99≤5ms, bbox query 10k p99≤1ms, GeoJSON parse p99≤500µs, Haversine p99≤10µs, spatial join p99≤50ms, backend selection p99≤50µs) (Target: Q4 2026)
- [x] Validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026) — Release-gate benchmarks validated and documented (2026-08-07)

### Phase 6: Documentation and Acceptance
- [x] core geo module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] Contract header frozen: include/geo/geo_api_contract.h
- [x] Contract-hardening tests delivered: tests/geo/test_geo_contract_hardening_focused.cpp (GCH-01..GCH-16)
- [x] Release-gate benchmarks delivered: benchmarks/geo/bench_geo_release_gates.cpp (GRG-01..GRG-06)
- [x] Benchmark CMakeLists registered: benchmarks/geo/CMakeLists.txt
- [x] Q3 2026 status sync: roadmap and future priorities revalidated against full module docs (2026-07-29)
- [~] focused build/test evidence refresh attempted; explicit dependency blocker documented (2026-07-29)

## Production Readiness Checklist

- [x] core geo surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] Contract header frozen: include/geo/geo_api_contract.h (Phase 1)
- [x] Contract-hardening tests: tests/geo/test_geo_contract_hardening_focused.cpp (Phase 4, GCH-01..GCH-16)
- [x] Release-gate benchmarks: benchmarks/geo/bench_geo_release_gates.cpp (Phase 5, GRG-01..GRG-06)
- [x] Benchmark CMakeLists registered: benchmarks/geo/CMakeLists.txt
- [x] remaining hardening tasks closed for fallback/validation/advanced-query edge paths (Phase 1–3 complete: GPU RAII, performance fixes, geo_policy, temporal R-Tree optimization, RFC 7946 annotations)
- [x] release benchmark stabilization complete (Phase 5, 2026-08-07)

## Evidence Summary (Issue #5646 Sync — 2026-07-29)

- Configure attempt 1: `cmake --preset community-release`
  - Result: now passes after installing required system dependencies (`librocksdb-dev`, `libfmt-dev`, `libspdlog-dev`, `nlohmann-json3-dev`, `libmimalloc-dev`, `libgtest-dev`, `libtbb-dev`, `libyaml-cpp-dev`, `libboost-system-dev`, `libboost-filesystem-dev`, `libcurl4-openssl-dev`).
- Configure attempt 2: `cmake --preset community-release-allow-missing-rocksdb -DCMAKE_DISABLE_FIND_PACKAGE_RocksDB=TRUE`
  - Result: passes and confirms RocksDB fallback detection via pkg-config dynamic path.
- Build attempt: `cmake --build /home/runner/work/ThemisDB/ThemisDB/build-community-release-allow-missing-rocksdb --target module_geo_test_aql_st_functions_focused`
  - Result: blocked by pre-existing compile error outside geo scope: `include/security/ai_snapshot_cleanup.h:63` (`AiSnapshotCleanupJob(Config cfg = {})` brace-init conversion failure).
- Focused build/test status in this environment: executable refresh still blocked; test binary not produced yet.
- Last known focused target evidence from issue context: PASS on `module_geo_test_aql_st_functions_focused.exe --gtest_brief=1` (`82` tests, exit `0`, validated `2026-07-18`).

## Open Work (Issue #5646)

- [x] validate and refine extracted roadmap priorities against full module docs in `src/geo/ROADMAP.md`
- [x] validate and refine extracted future focus points against full module docs in `src/geo/FUTURE_ENHANCEMENTS.md`
- [~] add/refresh focused build and test evidence for this module (configure is now unblocked; build is blocked by pre-existing non-geo compile error documented in Evidence Summary)
- [x] mark completed synced items and risks with explicit status transitions

## Closure Criteria (Issue #5646)

- [x] all module acceptance criteria updated and traceable
- [x] evidence updated with explicit justified gap in this environment
- [ ] parent epic task entry checked by maintainer
- [ ] status labels updated by maintainer before close
- [x] close reason documented as "sync pass complete; focused evidence refresh remains blocked by pre-existing non-geo compile error in current tree"

## Known Issues & Limitations

- behavior depends on runtime capability and build flag combinations.
- selected advanced geo paths still require ongoing hardening and benchmark expansion.
- spherical/ellipsoid and additional kernel-depth work remains long-term.

## Breaking Changes

No breaking geo contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `geo`
- [ ] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
- [ ] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
- [ ] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [ ] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)
