# Geo Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-11-06 (research/source alignment; no build or test run) -->
<!-- Issue: #5646 (Development Status 2026-07-18) -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production geo runtime exists across CPU/GPU backends, spatial indexing, GeoJSON geometry processing, spatial joins, clustering, raster queries, temporal-spatial workflows, and tile integration.

The following issue #5646 status was recorded in the historical 2026-07-29
sync and updated with evidence dated 2026-08-17. It is not a verified statement
of the issue's status or the current build environment.

## Implementation status recorded in the prior sync

- [x] GPU memory safety hardening: RAII wrappers for CUDA/HIP device memory, goto-pattern removal (Phase 1, 2026-07-29)
- [x] Performance hot-path fixes: GeoJSON serialization O(n²) → ostringstream, reserve() for R-Tree/GH arrays (Phase 2, 2026-07-29)
- [x] API completion: geo_policy.h + geo_policy.cpp, RFC 7946 Doxygen annotations, inline R-Tree for temporal queries (Phase 3, 2026-07-29)
- [x] hardening backend fallback parity for degraded and mixed-capability runtime paths (Target: Q3 2026)
- [x] benchmark stabilization for geo CPU/GPU, indexing, and join hot paths (Target: Q3 2026)
- [x] diagnostics consistency improvements for validation and backend-switch incidents (Target: Q3 2026)
- [x] GPU geospatial backend CUDA: Haversine distance and point-in-polygon kernels (Target: Q3 2026)
- [x] GPU geospatial backend CUDA: Vincenty distance path for batched per-pair kernel dispatch (Target: Q3 2026) (2026-08-08)
- [~] GPU geospatial backend HIP: AMD ROCm feature-parity work (Target: Q3 2026; status recorded in the prior sync, not revalidated here)

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
- [x] Add and document release-gate benchmark fixtures (2026-08-07). The fixture source uses CPU-only mock implementations; this completion does not establish current production-path p95/p99 or GPU performance.

### Phase 6: Documentation and Acceptance (status reported 2026-07-29)
- [x] core geo module docs reported aligned to source-verifiable behavior (as recorded 2026-07-29; this 2026-10-31 audit corrected further drift)
- [x] roadmap/future planning separated from historical changelog entries
- [x] Contract header frozen: include/geo/geo_api_contract.h
- [x] Contract-hardening tests delivered: tests/geo/test_geo_contract_hardening_focused.cpp (GCH-01..GCH-16)
- [x] Release-gate benchmarks delivered: benchmarks/geo/bench_geo_release_gates.cpp (GRG-01..GRG-06)
- [x] Benchmark CMakeLists registered: benchmarks/geo/CMakeLists.txt
- [x] Q3 2026 status sync: roadmap and future priorities revalidated against full module docs (2026-07-29)
- [~] focused build/test evidence refresh attempted; explicit dependency blocker documented (2026-07-29)

## Production Readiness Checklist (recorded 2026-07-29; not a current sign-off)

The checkboxes below preserve the prior status record. They were not
re-validated as release-readiness evidence during this documentation-only
sync; current issue and execution status remain unverified.

- [x] core geo surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] Contract header frozen: include/geo/geo_api_contract.h (Phase 1)
- [x] Contract-hardening tests: tests/geo/test_geo_contract_hardening_focused.cpp (Phase 4, GCH-01..GCH-16)
- [x] Release-gate benchmarks: benchmarks/geo/bench_geo_release_gates.cpp (Phase 5, GRG-01..GRG-06)
- [x] Benchmark CMakeLists registered: benchmarks/geo/CMakeLists.txt
- [x] Deliver the listed hardening changes (GPU buffer guards, geo policy, and temporal/index updates). This does not establish that every backend/operation has parity; see the open implementation notes and current source.
- [x] release benchmark stabilization complete (Phase 5, 2026-08-07)

## Historical Evidence Summary (Issue #5646 Sync — 2026-07-29; later note 2026-08-17)

- Configure attempt 1: `cmake --preset community-release`
  - Result: now passes after installing required system dependencies (`librocksdb-dev`, `libfmt-dev`, `libspdlog-dev`, `nlohmann-json3-dev`, `libmimalloc-dev`, `libgtest-dev`, `libtbb-dev`, `libyaml-cpp-dev`, `libboost-system-dev`, `libboost-filesystem-dev`, `libcurl4-openssl-dev`).
