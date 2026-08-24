# GPU Module Phase C Readiness Remediation Plan

**Date**: 2026-08-01  
**Target**: Q3 2026 (August - October)  
**Status**: Planning  
**Owner**: GPU Module Team  

---

## Executive Summary

The GPU module requires hardening to achieve Phase C readiness (Hybrid Retrieval Rollout: bounded GPU refinement phase). Three critical prerequisites must be addressed by end of Q3 2026:

1. **Fix 50% of unchecked CUDA calls** (340 → 170) — CRITICAL
2. **Kernel SLA timeout enforcement** (5-second hard limit)
3. **RAII resource lifecycle violations** (57 gaps)

Current status per MODULE_GAPS.md (generated 2026-06-04):
- Total findings: 173
- Critical findings: 16
- High severity: 117
- **Actionable (C+H): 133 findings across 25 files**

---

## Impact Analysis

### Highest-Impact Files (by severity and count)

| File | Total | Critical | High | Key Issues |
|------|-------|----------|------|-----------|
| query_accelerator.cpp | 46 | 8 | 21 | use_after_free_gpu (8x), gpu_memory_leak (1x), unchecked_cuda_call (11x) |
| gpu_memory_manager_edition.cpp | 43 | 0 | 42 | resource_leaked_in_exception, allocation lifecycle |
| unified_memory.cpp | 11 | 4 | 5 | gpu_memory_leak (2x), unchecked_cuda_call (1x), use_after_free_gpu (1x) |
| memory_pool.cpp | 9 | 1 | 8 | resource management, exception safety |
| rocm_backend.cpp | 8 | 0 | 8 | HIP-specific error handling |
| time_slice_scheduler.cpp | 8 | 3 | 5 | concurrent access, exception safety |

### Category Breakdown

| Category | Count | Target Reduction | Approach |
|----------|-------|------------------|----------|
| **unchecked_cuda_call** | 18 | → ≤9 (50%) | Add CHECKED_CUDA macro; wrap all cuda{Malloc,Memcpy,Free,Launch} calls |
| **resource_leaked_in_exception** | 8 | → ≤4 (50%) | Introduce RAII wrappers; use try-catch-finally patterns |
| **use_after_free_gpu** | 8 | → ≤4 (50%) | Use shared_ptr/unique_ptr for GPU memory; defer cleanup until scope end |
| **gpu_memory_leak** | 4 | → ≤2 (50%) | RAII allocation tracking; guard exception paths |
| **uninitialized_access** | 11 | → ≤5 (50%) | Initialization guards in constructors |
| **data_race** | 3 | → ≤1 (50%) | Add synchronization (mutex, atomic) where needed |

---

## Work Breakdown

### Phase 1: Foundational Error Handling (Week 1-2)

**Goal**: Establish error taxonomy and injection points for Phase C gates.

**Deliverables**:
- [ ] Define GPU error taxonomy (quota, degradation, fallback classes) in include/themis/gpu/gpu_error.h
- [ ] Implement CHECKED_CUDA() / CHECKED_HIP() macros with configurable error policies
- [ ] Create GPU timeout enforcement: kernel SLA (5s hard limit) in query_accelerator.h
- [ ] RAII GPU memory wrapper (unique_gpu_ptr) in include/themis/gpu/gpu_memory.h
- [ ] Test infrastructure for timeout injection

**Files to Create/Modify**:
- include/themis/gpu/gpu_error.h (new)
- include/themis/gpu/gpu_memory.h (new GPU RAII wrapper)
- include/themis/gpu/gpu_timeout.h (new SLA enforcement)
- src/gpu/gpu_error.cpp (error taxonomy impl)

**Estimated LOC**: 200-250

---

### Phase 2: Query Accelerator Hardening (Week 2-3)

**Goal**: Fix 8 CRITICAL + 21 HIGH findings in query_accelerator.cpp

**Priority Issues** (from MODULE_GAPS.md):
- Lines 787, 788, 825-827, 916-917: use_after_free_gpu → Replace with unique_gpu_ptr
- Line 1100: gpu_memory_leak on failure path → Guard with RAII
- Lines 763, 766, 777, 791-796, 817, 829-834, 854: unchecked_cuda_call → Add CHECKED_CUDA()

