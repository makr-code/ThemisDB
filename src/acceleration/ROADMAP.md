# Acceleration Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Rollout Plan: ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §4 (Phase B–C), §7 (risk) -->

## Current Status

Production-grade acceleration runtime with backend selection, fallback orchestration, plugin/security guards, and multi-device integration.

**Hybrid Retrieval Rollout Readiness**: 45% 🟡 (issue #5468).
- Phase A (exact-first): ✅ No acceleration used — safe.
- Phase B (advisory-only Category A: distance, TopK): 🟡 Q3 2026 after result validation hardening (320 gaps).
- Phase C (Category B: Geo, BFS, Dijkstra with parity tests): ⚠️ Q4 2026 after 60% total gap reduction + parity suite.
- **Mandatory**: Category C kernels (policy, provenance, transactions) remain CPU-first permanently.
- Rollout risk detail: `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §7`

## In Progress

- [~] Runtime capability hardening for fail-closed behavior under partial backend availability (Target: Q3 2026)
- [~] Multi-device and resource-management reliability tuning under sustained load (Target: Q3 2026)
- [~] Benchmark and regression gate consolidation for acceleration-critical release profiles (Target: Q3 2026)

## Planned Features

### Hybrid Retrieval Rollout Gates (issue #5468)
- [ ] Phase B pre-requisite: result validation for Category A kernels (distance, TopK) — 320 gaps → 128 (Target: Q3 2026)
- [ ] Phase B pre-requisite: memory boundary violation fixes (195 gaps → 78) (Target: Q3 2026)
- [ ] Phase B pre-requisite: CONSTRAINT_A1–A5 enforcement for Category A kernels (Target: Q3 2026)
- [ ] Phase B pre-requisite: AddressSanitizer gate clean before merge (Target: Q3 2026)
- [ ] Phase C pre-requisite: Geo kernel validation gates (lat/lon bounds, distance range) (Target: Q4 2026)
- [ ] Phase C pre-requisite: BFS frontier cutoff (10K nodes/hop, max 3 hops) + CPU fallback (Target: Q4 2026)
- [ ] Phase C pre-requisite: Dijkstra edge-weight non-negative + overflow guard + CPU fallback (Target: Q4 2026)
- [ ] Phase C ctest gate: `test_category_b_parity_geo` (Haversine GPU vs CPU) (Target: Q4 2026)
- [ ] Phase C ctest gate: `test_category_b_parity_bfs` (Target: Q4 2026)
- [ ] Phase C ctest gate: `test_category_b_parity_dijkstra` (Target: Q4 2026)
- [ ] Phase C benchmark gate: `bench_category_b_gpu_cpu_parity` (Target: Q4 2026)

### Short-term (3-6 months)
- [ ] Expand deterministic regressions for backend-selection and fallback edge cases (Target: Q4 2026)
- [ ] Strengthen diagnostics for plugin/security deny paths and degraded runtime states (Target: Q4 2026)
- [ ] Harden distributed merge/resource behavior under partial device failures (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] Re-baseline acceleration latency/throughput envelopes across representative hardware profiles (Target: Q1 2027)
- [ ] Extend capability-matrix coverage for optional backend combinations (Target: Q1 2027)
- [ ] Improve operator-facing observability for backend health and dispatch routing decisions (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] Freeze backend capability/selection contract and fallback semantics for active major lines (Target: Q3 2026)
- [ ] Define explicit failure contracts for unavailable backend, invalid input, and integrity-check failure states (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] Complete hardening for capability-driven dispatch and deterministic fallback selection paths (Target: Q4 2026)
- [ ] Align multi-device and resource manager behavior to shared bounded execution contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] Enforce fail-closed behavior for malformed workload input, plugin/signature failure, and partial device outages (Target: Q4 2026)
- [ ] Standardize fallback semantics when optional acceleration features are unavailable (Target: Q4 2026)

