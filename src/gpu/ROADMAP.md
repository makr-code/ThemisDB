# GPU Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-08-17 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · PRODUCTION_REQUIREMENTS.md · PERFORMANCE_EXPECTATIONS.md · SECURITY.md -->
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
- [~] Phase D ctest gate: `test_gpu_resource_exhaustion` (Target: Q4 2026)
- [~] Phase D ctest gate: `test_gpu_fallback_all_paths` (Target: Q4 2026)
  - 2026-08-18: test_gpu_resource_exhaustion.cpp (GPU-EXHAUST-01..12) and test_gpu_fallback_all_paths.cpp (GPU-FALLBACK-01..12) implemented
  - 2026-08-18: both suites promoted to `release_critical` via `tests/gpu/CMakeLists.txt`; green-on-`develop` evidence still pending
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
- [x] freeze resource/backend/acceleration/operations contracts for active major line (2026-08-09: GPU_CONTRACT.md created; allocation bounds, device selection, RAII, kernel SLA, diagnostics frozen)
- [x] define explicit error taxonomy for quota, degradation, and fallback classes (2026-08-09: GPU_CONTRACT.md §6 references frozen GPUDispatchErrorCode in gpu_backend_dispatch_contract.h)

### Phase 2: Core Implementation
- [x] complete hardening for allocation, backend selection, and dispatch internals (Delivered: Q3 2026)
  - Bounded runtime contracts documented: MAX_SELECT_DEVICE_LATENCY_US ≤100µs, MAX_ALLOCATE_LATENCY_US ≤1ms
  - Canonical lock order documented: allocation_mutex → device_state_mutex → dispatch_mutex
  - selectDevice() emits BACKEND_NO_DEVICE_AVAILABLE diagnostic on fail-closed
  - allocate() validates parameters early with fail-closed error codes (ALLOC_SIZE_EXCEEDS_LIMIT, ALLOC_INVALID_PARAMS)
  - SLA timing verification built into load_balancer.cpp and gpu_memory_allocator.cpp
  - GPUBackendDispatchDiagnostics infrastructure added for unified event emission
  - Contract header: `include/gpu/gpu_backend_dispatch_contract.h` (v1.0.0)
  - Diagnostics header: `include/gpu/gpu_backend_dispatch_diagnostics.h` (v1.0.0)
- [x] align advanced topology/partition/transfer behavior with bounded runtime contracts (Delivered: Q3 2026)
  - setTopology() and selectTopologyAware() honor load balancer bounds
  - Device health checks remain ≤100µs per contract
  - Topology unavailability falls back to LEAST_LOADED with diagnostic emission

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-safe behavior for capability mismatch and backend errors (Delivered: Q3 2026)
  - All error codes inherit from GPUDispatchErrorCode enum with fail-closed classification
  - isFailClosedClass() predicate ensures all errors trigger CPU degradation
  - BACKEND_CAPABILITY_MISMATCH maps to distinct event type for operator observability
  - Backend selection failures never silently retry; always emit diagnostic and return nullptr
- [x] unify diagnostics across denial, fallback, and degraded execution incidents (Delivered: Q3 2026)
  - emitDiagnostic() helper unifies log + event-callback emission for all error paths
  - All error codes have human-readable strings via errorCodeToString()
  - Event callback registration supports multiple diagnostic consumers
  - Diagnostic latency bounded to ≤100µs per contract

### Phase 4: Tests
- [x] expand focused regressions for mixed-backend/mixed-capability edge scenarios (Delivered: Q3 2026)
- [x] extend deterministic stress fixtures for multi-tenant and multi-device workloads (Delivered: Q3 2026)
  - Test file: `tests/gpu/test_gpu_phase2_phase3_focused.cpp`
  - Test cases: P23-01..P23-08 (backend selection fail-closed, bounded latency, diagnostic emission, error mapping)
  - kPhase23Seed = 42; all tests self-contained, no external I/O

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for GPU hot paths (Delivered: Q3 2026)
  - Benchmark file: `benchmarks/gpu/bench_gpu_phase2_phase3_gates.cpp`
  - Gates: GP23-01..GP23-06 (backend selection ≤100µs, allocation validation ≤1ms, 
    diagnostic emission ≤100µs, device health check ≤100µs, quota check ≤10µs, error string conversion)
  - kP23CanonicalSeed = 42; Repetitions(5); mock-only (no I/O, no threads)
- [x] validate p95/p99 and throughput behavior against release baselines (Delivered: Q3 2026)

### Phase 6: Documentation and Acceptance
- [x] core GPU module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] unified GPU memory manager hierarchy (IVRAMPolicy) — architecture docs updated (issue #5385)
- [x] Phase 2/3 hardening delivered (2026-08-05)
  - Bounded runtime contracts: gpu_backend_dispatch_contract.h v1.0.0
  - Diagnostics infrastructure: gpu_backend_dispatch_diagnostics.h v1.0.0
  - Load balancer hardening: fail-closed backend selection with latency bounds
  - Allocator hardening: fail-closed parameter validation with error codes
  - Test evidence: P23-01..P23-08 (8 focused tests)
  - Benchmark evidence: GP23-01..GP23-06 (6 performance gates)

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

## Program Execution Model — Wave Context

This module is scoped to **Wave A — Runtime Reliability First** in the program-level wave model.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full Wave A → B → C → D gate model and exit criteria.

### Wave A Scope for `gpu`
- [ ] Gpu: reduce unchecked CUDA-call exposure, close RAII lifecycle gaps, enforce kernel timeouts, and guarantee clean CPU degradation on every GPU failure (Target: Q3–Q4 2026)

### Wave A Exit Criteria (this module's contribution)
- [ ] Deterministic chaos evidence complete for recovery and failover paths (Target: Q4 2026)
- [ ] Fail-closed behavior verified for all distributed/acceleration paths in scope (Target: Q4 2026)
- [ ] `release_critical` CI green on `develop` (Target: Q4 2026)
- [ ] Representative-hardware p95/p99 baselines refreshed (Target: Q4 2026)
  - 2026-08-18: `bench_gpu_a8_baselines.cpp` registered in `benchmarks/CMakeLists.txt`; execution evidence pending representative hardware

### Wave A Closure Evidence Block
- [x] Focused regression closure: Phase 2/3 focused tests (P23-01..08) and release-gate benchmarks (GP23-01..06) are already delivered.
- [~] Chaos/fault-injection evidence: focused timeout/fallback/resource-safety CPU-only regressions are now covered; broader resource-exhaustion and all-path fault-injection suites remain open.
- [~] Fail-closed verification: timeout enforcement now avoids unsafe stream destruction and focused regressions prove clean CPU fallback on timeout/exception paths, but full all-path proof is still pending.
- [ ] Representative-hardware p95/p99 baselines: representative-hardware refresh remains open for backend and acceleration paths; `bench_gpu_a8_baselines` is now wired into the benchmark build for evidence capture.
- [~] `release_critical` coverage: timeout/fallback/resource-exhaustion focused targets are now registered `release_critical`, but full green-on-`develop` evidence remains open.
- [~] Next closure batch: resource-exhaustion and representative-hardware closure remain open after the delivered CUDA-call/RAII/timeout/fallback hardening.

### Dependencies on Later Waves
- Wave B performance consolidation depends on Wave A gate closure.
- Wave C security validation depends on stable Wave A runtime behavior.
- Wave D operability hardening depends on all prior waves being gate-complete.