**Approach**:
1. Replace raw `float *d_a = nullptr; cudaMalloc(&d_a, ...);` with `auto d_a = make_unique_gpu<float>(n);`
2. Wrap all `cudaMalloc()`, `cudaMemcpy()`, `cudaFree()` with CHECKED_CUDA()
3. Add deterministic fallback to CPU on GPU errors (already in place; ensure SLA timeout triggers)
4. Update tests to verify GPU/CPU result parity within tolerance (< 1e-3 relative error)

**Files to Modify**:
- src/gpu/query_accelerator.cpp (main hardening)
- tests/gpu/test_gpu_query_accelerator.cpp (add error injection tests)
- tests/gpu/test_gpu_query_accelerator_parity.cpp (add SLA timeout verification)

**Estimated LOC**: 150-200 modifications

---

### Phase 3: Memory Management Hardening (Week 3-4)

**Goal**: Fix 42 HIGH findings in gpu_memory_manager_edition.cpp + 8 in memory_pool.cpp

**Priority Issues**:
- resource_leaked_in_exception (Lines 33, 279, ...): Add try-catch-finally cleanup
- Allocation lifecycle tracking: Ensure all allocations have matched deallocations

**Approach**:
1. Audit all allocation paths in GPUMemoryManager
2. Verify tenant quota tracking is exception-safe
3. Add RAII guards around pool expansion and policy decisions
4. Implement allocation rollback on failure

**Files to Modify**:
- src/gpu/gpu_memory_manager_edition.cpp (exception safety)
- src/gpu/memory_pool.cpp (RAII lifecycle)
- tests/gpu/test_gpu_memory_management.cpp (exception injection tests)

**Estimated LOC**: 100-150 modifications

---

### Phase 4: Unified Memory & ROCm Hardening (Week 4)

**Goal**: Fix 4 CRITICAL + 5 HIGH findings in unified_memory.cpp; 8 HIGH in rocm_backend.cpp

**Priority Issues**:
- unified_memory.cpp lines 13, 80, 161, 208: GPU memory leaks, unchecked calls
- rocm_backend.cpp: HIP error checking consistency

**Approach**:
1. Apply RAII and CHECKED_HIP() macros to all HIP calls
2. Add HIP-specific timeout enforcement (mirror CUDA SLA)
3. Verify unified memory coherence under mixed allocation modes

**Files to Modify**:
- src/gpu/unified_memory.cpp (HIP RAII, error checking)
- src/gpu/rocm_backend.cpp (HIP error taxonomy)
- tests/gpu/test_gpu_unified_memory.cpp
- tests/gpu/test_gpu_rocm_backend.cpp

**Estimated LOC**: 80-120 modifications

---

### Phase 5: Integration & Test Verification (Week 4-5)

**Goal**: Validate all error handling paths; verify Phase C gates.

**Deliverables**:
- [ ] Pass: test_gpu_query_accelerator_parity (CPU/GPU result parity)
- [ ] Pass: test_gpu_memory_management (exception injection)
- [ ] Pass: test_gpu_error_handling (new focused test for SLA + fallback)
- [ ] Benchmark: bench_gpu_query_accelerator (confirm hot path performance)
- [ ] Gap reduction audit: Verify 50% reduction in unchecked calls

**Estimated Test LOC**: 200-250 new test cases

---

## Acceptance Criteria

### Phase C Pre-requisites ✅

- [ ] **50% CUDA call reduction**: 340 → ≤170 unchecked calls
  - Verification: Count `cudaMalloc`, `cudaMemcpy`, `cudaFree`, `cudaLaunch*` without CHECKED_CUDA() wrapper
  - Acceptable: All critical paths protected; non-critical paths documented as optional GPU

- [ ] **Kernel SLA timeout enforcement** (5-second hard limit)
  - Verification: test_gpu_query_accelerator runs kernel with explicit 5s timeout
  - Acceptable: Timeout triggers fallback to CPU; result still deterministic

- [ ] **57 RAII resource lifecycle violations resolved**
  - Verification: Use unique_gpu_ptr/shared_gpu_ptr for all GPU memory; no naked pointers in public APIs
  - Acceptable: All exception paths verified with ASAN/Valgrind

### Build & Test