- Configure attempt 2: `cmake --preset community-release-allow-missing-rocksdb -DCMAKE_DISABLE_FIND_PACKAGE_RocksDB=TRUE`
  - Result: passes and confirms RocksDB fallback detection via pkg-config dynamic path.
- Build attempt (historical, 2026-07-29): `cmake --build <build-dir> --target module_geo_test_aql_st_functions_focused`
  - Result at that time: blocked by the then-open cross-module `include/security/ai_snapshot_cleanup.h:63` constructor issue.
- Revalidation attempt (2026-08-17): header-only compile for `include/security/ai_snapshot_cleanup.h` passes under `g++ -std=c++17`, confirming the former constructor blocker is source-seitig closed.
- Revalidation attempt (2026-08-17): the recorded focused configure/build refresh was blocked because `community-release-allow-missing-rocksdb` failed without `spdlog`; the note also identified RocksDB as a dependency for broader coverage.
- Status at that historical update: the `ai_snapshot_cleanup.h` source defect was recorded as closed; the note left a fresh focused geo build/test run open.
- Last known focused target evidence from issue context: PASS on `module_geo_test_aql_st_functions_focused.exe --gtest_brief=1` (`82` tests, exit `0`, validated `2026-07-18`).

## Open Work (Issue #5646; status recorded 2026-08-17)

- [x] validate and refine extracted roadmap priorities against full module docs in `src/geo/ROADMAP.md`
- [x] validate and refine extracted future focus points against full module docs in `src/geo/FUTURE_ENHANCEMENTS.md`
- [~] add/refresh focused build and test evidence for this module (the 2026-08-17 note recorded the then-current dependency blockers; this is not a statement about the current environment)
- [x] mark completed synced items and risks with explicit status transitions

## Closure Criteria (Issue #5646; status recorded in the historical update)

- [x] all module acceptance criteria updated and traceable
- [x] evidence updated with explicit justified gap in this environment
- [ ] parent epic task entry checked by maintainer
- [ ] status labels updated by maintainer before close
- [x] close reason documented as "sync pass complete; historical non-geo compile blocker resolved, focused evidence refresh still needs a fresh current-cycle run"

## Known Issues & Limitations

- behavior depends on runtime capability and build flag combinations.
- selected advanced geo paths still require ongoing hardening and benchmark expansion.
- spherical/ellipsoid and additional kernel-depth work remains long-term.
- `GeoBackendDispatcher::computeVincentyBatch()` currently computes with the CPU
  implementation; its source explicitly leaves GPU dispatch integration open.
- Current issue state and fresh build/test execution evidence were not verified
  by this documentation-only synchronization.

## Breaking Changes

No breaking geo contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## v2.5.0 / v3.0.0 Research and Release Wave Plan

**Status:** Proposed module plan, not an implementation or release-readiness sign-off. The repository is currently `2.4.0` / `alpha`; `VERSIONING.md` names `v2.5.0` as the next planned minor. No approved v3.0.0 scope or date is asserted. Estimates below are engineering person-weeks, not calendar commitments. The root `ROADMAP.md` and `FUTURE_ENHANCEMENTS.md` remain authoritative for project-wide release order and gates.

### Current maturity assessment (source snapshot: 2026-11-06)

The integrated geo module has substantial implemented functionality, but available evidence does not establish uniform production readiness or current release-grade performance:

