# GPU Phase C Readiness Implementation Guide — Phases 2-4

**Document**: Implementation Reference for Query Accelerator, Memory Management, and Unified Memory Hardening  
**Date**: 2026-08-01  
**Target**: 50% reduction in unchecked CUDA calls by 2026-10-31

---

## Phase 2: Query Accelerator Hardening (In Progress)

### Status
⏳ **Running** (delegated to themisdb-implementer)

### Scope
File: `src/gpu/query_accelerator.cpp` (46 findings total)
- 8 CRITICAL (use_after_free_gpu)
- 21 HIGH (unchecked_cuda_call, others)
- 12 MEDIUM
- 5 LOW

### Key Refactoring Pattern

**Before** (raw pointers, manual cleanup, vulnerable to exceptions):
```cpp
float *d_a = nullptr;
const bool alloc_ok = cudaMalloc(&d_a, n * sizeof(float)) == cudaSuccess;
if (alloc_ok && cudaMemcpy(d_a, a.data(), ...) == cudaSuccess) {
  cublasSdot(blas.get(), n, d_a, 1, d_b, 1, &result);
  cudaFree(d_a);  // manual cleanup — leaks on exception
}
```

**After** (RAII, automatic cleanup, exception-safe):
```cpp
auto d_a = make_unique_gpu<float>(n);  // from gpu_memory.h
CHECKED_CUDA(cudaMemcpy(d_a.get(), a.data(), n * sizeof(float), cudaMemcpyHostToDevice));
cublasSdot(blas.get(), n, d_a.get(), 1, d_b.get(), 1, &result);
// d_a automatically freed on scope exit
```

### High-Priority Sections

| Lines | Precision | Kernel | Current Issues | Fix |
|-------|-----------|--------|-----------------|-----|
| 750-790 | FP32 | cublasSdot | use_after_free (lines 787-788) | unique_gpu_ptr<float> |
| 792-827 | FP16 | cublasGemmEx | use_after_free (lines 825-827) | unique_gpu_ptr<__half> |
| 829-867 | BF16 | cublasGemmEx | unchecked_cuda_call | CHECKED_CUDA() + unique_gpu_ptr |
| 900-950 | HIP paths | hipblasSdot / hipblasGemmEx | use_after_free (916-917) | CHECKED_HIP() + unique_gpu_ptr |
| 1050-1120 | Vector search | cuVS / hipVS | gpu_memory_leak (1100) | CHECKED_CUDA() + unique_gpu_ptr |

### Integration with Phase 1

Include files:
```cpp
#include <themis/gpu/gpu_error.h>      // CHECKED_CUDA, GPUErrorHandler
#include <themis/gpu/gpu_memory.h>     // unique_gpu_ptr, make_unique_gpu
#include <themis/gpu/gpu_timeout.h>    // KernelSLAGuard
```

Macro usage:
```cpp
// Wrap all CUDA calls
CHECKED_CUDA(cudaMemcpy(...));
CHECKED_CUDA(cudaMemset(...));

// Wrap HIP calls
CHECKED_HIP(hipMemcpy(...));

// Enforce 5-second SLA around hot kernels
{
  KernelSLAGuard sla_guard(std::chrono::seconds(5));
  cublasSdot(blas.get(), n, ...);
  if (sla_guard.checkTimeoutDeadline()) {
    result.used_gpu = false;  // fallback to CPU
  }
}
```

---

## Phase 3: Memory Management Hardening

### Status
⏭ **Queued** (will start after Phase 2)

### Scope
Files:
- `src/gpu/gpu_memory_manager_edition.cpp` (43 findings, primarily HIGH)
- `src/gpu/memory_pool.cpp` (9 findings)
- `tests/gpu/test_gpu_memory_management.cpp` (verification)

### Key Issues

| Category | Count | Pattern | Fix |
|----------|-------|---------|-----|
| resource_leaked_in_exception | 20+ | Allocation without try-catch cleanup | Add exception-safe guards |
| db_connection_leak | 55+ | Resource not released on error path | Ensure cleanup in finally block |
| uninitialized_access | 11+ | Container element access before init | Initialize in constructor |

### Refactoring Pattern

**Before** (manual resource cleanup, exception-vulnerable):
```cpp
bool GPUMemoryManager::allocateTenantQuota(const std::string& tenant_id, size_t bytes) {
  if (!policy_->canAllocate(tenant_id, bytes)) return false;
  
  void* ptr = nullptr;
  cudaError_t err = cudaMalloc(&ptr, bytes);  // no CHECKED_CUDA
  if (err != cudaSuccess) return false;
  
  // BUG: If assignment throws, ptr leaks
  quota_map_[tenant_id] = {ptr, bytes};
  policy_->onAllocate(tenant_id, bytes);
  return true;
}
```

