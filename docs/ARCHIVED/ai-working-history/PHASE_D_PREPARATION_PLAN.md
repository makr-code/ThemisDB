# Phase D — Optional GPU Acceleration: Preparation & Implementation Plan

**Status**: 🟡 **PLANNING** (2026-07-06)  
**Target Delivery**: Phase C gates → Phase D entry (Q1 2027)  
**Scope**: GPU acceleration module hardening, break-even validation, CUDA error handling

---

## 1. Context & Entry Requirements

### Phase D Goal
Enable bounded, optional GPU acceleration where break-even is **proven** and **maintained** throughout the operational lifetime. No unconditional GPU dependency. CPU-first fallback guaranteed on all paths.

### Entry Criteria (Gate from Phase C)
- [ ] Phase C distributed coordination complete (multi-shard exact routing, shard summary refresh, consensus gates)
- [ ] `sharding` thread-safety: 70% gap reduction (340+ → ~130 target) ✅ Required before Phase C
- [ ] `acceleration` Category B kernels: GPU/CPU parity tests **all passing**
- [ ] `gpu` CUDA error handling: **≥ 85% gap reduction** (340 unchecked calls → ≤ 51)

### Known Current State (as of 2026-07-06)
- **Phase A**: ✅ Complete (ArtifactManifest, ManifestStore, advisory-only policy, Prometheus metrics)
- **Phase B**: 🟡 In Progress (Q3 2026) — ANN + CPU validation, Category A kernels
- **Phase C**: 🟠 Planned (Q4 2026) — Multi-shard coordination, Category B kernels
- **Phase D**: 🔴 Not started — GPU acceleration hardening and parity validation

---

## 2. Phase D Deliverables Breakdown

### 2.1 CPU/GPU Break-Even Validation Framework

**Scope**: Establish deterministic, reproducible break-even benchmarking infrastructure.

**Components**:
- [ ] Break-even validation harness (CPU reference + GPU path profiling)
  - Latency threshold definition (per kernel: distance, TopK, BFS, Dijkstra, Geo containment)
  - Throughput parity baseline (messages/sec, queries/sec)
  - Cost model (GPU allocation overhead, data movement time, kernel execution)
- [ ] Benchmark suite for all Category A + B kernels
  - Fixed input sizes: 100, 1K, 10K, 100K, 1M vectors/nodes
  - Variable output sizes and selectivity
  - Synthetic worst-case adversarial inputs
- [ ] Prometheus metrics for continuous break-even monitoring
  - `gpu_acceleration_utilized` (boolean: GPU beneficial for this workload)
  - `gpu_acceleration_latency_ms` (histogram per kernel)
  - `gpu_acceleration_fallback_rate` (% GPU failures/timeouts)
  - `gpu_acceleration_break_even_ratio` (CPU time / GPU time, target ≥ 1.5x)
- [ ] Automated regression detection (alert if break-even ratio drops below 1.2x)

**Files to Create/Update**:
- `include/acceleration/break_even_validator.h` — interface
- `src/acceleration/break_even_validator.cc` — implementation
- `include/gpu/gpu_benchmark_suite.h` — benchmark harness
- `tests/gpu/test_break_even_validation.cpp` — tests (minimum 40+ cases)

**Acceptance Criteria**:
- All Category A kernels show ≥ 1.5x speedup on GPU vs. CPU (100K+ vector case)
- All Category B kernels (BFS, Dijkstra, Geo) show ≥ 1.3x speedup on GPU vs. CPU
- Benchmark suite runs in <30 min per target platform (NVIDIA RTX, AMD MI, Intel Arc)
- Zero stale data between CPU/GPU paths verified by deterministic seed tests

---

### 2.2 Full CUDA Error Handling Hardening (340 → ≤ 51)

**Scope**: Eliminate silent CUDA failures, resource leaks, and undefined behavior.