| Area | Source-backed status | Gap / interpretation |
|---|---|---|
| Geometry and formats | Typed GeoJSON validation, EWKB, WGS84 bounds and geometry operations exist (`include/geo/geo_json_geometry.h`, `include/geo/geo_api_contract.h`, `src/geo/geo_json_geometry.cpp`). | Coordinate order, units and CRS assumptions vary by API. `src/geo/RESEARCH.md` documents planar point-in-polygon limits and approximate spherical methods; global geometry correctness must be evaluated per operation. |
| Indexing and query | In-memory `GeoRTree`, range/cursor interfaces, spatial joins, clustering, raster, temporal-spatial, tile and point k-NN paths exist (`include/geo/`, `src/geo/`). | The benchmark suite has production-path index/join cases, but no current run artifact or representative-hardware p95/p99 results are recorded here. Limits in `geo_api_contract.h` are not proof that every operation enforces them. |
| Application usability | APIs support point proximity, spatial joins, GeoJSON/EWKB geometry, clustering, temporal-spatial queries, raster interfaces, tiles and point k-NN. | Callers must know per-entry-point coordinate order, units, CRS and approximation limits; examples and explicit operation contracts should be validated as part of the v3 API inventory. |
| CPU/GPU execution | CUDA/HIP code, CPU paths, dispatch, device detection and fallback surfaces exist. | `GeoBackendDispatcher::computeVincentyBatch()` currently stays on CPU (`src/geo/geo_backend_dispatch.cpp`); a requested GPU benchmark or CPU fallback is not GPU execution evidence. |
| Tests and benchmark quality | Focused tests cover geometry, joins, indexes, fallback and optional backend parity (`tests/geo/`). Production-path benchmarks include R-tree, joins, GeoJSON parsing and DBSCAN (`benchmarks/geo/`). | The GRG-01..06 release-gate benchmark uses CPU-only mock implementations. The high-cardinality stress fixture also uses stubs. Neither proves production-path throughput, memory use, GPU parity or sustained reliability. |
| Security and operations | Validation, error codes, policy limits, backend reporting and runbook surfaces exist (`src/geo/SECURITY.md`, `src/geo/OPERATIONS.md`). | Authorization remains at the owning server boundary; the geo collection allowlist is advisory. Resource bounds and failure behavior need call-path verification and stress evidence rather than inference from declarations. |

**Maturity conclusion:** implemented core with meaningful focused coverage; correctness across global edge cases, representative performance, cross-backend execution and sustained-load operations remain evidence gaps. Do not assign a numeric maturity score or claim a release gate passed without current run artifacts.

### Research findings and code gaps

1. **Interoperability and geometry semantics.** RFC 7946 defines GeoJSON coordinate and geometry rules, including linear-ring closure, the right-hand rule and antimeridian handling. OGC Simple Feature Access 1.2.1 defines a common geometry model associated with a spatial reference system. The code has validation and CRS labels, but module research describes planar polygon and spherical-distance limits; validation of syntax alone does not establish globally correct predicates or CRS transformation.
2. **Spatial-index and join performance is workload- and layout-dependent.** Rayhan and Aref's SIMD R-tree study reports gains for selected range-select and spatial-join experiments; SwiftSpatial explores specialized hardware and join scheduling. These results justify measured experiments, not a transferable speedup promise or immediate adoption. The current R-tree and join benchmarks provide a starting point for controlled comparisons.
3. **Benchmark evidence must be reproducible and representative.** SpatialBench methodology recommends representative spatial workloads and recording hardware/software context for comparison. The module already has seeded benchmarks at multiple scales, but the CPU-only release-gate mocks and benchmark registration are not measured production results. Capture data scale/selectivity, build/backend, hardware, correctness and latency distributions in each result artifact.
4. **Robust global geodesics are a strategic candidate.** The research influence matrix lists Karney's geodesic algorithms as planned for v2.5.0+, while the current implementation uses Vincenty with a Haversine fallback near non-convergence. Evaluate a robust reference/implementation and its API/cost implications before selecting a release or adding a dependency.

### v2.5.0 — compatibility-preserving hardening (total estimate: 10–16 engineer-weeks)

This track targets evidence, correctness and bounded operation within the existing public contract; it does not authorize breaking API changes.

#### Wave 1 — Baselines and evidence (P0; estimate: 2–3 engineer-weeks)

- [ ] Run production-path R-tree, spatial-join, GeoJSON and DBSCAN benchmarks with fixed inputs at 1k/10k/100k scale (and 1M for index cases where feasible); record p50/p95/p99, throughput, memory, selectivity, compiler/build flags, hardware, revision, backend and fallback status (Target: v2.5.0).
- [ ] Separate measured implementation results from GRG mock thresholds and GPU-requested cases that fall back to CPU; publish a reproducible manifest for every comparison (Target: v2.5.0).
- **Dependencies:** dependency-complete benchmark build; representative CPU runner; CUDA/HIP hardware for any GPU claim.
- **Risk / trade-off:** hardware and data distribution can dominate results; mitigate by retaining seeds, workload characteristics and environment metadata, and compare only like-for-like runs.
- **Success metrics:** every published result has all required manifest fields; both CPU and actual GPU execution are separately identified; baseline contains latency percentiles and memory for each selected production workload. No performance claim is accepted from a registered target alone.
- **Validation:** `benchmarks/geo/bench_spatial_index.cpp`, `bench_spatial_join.cpp`, `bench_geojson_parse.cpp`, `bench_geo_dbscan.cpp`; inspect benchmark output and execution backend.

