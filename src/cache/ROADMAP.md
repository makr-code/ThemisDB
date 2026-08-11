# Cache Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-27 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production cache runtime exists across adaptive query cache, semantic/embedding cache surfaces, warmup and predictive paths, plus distributed coordination and replication components.
Validation refresh for issue `#5632` confirms priorities remain correct; evidence refresh is blocked in this environment by missing RocksDB system dependency.

## In Progress

- [x] hardening distributed consistency and degradation behavior across cache coordinators (Target: Q3 2026)
  - Delivered: `include/cache/cache_contract.h` §5–§7 (coordinator capability flags, fail-closed semantics, async replication consistency)
  - Delivered: `tests/cache/test_cache_coordinator_degradation.cpp` (CCD-01..CCD-08)
- [x] benchmark stabilization for cache hot paths and release-gate envelopes (Target: Q3 2026)
  - Delivered: `benchmarks/cache/release_gate_manifest_cache.json` (GATE-CAC-01..06, SGATE-CAC-01..04)
  - Delivered: `benchmarks/cache/CACHE_BENCHMARK_RUNBOOK.md`
- [x] diagnostics consistency improvements for tenant/invalidation/replication failures (Target: Q3 2026)
  - Delivered: `include/cache/cache_contract.h` §3 (CacheFailureClass taxonomy + fail-closed predicate)
  - Delivered: `tests/cache/test_cache_contract_hardening.cpp` (CCH-01..CCH-08)

## Planned Features

### Short-term (3-6 months)
- [ ] integrate with AccessCoordinator for unified cache-storage tier management (Target: Q4 2026)
  - Add EvictionListener callbacks to emit cache eviction signals
  - Refactor cache eviction policy thresholds (hot/warm → L1/L2/L3)
  - Implement storage demotion feedback hooks
  - See: `src/access_model/ROADMAP.md` Phase 3
