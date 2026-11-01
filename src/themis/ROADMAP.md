---
Author: "ThemisDB Contributors"
Created: "2026-05-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Model_Attribution: "Exact model backend not verifiable from final document text alone."
AI_Contributor: "themisdb-doc-orchestrator"
Human_Review: false
Human_Author: "unknown"
---

# Themis Core Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

The source tree contains implementations for build identity, edition/license
gating, module loading and verification, dependency resolution, and wire-server
operation. This inventory does not establish module-level production
readiness; root `ROADMAP.md` remains authoritative for aggregate release
readiness and blockers.

> **Evidence boundary (2026-10-31):** Current source, test, and benchmark
> registration was checked, but no build, test, soak, or benchmark execution
> was performed. A test/benchmark file's presence is not evidence that a
> checklist item passed. Q3 2026 status labels and target dates below were not
> independently revalidated and should be treated as unresolved where they
> require run artifacts or sign-off.

## In Progress

- [?] hardening loader/verification behavior under platform and trust-edge scenarios (Target: Q3 2026; status not revalidated)
- [?] improving diagnostics consistency across license, verification, and wire runtime stages (Target: Q3 2026; status not revalidated)
- [?] stabilizing benchmark-backed release guardrails for themis core hot paths (Target: Q3 2026; status not revalidated)

## Planned Features

### Short-term (3-6 months)
- [x] tighten deterministic outcomes for dependency and module trust edge cases (Target: Q4 2026) — evidence: tests/themis/test_themis_highcardinality_stress.cpp (THSTR-03)
- [x] expand stress coverage for wire-session and loader contention scenarios (Target: Q4 2026) — evidence: tests/themis/test_themis_highcardinality_stress.cpp (THSTR-01..03)
- [x] improve operator-facing diagnostics for load/verify/gating incidents (Target: Q4 2026) — evidence: docs/operability/RUNBOOK_THEMIS_CORE.md

### Mid-term (6-12 months)
- [~] re-baseline p95/p99 envelopes for loader/gating/server-sensitive paths (Target: Q1 2027)
- [x] broaden benchmark depth for module lifecycle and wire runtime diversity (Target: Q1 2027) — evidence: benchmarks/themis/bench_themis_dedicated_gates.cpp (TH-BM-01..04)
- [x] harden long-run reliability under sustained core runtime pressure (Target: Q1 2027) — evidence: tests/integration/test_themis_core_soak.cpp (ThemisSoak_LoadVerifyThroughput, ThemisSoak_WireSessionStability, ThemisSoak_DependencyGatingReliability)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze themis core runtime/gating/lifecycle contracts for current major line (Target: Q3 2026)
- [x] define explicit error taxonomy for license/load/verify incident classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [x] complete hardening for loader/verifier and wire-server internals (Target: Q4 2026) — evidence: tests/themis/test_themis_highcardinality_stress.cpp
- [x] align platform-specific load behavior to bounded runtime contracts (Target: Q4 2026) — evidence: tests/themis/test_themis_highcardinality_stress.cpp THSTR-01

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-safe behavior for signature, dependency, and runtime gate faults (Target: Q4 2026) — evidence: docs/operability/RUNBOOK_THEMIS_CORE.md
- [x] unify diagnostics across license, lifecycle, and wire incident classes (Target: Q4 2026) — evidence: RUNBOOK_THEMIS_CORE.md alert→runbook mapping

### Phase 4: Tests
- [x] expand focused regressions for loader/verify and wire-session edge scenarios (Target: Q4 2026)
- [x] extend deterministic stress fixtures for core runtime concurrency (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [?] lock benchmark-backed release gates for themis core hot paths (Target: Q4 2026; no run artifact was checked)
- [?] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026; no run artifact was checked)

### Phase 6: Documentation and Acceptance
- [x] core themis module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core themis surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [?] remaining hardening tasks closed for loader/gating/wire edge paths — test source exists, but passing execution evidence was not checked
- [?] release benchmark stabilization complete — benchmark sources are registered, but no baseline or run result was checked

## Known Issues and Limitations

- runtime behavior depends on platform loader constraints and trust material configuration.
- selected verification and wire-session edge scenarios need continued hardening.
- benchmark depth should continue expanding for wider core runtime workloads.

## Breaking Changes

No breaking themis core contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `themis`
- [?] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027) — source artifacts exist; execution and acceptance were not checked
- [?] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027) — test source exists; registration and run results were not checked
- [?] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027) — runbook exists; current signal availability was not verified

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [?] This module's distributed/acceleration paths fail closed (Target: Q1 2027) — require path-specific test and configuration evidence
- [~] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [x] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027) — evidence: docs/operability/RUNBOOK_THEMIS_CORE.md
