# GPU Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Source-reviewed: 2026-11-06 for the release-plan section; no runtime validation performed -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · PRODUCTION_REQUIREMENTS.md · PERFORMANCE_EXPECTATIONS.md · SECURITY.md -->
<!-- Rollout Plan: ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §4 (Phase D), §7 (risk) -->
<!--
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Exact model backend not verifiable from final document text alone.
-->

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

## Research-based Release Roadmap: v2.5.0 and v3.0.0

This plan is a proposal, not a release commitment. It distinguishes the current
program Waves A–D above from these release-specific steps. The source tree has
broad GPU functionality (CUDA, ROCm/HIP, and Vulkan adapters; resource
governance; query/training paths; tests and benchmark cases), but current
release readiness is **implementation present, evidence and integration
hardening incomplete**. It is not justified to call every backend, fallback,
or security boundary production-ready.

### Research findings and code gaps

| Finding | Current evidence and gap | Roadmap consequence |
|---|---|---|
| GPU speedup is workload- and data-movement-dependent; kernel-only timings can hide query setup, transfer, synchronization, and result materialization costs. | [`RESEARCH.md`](RESEARCH.md) requires CPU comparison and transfer disclosure. [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) maps benchmark sources but declares no universal p99 threshold. | Capture cold and warm end-to-end plus operator timings, with transfer-inclusive and resident-data cases; qualify every speedup by workload and device. |
| Users need clear workload eligibility and backend behavior, not an opaque “GPU enabled” switch. | Existing benchmark cases cover query acceleration, vector search, training, erasure encoding, transfer, and backend comparison; their target availability depends on build configuration. | Publish workload/backend support and break-even guidance so callers can predict supported operations and select CPU/GPU paths. |
| Multi-backend source code does not establish feature or performance parity. | CUDA, ROCm, and Vulkan paths are represented in [`ARCHITECTURE.md`](ARCHITECTURE.md), while enabled features depend on build and device capabilities. | Publish an operation-by-backend capability matrix; unsupported operations must return a typed result or use a verified workload-specific CPU implementation. |
| Test presence or CTest registration is not execution evidence. | [`tests/gpu/CMakeLists.txt`](../../tests/gpu/CMakeLists.txt) currently skips six suites that use a superseded API, including `test_gpu_resource_exhaustion`; CPU/stub tests do not establish device behavior. | Restore coverage by updating or replacing obsolete tests, then record actual CPU and hardware runs separately. Do not merely remove quarantine conditions. |
| Fallback status is not equivalent to correct CPU computation. | [`include/gpu/GPU_CONTRACT.md`](../../include/gpu/GPU_CONTRACT.md) documents that `GPUModule::submitWork` can report fallback success without running the submitted work on CPU. | Verify each workload’s output and error contract; otherwise return an explicit failure and keep acceleration disabled for that operation. |
| Existing isolation controls do not establish a sandbox for hostile kernels or tenant separation across all consumers. | [`wasm_kernel_sandbox.cpp`](wasm_kernel_sandbox.cpp) currently follows a CPU simulation path even with `THEMIS_ENABLE_WASM`; selected LLM/LoRA accounting call sites do not pass tenant IDs (see [`ARCHITECTURE.md`](ARCHITECTURE.md)). | Do not advertise arbitrary-kernel isolation or complete tenant quota enforcement until runtime and cross-tenant tests prove those properties. |
| Historical CUDA-call reduction statements conflict and lack a current reproducible audit. | The root [`ROADMAP.md`](../../ROADMAP.md) records an estimated ~30% reduction, while [`GPU_CUDA_REDUCTION_TRACKING.md`](GPU_CUDA_REDUCTION_TRACKING.md) reports 75% in a historical snapshot but also says formal quantification is pending. | Re-audit the current tree against a reproducible baseline; do not use either percentage as a release result. |

### v2.5.0 — correctness, safety, and reproducible evidence

