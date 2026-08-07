# Chimera Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-27 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production adapter runtime (v0.0.47, 96/100 maturity score) exists for the current ThemisDB adapter implementation, including simulation-mode behavior and conditional engine-dispatch integration surfaces. Core module documentation is aligned to source-verifiable behavior. Build system corrected 2026-07-27 (CMakeLists.txt).

## In Progress

- [~] hardening parity between simulation-mode and engine-backed dispatch paths (Target: Q3 2026, evidence: 2200+ test LOC)
- [~] benchmark stabilization for adapter request/response compatibility pathways (Target: Q3 2026)
- [~] diagnostics consistency improvements for capability and dispatch failure classes (Target: Q3 2026)
- [x] v1.0.0: Production ThemisDB Adapter Integration core surfaces (delivered 2026-07-18)
- [~] v1.1.0: Transaction Management with ACID properties and savepoints (Target: Q3 2026)
- [~] v1.1.0: Error Recovery with exponential backoff retry strategy (Target: Q3 2026)
- [~] v1.1.0: Batch Operation Optimization for throughput (Target: Q3 2026)
- [ ] v1.2.0: MongoDB/Qdrant/Neo4j Real Driver Integration (Target: Q4 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior across adapter dispatch edge permutations (Target: Q4 2026)
- [ ] expand regression coverage for engine-availability and fallback behavior (Target: Q4 2026)
- [ ] improve operator diagnostics for adapter contract and runtime mismatch incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline adapter p95/p99 envelopes for release-profile compatibility paths (Target: Q1 2027)
- [ ] add dedicated chimera-native benchmark coverage for adapter operations (Target: Q1 2027)
- [ ] evaluate modular multi-vendor adapter expansion strategy within src/chimera (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] freeze adapter contract semantics for active major line (Target: Q3 2026)
- [ ] define explicit error taxonomy for connection/capability/dispatch failures (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete hardening for lifecycle and dispatch internals (Target: Q4 2026)
- [ ] align simulation and engine-backed behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-closed behavior for invalid connection/dispatch states (Target: Q4 2026)
- [ ] unify diagnostics across capability mismatch and unsupported-path classes (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for adapter lifecycle and dispatch edge scenarios (Target: Q4 2026)
- [ ] extend deterministic fixture coverage for engine-injection permutations (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for adapter compatibility hot paths (Target: Q4 2026) — GATE-CHM-01..06 benchmarks delivered (2026-08-07)
- [~] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026) — Benchmark execution in progress (2026-08-07)

### Phase 6: Documentation and Acceptance
- [x] core chimera module docs aligned to source-verifiable behavior (completed 2026-07-18)
- [x] roadmap/future planning separated from historical changelog entries (completed 2026-07-18)
- [x] build system corrected and test infrastructure validated (completed 2026-07-27)
- [x] module acceptance criteria documented and traceable (completed 2026-08-07)
- [x] Doxygen metadata: v0.0.47, 96/100 maturity score, production-ready classification (completed 2026-08-07)

## Production Readiness Checklist

- [x] core chimera surfaces documented and source-verified (completed 2026-07-18)
- [x] module-level security and failure behavior documented (completed 2026-07-18)
- [x] benchmark mapping documented in performance expectations (completed 2026-07-18)
- [x] build system validated and test infrastructure corrected (completed 2026-07-27)
- [~] remaining hardening tasks in progress for dispatch parity and edge cases (target Q4 2026)
- [~] release benchmark stabilization in progress (target Q4 2026, framework ready 2026-08-07)
- [x] focused test suite available: 2200+ LOC across 3 test files (test_themisdb_adapter.cpp, test_chimera_streaming.cpp, test_chimera_prepared_statements.cpp)

## Known Issues and Limitations

- current source layout contains the ThemisDB adapter implementation only; third-party adapters (MongoDB, Qdrant, Neo4j) are on roadmap for v1.2.0.
- behavior parity for all engine-backed dispatch paths requires continued hardening; simulation paths are fully functional and tested.
- benchmark depth remains limited for chimera-native adapter pathways; compatibility pathways are validated.
- RocksDB dependency required for community-release builds (librocksdb-dev); linux-release preset requires vcpkg.

## Breaking Changes

No breaking chimera-module contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.