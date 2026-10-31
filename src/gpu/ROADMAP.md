# GPU Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Source-reviewed: 2026-10-31; roadmap statuses and runtime evidence not revalidated -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · PRODUCTION_REQUIREMENTS.md · PERFORMANCE_EXPECTATIONS.md · SECURITY.md -->
<!-- Rollout Plan: ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §4 (Phase D), §7 (risk) -->

## Current Status

GPU runtime, resource accounting, backend, stream/launcher, fallback, and
accelerated query/training surfaces exist in the source tree. Runtime readiness
and release acceptance depend on the enabled backend, edition, feature flags,
workload, and validation evidence; this roadmap is not proof that each path is
production-ready. The canonical GPU accounting manager implements
`IVRAMPolicy`, but selected LLM/LoRA integrations do not pass tenant IDs through
to its tenant-aware overloads; see [ARCHITECTURE.md](ARCHITECTURE.md).

**Hybrid Retrieval Rollout Readiness**: 35% (issue #5468; historical tracked estimate, not revalidated here).
- Phase A (exact-first), Phase B (ANN + CPU validation), and Phase C/D readiness are recorded in the rollout plan; current phase completion is not established by this document.
- The 340-call baseline and 85% reduction target are historical planning metrics, not current source-tree counts or measured results.
- **Critical**: Re-run the CUDA-call audit before using its historical counts as a current release decision.
- GPU is advisory-only in all phases; Graph Truth Layer remains CPU-first unconditionally.
- Rollout risk detail: `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §7`

## In Progress

- [~] hardening topology-aware and peer-transfer edge behavior under mixed runtime capabilities (Target: Q3 2026)
- [~] benchmark stabilization for core allocation, backend, and acceleration hot paths (Target: Q3 2026)
- [~] diagnostics consistency for quota denials, backend degradation, and fallback incidents (Target: Q3 2026)
- [~] GPU query accelerator kernel launchers for common query types (Target: Q4 2026)
  - 2026-08-24: Sprint 1 — Filter/Join/Aggregation GPU paths confirmed production-wired (Thrust); hashJoin GPU-path brace bug fixed (stray `}` closed `if (use_gpu)` prematurely, making `gpu_done` out of scope in CUDA/HIP builds); topK operation added (GPU: `thrust::partial_sort_copy` O(n log k), CPU: `std::partial_sort` fallback); `QueryShape::OpType::TOPK` added; 10 topK focused tests (QA-TOPK-01..10) added to `tests/gpu/test_gpu_query_accelerator.cpp`.
- [~] GPU vector index CUDA backend integration and optimization (Target: Q4 2026)
- [~] GPU vector index HIP backend feature-parity (Target: Q4 2026)

## Planned Features

> **Wave A GPU CUDA Reduction Target:** ≥40% reduction vs Wave 7 baseline (Phase C pre-requisite)  
> **Tracking Document:** `GPU_CUDA_REDUCTION_TRACKING.md` (comprehensive progress tracking)  
> **Infrastructure Requirements:** `docs/governance/GPU_SELF_HOSTED_RUNNER_REQUIREMENTS.md`  
> **Baseline Evidence:** `benchmarks/wave8/GPU_BASELINES_2026_Q4.json` (template ready for execution)

### Hybrid Retrieval Rollout Gates (issue #5468)
- [~] Phase C pre-requisite: fix 50% of unchecked CUDA calls (340 → 170) — CRITICAL (Target: Q3 2026)
  - 2026-08-24: CUDA-call audit complete for `src/gpu/` + `include/gpu/`; `include/gpu/cuda_raii.h` added with `CudaStreamGuard`, `CudaEventGuard`, `CudaDeviceMemoryGuard`; all raw call sites catalogued in `WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md`; wrapper coverage confirmed for stream/event/memory; remaining destructor-only `cudaFree` sites documented as acceptable
- [~] Phase C pre-requisite: kernel SLA timeout enforcement (5-second hard limit) (Target: Q3 2026)
  - 2026-08-24: Confirmed present — `KernelSLAGuard` at `include/themis/gpu/gpu_timeout.h`; 11 active deployment sites in `query_accelerator.cpp` and `rocm_backend.cpp`; GPU-TIMEOUT-01..12 registered `release_critical`
- [~] Phase C pre-requisite: RAII resource lifecycle violations resolved (57 gaps) (Target: Q3 2026)
  - 2026-08-24: Wrapper infrastructure now spans `gpu_safe_raii.h` + `gpu_raii_wrappers.hpp` + `gpu_resource_handles.h` + `cuda_raii.h`; gap-by-gap migration in progress
- [~] Phase D gate: fix 85% of unchecked CUDA calls (340 → ≤ 51) (Target: Q4 2026)
- [ ] Phase D gate: resource exhaustion injection test suite (Target: Q4 2026)
- [ ] Phase D gate: all GPU failures degrade to CPU cleanly (Target: Q4 2026)
- [~] Phase D gate: CPU/GPU break-even benchmark results reviewed (Target: 2027)
- [ ] Phase D ctest gate: `test_gpu_error_handling` (currently quarantined in `tests/gpu/CMakeLists.txt`; target is not registered)
- [~] Phase D ctest gate: `test_gpu_resource_exhaustion` (source exists; currently quarantined and not registered by CMake)
- [~] Phase D ctest gate: `test_gpu_fallback_all_paths` (Target: Q4 2026)
  - 2026-08-18: test_gpu_resource_exhaustion.cpp (GPU-EXHAUST-01..12) and test_gpu_fallback_all_paths.cpp (GPU-FALLBACK-01..12) implemented
  - Current CMake check: `test_gpu_resource_exhaustion` is quarantined and skipped; `test_gpu_fallback_all_paths` is registered with `release_critical`. Neither registration alone is execution evidence.
- [~] Phase D benchmark gate: `bench_gpu_cpu_breakeven_category_a` (Target: 2027)
- [~] Phase D benchmark gate: `bench_gpu_cpu_breakeven_category_b` (Target: 2027)

### Short-term (3-6 months)
- [ ] tighten deterministic behavior for multi-device dispatch under heterogeneous hardware states (Target: Q4 2026)
- [x] extend stress coverage for sustained mixed query/training acceleration workloads (Wave D report: `tests/gpu/test_gpu_highcardinality_stress.cpp`, delivered 2026-09-16; source is in the current GPU test glob)
- [x] improve operator-facing incident diagnostics for fallback and capability mismatch scenarios (Wave D report: `docs/operability/RUNBOOK_GPU_MANAGER.md`, delivered 2026-09-16)

### Mid-term (6-12 months)
- [ ] re-baseline p95/p99 envelopes for backend and acceleration pathways (Target: Q1 2027)
- [x] broaden benchmark depth for topology, partition, and high-volume concurrency scenarios (Wave D: bench_gpu_dedicated_gates.cpp — GPU-BM-01..04 delivered 2026-09-16)
- [x] harden long-running reliability under sustained multi-tenant acceleration pressure (Wave D report: `tests/integration/test_gpu_manager_soak.cpp`, delivered 2026-09-16; this is not a `tests/gpu/` focused-suite source)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] document resource/backend/dispatch contract reference (2026-08-09: `include/gpu/gpu_backend_dispatch_contract.h` declares latency targets and an error enum; it does not freeze every numeric slot or prove universal fallback behavior)
- [x] define explicit error taxonomy for quota, degradation, and fallback classes (2026-08-09: see the current enum declaration and [`include/gpu/GPU_CONTRACT.md`](../../include/gpu/GPU_CONTRACT.md))

