# Document Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-28 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · DEVELOPMENT_STATUS_2026_07_28.md -->

## Current Status

Production document runtime exists across store/manager contracts, lifecycle hooks, schema evolution, diff/merge, XDOMEA connector interfaces, and store-backed round-trip snapshot editing.

## In Progress

- [~] hardening edge-case consistency for schema validation and merge conflict semantics (Target: Q3 2026)
- [~] benchmark stabilization for document serialization and document-list paths (Target: Q3 2026)
- [~] diagnostics consistency improvements for store and round-trip persistence failures (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] tighten deterministic conflict handling for multi-branch merge permutations (Target: Q4 2026)
- [ ] expand regression coverage for schema sealing/version transition edge cases (Target: Q4 2026)
- [ ] improve operator diagnostics for document round-trip and exchange failures (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for document serialization and list/read paths (Target: Q1 2027)
- [ ] add dedicated benchmark coverage for diff/merge and round-trip workflows (Target: Q1 2027)
- [ ] harden long-running reliability under document churn and conflict-heavy workloads (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] freeze store/manager/lifecycle/schema contracts for active major line (Target: Q3 2026)
- [x] define explicit error taxonomy for not-found, conflict, and schema-violation classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete hardening for manager orchestration and round-trip persistence internals (Target: Q4 2026)
- [ ] align schema/merge behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [~] standardize fail-closed behavior for invalid document and schema-transition scenarios (Target: Q4 2026)
- [~] unify diagnostics across store, lifecycle, merge, and exchange failure paths (Target: Q4 2026)

### Phase 4: Tests
- [~] expand focused regressions for schema, diff/merge, and round-trip edge scenarios (Target: Q4 2026)
- [ ] extend deterministic fixture coverage for XDOMEA and multi-version document cases (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [~] lock benchmark-backed release gates for document serialization and list/read hot paths (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core document module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core document surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for schema/merge/round-trip edge paths
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- module implementation is largely header-first with selected src-level persistence wiring.
- selected schema/merge edge paths require continued hardening and benchmark expansion.
- dedicated module-native benchmark breadth should grow beyond current proxy mappings.

## Breaking Changes

No breaking document-module contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.