**After** (exception-safe with RAII):
```cpp
bool GPUMemoryManager::allocateTenantQuota(const std::string& tenant_id, size_t bytes) {
  if (!policy_->canAllocate(tenant_id, bytes)) return false;
  
  try {
    auto gpu_alloc = make_unique_gpu<uint8_t>(bytes);  // RAII
    
    // Exception-safe: alloc succeeds atomically or throws
    quota_map_[tenant_id] = {std::move(gpu_alloc), bytes};
    policy_->onAllocate(tenant_id, bytes);
    
    return true;
  } catch (const std::exception& e) {
    SPDLOG_ERROR("Quota allocation failed for {}: {}", tenant_id, e.what());
    return false;
  }
  // gpu_alloc automatically freed on exception via RAII
}
```

### Acceptance Criteria

- [ ] All allocation paths wrapped with unique_gpu_ptr or CHECKED_CUDA()
- [ ] Try-catch-finally pattern around resource-sensitive operations
- [ ] Tenant quota map tracks allocations correctly
- [ ] test_gpu_memory_management passes (exception injection)
- [ ] Zero ASAN warnings on exception paths
- [ ] Rollback atomic: allocation succeeds fully or fails cleanly

---

## Phase 4: Unified Memory & ROCm Hardening

### Status
⏭ **Queued** (will start after Phase 3)

### Scope
Files:
- `src/gpu/unified_memory.cpp` (11 findings: 4 CRITICAL, 5 HIGH)
- `src/gpu/rocm_backend.cpp` (8 findings: all HIGH)
- `tests/gpu/test_gpu_unified_memory.cpp`
- `tests/gpu/test_gpu_rocm_backend.cpp`

### Key Issues

| File | Critical Issues | High Issues | Pattern |
|------|-----------------|-------------|---------|
| unified_memory.cpp | gpu_memory_leak (lines 13, 80, 161, 208) | unchecked HIP calls | hipMallocManaged without CHECKED_HIP |
| rocm_backend.cpp | — | unchecked hipMalloc, hipMemcpy, hipLaunch | Missing error checking |

### HIP-Specific Patterns

**CUDA pattern** (for reference):
```cpp
auto d_data = make_unique_gpu<float>(n);
CHECKED_CUDA(cudaMemcpy(d_data.get(), ...));
```

**HIP pattern** (mirror CUDA):
```cpp
auto d_data = make_unique_gpu<float>(n);  // same interface for both backends
CHECKED_HIP(hipMemcpy(d_data.get(), ...));  // CHECKED_HIP instead of CHECKED_CUDA
```

### Unified Memory Specifics

Unified memory (accessible from both host and device):
```cpp
float *ptr = nullptr;
CHECKED_HIP(hipMallocManaged(&ptr, bytes, hipMemAttachGlobal));
// ptr accessible from both host and device kernels
CHECKED_HIP(hipFree(ptr));  // unified deallocation
```

**With RAII**:
```cpp
auto ptr = make_unique_gpu<float>(bytes);  // same RAII as device memory
// Works for both cudaMalloc and hipMallocManaged
// Destructor calls appropriate cleanup based on backend
```

### Acceptance Criteria

- [ ] All hipMalloc / hipFree wrapped with CHECKED_HIP()
- [ ] Unified memory coherence verified (host/device access)
- [ ] HIP error codes properly mapped to GPUErrorClass
- [ ] test_gpu_rocm_backend passes (backend degradation scenarios)
- [ ] test_gpu_unified_memory passes (coherence + exception safety)
- [ ] Zero ASAN warnings
- [ ] ROCm 5.x compatibility verified

---

## Cross-Phase Verification Checklist

### Gap Reduction Audit

**Target**: 50% reduction in unchecked CUDA calls (340 → 170)

**Verification Script**:
```bash
#!/bin/bash
# Count unchecked CUDA/HIP calls (lines with cuda/hip call but no CHECKED_ wrapper)
CUDA_UNCHECKED=$(grep -r "cudaMalloc\|cudaMemcpy\|cudaFree\|cudaLaunch" \
  src/gpu/ include/gpu/ include/themis/gpu/ \
  | grep -v "CHECKED_CUDA\|CHECKED_HIP\|if.*cuda\|== cudaSuccess" | wc -l)

echo "Unchecked CUDA calls: $CUDA_UNCHECKED (target: ≤170)"
if [ $CUDA_UNCHECKED -le 170 ]; then
  echo "✅ Phase C prerequisite MET"
else
  echo "❌ Phase C prerequisite NOT MET"
fi
```

### Test Coverage Verification