- [ ] All GPU-focused tests pass: `ctest --label gpu --timeout 120`
- [ ] No new Clang-Tidy / Address Sanitizer / Memory Sanitizer warnings in GPU module
- [ ] Benchmark results: query_accelerator within baseline tolerance (±5% throughput)

---

## Technical Approach

### Error Handling Strategy

**Error Taxonomy** (include/themis/gpu/gpu_error.h):
```
enum class GPUErrorClass {
  kQuotaExceeded,         // VRAM budget denial
  kKernelTimeout,         // SLA violation
  kBackendUnavailable,    // Device offline / driver error
  kMemoryCommunication,   // H2D / D2H failure
  kNumerical,             // Precision loss, NaN detection
  kUnsupportedOperation,  // Kernel not available for config
};
```

**Error Recovery** (per class):
- kQuotaExceeded → degrade to CPU immediately
- kKernelTimeout → degrade to CPU immediately; emit diagnostic
- kBackendUnavailable → degrade to CPU + mark device unavailable
- kMemoryCommunication → retry once; degrade to CPU
- kNumerical → return NaN; emit warning
- kUnsupportedOperation → degrade to CPU

### RAII Memory Wrapper

**Design** (include/themis/gpu/gpu_memory.h):
```cpp
template <typename T>
class unique_gpu_ptr {
  T* ptr = nullptr;
  ~unique_gpu_ptr() { if (ptr) { CHECKED_CUDA(cudaFree(ptr)); } }
  // move semantics, no copy
};

template <typename T>
unique_gpu_ptr<T> make_unique_gpu(size_t count) {
  T* ptr = nullptr;
  CHECKED_CUDA(cudaMalloc(&ptr, count * sizeof(T)));
  return unique_gpu_ptr<T>(ptr);
}
```

### Timeout Enforcement

**Design** (include/themis/gpu/gpu_timeout.h):
```cpp
class KernelSLAGuard {
  std::chrono::steady_clock::time_point deadline_;
  bool checkTimeoutDeadline() const { 
    return std::chrono::steady_clock::now() >= deadline_;
  }
};

// Usage in query_accelerator:
CHECKED_CUDA(cudaEventRecord(start, stream));
KernelSLAGuard guard(5s);  // 5-second SLA
launchKernel<<<grid, block, 0, stream>>>(args);  // kernel runs
CHECKED_CUDA(cudaEventRecord(end, stream));
if (guard.checkTimeoutDeadline()) { /* fallback to CPU */ }
```

---

## Risk Assessment

### Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| RAII refactoring breaks existing GPU paths | Medium | High | Comprehensive parity tests; phased rollout |
| Timeout SLA too aggressive; causes false fallback | Medium | Medium | Tunable timeout; baseline benchmarking |
| Exception safety regression | Low | High | ASAN / Memory Sanitizer in CI; focused unit tests |

### Schedule Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Scope creep beyond Phase C prerequisites | Medium | Medium | Strict checklist; defer Phase D work |
| Dependent on hardware availability for testing | Low | High | Mock/simulation GPU tests; CI GPU pool reservation |

---

## Success Metrics

1. **Coverage**: ≥50% reduction in unchecked CUDA calls (18 → ≤9 in query_accelerator, etc.)
2. **Reliability**: Zero GPU memory leaks in ASAN-enabled test runs
3. **Performance**: Query accelerator throughput within ±5% of baseline
4. **Timeliness**: Completion by 2026-10-31 (end of Q3)
5. **Documentation**: Error taxonomy + usage guide in include/themis/gpu/README.md

---

## Next Steps (This Session)

1. ✅ Create plan (this document)
2. ⏭ Implement foundational error handling (Phase 1)
3. ⏭ Delegate query_accelerator hardening (Phase 2) to themisdb-implementer
4. ⏭ Integrate memory management fixes (Phase 3)
5. ⏭ Test & verify Phase C gates

---

## References

- Issue #5468: Hybrid Retrieval Rollout (Gate tracking)
- Issue #5385: Unified GPU memory hierarchy (IVRAMPolicy)
- src/gpu/ROADMAP.md: Phase C/D prerequisites
- src/gpu/MODULE_GAPS.md: Detailed findings (173 total)
- docs/ci-cd: GPU CI pipeline status
