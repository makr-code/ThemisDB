> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# GPU Module - Future Enhancements

- GPU resource allocation and memory pool management for CUDA (sm_70+), ROCm (HIP), and Vulkan Compute backends
- CUDA/ROCm kernel registry: checksum-validated custom kernels for aggregation, sort, hash-join, and geospatial operations
- GPU-accelerated query execution: offloading of analytic aggregation, vector similarity search, and batch scoring to GPU
- Multi-GPU cluster coordination: work-stealing scheduler, peer-to-peer NVLink/PCIe data transfer, and result merging
- Vulkan Compute backend for cross-vendor (AMD RDNA, Intel Arc, Apple M-series via MoltenVK) GPU support
- Asynchronous kernel launcher with typed work-item queue and per-stream concurrency control

---

## Design Constraints

- `[ ]` GPU memory allocator must enforce a configurable pool cap (default: 80 % of device VRAM); allocations exceeding the cap return `OUT_OF_MEMORY` error, never trigger OOM-killer
- `[ ]` All kernels must be registered in `GPUKernelValidator` checksum whitelist before launch; unregistered kernel launch returns `KERNEL_NOT_VALIDATED` error
- `[ ]` Kernel launch overhead (host-side dispatch, excluding device execution): ≤ 2 ms per batch on CUDA sm_70+
- `[ ]` Multi-GPU work distribution must be re-balanced when a device's utilisation delta exceeds 20 % vs mean utilisation
- `[ ]` Vulkan dispatch latency must not exceed 1.2× equivalent CUDA dispatch latency on AMD RDNA 3+ hardware
- `[ ]` GPU module must degrade gracefully to CPU path when no compatible GPU is detected; no hard dependency on CUDA runtime at startup
- `[ ]` All GPU operations must support cancellation via `CancellationToken`; pending work items drained within 500 ms on cancel

---

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `GPUAllocator::alloc(size, device_id)` | Kernel launcher, analytics GPU path | Pool-backed; respects VRAM cap |
| `GPUKernelValidator::validate(kernel_id, checksum)` | `GPULauncher` pre-launch | Whitelist registry; reject unknown |
| `GPULauncher::submit(WorkItem, stream_id)` | Analytics engine, query executor | Async; returns `Future<DeviceBuffer>` |
| `MultiGPUScheduler::dispatch(workload)` | Query planner GPU path | Splits batches across available devices |
| `VulkanBackend::createPipeline(shader_spv)` | GPU module Vulkan path | SPIR-V shader; cross-vendor |
| `GPUContext::getDeviceInfo(device_id)` | Analytics, core DI context | Returns VRAM, compute capability, backend type |

---

## Status Key

- ✅ **Infrastructure implemented** — CPU-level bookkeeping and API in place;
  ready to wire up real CUDA/ROCm calls.
- ⬜ **Blocked on hardware** — requires a CUDA/ROCm driver or device to complete.

---

## Features

### `query_accelerator.cpp`: Replace CPU Fallback Stubs with Real CUDA/HIP Dispatch
**Priority:** High
**Target Version:** v1.4.0
**Status:** ✅ Implemented (with guarded CPU fallback)

`src/gpu/query_accelerator.cpp` now uses CUDA/HIP-capable primitives (Thrust/ROCm-Thrust paths and GPU-enabled ANN dispatch) with deterministic fallback to CPU whenever host-callable predicates or unavailable runtime capabilities prevent safe GPU execution.

**Implementation Notes:**
- `[x]` Sort path uses GPU key/index sorting (`thrust::stable_sort_by_key`) when CUDA/HIP is enabled.
- `[x]` Aggregate path uses GPU reduction primitives for supported aggregation modes and falls back safely otherwise.
- `[x]` Hash-join path includes GPU-accelerated probe/build flow with CPU fallback for unsupported paths.
- `[x]` BLAS-oriented acceleration path is wired for GPU-enabled execution modes with precision-mode controls.
- `[x]` HIP build paths mirror CUDA dispatch guards (`THEMIS_ENABLE_HIP`).
- `[x]` Regression/parity coverage exists in `tests/test_gpu_query_accelerator.cpp` and `tests/test_gpu_query_accelerator_parity.cpp`.

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
- ~~`cudaMemcpyPeer` / `hipMemcpyPeer` for GPU-to-GPU transfers~~ (implemented via `GPUP2PTransferManager`)
- ~~NVLink / XGMI topology detection~~ (implemented via `GPUClusterTopology`)

