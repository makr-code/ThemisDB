# Distributed Tensor Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Rollout Plan: ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §4 (Phases A–D) -->

## Current Status

EPIC 3 distributed tensor contract ownership is documented for sub-issues 3.1–3.7.

**Phase 1 (Design/API Contract) Status:**
- 3.1 (Artifact Classes): ✅ Complete
- 3.2 (Manifest Schema): ✅ Complete  
- 3.3 (Shard Placement): ✅ Complete
- **3.4 (Integrity Model): ✅ COMPLETE** ← Just completed
- 3.5 (Recovery Strategy): 🔄 In scope
- 3.6 (Distributed Retrieval): 🔄 In scope
- 3.7 (Tensor Infrastructure): 🔄 In scope

Header and source files exist for 3.4 (`integrity_verification.h/.cc`) and now include
phase-2 canonical hashing plus manifest-facing receipt/proof serialization helpers.
Phase 3+ work (runtime recovery semantics, tests, benchmarks, and default integration)
remains for dedicated PRs.

**Tensor-Update Rollout Track**: dynamic tensor-update infrastructure is staged across four
phases (A–D) in `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md` (issue #5468). Phase A
(manifest schema + advisory-only policy) is ready to begin. Phase B (delta log + partial refit)
targets Q3 2026. Phase C (shard summary coordination) targets Q4 2026. Phase D (optional
acceleration) targets 2027+.

**Module readiness**: 30% 🔴. Runtime not yet implemented; scaffold only. Phase A entry requires
manifest schema wiring and advisory-only artifact policy documentation.

## In Progress

- [~] EPIC 3 documentation-governance alignment across roadmap/future/audit files (Target: Q3 2026)
- [~] phase-gate acceptance criteria definition for distributed test/benchmark readiness (Target: Q3 2026)
- [~] tensor artifact manifest schema: ArtifactManifest + ManifestStore wiring (Target: Q3 2026)

## Planned Features

### Tensor-Update Infrastructure Track (issue #5468)

#### Phase A — Exact-First Infrastructure (Q3 2026, start)
- [ ] tensor delta log schema (Target: Q3 2026)
- [x] manifest schema committed with advisory-only artifact policy documented (Target: Q3 2026)
- [x] Prometheus freshness metrics wired (`tensor_freshness_age_seconds`, `tensor_delta_log_entries_total`) (Target: Q3 2026)
- [ ] snapshot-based rebuild worker (Target: Q3 2026)
- [ ] exact graph fallback guarantee documented and tested (Target: Q3 2026)

#### Phase B — Local Tensor Maintenance (Q3 2026, weeks 6–9)
- [ ] patch path for small tensor deltas (bounded delta window) (Target: Q3 2026)
- [ ] partial refit for bounded windows (Target: Q3 2026)
- [ ] rebuild fallback for unstable or large update sets (Target: Q3 2026)
- [ ] planner freshness gates (`tensor_freshness_age_seconds` threshold checks) (Target: Q3 2026)
- [ ] ctest gate: `test_tensor_delta_log` (Target: Q3 2026)
- [ ] ctest gate: `test_tensor_rebuild_fallback` (Target: Q3 2026)
- [ ] benchmark gate: `bench_tensor_partial_refit` (Target: Q3 2026)

#### Phase C — Distributed Tensor Artifact Coordination (Q4 2026)
- [ ] shard summary refresh with freshness timestamps (Target: Q4 2026)
- [ ] summary-first routing with escalation to exact-on-demand (Target: Q4 2026)
- [ ] exact-on-demand tensor fetch (replaces summary when accuracy required) (Target: Q4 2026)
- [ ] multi-shard freshness consensus (Target: Q4 2026)
- [ ] ctest gate: `test_tensor_shard_summary` (Target: Q4 2026)
- [ ] benchmark gate: `bench_tensor_summary_first` (Target: Q4 2026)

#### Phase D — Optional Acceleration (2027+)
- [ ] CPU/GPU break-even validation for tensor operations (Target: 2027)
- [ ] bounded GPU refinement after break-even proven (Target: 2027)
- [ ] no unconditional GPU dependency on any tensor path (Target: 2027)

### Short-term (3-6 months)
- [ ] implement phase-3 runtime failure and degraded-mode semantics (Target: Q4 2026)
- [ ] deliver phase-4 distributed contract and fault-path tests (Target: Q4 2026)
- [ ] establish phase-5 distributed benchmark and hardening baselines (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] complete phase-6 acceptance documentation from measured evidence (Target: Q1 2027)
- [ ] complete phase-7 default workflow integration after gates pass (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
<<<<<<< HEAD
- [x] EPIC 3 contract ownership mapped and documented
- [x] tensor-update rollout track staged (Phases A–D) in `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md`
- [ ] manifest schema (ArtifactManifest fields + ManifestStore API) frozen for active line (Target: Q3 2026)
- [x] advisory-only artifact policy documented with invariant: tensor artifacts never replace graph-verified results (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] scaffold translation units to be added with the implementation PR
- [ ] Phase A: tensor delta log + manifest store wired (Target: Q3 2026)
- [ ] Phase A: snapshot-based rebuild worker implemented (Target: Q3 2026)
- [ ] Phase B: patch path + partial refit + rebuild fallback implemented (Target: Q3 2026)
- [ ] Phase C: shard summary refresh + summary-first routing + exact-on-demand fetch implemented (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] runtime distributed failure semantics implemented and verified
- [ ] Phase B: rebuild fallback triggered on delta log overflow and instability (Target: Q3 2026)
- [ ] Phase C: exact-on-demand fallback when summary freshness threshold exceeded (Target: Q4 2026)

### Phase 4: Tests
- [ ] contract and fault-path tests implemented in `tests/epic3_distributed_tensor/`
- [ ] `test_tensor_delta_log` — Phase B ctest gate (Target: Q3 2026)
- [ ] `test_tensor_rebuild_fallback` — Phase B ctest gate (Target: Q3 2026)
- [ ] `test_tensor_shard_summary` — Phase C ctest gate (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] benchmark suite implemented in `benchmarks/epic3_distributed_tensor/`
- [ ] `bench_tensor_partial_refit` — Phase B benchmark gate (Target: Q3 2026)
- [ ] `bench_tensor_summary_first` — Phase C benchmark gate (Target: Q4 2026)
=======
- [x] EPIC 3.1 (Artifact Classes) contract ownership mapped and documented
- [x] EPIC 3.2 (Manifest Schema) contract ownership mapped and documented
- [x] EPIC 3.3 (Shard Placement) contract ownership mapped and documented
- [x] **EPIC 3.4 (Integrity Model) contract ownership mapped and documented** ← Just completed

### Phase 2: Core Implementation
- [x] integrity translation units created for EPIC 3.4
- [x] deterministic JSON hashing and manifest-facing proof/receipt serialization implemented
- [x] cached-receipt versus fresh-verification rules documented for query-path consumers
- [x] 3.4 runtime distributed failure semantics completed across recovery/planner integration

### Phase 3: Error Handling and Edge Cases
- [x] runtime distributed failure semantics implemented and verified
- [x] corruption and tampering detection implemented
- [x] partial-receipt and stale-receipt edge case handling implemented
- [x] recovery coordination hooks implemented (EPIC 3.5 integration ready)

### Phase 4: Tests
- [x] unit tests for SHA-256 computation and validation (40+ test cases)
- [x] Merkle proof verification tests (happy path, invalid, tampering)
- [x] receipt chain verification tests (genesis, appends, tampering)
- [x] integration tests for full verification workflow
- [x] comprehensive benchmarks for performance baseline

### Phase 5: Performance and Hardening
- [x] benchmark suite implemented in `tests/epic3_distributed_tensor/integrity_verification_bench.cc`
- [ ] performance optimization with hardware acceleration (SHA-NI, AVX-512)
- [ ] alignment with graph provenance performance targets
>>>>>>> origin/develop

### Phase 6: Documentation and Acceptance
- [x] core distributed tensor docs aligned to source-verifiable scaffold state
- [x] tensor-update rollout track documented in `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md` (issue #5468)
- [ ] acceptance docs tied to measured runtime behavior

### Phase 7: Production Integration
- [ ] default pipeline integration enabled after gates are met
- [ ] Phase A production enable: manifest schema + advisory policy in place (Target: Q3 2026)
- [ ] Phase B production enable: delta log + partial refit with rebuild fallback (Target: Q3 2026)
- [ ] Phase C production enable: shard summary coordination (after sharding Phase C gates) (Target: Q4 2026)

## Production Readiness Checklist

- [x] contract ownership and scope boundaries documented
- [x] security and performance expectations documented for scaffold phase
- [x] tensor-update infrastructure rollout track defined (issue #5468)
- [ ] runtime resilience hardening completed
- [ ] test and benchmark gates implemented and passing
- [ ] Phase A advisory-only invariant enforced and verified
- [ ] Phase B ctest gates (`test_tensor_delta_log`, `test_tensor_rebuild_fallback`) passing
- [ ] Phase C ctest gates (`test_tensor_shard_summary`) passing
- [ ] Phase C benchmark gates (`bench_tensor_summary_first`) passing

## Known Issues and Limitations

- Runtime not yet implemented; scaffold and contract ownership only at this stage.
- Phase C depends on `sharding` reaching 70% gap reduction — currently 35% ready (🔴).
- Phase D (optional GPU) requires `gpu` reaching 85% error-handling gap reduction — currently 35% ready (🔴).
- Advisory artifact invariant: tensor summaries and shard summaries are never authoritative truth;
  Graph Truth Layer remains CPU-first and exact in all phases.

## Breaking Changes

- No roadmap-level breaking change planned. Phase transitions are additive and gated.
- Any change to the ArtifactManifest schema must be backward-compatible or versioned.