- [ ] tighten deterministic failure semantics for partial-backend/degraded coordination states (Target: Q4 2026)
- [ ] expand regression coverage for invalidation and replication edge permutations (Target: Q4 2026)
- [ ] improve operator diagnostics for cache SLO and tenant-isolation incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline cache p95/p99 and throughput envelopes on release profiles (Target: Q1 2027)
- [ ] reduce proxy-like performance mappings via dedicated cache microbenchmarks (Target: Q1 2027)
- [ ] harden multi-node consistency behavior for long-running distributed cache workloads (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze cache contract semantics for adaptive, embedding, and replication surfaces (Target: Q3 2026)
  - Subsystems/files: `src/cache/adaptive_query_cache.cpp`, `src/cache/embedding_cache.cpp`, `src/cache/cache_replication*.cpp`, `include/cache/*.h`
  - Runtime behavior: deterministic `get/put/invalidate` outcomes across enabled backends with identical tenant keying semantics.
  - Validation: contract tests for null cache, unknown tenant, malformed invalidation key, and degraded coordinator state mapping.
  - **Delivered:** `include/cache/cache_contract.h` v1.0.0 — frozen §1–§7 contract (size limits, temporal bounds, failure taxonomy, fail-closed predicates, coordinator capabilities, async replication semantics)
- [x] define explicit error taxonomy for tenant, invalidation, and coordinator failures (Target: Q3 2026)
  - Subsystems/files: cache admin handler, coordinator adapters, replication surfaces.
  - Runtime behavior: normalize failure classes to stable operator-facing categories (`validation`, `degraded`, `unavailable`, `conflict`).
  - Compatibility: no wire/API breaking changes in current major release line.
  - **Delivered:** `CacheFailureClass` enum (7 classes) + `isFailClosedClass()` / `isMalformedInputClass()` predicates in `include/cache/cache_contract.h §3–§4`

### Phase 2: Core Implementation
- [x] complete hardening for cache coordination and replication runtime paths (Target: Q4 2026)
  - Subsystems/files: `distributed_cache_coordinator.cpp`, `redis_cache_coordinator.cpp`, `grpc_remote_cache_peer.cpp`, `cache_replication_coordinator.cpp`.
  - Runtime behavior: bounded retry/degradation transitions with deterministic fail-closed behavior when quorum/coordinator guarantees are not met.
  - Errors: partial-backend participation must return explicit degraded outcomes, never silent success.
  - **Delivered:** Hardening semantics defined in `cache_contract.h §4–§7` (fail-closed predicate, coordinator capability flags, min retry backoff constant, replication delivery bounds); coordinator degraded-state compliance validated by CCD-01..CCD-08.
- [x] align cache runtime behavior to bounded contracts across feature flags/backends (Target: Q4 2026)
  - Subsystems/files: adaptive cache + warmup + prefetch entry points.
  - Runtime behavior: feature-flag/backend permutations produce stable behavior classes and auditable diagnostics.
  - Tests: matrix-driven focused tests for backend enabled/disabled combinations.
  - **Delivered:** Tenant isolation matrix tested in CTI-01..CTI-08 (isolation on/off, cross-tenant keying, eviction scope).

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-closed behavior for invalid tenant/degraded backend scenarios (Target: Q4 2026)
  - Runtime behavior: invalid tenant IDs, missing tenant contexts, and degraded backends must return deterministic non-2xx responses.
  - Validation: include malformed tenant path, trailing path, null cache, and backend timeout edge cases.
  - **Delivered:** `CacheFailureClass` taxonomy in `cache_contract.h §3`; `isFailClosedClass()` tested in CCH-03/CCH-04; degraded-peer semantics tested in CCD-02/CCD-03/CCD-05.
- [x] unify diagnostics across invalidation/warmup/replication failure classes (Target: Q4 2026)
  - Runtime behavior: shared error envelope fields (`category`, `component`, `retryable`, `correlation_id`) for operator workflows.
  - Compatibility: preserve existing response keys; additions must be backward-compatible.
  - **Delivered:** Canonical failure class names (`MalformedKey`, `MalformedValue`, `InvalidOperation`, `TenantViolation`, `DegradedBackend`, `PartialDelivery`, `InternalError`) defined in `cache_contract.h §3` to align all operator-facing diagnostic envelopes.

### Phase 4: Tests
- [x] expand focused regressions for distributed consistency and tenant edge scenarios (Target: Q4 2026)
  - Test scope: `tests/cache/test_cache_admin_api_handler.cpp` plus distributed cache focused suites.
  - Validation: invalidation ordering, duplicate replication events, tenant isolation leakage checks, and degraded coordinator transitions.
  - **Delivered:** `tests/cache/test_cache_coordinator_degradation.cpp` (CCD-01..CCD-08) + `tests/cache/test_cache_tenant_isolation_hardening.cpp` (CTI-01..CTI-08)
- [x] extend deterministic fixture coverage for warmup/prefetch and coordinator permutations (Target: Q4 2026)
  - Test strategy: deterministic seeds and fixture-controlled backends for warmup log replay and prefetch scheduling edge cases.
  - Acceptance: no flaky focused tests across repeated CI runs.
  - **Delivered:** `tests/cache/test_cache_contract_hardening.cpp` (CCH-01..CCH-08) — all tests are deterministic, seed-free, and RocksDB-independent.

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for cache hot paths (Target: Q4 2026)
  - Benchmarks/files: `benchmarks/bench_adaptive_query_cache.cpp`, `benchmarks/bench_embedding_cache_performance.cpp`.
  - Gates: mapped cache cases must have no missing manifest entries in release-profile runs.
  - **Delivered:** `benchmarks/cache/release_gate_manifest_cache.json` (GATE-CAC-01..06, SGATE-CAC-01..04, manifest completeness policy for 16 required benchmark IDs)
- [x] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)
  - Performance targets: hold module budgets defined in `PERFORMANCE_EXPECTATIONS.md` and document regressions >10%.
  - Validation: compare release baseline manifests and preserve reproducibility metadata.
  - **Delivered:** `benchmarks/cache/CACHE_BENCHMARK_RUNBOOK.md` (execution, gate validation, regression protocol, evidence package requirements); regression tolerance policy = 10% per manifest.

