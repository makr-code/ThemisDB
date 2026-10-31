# Distributed Tensor Module Roadmap

**Last Updated:** 2026-10-31
**Source level:** Level 1 — module implementation
**SOT domain:** module-behavior
**Canonical references:** `src/distributed_tensor/CMakeLists.txt`, `cmake/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/distributed_tensor/CMakeLists.txt`, `tests/epic3_distributed_tensor/CMakeLists.txt`, `tests/integration/CMakeLists.txt`, `benchmarks/CMakeLists.txt`, `benchmarks/distributed_tensor/CMakeLists.txt`, `benchmarks/epic3_distributed_tensor/CMakeLists.txt`
**Related:** Issue #7341

<!--
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot Coding Agent
Human_Review: false
Human_Author: unknown
Exact model backend not verifiable from final document text alone.
-->

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Source inventory reconciled: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Rollout Plan: ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §4 (Phases A–D) -->

## Current Status

The current module target is `themis_distributed_tensor`, built from the source
list in `src/distributed_tensor/CMakeLists.txt`. It declares nine public headers.
The target publicly requests `cxx_std_20`, matching the `std::span` API in
`tensor_storage_strategy.h`; CMake consumers linking the target inherit the
requirement, and direct header consumers must compile as C++20. Local CMake
files register three focused test executables and
one in-process benchmark under `tests/distributed_tensor/` and
`benchmarks/distributed_tensor/`. Separately, `tests/CMakeLists.txt` adds
`tests/epic3_distributed_tensor/`, whose CMake registers the EPIC 3
phase/contract sources and an integrity benchmark test; the integration CMake
includes the distributed-tensor soak source when present. The benchmark parent
configuration also adds `benchmarks/epic3_distributed_tensor/` and its
placement, integrity, recovery, retrieval, partial-refit, and summary-first
sources.

These registrations confirm configured source paths, not the historical test
counts or pass claims reported below. A diagnostic configure was attempted for
this follow-up but stopped at `cmake/Dependencies.cmake:727` because the
required `fmt` library was unavailable; no build tree or module targets were
generated. No module test or benchmark was run.

The source inventory establishes implementation surfaces, but not production
enablement, release readiness, or measured runtime behavior. Phase labels and
gate targets below remain planning/status records unless backed by current
source and separately verified evidence.

**Integrity/training source caveat:** The active library target compiles
`integrity_verification.cc`, where `simple_sha256_hash()` is FNV-1a demonstration
code, not SHA-256. Merkle-shaped structures built by that target are not
cryptographic integrity security. The separate nested SHA-256 implementation
is not listed in the module target's CMake sources. `TensorTrainingCoordinator`
executes registered worker callbacks synchronously in-process; it does not
provide networked or distributed worker coordination.