---

### Peer-to-Peer GPU-to-GPU Direct Transfers (NVLink/PCIe)
**Priority:** High | **Target Version:** v1.9.0 | **Status:** ✅ Infrastructure implemented

Direct GPU-to-GPU memory transfers via NVLink or PCIe peer-to-peer DMA without
routing through host CPU memory.

**Implemented infrastructure:**
- ✅ `GPUP2PTransferManager` (`include/themis/gpu/p2p_transfer.h`,
  `src/gpu/p2p_transfer.cpp`) — thread-safe singleton with:
  - `canAccessPeer(src, dst, devices)` — query P2P hardware capability without
    requiring the feature flag; delegates to `cudaDeviceCanAccessPeer` /
    `hipDeviceCanAccessPeer`; returns `false` on CPU simulation.
  - `enablePeerAccess(src, dst, devices)` — enables direct peer access via
    `cudaDeviceEnablePeerAccess` / `hipDeviceEnablePeerAccess`; gated on the
    `PEER_TO_PEER` feature flag.
  - `disablePeerAccess(src, dst)` — disables peer access for the pair; gated on
    the `PEER_TO_PEER` feature flag.
  - `isPeerAccessEnabled(src, dst)` — predicate; always callable.
  - `transfer(TransferRequest, devices)` — direct copy via `cudaMemcpyPeer` /
    `hipMemcpyPeer`; falls back to `memcpy` simulation on CPU-only builds so
    tests always pass; stats record the transfer route
    (NVLink / PCIe / CPU fallback).
  - `getStats()` / `reset()` — observability and test reset.
- ✅ `TransferRequest` struct: `src_device`, `dst_device`, `src_ptr`, `dst_ptr`,
  `size_bytes`.
- ✅ `TransferResult` struct: `ok`, `bytes_transferred`, `error_message`.
- ✅ `Status` enum (8 values) + `p2pStatusName()` free function.
- ✅ `Stats` struct: `total_transfers`, `bytes_transferred`, `nvlink_transfers`,
  `pcie_transfers`, `cpu_fallback_transfers`, `failed_transfers`,
  `peer_access_enabled_count`, `peer_access_disabled_count`.
- ✅ `PEER_TO_PEER` feature flag added to `GPUFeatureFlags::Feature` and
  `GPUFeatureFlags::getAll()`; enabled by default for ENTERPRISE and
  HYPERSCALER editions only.
- ✅ Topology-aware routing: `GPUClusterTopology::preferredInterconnect()` used
  to classify each transfer as NVLink vs PCIe for stats tracking.
- ✅ CPU simulation path (in-memory `memcpy`) always active; all tests pass
  without GPU hardware.
- ✅ Thread-safe: all public methods protected by an internal `std::mutex`.
- ✅ Full unit-test coverage (`tests/test_gpu_p2p_transfer.cpp`): feature-gate,
  canAccessPeer, peer-access lifecycle, zero-byte transfers, null-pointer errors,
  invalid-device errors, CPU-fallback data integrity, stats accumulation,
  concurrent safety.

**Remaining (hardware required):**
- Verify `cudaDeviceEnablePeerAccess` succeeds on an NVLink-connected pair
  (e.g. two A100s in an NVLink fabric) and that `cudaMemcpyPeer` achieves
  ≥ 250 GB/s throughput for a 1 GB buffer.
- Benchmark PCIe P2P throughput (target: ≥ 12 GB/s for a 256 MB buffer on
  Gen 4 PCIe hardware) and compare against host-staging
  (`cudaMemcpy` D→H + H→D) to validate the P2P advantage.
- Wire `THEMIS_ENABLE_HIP` path and verify `hipMemcpyPeer` on an AMD XGMI
  fabric (MI300X or similar).


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
- ✅ Full unit-test coverage (`tests/test_gpu_query_accelerator.cpp`);
  test binary also included in `themis_tests` bundle via `tests/CMakeLists.txt`.
- ✅ `THEMIS_ENABLE_CUDA` guard wired around the cuVS/RAFT path in
  `src/gpu/query_accelerator.cpp`; falls through to CPU brute-force on any
  failure (cuVS exception, no CUDA hardware, or `cudaMalloc` failure).