#### Wave 2 — Correctness, input bounds and fallback parity (P0; estimate: 3–5 engineer-weeks)

- [ ] Reconcile GeoJSON, coordinate-order, CRS, units, ring and antimeridian expectations against RFC 7946, OGC SFA and the operation-specific public headers; list unsupported cases explicitly without silently changing existing semantics (Target: v2.5.0).
- [ ] Trace each advertised geometry/collection/query limit to its consuming call path; add focused implementation regressions for malformed, oversized, empty, non-finite, antimeridian, polar and near-antipodal inputs where the operation claims support (Target: v2.5.0).
- [ ] Verify deterministic CPU/GPU results and diagnostics for supported kernels, including unavailable device, dispatch failure and CPU fallback; keep unsupported operations explicitly reported (Target: v2.5.0).
- **Dependencies:** Wave 1 benchmark/test environment; an agreed reference corpus and per-operation tolerances; actual CUDA/HIP execution for parity sign-off.
- **Risk / trade-off:** a broad standards interpretation may expose existing behavior differences; keep this release contract-compatible and defer API/semantic breaks to the v3 decision wave.
- **Success metrics:** zero unexplained mismatches against the curated reference fixtures; every supported/unsupported edge case has an explicit test outcome; no invalid or oversized input reaches a backend on audited paths; fallback tests distinguish CPU execution from GPU success.
- **Validation:** `test_geo_json_geometry.cpp`, `test_geo_ewkb.cpp`, `test_geo_wgs84_spherical.cpp`, `test_geo_hardening_focused.cpp`, `test_category_b_parity_geo.cpp`, and corresponding backend-specific tests.

#### Wave 3 — Profile-led index, join and dispatch improvements (P1; estimate: 3–5 engineer-weeks)

- [ ] Use Wave 1 profiles to select one or more measured hotspots; compare the current R-tree/join path with only evidence-justified alternatives (including R*-tree layout or SIMD filtering) before implementing any change (Target: v2.5.0).
- [ ] Measure batch break-even and CPU/GPU parity with transfer and fallback costs included; do not connect or advertise GPU Vincenty until its actual dispatch path and execution are validated (Target: v2.5.0).
- [ ] Preserve exact predicate refinement after approximate MBR candidate filtering and retain deterministic result/error behavior (Target: v2.5.0).
- **Dependencies:** Waves 1–2; representative hardware; no new dependency or public API unless separately reviewed and approved.
- **Risk / trade-off:** SIMD/GPU paths can improve throughput but increase portability, maintenance and precision risk; retain the simpler path when measured gains do not justify that cost.
- **Success metrics:** at least one selected production workload improves p95 by ≥10% against its recorded baseline, with no >10% p99 regression in the other gated workloads and no correctness mismatch; otherwise record the experiment and do not claim an optimization.
- **Validation:** repeat production benchmarks with identical workload and environment metadata; run concrete index/join tests plus CPU/GPU parity suites where the relevant device is available.

#### Wave 4 — Release verification and operational handoff (P0; estimate: 2–3 engineer-weeks)

- [ ] Run the focused geo implementation suites and applicable `release_critical` checks on the release target; archive current CI/test and benchmark artifacts (Target: v2.5.0).
- [ ] Review geometry/resource exhaustion, fallback diagnostics, authorization boundary, data redaction and operator remediation/runbook coverage; track unresolved high-risk items instead of inferring closure from docs (Target: v2.5.0).
- [ ] Update module API/research/performance/operations documentation to match verified behavior and record any unmet gate as open (Target: v2.5.0).
- **Dependencies:** Waves 1–3 and a current dependency-complete build; GPU sign-off depends on device-backed jobs.
- **Risk / trade-off:** CI success without representative execution may give false confidence; gate GPU and performance claims on matching artifacts.
- **Success metrics:** all selected focused suites and required CI gates pass on current source; zero open critical/high geo findings for the approved release scope; every production performance claim links to a current manifest; runbook and unresolved-risk list reviewed.
- **Validation:** tests in `tests/geo/CMakeLists.txt`, applicable root release gates, benchmark manifests, security review and documentation link/source checks.

