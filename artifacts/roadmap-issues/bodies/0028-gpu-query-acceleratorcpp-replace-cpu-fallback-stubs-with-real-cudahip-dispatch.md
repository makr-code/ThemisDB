### Context

This issue implements the roadmap item '`query_accelerator.cpp`: Replace CPU Fallback Stubs with Real CUDA/HIP' for the gpu domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.4.0.

Primary detail section: `query_accelerator.cpp`: Replace CPU Fallback Stubs with Real CUDA/HIP Dispatch

### Goal

Deliver the scoped changes for `query_accelerator.cpp`: Replace CPU Fallback Stubs with Real CUDA/HIP in src/gpu/ and complete the linked detail section in a release-ready state for v1.4.0.

### Detailed Scope

### `query_accelerator.cpp`: Replace CPU Fallback Stubs with Real CUDA/HIP Dispatch
**Priority:** High
**Target Version:** v1.4.0

`src/gpu/query_accelerator.cpp` has **5 GPU stubs** that fall through to sequential CPU implementations:
- Line 230: "GPU path stub: when `THEMIS_ENABLE_CUDA`/`THEMIS_ENABLE_HIP` is defined" — sort dispatch
- Line 277: "GPU stub: would copy IDs + keys to device, run Thrust `stable_sort_by_key`"
- Line 325: "GPU stub: would use `cub::DeviceReduce`"
- Line 383: "GPU stub: would use a parallel hash join kernel"
- Line 445: "GPU stub: would dispatch to `cublasSgemv` (FP32), `cublasHgemm` (FP16)"

All 5 stubs are guarded by `#ifdef THEMIS_ENABLE_CUDA` / `#ifdef THEMIS_ENABLE_HIP` but the guarded block is a stub comment, not real implementation.

**Implementation Notes:**
- `[ ]` **Sort (line 277)**: implement `#ifdef THEMIS_ENABLE_CUDA` block using `thrust::stable_sort_by_key` on device vectors; handle device memory alloc/free via `GpuMemoryManager`.
- `[ ]` **Reduce (line 325)**: implement using `cub::DeviceReduce::Sum`/`Max`/`Min`; allocate temp storage from `GpuMemoryPool`.
- `[ ]` **Hash join (line 383)**: implement a two-phase GPU hash join (build hash table on device, probe from device memory); reuse `memory_pool.cpp` for device allocation.
- `[ ]` **BLAS matrix-vector (line 445)**: dispatch `cublasSgemv` (FP32) or `cublasHgemm` (FP16) depending on `config_.precision`; handle cuBLAS handle lifecycle in `GpuModule`.
- `[ ]` Add `THEMIS_ENABLE_HIP` equivalents using `hipblas` / `rocThrust` / `hipcub`.
- `[ ]` Add CUDA/CPU parity tests for all 5 operations with input sizes 1 K, 100 K, 10 M.

**Performance Targets:**
- Sort 10 M int64 keys: ≥ 5× speedup vs. CPU `std::stable_sort` on RTX 3080.
- Hash join 2 × 1 M rows: ≥ 8× speedup vs. CPU nested-loop join.

---


**Priority:** High | **Target Version:** v1.1.0 | **Status:** ✅ Infrastructure implemented

Custom CUDA/ROCm kernels for specialised operations.

**Implemented infrastructure:**
- ✅ `GPUKernelValidator` — checksum/whitelist registry, validate-before-launch
- ✅ `GPULauncher` — typed async work-item / batch launcher with `BackendFn` hook;
  `timeout_ms` is now enforced via `std::async` + `wait_for`, with `timed_out`
  counter incremented on expiry
- ✅ `GPUStreamManager` — named async streams, CPU fallback budget enforcement;
  default backend registers a named HIP stream via `ROCmBackend::createStream()`
  (enabling future `synchronizeStream()` calls) and uses `ROCmBackend::createBackendFn()`
  as the work dispatcher; when `THEMIS_ENABLE_CUDA` is active a `cudaStream_t` is
  also created via `cudaStreamCreate()`; both handles are properly destroyed in
  `destroyStream()` and `~GPUStreamManager()`
  `createStream(nullptr)` now calls `ROCmBackend::createStream()` to own a real
  HIP stream for the stream's lifetime; `destroyStream()` calls
  `ROCmBackend::destroyStream()` for proper HIP stream cleanup; destructor
  tears down all ROCm-owned streams