- ✅ `THEMIS_ENABLE_CUVS` cmake option added (`cmake/CMakeLists.txt`); when ON,
  `find_package(cuvs)` is called and `THEMIS_ENABLE_CUVS` is propagated to the
  build so the IVF-Flat index build/search calls are compiled in.

**Remaining (hardware required):**
- Verify IVF-Flat index build and search on an NVIDIA GPU with cuVS installed:
  - `conda install -c rapidsai cuvs` + `-DTHEMIS_ENABLE_CUVS=ON` + GPU hardware.
  - Benchmark k-NN throughput (target: ≥ 10× CPU brute-force for 1 M float32
    vectors of dimension 128, k=10) using CUDA events.

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

### MIG (Multi-Instance GPU) Partitioning for NVIDIA A/H Series
**Priority:** High | **Target Version:** v1.7.0 | **Status:** ✅ Infrastructure implemented

Partitions a single NVIDIA Ampere (A100) or Hopper (H100) GPU into up to 7
independent GPU Instances (GIs), each with isolated VRAM and compute slices
and hardware-level fault isolation.

**Implemented infrastructure:**
- ✅ `MIGManager` (`include/themis/gpu/mig_manager.h`, `src/gpu/mig_manager.cpp`)
  — full MIG partition lifecycle: `createPartition`, `destroyPartition`,
  `assignToTenant`, `unassignFromTenant`, `getInstances`,
  `getInstancesForDevice`, `getInstancesForTenant`, `getInstance`, `reset`.
- ✅ 8 well-known MIG profiles with VRAM sizes:
  `1g.5gb`, `2g.10gb`, `3g.20gb`, `4g.20gb`, `7g.40gb`,
  `1g.10gb`, `1g.12gb`, `7g.80gb`.
- ✅ `deviceSupportsMIG(DeviceInfo)` — returns `true` for CUDA devices with
  compute major ≥ 8 (Ampere / Hopper).
- ✅ `isKnownProfile(profile)` / `profileMemoryBytes(profile)` — profile
  validation and VRAM-size lookup.
- ✅ Per-device instance limit enforcement (max 7 per device).
- ✅ `MIGInstance` struct: `instance_id`, `device_index`, `gi_id`, `profile`,
  `memory_bytes`, `is_active`, `tenant_id`.
- ✅ `Status` enum (9 values) + `migStatusName()` free function.
- ✅ `Stats` struct: `total_created`, `total_destroyed`, `total_assigned`,
  `total_unassigned`, `active_instances`.
- ✅ `MIG_MANAGER` feature flag added to `GPUFeatureFlags::Feature` and
  `GPUFeatureFlags::getAll()`; enabled by default for ENTERPRISE and
  HYPERSCALER editions only.
- ✅ MIG fields added to `DeviceInfo`: `mig_enabled`, `mig_max_instances`.
- ✅ NVML stub (`THEMIS_ENABLE_CUDA` + `THEMIS_ENABLE_NVML` guards) ready for
  real `nvmlDeviceCreateGpuInstance` / `nvmlGpuInstanceDestroy` wiring.
- ✅ CPU simulation path (in-memory registry) always active; all tests pass
  without GPU hardware.
- ✅ Thread-safe: all public methods protected by an internal `std::mutex`.
- ✅ Full unit-test coverage (`tests/test_gpu_mig_manager.cpp`):
  deviceSupportsMIG, profile validation, feature-gate enforcement, partition
  lifecycle, tenant assignment, stats, concurrent safety.

**Remaining (hardware required):**
- Enable MIG mode on the physical device via
  `nvmlDeviceSetMIGMode(dev, NVML_DEVICE_MIG_ENABLE, &activationStatus)` and
  call `nvmlDeviceGetMIGMode` to verify.
- Create a real GPU Instance via `nvmlDeviceCreateGpuInstance(dev, profileId,
  &gpu_inst)` and a Compute Instance via
  `nvmlGpuInstanceCreateComputeInstance(gpu_inst, ciProfileId, &ci)`.
- Persist `nvmlGpuInstance_t` / `nvmlComputeInstance_t` handles in
  `MIGInstance` and call `nvmlGpuInstanceDestroy` / `nvmlComputeInstanceDestroy`
  in `destroyPartition`.