**Category Breakdown** (340 total unchecked CUDA calls):
1. **Kernel Launch Errors** (~85): Missing `cudaGetLastError()` + error check
2. **Memory Management** (~110): Unverified `cudaMalloc`, `cudaMemcpy`, `cudaFree`
3. **Device/Stream Management** (~65): Device selection, stream creation, synchronization
4. **Library Calls** (~50): cuDNN, cuBLAS, CUTLASS, cuSPARSE error codes ignored
5. **Async Operations** (~30): Kernel completion checks, stream ordering violations

**Hardening Approach**:
- Use `CudaRaii` wrapper (std::unique_ptr + RAII deletion)
- Wrap all CUDA calls with exception-safe macros
- Enforce synchronous error checking at layer boundaries
- Phase approach:
  - **Phase D1** (Weeks 1-2): Kernel Launch + Memory Management (~195 errors)
  - **Phase D2** (Weeks 3-4): Device/Stream + Library Calls (~115 errors)
  - **Phase D3** (Weeks 5-6): Async Operations + edge case hardening (~30 errors)

**Files to Create/Update**:
- `include/gpu/cuda_error_handling.h` — macro suite (CUDA_CHECK, CUDA_TRY, etc.)
- `include/gpu/cuda_raii.h` — RAII wrappers for GPU resources
- `src/gpu/cuda_error_hardening_*.cpp` — per-subsystem patches
- `tests/gpu/test_cuda_error_injection.cpp` — error injection harness (50+ cases)

**Acceptance Criteria**:
- All CUDA calls wrapped in verified error handlers
- Zero unchecked CUDA errors in production code paths
- ASAN/TSAN gate pass with GPU error injection
- Memory leak detection: 0 leaked CUDA allocations
- Dynamic exception testing with 100% coverage of error paths

---

### 2.3 Bounded GPU Refinement (Category A + B Kernels)

**Scope**: Implement safety guardrails for all GPU-accelerated kernels.

**Category A Kernels** (Distance, TopK):
- [ ] Distance kernels (L2, Cosine, InnerProduct)
  - Input validation: non-NaN vectors, valid dimensions
  - Output bounds checking: distances ∈ [0, ∞) or [-1, 1] by kernel
  - Precision tolerance: ε ≤ 1e-6 vs. CPU reference
- [ ] TopK selection
  - Input: bounded K ≤ min(1000, vector_count / 10)
  - Output: exactly K results, sorted order verified
  - Tie-breaking determinism vs. CPU version

**Category B Kernels** (Graph algorithms):
- [ ] Breadth-First Search (BFS)
  - Frontier size cap: max 10K nodes per hop, max 3 hops total
  - Cycle detection: no revisited nodes in output
  - Output completeness: reachable nodes ≤ frontier cap
- [ ] Dijkstra shortest path
  - Non-negative weight enforcement: all edge weights ≥ 0
  - Overflow prevention: accumulated cost ≤ FLT_MAX
  - Optimality: shortest path cost ≤ best CPU reference + 1e-6
- [ ] Geospatial distance/containment
  - WGS84 bounds enforcement: lat ∈ [-90, 90], lon ∈ [-180, 180]
  - Distance tolerance: computed ≤ true distance + 1e-6 m
  - Containment: boolean result matches CPU reference exactly

**Implementation**:
- [ ] Safe kernel wrapper layer (dispatch guards, pre/post validation)
- [ ] GPU/CPU parity test harness (deterministic seeds, cross-validation)
- [ ] Automated regression detection on parity metrics

**Files to Update**:
- `include/acceleration/safe_gpu_kernels.h` — guard definitions
- `src/acceleration/safe_gpu_kernels.cc` — wrapper implementations
- `tests/gpu/test_category_a_parity.cpp` — parity tests (40+ cases)
- `tests/gpu/test_category_b_parity.cpp` — parity tests (50+ cases)

**Acceptance Criteria**:
- All kernels pass parity tests against CPU reference (100% accuracy match)
- All input/output bounds enforced and tested
- Zero undetected overflow or precision issues
- Guard overhead measured: <5% latency increase vs. unguarded GPU

---

### 2.4 Resource Exhaustion Handling

**Scope**: Graceful degradation when GPU resources depleted (memory, compute, power).

