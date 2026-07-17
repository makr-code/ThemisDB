# Distributed Tensor Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-13 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Rollout Plan: ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §4 (Phases A–D) -->

## Current Status

EPIC 3 distributed tensor contract ownership and core implementation are complete
for sub-issues 3.1–3.7. Phase 3 runtime failure/degraded-mode semantics are
implemented and verified. Phase 4 broadened contract and fault-path coverage has
been delivered: `test_phase4_contract_coverage.cpp` adds 58 tests spanning all 7
sub-issues (lifecycle, subclasses, manifest, placement, integrity, recovery,
planner, infrastructure). Benchmark evidence and final acceptance documentation
remain pending.

**Tensor-Update Rollout Track**: dynamic tensor-update infrastructure is staged across four
phases (A–D) in `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md` (issue #5468). Phase A
(manifest schema + advisory-only policy) is ready to begin. Phase B (delta log + partial refit)
targets Q3 2026. Phase C (shard summary coordination) targets Q4 2026. Phase D (optional
acceleration) targets 2027+.

**Module readiness**: implementation and Phase 3/4 validation are complete for the EPIC 3
core artifact path; benchmark evidence, acceptance documentation, and the follow-on tensor-update
track remain gated.

## In Progress

- [~] EPIC 3 documentation-governance alignment across roadmap/future/audit files (Target: Q3 2026)
- [~] phase-gate acceptance criteria definition for distributed test/benchmark readiness (Target: Q3 2026)
- [~] Phase 5: Adapter-Distribution & Sharding-Kopplung — LoRA artifact distribution interfaces,
  RAID/Merkle proof engine, distribution receipts, shard snapshots, and recovery ordering (Issue #5418, Target: Q3 2026)
- [~] tensor artifact manifest schema: ArtifactManifest + ManifestStore wiring (Target: Q3 2026)
- [x] phase-4 distributed contract and fault-path coverage expansion beyond the focused regression suite (Target: Q4 2026)

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
- [x] implement phase-3 runtime failure and degraded-mode semantics (Target: Q4 2026)
- [x] deliver phase-4 distributed contract and fault-path tests (Target: Q4 2026)
- [ ] establish phase-5 distributed benchmark and hardening baselines (Target: Q4 2026)

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

### Phase 2: Core Implementation
- [x] production translation units and public headers implemented
- [x] `themis_distributed_tensor` wired into the active community build graph
- [x] integrity translation units created for EPIC 3.4
- [x] deterministic JSON hashing and manifest-facing proof/receipt serialization implemented
- [x] cached-receipt versus fresh-verification rules documented for query-path consumers
- [x] 3.4 runtime distributed failure semantics completed across recovery/planner integration
- [ ] Phase A: tensor delta log + manifest store wired (Target: Q3 2026)
- [ ] Phase A: snapshot-based rebuild worker implemented (Target: Q3 2026)
- [ ] Phase B: patch path + partial refit + rebuild fallback implemented (Target: Q3 2026)
- [ ] Phase C: shard summary refresh + summary-first routing + exact-on-demand fetch implemented (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [x] runtime distributed failure semantics implemented and verified
- [x] corruption and tampering detection implemented
- [x] partial-receipt and stale-receipt edge case handling implemented
- [x] recovery coordination hooks implemented (EPIC 3.5 integration ready)
- [ ] Phase B: rebuild fallback triggered on delta log overflow and instability (Target: Q3 2026)
- [ ] Phase C: exact-on-demand fallback when summary freshness threshold exceeded (Target: Q4 2026)

### Phase 4: Tests
- [x] focused Phase 3 regression suite implemented in `tests/epic3_distributed_tensor/`
- [x] broaden contract and fault-path coverage across all EPIC 3 components — `test_phase4_contract_coverage.cpp` (58 tests, sub-issues 3.1–3.7)
- [x] unit tests for SHA-256 computation and validation (40+ test cases)
- [x] Merkle proof verification tests (happy path, invalid, tampering)
- [x] receipt chain verification tests (genesis, appends, tampering)
- [x] integration tests for full verification workflow
- [x] integrity verification unit and benchmark sources added for follow-on coverage expansion
- [ ] `test_tensor_delta_log` — Phase B ctest gate (Target: Q3 2026)
- [ ] `test_tensor_rebuild_fallback` — Phase B ctest gate (Target: Q3 2026)
- [ ] `test_tensor_shard_summary` — Phase C ctest gate (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [~] Adapter-Distribution & Sharding-Kopplung: LoRA artifact distribution APIs,
  RAID/Merkle proof engine, distribution receipts, shard snapshots, recovery ordering (Issue #5418)
- [ ] benchmark suite implemented in `benchmarks/epic3_distributed_tensor/`
- [x] benchmark suite implemented in `tests/epic3_distributed_tensor/integrity_verification_bench.cc`
- [ ] performance optimization with hardware acceleration (SHA-NI, AVX-512)
- [ ] alignment with graph provenance performance targets
- [ ] `bench_tensor_partial_refit` — Phase B benchmark gate (Target: Q3 2026)
- [ ] `bench_tensor_summary_first` — Phase C benchmark gate (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core distributed tensor docs aligned to source-verifiable implementation state
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
- [x] runtime resilience hardening completed for Phase 3 safety gates
- [x] test gates implemented and passing for Phase 3 and Phase 4 regression suites
- [ ] benchmark gates implemented and passing
- [ ] Phase A advisory-only invariant enforced and verified
- [ ] Phase B ctest gates (`test_tensor_delta_log`, `test_tensor_rebuild_fallback`) passing
- [ ] Phase C ctest gates (`test_tensor_shard_summary`) passing
- [ ] Phase C benchmark gates (`bench_tensor_summary_first`) passing

## Known Issues & Limitations

- Benchmark evidence and final acceptance documentation are still pending before default pipeline integration can be enabled.
- Phase A/B/C tensor-update rollout items remain incomplete and must preserve the advisory-only artifact invariant until their gates pass.
- Phase C depends on `sharding` reaching 70% gap reduction — currently 35% ready (🔴).
- Phase D (optional GPU) requires `gpu` reaching 85% error-handling gap reduction — currently 35% ready (🔴).
- Advisory artifact invariant: tensor summaries and shard summaries are never authoritative truth;
  Graph Truth Layer remains CPU-first and exact in all phases.

## Breaking Changes

- No roadmap-level breaking change planned. Phase transitions are additive and gated.
- Any change to the ArtifactManifest schema must be backward-compatible or versioned.
