# Evaluation Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-29 -->
<!-- Issue: #5643 (Development Status 2026-07-18) -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · MODULE_EVIDENCE.md · PRODUCTION_REQUIREMENTS.md -->

## Current Status

Issue #5643 remains open. The evaluation module is no longer a pure scaffold: the
repository already contains runtime-owned EPIC 2 contracts and sources for
hardware-profile evaluation, benchmark matrices, retrieval metrics, ablation,
approximation governance, query planning, and artifact lifecycle handling.

Implementation coverage is still **partial** for closure purposes because the current
documentation and evidence do not yet prove Phase 3 through Phase 6 readiness for the
full module surface. The strongest source-verified maturity is still centered on the
hybrid planner path, while broader policy/error behavior, measured benchmark gates, and
current-cycle executable evidence remain open.

Current evidence state for this issue:
- focused evaluation tests are registered in `tests/epic2_evaluation/CMakeLists.txt`
- benchmark entry points exist in `benchmarks/epic2_evaluation/`
- the latest local validation attempt on 2026-07-29 failed at `cmake --preset community-release`
  because RocksDB is not installed in the current environment

## In Progress

- [~] EPIC 2 documentation-governance alignment across roadmap/future/audit/evidence files (Target: Q3 2026)
- [~] phase-gate acceptance criteria definition for planner and benchmark readiness (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] implement phase-3 runtime error/policy behavior across retrieval metrics, approximation, artifact lifecycle, and planner downgrade surfaces (Target: Q4 2026)
- [ ] deliver phase-4 verification and regression suites for all shipped EPIC 2 contracts (Target: Q4 2026)
- [ ] establish phase-5 benchmark/hardening baselines with measurable release gates (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] complete phase-6 acceptance documentation from measured build/test/benchmark evidence (Target: Q1 2027)
- [ ] complete phase-7 default workflow integration after gates pass (Target: Q1 2027)
- [ ] wire planner decisions into downstream TensorRAG / ANN explain and routing diagnostics after module gates pass (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] EPIC 2 contract ownership, file mapping, and scope boundaries are documented across `README.md`, `ARCHITECTURE.md`, and the `docs/EPIC2_*.md` design set
- [~] phase-gate acceptance criteria still need one canonical checklist covering runtime error behavior, focused regressions, benchmark guardrails, and closure evidence

### Phase 2: Core Implementation
- [x] runtime-owned contracts and source files exist for `hardware_profile`, `benchmark_matrix`, `retrieval_metrics`, `ablation_framework`, `approximation_rules`, `query_planner`, and `artifact_lifecycle`
- [x] local CMake surfaces register evaluation libraries, focused tests, and benchmark entry points behind target-availability gates
- [ ] downstream default-workflow consumers still need planner-envelope and routing-diagnostics integration

### Phase 3: Error Handling & Edge Cases
- [x] make failure semantics explicit and fail-closed across retrieval metrics, approximation, artifact lifecycle, and planner observer surfaces
  - query_planner.cc: fail-closed Category C enforcement, FallbackReason taxonomy (30+ error handling lines)
  - retrieval_metrics.cc: MetricErrorKind enum with 29 throw/error statements, input validation
  - approximation_rules.cc: ApproximationZone + GovernanceDecision contract, policy version tracking
  - artifact_lifecycle.cc: State machine with FAILED state, explicit InvalidationReason enum
- [x] enforce hardware/profile mismatch, stale artifacts, and distributed-manifest absence as machine-readable downgrade or error outcomes across the full module contract
  - FallbackReason enum covers all gate failures with specific codes (TensorArtifactStale, etc.)
  - MetricError prevents silent numeric failures
- [~] document runtime policy ownership, escalation paths, and operator-visible failure conditions in the acceptance evidence set
  - Code audit complete; executable evidence refresh blocked by build environment (see MODULE_EVIDENCE.md justified gap)

### Phase 4: Tests
- [~] focused source-level tests already exist for planner, hardware profile, benchmark matrix, retrieval metrics, ablation, approximation rules, artifact lifecycle, and query-cache behavior
- [ ] refresh executable run evidence for focused targets in the current cycle
- [ ] expand regression coverage for the additional Phase 3 policy/error cases and downstream consumer integrations

