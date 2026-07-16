# Retrieval Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-16 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Phase 3 (Issue #5416) delivered: LoRAPackage and PortableAdapterProduct artifact
classes, serialization APIs, integrity helpers, and LoRAManifestStore implemented.
55 GTest cases and a Google Benchmark suite are in place.

## In Progress

- [x] planning-to-contract traceability cleanup across EPIC 1 module docs (Target: Q3 2026)
- [x] phase-gate readiness definitions for tests and benchmarks (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [x] implement phase-3 runtime error handling and degraded-mode semantics (Target: Q4 2026)
- [x] deliver phase-4 contract and scenario test coverage (Target: Q4 2026)
- [x] establish phase-5 benchmark and hardening baselines (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] complete phase-6 acceptance documentation from measured behavior (Target: Q1 2027)
- [ ] complete phase-7 integration into default build/test pipelines (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] EPIC 1 contract ownership mapped to headers and planning docs
- [x] LoRAPackage / PortableAdapterProduct artifact taxonomy defined

### Phase 2: Core Implementation
- [x] `src/retrieval/include/lora_package.h` — full public API (Phase 3, 2026-07-16)
- [x] `src/retrieval/src/lora_package.cc` — serialization, integrity, store CRUD

### Phase 3: Error Handling and Edge Cases
- [x] `from_json` throws `std::invalid_argument` on missing required fields
- [x] `verifyIntegrity` returns false on empty hash, tampered fields, bad signature
- [x] Malformed import entries skipped gracefully in `importPackages`
- [x] Architecture allow-list enforced in `supportsArchitecture` (case-insensitive)

### Phase 4: Tests
- [x] 55 GTest cases in `tests/epic1_retrieval/lora_package_test.cc`
- [x] Coverage: serialization round-trips, lifecycle status, integrity hash, signature
      verifier callback, CRUD operations, bulk export/import, lifecycle integration test

### Phase 5: Performance and Hardening
- [x] Benchmark suite in `benchmarks/epic1_retrieval/lora_loading_bench.cc` (12 BM_ scenarios)
- [x] Portable SHA-256 implementation (no third-party crypto dependency)
- [x] Thread-safe `LoRAManifestStore` using `std::mutex`

### Phase 6: Documentation and Acceptance
- [x] Full Doxygen API documentation on all public types and methods
- [x] `docs/EPIC1_LORA_ARTIFACTS.md` updated to reflect implemented state
- [ ] Acceptance docs tied to measured runtime evidence (Target: Q1 2027)

### Phase 7: Production Integration
- [x] `src/retrieval/CMakeLists.txt` — `themis_retrieval` library target activated
- [x] `tests/epic1_retrieval/CMakeLists.txt` — GTest target activated
- [x] `benchmarks/epic1_retrieval/CMakeLists.txt` — benchmark target activated
- [x] `cmake/CMakeLists.txt` — `src/retrieval` subdirectory wired into build

## Production Readiness Checklist

- [x] contract ownership and scope boundaries documented
- [x] security and performance expectations documented for scaffold phase
- [x] runtime behavior hardening completed (Phase 3)
- [x] test and benchmark gates implemented and passing
- [x] CMake targets enabled and library compiles independently

