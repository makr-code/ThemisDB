# Metadata Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: planning snapshot reconciled 2026-10-31; release measurements not revalidated -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

**Author:** ThemisDB Contributors  
**Created:** 2026-10-31  
**Last Updated:** 2026-10-31  
**Status:** review  
**AI_Used:** true  
**AI_Models:** unknown  
**AI_Contributor:** GitHub Copilot coding agent / ThemisDB documentation orchestrator  
**Human_Review:** false  
**Human_Author:** ThemisDB Contributors  
**Source level:** level1 — module-local  
**SOT domain:** module-behavior  
**Canonical source references:** [metadata benchmark mappings](./PERFORMANCE_EXPECTATIONS.md), [benchmark sources](../../benchmarks/metadata/), [module runbook](./RUNBOOK.md)  
**Related issue / milestone:** [#7362](https://github.com/makr-code/ThemisDB/issues/7362) · DOC-WEEKLY-2026-W44

> Exact model backend not verifiable from final document text alone.

## Current Status

Production metadata runtime exists across schema discovery, consistency tooling, lineage/export behavior, distributed metadata surfaces, and information-schema/statistics support.

## In Progress

- [~] re-baseline p95/p99 envelopes for metadata access and cache operations (Target: Q1 2027)
- [~] broaden benchmark depth beyond cache-centric metadata hot paths (Target: Q1 2027)
- [~] harden long-running reliability under sustained schema mutation pressure (Target: Q1 2027)

## Planned Features

### Mid-term (6-12 months)
- [~] expand deep concurrent-access stress coverage beyond focused unit tests (Target: Q2 2027) — `tests/metadata/test_metadata_highcardinality_stress.cpp` (HC-MD-03) delivered; Q2 2027 extended coverage in progress
- [?] confirm whether the deployment runbook meets the planned incident-triage scope (Target: Q2 2027) — a module developer runbook exists at `RUNBOOK.md`; deployment command coverage and completion status are not verified here

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
- [~] maintain benchmark cases and gate targets for metadata hot paths (Target: Q4 2026) — case presence is documented; a passing release run is not established by this source review
- [?] validate p95/p99 and throughput behavior against recorded release baselines (Target: Q4 2026) — measurement output and baseline comparison not verified
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
- [?] release benchmark stabilization — pending recorded representative-hardware results and baseline evidence
- [x] CRITICAL gap closure: blocking_no_timeout (schema_manager), circular_lock_ordering (statistics_collector), const_cast (schema_consistency_checker) — resolved 2026-08-04

## Known Issues and Limitations

- runtime behavior depends on schema scale, cache state, and configured integration/export paths.
- deep concurrent-access stress beyond focused unit level still expanding (Q2 2027).
- a module developer runbook exists at `RUNBOOK.md`; deployment-runbook scope and the conflicting status claims in this roadmap and `FUTURE_ENHANCEMENTS.md` require follow-up.
- the older gap-scanner severity/count snapshot is undated and not revalidated;
  rerun/review it before using its findings or severity labels to prioritize the
  Q2 2027 maintenance sweep (see [`MODULE_GAPS.md`](./MODULE_GAPS.md)).

## Breaking Changes

No breaking metadata contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `metadata`
- [~] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027) — high-cardinality test source is present; deployment-runbook coverage is not verified here
- [x] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027) — `tests/integration/test_metadata_store_soak.cpp` delivered (MD-SOAK-01..03); registered in Wave D foreach with TIMEOUT 120
- [?] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027) — verify deployment-runbook coverage and status before sign-off

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [x] This module's distributed/acceleration paths fail closed (Target: Q1 2027) — partition inconsistency and index corruption paths tested in soak and stress suites
- [~] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027) — re-baseline in progress (Q1 2027); representative hardware sign-off pending
- [?] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027) — deployment evidence not reviewed for this update