Scope: close current release blockers without expanding the public API or
claiming unsupported hardware coverage. GPU acceleration remains optional where
required hardware evidence is unavailable.

#### Wave 1 — executable correctness and recovery gates (P0)

**Goal:** Turn existing GPU test sources into executable, trustworthy release evidence.

- [ ] Update obsolete failure/exhaustion suites for the current error API and register the intended `release_critical` cases; keep a suite quarantined until its assertions and linkage are valid (Target: v2.5.0).
- [ ] Exercise workload-specific CPU/GPU output parity, cancellation/timeout, allocation failure, and device-unavailable behavior; distinguish “fallback executed” from “fallback unavailable” (Target: v2.5.0).
- [ ] Re-run the CUDA-call and RAII ownership audit on current `src/gpu/` and `include/gpu/` sources; compare with the exact baseline revision and the root roadmap’s ≥40% Wave A reduction gate (Target: v2.5.0).

**Dependencies:** Current public error and dispatch contracts; working CPU CI; a
CUDA runner for hardware-only cases.<br>
**Risk:** Old suites may encode superseded APIs or assert policy classification
without executing a workload. Fix the tests and contracts rather than treating
source presence as coverage.<br>
**Effort:** 3–5 person-weeks, excluding runner provisioning.<br>
**Success metrics:** 100% of in-scope release-critical tests build, are
registered, and have a recorded execution result; zero quarantined release
gates; the current-source CUDA audit either demonstrates the ≥40% reduction
against the same baseline or records a release blocker for human disposition;
every enabled workload either returns a parity-checked result or an
explicit error; zero release claims based only on a CPU stub.<br>
**Validation:** Focused GPU CTest on CPU-only and supported CUDA configurations;
fault injection for timeout, quota/OOM, backend loss, and invalid capability;
sanitizer runs where supported; archive configuration, device/runtime, command,
and results.

#### Wave 2 — tenant, kernel, and failure-boundary review (P0)

**Goal:** Ensure the enabled execution and accounting paths enforce the declared tenant and trust boundaries.

- [ ] Trace tenant identity through each enabled allocation/accounting path and add concurrent cross-tenant quota and cleanup regressions (Target: v2.5.0).
- [ ] Define the supported trust boundary for kernel inputs; keep third-party/untrusted kernel execution disabled unless a real isolated runtime and its limits are implemented and independently reviewed (Target: v2.5.0).
- [ ] Verify that each diagnostic, audit, quota-denial, and backend-degradation path reports a truthful result without converting an unexecuted fallback into success (Target: v2.5.0).

**Dependencies:** Wave 1 test harness; deployment threat model and edition
policy owners.<br>
**Risk:** The current WASM-named implementation is not a WASM isolation runtime;
claiming otherwise would create a false security boundary. Fixing tenant
propagation may expose existing accounting assumptions in downstream modules.<br>
**Effort:** 3–5 person-weeks.<br>
**Success metrics:** Zero cross-tenant quota bypasses in concurrent tests; 100%
of enabled allocation consumers either account against the correct tenant or
are explicitly documented as non-tenant-scoped; no untrusted-kernel execution
claim without an isolated runtime and reviewed threat model; no silent-success
fallback outcomes.<br>
**Validation:** Adversarial quota and device-loss tests, static review of
ownership/error paths, and security review of enabled execution boundaries.

#### Wave 3 — representative performance baselines (P1)

**Goal:** Establish workload-specific GPU value and regression budgets from reproducible, representative measurements.

- [ ] Run the mapped GPU benchmark set against the same CPU reference inputs; report cold-start and warm/steady-state cases, transfer inclusion, batch/concurrency, device/driver/runtime, build flags, and output validation (Target: v2.5.0).
- [ ] Establish approved per-case p95/p99 and throughput budgets from representative hardware; retain raw results and profiler artifacts rather than importing historical estimates (Target: v2.5.0).
- [ ] Keep a GPU route disabled or CPU-selected when it fails its measured break-even gate; for Category A distance/Top-K candidates, validate the existing proposed ≥1.5× speedup gate before enabling (Target: v2.5.0).