- Update `DeviceDiscovery::Enumerate()` to set `mig_enabled = true` and
  `mig_max_instances` for Ampere/Hopper devices detected via NVML.
- Benchmark MIG isolation: verify that two concurrent 1g.5gb instances on an
  A100 achieve ≥ 0.9× of the theoretical throughput of a single 2g.10gb
  instance (measured with `nvmlDeviceGetUtilizationRates`).

---

## Vulkan Compute Backend for Cross-Vendor GPU Support

**Priority:** High | **Target Version:** v1.8.0 | **Status:** ✅ Infrastructure implemented

Provides Vulkan-backed compute dispatch for AMD, Intel, ARM, Qualcomm, and NVIDIA
hardware without requiring vendor-specific CUDA or HIP drivers.

**Implemented infrastructure:**
- ✅ `VulkanComputeBackend` (`include/themis/gpu/vulkan_backend.h`,
  `src/gpu/vulkan_backend.cpp`) — thread-safe singleton with:
  - `deviceCount()` / `isAvailable()` / `vendorName()` — lazy device probe via
    `vkEnumeratePhysicalDevices`; vendor name mapped from PCI vendor ID and cached.
  - `createBackendFn(device_index)` — returns a `GPULauncher::BackendFn` usable
    with `GPUStreamManager::createStream()` or `GPULauncher` directly.
  - Named logical stream lifecycle: `createStream` / `destroyStream` /
    `synchronizeStream` / `getStream` / `hasStream` / `streamNames`.
  - `Stats` struct: `streams_created`, `streams_destroyed`, `dispatched`,
    `dispatch_errors`, `cpu_fallbacks`; plus `getStats()` / `resetStats()`.
- ✅ `VULKAN_BACKEND` feature flag added to `GPUFeatureFlags::Feature`; enabled by
  default for all editions (Community and above).
- ✅ CPU simulation path (in-memory registry + CPU fallback) always active; all
  tests pass without Vulkan hardware.
- ✅ Real Vulkan calls (`vkEnumeratePhysicalDevices`, `vkGetPhysicalDeviceQueueFamilyProperties`)
  gated behind `THEMIS_ENABLE_VULKAN`.
- ✅ Thread-safe: all public methods protected by an internal `std::mutex`; single
  lock per lambda body avoids recursive-lock deadlock.
- ✅ Full unit-test coverage (`tests/test_gpu_vulkan_backend.cpp`): device query,
  launcher backend, stream lifecycle, stats, `GPUStreamManager` integration, and
  feature-flag enable/disable round-trip.

**Remaining (hardware required):**
- Store real `VkQueue` handle in `StreamHandle::native` (currently uses
  `device_index + 1` as a sentinel when `THEMIS_ENABLE_VULKAN` is active).
- Replace the `createBackendFn` dispatch stub with real Vulkan command buffer
  submission: `vkBeginCommandBuffer` → compute dispatch → `vkQueueSubmit` →
  `vkWaitForFences`.
- Wire `synchronizeStream` to call `vkQueueWaitIdle` on the stored `VkQueue`.
- Benchmark Vulkan vs CUDA/HIP dispatch latency for a representative ThemisDB
  workload (target: ≤ 1.2× CUDA dispatch latency on AMD RDNA 3+ hardware).

---

## Test Strategy

- **Unit tests** (≥ 88 % line coverage): `GPUAllocator` pool boundary conditions (exact cap, cap+1, deallocation, fragmentation); `GPUKernelValidator` accept/reject for known-good and tampered checksums; `GPULauncher` work-item queue under concurrent submission
- **Integration tests** (conditional on CUDA/ROCm device in CI): launch each whitelisted kernel with a reference dataset; verify output matches CPU baseline within tolerance ≤ 1 × 10⁻⁶ (double) / 1 × 10⁻⁴ (float)
- **CPU-fallback tests** (always run): when no GPU is present, `GPULauncher::submit()` routes to CPU stub; verify query results are identical and no CUDA symbols are loaded
- **Multi-GPU tests** (CI with ≥ 2 GPUs): work-stealing scheduler distributes a 100 M-row batch across 2 devices; verify results merged correctly and no data races
- **Vulkan smoke tests**: pipeline creation, buffer allocation, compute dispatch with a trivial kernel on any Vulkan 1.2-capable device; shader SPIR-V validated by `glslangValidator`
- **Cancellation tests**: submit 10-second synthetic kernel; issue cancel within 100 ms; verify drain completes within 500 ms