**Tensor-Update Rollout Track**: dynamic tensor-update infrastructure is staged across four
phases (A–D) in `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md` (issue #5468). Phase A
(manifest schema + advisory-only policy) is complete. Phase B (delta log + partial refit)
targets Q3 2026. Phase C (shard summary coordination) targets Q4 2026. Phase D (optional
acceleration) targets 2027+.

**Module readiness**: the module implementation and test/benchmark source
registrations are present. Phase 3/4 pass status, benchmark evidence,
acceptance, and production enablement require execution evidence and remain
unverified. Representative-hardware performance and production sign-off remain
pending.

## In Progress

- [~] EPIC 3 documentation-governance alignment across roadmap/future/audit files (Target: Q3 2026)
- [~] phase-gate acceptance criteria definition for distributed test/benchmark readiness (Target: Q3 2026)
- [~] Phase 5: Adapter-Distribution & Sharding-Kopplung — LoRA artifact distribution interfaces,
  RAID/Merkle proof engine, distribution receipts, shard snapshots, and recovery ordering (Issue #5418, Target: Q3 2026)
- [~] tensor artifact manifest schema: ArtifactManifest + ManifestStore wiring (Target: Q3 2026)
- [x] derived-artifact lifecycle and staleness management: LifecycleState FSM, ArtifactClassifier, manifest fields, invalidation manager (issue #5442, Target: Q3 2026)
- [x] phase-4 distributed contract and fault-path coverage expansion beyond the focused regression suite (Target: Q4 2026)

## Planned Features

### Tensor-Update Infrastructure Track (issue #5468)

#### Phase A — Exact-First Infrastructure (Q3 2026, start)
- [x] tensor delta log schema (DeltaLogEntry, DeltaWindow, TensorDeltaLog in src/distributed_tensor/include/tensor_delta_log.h; 309 LOC)
- [x] manifest schema committed with advisory-only artifact policy documented (Target: Q3 2026)
- [x] Prometheus freshness metrics wired (`tensor_freshness_age_seconds`, `tensor_delta_log_entries_total`) (Target: Q3 2026)
- [x] snapshot-based rebuild worker (src/distributed_tensor/src/snapshot_update_worker.cc; 524 LOC; patch/refit/rebuild thresholds)
- [x] exact graph fallback guarantee documented and tested (src/distributed_tensor/include/exact_graph_fallback.h; tests/epic3_distributed_tensor/test_tensor_rebuild_fallback.cpp)

#### Phase B — Local Tensor Maintenance (Q3 2026, weeks 6–9)
- [x] patch path for small tensor deltas (bounded delta window) (patch_threshold_pct_ in snapshot_update_worker.cc)
- [x] partial refit for bounded windows (refit_threshold_pct_ in snapshot_update_worker.cc)
- [x] rebuild fallback for unstable or large update sets (fallback rebuild in snapshot_update_worker.cc:139-146)
- [x] planner freshness gates (`tensor_freshness_age_seconds` threshold checks) (Prometheus metric wired)
- [x] ctest gate: `test_tensor_delta_log` (tests/epic3_distributed_tensor/test_tensor_delta_log.cpp)
- [x] ctest gate: `test_tensor_rebuild_fallback` (tests/epic3_distributed_tensor/test_tensor_rebuild_fallback.cpp)
- [x] ctest gate: `test_phase_b_edge_cases` (tests/epic3_distributed_tensor/test_phase_b_edge_cases.cpp) — Phase B comprehensive edge case testing
- [x] benchmark gate: `bench_tensor_partial_refit` (benchmarks/epic3_distributed_tensor/bench_tensor_partial_refit.cc) — Performance targets: p99 <= 30ms for bounded windows

**Phase B Implementation (2026-08-17 Completion)**:
- [x] Enhanced patch path with delta window bounds checking (sequence continuity, age validation, structural mutation detection)
- [x] Partial refit with state machine transitions (PARTIAL_REFITTED state, rank cap enforcement, residual thresholds)
- [x] Rebuild fallback triggering on:
  - [x] Delta log overflow detection (95% threshold on max entries)
  - [x] Instability detection heuristics (high mutation frequency, residual thresholds)
  - [x] Fail-closed behavior on manifest validation failure
- [x] Comprehensive edge case test suite (PBE-01..24): 24 tests covering patch bounds, overflow, instability, state transitions
- [x] Full benchmark suite with performance targets:
  - Patch path: p99 <= 5ms (10-50 entry windows)
  - Partial refit: p99 <= 30ms (100-3200 entry windows)
  - Rebuild fallback: p99 <= 100ms
  - Decision logic: < 100µs per call
- [x] Production-ready error handling: manifest validation, rank cap checks, residual enforcement
- [x] State machine transitions for all three paths (PATCHED, PARTIAL_REFITTED, REBUILT)

#### Phase C — Distributed Tensor Artifact Coordination (Q4 2026) ✅ COMPLETE
- [x] shard summary refresh with freshness timestamps (✅ Implemented 2026-08-17)
- [x] summary-first routing with escalation to exact-on-demand (✅ Implemented 2026-08-17)
- [x] exact-on-demand tensor fetch (replaces summary when accuracy required) (✅ Implemented 2026-08-17)
- [x] multi-shard freshness consensus (✅ Implemented 2026-08-17)
- [x] ctest gate: `module_epic3_distributed_tensor_tensor_shard_summary_focused` (✅ 41 tests 2026-08-17)
- [x] benchmark gate: `module_epic3_distributed_tensor_bench_summary_first_focused` (✅ 12 benchmarks 2026-08-17)

#### Phase D — Optional Acceleration (2027+)
- [~] CPU/GPU break-even validation for tensor operations (Target: 2027)
- [~] bounded GPU refinement after break-even proven (Target: 2027)
- [ ] no unconditional GPU dependency on any tensor path (Target: 2027)

### Short-term (3-6 months)
- [x] implement phase-3 runtime failure and degraded-mode semantics (Target: Q4 2026)
- [x] deliver phase-4 distributed contract and fault-path tests (Target: Q4 2026)
- [x] establish phase-5 distributed benchmark and hardening baselines (Wave D: bench_distributed_tensor_dedicated_gates.cpp — DT-BM-01..04 delivered 2026-09-16)

### Mid-term (6-12 months)
- [ ] complete phase-6 acceptance documentation from measured evidence (Target: Q1 2027)
- [ ] complete phase-7 default workflow integration after gates pass (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] EPIC 3.1 (Artifact Classes) contract ownership mapped and documented
- [x] EPIC 3.2 (Manifest Schema) contract ownership mapped and documented
- [x] EPIC 3.3 (Shard Placement) contract ownership mapped and documented
- [x] EPIC 3.4 (Integrity Model) contract ownership mapped and documented
- [x] tensor-update rollout track staged (Phases A–D) in `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md`
- [x] advisory-only artifact policy documented with invariant: tensor artifacts never replace graph-verified results (Target: Q3 2026)
- [x] derived-artifact lifecycle model: `LifecycleState` (READY/STALE/INVALIDATED/REBUILDING/FAILED), `TruthSemantic`, `ArtifactClass`, `RebuildState`, `UpdateMode`, `InvalidationReason` defined in `artifact_manifest.h` (issue #5442, Target: Q3 2026)
- [x] manifest lifecycle fields defined: `source_seq_start`, `source_seq_end`, `delta_lag`, `artifact_age_ms`, `residual`, `rank_cap`, `lifecycle_state`, `invalidation_reason` (issue #5442, Target: Q3 2026)
- [x] advisory-only enforcement: `ArtifactClassifier::isValidCombination` prevents DERIVED/EPHEMERAL from acquiring GROUND_TRUTH semantic (issue #5442, Target: Q3 2026)

### Phase 2: Core Implementation
- [x] production translation units and public headers implemented
- [x] `themis_distributed_tensor` wired into the active community build graph
- [x] integrity translation units created for EPIC 3.4
- [x] deterministic JSON hashing and manifest-facing proof/receipt serialization implemented
- [x] cached-receipt versus fresh-verification rules documented for query-path consumers
- [x] 3.4 runtime distributed failure semantics completed across recovery/planner integration
- [x] lifecycle transition helpers (`transitionToRebuilding`, `transitionToReadyAfterRebuild`, `transitionToFailed`) and planner freshness gate helper (`shouldRejectForPlanner`) implemented in `ArtifactInvalidationManager` (issue #5442, Target: Q3 2026)
- [x] Phase A: tensor delta log + manifest store wired (Target: Q3 2026) ✅ **TDL-01..18 in tests/epic3_distributed_tensor/test_tensor_delta_log.cpp**
- [x] Phase A: snapshot-based rebuild worker implemented (Target: Q3 2026) ✅ **RFB-01..10 in tests/epic3_distributed_tensor/test_tensor_rebuild_fallback.cpp**
- [x] Phase B: patch path + partial refit + rebuild fallback implemented (Target: Q3 2026) ✅ **2026-08-17 complete: tests/epic3_distributed_tensor/test_phase_b_edge_cases.cpp (24 tests), benchmarks/epic3_distributed_tensor/bench_tensor_partial_refit.cc**
- [x] Phase C: shard summary refresh + summary-first routing + exact-on-demand fetch implemented (Target: Q4 2026) ✅ **2026-08-17 complete: tests/epic3_distributed_tensor/test_tensor_shard_summary.cpp (41 tests), benchmarks/epic3_distributed_tensor/bench_tensor_summary_first.cc (12 benchmarks)**

### Phase 3: Error Handling and Edge Cases
- [x] runtime distributed failure semantics implemented and verified
- [?] Hash/receipt mismatch handling is present, but the active implementation uses an FNV-1a demonstration hash and does not establish cryptographic tamper resistance.
- [x] partial-receipt and stale-receipt edge case handling implemented
- [x] recovery coordination hooks implemented (EPIC 3.5 integration ready)
- [x] Phase B: rebuild fallback triggered on delta log overflow and instability (Target: Q3 2026) ✅ **2026-08-17: delta log overflow detection (95% threshold), instability detection heuristics, fail-closed behavior**
- [x] Phase C: exact-on-demand fallback when summary freshness threshold exceeded (Target: Q4 2026) ✅ **2026-08-17: multi-shard freshness consensus, exact-on-demand escalation logic**

### Phase 4: Tests
- [x] focused Phase 3 regression suite implemented in `tests/epic3_distributed_tensor/`
- [x] broaden contract and fault-path coverage across all EPIC 3 components — `test_phase4_contract_coverage.cpp` (58 tests, sub-issues 3.1–3.7)
- [?] Historical roadmap entries report SHA-256, Merkle, and receipt-chain tests. Test-source presence/pass status was not revalidated here, and such tests do not establish cryptographic behavior for the active module target's FNV-1a demonstration hash.
- [?] Historical roadmap reported full verification tests; current behavior and pass status are unverified, and the active FNV-1a demonstration hash does not establish cryptographic integrity.
- [?] Integrity test and benchmark sources are configured, but their results and cryptographic behavior are unverified.
- [x] lifecycle & staleness management unit tests — `test_lifecycle_staleness_management.cpp` (31 tests, LSM-01..LSM-31, issue #5442)
- [x] `test_tensor_delta_log` — Phase A ctest gate ✅ **Done; TDL-01..TDL-18, TDL-A1..A3 (tests/epic3_distributed_tensor/test_tensor_delta_log.cpp)**
- [x] `test_tensor_rebuild_fallback` — Phase A/B ctest gate ✅ **Done; RFB-01..RFB-10, RFB-A1..A5 (tests/epic3_distributed_tensor/test_tensor_rebuild_fallback.cpp)**
- [x] `test_phase_b_edge_cases` — Phase B edge case ctest gate ✅ **Done; PBE-01..PBE-24 (24 tests, tests/epic3_distributed_tensor/test_phase_b_edge_cases.cpp)**
- [x] `test_tensor_shard_summary` — Phase C ctest gate ✅ **Done; 41 tests (tests/epic3_distributed_tensor/test_tensor_shard_summary.cpp), 2026-08-17 completion**

### Phase 5: Performance and Hardening
- [~] Adapter-Distribution & Sharding-Kopplung: LoRA artifact distribution APIs,
  RAID/Merkle proof engine, distribution receipts, shard snapshots, recovery ordering (Issue #5418)
- [x] benchmark suite implemented in `benchmarks/epic3_distributed_tensor/`
- [x] benchmark suite implemented in `tests/epic3_distributed_tensor/integrity_verification_bench.cc`
- [~] performance optimization with hardware acceleration (SHA-NI, AVX-512)
- [ ] alignment with graph provenance performance targets
- [x] `bench_tensor_partial_refit` — Phase B benchmark gate ✅ **Done; 8 benchmarks with performance targets: p99 <= 30ms for 100-3200 entry windows, 2026-08-17 completion**
- [x] `bench_tensor_summary_first` — Phase C benchmark gate ✅ **Done; 12 benchmarks validating summary-first routing and exact-on-demand escalation, 2026-08-17 completion**

### Phase 6: Documentation and Acceptance
- [x] core distributed tensor docs aligned to source-verifiable implementation state
- [x] tensor-update rollout track documented in `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md` (issue #5468)
- [~] acceptance docs tied to measured runtime behavior ⏳ **Phase 6 Acceptance Checklist updated 2026-08-17; benchmark evidence collection pending**

### Phase 7: Production Integration
- [ ] default pipeline integration enabled after gates are met
- [~] Phase A production enable: manifest schema + advisory policy validation gates pending (Target: Q3 2026)
- [~] Phase B production enable: rebuild fallback & instability detection gates pending (Target: Q3 2026)
- [~] Phase C production enable: shard summary coordination gates pending (after sharding Phase C gates at 70% ready) (Target: Q4 2026)

## Production Readiness Checklist

- Current source registration is summarized below; test-pass and benchmark-pass
  status is not established by source presence.
- Earlier checklist entries for Phase A/B/C tests, benchmark outcomes, and
  dependency gap percentages are dated roadmap assertions. Their cited
  `epic3_distributed_tensor` paths/targets are absent from the current local
  module CMake inventories, so those status values are unresolved as current
  readiness evidence.

## Current Test and Benchmark Registration

The current configured inventory is maintained in
[`../../tests/distributed_tensor/CMakeLists.txt`](../../tests/distributed_tensor/CMakeLists.txt)
and [`../../benchmarks/distributed_tensor/CMakeLists.txt`](../../benchmarks/distributed_tensor/CMakeLists.txt):

- `module_distributed_tensor_test_tensor_allocation_focused`
- `module_distributed_tensor_test_tensor_operations_focused`
- `module_distributed_tensor_test_tensor_performance_focused`
- `bench_distributed_tensor_dedicated_gates` (when
  `THEMIS_BUILD_BENCHMARKS` is enabled; source uses in-process stubs)

`tests/epic3_distributed_tensor/` additionally configures phase/contract tests
and an integrity-verification benchmark test; see its `CMakeLists.txt` for
guards and exact targets. The integration CMake conditionally adds
`test_distributed_tensor_soak.cpp`. `benchmarks/epic3_distributed_tensor/`
configures four standard benchmark sources and the partial-refit and
summary-first targets when benchmarks are enabled.

The corresponding CTest registration names and evidence requirements are in
[`TEST_EVIDENCE_COLLECTION.md`](TEST_EVIDENCE_COLLECTION.md). Registration is
not evidence of build or test success. The prior EPIC 3 source/target paths are
present in current CMake; their dated pass-status claims have not been
revalidated in this source-only review.

## Known Issues & Limitations

- Benchmark evidence and final acceptance documentation are still pending before default pipeline integration can be enabled.
- Phase A/B/C enablement and the Phase D GPU plan are described in dated
  planning material; their current gate status and cross-module percentage
  dependencies were not verified in this source review.
- Advisory artifact handling is specified in the relevant public headers; its
  enforcement across all consumers was not validated in this review.

## Breaking Changes

- No roadmap-level breaking change planned. Phase transitions are additive and gated.
- Any change to the ArtifactManifest schema must be backward-compatible or versioned.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `distributed_tensor`
- A 2026-09-16 entry reported a high-cardinality test and runbook; their
  current source/artifact availability was not verified here.
- `tests/integration/test_distributed_tensor_soak.cpp` is present and
  `tests/integration/CMakeLists.txt` conditionally registers it with a
  120-second timeout. Its runtime result was not checked.

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [?] Operator-critical runbook coverage was reported in a 2026-09-16 snapshot; current artifact availability and validation are unresolved.