### Phase 2: Core Implementation
- [x] complete hardening for allocation, backend selection, and dispatch internals (Delivered: Q3 2026)
  - Bounded runtime contracts documented: MAX_SELECT_DEVICE_LATENCY_US ≤100µs, MAX_ALLOCATE_LATENCY_US ≤1ms
  - Canonical lock order documented: allocation_mutex → device_state_mutex → dispatch_mutex
  - selectDevice() emits BACKEND_NO_DEVICE_AVAILABLE diagnostic on fail-closed
  - allocate() validates parameters early with fail-closed error codes (ALLOC_SIZE_EXCEEDS_LIMIT, ALLOC_INVALID_PARAMS)
  - Timing checks are present in selected code; they do not prove every operation remains within a hard real-time bound.
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
  - isFailClosedClass() classifies non-success enum values; a classification does not itself execute CPU work.
  - BACKEND_CAPABILITY_MISMATCH maps to distinct event type for operator observability
  - Backend selection failures never silently retry; always emit diagnostic and return nullptr
- [x] unify diagnostics across denial, fallback, and degraded execution incidents (Delivered: Q3 2026)
  - emitDiagnostic() helper unifies log + event-callback emission for all error paths
  - All error codes have human-readable strings via errorCodeToString()
  - Event callback registration is available; emission depends on call sites.
  - Diagnostic latency bounded to ≤100µs per contract

### Phase 4: Tests
- [x] expand focused regressions for mixed-backend/mixed-capability edge scenarios (Delivered: Q3 2026)
- [x] extend deterministic stress fixtures for multi-tenant and multi-device workloads (Delivered: Q3 2026)
  - Test source: `tests/gpu/test_gpu_phase2_phase3_focused.cpp`; current CMake quarantines it, so it has no registered focused target.
  - Historical cases: P23-01..P23-08 (backend selection, latency, diagnostics, error mapping); source presence is not current execution evidence.

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
- [~] Gpu: reduce unchecked CUDA-call exposure, close RAII lifecycle gaps, enforce kernel timeouts, and guarantee clean CPU degradation on every GPU failure (Target: Q3–Q4 2026)
  - 2026-08-24 report: CUDA-call audit and wrapper work recorded; current CMake registers the timeout and fallback suites with `release_critical`, but quarantines the resource-exhaustion suite.

