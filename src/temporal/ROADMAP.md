# Temporal Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-capable temporal runtime exists for temporal/bitemporal querying, system-versioned behavior, retention/snapshot lifecycle, and temporal indexing/CDC/compression paths.

## In Progress

- [~] hardening edge-case behavior under concurrent bitemporal write/query pressure (Target: Q3 2026)
- [~] improving diagnostics consistency across query/lifecycle/conflict stages (Target: Q3 2026)
- [~] stabilizing benchmark-backed release guardrails for temporal hot paths (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic behavior under retention and snapshot stress conditions (Target: Q4 2026)
- [ ] expand stress coverage for conflict-resolution and CDC edge scenarios (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for temporal lifecycle incidents (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for temporal query and history retrieval paths (Target: Q1 2027)
- [ ] broaden benchmark depth for advanced temporal workload shapes (Target: Q1 2027)
- [ ] harden long-run reliability under sustained temporal mutation/query traffic (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze temporal query/version contracts for current major line (Completed 2026-07-29)
- [x] define explicit error taxonomy for temporal lifecycle incident classes (Completed 2026-07-29)

### Phase 2: Core Implementation
- [ ] complete hardening for bitemporal/query and lifecycle internals (Target: Q4 2026)
- [ ] align CDC/compression/tiering behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior for snapshot, retention, and conflict faults (Target: Q4 2026)
- [ ] unify diagnostics across query, lifecycle, and CDC incident classes (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for temporal query, snapshot, and retention edge scenarios (Completed 2026-07-29 — test_temporal_contract_hardening_focused.cpp, TCH-01..TCH-16)
- [x] extend deterministic stress fixtures for concurrent temporal workloads (Completed 2026-07-29)

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for temporal hot paths (Completed 2026-07-29 — bench_temporal_release_gates.cpp, TRG-01..TRG-06)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core temporal module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] temporal_api_contract.h frozen contract header published (Completed 2026-07-29)

## Production Readiness Checklist

- [x] core temporal surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] temporal_api_contract.h frozen contract header (Phase 1 closure, 2026-07-29)
- [x] test_temporal_contract_hardening_focused.cpp — TCH-01..TCH-16 (Phase 4 closure, 2026-07-29)
- [x] bench_temporal_release_gates.cpp — TRG-01..TRG-06 gate benchmarks (Phase 5 closure, 2026-07-29)
- [ ] remaining hardening tasks closed for temporal lifecycle edge paths
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on temporal workload shape and retention/snapshot configuration.
- selected lifecycle and conflict edge scenarios need continued hardening.
- benchmark depth should continue expanding for advanced temporal workloads.

## Breaking Changes

No breaking temporal contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.