### v3.0.0 — strategic contract and scale evolution (total estimate: 15–26 engineer-weeks)

The v3 track is a research proposal pending API-owner and release-governance decisions. In particular, `include/geo/geo_api_contract.h` still says “stable within v1.x” and “breaking changes require v2.0”; that version statement conflicts with the repository's current v2.x line and must be resolved before treating any breaking geo contract as approved.

#### Wave 1 — Contract and migration decisions (P0; estimate: 2–3 engineer-weeks)

- [ ] Inventory public geo entry points and resolve coordinate order, CRS transformation/identity, units, exact-vs-approximate semantics, geometry validity, failure, resource-limit and backend-reporting contracts (Target: v3.0.0).
- [ ] Produce an API compatibility decision and migration table; require human/API-owner approval before changing signatures, defaults or persisted representations (Target: v3.0.0).
- **Dependencies:** v2.5.0 evidence; API owner and release-governance review; root `VERSIONING.md` and `RELEASE_STRATEGY.md`.
- **Risk / trade-off:** contract unification can break callers and alter numeric results; use explicit major-version migration notes and avoid implicit legacy shims.
- **Success metrics:** 100% of public geo operations in the inventory state coordinate order, CRS, units, approximation and error behavior; every approved breaking change has a migration example and compatibility test.
- **Validation:** API/header review, contract tests, migration fixtures and cross-document review. No implementation starts before contract approval.

#### Wave 2 — Robust global geometry and geodesics (P0; estimate: 6–10 engineer-weeks)

- [ ] Evaluate and, if approved, implement reference-backed ellipsoidal geodesics (including antipodal cases) and clearly scoped global/antimeridian/polar predicates; assess Karney's algorithm against current Vincenty/Haversine behavior and the required accuracy/cost (Target: v3.0.0).
- [ ] Define explicit planar/projected/geodesic operation modes and CRS behavior; keep unsupported transformations and geometry cases fail-closed (Target: v3.0.0).
- **Dependencies:** Wave 1 contract approval; standards/reference corpus; dependency, licensing and portability review if an external implementation is proposed.
- **Risk / trade-off:** stronger geographic correctness may increase CPU cost and change long-distance results; offer explicit semantics and measure latency/accuracy rather than silently replacing approximations.
- **Success metrics:** zero mismatches across the approved deterministic global-edge corpus and ≥100,000 generated distance pairs against the chosen reference within the published per-mode tolerance; no success result for unsupported CRS/geometry inputs.
- **Validation:** reference-based unit/property tests for poles, antimeridian, near-antipodal points, degenerate/invalid rings and CRS errors; CPU/backend parity and numerical/performance benchmarks.

#### Wave 3 — Scalable indexing and query execution (P1; estimate: 4–8 engineer-weeks)

- [ ] Evaluate index layout, spatial-join algorithms and vectorized filtering on representative sparse/dense and skewed datasets; adopt R*-tree/SIMD or other alternatives only when measured against the current R-tree baseline (Target: v3.0.0).
- [ ] Measure bounded clustering, temporal-spatial and k-NN workloads at larger cardinalities, reporting query latency, throughput, memory, candidate selectivity and exact-refinement cost (Target: v3.0.0).
- [ ] Specify and validate an explicit GPU dispatch/CPU fallback matrix, including Vincenty only if Wave 2 accuracy and Wave 1 hardware evidence support it (Target: v3.0.0).
- **Dependencies:** Waves 1–2; representative datasets and hardware; accepted single-node vs distributed scope (distribution is not assumed to exist).
- **Risk / trade-off:** sophisticated indexing/acceleration may raise write cost, memory, portability and operational complexity; compare read/write and memory trade-offs before selection.
- **Success metrics:** publish scaling curves from 1k through at least 1M records for chosen index workloads and up to the supported join limit where feasible; validate the existing `bench_spatial_join.cpp` 100k×100k/first-1k ≤500ms target on recorded hardware or formally revise it with evidence; no >10% p99 regression in unchanged baseline workloads.
- **Validation:** indexed-vs-linear correctness, spatial-join completeness/soundness, skew/selectivity benchmarks, memory tracking and actual CPU/GPU execution reports.

#### Wave 4 — Interoperability, security and production acceptance (P0; estimate: 3–5 engineer-weeks)