**Scenarios to Handle**:
- [ ] GPU out-of-memory (cudaErrorMemoryAllocation)
  - Trigger: allocation fails
  - Recovery: fall back to CPU path, release reserved GPU memory
  - Metrics: `gpu_oom_fallback_count`, `gpu_max_utilization_percent`
- [ ] GPU compute saturation
  - Trigger: kernel execution timeout (default 5 sec)
  - Recovery: queue CPU task, release GPU context
  - Metrics: `gpu_compute_saturation_percent`
- [ ] GPU power/thermal throttling
  - Trigger: NVIDIA NVML reports throttling active
  - Recovery: shift workload to CPU, log thermal state
  - Metrics: `gpu_thermal_throttling_active`, `gpu_power_state`
- [ ] Lost GPU context (driver reset, process migration)
  - Trigger: cudaErrorDeviceUnloaded
  - Recovery: reinitialize GPU or fall back to CPU permanently
  - Metrics: `gpu_context_loss_count`, `gpu_reinit_attempts`

**Implementation**:
- [ ] Resource pool manager (preallocate + bound GPU memory)
- [ ] Timeout-safe async kernel dispatch
- [ ] NVML telemetry integration (thermal, power, clocks)
- [ ] Automatic CPU fallback orchestrator

**Files to Create/Update**:
- `include/gpu/gpu_resource_manager.h` — resource pooling
- `src/gpu/gpu_resource_manager.cc` — implementation
- `include/gpu/gpu_thermal_monitor.h` — NVML integration
- `tests/gpu/test_resource_exhaustion_injection.cpp` — injection tests (30+ cases)

**Acceptance Criteria**:
- All OOM scenarios recover to CPU without data loss
- Thermal throttling detected and logged
- GPU context loss recoverable or fails gracefully
- Metrics exportable to Prometheus with SLO bounds

---

### 2.5 Graceful CPU Fallback Paths

**Scope**: Ensure every GPU execution path has a functional, tested CPU fallback.

**Pattern**:
```
result = try_gpu_path() ? gpu_result : cpu_path_fallback()
```

**Requirements**:
- [ ] Every GPU kernel dispatch has corresponding CPU implementation
- [ ] Fallback is functionally identical (same algorithm or mathematically equivalent)
- [ ] Fallback is tested with same input suite as GPU path
- [ ] Fallback latency acceptable (≤ 10x GPU path latency, or <100ms)
- [ ] Error paths in GPU → CPU without deadlock or resource leak

**Implementation Strategy**:
- [ ] Dual-path dispatcher interface (GPU + CPU branches)
- [ ] Unified input validation (shared between paths)
- [ ] Automated fallback injection testing (GPU → CPU via flag)
- [ ] Latency tracking per path (CPU vs. GPU time histograms)

**Files to Create/Update**:
- `include/acceleration/dual_path_dispatch.h` — dispatcher
- `src/acceleration/dual_path_dispatch.cc` — implementation
- `tests/acceleration/test_gpu_cpu_fallback_parity.cpp` — fallback tests (50+ cases)

**Acceptance Criteria**:
- Zero correctness divergence between GPU + CPU paths (parity tests 100% pass)
- Fallback latency meets SLO (100ms max for ANN, 500ms max for graph)
- Fallback executed successfully in ≥99% of error injection cases
- All tests run with GPU disabled (CPU fallback only) and pass

---

### 2.6 HIP/ROCm Backend Parity (AMD Support)

**Scope**: Extend GPU support to AMD RDNA/MI series via HIP.

**Requirements**:
- [ ] HIP code generation from CUDA kernels (automated or manual C++ equivalents)
- [ ] Feature parity test matrix (same inputs, both CUDA + HIP backends)
- [ ] Performance parity: AMD GPU results ≥ 70% of NVIDIA performance
- [ ] Build system: CMake support for `HIPCC` + ROCm toolchain
- [ ] CI pipeline: Test on AMD GPU (MI210, MI300) if available