Run full GPU test suite:
```bash
ctest --label gpu --timeout 120 --verbose
```

Expected results:
- `module_gpu_test_gpu_error_handling_focused`: ✅ PASS (15+ tests)
- `module_gpu_test_gpu_query_accelerator_focused`: ✅ PASS (GPU/CPU parity)
- `module_gpu_test_gpu_memory_management_focused`: ✅ PASS (exception injection)
- `module_gpu_test_gpu_unified_memory_focused`: ✅ PASS (coherence)
- `module_gpu_test_gpu_rocm_backend_focused`: ✅ PASS (backend degradation)

### Address Sanitizer & Memory Sanitizer

Run with ASAN enabled:
```bash
ASAN_OPTIONS=detect_leaks=1,halt_on_error=1 ctest --label gpu --timeout 120
```

Expected: Zero memory leaks, zero use-after-free, zero double-free

### Performance Baseline

Benchmark hot paths before and after:
```bash
./benchmarks/gpu/bench_gpu_query_accelerator
./benchmarks/gpu/bench_gpu_memory_manager_edition
```

Expected: Throughput within ±5% of baseline (RAII overhead negligible)

---

## Documentation Updates

### 1. include/themis/gpu/README.md

Add section:
```markdown
## Phase C Hardening Integration

All GPU module files have been refactored to use Phase C error handling infrastructure:

- **Error Taxonomy**: Each GPU subsystem maps failures to `GPUErrorClass` (quota, timeout, backend, memory, etc.)
- **RAII Memory**: Raw GPU pointers replaced with `unique_gpu_ptr<T>` and `shared_gpu_ptr<T>`
- **SLA Enforcement**: Hot kernels wrapped with `KernelSLAGuard` (5-second hard limit)
- **Error Macros**: All CUDA/HIP calls use `CHECKED_CUDA()` / `CHECKED_HIP()`

See also:
- [GPU Error Handling](gpu_error.h)
- [RAII Memory Wrappers](gpu_memory.h)
- [SLA Enforcement](gpu_timeout.h)
- [Implementation Plan](../../ai_working/gpu_phase_c_readiness_plan.md)
```

### 2. src/gpu/ROADMAP.md

Update status:
```markdown
## Phase C Pre-requisites Status

- [~] Fix 50% of unchecked CUDA calls (340 → 170) — Phase 1-4 hardening (Target: Q3 2026)
  - Phase 1: ✅ Error handling infrastructure
  - Phase 2: 🔄 Query accelerator (in progress)
  - Phase 3: ⏳ Memory management (queued)
  - Phase 4: ⏳ Unified memory & ROCm (queued)
  
- [~] Kernel SLA timeout enforcement (5-second hard limit) — Phase 1 ✅
- [~] RAII resource lifecycle violations (57 gaps) — Phases 2-4 (in progress)
```

---

## Risk Mitigation

### Risk: RAII Refactoring Breaks Existing Paths

**Mitigation**:
- Comprehensive parity tests (GPU/CPU result verification)
- ASAN/MSAN enabled in CI
- Phased rollout: query_accelerator → memory_manager → unified_memory
- Rollback plan: git revert by phase if critical regression detected

### Risk: Timeout SLA Too Aggressive

**Mitigation**:
- Baseline benchmarking before Phase 2 deployment
- Tunable SLA: default 5s, configurable per call
- Extensive 100ms/1s/5s test coverage in test_gpu_error_handling.cpp

### Risk: Hidden GPU Memory Leaks

**Mitigation**:
- ASAN/LSAN enabled in focused test targets
- GPU memory tracker in gpu_error.cpp logs allocation/deallocation pairs
- Quota map drift monitoring: verify used_bytes() == sum(all active allocations)

---

## Next Steps

1. ✅ **Phase 1**: Foundation complete (error handling, RAII, SLA)
2. 🔄 **Phase 2**: Query accelerator (currently running, ETA 2-3 hours)
3. ⏳ **Phase 3**: Memory management (start after Phase 2 validation)
4. ⏳ **Phase 4**: Unified memory & ROCm (start after Phase 3 validation)
5. ⏳ **Phase 5**: Integration & final verification

**Timeline**: 2026-08-01 to 2026-10-31 (Q3)

---

## References

- Plan: ai_working/gpu_phase_c_readiness_plan.md
- Phase 1 Report: ai_working/GPU_PHASE_C_PHASE1_IMPLEMENTATION_REPORT.md
- Roadmap: src/gpu/ROADMAP.md (Phase C prerequisites)
- Gaps: src/gpu/MODULE_GAPS.md (173 findings)
- Issue #5468: Hybrid Retrieval Rollout (gate tracking)
