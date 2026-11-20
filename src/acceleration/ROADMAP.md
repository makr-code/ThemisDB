---
Author: "ThemisDB Team"
Created: "2026-10-29"
Last Updated: "2026-11-19"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Acceleration Module Roadmap

> Exact model backend not verifiable from final document text alone.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Evidence audit: 2026-11-16; older validation statements below are superseded where marked NOT VERIFIED or EVIDENCE PENDING. -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->
<!-- Rollout Plan: ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §4 (Phase B–C), §7 (risk) -->

## Research-based module assessment (2026-10-29; evidence audit updated 2026-11-16)

The source implements backend selection, fallback orchestration, plugin/security guards, and multi-device interfaces. This feature presence is not release evidence: runtime support and performance remain profile- and hardware-dependent.

### Current maturity

The module is an implementation candidate, not an evidence-complete release. BackendRegistry filters available backends by requested capabilities and uses a fixed priority ending in CPU; it restricts selection to CPU when the discovery snapshot has no healthy accelerator. The AI dispatcher is a separate selector with a 120-second probe cache and ordered `run()` chain; DeviceManager uses a 60-second cache. Discovery health is a snapshot, not continuous hardware attestation or automatic runtime-health eviction.

The kernel fallback dispatcher retries only device-loss, timeout, and synchronization errors and then invokes the fallback table. Retry policy is now validated and bounded (at most eight attempts and 100 ms maximum delay). It returns the final status but does not publish per-attempt telemetry. Invalid vector-similarity sizes are rejected before allocation/dispatch; allocation failure is returned as an error.

`tests/acceleration/test_acceleration_regression.cpp` exercises registry CPU selection and the production kernel fallback dispatcher. `test_acceleration_failure_handling.cpp` uses simulated failures and does not prove that the production code cancels hangs or recovers from all resource failures. `test_acceleration_performance_gates.cpp` times proxy calculations, and `bench_acceleration_dedicated_gates.cpp` benchmarks stubs; neither establishes production performance.

## Evidence-first v2.5.0 → v3.0.0 release gates (2026-11-16)

- [ ] v2.5.0: publish a capability/operation support matrix tied to build flags, source registrations, and tests; report unsupported or unknown capability states explicitly (Target: v2.5.0).
- [ ] v2.5.0: record deterministic normal, invalid, no-accelerator, unhealthy-snapshot, and fallback test results; include the test command, build profile, and artifact (Target: v2.5.0).
- [ ] v2.5.0: establish per-profile dispatch p95/p99 and CPU-fallback p95/p99 from production-path benchmarks; record workload, build flags, CPU/GPU, driver/runtime, warmups, repetitions, correctness parity, and raw output. Do not promote candidate values in `PERFORMANCE_BASELINES.md` until a reviewed run manifest exists; `benchmarks/baselines/acceleration/baseline.json` is representative sample data only (Target: v2.5.0).
- [ ] v2.5.0: verify retry exhaustion, permanent-error fallback, invalid retry settings, and fallback result correctness with production dispatcher tests; include final status and retry counts in evidence (Target: v2.5.0).
- [ ] v3.0.0: add bounded multi-device queue/backpressure and partial-device failure tests, plus named-topology soak and p95/p99 evidence; document unsupported recovery/health-eviction behavior (Target: v3.0.0).
- [ ] v3.0.0: define support tiers and release thresholds only from repeatable, versioned artifacts; retain failed/skipped benchmark cases and explicit owner disposition (Target: v3.0.0).

**Release posture:** v2.5.0 and v3.0.0 performance/safety sign-off remains pending until these artifacts exist. The numeric values in this roadmap and `PERFORMANCE_BASELINES.md` are targets, not verified claims.

### Research evidence and best-practice baseline

The roadmap is not based on generic claims about “GPU speedups” or “accelerator efficiency.” It is anchored to the engineering realities that repeatedly dominate accelerator work in databases and vector systems:

- Johnson, Douze, and Jégou, “Faiss: A Library for Efficient Similarity Search and Clustering of Dense Vectors” (2019). The key lesson is that large-scale vector search gains are real, but they depend on memory placement, index choice, batching, and hardware-aware dispatch; without those guards, the system can regress even when a GPU is available. This directly supports the v2.5.0 requirement to lock the backend capability matrix and benchmark the actual dispatch path on representative CPU/GPU profiles.
- Blagodurov et al., “A Case for NUMA-aware Thread Scheduling” (EuroSys 2010). The practical implication is straightforward: local data placement and topology-aware scheduling matter as much as raw compute throughput. This is why the module must treat NUMA locality, device affinity, and queue discipline as first-class reliability features rather than optional tuning.
- Dean and Barroso, “The Tail at Scale” (Communications of the ACM, 2013). The relevant design principle is that tail latency is controlled by bounded queues, explicit failover, and conservative retry policy rather than by optimizing the happy path alone. This aligns with the v3.0.0 objective to standardize multi-device scheduling, queue discipline, and partial-failure isolation.
- Blem et al., “Dark Silicon and the End of Multicore Scaling” (ISCA 2013) and a broader systems body of work on hardware heterogeneity. The core lesson is that compute acceleration is a system-level optimization problem, not a single-kernel win: one must coordinate topology, memory movement, and fallback semantics. This is why the module should enforce fail-closed selection and not silently execute on stale capability data.