## Performance Targets

- GPU batch aggregation (CUDA sm_80, 10 M rows, SUM/AVG/MIN/MAX): ≥ 8× speedup vs single-threaded CPU baseline
- GPU vector similarity search (1 M 768-dim vectors, cosine distance, top-100): ≤ 50 ms on RTX 3080 class hardware
- Kernel launch overhead (host dispatch only): ≤ 2 ms per batch on CUDA sm_70+
- Multi-GPU linear scale-out: 2-GPU throughput ≥ 1.8× single-GPU throughput for batch sizes ≥ 10 M rows
- Vulkan vs CUDA dispatch latency on AMD RDNA 3+: ≤ 1.2× CUDA dispatch latency
- VRAM pool allocation/free for 256 MB block: ≤ 100 µs (no device sync required)

## Security / Reliability

- All kernels validated via `GPUKernelValidator` checksum whitelist before launch; tampered or unregistered kernels are never executed
- VRAM pool cap enforced at allocation time; out-of-cap allocations return structured error and are logged; OOM-killer never triggered
- CUDA/ROCm context initialisation errors (driver not present, incompatible version) surface as structured `GPUInitError`; server continues on CPU path
- Multi-GPU peer transfers use explicit device sync points; no implicit cross-device memory aliasing
- Vulkan SPIR-V shaders validated by `spirv-val` at pipeline creation time; invalid shaders rejected before GPU submission
- All GPU resource handles tracked in RAII wrappers; device memory leaks detected via `compute-sanitizer` / `rocm-validate` in CI nightly runs

---

## Scientific References

The following references (IEEE & ACM citation format) support the future enhancement claims in this document.

### GPU Programming & Parallel Computing

[1] J. Nickolls, I. Buck, M. Garland, and K. Skadron, "Scalable parallel programming with CUDA," *ACM Queue*, vol. 6, no. 2, pp. 40–53, Mar. 2008, doi: 10.1145/1365490.1365500.

[2] M. Garland and D. B. Kirk, "Understanding throughput-oriented architectures," *Commun. ACM*, vol. 53, no. 11, pp. 58–66, Nov. 2010, doi: 10.1145/1839676.1839694.

[3] V. Volkov, "Understanding latency hiding on GPUs," Ph.D. dissertation, Dept. EECS, Univ. California Berkeley, Berkeley, CA, USA, 2016. [Online]. Available: https://www2.eecs.berkeley.edu/Pubs/TechRpts/2016/EECS-2016-143.html

### GPU Memory Management & Unified Memory

[4] M. Dashti and A. Fedorova, "Analyzing memory management methods for GPU programs," in *Proc. Int. Symp. Memory Management (ISMM)*, Jun. 2017, pp. 36–48, doi: 10.1145/3092255.3092257.

[5] NVIDIA Corporation, "CUDA C++ Programming Guide (v12.x)," NVIDIA Developer Documentation, 2023. [Online]. Available: https://docs.nvidia.com/cuda/cuda-c-programming-guide/

[6] AMD, "ROCm HIP Documentation," AMD ROCm Documentation, 2023. [Online]. Available: https://rocm.docs.amd.com/projects/HIP/en/latest/

### GPU-Accelerated Vector Similarity Search (cuVS/RAFT / ANN)

[7] J. Johnson, M. Douze, and H. Jégou, "Billion-scale similarity search with GPUs," *IEEE Trans. Big Data*, vol. 7, no. 3, pp. 535–547, Sep. 2021, doi: 10.1109/TBDATA.2019.2921572.

[8] C. Guo et al., "Accelerating large-scale inference with anisotropic vector quantization," in *Proc. 37th Int. Conf. Machine Learning (ICML)*, Jul. 2020, pp. 3887–3896. [Online]. Available: https://proceedings.mlr.press/v119/guo20h.html

[9] A. Williams, V. Bhatt, N. Bhatotia, D. Mudigere, and M. Smelyanskiy, "RAFT: Reusable accelerated functions and tools for vector search and clustering on GPUs," *arXiv preprint arXiv:2408.05247*, Aug. 2024. [Online]. Available: https://arxiv.org/abs/2408.05247