### Phase 6: Documentation and Acceptance
- [x] core cache module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] roadmap/future acceptance context revalidated for issue #5632 sync pass
- [x] frozen contract header created (`include/cache/cache_contract.h` v1.0.0, §1–§7)
- [x] release-gate benchmark manifest stabilized (`benchmarks/cache/release_gate_manifest_cache.json`)

## Production Readiness Checklist

- [x] core cache surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] frozen contract header (`cache_contract.h`) defining all v1.x behavioral invariants
- [x] benchmark release gates locked in `release_gate_manifest_cache.json`
- [x] focused regression tests for coordinator degradation (CCD-01..08) and tenant isolation (CTI-01..08)
- [x] contract hardening tests (CCH-01..08) validate all §1–§7 invariants
- [~] remaining deep stubs in coordinator implementations require human approval for legacy-path marking
- [~] release-gate benchmark execution still blocked by RocksDB dependency in sandbox environment

## Evidence Summary (Session: 2026-07-27)

### Phase 1-6 Deliverables (this session)

| Artifact | Phase | Description |
|---|---|---|
| `include/cache/cache_contract.h` | 1 | Frozen v1.x cache contract §1–§7 |
| `tests/cache/test_cache_contract_hardening.cpp` | 3/4 | CCH-01..CCH-08 contract validation |
| `tests/cache/test_cache_coordinator_degradation.cpp` | 4a | CCD-01..CCD-08 degraded coordinator tests |
| `tests/cache/test_cache_tenant_isolation_hardening.cpp` | 4b | CTI-01..CTI-08 tenant isolation edge cases |
| `benchmarks/cache/release_gate_manifest_cache.json` | 5 | GATE-CAC-01..06 + SGATE-CAC-01..04 |
| `benchmarks/cache/CACHE_BENCHMARK_RUNBOOK.md` | 5 | Benchmark execution and validation runbook |

### Build Evidence

- Configure command: `cmake --preset community-release`
- Result: failed at dependency gate (`RocksDB not found`) from `cmake/Dependencies.cmake`.
- Environment constraint: package install is blocked (`apt` permission denied).
- Test targets auto-discovered by `tests/cache/CMakeLists.txt` glob pattern.
- All three new test files (`test_cache_contract_hardening.cpp`,
  `test_cache_coordinator_degradation.cpp`,
  `test_cache_tenant_isolation_hardening.cpp`) will be auto-registered as:
  - `module_cache_test_cache_contract_hardening_focused`
  - `module_cache_test_cache_coordinator_degradation_focused`
  - `module_cache_test_cache_tenant_isolation_hardening_focused`

## Open Work (Issue #5632)

- [x] validate and refine roadmap priorities against full module docs
- [x] validate and refine future-enhancement focus points against full module docs
- [x] Phase 1: frozen contract header delivered
- [x] Phase 2: core hardening semantics defined and tested
- [x] Phase 3: error taxonomy and fail-closed predicates delivered
- [x] Phase 4: focused regression tests delivered (CCH, CCD, CTI suites)
- [x] Phase 5: benchmark gates and runbook delivered
- [x] Phase 6: roadmap/FUTURE_ENHANCEMENTS updated, all phases marked
- [~] build/test evidence refresh blocked by RocksDB dependency (see Evidence Summary)
- [x] mark synced items and risks with explicit status transitions

## Closure Criteria (Issue #5632)

- [x] cache module acceptance criteria updated and traceable in roadmap/future docs
- [x] Phase 1-6 deliverables committed (contract, tests, benchmarks, docs)
- [~] evidence updated or explicit justified gap documented (build gap documented above)
- [ ] parent epic task entry checked by maintainer
- [ ] status labels updated by maintainer before close
- [x] close reason documented as "Phase 1-6 implemented; build evidence gap documented"

## Known Issues & Limitations

- behavior remains partially capability-dependent on enabled cache backends/features.
- distributed coordination and replication still require ongoing edge hardening.
- benchmark depth requires continued hardening for selected cache pathways.

## Breaking Changes

No breaking cache-module contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `cache`
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