**Implementation Timeline**:
- **Phase D3** (Weeks 7-8): CUDA kernel → HIP translation, basic build support
- **Phase D4** (Weeks 9-10): Parity testing, performance tuning
- **Phase D5** (Weeks 11-12): Documentation, community packaging

**Files to Create/Update**:
- `include/gpu/hip_backend.h` — HIP API wrapper
- `src/gpu/hip_backend.cc` — implementation
- `src/gpu/kernels_hip.cu` — HIP kernel implementations
- `CMakeLists.txt` — HIP toolchain integration
- `tests/gpu/test_hip_parity.cpp` — HIP tests (30+ cases)

**Acceptance Criteria**:
- HIP build succeeds on ROCm-enabled system
- All parity tests pass (same results as CUDA backend)
- AMD MI210 performance ≥ 70% of RTX equivalent
- CI pipeline runs HIP tests on schedule or on-demand

---

## 3. Execution Roadmap

### Timeline (Post-Phase C Gate Entry: Q1 2027)

| Week | Deliverable | Owner | Blocks |
|------|-------------|-------|--------|
| **Phase D1** | Break-even validator framework | GPU team | D2-D6 |
| 1-2 | Kernel launch + memory error handling (195 gaps) | GPU team | |
| 3-4 | Device/stream + library error handling (115 gaps) | GPU team | D2 completion gate |
| 5-6 | Async operations + edge case hardening (30 gaps) | GPU team | Full error hardening gate |
| 7-8 | Category A kernel parity tests, safety guards | Accel team | D3 validation |
| 9-10 | Category B kernel parity tests, safety guards | Accel team | D3 validation gate |
| 11-12 | Resource exhaustion handling (OOM, thermal, context loss) | GPU team | |
| 13-14 | CPU fallback parity testing, dual-path dispatch | Accel team | D5 gate |
| 15-16 | HIP translation, build system integration | GPU team | |
| 17-18 | HIP parity testing, performance tuning | GPU team | D6 gate |
| 19-20 | Documentation, community packaging, final QA | Docs + GPU teams | Phase D Release |

**Critical Path**:
1. Break-even framework (foundation for all phases)
2. CUDA error handling (85% gap reduction gate)
3. Kernel parity tests (Category A + B validation)
4. Resource exhaustion + fallback (operational safety)
5. HIP backend (platform expansion)

---

## 4. Success Metrics & Validation Gates

### Gate 1: Break-Even Validator Operational
- [ ] Framework accepts workload config (kernel type, input size, output selectivity)
- [ ] CPU reference + GPU path profiled on 5 representative workloads
- [ ] Break-even ratio ≥ 1.5x for Category A, ≥ 1.3x for Category B
- [ ] Prometheus export functional

### Gate 2: CUDA Error Handling 85% Reduction
- [ ] All 340 unchecked CUDA calls audited
- [ ] ≤ 51 unchecked calls remain (85% reduction)
- [ ] ASAN/TSAN gate clean with error injection
- [ ] Zero known GPU memory leaks

### Gate 3: Kernel Parity Tests 100% Pass
- [ ] All Category A parity tests: 100% accuracy match (distance, TopK)
- [ ] All Category B parity tests: 100% accuracy match (BFS, Dijkstra, Geo)
- [ ] Deterministic seed correlation ≥ 99.99%
- [ ] Input/output bounds enforced and tested

### Gate 4: Resource Exhaustion Handling
- [ ] OOM recovery: GPU → CPU without data loss
- [ ] Thermal monitoring: alerts on throttling
- [ ] Context loss recovery: graceful degradation
- [ ] Fallback latency SLO met: 100ms ANN, 500ms graph

### Gate 5: CPU Fallback Parity 100% Coverage
- [ ] All GPU paths have CPU equivalents
- [ ] Fallback injection testing: 100% success rate
- [ ] Latency tracking: CPU/GPU histograms exported
- [ ] All tests pass with GPU disabled

### Gate 6: HIP Backend Feature Parity
- [ ] HIP build succeeds on ROCm system
- [ ] All HIP parity tests: 100% accuracy match
- [ ] AMD MI210 performance ≥ 70% NVIDIA RTX baseline
- [ ] CI pipeline: HIP tests automated

