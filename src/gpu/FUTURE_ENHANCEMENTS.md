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

### GPU-Accelerated ANN (Vector Similarity) via cuVS/RAFT
**Priority:** High | **Target Version:** v1.5.0 | **Status:** ✅ Infrastructure implemented

Approximate k-nearest-neighbor (ANN) vector similarity search accelerated by
the [cuVS/RAFT](https://github.com/rapidsai/cuvs) library on NVIDIA GPUs.

**Implemented infrastructure:**
- ✅ `GPUQueryAccelerator::annSearch()` — accepts a flat query array and a flat
  database array, returns the k nearest neighbors per query sorted ascending by
  distance.  Supports L2 (squared Euclidean) and inner-product distance metrics.
- ✅ CPU brute-force exact k-NN fallback (max-heap per query) — always available
  without GPU hardware; activated when the database size is below
  `Config::gpu_threshold_rows` or `force_cpu = true`.
- ✅ Graph-cache integration — recurring ANN queries with the same shape
  (`numQueries × dim`, `k`, metric) are tracked in `GPUGraphCache` with
  `QueryShape::OpType::ANN_SEARCH`; hit/miss counters visible in
  `GPUQueryAccelerator::Stats::graph_cache_hits` / `graph_cache_misses`.
- ✅ `Stats::total_ann_searches` counter for observability.
- ✅ Full unit-test coverage (`tests/test_gpu_query_accelerator.cpp`)

**Remaining (hardware required):**
- Allocate device buffers via `cudaMalloc` and copy database + queries with
  `cudaMemcpy`.
- Build an IVF-Flat index using
  `cuvs::neighbors::ivf_flat::build(handle, idx_params, db_view)`.
- Execute the search with
  `cuvs::neighbors::ivf_flat::search(handle, search_params, index, q_view,
  neighbors_view, distances_view)`.
- Copy `neighbors` and `distances` device arrays back to host and populate
  `AnnResult`.
- Wire `THEMIS_ENABLE_CUDA` guard around the cuVS path; fall back to CPU on
  `cudaMalloc` failure.

---

---

### Unified Memory Support (CPU+GPU Shared Address Space)
**Priority:** High | **Target Version:** v1.5.0 | **Status:** ✅ Infrastructure implemented

Unified memory allocates a single managed address space accessible by both the
CPU and any configured CUDA or HIP device.  The CUDA/HIP runtime automatically
migrates pages between CPU DRAM and GPU VRAM as they are accessed, eliminating
explicit `cudaMemcpy` transfers for workloads that share data between CPU and GPU.

**Implemented infrastructure:**
- ✅ `GPUUnifiedMemoryAllocator` (`include/themis/gpu/unified_memory.h`,
  `src/gpu/unified_memory.cpp`) — `allocate`, `free`, `prefetch`, `advise`,
  `isSupported`, `getStats`, `getActiveAllocations`, `getTenantBytes`, `reset`.
- ✅ CUDA path: `cudaMallocManaged` / `cudaFree` / `cudaMemPrefetchAsync` /
  `cudaMemAdvise` — gated on `THEMIS_ENABLE_CUDA`.
- ✅ HIP path: `hipMallocManaged` / `hipFree` / `hipMemPrefetchAsync` /
  `hipMemAdvise` — gated on `THEMIS_ENABLE_HIP`.
- ✅ CPU fallback: `malloc` / `free`; `prefetch` and `advise` are no-ops that
  return `true`; `isSupported()` returns `false`.
- ✅ `MemAdvice` enum mirrors `cudaMemoryAdvise` / `hipMemoryAdvice`: six hints
  (`SET_PREFERRED_LOCATION`, `SET_ACCESSED_BY`, `SET_READ_MOSTLY`, and their
  `UNSET_*` counterparts).
- ✅ Per-tenant byte tracking — each allocation may carry an optional
  `tenant_id`; `getTenantBytes(tenant_id)` returns current live usage.
- ✅ `Stats` struct: `total_allocations`, `total_frees`, `allocated_bytes`,
  `peak_bytes`, `prefetch_calls`, `advise_calls`, `hardware_unified`.
- ✅ Thread-safe: all public methods protected by an internal `std::mutex`.
- ✅ Full unit-test coverage (`tests/test_gpu_unified_memory.cpp`, 24 tests).

**Remaining (hardware required):**
- Verify hardware page-migration with a real `cudaMallocManaged` allocation on
  an NVIDIA Volta/Ampere GPU: page-fault latency must be < 5 ms for a 256 MB
  buffer that is first written on the CPU and then read on device via a simple
  CUDA kernel; measured with CUDA events.
- Benchmark unified memory throughput vs. explicit `cudaMemcpy` for ThemisDB
  batch sizes: unified memory must achieve ≥ 0.75× the throughput of explicit
  `cudaMemcpy` for 1M float32 vectors (4 MB) on an RTX-class GPU; measured in
  GB/s using CUDA events averaged over 100 iterations.
- Consider wrapping `GPUUnifiedMemoryAllocator::allocate` into an
  RAII helper `UnifiedBuffer<T>` analogous to `make_cuda_unique<T>` in
  `include/utils/memory_utils.h`.

---

### Dynamic GPU Time-Slicing for Multi-Tenant Isolation
**Priority:** High | **Target Version:** v1.5.0 | **Status:** ✅ Infrastructure implemented

Prevents any single tenant from monopolizing the GPU by assigning each
tenant a configurable time quantum and dispatching work in round-robin order.

**Implemented infrastructure:**
- ✅ `GPUTimeSliceScheduler` (`include/themis/gpu/time_slice_scheduler.h`,
  `src/gpu/time_slice_scheduler.cpp`) — round-robin time-sliced dispatcher.
  - `registerTenant(TenantConfig)` / `unregisterTenant(tenant_id)` — tenant lifecycle.
  - `submit(tenant_id, WorkItem)` — enqueue work for a tenant's FIFO queue.
  - `dispatch(backend)` — one scheduling round: visit each tenant in
    registration order; execute items until the slice (`slice_ms`) expires,
    then move to the next tenant.  Remaining items are deferred to the next
    `dispatch()` call; `preempted` counter incremented when the slice expires
    with items still in the queue.
  - `drainAll(backend)` — calls `dispatch()` until all queues are empty;
    safe for batch workflows and tests.
  - `allQueuesEmpty()` — predicate for scheduler idle detection.
  - `getTenantStats(tenant_id)` / `getAllTenantStats()` / `getStats()` —
    per-tenant and aggregate observability (`submitted`, `completed`,
    `preempted`, `total_elapsed_ms`, `queue_depth`, `slice_ms`).
  - `resetStats()` — clear counters and queues, keeps tenant registrations.
- ✅ CPU no-op backend used automatically when `dispatch(nullptr)` is called.
- ✅ Thread-safe: all public methods protected by an internal `std::mutex`.
- ✅ Full unit-test coverage (`tests/test_gpu_time_slice_scheduler.cpp`).

**Remaining (hardware required):**
- Wire a real CUDA/ROCm stream into the `dispatch()` `BackendFn` so items
  are submitted to `cudaStream_t` / `hipStream_t` rather than a CPU callback.
- Implement hardware-level preemption (CUDA MPS context switching) for
  true sub-kernel preemption within a running CUDA kernel.

---

### WASM-based GPU Kernel Sandbox for Untrusted Third-Party Kernels
**Priority:** High | **Target Version:** v1.6.0 | **Status:** ✅ Infrastructure implemented

Provides an isolated execution environment for GPU kernel blobs submitted by
untrusted third parties.  Two enforcement layers prevent unauthorized or
tampered code from reaching the GPU:

1. **Whitelist + checksum gate** — delegated to `GPUKernelValidator`; only
   registered kernel IDs with matching FNV-1a checksums are admitted.
2. **Sandbox execution** — memory ceiling and wall-clock timeout enforced
   before the kernel blob reaches the GPU backend.

**Implemented infrastructure:**
- ✅ `WASMKernelSandbox` (`include/themis/gpu/wasm_kernel_sandbox.h`,
  `src/gpu/wasm_kernel_sandbox.cpp`) — feature-gated sandbox with
  `SandboxConfig` (memory limit, timeout, host-call toggle), `ExecutionResult`,
  `Status` enum (8 values), and `Stats`.
- ✅ `execute(kernel_id, blob, backend)` — full validation pipeline:
  feature-gate → empty-blob check → memory-limit check →
  `GPUKernelValidator` whitelist/checksum → sandboxed CPU execution with
  optional timeout via `std::async` + `wait_for`.
- ✅ `isWASMSupported()` — returns `true` when `THEMIS_ENABLE_WASM` is
  defined; always `false` in the current CPU simulation build.
- ✅ `WASM_SANDBOX` feature flag added to `GPUFeatureFlags::Feature` and
  `GPUFeatureFlags::getAll()`; enabled by default for ENTERPRISE and
  HYPERSCALER editions only.
- ✅ `sandboxStatusName()` free function for human-readable status strings.
- ✅ Thread-safe: all public methods protected by an internal `std::mutex`.
- ✅ Full unit-test coverage (`tests/test_gpu_wasm_kernel_sandbox.cpp`):
  feature-gate, empty blob, whitelist, checksum mismatch, memory limit,
  timeout, custom backend, stats, concurrent safety.

**Remaining (WASM runtime required):**
- Add `wasm_plugin_loader.cpp` alongside `wasm_kernel_sandbox.cpp`; select
  loader via `SandboxConfig::runtime` field (`"cpu"` | `"wasmtime"` | `"wasmedge"`).
- Replace the `runInSandbox` CPU-simulation path with Wasmtime / WasmEdge
  WASM module instantiation gated on `THEMIS_ENABLE_WASM`.
- Enforce linear-memory hard ceiling at the WASM runtime level
  (`wasmtime_store_limiter` / `WasmEdge_ConfigureCompilerSetMemoryImportExportPolicy`).
- Wire `SandboxConfig::allow_host_calls` to the WASM import resolution
  callback so that only explicitly allowlisted host functions are importable.
- Add SHA-256 or BLAKE3 hash verification in addition to FNV-1a for
  cryptographic-strength blob integrity assurance.
- Benchmark WASM sandbox overhead vs. native dispatch for 1 M lightweight
  kernel invocations: target < 2× overhead vs. unsandboxed CPU path.

---

## See Also

- [README.md](README.md) — Current module documentation
- [../../docs/gpu_roadmap.md](../../docs/gpu_roadmap.md) — Production-readiness
  assessment and full roadmap

---

*Last Updated: February 2026*  
*Module Version: v1.4.0*
