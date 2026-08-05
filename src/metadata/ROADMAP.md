# Metadata Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-08-04 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production metadata runtime exists across schema discovery, consistency tooling, lineage/export behavior, distributed metadata surfaces, and information-schema/statistics support.

## In Progress

- [~] re-baseline p95/p99 envelopes for metadata access and cache operations (Target: Q1 2027)
- [~] broaden benchmark depth beyond cache-centric metadata hot paths (Target: Q1 2027)
- [~] harden long-running reliability under sustained schema mutation pressure (Target: Q1 2027)

## Planned Features

### Mid-term (6-12 months)
- [ ] expand deep concurrent-access stress coverage beyond focused unit tests (Target: Q2 2027)
- [ ] add operator runbook for metadata incident triage with actionable diagnostic steps (Target: Q2 2027)

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
- [x] Phase A: schema churn stress + SchemaVersionManager lock contract (MCH-S01..S08, MCH-L01..L04)
- [x] Phase B: consistency edge cases + lineage traversal + export failure paths (MCH-C01..C08, MCH-LN01..LN04, MCH-EX01..EX04)
- [x] Phase C: distributed catalog diagnostics + RBAC diagnostics (MCH-DC01..DC04, MCH-SEC01..SEC04)

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for metadata hot paths (Target: Q4 2026)
- [x] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)
- [x] Phase D: broaden benchmarks to consistency/lineage hot paths (GATE-MCL-01..04)
- [x] fix blocking_no_timeout gaps in schema_manager: removed unbounded shared-lock re-acquisition in getTable/getAllRelationships/getDatabaseMetadata; read under existing write lock instead (2026-08-04)
- [x] fix circular_lock_ordering / deadlock_risk in statistics_collector: release refresh_mutex_ before acquiring cache_mutex_ in refreshLoop_ (2026-08-04)

### Phase 6: Documentation and Acceptance
- [x] core metadata module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] benchmark README extended with full gate table (GATE-MET-01..04, GATE-MCL-01..04)
- [x] fix schema_consistency_checker: removed const_cast in runCheck() — results_mutex_ already declared mutable (2026-08-04)

## Production Readiness Checklist

- [x] core metadata surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] Phase 2/3 hardening closed: ConsistencyIssue diagnostics, ColumnRef contracts, fail-safe edge paths (MCH-01..MCH-08)
- [x] Phase A–D tests and benchmarks delivered and registered (MCH-S/L/C/LN/EX/DC/SEC + GATE-MCL-01..04)
- [x] release benchmark stabilization complete
- [x] CRITICAL gap closure: blocking_no_timeout (schema_manager), circular_lock_ordering (statistics_collector), const_cast (schema_consistency_checker) — resolved 2026-08-04

## Known Issues and Limitations

- runtime behavior depends on schema scale, cache state, and configured integration/export paths.
- deep concurrent-access stress beyond focused unit level still expanding (Q2 2027).
- operator runbook for metadata incident triage not yet written (Q2 2027).
- remaining gap scanner findings (scope_mismatch, string_concat_loop, copy_overhead, range_temporary) are MEDIUM/LOW severity and deferred to Q2 2027 maintenance sweep.

## Breaking Changes

No breaking metadata contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.