**Dependencies:** Wave 1 correctness; representative CUDA hardware and
configured CI runner; benchmark targets in root<br>
`benchmarks/CMakeLists.txt`.<br>
**Risk:** GPU runner scarcity, thermal/driver variance, and cold-cache effects
can make results incomparable. No numeric universal p99 limit is currently
declared by the module.<br>
**Effort:** 2–4 person-weeks plus hardware scheduling.<br>
**Success metrics:** 100% of release-mapped benchmark cases have a result or an
explicit hardware-blocked status; each result includes p50/p95/p99, throughput,
memory use, CPU comparison, and full environment metadata; Category A routes
meet ≥1.5× or remain disabled; no >10% regression against an approved
per-case baseline without an explicit waiver.<br>
**Validation:** Repeated runs with fixed inputs and warm-up; compare
transfer-inclusive end-to-end latency and kernel-only time; verify result
parity. The 10% target is a proposed per-case guard, not a universal p99
contract. Separately report the declared control-plane targets (device
selection ≤100 µs, allocation ≤1 ms, quota check ≤10 µs, diagnostics ≤100 µs)
as measured or unverified; contract declarations alone are not results.

#### Wave 4 — v2.5.0 release handoff (P0)

**Goal:** Make every v2.5.0 GPU readiness claim reproducible, reviewable, and reversible.

- [ ] Publish a signed evidence manifest linking source revision, enabled backend, test results, benchmark artifacts, threat-model review, and known limitations (Target: v2.5.0).
- [ ] Keep each optional backend behind its existing capability/feature gates until its corresponding build, focused tests, and hardware evidence pass; update GPU runbook and release notes from the verified results (Target: v2.5.0).

**Dependencies:** Waves 1–3; platform/release and security reviewers.<br>
**Risk:** Missing hardware or owner sign-off can delay the GPU release claim;
the safe outcome is to defer that backend’s readiness, not to infer a pass.<br>
**Effort:** 1–2 person-weeks.<br>
**Success metrics:** Every v2.5.0 GPU readiness statement resolves to current,
reviewed evidence; all release-critical checks pass or have an explicit
blocker/waiver; rollback is a tested feature-gate/configuration change.<br>
**Validation:** Release-candidate CI on the declared backend matrix, artifact
and link audit, rollback drill, and human sign-off.

### v3.0.0 — portable, multi-tenant, and operable acceleration

Scope: strategic architecture work after v2.5.0 correctness contracts and
representative baselines exist. v3.0.0 must not assume CUDA, HIP, and Vulkan are
interchangeable or that process/container sharing is a hostile-tenant security
boundary.

#### Wave 5 — backend capability and parity contract (P1)

**Goal:** Make cross-backend support explicit and preserve workload correctness despite backend-specific features.

- [ ] Define the common operation/capability contract and explicit CUDA, ROCm/HIP, and Vulkan feature matrix; expose unsupported, disabled, and degraded states distinctly (Target: v3.0.0).
- [ ] Implement cross-backend conformance tests for each operation advertised by that backend and document numerical tolerances and deterministic ordering requirements (Target: v3.0.0).

**Dependencies:** v2.5.0 Waves 1 and 3; backend owners and supported-device
inventory.<br>
**Risk:** A lowest-common-denominator API can reduce backend-specific
performance, while separate behavior can fragment caller semantics.<br>
**Effort:** 6–10 person-weeks.<br>
**Success metrics:** 100% of advertised backend/operation pairs have a passing
conformance result on declared hardware; exact operators match exactly and
floating-point/vector outputs meet documented per-operation tolerances (the
existing vector-search proposal is relative error <1e-3); unsupported pairs
fail explicitly or use verified CPU execution.<br>
**Validation:** Backend CI matrix, deterministic parity corpus, numerical
edge cases, and transfer-inclusive benchmark comparison.

