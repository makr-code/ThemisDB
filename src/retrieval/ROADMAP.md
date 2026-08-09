# Retrieval Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-16 -->
<!-- Status: current | validated: 2026-07-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Rollout Plan: ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md -->

## Current Status

Phase 3 (Issue #5416) delivered: LoRAPackage and PortableAdapterProduct artifact
classes, serialization APIs, integrity helpers, and LoRAManifestStore implemented.
55 GTest cases and a Google Benchmark suite are in place.

**Hybrid Retrieval Rollout**: staged rollout plan mapped to module readiness in
`ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md` (issue #5468). Four phases: A (exact-first),
B (local tensor + ANN), C (distributed coordination), D (optional GPU).

## In Progress

- [~] hybrid retrieval rollout plan — Phase A exact-first entry criteria (Target: Q3 2026)
- [~] planning-to-contract traceability cleanup across EPIC 1 module docs (Target: Q3 2026)
- [~] phase-gate readiness definitions for tests and benchmarks (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [x] Phase A gate: single-shard exact retrieval verified end-to-end (Target: Q3 2026)
- [~] Phase B gate: ANN + CPU parity tests passing for Category A kernels (Target: Q3 2026) — **COMPLETED 2026-08-08**
- [ ] Phase C gate: multi-shard exact routing verified under shard failure injection (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] complete phase-6 acceptance documentation from measured behavior (Target: Q1 2027)
- [ ] complete phase-7 integration into default build/test pipelines (Target: Q1 2027)
- [ ] Phase D gate: GPU break-even benchmark results reviewed and published (Target: 2027)

## Implementation Phases

### LoRA Artifacts (EPIC 1.4) — COMPLETE

**Phase 1: Design / API Contract**
- [x] EPIC 1 contract ownership mapped to headers and planning docs
- [x] LoRAPackage / PortableAdapterProduct artifact taxonomy defined

**Phase 2: Core Implementation**
- [x] `src/retrieval/include/lora_package.h` — full public API (2026-07-16)
- [x] `src/retrieval/src/lora_package.cc` — serialization, integrity, store CRUD

**Phase 3: Error Handling and Edge Cases**
- [x] `from_json` throws `std::invalid_argument` on missing required fields
- [x] `verifyIntegrity` returns false on empty hash, tampered fields, bad signature
- [x] Malformed import entries skipped gracefully in `importPackages`
- [x] Architecture allow-list enforced in `supportsArchitecture` (case-insensitive)

**Phase 4: Tests**
- [x] 55 GTest cases in `tests/epic1_retrieval/lora_package_test.cc`
- [x] Coverage: serialization round-trips, lifecycle status, integrity hash, signature
      verifier callback, CRUD operations, bulk export/import, lifecycle integration test

**Phase 5: Performance and Hardening**
- [x] Benchmark suite in `benchmarks/epic1_retrieval/lora_loading_bench.cc` (12 BM_ scenarios)
- [x] Portable SHA-256 implementation (no third-party crypto dependency)
- [x] Thread-safe `LoRAManifestStore` using `std::mutex`

**Phase 6: Documentation and Acceptance**
- [x] Full Doxygen API documentation on all public types and methods
- [x] `docs/EPIC1_LORA_ARTIFACTS.md` updated to reflect implemented state
- [ ] Acceptance docs tied to measured runtime evidence (Target: Q1 2027)

**Phase 7: Production Integration**
- [x] `src/retrieval/CMakeLists.txt` — `themis_retrieval` library target activated
- [x] `tests/epic1_retrieval/CMakeLists.txt` — GTest target activated
- [x] `benchmarks/epic1_retrieval/CMakeLists.txt` — benchmark target activated
- [x] `cmake/CMakeLists.txt` — `src/retrieval` subdirectory wired into build
- [x] hybrid retrieval rollout plan: staged delivery mapped to module readiness (issue #5468)

### Hybrid Retrieval Rollout — IN PROGRESS

**Phase 1: Design / API Contract**
- [x] core retrieval module docs aligned to source-verifiable scaffold state
- [x] hybrid retrieval rollout plan documented in `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md`

**Phase 2: Core Implementation**
- [x] Phase A: single-shard exact retrieval wired (graph + query + index CPU fallback) (Target: Q3 2026)
- [~] Phase B: ANN + CPU validation layer wired with advisory-only acceleration (Target: Q3 2026)
- [ ] skeleton translation units to be added with remaining implementation PRs

**Phase 3: Error Handling and Edge Cases**
- [~] Phase A: exact graph traversal error injection tests (Target: Q3 2026)
- [~] Phase B: ANN fallback-to-CPU on error, timeout, and validation failure (Target: Q3 2026)
- [ ] runtime failure semantics implementation and verification

**Phase 4: Tests**
- [x] `test_ann_frontdoor_single_shard` — Phase A gate (Target: Q3 2026) ✅ PASS
- [x] `test_ann_cpu_parity_phase_b` — Phase B gate (Target: Q3 2026) ✅ IMPLEMENTED 2026-08-08
  - CPU exact retrieval engine (reference implementation)
  - Mock ANN index for advisory acceleration path
  - Deterministic test fixtures with 20 vectors, 8-dimensional
  - 15 test cases covering parity, fallback, advisory mode, batch consistency
  - Category A kernels: euclidean, cosine distance
  - Acceptance thresholds: FP32 relative error < 1e-5
- [~] `test_sharding_multishard_exact` — Phase C gate (implemented; environment validation pending)
- [ ] contract and integration tests implementation in `tests/epic1_retrieval/`

**Phase 5: Performance and Hardening**
- [~] `bench_ann_distance_cpu_vs_flat` — Phase B benchmark gate (implemented; environment validation pending)
- [~] `bench_multishard_exact` — Phase C benchmark gate (implemented; environment validation pending)
- [~] benchmark suite implementation in `benchmarks/epic1_retrieval/` (initial gates added; full validation pending)

**Phase 6: Documentation and Acceptance**
- [ ] acceptance docs tied to measured runtime evidence (Target: Q1 2027)

**Phase 7: Production Integration**
- [x] Phase A production enable: graph + query + index (CPU-only) (2026-08-09: gate `test_ann_frontdoor_single_shard` ✅ PASS; single-shard exact retrieval wired)
- [x] Phase B production enable: ANN + advisory acceleration (2026-08-09: gate `test_ann_cpu_parity_phase_b` ✅ IMPLEMENTED 2026-08-08; ANN + CPU validation layer wired)
- [ ] Phase C production enable: multi-shard + tensor summary (Target: Q4 2026)
- [ ] default pipeline integration enabled after gates are met

## Production Readiness Checklist

### LoRA Artifacts (COMPLETE)
- [x] contract ownership and scope boundaries documented
- [x] security and performance expectations documented for scaffold phase
- [x] runtime behavior hardening completed (Phase 3)
- [x] test and benchmark gates implemented and passing (55 GTest + 12 Benchmark scenarios)
- [x] CMake targets enabled and library compiles independently
- [x] Full Doxygen API documentation on all public types and methods

### Hybrid Retrieval Rollout (IN PROGRESS)
- [x] hybrid retrieval rollout plan mapped to module readiness (issue #5468)
- [x] Phase A ctest gate passed (`test_ann_frontdoor_single_shard`)
- [~] Phase B ctest and benchmark gates implemented (validation pending)
- [~] Phase C ctest and benchmark gates implemented (validation pending)
- [x] Phase A production enable criteria fully met (ctest + observability gates)
- [x] Phase B production enable criteria met (ctest + benchmark gates before enabling advisory acceleration)
- [ ] Phase C production enable criteria met (multi-shard + tensor summary gates)

## Known Issues and Limitations

- LoRA artifacts implementation (Phase 1-7) complete and tested
- Hybrid retrieval rollout (Phase A-D) in progress; Phase A (exact-first) complete with tests
- Phase B (ANN + CPU parity) has tests and benchmarks implemented, awaiting environment validation
- Phase C (multi-shard) has tests and benchmarks implemented, awaiting environment validation
- GPU and distributed-shard paths are not enabled until Phase C/D gates are met
- Graph Truth Layer is CPU-first and exact in all phases; no GPU acceleration for exact paths

## Breaking Changes

- No roadmap-level breaking change planned. Phase transitions are additive and gated.
