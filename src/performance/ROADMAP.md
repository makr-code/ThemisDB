# Performance Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-08-05 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production performance runtime with comprehensive measurement, optimization strategy control, NUMA/cache tuning, and hardware-aware acceleration behavior.
Complete Phase 1–6 delivery with contract frozen, error taxonomy defined, focused tests (PFM-01..16), benchmarks (GATE-PFM-01..06), and documentation aligned.

## In Progress

### Q1 2027 priorities
- [ ] re-baseline p95/p99 envelopes for core performance module operations (Target: Q1 2027)
- [ ] broaden benchmark depth for mixed and distributed performance scenarios (Target: Q1 2027)
- [ ] harden long-running reliability under sustained adaptive workload shifts (Target: Q1 2027)

## Planned Features

### Mid-term (6-12 months, Q2 2027+)
- [ ] distributed performance tracing integration (Target: Q2 2027)
- [ ] adaptive memory-pool resizing under NUMA topology awareness (Target: Q2 2027)

## Completed Highlights (Q4 2026)

- [x] Phase 2 feature hardening: optimization algorithm validation and hardware-aware adaptation
  - evidence: phase2_feature_flags hardware detection (x86/x64 CPUID, ARM NEON, storage); wisckey/dostoevsky/cicada/ligra/rabitq fail-closed validation; bounds checking on parameter spaces
- [x] Phase 3 error handling standardization: unified diagnostics across adaptive/measurement/export subsystems
  - evidence: adaptive_batch_tuner config validation (min/max sizes, ema_alpha ∈ [0,1]); per_query_cost_model bounds (cpuCost, pageReadCost); consistent null-safety and exception paths
- [x] hardening edge-case behavior across adaptive optimization and hardware fallback paths
  - evidence: test_cicada.cpp (14 deterministic GTest cases); test_performance_contract_hardening_focused.cpp (PFM-01..16)
- [x] benchmark stabilization for performance module hot paths and scalability cases
  - evidence: `benchmarks/performance/bench_performance_release_gates.cpp` (GATE-PFM-01..06 with deterministic seeding kCanonicalSeed=42)
- [x] diagnostics consistency for profiling/export/optimization incident classes
  - evidence: performance_api_contract.h error taxonomy (PERF_COMPILE_TIMEOUT..PERF_STATS_UNAVAILABLE); contract-hardening tests verify consistency

## Completed Highlights (Q3 2026)

- [x] API contract freeze: PerfError taxonomy, CacheStats, PoolAcquireResult, CostEstimate frozen v1.0
  - evidence: `include/performance/performance_api_contract.h` §Error Taxonomy
- [x] error taxonomy definition: 7 standardized error codes (PERF_COMPILE_TIMEOUT..PERF_STATS_UNAVAILABLE)
  - evidence: `include/performance/performance_api_contract.h`## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze measurement/optimization/memory-hardware contracts for active major line (Target: Q3 2026)
  - evidence: `include/performance/performance_api_contract.h` — PerfError taxonomy, CacheStats, PoolAcquireResult, CostEstimate frozen v1.0
- [x] define explicit error taxonomy for performance subsystem failures (Target: Q3 2026)
  - evidence: `include/performance/performance_api_contract.h` §Error Taxonomy (7 codes PERF_COMPILE_TIMEOUT..PERF_STATS_UNAVAILABLE)

### Phase 2: Core Implementation
- [x] complete hardening for optimization and profiling internals (Target: Q4 2026)
  - evidence: phase2_feature_flags.cpp hardware detection (CPUID, ARM NEON, storage); cicada/dostoevsky/ligra/rabitq/wisckey hardened with validation
- [x] align feature gating and fallback behavior to bounded runtime contracts (Target: Q4 2026)
  - evidence: runtime detection syncs with compile-time flags (THEMIS_ENABLE_*); fail-closed on unsupported hardware; graceful diagnostics

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-safe behavior for unsupported hardware and invalid tuning inputs (Target: Q4 2026)
  - evidence: adaptive_batch_tuner config validation (min/max, ema_alpha ∈ [0,1]); per_query_cost_model bounds checking; throw on invalid
- [x] unify diagnostics across measurement/export/optimization incidents (Target: Q4 2026)
  - evidence: consistent error propagation via exceptions; parameter validation gates; null-safety checks across all phase3 components

### Phase 4: Tests
- [x] expand focused regressions for optimization, cache, and NUMA edge scenarios (Target: Q4 2026)
  - evidence: `tests/performance/test_performance_contract_hardening_focused.cpp` — PFM-01..PFM-16 (16 deterministic GTest cases, kSeed=42)
- [x] extend deterministic stress fixtures for high-throughput performance operations (Target: Q4 2026)
  - evidence: PFM-05..PFM-08 batch/arithmetic coverage in same file

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for performance hot paths (Target: Q4 2026)
  - evidence: `benchmarks/performance/bench_performance_release_gates.cpp` — GATE-PFM-01..06
- [x] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)
  - evidence: GATE-PFM-01 (CacheStats ≤500ns p99), GATE-PFM-03 (error cast ≥50M ops/s)

### Phase 6: Documentation and Acceptance
- [x] core performance module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries

## Production Readiness Checklist

- [x] core performance surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] remaining hardening tasks closed for adaptive/memory/hardware edge paths
  - evidence: contract-hardening tests (PFM-01..16), cicada tests (14 cases), benchmark gates (GATE-PFM-01..06) all passing
- [x] release benchmark stabilization complete
  - evidence: GATE-PFM-01..06 in `benchmarks/performance/bench_performance_release_gates.cpp`

## Known Issues and Limitations

- runtime behavior depends on host hardware capabilities and enabled feature flags.
- selected adaptive and hardware-dependent edge scenarios need continued hardening.
- benchmark breadth should continue expanding for advanced distributed scenarios.

## Breaking Changes

No breaking performance contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `performance`
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
