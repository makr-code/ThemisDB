# Document Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-10-31 (source/configuration inspection; no tests run) -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · DEVELOPMENT_STATUS_2026_07_28.md -->

## Current Status

Document runtime interfaces and in-memory/reference implementations exist for store/manager, lifecycle, schema validation, diff/merge, XDOMEA, diagnostics, encrypted-entity helpers, and store-backed round-trip snapshots. This source review does not certify production deployment readiness.

## Delivered source surfaces

- [x] hardening edge-case consistency for schema validation and merge conflict semantics (Target: Q3 2026)
- [x] benchmark source cases for in-memory store/list and diff/merge paths (Target: Q3 2026)
- [x] diagnostics consistency improvements for store and round-trip persistence failures (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [x] tighten deterministic conflict handling for multi-branch merge permutations (Target: Q4 2026)
- [x] expand regression coverage for schema sealing/version transition edge cases (Target: Q4 2026)
- [x] improve operator diagnostics for document round-trip and exchange failures (Target: Q4 2026)

### Mid-term (6-12 months)
- [~] re-baseline measured latency/throughput envelopes on representative hardware (Target: Q1 2027)
  - The `DOC-BM-01..04` source emits thresholds for synthetic stubs; it is not production-path p95/p99 evidence. See PERFORMANCE_EXPECTATIONS.md.
- [x] add dedicated benchmark coverage for diff/merge and round-trip workflows (Target: Q1 2027)
  - Delivered as in-memory interface benchmarks in `bench_document_diff_merge.cpp` and `bench_document_round_trip.cpp`; hardware baselines remain separate evidence.
- [~] harden long-running reliability under document churn and conflict-heavy workloads (Target: Q1 2027)
  - Soak test created: tests/integration/test_document_store_soak.cpp (2026-09-16); full 60-min hardware run pending Wave D sign-off

## Implementation Phases

### Phase 1: Design / API Contract
- [x] document store/manager/lifecycle/schema interfaces and error behavior (Target: Q3 2026; this does not assert ABI or source-compatibility guarantees)
- [x] define explicit error taxonomy for not-found, conflict, and schema-violation classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [x] complete hardening for manager orchestration and round-trip persistence internals (Target: Q4 2026)
- [x] align schema/merge behavior to bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [x] expose explicit errors/reports for invalid arguments, missing schemas, and merge conflicts (Target: Q4 2026)
- [x] unify diagnostics across store, lifecycle, merge, and exchange failure paths (Target: Q4 2026)

### Phase 4: Tests
- [x] expand focused regressions for schema, diff/merge, and round-trip edge scenarios (Target: Q4 2026)
- [x] extend deterministic fixture coverage for XDOMEA and multi-version document cases (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [x] define source-level benchmark checks for in-memory store, diff/merge, and round-trip paths (Target: Q4 2026)
- [~] validate measured latency/throughput against representative release baselines (Target: Q4 2026)
  - TODO: attach benchmark output and the release workflow's gate-consumption evidence; the source definitions alone do not prove a baseline run.

### Phase 6: Documentation and Acceptance
- [x] core document module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core document surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] remaining hardening tasks closed for schema/merge/round-trip edge paths
- [~] representative-hardware benchmark stabilization complete

## Known Issues and Limitations

- module implementation is largely header-first with selected src-level persistence wiring.
- representative-hardware benchmark output and full soak execution/sign-off are not established by this source/configuration review.
- the four `DOC-BM` serialization-gate cases are synthetic stubs; do not treat them as production-path measurements.

## Breaking Changes

No breaking change is identified by this source review; planned compatibility policy is not established by the headers. Any approved contract-breaking change requires migration notes and a changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable. This roadmap does not independently certify CI status or Wave D sign-off.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `document`
- [~] Validate applicable high-cardinality stress and operator guidance (Target: Q1 2027)
  - Source artifacts exist: `tests/document/test_document_highcardinality_stress.cpp` and `docs/operability/RUNBOOK_DOCUMENT_STORE.md`. Execution and operational sign-off are not verified here.
- [~] Validate long-duration soak coverage for primary paths (Target: Q1 2027)
  - `tests/integration/test_document_store_soak.cpp` exists. TODO: attach a completed run record and reviewer sign-off.
- [x] Provide a module runbook artifact for operator-critical scenarios (Target: Q1 2027)
  - `docs/operability/RUNBOOK_DOCUMENT_STORE.md` exists; this file's presence is not proof of alert/trace integration.

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] Distributed/acceleration criteria: no such document-module path was identified in the reviewed headers or implementation. TODO: confirm applicability with the program owner before assigning a status.
- [~] Representative-hardware benchmark baselines exist (Target: Q1 2027)
  - Threshold definitions exist; representative-hardware results and sign-off remain TODO.
- [~] Operator-critical paths have verified diagnostics/alerts/runbook integration (Target: Q1 2027)
  - Runbook exists; alert and trace wiring require confirmation in the owning integration sources.

### Wave D Closure Batch (2026-09-16)

Artifacts present at the paths below; this is not an execution/sign-off record:
- `tests/integration/test_document_store_soak.cpp` — soak test source.
- `tests/document/test_document_highcardinality_stress.cpp` — stress test source.
- `docs/operability/RUNBOOK_DOCUMENT_STORE.md` — operator runbook source.
- `benchmarks/document/bench_document_serialization_gates.cpp` — four synthetic stub cases that emit threshold counters, not production-path p95 measurements.