### Wave A Exit Criteria (this module's contribution)
- [ ] Deterministic chaos evidence complete for recovery and failover paths (Target: Q4 2026)
- [ ] Fail-closed behavior verified for all distributed/acceleration paths in scope (Target: Q4 2026)
- [ ] `release_critical` CI green on `develop` (Target: Q4 2026)
  - 2026-08-18: `ci-pr-gates` develop push runs were green, but the `Release-Critical Test Suite` job is skipped on push; no completed `ci-build` develop run has yet produced GPU-specific green evidence.
- [ ] Representative-hardware p95/p99 baselines refreshed (Target: Q4 2026)
  - 2026-08-18: `bench_gpu_a8_baselines.cpp` registered in `benchmarks/CMakeLists.txt`; execution evidence pending representative hardware
  - 2026-08-18: clean release benchmark build was re-attempted in sandbox after installing system packages, but the benchmark target still had to traverse the full shared build graph and did not reach the GPU benchmark binary within the sandbox execution window.

### Wave A Closure Evidence Block
- [x] Focused regression closure: Phase 2/3 focused tests (P23-01..08) and release-gate benchmarks (GP23-01..06) are already delivered.
- [x] Kernel SLA test source exists: `KernelSLAGuard` is declared in `include/themis/gpu/gpu_timeout.h`; `test_gpu_wave_a_timeout_closure.cpp` is registered with `release_critical`. The historical execution status is in `WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md`.
- [~] Resource-exhaustion and all-path fallback test sources exist: `test_gpu_resource_exhaustion.cpp` is currently quarantined in CMake; `test_gpu_fallback_all_paths.cpp` is registered with `release_critical`. Current green execution evidence is not established by registration.
- [~] Fault-injection/test evidence: historical test sources exist, but current CMake quarantines the resource-exhaustion suite. Revalidate each test and execution result before claiming coverage.
- [~] Fail-closed/fallback verification: no universal CPU-computation fallback is established. `GPUModule::submitWork` currently records fallback and returns success without executing its work item on CPU; verify workload-specific fallback separately.
  - 2026-08-24: audit pass complete; `include/gpu/cuda_raii.h` delivers `CudaStreamGuard`/`CudaEventGuard`/`CudaDeviceMemoryGuard`; all `src/gpu/` raw call sites catalogued (see `WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md §CUDA Call Reduction Audit`).
- [~] RAII lifecycle gap closure: 57 gaps identified; `gpu_safe_raii.h` + `gpu_raii_wrappers.hpp` + `cuda_raii.h` (2026-08-24) deliver wrapper infrastructure; `CudaStreamGuard::adopt()` factory added + `stream_manager.cpp` migrated from `uintptr_t` registry to `CudaStreamGuard` (2026-08-24); full gap closure in progress.
- [~] Representative-hardware p95/p99 baselines: `bench_gpu_a8_baselines.cpp` now emits gate counters and the dedicated Wave-A GPU workflow now has an explicit opt-in self-hosted representative-hardware artifact job; authoritative A100/H100-class execution evidence is still pending because run `34322900559` had no available `gpu-cuda` runner. (Target: Q4 2026)
  - EVIDENCE-NOTE: sandbox environment does not provide CUDA-capable hardware; CI on `develop` remains the authoritative baseline record.
- [~] `release_critical` coverage: timeout/fallback-related targets are registered; resource-exhaustion is quarantined, and green-on-`develop` execution evidence remains pending.

### Wave D Contribution for `gpu`
- [x] Deliver high-cardinality stress coverage for GPU manager hot paths (Wave D: test_gpu_highcardinality_stress.cpp — HighCardinalityKernelSubmit, ConcurrentMemoryAllocStress, MultiGPURoutingStress delivered 2026-09-16)
- [x] Deliver long-duration soak test coverage for GPU manager primary paths (Wave D: test_gpu_manager_soak.cpp — GPUSoak_KernelDispatchThroughput, GPUSoak_MemoryManagerStability, GPUSoak_MultiGPURoutingReliability delivered 2026-09-16)
- [x] Ensure runbook coverage for operator-critical GPU scenarios (Wave D: RUNBOOK_GPU_MANAGER.md — 5 scenarios with log patterns delivered 2026-09-16)
- [x] Deliver dedicated benchmark gates for GPU operability hot paths (Wave D: bench_gpu_dedicated_gates.cpp — GPU-BM-01..04 delivered 2026-09-16)

### Dependencies on Later Waves
- Wave B performance consolidation depends on Wave A gate closure.
- Wave C security validation depends on stable Wave A runtime behavior.
- Wave D operability hardening depends on all prior waves being gate-complete.