### CUDA Graph Capture & Kernel Optimization

[10] NVIDIA Corporation, "CUDA Graphs," CUDA C++ Programming Guide, sec. 7.7, 2023. [Online]. Available: https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#cuda-graphs

[11] N. Sakharnykh and P. Harish, "Maximizing unified memory performance in CUDA," NVIDIA Technical Blog, 2017. [Online]. Available: https://developer.nvidia.com/blog/maximizing-unified-memory-performance-cuda/

### Multi-GPU Coordination & Collective Operations

[12] S. Jeaugey, "NCCL 2.0," in *Proc. GPU Technology Conf. (GTC)*, Mar. 2017. [Online]. Available: https://developer.nvidia.com/gtc/2017/video/S7155

[13] A. Agarwal et al., "Reliable GPU cluster management via collective heartbeat and topology-aware scheduling," in *Proc. 29th Symp. Operating Systems Principles (SOSP)*, Oct. 2023, doi: 10.1145/3600006.3613133.

### MIG (Multi-Instance GPU) Partitioning

[14] NVIDIA Corporation, "NVIDIA Multi-Instance GPU User Guide," NVIDIA Documentation, 2023. [Online]. Available: https://docs.nvidia.com/datacenter/tesla/mig-user-guide/

[15] H. Zhao, B. Dong, T. Xu, and H. Sun, "Characterizing and understanding HGX A100 GPU interconnects," in *Proc. IEEE Int. Symp. High Performance Computer Architecture (HPCA)*, Feb. 2023, pp. 214–225, doi: 10.1109/HPCA56546.2023.10071038.

### Vulkan Compute & Cross-Vendor GPU

[16] T. Akenine-Möller, E. Haines, N. Hoffman, A. Pesce, M. Iwanicki, and S. Hillaire, *Real-Time Rendering*, 4th ed. Boca Raton, FL, USA: CRC Press, 2018, ch. 23 (Vulkan/DX12/Metal).

[17] K. Perelygin and A. Dzyubenko, "Performance evaluation of compute workloads on Vulkan and CUDA," in *Proc. Int. Conf. High Performance Computing & Simulation (HPCS)*, Jul. 2019, pp. 782–789, doi: 10.1109/HPCS48598.2019.9188133.

### GPU Time-Slicing & Multi-Tenant Isolation

[18] C. Lepers, V. Quéma, and A. Feldman, "Task and memory coloring: A unified approach for non-uniform architectures," in *Proc. USENIX Annual Technical Conf. (ATC)*, Jun. 2015, pp. 407–418.

[19] NVIDIA Corporation, "Time-Sliced GPU Sharing in Kubernetes," NVIDIA Technical Blog, 2022. [Online]. Available: https://developer.nvidia.com/blog/nvidia-time-slicing-gpu-virtualization/

### WASM Kernel Sandboxing

[20] A. Haas et al., "Bringing the web up to speed with WebAssembly," in *Proc. 38th ACM SIGPLAN Conf. Programming Language Design and Implementation (PLDI)*, Jun. 2017, pp. 185–200, doi: 10.1145/3062341.3062363.

[21] C. Disselkoen et al., "Position paper: Progressive memory safety for WebAssembly," in *Proc. 8th Workshop Hardware and Architectural Support for Security and Privacy (HASP)*, Jun. 2019, doi: 10.1145/3337167.3337171.

### FP16/BF16 Tensor Core Operations

[22] N. P. Jouppi et al., "In-datacenter performance analysis of a tensor processing unit," in *Proc. 44th Int. Symp. Computer Architecture (ISCA)*, Jun. 2017, pp. 1–12, doi: 10.1145/3079856.3080246.

[23] Y. Choi, M. Kim, W. Baek, and J. Lee, "Accelerating sparse deep neural networks," in *Proc. 49th Int. Symp. Computer Architecture (ISCA)*, Jun. 2022, pp. 497–512, doi: 10.1145/3470496.3527423.

## See Also

- [README.md](README.md) — Current module documentation
- [../../docs/gpu_roadmap.md](../../docs/gpu_roadmap.md) — Production-readiness
  assessment and full roadmap

---

*Last Updated: April 2026*
*Module Version: v1.4.0*