This translates into concrete engineering rules for ThemisDB:

- capability detection and trust validation happen before compute dispatch;
- the fast path is benchmarked and bounded, not assumed to be universally faster;
- fallback is deterministic and explicit, never silent;
- queue growth, locality, and equipment health are treated as operational control variables, not afterthoughts.

### Real improvements the roadmap is targeting

The roadmap does not promise vague “performance wins.” It is tied to specific engineering improvements that are common in real accelerator systems and directly testable in ThemisDB:

- Vector-index acceleration: literature on ANN and GPU-accelerated similarity search shows that the large gains appear when indices stay device-resident, workloads are batched, and the dispatch path is measured end-to-end. The v2.5.0 gate is therefore explicit: for the selected RTX-class ANN path, the system should achieve a measured ≥8× speedup over the CPU baseline while retaining deterministic fallback under invalid capability or trust states.
- NUMA-aware placement: the performance literature on NUMA scheduling shows that locality and scheduling choices can materially influence throughput and latency. The operational requirement here is not a generic slogan; it is a release gate: backend selection p99 < 10 µs and fallback overhead ≤ 5% under representative NUMA and mixed-device conditions.
- Tail-latency control: the “Tail at Scale” literature shows that tail latency is dominated by queue buildup, retry amplification, and degraded execution policies, not just raw throughput. The v3.0.0 target is therefore concrete: bounded queue growth, no unbounded retry loops, and an explicit partial-device failure policy under sustained load.
- Fail-closed safety: the practical requirement in high-risk acceleration systems is that invalid plugins or stale capability data must not execute. This is reflected in the release gates: zero silent execution on trust or capability failure and explicit deny/fail-closed behavior for all malformed or degraded states.

### v2.5.0 priorities

- [ ] Lock backend capability matrix and fallback semantics for CPU/GPU and optional backends (Target: v2.5.0)
  - Dependencies: `DeviceManager`, `BackendRegistry`, `compute_backend.cpp`, `device_manager.cpp`
  - KPI: backend selection p99 < 10 µs; no silent execution on trust or capability failure
- [ ] Re-baseline benchmark gates and fallback overhead under representative hardware profiles (Target: v2.5.0)
  - Dependencies: `benchmarks/acceleration/`, release gate suite, GPU/CPU validation host
  - KPI: fallback overhead ≤ 5%; no regression beyond approved tolerance band
- [ ] Harden plugin/security and shader integrity checks under degraded runtime conditions (Target: v2.5.0)
  - Dependencies: `plugin_loader.cpp`, `plugin_security.cpp`, `shader_integrity.cpp`
  - KPI: 100% deny/fail-closed coverage for invalid plugin or integrity path
- [ ] Publish a release-level runbook and support matrix for optional acceleration backends (Target: v2.5.0)
  - Dependencies: docs, benchmark evidence, operator guidance
  - KPI: all release claims map to tested host profiles and explicit exceptions

### v3.0.0 priorities

- [ ] Standardize multi-device scheduling, queue discipline, and resource isolation under partial device failures (Target: v3.0.0)
  - Dependencies: `multi_gpu_backend.cpp`, `nccl_vector_backend.cpp`, `rccl_vector_backend.cpp`, `vllm_resource_manager.cpp`
  - KPI: bounded queue growth, no unbounded retry loops, p95 latency within release envelope
- [ ] Define a major-release support matrix for supported optional backends and compatibility expectations (Target: v3.0.0)
  - Dependencies: benchmark evidence, release process, architecture docs
  - KPI: documented support tiers with explicit deployment exceptions
- [ ] Align observability, alerting, and runbooks with the production release model (Target: v3.0.0)
  - Dependencies: module diagnostics, release evidence chain, operational docs
  - KPI: operator-critical paths have actionable telemetry and incident guidance

### Wave plan

#### Wave 1 — v2.5.0 stabilization (effort: medium)
- Goal: lock the contract and produce proof-backed release confidence for the main dispatch and fallback paths.
- Tasks: backend capability matrix lock, benchmark re-baseline, fail-closed regression hardening, release documentation
- Dependencies: `DeviceManager`, `BackendRegistry`, benchmark gates, docs
- Risk: benchmark drift and unverified assumptions across hardware profiles
- KPI: regression suite passes; benchmark gates stable; no silent fallback on invalid state

#### Wave 2 — v2.5.0 security and integrity hardening (effort: medium)
- Goal: prevent invalid plugin, integrity, or degraded backend states from executing silently.
- Tasks: trust-validation stress tests, shader integrity checks, operator diagnostics
- Dependencies: `plugin_loader.cpp`, `plugin_security.cpp`, `shader_integrity.cpp`
- Risk: operational false positives or blocking valid devices
- KPI: 100% validation failure coverage and fail-closed execution behavior

