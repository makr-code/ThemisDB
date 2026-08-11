# Replication Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-capable replication runtime exists for orchestration, promotion/failover, conflict resolution, logical replication/CDC streaming, and replication observability.

## In Progress

- [~] hardening failover/promotion behavior under lag and contention (Target: Q3 2026)
- [~] improving conflict-resolution diagnostics consistency across strategy paths (Target: Q3 2026)
- [~] stabilizing benchmark-backed release guardrails for replication hot paths (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior under sustained replication backpressure (Target: Q4 2026)
- [ ] expand stress coverage for slot/stream/CDC edge scenarios (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for failover and lag incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for promotion, conflict, and CDC paths (Target: Q1 2027)
- [ ] broaden benchmark depth for advanced multi-tier and multi-writer scenarios (Target: Q1 2027)
- [ ] harden long-run reliability under sustained cross-node replication traffic (Target: Q1 2027)

### Distributed Maturity Phase 3 — Track 2 Items (Q3–Q4 2026)

These items are part of the next-phase **Track 2: Distributed Systems Maturity** plan
(see `ROADMAP.md §Track 2`). Hard gate per item: deterministic under-load benchmark + `release_critical` CI green.

#### 3.1 Replication

- [~] **Geographic replica placement policies**: extend `ReplicationManager` to accept placement
  constraints (region/zone affinity, anti-affinity, minimum copies per DC); reflect constraint in
  leader election and failover candidate selection (Target: Q3 2026) — basic multi-region enablement exists via `ReplicationManager::enableMultiRegion()`; configurable placement/election policy still pending
  - Inputs: placement policy DSL (JSON/YAML); DC topology map
  - Acceptance: leader election respects placement constraints in 3-DC topology test; failover
    selects candidate in the correct DC; deterministic benchmark confirms sub-50 ms election
- [ ] **Async cross-region WAL shipping with configurable lag limits**: replicate WAL segments to
  remote DCs asynchronously; operator-configurable lag limit (default 1 s); emit alert metric
  when lag limit exceeded (Target: Q3 2026)
  - Inputs: `replication.wal_shipping.max_lag_ms` config key; remote DC endpoint
  - Acceptance: WAL ship throughput ≥ 80 MB/s on GbE link; lag alert fires within 2× lag window;
    `replication_wal_lag_ms` Prometheus histogram wired

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze replication/slot/conflict contracts for current major line (Completed 2026-07-29)
- [x] define explicit error taxonomy for replication failure classes (Completed 2026-07-29)

### Phase 2: Core Implementation
- [ ] complete hardening for replication manager and failover internals (Target: Q4 2026)
- [ ] align logical replication/CDC behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior for promotion failures, lag spikes, and slot faults (Target: Q4 2026)
- [ ] unify diagnostics across orchestration/conflict/stream incident classes (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for failover and conflict-heavy edge scenarios (Completed 2026-07-29 — test_replication_contract_hardening_focused.cpp, RCH-01..RCH-16)
- [x] extend deterministic stress fixtures for replication backpressure and lag workloads (Completed 2026-07-29)

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for replication hot paths (Completed 2026-07-29 — bench_replication_release_gates.cpp, RRG-01..RRG-06)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core replication module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] comprehensive Doxygen documentation for all public APIs in replication manager, logical replication, and observability
- [x] focused conflict resolution test suite (RCS-01..RCS-06) validates strategy determinism and edge-case handling
- [x] replication_api_contract.h frozen contract header published (Completed 2026-07-29)

## Production Readiness Checklist

- [x] core replication surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] focused conflict resolution tests provide evidence for conflict-resolution diagnostics consistency
- [x] Doxygen coverage verified for all replication module headers and sources
- [x] replication_api_contract.h frozen contract header (Phase 1 closure, 2026-07-29)
- [x] test_replication_contract_hardening_focused.cpp — RCH-01..RCH-16 (Phase 4 closure, 2026-07-29)
- [x] bench_replication_release_gates.cpp — RRG-01..RRG-06 gate benchmarks (Phase 5 closure, 2026-07-29)
- [ ] remaining hardening tasks closed for failover/conflict/CDC edge paths
- [ ] release benchmark stabilization complete

## Evidence Summary (Session: 2026-07-19)

### Test Coverage
- Build Target: `module_replication_test_replication_conflict_focused_autofocused`
- Test Tier: unit, Timeout: 120s
- Test File: `tests/replication/test_replication_conflict_focused.cpp`
- Test Cases: 17 focused tests covering:
  - **RCS-01**: Three-Way Merge strategy (4 tests)
  - **RCS-02**: Field-Level Merge strategies (5 tests)
  - **RCS-03**: Conflict context semantics (2 tests)
  - **RCS-04**: Deterministic behavior (2 tests)
  - **RCS-05**: Edge cases (2 tests)
  - **RCS-06**: Diagnostics consistency (2 tests)

### Documentation Coverage
- **Doxygen Headers**: All 13 replication module headers updated with @file metadata
- **Replication Manager**: Enhanced with comprehensive @brief, @param, @return, @throws annotations
- **Logical Replication**: Full documentation of slot lifecycle, change streaming, and callbacks
- **Observability**: Verified comprehensive documentation for observer API
- **Conflict Resolution**: All strategy classes properly documented

### Compliance
- Documentation Enforcement: Applied per `.github/instructions/documentation-enforcement.instructions.md`
- C++ Best Practices: RAII, const-correctness, thread-safety emphasis
- Test Driven: Focused tests provide evidence for module acceptance criteria

## Known Issues and Limitations

- runtime behavior depends on topology, lag, and replication mode configuration.
- selected failover and high-contention edge scenarios need continued hardening.
- benchmark depth should continue expanding for advanced distributed replication workflows.

## Breaking Changes

No breaking replication contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.