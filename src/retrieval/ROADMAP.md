# Retrieval Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Rollout Plan: ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md -->

## Current Status

EPIC 1 retrieval contract ownership is documented for sub-issues 1.1 through 1.7.
Header and source files are deferred to a dedicated implementation PR.
Production behavior and benchmark evidence are pending phase progression.

**Hybrid Retrieval Rollout**: staged rollout plan mapped to module readiness in
`ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md` (issue #5468). Four phases: A (exact-first),
B (local tensor + ANN), C (distributed coordination), D (optional GPU).

## In Progress

- [~] planning-to-contract traceability cleanup across EPIC 1 module docs (Target: Q3 2026)
- [~] phase-gate readiness definitions for tests and benchmarks (Target: Q3 2026)
- [~] hybrid retrieval rollout plan — Phase A exact-first entry criteria (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] implement phase-3 runtime error handling and degraded-mode semantics (Target: Q4 2026)
- [ ] deliver phase-4 contract and scenario test coverage (Target: Q4 2026)
- [ ] establish phase-5 benchmark and hardening baselines (Target: Q4 2026)
- [ ] Phase A gate: single-shard exact retrieval verified end-to-end (Target: Q3 2026)
- [ ] Phase B gate: ANN + CPU parity tests passing for Category A kernels (Target: Q3 2026)
- [ ] Phase C gate: multi-shard exact routing verified under shard failure injection (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] complete phase-6 acceptance documentation from measured behavior (Target: Q1 2027)
- [ ] complete phase-7 integration into default build/test pipelines (Target: Q1 2027)
- [ ] Phase D gate: GPU break-even benchmark results reviewed and published (Target: 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] EPIC 1 contract ownership mapped to headers and planning docs
- [x] hybrid retrieval rollout plan: staged delivery mapped to module readiness (issue #5468)

### Phase 2: Core Implementation
- [ ] skeleton translation units to be added with the implementation PR
- [ ] Phase A: single-shard exact retrieval wired (graph + query + index CPU fallback) (Target: Q3 2026)
- [ ] Phase B: ANN + CPU validation layer wired with advisory-only acceleration (Target: Q3 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] runtime failure semantics implemented and verified
- [ ] Phase A: exact graph traversal error injection tests (Target: Q3 2026)
- [ ] Phase B: ANN fallback-to-CPU on error, timeout, and validation failure (Target: Q3 2026)

### Phase 4: Tests
- [ ] contract and integration tests implemented in `tests/epic1_retrieval/`
- [ ] `test_ann_frontdoor_single_shard` — Phase A gate (Target: Q3 2026)
- [ ] `test_ann_cpu_parity` — Phase B gate (Target: Q3 2026)
- [ ] `test_sharding_multishard_exact` — Phase C gate (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] benchmark suite implemented in `benchmarks/epic1_retrieval/`
- [ ] `bench_ann_distance_cpu_vs_flat` — Phase B benchmark gate (Target: Q3 2026)
- [ ] `bench_multishard_exact` — Phase C benchmark gate (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core retrieval module docs aligned to source-verifiable scaffold state
- [x] hybrid retrieval rollout plan documented in `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md`
- [ ] acceptance docs tied to measured runtime evidence

### Phase 7: Production Integration
- [ ] default pipeline integration enabled after gates are met
- [ ] Phase A production enable: graph + query + index (CPU-only) (Target: Q3 2026)
- [ ] Phase B production enable: ANN + advisory acceleration (Target: Q3 2026)
- [ ] Phase C production enable: multi-shard + tensor summary (Target: Q4 2026)

## Production Readiness Checklist

- [x] contract ownership and scope boundaries documented
- [x] security and performance expectations documented for scaffold phase
- [x] hybrid retrieval rollout plan mapped to module readiness (issue #5468)
- [ ] runtime behavior hardening completed
- [ ] test and benchmark gates implemented and passing
- [ ] Phase A ctest and observability gates met before production
- [ ] Phase B ctest and benchmark gates met before enabling advisory acceleration

## Known Issues and Limitations

- Retrieval runtime behavior pending implementation PR; scaffold only at this stage.
- GPU and distributed-shard paths are not enabled until Phase C/D gates are met.
- Graph Truth Layer is CPU-first and exact in all phases; no GPU acceleration for exact paths.

## Breaking Changes

- No roadmap-level breaking change planned. Phase transitions are additive and gated.