- [ ] Deliver the approved v3 contract with API migration notes, standards/conformance cases, resource limits, error semantics and compatibility guidance for GeoJSON/EWKB integrations (Target: v3.0.0).
- [ ] Validate access-control ownership at server integration boundaries, fail-closed behavior, bounded adversarial inputs, diagnostics redaction, sustained mixed-load behavior and operator recovery procedures (Target: v3.0.0).
- [ ] Archive reproducible test, benchmark, security and operational evidence; resolve open assumptions before release sign-off (Target: v3.0.0).
- **Dependencies:** Waves 1–3; owning server/security/API reviewers; release-critical CI and representative hardware.
- **Risk / trade-off:** standard conformance, migration and operations work can exceed implementation effort; defer optional algorithms rather than weaken correctness or security exit criteria.
- **Success metrics:** all approved migration cases pass; zero critical/high geo security findings remain open for the release scope; all required focused tests pass; every required benchmark and soak result is reproducible with full metadata; no authorization decision relies on the advisory geo policy.
- **Validation:** API compatibility and conformance suite, adversarial geometry/resource tests, sustained-load and recovery checks, server-boundary authorization tests, release-critical CI and human sign-off.

### Cross-wave risks, assumptions and unresolved questions

| Topic | Risk / assumption | Required resolution |
|---|---|---|
| API version | `geo_api_contract.h` references v1.x/v2.0 despite the current project version v2.4.0. | API owner must correct/confirm the contract's major-version policy before v3.0.0 compatibility decisions. |
| Antimeridian status | `research/implementation_influence/by_module.md` records an antimeridian fix as delivered in 2026-07, while current `src/geo/RESEARCH.md` limits planar containment and does not establish global correctness for every operation. | Identify the exact operation/test that supports the historical fix; do not generalize it until concrete test evidence does. |
| Performance | GRG release gates are CPU-only mocks; benchmark source registration and comments are not results. | Require current production-path manifests, representative hardware and percentiles before promoting performance claims. |
| GPU support | Build-time CUDA/HIP selection, runtime device availability and actual per-operation dispatch are distinct. | Record the backend actually executed; CPU fallback validates fallback, not GPU capability or performance. |
| Workload scope | Current sources describe in-memory indexing and bounded/operation-specific query paths; they do not establish persistent/distributed geo indexing. | Treat distributed indexing as out of scope unless an architecture decision and storage/sharding owner are approved. |
| External research | Published speedups depend on implementation, hardware, data and selectivity; standards define semantics, not this module's compliance. | Treat papers/standards as design input and validate every selected behavior/performance target locally. |

### Research and evidence references

- RFC 7946, *The GeoJSON Format*, especially §§3.1 and 3.1.9: <https://www.rfc-editor.org/rfc/rfc7946>
- OGC, *Simple Feature Access — Part 1: Common Architecture*, version 1.2.1: <https://www.ogc.org/it/standards/sfa/>
- Rayhan and Aref, *SIMD-ified R-tree Query Processing and Optimization* (2023), arXiv:2309.16913: <https://arxiv.org/abs/2309.16913>
- Jiang, Parvanov and Alonso, *SwiftSpatial: Spatial Joins on Modern Hardware* (2023), arXiv:2309.16520: <https://arxiv.org/abs/2309.16520>
- Apache Sedona, *SpatialBench Methodology*: <https://sedona.apache.org/spatialbench/overview-methodology/>
- Karney (2013), *Algorithms for geodesics*, J. Geodesy 87: <https://arxiv.org/abs/1109.4448>. Listed as planned in [`research/implementation_influence/by_module.md`](../../research/implementation_influence/by_module.md).
- Implementation and evidence map: [`RESEARCH.md`](./RESEARCH.md), [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md), [`SECURITY.md`](./SECURITY.md), [`tests/geo/`](../../tests/geo/) and [`benchmarks/geo/`](../../benchmarks/geo/).

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `geo`
- [ ] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
- [I] **GPU Vincenty batch dispatch**: connect the CUDA formula dispatch to `GeoBackendDispatcher::computeVincentyBatch()`. The current method retains the CPU implementation and source comments identify the missing GPU integration. (Target: Q1 2027; retain as an open task until verified)
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

---

**AI_Used:** true · **AI_Models:** unknown · **AI_Contributor:** GitHub Copilot coding agent · **Human_Review:** false · **Human_Author:** ThemisDB Contributors
Exact model backend not verifiable from final document text alone.