#### Wave 6 — topology-aware multi-GPU scheduling (P1)

**Goal:** Improve scaling only where measured end-to-end benefit exceeds data-movement and coordination overhead.

- [ ] Measure partitioning, peer transfer, queueing, and synchronization costs; use workload shape and measured break-even thresholds to select CPU, one GPU, or multiple GPUs (Target: v3.0.0).
- [ ] Add deterministic recovery tests for device loss, stale topology, partial completion, and retry safety; retry only operations whose semantics permit it (Target: v3.0.0).

**Dependencies:** Wave 5 contracts; representative 1/2/N-device systems and
topology telemetry.<br>
**Risk:** Interconnect and workload imbalance can erase nominal compute scaling;
blind retries can duplicate non-idempotent work.<br>
**Effort:** 6–10 person-weeks.<br>
**Success metrics:** Report end-to-end scaling for 1, 2, and available N-device
cases; for workloads classified as data-parallel, target ≥1.5× end-to-end
speedup at two devices (confirm or revise after a Wave 5 pilot); zero duplicate
commits/results in injected-failure tests; all scheduler choices have
observable reasons.<br>
**Validation:** Same-input scaling benchmarks including transfer and
coordination; seeded fault injection; compare against single-device and CPU
results.

#### Wave 7 — explicit tenant isolation and trusted execution policy (P1; gate before enabling untrusted execution)

**Goal:** Prevent accidental claims of isolation and make tenant resource ownership auditable across supported paths.

- [ ] Separate trusted built-in kernels from tenant-supplied code in policy and API contracts; keep untrusted execution denied until an actual sandbox/isolation design has passed an independent security review (Target: v3.0.0).
- [ ] Enforce tenant identity, quotas, memory lifetime, admission, and audit attribution end-to-end across every enabled backend and consumer (Target: v3.0.0).
- [ ] Evaluate hardware partitioning (for supported NVIDIA MIG configurations) as an optional deployment control, not a portable or sufficient software sandbox (Target: v3.0.0).