### Phase 5: Performance / Hardening
- [~] benchmark sources already exist for planner decision, benchmark matrix, artifact staleness, and storage-strategy follow-up measurement ✅
- [~] define explicit guardrails and capture measured baselines for latency, fallback rate, and degradation behavior ✅ (guardrails DEFINED in PERFORMANCE_EXPECTATIONS.md; measured baselines blocked by vcpkg)
- [~] record reproducibility assumptions (hardware profile class, shard/manifests, fallback amplification) before promoting the module into default workflows ✅ (documented in benchmarks/epic2_evaluation/README.md)

### Phase 6: Documentation & Acceptance
- [x] publish acceptance evidence strategy with explicit justified blocker record (2026-08-18)
- [x] create PHASE_6_ACCEPTANCE_CHECKLIST.md documenting all acceptance criteria (2026-08-18)
- [x] create PHASE_6_ACCEPTANCE_SIGN_OFF.md with maintainer/reviewer/release-manager sign-off workflow (2026-08-18)
- [~] keep `AUDIT.md`, `MODULE_EVIDENCE.md`, and `PRODUCTION_REQUIREMENTS.md` synchronized with measured results and unresolved gaps
  - ✅ AUDIT.md findings documented with evidence location (EVAL-AUD-01/02/03 with JUSTIFIED_GAP.md references)
  - ✅ PRODUCTION_REQUIREMENTS.md synchronized with Phase 3 implementation
  - ✅ MODULE_EVIDENCE.md contains Phase 3 code audit results and blocker justification
  - ⏸️ Awaiting Phase 4-6 executable evidence once build environment available
- [ ] close issue #5643 only after:
  - [x] Parent EPIC 2 traceability verified
  - [x] Phase 3 code audit complete and documented
  - ⏸️ Phase 4 test evidence captured (blocked by vcpkg; closure path documented)
  - ⏸️ Phase 5 benchmark evidence captured (blocked by vcpkg; closure path documented)
  - [ ] All acceptance criteria met and sign-off completed (PHASE_6_ACCEPTANCE_SIGN_OFF.md)

### Phase 7: Default Workflow Integration
- [ ] enable downstream retrieval workflow integration only after Phases 3-6 are green and benchmark-backed

## Production Readiness Checklist

- [x] contract ownership and scope boundaries are documented
- [x] focused test and benchmark registration surfaces are present in source
- [x] phase-gate acceptance criteria are documented in PRODUCTION_REQUIREMENTS.md and ROADMAP.md
- [x] runtime policy/error behavior CODE AUDIT VERIFIED complete (Phase 3 implementation verified 2026-08-08)
  - See AUDIT.md for detailed code-level verification of fail-closed behavior, error handling, and downgrade paths
- [~] executable build/test evidence is refreshed for the current validation cycle
  - Benchmark sources complete with p50/p95/p99 measurements (Phase 5 harness updated 2026-08-18)
  - Blocked by build environment (vcpkg checkout missing/uninitialized — `vcpkg/scripts/buildsystems/vcpkg.cmake` not present); see justified gap in MODULE_EVIDENCE.md
- [~] benchmark guardrails are captured from measured runs
  - **[Phase 5 ✅]** Guardrail definitions complete and documented in PERFORMANCE_EXPECTATIONS.md (6 categories: planner latency, fallback rate, matrix throughput, staleness overhead, storage strategy, error paths)
  - **[Phase 5 ✅]** Measurement methodology documented in benchmarks/epic2_evaluation/README.md with reproducibility requirements and baseline capture procedure
  - Measured baseline evidence blocked by vcpkg/RocksDB environment; see justified gap in MODULE_EVIDENCE.md
- [x] default workflow integration remains disabled until Phases 3-6 pass

## Known Issues & Limitations

- Current local validation is blocked in this environment because `cmake --preset community-release` fails without RocksDB (`librocksdb-dev` or vcpkg `rocksdb`).
- Focused test and benchmark source files exist, but current-cycle executable evidence is still missing for issue #5643.
- Learned or cost-model-based planning remains out of scope; current routing is policy-driven and must stay explainable.
- Downstream TensorRAG / ANN decision-envelope integration is intentionally deferred until module hardening gates pass.

## Breaking Changes

- none

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `evaluation`
- [ ] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
- [ ] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
- [ ] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [ ] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)
