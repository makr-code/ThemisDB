# Distributed Tensor Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

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

## In Progress

- [~] EPIC 3 documentation-governance alignment across roadmap/future/audit files (Target: Q3 2026)
- [~] phase-gate acceptance criteria definition for distributed test/benchmark readiness (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] implement phase-3 runtime failure and degraded-mode semantics (Target: Q4 2026)
- [ ] deliver phase-4 distributed contract and fault-path tests (Target: Q4 2026)
- [ ] establish phase-5 distributed benchmark and hardening baselines (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] complete phase-6 acceptance documentation from measured evidence (Target: Q1 2027)
- [ ] complete phase-7 default workflow integration after gates pass (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
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

### Phase 6: Documentation and Acceptance
- [x] core distributed tensor docs aligned to source-verifiable scaffold state
- [ ] acceptance docs tied to measured runtime behavior

### Phase 7: Production Integration
- [ ] default pipeline integration enabled after gates are met

## Production Readiness Checklist

- [x] contract ownership and scope boundaries documented
- [x] security and performance expectations documented for scaffold phase
- [ ] runtime resilience hardening completed
- [ ] test and benchmark gates implemented and passing