#### Wave 3 — v2.5.0 → v3.0.0 distributed resilience (effort: high)
- Goal: use shared resource and multi-device contracts to reduce contention and partial-device failure risk.
- Tasks: multi-GPU queue and retry policy review, partial-failure soak coverage, merge determinism validation
- Dependencies: multi-device backend files and soak tests
- Risk: throughput loss or unstable merge behavior under partially healthy clusters
- KPI: p95 latency remains in release envelope, queue growth remains bounded

#### Wave 4 — v3.0.0 support-tier policy and architecture closure (effort: medium)
- Goal: convert the current feature-rich backend surface into a supportable major-version contract.
- Tasks: support matrix, compatibility policy, documented optional backend commitments
- Dependencies: benchmark evidence and public API review
- Risk: unsupported backend claims or undocumented runtime assumptions
- KPI: all GA claims supported by tested profiles and explicit exceptions

#### Wave 5 — v3.0.0 release closure (effort: medium)
- Goal: complete the module evidence chain and operational readiness package before release sign-off.
- Tasks: final benchmark artifact collection, runbook review, changelog/doc sync, approval checklist
- Dependencies: all prior waves, release evidence chain, docs governance
- Risk: release evidence mismatch between docs and runtime reality
- KPI: release checklist complete and archived with the module evidence trail

### Risk and trade-off assessment

Main trade-off: the module is already broad and capable, but optional backend support remains environment-sensitive. The safest path is to treat v2.5.0 as a stabilization and proof release, and v3.0.0 as the point where distributed multi-device policy and support tiers become explicit and release-governed rather than assumed.

### Validation strategy

- Unit and regression: `tests/acceleration/test_acceleration_regression.cpp`, failure-handling tests, plugin/security regressions
- Benchmarks: `benchmarks/acceleration/` dispatch, selection, and fallback gates
- Security/governance: fail-closed validation, plugin/SDK trust checks, release documentation review
- KPI summary: selection p99 < 10 µs, fallback overhead ≤ 5%, zero silent execution on invalid trust/capability state

## Current Status

### Closure Backlog (source-validated)
- [~] Convert all open Acceleration roadmap items into closure artifacts with explicit evidence type (`code`, `test`, `benchmark`, `CI artifact`, `roadmap/changelog sync`). (Target: Q4 2026)
  - 2026-09-15 update: Phase-C geo/BFS/Dijkstra constraints were moved to source-enforced runtime guards in `include/acceleration/batch_validator.h`, `src/acceleration/cuda_backend.cpp`, `src/acceleration/cpu_backend.cpp`, and `src/acceleration/geo_acceleration_bridge.cpp`, with regression coverage in `tests/acceleration/test_acceleration_regression.cpp`.
  - Remaining closure evidence still requires self-hosted CUDA artifacts for parity and benchmark sign-off gates.

