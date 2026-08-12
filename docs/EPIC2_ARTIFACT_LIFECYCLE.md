## Status: Stale – Archivierungskandidat
> **Hinweis (2026-08-12):** Diese Datei enthält TODO/FIXME/STALE/TBD/PLACEHOLDER-Marker und wird als Archivierungskandidat geführt. Inhalte wurden nicht gelöscht. Für den aktuellen Stand bitte kanonische Quellen und den [Root-Index](00_DOCUMENTATION_INDEX.md) konsultieren.
<!-- stale-marker: DOC-WEEKLY-2026-33 -->


# EPIC 2.6 Artifact Lifecycle

<!-- Status: implemented | Issue #5442 | validated: 2026-07-18 -->

## Summary

Derived artifact lifecycle and staleness policy for retrieval and tensor artifacts.

## Scope

- Source-of-truth versus derived artifact classes
- Refresh windows, invalidation triggers, and rebuild priorities
- Lifecycle signals consumed by planner and recovery flows

## Implemented Repository Surfaces

| File | Purpose |
|---|---|
| `src/evaluation/include/artifact_lifecycle.h` | Core API: LifecycleState, InvalidationReason, StalenessPolicy, ArtifactLifecycleManager |
| `src/evaluation/src/artifact_lifecycle.cc` | Implementation of lifecycle state machine and staleness detection |
| `tests/epic2_evaluation/artifact_lifecycle_test.cc` | 58 comprehensive unit tests covering all phases |
| `benchmarks/epic2_evaluation/artifact_staleness_bench.cc` | Performance benchmarks for staleness detection and batch operations |

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [x] Define freshness and staleness metadata shared across all epics
- [x] Lifecycle states: PRISTINE, READY, STALE, INVALIDATED, REBUILDING, FAILED
- [x] Invalidation reasons: INTEGRITY_CHECK_FAILED, STALENESS_EXCEEDED, SOURCE_INVALIDATED, POLICY_VIOLATION, etc.
- [x] StalenessPolicy with configurable thresholds (age, delta lag, residual, rank cap, variance)

### Phase 2: Core implementation
- [x] ArtifactLifecycleManager with state computation and transition logic
- [x] Staleness detection hooks for age, delta lag, and residual metrics
- [x] Batch operations for large-scale artifact scanning
- [x] Document lifecycle ownership and background rebuild expectations

### Phase 3: Error handling and edge cases
- [x] Terminal state preservation (INVALIDATED, REBUILDING, FAILED remain unchanged)
- [x] Staleness diagnosis with root cause identification
- [x] Edge cases: empty policies, multiple threshold violations, PRISTINE→READY transition

### Phase 4: Tests
- [x] 58 comprehensive unit tests in `artifact_lifecycle_test.cc`
- [x] State transition tests (all 6 state paths)
- [x] Staleness detection tests (all threshold types)
- [x] Batch operation tests (compute, filter, identify)
- [x] String conversion and utility tests

### Phase 5: Performance and hardening
- [x] Benchmarks in `artifact_staleness_bench.cc` with single-artifact and batch scenarios
- [x] Throughput targets: 1M artifacts/second for state computation
- [x] Latency targets: < 1 µs per artifact for single-state computation
- [x] Align rebuild semantics with EPIC 3 recovery and integrity documents

### Phase 6: Documentation and acceptance
- [x] API documentation in header comments (Doxygen-style)
- [x] Integration examples in usage section of header
- [x] Alignment with EPIC 2.1 (hardware profiles) and EPIC 2.5 (query planner)
- [x] Acceptance signals verified

### Phase 7: Integration
- [x] Wire `artifact_lifecycle.h/cc` into `src/evaluation/CMakeLists.txt`
- [x] Create `epic2_artifact_lifecycle_lib` library target
- [x] Add to `themis_evaluation` public headers
- [x] Integration verified with other EPIC 2 components

## Acceptance Signals

- [x] LifecycleState and InvalidationReason enums fully defined and documented
- [x] StalenessPolicy builder pattern supports flexible threshold configuration
- [x] ArtifactLifecycleManager state machine is deterministic and reversible
- [x] Batch operations scale linearly with artifact count (O(n) complexity)
- [x] All tests pass; 58 coverage cases across all phases
- [x] Benchmark suite demonstrates < 1 µs/artifact latency

## Production Readiness Checklist

- [x] Artifact lifecycle API is stable and complete
- [x] Staleness detection logic is tested and hardened
- [x] Batch operations support distributed retrieval scenarios (1000+ artifacts)
- [x] Integration hooks defined for query planner and recovery manager
- [x] Performance targets met (1M artifacts/s, < 1 µs/artifact)
- [x] Documentation complete with examples and edge case handling
- [x] All tests passing; no compilation warnings

## Known Issues & Limitations

- Rebuild semantics (transition to REBUILDING, then READY/FAILED) are asynchronous; synchronization with actual rebuild completion is caller's responsibility
- PRISTINE state is internal; artifacts should enter READY or STALE before use by planner
- Staleness diagnosis returns first-matched threshold, not all exceeded thresholds (use diagnoseStalenessCause for details)

## Breaking Changes

- None. ArtifactLifecycleManager is a new API; no existing code modified.

## References

- `HARDWARE_REQUIREMENTS.md`
- `docs/EPIC2_ARTIFACT_LIFECYCLE.md` (planning scaffold)
- `src/distributed_tensor/include/artifact_manifest.h` (LifecycleState enums)
- `src/evaluation/include/query_planner.h` (planner routing integration)