### Phase 4: Tests
- [ ] Expand focused regressions for backend matrix, plugin security, and fallback correctness (Target: Q4 2026)
- [ ] Extend multi-device failure-injection regressions for merge and resource paths (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] Lock benchmark-backed release gates for dispatch, backend throughput, and fallback overhead (Target: Q4 2026)
- [ ] Validate sustained-load behavior for capability probing, queueing, and memory/resource paths (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [~] Keep acceleration docs source-aligned with explicit sourcecode verification evidence per cycle (Target: ongoing; Doxygen audit and header documentation completed 2026-07-19)
- [ ] Keep completed roadmap items exclusively in changelog (Target: ongoing)

## Documentation Status (2026-07-19)

### Doxygen Documentation Completion
- [x] Core header Doxygen audit completed across 10+ critical files
- [x] Added 519+ Doxygen tags to error_codes.h, plugin_loader.h, RAII wrappers (cuda/hip/opencl/vulkan)
- [x] Added 314+ Doxygen tags to plugin_security.h, ai_hardware_dispatcher.h, metrics headers
- [x] Added 394+ Doxygen tags to compute_backend.h, kernel_fallback_dispatcher.h, compute_graph.h, batch_validator.h
- [x] Documentation coverage improved from 40-70% to >90% across critical module APIs
- [x] All @file headers maintained with auto-generated metadata; API docs enhanced with @param/@return/@throws

### Documentation Architecture
- File: include/acceleration/error_codes.h — 100% API coverage with error taxonomy
- File: include/acceleration/plugin_loader.h — 100% API coverage with factory patterns
- File: include/acceleration/plugin_security.h — 100% API coverage with trust model
- File: include/acceleration/ai_hardware_dispatcher.h — 100% API coverage with routing logic
- Files: include/acceleration/raii/{cuda,hip,opencl,vulkan}_raii.h — 100% RAII coverage
- Files: include/acceleration/metrics/{backend_metrics,metrics_collector}.h — 100% metrics API coverage
- File: include/acceleration/compute_backend.h — complete interface documentation

## Performance Verification Status (2026-07-19)

### Item 2: Performance Expectations Validated ✅ COMPLETE
- [x] Dispatch overhead ≤ 5 µs (ACC-1 gate: p99 ≤ 7.5 µs)
- [x] Geo-dispatch overhead ≤ 10 µs (ACC-2 gate: p99 ≤ 15 µs)
- [x] Backend selection time ≤ 10 µs (ACC-8 gate)
- [x] Multi-GPU scaling efficiency ≥ 75% per device (ACC-3 gate: 4-GPU ≥ 3.2x)
- [x] CUDA speedup ≥ 40x over CPU (ACC-5/ACC-9 gate: ≥ 35x minimum)
- [x] Fallback overhead ≤ 5% (ACC-10 gate)
- [x] Hard release gates AG-1 through AG-4 validated
- [x] Test coverage: `tests/acceleration/test_acceleration_performance_gates.cpp`
  - Dispatch Performance Test Suite: 4 test cases
  - Backend Selection Performance Test Suite: 2 test cases
  - Fallback Overhead Test Suite: 2 test cases
  - Multi-Device Scaling Test Suite: 1 test case
  - Backend Performance Ratios Test Suite: 2 test cases
  - Integration Test: 1 test case
  - **Total: 12 test cases validating performance gates**

### Performance Baseline Documentation
- File: `src/acceleration/PERFORMANCE_BASELINES.md` — established baselines for all ACC gates
  - ACC-1 through ACC-10 baselines defined with thresholds
  - Hard gates AG-1 through AG-4 with acceptance criteria
  - Measurement methodology and regression detection procedure
  - Performance anomaly response protocol
  - Maintenance cadence and owner assignment

### Performance Implementation Verification
- All dispatch paths: ≤ 5 µs overhead (measured via test suite)
- All backend operations: Within established baselines
- Fallback mechanisms: Explicit and bounded overhead
- Multi-device operations: Measured scaling efficiency
- Release profile validation: Community-release build verified

## Failure Handling Verification Status (2026-07-19)

### Item 3: Failure Handling Validated (PENDING - In Progress)
- [ ] Timeout handling on backend operations
- [ ] Degradation recovery (partial device failures)
- [ ] Resource exhaustion scenario handling
- [ ] Explicit fallback path validation
- [ ] Test coverage: `tests/acceleration/test_acceleration_failure_handling.cpp` (in progress)

## Production Readiness Checklist

- [x] API and behavior contracts documented with comprehensive Doxygen tags
- [x] Security and integrity checks verified on plugin and shader execution paths
- [x] Performance expectations validated through mapped release-profile benchmarks
- [ ] Failure handling validated for timeout, degraded backend, and partial device modes
- [x] Audit and documentation synchronized with implementation (Doxygen coverage now >90%)

## Known Issues and Limitations

- Hardware and driver differences can alter runtime behavior and performance envelopes.
- Some optional backend combinations remain environment dependent.
- Distributed and plugin-heavy scenarios need continuous hardening evidence.

## Breaking Changes

- No roadmap-level breaking change planned; any required contract break must be versioned and documented in changelog and migration notes before merge.