---

## 5. Risk Mitigation

| Risk | Impact | Mitigation |
|------|--------|-----------|
| **CUDA error handling too complex** | Schedule slip 3-4 weeks | Pre-allocate error handler patterns from Phase D1; consider compiler-based automated wrapping |
| **GPU/CPU parity hard to achieve** | D3 validation failure | Early prototype parity harness in Phase C; establish tolerance thresholds upfront |
| **AMD HIP translation effort underestimated** | HIP backend delayed to 2027 Q2 | Prioritize Feature Core kernels first; defer optional kernels to Phase D5 |
| **GPU resource exhaustion testing insufficient** | Production incidents | Deploy resource injection harness in Q4 2026 (Phase C); accumulate real-world failure modes |
| **Break-even assumptions invalidated** | Phase D rationale questioned | Establish break-even baseline in Phase C with realistic workload mix; update quarterly |

---

## 6. Deliverables Summary

### Code
- ✅ `include/acceleration/break_even_validator.h` + implementation
- ✅ `include/gpu/cuda_error_handling.h` — CUDA error macro suite
- ✅ `include/gpu/cuda_raii.h` — GPU resource RAII wrappers
- ✅ `include/acceleration/safe_gpu_kernels.h` — kernel safety guards
- ✅ `include/gpu/gpu_resource_manager.h` — resource pooling
- ✅ `include/acceleration/dual_path_dispatch.h` — dual-path dispatcher
- ✅ `include/gpu/hip_backend.h` — HIP API wrapper (if AMD GPU available)

### Tests
- ✅ 40+ break-even validation tests
- ✅ 50+ CUDA error injection tests
- ✅ 40+ Category A parity tests
- ✅ 50+ Category B parity tests
- ✅ 30+ resource exhaustion tests
- ✅ 50+ GPU/CPU fallback parity tests
- ✅ 30+ HIP parity tests (if applicable)

### Documentation
- ✅ `docs/PHASE_D_IMPLEMENTATION_GUIDE.md` — developer guide
- ✅ Doxygen API docs for all new headers
- ✅ Break-even benchmark results (per platform: NVIDIA RTX, AMD MI, Intel Arc)
- ✅ CUDA error handling patterns and guidelines
- ✅ GPU fallback decision tree (when GPU, when CPU)

### Metrics & Observability
- ✅ Prometheus metrics: break-even ratio, fallback rate, thermal state, context loss
- ✅ Latency histograms (CPU vs. GPU per kernel)
- ✅ Error injection telemetry
- ✅ Continuous regression detection alerts

---

## 7. Next Steps

### Immediate (Post-Phase C Gate)
1. **Validate Phase C entry criteria** — confirm all gates passed
2. **Establish GPU team** — assign D1-D6 task ownership
3. **Finalize break-even baseline** — agree on thresholds (1.5x Category A, 1.3x Category B)
4. **Begin Phase D1** — break-even validator framework + CUDA error cataloging

### Phase C → Phase D Transition Checklist
- [ ] Phase C distributed coordination complete
- [ ] Multi-shard exact routing tested under failure injection
- [ ] Category B kernel parity tests all passing
- [ ] Sharding thread-safety 70% gate verified
- [ ] Phase D plan reviewed and approved by team
- [ ] GPU team committed to Phase D delivery timeline

---

## 8. References

- **HYBRID_RETRIEVAL_ROLLOUT_PLAN.md** — Phase A-D overall strategy
- **TARGET_ARCHITECTURE.md** — four-layer hybrid architecture
- **KERNEL_CLASSIFICATION_REVIEW.md** — Category A/B/C kernel safety classification
- **PHASE_C_DELIVERABLES.md** — Phase C completion gates (referenced)
- **GPU module design** — `include/themis/gpu/`, `src/gpu/`

---

**Document Version**: 1.0  
**Last Updated**: 2026-07-06  
**Status**: 🟡 **PLANNING** — Awaiting Phase C gate entry and team commitment  
**Next Review**: Q4 2026 (Phase C completion)
