# GPU Module - Future Enhancements

## Status Key

- ✅ **Infrastructure implemented** — CPU-level bookkeeping and API in place;
  ready to wire up real CUDA/ROCm calls.
- ⬜ **Blocked on hardware** — requires a CUDA/ROCm driver or device to complete.

---

## Features

### CUDA / ROCm Kernel Support
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

### GPU Query Acceleration
**Priority:** High | **Target Version:** v1.2.0 | **Status:** ✅ Infrastructure implemented

Accelerate database query operations using GPU.

**Implemented:**
- ✅ `GPUQueryAccelerator` — parallel scan with filter pushdown, sort (ASC/DESC),
  aggregate (SUM/COUNT/MIN/MAX/AVG), hash join
- ✅ CPU-path fallback for environments without GPU
- ✅ GPU-threshold dispatch: switches to GPU path above
  `Config::gpu_threshold_rows`
- ✅ FP16/BF16 Tensor Core dot-product (`PrecisionMode::FP16` / `::BF16`):
  inputs are round-tripped through half/bfloat16 encoding to simulate Tensor
  Core precision; on real hardware replaced by cuBLAS `cublasHgemm` (FP16) or
  `cublasGemmEx` with `CUDA_R_16BF` (BF16)
- ✅ Full unit-test coverage (`tests/test_gpu_query_accelerator.cpp`)

**Remaining (hardware required):**
- Replace CPU `std::stable_sort` with Thrust `stable_sort_by_key`
- Replace CPU reduction with `cub::DeviceReduce`
- Replace CPU hash join with a parallel GPU hash join kernel
- Replace sequential scan with `Thrust::copy_if` / `cub::DeviceSelect`

---

### Multi-GPU Support
**Priority:** Medium | **Target Version:** v1.3.0 | **Status:** ✅ Infrastructure implemented

Support for multiple GPUs and distributed computation.

**Implemented:**
- ✅ `GPULoadBalancer` — ROUND_ROBIN / LEAST_LOADED / FIRST_HEALTHY strategies,
  per-device VRAM tracking, `markDeviceFailed` / `resetDevice`
- ✅ `GPUDeviceDiscovery` — enumerate CUDA/ROCm devices, CPU-fallback sentinel,
  `GetBestDevice`, `GetHealthyDevices`
- ✅ `GPUClusterCoordinator` — multi-node cluster coordination with heartbeat-based
  health tracking, stale-node expiry, least-loaded node selection, and optional
  `ClusterConfig` block (STANDALONE / COORDINATOR / WORKER modes)

**Remaining (hardware required):**
- `cudaMemcpyPeer` / `hipMemcpyPeer` for GPU-to-GPU transfers
- ~~NVLink / XGMI topology detection~~ (implemented via `GPUClusterTopology`)

---

### GPU Memory Pooling
**Priority:** Medium | **Target Version:** v1.2.0 | **Status:** ✅ Infrastructure implemented

Efficient VRAM allocation with pooling.

**Implemented:**
- ✅ `GPUMemoryPool` — slab-based pre-allocator, `setZeroOnFree`, fragmentation
  tracking, pool stats, and `defragment()` routine (compacts occupied slabs,
  recalculates wasted bytes from per-slab `request_size`)
- ✅ `GPUMemoryManager` — pre-allocation hints (`ReserveHint` / `ConsumeHint`),
  tenant-aware quotas, peak tracking

**Remaining (hardware required):**
- Replace bookkeeping counters with real `cudaMalloc` / `hipMalloc` calls

---

### GPU Tensor Buffer
**Priority:** Medium | **Target Version:** v1.2.0 | **Status:** ✅ Infrastructure implemented

Typed, self-describing tensor containers for ML workloads.

**Implemented:**
- ✅ `GPUTensorBuffer` — shape/dtype, host-side backing store, fill, copy,
  named views, serialise / deserialise for checkpointing, global stats
- ✅ Full unit-test coverage (`tests/test_gpu_tensor.cpp`)

**Remaining (hardware required):**
- Add `device_ptr_` member populated by `cudaMalloc` / `hipMalloc`
- `uploadToDevice()` / `downloadFromDevice()` via `cudaMemcpy`

---

### GPU Training Loop
**Priority:** Medium | **Target Version:** v1.3.0 | **Status:** ✅ Infrastructure implemented

Training loop coordinator for GPU-backed ML workloads.

**Implemented:**
- ✅ `GPUTrainingLoop` — batch iteration, loss tracking, early stopping,
  checkpoint callbacks, per-epoch statistics
- ✅ Full unit-test coverage (`tests/test_gpu_training_loop.cpp`)

**Remaining (hardware required):**
- Wire a real CUDA/ROCm forward+backward pass into the `LossFn` callback

---

### CUDA Graph Capture for Recurring Query Execution Patterns
**Priority:** High | **Target Version:** v1.4.0 | **Status:** ✅ Infrastructure implemented

Eliminates repeated kernel-launch overhead for queries that share the same
execution shape (operation type, row count, parameter profile) by capturing
the kernel sequence once and replaying it on subsequent calls.

**Implemented infrastructure:**
- ✅ `GPUGraphCache` (`include/themis/gpu/graph_cache.h`, `src/gpu/graph_cache.cpp`)
  — LRU-bounded cache (max 32 entries) keyed on `QueryShape`
  (`OpType` × `row_count` × `param_hash`).  Tracks `capture_count`,
  `replay_count`, and `last_access` for each entry.
- ✅ `QueryShape` + `QueryShapeHash` — FNV-1a–based identity and hash for
  recurring query patterns.
- ✅ `GPUQueryAccelerator` integration — all four operations (`scan`, `sort`,
  `aggregate`, `hashJoin`) check the graph cache when
  `Config::enable_graph_cache = true`.  Cache hit/miss counters visible in
  `GPUQueryAccelerator::Stats::graph_cache_hits` /
  `graph_cache_misses`.
- ✅ Runtime enable/disable via `enableGraphCache()` / `disableGraphCache()`.
- ✅ `getGraphCacheStats()` exposes hit/miss/eviction counters.
- ✅ Full unit-test coverage (`tests/test_gpu_graph_cache.cpp`)

**Remaining (hardware required):**
- Populate `GraphEntry::graph` / `GraphEntry::exec` with real `cudaGraph_t` /
  `cudaGraphExec_t` handles when `THEMIS_ENABLE_CUDA` is defined.
- Replace the CPU-simulation `capture()` body with
  `cudaStreamBeginCapture` → kernel launches → `cudaStreamEndCapture` →
  `cudaGraphInstantiate`.
- Replace the CPU `lookup()` replay path with `cudaGraphLaunch` on the main
  stream, then `cudaMemcpy` to copy results back.

---

## See Also

- [README.md](README.md) — Current module documentation
- [../../docs/gpu_roadmap.md](../../docs/gpu_roadmap.md) — Production-readiness
  assessment and full roadmap

---

*Last Updated: February 2026*  
*Module Version: v1.2.0*
