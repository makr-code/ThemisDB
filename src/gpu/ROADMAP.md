# GPU Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Rollout Plan: ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §4 (Phase D), §7 (risk) -->

## Current Status

Production GPU runtime exists across device discovery, allocation/governance, backend execution, stream/launcher orchestration, fallback management, and accelerated query/training paths.  The GPU memory manager hierarchy has been consolidated under a unified `IVRAMPolicy` interface (issue #5385).

**Hybrid Retrieval Rollout Readiness**: 35% 🔴 (issue #5468).
- Phase A (exact-first): ❌ No GPU used.
- Phase B (ANN + CPU validation): ❌ GPU not recommended — 340 unchecked CUDA calls remain.
- Phase C (bounded GPU refinement): 🟡 Q3 2026 after error handling hardening.
- Phase D (optional acceleration): ✅ Q4 2026 after 85% gap reduction.
- **Critical**: All 340 unchecked CUDA calls must be addressed before any production GPU path.
- GPU is advisory-only in all phases; Graph Truth Layer remains CPU-first unconditionally.
- Rollout risk detail: `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §7`

## In Progress

- [~] hardening topology-aware and peer-transfer edge behavior under mixed runtime capabilities (Target: Q3 2026)
- [~] benchmark stabilization for core allocation, backend, and acceleration hot paths (Target: Q3 2026)
- [~] diagnostics consistency for quota denials, backend degradation, and fallback incidents (Target: Q3 2026)
- [~] GPU query accelerator kernel launchers for common query types (Target: Q4 2026)
- [~] GPU vector index CUDA backend integration and optimization (Target: Q4 2026)
- [~] GPU vector index HIP backend feature-parity (Target: Q4 2026)

## Planned Features

### Hybrid Retrieval Rollout Gates (issue #5468)
- [ ] Phase C pre-requisite: fix 50% of unchecked CUDA calls (340 → 170) — CRITICAL (Target: Q3 2026)
- [ ] Phase C pre-requisite: kernel SLA timeout enforcement (5-second hard limit) (Target: Q3 2026)
- [ ] Phase C pre-requisite: RAII resource lifecycle violations resolved (57 gaps) (Target: Q3 2026)
- [ ] Phase D gate: fix 85% of unchecked CUDA calls (340 → ≤ 51) (Target: Q4 2026)
- [ ] Phase D gate: resource exhaustion injection test suite (Target: Q4 2026)
- [ ] Phase D gate: all GPU failures degrade to CPU cleanly (Target: Q4 2026)
- [ ] Phase D gate: CPU/GPU break-even benchmark results reviewed (Target: 2027)
- [ ] Phase D ctest gate: `test_gpu_error_handling` (Target: Q4 2026)
- [ ] Phase D ctest gate: `test_gpu_resource_exhaustion` (Target: Q4 2026)
- [ ] Phase D ctest gate: `test_gpu_fallback_all_paths` (Target: Q4 2026)
- [ ] Phase D benchmark gate: `bench_gpu_cpu_breakeven_category_a` (Target: 2027)
- [ ] Phase D benchmark gate: `bench_gpu_cpu_breakeven_category_b` (Target: 2027)

### Short-term (3-6 months)
- [ ] tighten deterministic behavior for multi-device dispatch under heterogeneous hardware states (Target: Q4 2026)
- [ ] extend stress coverage for sustained mixed query/training acceleration workloads (Target: Q4 2026)
- [ ] improve operator-facing incident diagnostics for fallback and capability mismatch scenarios (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for backend and acceleration pathways (Target: Q1 2027)
- [ ] broaden benchmark depth for topology, partition, and high-volume concurrency scenarios (Target: Q1 2027)
- [ ] harden long-running reliability under sustained multi-tenant acceleration pressure (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [ ] freeze resource/backend/acceleration/operations contracts for active major line (Target: Q3 2026)
- [ ] define explicit error taxonomy for quota, degradation, and fallback classes (Target: Q3 2026)

### Phase 2: Core Implementation
- [ ] complete hardening for allocation, backend selection, and dispatch internals (Target: Q4 2026)
- [ ] align advanced topology/partition/transfer behavior with bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-safe behavior for capability mismatch and backend errors (Target: Q4 2026)
- [ ] unify diagnostics across denial, fallback, and degraded execution incidents (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for mixed-backend/mixed-capability edge scenarios (Target: Q4 2026)
- [ ] extend deterministic stress fixtures for multi-tenant and multi-device workloads (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for GPU hot paths (Target: Q4 2026)
- [ ] validate p95/p99 and throughput behavior against release baselines (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core GPU module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] unified GPU memory manager hierarchy (IVRAMPolicy) — architecture docs updated (issue #5385)

## Production Readiness Checklist

- [x] core GPU surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [ ] remaining hardening tasks closed for topology/partition/fallback edge paths
- [ ] release benchmark stabilization complete

## Known Issues and Limitations

- runtime behavior depends on available hardware capability and configured feature gates.
- advanced topology and partition surfaces need continued hardening in mixed environments.
- benchmark breadth should continue expanding for complex multi-device scenarios.

## Breaking Changes

No breaking GPU contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.