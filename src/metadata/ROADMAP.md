# Metadata Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-08-03 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production metadata runtime exists across schema discovery, consistency tooling, lineage/export behavior, distributed metadata surfaces, and information-schema/statistics support.

## In Progress

- [~] tighten deterministic behavior under high metadata churn and concurrent access scenarios (Target: Q4 2026)
- [~] expand stress coverage for consistency/lineage/export edge cases (Target: Q4 2026)
- [~] improve operator-facing diagnostics for metadata incident triage (Target: Q4 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] re-baseline p95/p99 envelopes for metadata access and cache operations (Target: Q1 2027)
- [ ] broaden benchmark depth beyond cache-centric metadata hot paths (Target: Q1 2027)
- [ ] harden long-running reliability under sustained schema mutation pressure (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze schema/consistency/export contracts for active major line (Target: Q3 2026)
- [x] define explicit error taxonomy for schema/export/distributed metadata failure classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [x] complete hardening for metadata orchestration internals (Target: Q4 2026)
- [x] align consistency/lineage/export behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-safe behavior for malformed schema and export failure scenarios (Target: Q4 2026)
- [x] unify diagnostics across schema/consistency/export incidents (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for metadata consistency and export edge scenarios (Target: Q4 2026)
- [x] extend deterministic stress fixtures for metadata cache and schema mutation operations (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for metadata hot paths (Target: Q4 2026)
- [x] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core metadata module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core metadata surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] Phase 2/3 hardening closed: ConsistencyIssue diagnostics, ColumnRef contracts, fail-safe edge paths (MCH-01..MCH-08)
- [x] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on schema scale, cache state, and configured integration/export paths.
- deterministic stress coverage under high metadata churn still expanding (Q4 2026).
- benchmark breadth should continue expanding beyond metadata-cache dominant paths.

## Breaking Changes

No breaking metadata contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.