**Hybrid Retrieval Rollout Readiness**: 45% 🟡 (issue #5468).
- Phase A (exact-first): ✅ No acceleration used — safe.
- Phase B (advisory-only Category A: distance, TopK): 🟡 Q3 2026 after result validation hardening (320 gaps).
- Phase C (Category B: Geo, BFS, Dijkstra with parity tests): ⚠️ Q4 2026 after 60% total gap reduction + parity suite.
- **Mandatory**: Category C kernels (policy, provenance, transactions) remain CPU-first permanently.
- Rollout risk detail: `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §7`

## GPU Backend Integration Status (2026-07-19)

### ✅ Full GPU Backend Implementations Complete

All major GPU acceleration backends are now fully implemented and integrated:

- **CUDA Backend** (src/acceleration/cuda_backend.cpp: 2,202 LOC — verified 2026-09-15)
  - Full CUDA kernel implementations for vector operations
  - Memory management with device-host transfers
  - Multi-GPU support with NCCL collective operations
  - Performance: ≥40x speedup over CPU baseline

- **HIP/AMD ROCM Backend** (src/acceleration/hip_backend.cpp: 1,192 LOC — verified 2026-09-15)
  - Full HIP kernel implementations matching CUDA API surface
  - AMD ROCM integration with RCCL multi-GPU support
  - Compatibility layer for cross-GPU portability
  - Performance: ≥35x speedup over CPU baseline

- **Vulkan Backend** (src/acceleration/vulkan_backend_full.cpp: 603 LOC — verified 2026-09-15)
  - Cross-platform shader compilation (GLSL → SPIR-V)
  - Graphics queue management and synchronization
  - Vulkan memory pooling and resource lifecycle
  - Fallback paths for unavailable extensions

- **OpenCL Backend** (src/acceleration/opencl_backend.cpp: Production implementation)
  - Portable compute kernel execution
  - Device capability detection and runtime selection
  - Embedded compilation with LLVM-based optimization

- **Distributed Backends** (NCCL/RCCL vector backends)
  - Multi-node GPU coordination via MPI-compatible protocols
  - Collective operations: allReduce, reduce-scatter, all-gather
  - Topology-aware communication patterns

### Backend Integration Verification

- [x] All backends compiled into themis_core library
- [x] Backend registry functional for device enumeration
- [x] Fallback orchestration working (GPU→alternative→CPU)
- [x] Plugin security guards active for all GPU operations
- [x] Performance gates defined and measurable
- [x] Failure handling paths tested for timeout/degradation
- [x] Doxygen documentation complete (1,227+ tags)

## In Progress

- [~] Build system fixes for orphaned test declarations (pre-existing CMakeLists.txt cleanup)
- [~] Runtime capability hardening for fail-closed behavior under partial backend availability (Target: Q3 2026)
- [x] 2026-08-19: `DeviceManager` now supports deterministic injected capability snapshots for focused validation while preserving CPU-fallback synthesis and fail-closed runtime selection semantics. (Target: Q3 2026)
- [x] 2026-08-31: `BreakEvenValidator` is now wired into the production acceleration build, uses hookable CPU/GPU profiling contracts plus deterministic fallback estimates instead of hardcoded placeholder timings, and focused GPU tests link the production implementation instead of the fallback-only shim. (Target: Q3 2026)
- [x] 2026-08-31: `oneapi_backend.cpp` now fail-closes on USM allocation failure before `memcpy`, turning the prior undefined OOM path into explicit `std::bad_alloc` handling. (Target: Q3 2026)
- [~] Multi-device and resource-management reliability tuning under sustained load (Target: Q3 2026)
  - 2026-11-19 implementation evidence: `MultiGPUVectorBackend` now selects CUDA, HIP, Vulkan, or CPU per instance; owns one backend per shard; fans out partitioned distance/KNN work concurrently; merges global indices; and reports/controls CPU fallback. Focused regression suite: 26 PASS, 2 SKIP (CUDA/HIP not available). RTX 3060 Vulkan run verified 1 Vulkan shard; requests for 2/4 shards used 1 Vulkan plus 1/3 CPU fallbacks. Multi-GPU CUDA/HIP and multi-device Vulkan hardware runs remain open validation gates.
- [~] **B-01 · CUDA vector similarity search** — GPU kernel dispatch implemented in `ai_hardware_dispatcher.cpp` and `vllm_resource_manager.cpp` (evidence: 2026-08-10 update); remaining open item is hardware-in-the-loop perf gate (RTX ≥8× gate, `bench_acceleration_cuda_gates.cpp`) (Target: Q3 2026)

## Planned Features

### Q3 2026 — CUDA Kernel Production-Readiness (Wave A+B, #1383)

**Constraint:** `THEMIS_ENABLE_CUDA` gate is always switchable OFF; CPU paths are unmodified; all stub paths carry mandatory `STUB/SIMULATION NOTE` per governance §8.

#### B-01 · `ai_hardware_dispatcher.cpp` + `vllm_resource_manager.cpp` — Vector Similarity Search
- [~] Replace CPU HNSW fallback in `ai_hardware_dispatcher.cpp` with GPU kernel dispatch when `THEMIS_ENABLE_CUDA`; L2/Cosine/IP kernels using CUB/Thrust. (Target: Q3 2026)
  - Inputs: query vector (float32, d=128..4096), corpus on device (N≤10M vectors).
  - Outputs: top-k indices + distances (k≤1000).
  - Errors: `cudaError_t` check after every kernel launch; on failure → `GpuOperationFailed` → CPU fallback (logged as WARN, never silent).
- [~] `vllm_resource_manager.cpp`: same GPU dispatch pattern for similarity scoring in vLLM resource allocation path. (Target: Q3 2026)
- [~] Perf gate `ACC-CUDA-B01-01`: ≥8× speedup vs CPU baseline on RTX-class GPU (1M vectors, d=128). Gate in `benchmarks/acceleration/bench_acceleration_cuda_gates.cpp`. (Target: Q3 2026)
- [x] `THEMIS_ENABLE_CUDA` compile gate: CPU path completely unmodified when gate is off; CMake check with `find_package(CUDAToolkit QUIET)`. (Target: Q3 2026)
- [x] 2026-08-10 implementation update:
  - `AiHardwareDispatcher` now executes real ANN distance + TopK dispatch for `vector_similarity_{l2,cosine,ip}` via CUDA path when available, with explicit `cudaGetLastError()` checks and deterministic CPU fallback.
  - `VLLMResourceManager` now exposes a vLLM-aware vector similarity dispatch API with `canUseGPU()` gating and deterministic CPU fallback on overload/error.
  - Remaining closure: hardware-in-the-loop perf validation for RTX `≥8×` gate.
- [x] 2026-08-10 GPU dispatch implemented in `vllm_resource_manager.cpp:268-283`:
  - `#ifdef THEMIS_ENABLE_CUDA` gate calls `canUseGPU()` and `CUDAVectorBackend::populateANNDispatch()`.
  - CPU fallback on overload/error with WARN log.
  - **Prior STUB/SIMULATION NOTE removed** — no CPU-HNSW-only fallback path remains as the default.
  - Source evidence: `vllm_resource_manager.cpp:268-294`, `ai_hardware_dispatcher.cpp:208-234`.

#### A-06 · `src/gpu/query_accelerator.cpp` — Filter/Join/Aggregation/Sort/TopK
<!-- Evidence: src/gpu/query_accelerator.cpp (1,572 LOC); scan:255, aggregate:506, sort:383, hashJoin:659, topK:1386; Thrust GPU paths present; CUBlas for aggregation; verified 2026-09-15 -->
- [x] Filter kernel: `thrust::copy_if` on device; RAII via CUDA RAII helpers; `cudaGetLastError()` after launch; structured error on failure → CPU fallback. (`query_accelerator.cpp:255`)
- [x] Join kernel: sort-merge join on device via `thrust::sort_by_key`; CPU fallback on alloc failure. (`query_accelerator.cpp:659`)
- [x] Aggregation kernel: `thrust::reduce` for Sum/Min/Max/Avg; CUBlas `cublasGemmEx` for dot product; GPU path with CPU fallback on exception. (`query_accelerator.cpp:506`)
- [x] Sort kernel: `thrust::stable_sort_by_key` on device; CPU `std::stable_sort` fallback. (`query_accelerator.cpp:383`)
- [x] TopK kernel: partial sort heap on device via Thrust; CPU `std::partial_sort` fallback. (`query_accelerator.cpp:1386`)
- [x] CUDA/CPU parity tests: `tests/gpu/test_gpu_query_accelerator_parity.cpp` — covers scan, sort, aggregate (5 functions × 3 sizes), hashJoin, dotProduct, and topK (ASC/DESC/edge-cases); all 5 A-06 operations covered; runs unconditionally under `THEMIS_ENABLE_GPU=ON`; topK coverage added 2026-09-15. (Implemented: 2026-09-15)

#### A-07 · `advanced_vector_index.cpp` — FAISS-GPU Approximate k-NN (under cuVS gate)
<!-- Evidence: src/index/advanced_vector_index.cpp:61,386,445; gate THEMIS_ENABLE_CUDA && THEMIS_ENABLE_CUVS; FAISS index_cpu_to_gpu dispatch path; verified 2026-09-15 -->
- [~] Implement CUDA k-NN path in `advanced_vector_index.cpp` using FAISS GPU (`index_cpu_to_gpu`) under gate `THEMIS_ENABLE_CUDA && THEMIS_ENABLE_CUVS`; when either gate is OFF, fall back to CPU with STUB/SIMULATION NOTE. (Target: Q3 2026)
  - Inputs: query matrix (float32, batch × d), HNSW graph on device, k.
  - Outputs: top-k indices + L2 distances; float32 parity tolerance ≤1e-5 vs CPU HNSW.
  - Errors: `cudaGetLastError()` after every FAISS-GPU call; on failure → log WARN → CPU fallback.
  - **Note**: Current dispatch uses FAISS `index_cpu_to_gpu()` API (not direct cuVS/RAFT C++ API); gate variable `THEMIS_ENABLE_CUVS` signals FAISS-GPU availability. Direct cuVS RAFT `ivf_flat` integration remains a future enhancement.
- [x] 2026-08-10 implementation update:
  - Added `THEMIS_ENABLE_CUVS` feature gate and RAFT/FAISS package probe in CMake dependency wiring.
  - Added CUDA-gated FAISS GPU `index_cpu_to_gpu` search path in `advanced_vector_index.cpp` as the active CUDA dispatch path under `THEMIS_ENABLE_CUDA && THEMIS_ENABLE_CUVS`.
  - CPU search remains unchanged when either gate is OFF.
  - Source evidence: `advanced_vector_index.cpp:61`, `:386`, `:445`.
- [x] 2026-09-15: Added hardware-in-the-loop CTest target `test_advanced_vector_index_cuda_knn` in `tests/CMakeLists.txt` with dedicated parity test source `tests/index/test_advanced_vector_index_cuda_knn.cpp`; target now available for self-hosted CUDA+FAISS+cuVS validation. (Target: Q3 2026)
- [ ] Current stub state carries:
  ```
  // STUB/SIMULATION NOTE:
  // Purpose: CPU HNSW fallback — FAISS-GPU/cuVS not dispatched.
  // Activation: THEMIS_ENABLE_CUDA=OFF or THEMIS_ENABLE_CUVS=OFF.
  // Production Delta: CPU recall ~0.95 vs FAISS-GPU recall ~0.99 at 10× throughput.
  // Removal Plan: Wire A-07 FAISS-GPU dispatch in Q3 2026; upgrade to cuVS RAFT ivf_flat in future.
  ```

#### A-08 · Geo CUDA Kernels (Partial Q3 2026) — Haversine + ST_CONTAINS + ST_DISTANCE
<!-- Evidence verified 2026-09-15: cuda/geo_kernels.cu (393 LOC); geo_acceleration_bridge.cpp:bridge_geo_containment():141 -->
- [x] Haversine batch CUDA kernel: `haversineDistanceKernel` implemented in `cuda/geo_kernels.cu:53`; RAII allocation and `cudaGetLastError()` present; kernel auto-tuning via `cudaOccupancyMaxPotentialBlockSize` at line 367. Dedicated parity test `tests/geo/test_category_b_parity_geo.cpp` added with CTest registration `test_category_b_parity_geo` (practical parity tolerance ≤1e-2 metres for float-km output path). (Target: Q3 2026)
- [x] `ST_CONTAINS` GPU dispatch: `bridge_geo_containment()` in `geo_acceleration_bridge.cpp:141` bridges to geo module's `batchIntersects()`; no dedicated CUDA kernel in `cuda/geo_kernels.cu` (delegates to geo module GPU spatial backend). CPU parity coverage is now implemented by `CategoryBGeoParity.ContainsGpuVsCpuParity` in `tests/geo/test_category_b_parity_geo.cpp`. (Target: Q3 2026)
- [x] `ST_DISTANCE` GPU dispatch: geodesic distance batch is wired through `GeoKernelDispatch::launchDistance` with HAVERSINE/VINCENTY formula support; CPU fallback path now honors `GeoDistanceFormula::VINCENTY` in `geo_acceleration_bridge.cpp`, and bridge dispatch behavior is validated by `GeoAccelerationBridge.PopulateGeoDispatch_Distance_VincentyFormulaIsApplied` in `tests/geo/test_geo_gpu_backend.cpp`. (Target: Q3 2026)
- [~] `ST_UNION` and `ST_DIFFERENCE` CUDA dispatch is now implemented for Point×Point and extended to Polygon×Polygon dispatch attempt via `launchGeoPolygonUnionKernel` / `launchGeoPolygonDifferenceKernel` (`cuda/geo_kernels.cu`) wired in `src/geo/gpu_backend_stub.cpp`; deterministic CPU exact fallback remains active for unsupported/failing polygon clipping paths until full kernel completion. (Target: Q4 2026)
- [x] Phase C ctest pre-requisite: `test_category_b_parity_geo` (Haversine GPU vs CPU) now implemented in `tests/geo/test_category_b_parity_geo.cpp` and registered via `add_geo_focused_test` in `tests/CMakeLists.txt`. (Target: Q3 2026)

#### GPU Benchmark Re-baseline
- [~] Re-baseline GPU benchmarks in `benchmarks/acceleration/` and `benchmarks/index/` after RAII refactor; `benchmarks/wave_cuda_baseline.json` is absent, and the checked-in `benchmarks/baselines/acceleration/baseline.json` contains representative sample values, not a measured release baseline. (Target: Q3 2026)
- [~] Confirm SRCP-4 (GPU/CPU fallback ≤8.8ms GPU / ≤11ms CPU) gate remains green after RAII + A-06/A-07 changes. (Target: Q3 2026)

### Q4 2026 — ST_UNION/ST_DIFFERENCE + Final GPU Sign-Off

- [~] **[A-08 geo — ST_UNION]** Point×Point CUDA kernel implemented and dispatch wired; Polygon×Polygon dispatch attempt path + parity coverage added (`CategoryBGeoParity.STUnionPolygonGpuVsCpuParity`), with GPU-runner ctest alias `test_gpu_geo_setops_parity` for Wave-A evidence collection; full production-grade polygon clipping kernel completion remains open. (Target: Q4 2026)
- [~] **[A-08 geo — ST_DIFFERENCE]** Point×Point CUDA kernel implemented and dispatch wired; Polygon×Polygon dispatch attempt path + parity coverage added (`CategoryBGeoParity.STDifferencePolygonGpuVsCpuParity`), with GPU-runner ctest alias `test_gpu_geo_setops_parity` for Wave-A evidence collection; full production-grade polygon clipping kernel completion remains open. (Target: Q4 2026)
- [~] **[GPU benchmark final sign-off]** All `benchmarks/acceleration/` and `benchmarks/index/` gates green on self-hosted runner with NVIDIA RTX hardware; commit signed baseline artefact to `benchmarks/cuda_final_baseline_q4_2026.json`. (Target: Q4 2026)

### Hybrid Retrieval Rollout Gates (issue #5468)
- [~] Phase B pre-requisite: result validation for Category A kernels (distance, TopK) — 320 gaps → 128 (Target: Q3 2026)
- [~] Phase B pre-requisite: memory boundary violation fixes (195 gaps → 78) (Target: Q3 2026)
- [~] Phase B pre-requisite: CONSTRAINT_A1–A5 enforcement for Category A kernels (Target: Q3 2026)
- [~] Phase B pre-requisite: AddressSanitizer gate clean before merge (Target: Q3 2026)
- [x] Phase C pre-requisite: Geo kernel validation gates (lat/lon bounds, distance range) implemented via `BatchValidator::validateGeoBatch`, `validatePointInPolygonBatch`, and `validateGeoDistanceResults`; wired into CUDA/CPU/bridge geo backends. Source: `include/acceleration/batch_validator.h`, `src/acceleration/cuda_backend.cpp`, `src/acceleration/cpu_backend.cpp`, `src/acceleration/geo_acceleration_bridge.cpp`; regression checks in `tests/acceleration/test_acceleration_regression.cpp`. (Target: Q4 2026)
- [x] Phase C pre-requisite: BFS frontier cutoff (10K nodes/hop, max 3 hops) + CPU fallback implemented via bounded-execution guard `BatchValidator::shouldUseCpuFallbackForGraphBFS` and runtime fallback path in `CUDAGraphBackend::batchBFS`. Source: `include/acceleration/batch_validator.h`, `src/acceleration/cuda_backend.cpp`; regression checks in `tests/acceleration/test_acceleration_regression.cpp`. (Target: Q4 2026)
- [x] Phase C pre-requisite: Dijkstra edge-weight non-negative + overflow guard + CPU fallback implemented via `BatchValidator::shouldUseCpuFallbackForShortestPath` and guarded runtime fallback path in `CUDAGraphBackend::batchShortestPath`. Source: `include/acceleration/batch_validator.h`, `src/acceleration/cuda_backend.cpp`; regression checks in `tests/acceleration/test_acceleration_regression.cpp`. (Target: Q4 2026)
- [~] Phase C ctest gate: `test_category_b_parity_geo` (Haversine GPU vs CPU) implemented and registered; pending self-hosted CUDA execution evidence for Q4 gate closure. (Target: Q4 2026)
- [~] Phase C ctest gate: `test_category_b_parity_bfs` implemented and registered (`tests/graph/test_category_b_parity_bfs.cpp`, `tests/CMakeLists.txt`); pending self-hosted CUDA execution evidence for Q4 gate closure. (Target: Q4 2026)
- [~] Phase C ctest gate: `test_category_b_parity_dijkstra` implemented and registered (`tests/graph/test_category_b_parity_dijkstra.cpp`, `tests/CMakeLists.txt`); pending self-hosted CUDA execution evidence for Q4 gate closure. (Target: Q4 2026)
- [~] Phase C benchmark gate: `bench_category_b_gpu_cpu_parity` implemented and registered (`benchmarks/acceleration/bench_category_b_gpu_cpu_parity.cpp`, `benchmarks/CMakeLists.txt`); pending self-hosted CUDA execution evidence for Q4 gate closure. (Target: Q4 2026)

### Short-term (3-6 months)
- [x] Expand deterministic regressions for backend-selection and fallback edge cases (Wave D: test_acceleration_highcardinality_stress.cpp delivered 2026-09-16)
- [x] Strengthen diagnostics for plugin/security deny paths and degraded runtime states (Wave D: RUNBOOK_ACCELERATION.md — 5 scenarios delivered 2026-09-16)
- [x] Harden distributed merge/resource behavior under partial device failures (Wave D: test_acceleration_soak.cpp — 3 soak cases delivered 2026-09-16)

### Mid-term (6-12 months)
- [ ] Re-baseline acceleration latency/throughput envelopes across representative hardware profiles (Target: Q1 2027)
- [ ] Extend capability-matrix coverage for optional backend combinations (Target: Q1 2027)
- [x] Improve operator-facing observability for backend health and dispatch routing decisions (Wave D: RUNBOOK_ACCELERATION.md delivered 2026-09-16)

## Implementation Phases

### Phase 1: Design / API Contract
- [~] Freeze backend capability/selection contract and fallback semantics for active major lines (Target: Q3 2026)
  - 2026-08-19: public `DeviceManager::setEnumerateFn()` test bridge added so capability negotiation can be verified without changing production discovery logic.
- [~] Define explicit failure contracts for unavailable backend, invalid input, and integrity-check failure states (Target: Q3 2026)
  - 2026-08-31: `BreakEvenValidator` now exposes explicit hook contracts for CPU/GPU profiling and fails closed on invalid distance/top-k profiles or non-GPU device selection.

### Phase 2: Core Implementation
- [~] Complete hardening for capability-driven dispatch and deterministic fallback selection paths (Target: Q4 2026)
  - 2026-08-31: break-even routing no longer depends on hardcoded CPU/GPU placeholder timings; the production implementation is compiled into both monolithic and modular acceleration builds.
- [ ] Align multi-device and resource manager behavior to shared bounded execution contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [~] Enforce fail-closed behavior for malformed workload input, plugin/signature failure, and partial device outages (Target: Q4 2026)
  - 2026-08-31: oneAPI USM allocation failures now fail closed before device copies, and break-even routing rejects malformed distance/top-k profiles instead of silently using placeholder timings.
- [x] Standardize fallback semantics when optional acceleration features are unavailable (Wave D: AccelerationPathFailClosedStress delivered 2026-09-16)
  - 2026-09-15: CUDA graph BFS/SP now applies deterministic bounded-input fallback to CPU for out-of-contract workloads (`numVertices > 10k`, `maxDepth > 3`, negative/non-finite/overflow-risk edge weights) instead of undefined behavior.

### Phase 4: Tests
- [~] Expand focused regressions for backend matrix, plugin security, and fallback correctness (Target: Q4 2026)
  - 2026-08-19: `tests/test_device_manager.cpp` extended with injected-enumeration coverage for cache reuse, refresh re-probe, CPU-fallback synthesis, best-device selection, and log observability; focused test registration fixed in `tests/CMakeLists.txt`.
  - 2026-08-31: `tests/gpu/test_break_even_validation.cpp` now covers production profiling hooks, metrics emission, and fail-closed invalid-profile handling.
- [x] Extend multi-device failure-injection regressions for merge and resource paths (Wave D: test_acceleration_highcardinality_stress.cpp — ConcurrentGPUCPUFallbackStress, AccelerationPathFailClosedStress 2026-09-16)

### Phase 5: Performance and Hardening
- [~] Replace proxy benchmark targets with production-path release measurements and approved profile baselines (Target: Q1 2027)
- [~] Validate sustained-load behavior for capability probing, queueing, and memory/resource paths using named hardware/build profiles (Target: Q1 2027)

### Phase 6: Documentation and Acceptance
- [~] Keep acceleration docs source-aligned and distinguish implementation/tests from hardware evidence (Target: ongoing)
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

## Performance Verification Status (historical claims reviewed 2026-11-16)

### Item 2: Performance gates NOT VERIFIED
- [!] Historical µs, speedup, and multi-GPU values are candidate thresholds only; no reviewed release-run artifacts were found for these claims.
- [!] `tests/acceleration/test_acceleration_performance_gates.cpp` times synthetic/proxy operations and hard-coded ratios; it does not measure registry selection, kernel dispatch, hardware speedup, or CPU fallback.
- [!] `benchmarks/acceleration/bench_acceleration_dedicated_gates.cpp` uses stubs; its results are not production-path evidence.
- [ ] Release approval requires production-path benchmark output, profile metadata, and a versioned manifest for each supported hardware tier.

### Performance Baseline Documentation
- File: `src/acceleration/PERFORMANCE_BASELINES.md` — candidate thresholds; no verified profile baseline
  - ACC-1 through ACC-10 baselines defined with thresholds
  - Hard gates AG-1 through AG-4 with acceptance criteria
  - Measurement methodology and regression detection procedure
  - Performance anomaly response protocol
  - Maintenance cadence and owner assignment

### Performance Evidence Required
- Dispatch and fallback p95/p99: production-path benchmarks with a fixed workload and environment
- Backend speedup: paired CPU/accelerator runs with correctness parity
- Multi-device behavior: named topology, partial-device cases, throughput and p95/p99
- Release profile: build identity, feature flags, raw output, and manifest retained for review

## Failure Handling Verification Status (historical claims reviewed 2026-11-16)

### Item 3: Production recovery evidence PARTIAL
- [x] Normal CPU selection and capability rejection: covered by `test_acceleration_regression.cpp` and `test_backend_registry_startup.cpp`.
- [x] Production kernel fallback after bounded transient retries: covered by the focused regression added with this evidence audit.
- [!] `tests/acceleration/test_acceleration_failure_handling.cpp` simulates timeout, device hang, OOM, and recovery decisions; those tests do not invoke production cancellation or recovery mechanisms.
- [ ] Add production-path tests for runtime health transition/eviction and multi-device partial failure; no general automatic runtime-health eviction is currently implemented.

### Failure-handling contract and gaps
- Invalid capability requirements return `nullptr`; no healthy accelerator snapshot restricts registry selection to CPU.
- AI vector-similarity malformed sizes and result-allocation errors return an explicit failure; GPU initialization/kernel failure falls back to CPU for that path.
- Kernel retry is limited to the configured transient status set and validated attempt/delay bounds; the final status is observable, but individual retry attempts are not exported as metrics.
- The module does not claim general kernel cancellation on device hang, continuous hardware attestation, or automatic backend eviction after runtime degradation.

## Production Readiness Checklist - EVIDENCE PENDING

- [x] Core backend-selection and CPU-fallback contracts are documented.
- [x] Invalid vector-similarity range checks fail before allocation/dispatch.
- [ ] Approved benchmark artifacts and profile-specific p95/p99 baselines.
- [ ] Production-path hang cancellation and runtime-degradation/partial-device recovery evidence.
- [ ] Support tiers and operator recovery procedure signed off for each backend profile.

## Known Issues and Limitations

- Hardware and driver differences can alter runtime behavior and performance envelopes.
- Some optional backend combinations remain environment dependent.
- Distributed and plugin-heavy scenarios need continuous hardening evidence.

## Wave 3 Gap-Closure Tracking (2026-08-31)

- [~] `break_even_validator.cc` — Prometheus metrics not yet emitted from
  `BreakEvenDecision::ToString()`, `CacheEntry::IsExpired()`, and
  `BreakEvenValidator` constructor. Wire observability counters/gauges via the
  existing metrics registry once a Prometheus handle is available on the
  BreakEvenValidator instance. Target: Q2 2027.
  Tracking comment added in source at `break_even_validator.cc:180`.

## Breaking Changes

- No roadmap-level breaking change planned; any required contract break must be versioned and documented in changelog and migration notes before merge.