- ✅ `ROCmBackend` — HIP stream lifecycle (`hipStreamCreate` / `hipStreamDestroy`
  / `hipStreamSynchronize`), device memory (`hipMalloc` / `hipFree` / `hipMemset`),
  and launcher `BackendFn` with CPU fallback when `THEMIS_ENABLE_HIP` is absent

**Remaining (hardware required):**
- Wire `cudaMalloc` into `GPUMemoryManager` (CUDA-only path)
- Plug kernel `.ptx` / `.hsaco` blobs into `GPULauncher::BackendFn`
- Activate `cudaMemset` / `hipMemset` zero-on-free in `GPUMemoryPool::release()`

---

### Acceptance Criteria

- [ ] Line 230: "GPU path stub: when `THEMIS_ENABLE_CUDA`/`THEMIS_ENABLE_HIP` is defined" — sort dispatch
- [ ] Line 277: "GPU stub: would copy IDs + keys to device, run Thrust `stable_sort_by_key`"
- [ ] Line 325: "GPU stub: would use `cub::DeviceReduce`"
- [ ] Line 383: "GPU stub: would use a parallel hash join kernel"
- [ ] Line 445: "GPU stub: would dispatch to `cublasSgemv` (FP32), `cublasHgemm` (FP16)"
- [ ] **Sort (line 277)**: implement `#ifdef THEMIS_ENABLE_CUDA` block using `thrust::stable_sort_by_key` on device vectors; handle device memory alloc/free via `GpuMemoryManager`.
- [ ] **Reduce (line 325)**: implement using `cub::DeviceReduce::Sum`/`Max`/`Min`; allocate temp storage from `GpuMemoryPool`.
- [ ] **Hash join (line 383)**: implement a two-phase GPU hash join (build hash table on device, probe from device memory); reuse `memory_pool.cpp` for device allocation.
- [ ] **BLAS matrix-vector (line 445)**: dispatch `cublasSgemv` (FP32) or `cublasHgemm` (FP16) depending on `config_.precision`; handle cuBLAS handle lifecycle in `GpuModule`.
- [ ] Add `THEMIS_ENABLE_HIP` equivalents using `hipblas` / `rocThrust` / `hipcub`.
- [ ] Add CUDA/CPU parity tests for all 5 operations with input sizes 1 K, 100 K, 10 M.
- [ ] Sort 10 M int64 keys: ≥ 5× speedup vs. CPU `std::stable_sort` on RTX 3080.
- [ ] Hash join 2 × 1 M rows: ≥ 8× speedup vs. CPU nested-loop join.
- [ ] ✅ `GPUKernelValidator` — checksum/whitelist registry, validate-before-launch
- [ ] ✅ `GPULauncher` — typed async work-item / batch launcher with `BackendFn` hook;
- [ ] ✅ `GPUStreamManager` — named async streams, CPU fallback budget enforcement;
- [ ] ✅ `ROCmBackend` — HIP stream lifecycle (`hipStreamCreate` / `hipStreamDestroy`
- [ ] Wire `cudaMalloc` into `GPUMemoryManager` (CUDA-only path)
- [ ] Plug kernel `.ptx` / `.hsaco` blobs into `GPULauncher::BackendFn`
- [ ] Activate `cudaMemset` / `hipMemset` zero-on-free in `GPUMemoryPool::release()`

### Relationships

- Roadmap row: #28 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/gpu/FUTURE_ENHANCEMENTS.md#query_acceleratorcpp-replace-cpu-fallback-stubs-with-real-cudahip-dispatch
- Source key: roadmap:28:gpu:v1.4.0:query-acceleratorcpp-replace-cpu-fallback-stubs-with-real-cudahip-dispatch

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:28:gpu:v1.4.0:query-acceleratorcpp-replace-cpu-fallback-stubs-with-real-cudahip-dispatch -->
<!-- roadmap-ref: row=28;module=gpu;target=v1.4.0 -->
<!-- roadmap-detail: src/gpu/FUTURE_ENHANCEMENTS.md#query_acceleratorcpp-replace-cpu-fallback-stubs-with-real-cudahip-dispatch -->