**Dependencies:** Wave 2 threat model and tenant tests; backend capability
contract; security architecture approval.<br>
**Risk:** Hardware partitioning is vendor-/device-specific; a WASM runtime,
MIG, MPS, process isolation, or container boundary alone must not be presented
as universal protection from malicious kernels or side channels. See NVIDIA’s
[MPS documentation](https://docs.nvidia.com/deploy/mps/latest/index.html) for
its multi-process sharing model.<br>
**Effort:** 6–12 person-weeks, plus external review and hardware access.<br>
**Success metrics:** Zero cross-tenant data or quota leakage in stress and
fault-injection tests; all resource usage and denials attributable to a tenant;
untrusted execution either remains disabled or passes a documented isolation,
memory-limit, time-limit, and escape-test suite; zero unresolved critical/high
findings at the release gate.<br>
**Validation:** Threat-model review, hostile-input/kernel tests where enabled,
quota fuzzing, sanitizer/driver diagnostics, independent security review, and
deployment-specific MIG validation when used.

#### Wave 8 — operations, governance, and v3.0.0 acceptance (P1)

**Goal:** Make GPU behavior observable and the release support promise reproducible for operators.

- [ ] Correlate query-level latency, transfer bytes, queue wait, fallback/error reason, memory pressure, and backend health with vendor device telemetry (Target: v3.0.0).
- [ ] Publish a reproducible backend support matrix, dependency/license inventory, operator runbooks, SLOs, upgrade/rollback procedure, and signed evidence bundle (Target: v3.0.0).

**Dependencies:** Waves 5–7; observability, packaging, security, and release
owners.<br>
**Risk:** Vendor telemetry can be unavailable or non-comparable; never make
release acceptance depend on a metric that the supported backend cannot emit
without documenting an equivalent.<br>
**Effort:** 3–6 person-weeks.<br>
**Success metrics:** 100% of supported deployment profiles expose the declared
minimum health/error/fallback signals; all advertised backend profiles have a
reproducible test/benchmark record and rollback path; required supply-chain,
security, and human approvals are present.<br>
**Validation:** Failure/soak drills, telemetry completeness checks, fresh
environment reproduction, artifact/license audit, and release sign-off.

### Open assumptions and trade-offs

- Release dates, target device families, supported driver/runtime ranges, and
  the availability of dedicated CUDA/ROCm/Vulkan hardware are not established
  by this plan; effort is person-weeks and excludes infrastructure lead time.
- No current, comparable GPU benchmark result or numeric universal p99 budget
  is established by the reviewed module docs. Budgets must be approved per
  workload and hardware profile before performance gates are enforced.
- The ≥1.5× Category A break-even target and <1e-3 vector-search error are
  existing proposals in module benchmark/future-enhancement sources; they are
  not reported as measured results.
- Isolation requirements depend on the deployment threat model. The plan makes
  no compliance certification claim and treats vendor hardware partitioning as
  an optional control with separate validation.
- The generated developer wiki was last refreshed 2026-11-02 and reports a
  large source delta; it is context only. Current module sources and the
  2026-11-06 root roadmap take precedence until wiki synchronization is
  refreshed.

### Research references

1. Williams, Waterman & Patterson, “Roofline: An Insightful Visual Performance
   Model for Multicore Architectures,” CACM 52(4), 2009,
   [doi:10.1145/1498765.1498785](https://doi.org/10.1145/1498765.1498785).
   Use as a compute/bandwidth ceiling model; adapt it for query planning,
   launch, synchronization, and transfer costs.
2. Amdahl, “Validity of the Single Processor Approach to Achieving Large Scale
   Computing Capabilities,” AFIPS, 1967,
   [IEEE record](https://ieeexplore.ieee.org/abstract/document/4785615).
   Accelerating one operator cannot remove serial/host/transfer fractions.
3. Shanbhag et al., “Revisiting Query Performance in GPU Database Systems,”
   2023, [arXiv:2302.00734](https://arxiv.org/abs/2302.00734). The examined
   systems show that setup and data movement can dominate some query cases;
   this is a motivation to measure, not a universal overhead estimate.
4. NVIDIA, [CUDA C++ Best Practices Guide 13.0](https://docs.nvidia.com/cuda/archive/13.0.2/pdf/CUDA_C_Best_Practices_Guide.pdf),
   2025; recommends profiling and minimizing host/device transfers.
5. Mattson et al., “MLPerf Inference Benchmark,” 2019,
   [arXiv:1911.02549](https://arxiv.org/abs/1911.02549). Its disciplined,
   scenario-specific system measurement is a methodology precedent, not a
   database workload benchmark.
6. AMD, [HIP Porting Guide](https://rocm.docs.amd.com/projects/HIP/en/develop/how-to/hip_porting_guide.html);
   Khronos, [Vulkan Portability Subset](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDevicePortabilitySubsetFeaturesKHR.html).
   Porting and feature support require per-backend capability validation.
7. NVIDIA, [Multi-Instance GPU User Guide](https://docs.nvidia.com/datacenter/tesla/mig-user-guide/latest/introduction.html);
   Sridhara et al., “AcAi: Protecting Accelerator Execution with Arm
   Confidential Computing Architecture,” USENIX Security 2024,
   [paper](https://www.usenix.org/system/files/usenixsecurity24-sridhara.pdf).
   These describe specific isolation approaches, not a universal GPU sandbox.
8. NVIDIA, [DCGM](https://developer.nvidia.com/dcgm); AMD,
   [AMD SMI](https://rocm.docs.amd.com/projects/amdsmi/en/develop/how-to/amdsmi-cli-tool.html).
   Device health telemetry should complement database-level workload metrics.
